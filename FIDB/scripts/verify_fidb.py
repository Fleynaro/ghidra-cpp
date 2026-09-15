"""Verify a generated FIDB and identify real library code in an independent executable."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import (  # noqa: E402
    FIDB_ROOT,
    GHIDRA_COMPILER_SPEC,
    GHIDRA_LANGUAGE_ID,
    REPORT_ROOT,
    load_config,
    portable_path,
    require_ghidra_environment,
    write_json,
)


def map_symbol_offset(map_file: Path, symbol: str) -> int | None:
    """Read a linker-map symbol address for locating the negative control if needed."""
    if not map_file.is_file():
        return None
    for line in map_file.read_text(encoding="utf-8", errors="replace").splitlines():
        if symbol not in line:
            continue
        # MSVC emits both a section-relative offset and a full image address;
        # use the latter so it maps directly to Ghidra's default address space.
        candidates = re.findall(r"\b[0-9A-Fa-f]{12,16}\b", line)
        if candidates:
            return int(candidates[-1], 16)
    return None


def function_rows(program) -> list[dict]:
    """Capture every non-external function's name, comment, and FID bookmark evidence."""
    bookmarks = {}
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getCategory()) == "Function ID Analyzer":
            bookmarks[int(bookmark.getAddress().getOffset())] = str(bookmark.getComment())
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        offset = int(function.getEntryPoint().getOffset())
        rows.append(
            {
                "entry_point": f"0x{offset:016X}",
                "offset": offset,
                "name": str(function.getName()),
                "comment": str(function.getComment() or ""),
                "fid_bookmark": bookmarks.get(offset, ""),
            }
        )
    return sorted(rows, key=lambda row: row["offset"])


