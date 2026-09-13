#!/usr/bin/env python3
"""Generate the behavioral report for the Condense Filler Bytes fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "Condense Filler Bytes"
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
    options.setString(ANALYZER_NAME + ".Filler Value", "Auto")
    options.setInt(ANALYZER_NAME + ".Minimum number of sequential bytes", 1)
    return sorted(str(name) for name in options.getOptionNames() if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def alignment_rows(program) -> list[tuple[int, int, str]]:
    """Extract AlignmentDataType ranges created by the target analyzer."""
    rows = []
    iterator = program.getListing().getDefinedData(True)
    while iterator.hasNext():
        data = iterator.next()
        if data.getDataType().getName() != "Alignment":
            continue
        rows.append((int(data.getMinAddress().getOffset()), int(data.getMaxAddress().getOffset()), data.getDefaultValueRepresentation()))
    return sorted(rows)


def write_report(output_path: Path, input_path: Path, enabled: list[str], rows) -> None:
    """Write observed filler condensations without inventing linker-dependent values."""
    lines = [
        "# Condense Filler Bytes Behavioral Fixture", "",
        "> Generated automatically with PyGhidra.", "",
        "## Input", "", f"- **File:** `{input_path.name}`", f"- **File size:** `{input_path.stat().st_size}` bytes", "",
        "## Analysis Configuration", "", f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", "- **Filler Value:** `Auto`", "- **Minimum number of sequential bytes:** `1`", "",
        "## Alignment Data Created", "", "| Start | End | Representation |", "| --- | --- | --- |",
    ]
    lines.extend(f"| `0x{start:016X}` | `0x{end:016X}` | `{representation}` |" for start, end, representation in rows)
    lines.extend(["", f"- **Alignment ranges created:** `{len(rows)}`.", ""])
    output_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Create, save, reopen, analyze, report, and cleanly close the temporary project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_condense_filler_bytes.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_condense_filler_bytes.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File
    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_condense_filler_bytes_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "condense_filler_bytes", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = GhidraProject.openProject(str(project_parent), "condense_filler_bytes", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Target analyzer was not enabled: {enabled}")
        project.analyze(program)
        write_report(output_path, input_path, enabled, alignment_rows(program))
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
