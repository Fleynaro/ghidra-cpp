"""Populate a genuine packed .fidb through Ghidra's FidService ingestion path."""

from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
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


def database_rows(database) -> list[dict]:
    """Read function records through Ghidra's public FID database query API."""
    rows = []
    for record in database.findFunctionsByNameSubstring(""):
        rows.append(
            {
                "name": str(record.getName()),
                "entry_point": f"0x{int(record.getEntryPoint()):016X}",
                "domain_path": str(record.getDomainPath()),
                "full_hash": f"0x{int(record.getFullHash()) & ((1 << 64) - 1):016X}",
                "specific_hash": f"0x{int(record.getSpecificHash()) & ((1 << 64) - 1):016X}",
            }
        )
    return rows


def source_language_filter(library) -> str:
    """Read source-language metadata when supported by the installed FID schema."""
    try:
        value = library.getGhidraSourceLanguageID()
    except AttributeError:
        # The installed Ghidra 12.1.3 FunctionID.jar uses the older library
        # schema, whose metadata column has a compiler spec but no source ID.
        return ""
    return "" if value is None else str(value)


def remove_known_user_file(manager, path: Path) -> None:
    """Detach an earlier generated database so Ghidra does not cache stale metadata."""
    for fid_file in list(manager.getUserAddedFiles()):
        if Path(str(fid_file.getPath())).resolve() == path.resolve():
            manager.removeUserFile(fid_file)


