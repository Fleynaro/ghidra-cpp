#!/usr/bin/env python3
"""Generate the behavioral report for the ASCII Strings fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "ASCII Strings"
DEPENDENCIES: tuple[str, ...] = ()
POSITIVE_SYMBOLS = ("ascii_welcome", "ascii_protocol")
NEGATIVE_SYMBOLS = ("ascii_short", "ascii_unterminated", "ascii_non_ascii")


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


def fixture_symbol(program, name):
    """Find one exported fixture symbol by its stable C-linkage name."""
    return next(iter(program.getSymbolTable().getGlobalSymbols(name)), None)


def fixture_string_facts(program):
    """Extract only data at exported positive/negative fixture symbols."""
    rows = []
    listing = program.getListing()
    for expected, names in (("positive", POSITIVE_SYMBOLS), ("negative", NEGATIVE_SYMBOLS)):
        for name in names:
            symbol = fixture_symbol(program, name)
            if symbol is None:
                raise RuntimeError(f"Fixture symbol was not imported: {name}")
            address = symbol.getAddress()
            data = listing.getDataAt(address)
            defined = data is not None and data.isDefined()
            data_type = str(data.getDataType()) if defined else "undefined"
            is_string = defined and "string" in str(data.getDataType().getName()).lower()
            value = str(data.getValue()) if is_string else ""
            end = int(data.getMaxAddress().getOffset()) if defined else int(address.getOffset())
            rows.append((expected, name, int(address.getOffset()), end, defined, data_type, value, is_string))
    return sorted(rows, key=lambda row: row[2])


def write_report(output_path: Path, input_path: Path, enabled: list[str], before, after) -> None:
    """Write fixture-owned positive strings and retained negative-control context."""
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
        "## Fixture-Owned Strings Created",
        "",
        "| Symbol | Start | End | Data type | Value |",
        "| --- | --- | --- | --- | --- |",
    ]
    positive = [row for row in after if row[0] == "positive" and row[7]]
    for _, name, address, end, _, data_type, value, _ in positive:
        lines.append(f"| `{name}` | `0x{address:016X}` | `0x{end:016X}` | `{data_type}` | `{value}` |")
    negatives = [row for row in after if row[0] == "negative"]
    lines.extend([
        "",
        "## Negative Controls",
        "",
        "| Symbol | Address | Defined data | Data type | Accepted as ASCII string |",
        "| --- | --- | --- | --- | --- |",
    ])
    lines.extend(
        f"| `{name}` | `0x{address:016X}` | `{str(defined).lower()}` | `{data_type}` | `{str(is_string).lower()}` |"
        for _, name, address, _, defined, data_type, _, is_string in negatives
    )
    lines.extend([
        "",
        f"- **Fixture-owned strings created:** `{len(positive)}`.",
        f"- **Negative controls retained:** `{len(negatives)}`.",
        "- PE metadata, import names, and unrelated initialized text are excluded by symbol ownership rather than mistaken for fixture discoveries.",
        "",
    ])
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
        before = fixture_string_facts(program)
        project.analyze(program)
        after = fixture_string_facts(program)
        if not all(row[7] for row in after if row[0] == "positive"):
            raise RuntimeError(f"Not all positive fixture strings were created: before={before}, after={after}")
        if any(row[7] for row in after if row[0] == "negative"):
            raise RuntimeError(f"A negative fixture string was accepted: before={before}, after={after}")
        write_report(output_path, input_path, enabled, before, after)
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
