"""Import and analyze zlib object files using the official FID prescript behavior."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import (  # noqa: E402
    BUILD_ROOT,
    FIDB_ROOT,
    GHIDRA_COMPILER_SPEC,
    GHIDRA_LANGUAGE_ID,
    REPORT_ROOT,
    ZLIB_BUILD_ROOT,
    load_config,
    portable_path,
    require_ghidra_environment,
    write_json,
)


def function_rows(program) -> list[dict]:
    """Extract analyzed function names and symbol provenance for the report."""
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        symbol = function.getSymbol()
        rows.append(
            {
                "entry_point": f"0x{int(function.getEntryPoint().getOffset()):016X}",
                "name": str(function.getName()),
                "symbol_source": str(symbol.getSource()),
                "thunk": bool(function.isThunk()),
            }
        )
    return rows


def source_language_ids(program) -> list[str]:
    """Read source-language metadata across the installed and repository APIs."""
    try:
        return [str(item) for item in program.getSourceLanguageIDs()]
    except AttributeError:
        # The installed 12.1.3 runtime used for this project predates the
        # repository's default Program.getSourceLanguageIDs() method. Its
        # Program Information option stores the same comma-separated IDs.
        from ghidra.program.model.listing import Program

        raw = program.getOptions(Program.PROGRAM_INFO).getString("Source Languages", "")
        return [item.strip() for item in str(raw).split(",") if item.strip()]


def main() -> int:
    """Create a persistent Ghidra project containing analyzed library object programs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, help="JSON stage manifest for arguments exceeding wrapper limits")
    parser.add_argument("--metadata", type=Path, help="build metadata JSON; defaults to the zlib manifest")
    parser.add_argument("--project-parent", type=Path, help="generated Ghidra project parent directory")
    parser.add_argument("--project-name", help="generated Ghidra project name")
    parser.add_argument("--report", type=Path, help="analysis report path")
    parser.add_argument("--display-name", help="library display name")
    parser.add_argument("--force", action="store_true", help="accepted for an explicit generated-state rebuild")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8")) if args.manifest else {}
    config = load_config()
    ghidra_install_dir = require_ghidra_environment()
    metadata_path = Path(manifest.get("metadata", args.metadata or ZLIB_BUILD_ROOT / "metadata.json"))
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    display_name = manifest.get("display_name", args.display_name or metadata.get("display_name", "zlib"))
    project_parent = Path(manifest.get("project_parent", args.project_parent or BUILD_ROOT / "ghidra_library_project"))
    project_name = manifest.get("project_name", args.project_name or f"{metadata.get('library_key', 'zlib')}_library")
    report_path = Path(manifest.get("report", args.report or REPORT_ROOT / "library_analysis.json"))
    # The project is generated state, so rebuilding this stage is safe and
    # keeps a no-flag rerun from accidentally ingesting stale programs.
    if project_parent.exists():
        shutil.rmtree(project_parent)

    import pyghidra

    # PyGhidra is started through TEST/run_ghidra_python.bat by the outer pipeline.
    # The Java APIs below are the same GhidraProject and analysis APIs used by
    # Ghidra headless workflows.
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from ghidra.program.model.lang import CompilerSpecID, LanguageID
    from ghidra.program.util import DefaultLanguageService
    from java.io import File

    project = None
    analyzed = []
    try:
        project_parent.mkdir(parents=True, exist_ok=True)
        project = GhidraProject.createProject(str(project_parent), project_name, False)
        language = DefaultLanguageService.getLanguageService().getLanguage(LanguageID(GHIDRA_LANGUAGE_ID))
        compiler = language.getCompilerSpecByID(CompilerSpecID(GHIDRA_COMPILER_SPEC))
        object_paths = [Path(path) for path in metadata["object_files"]]
        if not object_paths:
            raise RuntimeError(f"{display_name} build metadata contains no object files")
        for object_path in object_paths:
            imported = project.importProgram(File(str(object_path)), language, compiler)
            if imported is None:
                raise RuntimeError(f"Ghidra failed to import object file {object_path}")
            project.saveAs(imported, "/", object_path.name, True)
            project.close(imported)

        # This is the original FunctionIDHeadlessPrescript.java from this
        # repository, not a Python reimplementation of its option changes.
        prescript = (
            ghidra_install_dir
            / "Ghidra"
            / "Features"
            / "FunctionID"
            / "ghidra_scripts"
            / "FunctionIDHeadlessPrescript.java"
        )
        if not prescript.is_file():
            raise RuntimeError(f"Original FID prescript is missing: {prescript}")

        for object_path in object_paths:
            program = project.openProgram("/", object_path.name, False)
            if program is None:
                raise RuntimeError(f"Ghidra could not reopen {object_path.name}")
            try:
                stdout, stderr = pyghidra.ghidra_script(
                    prescript,
                    project.getProject(),
                    program,
                    echo_stdout=False,
                    echo_stderr=False,
                )
                pyghidra.analyze(program)
                options = project.getAnalysisOptions(program)
                required_options = {
                    "Function ID": False,
                    "Library Identification": False,
                    "Scalar Operand References": True,
                }
                actual_options = {
                    name: bool(options.getBoolean(name, not expected))
                    for name, expected in required_options.items()
                    if name in list(options.getOptionNames())
                }
                for name, expected in required_options.items():
                    if name in actual_options and actual_options[name] != expected:
                        raise RuntimeError(
                            f"FID prescript option mismatch in {object_path.name}: "
                            f"{name}={actual_options[name]}, expected {expected}"
                        )
                rows = function_rows(program)
                analyzed.append(
                    {
                        "program": object_path.name,
                        "language_id": str(program.getLanguageID()),
                        "compiler_spec": str(program.getCompilerSpec().getCompilerSpecID()),
                        "source_languages": source_language_ids(program),
                        "function_count": len(rows),
                        "named_functions": sorted(
                            row["name"] for row in rows if not row["name"].startswith("FUN_")
                        ),
                        "functions": rows,
                        "prescript_stdout": str(stdout),
                        "prescript_stderr": str(stderr),
                    }
                )
                project.save(program)
            finally:
                project.close(program)
    finally:
        if project is not None:
            project.close()

    if not analyzed:
        raise RuntimeError(f"No {display_name} object programs were analyzed")
    report = {
        "stage": "analyze",
        "library": display_name,
        "compiler": metadata.get("compiler", ""),
        "compiler_version": metadata.get("compiler_version", ""),
        "architecture": metadata.get("architecture", ""),
        "configuration": metadata.get("configuration", ""),
        "linkage": metadata.get("linkage", ""),
        "ghidra_install_dir": portable_path(ghidra_install_dir),
        "ghidra_language_id": GHIDRA_LANGUAGE_ID,
        "ghidra_compiler_spec": GHIDRA_COMPILER_SPEC,
        "project_parent": portable_path(project_parent),
        "project_name": project_name,
        "object_programs": analyzed,
        "total_functions": sum(item["function_count"] for item in analyzed),
        "expected_functions": metadata.get("expected_functions", config["expected_functions"]),
    }
    write_json(report_path, report)
    print(f"[+] Analyzed {len(analyzed)} {display_name} object programs")
    print(f"[+] Total analyzed functions: {report['total_functions']}")
    print(f"[+] Analysis report: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