def main() -> int:
    """Create, populate, save, reopen, and validate the requested zlib FID database."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="accepted for an explicit generated-state rebuild")
    args = parser.parse_args()
    config = load_config()
    require_ghidra_environment()
    analysis_report = json.loads((REPORT_ROOT / "library_analysis.json").read_text(encoding="utf-8"))
    object_programs = analysis_report["object_programs"]
    destination = (
        FIDB_ROOT
        / "libraries"
        / "zlib"
        / config["zlib_version"]
        / f"zlib-{config['zlib_version']}-{config['fid_library_variant']}.fidb"
    )
    destination.parent.mkdir(parents=True, exist_ok=True)
    # The destination is a generated artifact. Replace it atomically after the
    # new database has been populated and reopened, even on a normal rerun.

    import pyghidra

    # The following imports and calls intentionally mirror
    # Ghidra/Features/FunctionID/ghidra_scripts/CreateMultipleLibraries.java:
    # FidFileManager creates the packed schema, FidService performs the normal
    # FidServiceLibraryIngest path, and FidDB.saveDatabase commits it.
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from ghidra.feature.fid.db import FidFileManager
    from ghidra.feature.fid.service import FidService
    from ghidra.program.model.lang import LanguageID
    from ghidra.util.task import TaskMonitor
    from java.io import File
    from java.util import ArrayList

    project_parent = BUILD_ROOT / "ghidra_library_project"
    project = GhidraProject.openProject(str(project_parent), "zlib_library", True)
    manager = FidFileManager.getInstance()
    temporary_fd, temporary_name = tempfile.mkstemp(
        prefix=f".{destination.stem}.", suffix=".fidb", dir=str(destination.parent)
    )
    os.close(temporary_fd)
    temporary_path = Path(temporary_name)
    temporary_path.unlink()
    fid_file = None
    database = None
    committed = False
    try:
        if destination.exists():
            remove_known_user_file(manager, destination)
            destination.unlink()
        manager.createNewFidDatabase(File(str(temporary_path)))
        fid_file = manager.addUserFidFile(File(str(temporary_path)))
        if fid_file is None:
            raise RuntimeError("Ghidra rejected the newly created packed FID database")
        database = fid_file.getFidDB(True)
        program_files = ArrayList()
        for item in object_programs:
            domain_file = project.getProjectData().getFile("/" + item["program"])
            if domain_file is None:
                raise RuntimeError(f"Analyzed Ghidra domain file is missing: {item['program']}")
            program_files.add(domain_file)

        try:
            from ghidra.feature.fid.db import FidFilter

            filter_argument = FidFilter(GHIDRA_LANGUAGE_ID, GHIDRA_COMPILER_SPEC, config["source_language"])
            filter_api = "FidFilter(LanguageID, compiler spec, source language)"
        except ImportError:
            # Ghidra 12.1.3 installations may expose the older overload used by
            # the official CreateMultipleLibraries script. It still delegates
            # to FidServiceLibraryIngest and does not alter FID hashing.
            filter_argument = LanguageID(GHIDRA_LANGUAGE_ID)
            filter_api = "LanguageID compatibility overload"

        result = FidService().createNewLibraryFromPrograms(
            database,
            config["fid_library_family"],
            config["zlib_version"],
            config["fid_library_variant"],
            program_files,
            None,
            filter_argument,
            None,
            None,
            TaskMonitor.DUMMY,
        )
        if result is None:
            raise RuntimeError("FidService returned no FidPopulateResult")
        database.saveDatabase("Saving zlib FID database", TaskMonitor.DUMMY)
        libraries = database.getAllLibraries()
        rows = database_rows(database)
        if not libraries or not rows:
            raise RuntimeError("FidService produced no library or function records")
        expected = set(config["expected_functions"])
        actual_names = {row["name"] for row in rows}
        missing = sorted(expected - actual_names)
        if missing:
            failures = {
                str(key): int(value)
                for key, value in result.getFailures().entrySet()
            }
            raise RuntimeError(
                "Generated FID database is missing expected names: "
                f"{missing}; actual records={sorted(actual_names)}; exclusions={failures}"
            )
        library = libraries[0]
        generation = {
            "stage": "generate",
            "ghidra_version": str(library.getGhidraVersion()),
            "ghidra_install_dir": portable_path(require_ghidra_environment()),
            "library_path": portable_path(destination),
            "library_family": str(library.getLibraryFamilyName()),
            "library_version": str(library.getLibraryVersion()),
            "library_variant": str(library.getLibraryVariant()),
            "language_id": str(library.getGhidraLanguageID()),
            "language_version": int(library.getGhidraLanguageVersion()),
            "language_minor_version": int(library.getGhidraLanguageMinorVersion()),
            "compiler_spec_filter": str(library.getGhidraCompilerSpecID()),
            "source_language_filter": source_language_filter(library),
            "source_language_declared": config["source_language"],
            "source_language_metadata_note": (
                "Installed Ghidra 12.1.3 legacy FunctionID schema has no source-language column; "
                "empty metadata means all source languages."
            ),
            "filter_api": filter_api,
            "functions_attempted": int(result.getTotalAttempted()),
            "functions_added": int(result.getTotalAdded()),
            "functions_excluded": int(result.getTotalExcluded()),
            "function_record_count": len(rows),
            "expected_functions": config["expected_functions"],
            "function_records": rows,
        }
        database.close()
        database = None
        manager.removeUserFile(fid_file)
        fid_file = None
        os.replace(temporary_path, destination)
        committed = True
        final_file = manager.addUserFidFile(File(str(destination)))
        if final_file is None:
            raise RuntimeError("Ghidra rejected the saved generated FID database on reopen")
        final_database = final_file.getFidDB(False)
        try:
            reopened_rows = database_rows(final_database)
            if len(reopened_rows) != len(rows):
                raise RuntimeError(
                    f"FID database reopen changed record count: {len(rows)} -> {len(reopened_rows)}"
                )
            generation["reopened_function_record_count"] = len(reopened_rows)
        finally:
            final_database.close()
        report_path = REPORT_ROOT / "fidb_generation.json"
        write_json(report_path, generation)
        print(f"[+] Generated valid Ghidra FIDB: {destination}")
        print(f"[+] Ingested function records: {len(rows)}")
        print(f"[+] Generation report: {report_path}")
    finally:
        if database is not None:
            database.close()
        if fid_file is not None:
            manager.removeUserFile(fid_file)
        if not committed and temporary_path.exists():
            temporary_path.unlink()
        project.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
