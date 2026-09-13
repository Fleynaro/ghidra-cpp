#!/usr/bin/env python3
"""Generate the behavioral report for the Apply Data Archives fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "Apply Data Archives"
DEPENDENCIES: tuple[str, ...] = ()
ARCHIVE_NAME = "generic_clib_64.gdt"


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated analyzers and configure the target archive options explicitly."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    wanted = {ANALYZER_NAME, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in wanted)
    options.setString(ANALYZER_NAME + ".Archive Chooser", ARCHIVE_NAME)
    options.setBoolean(ANALYZER_NAME + ".Create Analysis Bookmarks", False)
    return sorted(str(name) for name in options.getOptionNames() if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def function_rows(program) -> list[tuple[str, str, str, str]]:
    """Extract names, signatures, calling conventions, and signature sources."""
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        rows.append((str(function.getName()), str(function.getSignature()), str(function.getCallingConventionName()), str(function.getSignatureSource())))
    return sorted(rows)


def write_report(output_path: Path, input_path: Path, enabled: list[str], before, after) -> None:
    """Write the archive configuration and stable before/after signature observations."""
    before_map = {row[0]: row for row in before}
    changed = [row for row in after if before_map.get(row[0]) != row]
    lines = [
        "# Apply Data Archives Behavioral Fixture", "", "> Generated automatically with PyGhidra.", "",
        "## Input", "", f"- **File:** `{input_path.name}`", f"- **File size:** `{input_path.stat().st_size}` bytes", "",
        "## Analysis Configuration", "", f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", f"- **Archive Chooser:** `{ARCHIVE_NAME}`", "- **Create Analysis Bookmarks:** `false`", "",
        "## Function Signatures Changed", "", "| Name | Signature | Calling convention | Source |", "| --- | --- | --- | --- |",
    ]
    lines.extend(f"| `{name}` | `{signature}` | `{convention}` | `{source}` |" for name, signature, convention, source in changed)
    lines.extend(["", f"- **Changed signature count:** `{len(changed)}`.", "- **Archive availability:** `Observed by analyzer run; see changed rows rather than assuming archive contents.`", ""])
    output_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Create, save, reopen, analyze, report, and cleanly close the temporary project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_apply_data_archives.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_apply_data_archives.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File
    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_apply_data_archives_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "apply_data_archives", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = GhidraProject.openProject(str(project_parent), "apply_data_archives", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Target analyzer was not enabled: {enabled}")
        before = function_rows(program)
        project.analyze(program)
        write_report(output_path, input_path, enabled, before, function_rows(program))
        project.save(program)
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            if "program" in locals():
                project.save(program)
                project.close(program)
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
