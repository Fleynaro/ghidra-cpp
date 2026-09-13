#!/usr/bin/env python3
"""Generate the behavioral report for the ASCII Strings fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "ASCII Strings"
DEPENDENCIES: tuple[str, ...] = ()


def address_text(address) -> str:
    """Render an address as a stable image-relative hexadecimal offset."""
    return f"0x{int(address.getOffset()):016X}"


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and enable the target plus dependencies."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    wanted = {ANALYZER_NAME, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in wanted)
    return sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE
        and options.getBoolean(name, False)
    )


def extract_strings(program) -> list[tuple[int, int, str]]:
    """Extract final defined string data units and their decoded values."""
    rows = []
    iterator = program.getListing().getDefinedData(True)
    while iterator.hasNext():
        data = iterator.next()
        if data.getDataType().getName().lower().find("string") < 0:
            continue
        value = data.getValue()
        rows.append((int(data.getMinAddress().getOffset()), int(data.getMaxAddress().getOffset()), str(value)))
    return sorted(rows)


def write_report(output_path: Path, input_path: Path, enabled: list[str], rows) -> None:
    """Write only stable analyzer-specific configuration and created string data."""
    lines = [
        "# ASCII Strings Behavioral Fixture",
        "",
        "> Generated automatically with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "",
        "## Analysis Configuration",
        "",
        f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`",
        "- **Minimum string length:** `5` (authoritative default)",
        "- **Require null termination:** `true` (authoritative default)",
        "",
        "## Strings Created",
        "",
        "| Start | End | Value |",
        "| --- | --- | --- |",
    ]
    lines.extend(f"| `0x{start:016X}` | `0x{end:016X}` | `{value}` |" for start, end, value in rows)
    lines.extend(["", f"- **Created string count:** `{len(rows)}`.", ""])
    output_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Create, save, reopen, analyze, report, and cleanly close the temporary project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_ascii_strings.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_ascii_strings.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_ascii_strings_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "ascii_strings", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = GhidraProject.openProject(str(project_parent), "ascii_strings", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Target analyzer was not enabled: {enabled}")
        project.analyze(program)
        write_report(output_path, input_path, enabled, extract_strings(program))
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