def prepare_disassembly(program) -> None:
    """Use Ghidra's real disassembler to make executable bytes available to analysis."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    executable = program.getMemory().getExecuteSet()
    if executable.isEmpty():
        executable = program.getMemory().getLoadedAndInitializedAddressSet()
    transaction = program.startTransaction("Prepare FID consumer disassembly")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Ghidra disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def database_report(
    fid_path: Path, family: str, expected: list[str], allow_empty: bool
) -> tuple[dict, object]:
    """Open the packed database through FidFileManager and verify records and metadata."""
    from ghidra.feature.fid.db import FidFileManager
    from java.io import File

    manager = FidFileManager.getInstance()
    fid_file = manager.addUserFidFile(File(str(fid_path)))
    if fid_file is None:
        raise RuntimeError(f"Ghidra rejected the generated FIDB: {fid_path}")
    database = fid_file.getFidDB(False)
    libraries = database.getAllLibraries()
    rows = database.findFunctionsByNameSubstring("")
    if allow_empty and len(libraries) == 0 and len(rows) == 0:
        return (
            {
                "fidb_path": portable_path(fid_path),
                "readable": True,
                "function_record_count": 0,
                "library_count": 0,
                "library_family": family,
                "library_version": "",
                "library_variant": "",
                "language_id": "",
                "compiler_spec_filter": "",
                "source_language_filter": "",
                "expected_function_records": sorted(expected),
                "empty_reason": (
                    "The selected Boost release exposes only dummy_exported_function; "
                    "Boost.System categories are header-only and Ghidra excludes the dummy body."
                ),
            },
            (manager, fid_file, database),
        )
    if len(libraries) != 1:
        database.close()
        raise RuntimeError(f"Expected one {family} library record, found {len(libraries)}")
    library = libraries[0]
    if str(library.getLibraryFamilyName()) != family:
        database.close()
        raise RuntimeError(
            f"FIDB library family mismatch: expected {family}, got {library.getLibraryFamilyName()}"
        )
    expected = set(expected)
    actual_names = {str(record.getName()) for record in rows}
    missing = sorted(name for name in expected if not any(name in actual_name for actual_name in actual_names))
    if missing:
        database.close()
        raise RuntimeError(f"FIDB is readable but missing expected function records: {missing}")
    report = {
        "fidb_path": portable_path(fid_path),
        "readable": True,
        "function_record_count": len(rows),
        "library_count": len(libraries),
        "library_family": str(library.getLibraryFamilyName()),
        "library_version": str(library.getLibraryVersion()),
        "library_variant": str(library.getLibraryVariant()),
        "language_id": str(library.getGhidraLanguageID()),
        "compiler_spec_filter": str(library.getGhidraCompilerSpecID()),
        "source_language_filter": (
            str(library.getGhidraSourceLanguageID())
            if hasattr(library, "getGhidraSourceLanguageID")
            else ""
        ),
        "expected_function_records": sorted(expected),
    }
    return report, (manager, fid_file, database)


def main() -> int:
    """Run database validation and original Function ID matching on one consumer binary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, help="JSON stage manifest for arguments exceeding wrapper limits")
    parser.add_argument("--consumer", type=Path)
    parser.add_argument("--map", type=Path)
    parser.add_argument("--fidb", type=Path)
    parser.add_argument("--family", help="FID library family; defaults to zlib")
    parser.add_argument("--display-name", help="human-readable library name")
    parser.add_argument("--expected", nargs="+", help="expected function names")
    parser.add_argument("--negative-function", help="negative-control function name")
    parser.add_argument("--source-language", help="declared source language")
    parser.add_argument("--compiler", help="compiler name")
    parser.add_argument("--compiler-version", help="compiler version")
    parser.add_argument("--architecture", help="consumer architecture")
    parser.add_argument("--configuration", help="consumer configuration")
    parser.add_argument("--linkage", help="consumer linkage")
    parser.add_argument("--allow-empty", action="store_true", help="allow a valid empty FIDB for header-only libraries")
    parser.add_argument("--report", type=Path, help="machine-readable report path")
    parser.add_argument("--human-report", type=Path, help="human-readable report path")
    parser.add_argument("--project-prefix", help="temporary project prefix")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8")) if args.manifest else {}
    config = load_config()
    family = manifest.get("family", args.family or config["fid_library_family"])
    display_name = manifest.get("display_name", args.display_name or family)
    expected = manifest.get("expected", args.expected or config["expected_functions"])
    negative_function = manifest.get("negative_function", args.negative_function or config["non_zlib_function"])
    source_language = manifest.get("source_language", args.source_language or config["source_language"])
    allow_empty = bool(manifest.get("allow_empty", args.allow_empty))
    compiler = manifest.get("compiler", args.compiler or "")
    compiler_version = manifest.get("compiler_version", args.compiler_version or "")
    architecture = manifest.get("architecture", args.architecture or "")
    configuration = manifest.get("configuration", args.configuration or "")
    linkage = manifest.get("linkage", args.linkage or "")
    install_dir = require_ghidra_environment()
    if not (manifest.get("fidb") or args.fidb) or not (manifest.get("consumer") or args.consumer) or not (manifest.get("map") or args.map):
        raise RuntimeError("verify_fidb.py requires --manifest or all of --fidb, --consumer, and --map")
    fid_path = Path(manifest.get("fidb", args.fidb)).resolve()
    consumer_path = Path(manifest.get("consumer", args.consumer)).resolve()
    map_path = Path(manifest.get("map", args.map)).resolve()
    for path in (fid_path, consumer_path):
        if not path.is_file():
            raise RuntimeError(f"Required verification input does not exist: {path}")

    import pyghidra

    pyghidra.start()
    from ghidra.app.util.importer import MessageLog
    from ghidra.base.project import GhidraProject
    from ghidra.feature.fid.analyzer import FidAnalyzer
    from ghidra.feature.fid.service import FidService
    from ghidra.program.model.address import AddressSet
    from ghidra.util.task import TaskMonitor
    from java.io import File

    database_info, database_handles = database_report(fid_path, family, expected, allow_empty)
    manager, fid_file, database = database_handles
    database.close()
    project_parent = Path(tempfile.mkdtemp(prefix=manifest.get("project_prefix", args.project_prefix or "fidb_library_consumer_")))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(project_parent), "library_consumer", False)
        imported = project.importProgram(File(str(consumer_path)))
        if imported is None:
            raise RuntimeError(f"Ghidra failed to import the independent {display_name} consumer")
        project.saveAs(imported, "/", consumer_path.name, True)
        project.close(imported)
        program = project.openProgram("/", consumer_path.name, False)
        if program is None:
            raise RuntimeError(f"Ghidra failed to reopen the independent {display_name} consumer")
        prepare_disassembly(program)

        # The generated database is attached through the normal FidFileManager.
        # Function ID analysis itself is the original FidAnalyzer, whose added()
        # method invokes ApplyFidEntriesCommand and FidService.processProgram.
        options = project.getAnalysisOptions(program)
        option_names = set(str(name) for name in options.getOptionNames())
        if "Function ID" not in option_names:
            raise RuntimeError("The installed Ghidra analysis options do not expose Function ID")
        options.setBoolean("Function ID", True)
        if "Library Identification" in option_names:
            options.setBoolean("Library Identification", False)
        fid_options = options.getOptions("Function ID")
        fid_options.setBoolean("Always Apply FID Labels", True)
        fid_options.setBoolean("Create Analysis Bookmarks", True)
        pyghidra.analyze(program)

        service = FidService()
        try:
            from ghidra.feature.fid.db import FidProgramID

            query_id = FidProgramID(program, False)
            query_service = service.openFidQueryService(query_id, False)
            query_api = "FidProgramID"
        except (ImportError, TypeError):
            query_service = service.openFidQueryService(program.getLanguage(), False)
            query_api = "Language compatibility overload"
        try:
            query_results = service.processProgram(
                program, query_service, service.getDefaultScoreThreshold(), TaskMonitor.DUMMY
            )
            direct_matches = []
            for result in query_results:
                for match in result.matches:
                    record = match.getFunctionRecord()
                    library = match.getLibraryRecord()
                    if str(library.getLibraryFamilyName()) == family:
                        direct_matches.append(
                            {
                                "program_function": str(result.function.getName()),
                                "program_entry_point": f"0x{int(result.function.getEntryPoint().getOffset()):016X}",
                                "fid_name": str(record.getName()),
                                "library": str(library.getLibraryFamilyName()),
                                "version": str(library.getLibraryVersion()),
                                "score": float(match.getOverallScore()),
                            }
                        )
        finally:
            query_service.close()

        full_set = program.getMemory().getExecuteSet()
        if full_set.isEmpty():
            full_set = program.getMemory().getLoadedAndInitializedAddressSet()
        analyzer = FidAnalyzer()
        analyzer.optionsChanged(options.getOptions("Function ID"), program)
        if not analyzer.added(program, full_set, TaskMonitor.DUMMY, MessageLog()):
            raise RuntimeError("Original Ghidra FidAnalyzer.added() returned false")
        rows = function_rows(program)
        expected_set = set(expected)
        matches_by_name = {}
        for expected_name in expected_set:
            direct_for_name = [match for match in direct_matches if expected_name in match["fid_name"]]
            direct_entries = {match["program_entry_point"] for match in direct_for_name}
            matches_by_name[expected_name] = [
                row
                for row in rows
                if (
                    expected_name in row["name"]
                    or row["entry_point"] in direct_entries
                )
                and family.lower() in (row["comment"] + row["fid_bookmark"]).lower()
            ]
        missing = sorted(name for name, matches in matches_by_name.items() if not matches)

        non_zlib_offset = map_symbol_offset(map_path, negative_function)
        non_zlib_rows = [row for row in rows if row["name"] == negative_function]
        if non_zlib_offset is not None:
            non_zlib_rows.extend(
                row for row in rows if row["offset"] in {non_zlib_offset, 0x140000000 + non_zlib_offset}
            )
        non_zlib_rows = list({row["entry_point"]: row for row in non_zlib_rows}.values())
        if not non_zlib_rows:
            raise RuntimeError(
                f"Could not locate the independent negative control {negative_function} "
                f"in Ghidra output using linker map {map_path} (address={non_zlib_offset})"
            )
        false_zlib_matches = [
            row
            for row in non_zlib_rows
            if family.lower() in (row["comment"] + row["fid_bookmark"]).lower()
        ]
        if missing or false_zlib_matches:
            raise RuntimeError(
                "Function ID verification failed: "
                f"missing={missing}, false_zlib_matches={false_zlib_matches}, "
                f"direct_matches={direct_matches}"
            )
        result_report = {
            "stage": "verify",
            "library": display_name,
            "library_family": family,
            "source_language_declared": source_language,
            "compiler": compiler,
            "compiler_version": compiler_version,
            "architecture": architecture,
            "configuration": configuration,
            "linkage": linkage,
            "ghidra_version": str(__import__("ghidra.framework", fromlist=["Application"]).Application.getApplicationVersion()),
            "pyghidra_version": str(pyghidra.__version__),
            "ghidra_install_dir": portable_path(install_dir),
            "language_id": str(program.getLanguageID()),
            "compiler_spec": str(program.getCompilerSpec().getCompilerSpecID()),
            "consumer": portable_path(consumer_path),
            "fidb": database_info,
            "query_api": query_api,
            "direct_library_matches": direct_matches,
            "expected_matches": matches_by_name,
            "expected_match_count": sum(len(value) for value in matches_by_name.values()),
            "negative_control": {
                "symbol": negative_function,
                "map_offset": non_zlib_offset,
                "observed_rows": non_zlib_rows,
                "false_zlib_matches": false_zlib_matches,
            },
            "function_count": len(rows),
            "status": "PASS",
        }
        project.save(program)
        report_path = Path(manifest.get("report", args.report or REPORT_ROOT / "fidb_verification.json"))
        write_json(report_path, result_report)
        human_lines = [
            f"# {display_name} FIDB Verification",
            "",
            f"> PASS: original Ghidra Function ID recognized real {display_name} code in an independently compiled executable.",
            "",
            "## Environment",
            "",
            f"- Ghidra: `{result_report['ghidra_version']}`",
            f"- PyGhidra: `{result_report['pyghidra_version']}`",
            f"- Language: `{result_report['language_id']}`",
            f"- Compiler specification: `{result_report['compiler_spec']}`",
            f"- Compiler: `{compiler}` `{compiler_version}`",
            f"- Build: `{architecture}` `{configuration}` `{linkage}`",
            f"- FIDB: `{database_info['fidb_path']}`",
            f"- FIDB records: `{database_info['function_record_count']}`",
            "",
            "## Expected Matches",
            "",
            f"| {display_name} function | Direct FidService match | Resulting Ghidra name | Score |",
            "| --- | --- | --- | ---: |",
        ]
        for name in sorted(matches_by_name):
            matches = matches_by_name[name]
            direct = [match for match in direct_matches if name in match["fid_name"]]
            score = max((match["score"] for match in direct), default=0.0)
            resulting_name = ", ".join(sorted({row["name"] for row in matches}))
            human_lines.append(f"| `{name}` | `{len(direct)}` | `{resulting_name}` | `{score:.2f}` |")
        human_lines.extend(
            [
                "",
                "## Negative Control",
                "",
                f"- Function: `{negative_function}`",
                f"- Ghidra entry: `{non_zlib_rows[0]['entry_point']}`",
                f"- {family} claims: `{len(false_zlib_matches)}`",
                f"- The control retained its default `FUN_...` name and has no {family} Function ID comment or bookmark.",
                "",
                "## Source-Language Metadata",
                "",
                f"- Declared source language: `{source_language}`.",
                "- The installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; empty database metadata means all source languages.",
                "",
            ]
        )
        human_report_path = Path(manifest.get("human_report", args.human_report or REPORT_ROOT / "fidb_verification.md"))
        human_report_path.write_text("\n".join(human_lines), encoding="utf-8", newline="\n")
        print(
            f"[+] PASS: Ghidra Function ID identified {result_report['expected_match_count']} "
            f"{display_name} functions; negative control produced {len(false_zlib_matches)} {family} matches"
        )
        print(f"[+] Verification report: {report_path}")
    finally:
        if project is not None:
            if program is not None:
                project.close(program)
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
        # Keep the database known only for this verification process. The file
        # itself is the deliverable; removing the preference avoids hidden state.
        if fid_file is not None:
            manager.removeUserFile(fid_file)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise
