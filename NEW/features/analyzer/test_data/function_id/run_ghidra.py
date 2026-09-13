#!/usr/bin/env python3
"""Build a real fixture FID database and extract Function ID analyzer output."""

from __future__ import annotations

import os
import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Function ID"
FID_ARTIFACT_GHIDRA_VERSION = "12.1.3"


def value(address) -> int:
    """Return an image-relative address offset."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Close the saved analyzed target through the GhidraProject lifecycle API."""
    if program is not None:
        project.close(program)
    else:
        project.close()


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes without submitting unrelated analysis work."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    executable = program.getMemory().getExecuteSet()
    transaction = program.startTransaction("Prepare Function ID fixture")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def seed_export_functions(program) -> None:
    """Create only the real PE-exported functions required by FID hashing."""
    from ghidra.app.cmd.function import CreateFunctionCmd

    iterator = program.getSymbolTable().getExternalEntryPointIterator()
    while iterator.hasNext():
        address = iterator.next()
        command = CreateFunctionCmd(address)
        if not command.applyTo(program):
            raise RuntimeError(f"Could not seed exported function at {address}")


def build_fid_database(project, library_name: str, fid_path: Path) -> tuple[int, str]:
    """Populate a packed FID database through Ghidra's production FID APIs."""
    from ghidra.feature.fid.db import FidFileManager
    from ghidra.feature.fid.service import FidService
    from ghidra.util.task import TaskMonitor
    from java.io import File
    from java.util import ArrayList

    temporary_fd, temporary_name = tempfile.mkstemp(
        prefix=f".{fid_path.stem}.", suffix=fid_path.suffix, dir=str(fid_path.parent)
    )
    os.close(temporary_fd)
    temporary_path = Path(temporary_name)
    temporary_path.unlink()
    manager = FidFileManager.getInstance()
    fid_file = None
    database = None
    replacement_committed = False
    try:
        # Build beside the committed artifact and replace it only after the
        # complete database has been populated, saved, and re-opened.
        manager.createNewFidDatabase(File(str(temporary_path)))
        fid_file = manager.addUserFidFile(File(str(temporary_path)))
        if fid_file is None:
            raise RuntimeError("Ghidra rejected the newly created packed FID database")

        library = project.openProgram("/", library_name, False)
        prepare_disassembly(library)
        seed_export_functions(library)
        project.save(library)
        language_id = str(library.getLanguageID())
        # Current repository sources use FidFilter; the verified 12.1.3 runtime
        # predates that class and exposes the equivalent LanguageID overload.
        try:
            from ghidra.feature.fid.db import FidFilter
            filter_argument = FidFilter(language_id, "", "")
        except ImportError:
            from ghidra.program.model.lang import LanguageID
            filter_argument = LanguageID(language_id)
        domain_file = library.getDomainFile()
        project.close(library)

        program_files = ArrayList()
        program_files.add(domain_file)
        database = fid_file.getFidDB(True)
        service = FidService()
        result = service.createNewLibraryFromPrograms(
            database,
            "Analyzer Fixture Library",
            "1.0",
            "x64",
            program_files,
            None,
            filter_argument,
            None,
            None,
            TaskMonitor.DUMMY,
        )
        if result is None:
            raise RuntimeError("FidService did not create a library record")
        database.saveDatabase("Saving analyzer fixture FID database", TaskMonitor.DUMMY)
        libraries = database.getAllLibraries()
        if not libraries:
            raise RuntimeError("FID database population produced no library records")
        added_count = int(result.getTotalAdded())
        database.close()
        database = None

        # FidFile caches its language filter when first opened. Re-register the
        # populated temporary file, then atomically replace the committed path.
        manager.removeUserFile(fid_file)
        fid_file = None
        committed_file = File(str(fid_path))
        for existing in list(manager.getUserAddedFiles()):
            if Path(existing.getPath()).resolve() == fid_path.resolve():
                manager.removeUserFile(existing)
        os.replace(temporary_path, fid_path)
        if manager.addUserFidFile(committed_file) is None:
            raise RuntimeError("Ghidra could not reload the populated packed FID database")
        replacement_committed = True
        return added_count, language_id
    finally:
        if database is not None:
            database.close()
        if fid_file is not None:
            manager.removeUserFile(fid_file)
        if not replacement_committed and temporary_path.exists():
            temporary_path.unlink()


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and explicitly enable Function ID."""
    from ghidra.framework.options import OptionType

    analysis_options = project.getAnalysisOptions(program)
    for name in list(analysis_options.getOptionNames()):
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE:
            analysis_options.setBoolean(name, str(name) == ANALYZER_NAME)
    # The default score and multi-match thresholds are intentionally retained.
    fid_options = analysis_options.getOptions(ANALYZER_NAME)
    fid_options.setBoolean("Always Apply FID Labels", True)
    fid_options.setBoolean("Create Analysis Bookmarks", True)
    enabled = sorted(
        str(name)
        for name in analysis_options.getOptionNames()
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE
        and analysis_options.getBoolean(name, False)
    )
    expected = [
        ANALYZER_NAME,
        f"{ANALYZER_NAME}.Always Apply FID Labels",
        f"{ANALYZER_NAME}.Create Analysis Bookmarks",
    ]
    if enabled != sorted(expected):
        raise RuntimeError(f"Unexpected enabled Function ID options: {enabled}")
    return enabled


def target_rows(program):
    """Extract names, comments, and no assumptions from every target function."""
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        comment = function.getComment()
        rows.append((value(function.getEntryPoint()), str(function.getName()), str(comment) if comment else ""))
    return sorted(rows)


def bookmark_rows(program):
    """Extract only Function ID analyzer bookmarks from the saved listing."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getCategory()) == "Function ID Analyzer":
            rows.append((value(bookmark.getAddress()), str(bookmark.getComment())))
    return sorted(rows)


def query_match_count(program) -> tuple[int, int]:
    """Query the generated database through the installed FidService API."""
    from ghidra.feature.fid.service import FidService
    from ghidra.util.task import TaskMonitor

    service = FidService()
    query_service = service.openFidQueryService(program.getLanguage(), False)
    try:
        results = service.processProgram(program, query_service, service.getDefaultScoreThreshold(), TaskMonitor.DUMMY)
        return len(results), sum(len(result.matches) for result in results)
    finally:
        query_service.close()


def report(input_path: Path, enabled, added_count: int, language_id: str, query_counts, functions, bookmarks) -> str:
    """Render actual FID population and target markup observations."""
    lines = [
        "# Function ID Behavioral Fixture",
        "",
        "> Generated from a real packed FID database and the saved target program with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **Target:** `{input_path.name}`",
        f"- **Target file size:** `{input_path.stat().st_size}` bytes",
        f"- **FID artifact version:** `Ghidra {FID_ARTIFACT_GHIDRA_VERSION}`",
        f"- **FID language ID:** `{language_id}`",
        f"- **Direct FidService query results:** `{query_counts[0]}` functions, `{query_counts[1]}` matches",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean option |",
        "| --- |",
        *[f"| `{name}` |" for name in enabled],
        "",
        "## Target Functions After Function ID",
        "",
        "| Entry offset | Name | Plate comment |",
        "| --- | --- | --- |",
    ]
    for address, name, comment in functions:
        lines.append(f"| `0x{address:016X}` | `{name}` | `{comment.replace(chr(10), '<br>')}` |")
    lines.extend(["", "## Function ID Bookmarks", "", "| Entry offset | Comment |", "| --- | --- |"])
    lines.extend(f"| `0x{address:016X}` | `{comment}` |" for address, comment in bookmarks)
    lines.extend(["", "## Fixture Assertions", "", f"- **Functions added to the generated FID library:** `{added_count}`.", f"- **Target functions observed:** `{len(functions)}`.", f"- **Function ID bookmarks:** `{len(bookmarks)}`.", ""])
    return "\n".join(lines)


def main() -> int:
    """Import both PE artifacts, populate FID, reopen/analyze the target, and save."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_function_id.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_function_id.md"
    library_path = fixture_dir / "test_function_id_library.exe"
    fid_path = fixture_dir / "test_function_id.fidb"
    if not input_path.is_file() or not library_path.is_file():
        print(f"Target or library executable is missing: {input_path}, {library_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_function_id_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "function_id", True)
        target_imported = project.importProgram(File(str(input_path)))
        library_imported = project.importProgram(File(str(library_path)))
        if target_imported is None or library_imported is None:
            raise RuntimeError("Ghidra failed to import the target or library executable")
        project.saveAs(target_imported, "/", input_path.name, True)
        project.saveAs(library_imported, "/", library_path.name, True)
        project.close(target_imported)
        project.close(library_imported)
        added_count, language_id = build_fid_database(project, library_path.name, fid_path)
        program = project.openProgram("/", input_path.name, False)
        prepare_disassembly(program)
        seed_export_functions(program)
        enabled = configure_analysis(project, program)
        project.analyze(program)
        # Exercise the same registered analyzer action explicitly after the
        # FID file has been reloaded, so the generated database is observable in
        # ApplyFidEntriesCommand's normal markup path as well as in the query.
        from ghidra.feature.fid.analyzer import FidAnalyzer
        from ghidra.app.util.importer import MessageLog
        from ghidra.util.task import TaskMonitor
        fid_analyzer = FidAnalyzer()
        fid_analyzer.optionsChanged(
            project.getAnalysisOptions(program).getOptions(ANALYZER_NAME), program
        )
        fid_analyzer.added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())
        functions = target_rows(program)
        query_counts = query_match_count(program)
        bookmarks = bookmark_rows(program)
        project.save(program)
        output_path.write_text(report(input_path, enabled, added_count, language_id, query_counts, functions, bookmarks), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
