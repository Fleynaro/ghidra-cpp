#!/usr/bin/env python3
"""Extract External Entry References behavior from a saved Ghidra program."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "test_support"))
from evidence import render_evidence

ANALYZER_NAME = "External Entry References"


def address_value(address) -> int:
    """Return the offset used for stable report rows."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Close the saved analyzed program through the GhidraProject lifecycle API."""
    if program is not None:
        project.close(program)
    else:
        project.close()


def hex_offset(value: int) -> str:
    """Render an address offset without a temporary image base."""
    return f"0x{value:016X}"


def configure_analysis(project, program) -> list[str]:
    """Disable all boolean analyzers and enable only the target analyzer."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) == ANALYZER_NAME)
    enabled = sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    )
    if enabled != [ANALYZER_NAME]:
        raise RuntimeError(f"Unexpected enabled analyzers: {enabled}")
    return enabled


def prepare_disassembly(program) -> None:
    """Disassemble loaded executable bytes without enabling code analysis."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    executable = program.getMemory().getExecuteSet()
    transaction = program.startTransaction("Prepare external-entry fixture")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def external_entries(program) -> list[int]:
    """Read the actual PE export-derived external-entry address set."""
    iterator = program.getSymbolTable().getExternalEntryPointIterator()
    values = []
    while iterator.hasNext():
        values.append(address_value(iterator.next()))
    return sorted(values)


def function_entries(program) -> list[int]:
    """Return non-external function entries from the saved listing."""
    values = []
    for function in program.getFunctionManager().getFunctions(True):
        if not function.isExternal():
            values.append(address_value(function.getEntryPoint()))
    return sorted(values)


def bookmark_evidence_rows(program):
    """Capture bookmarks in the target category for phase comparison."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getCategory()) == ANALYZER_NAME:
            rows.append((f"{hex_offset(address_value(bookmark.getAddress()))} {ANALYZER_NAME}", str(bookmark.getComment())))
    return rows


def evidence_snapshot(project, program):
    """Capture external entries, local functions, bookmarks, and analyzer options."""
    analysis_options = project.getAnalysisOptions(program)
    return {
        "Data": [(hex_offset(entry), "External entry point") for entry in external_entries(program)],
        "Functions": [
            (hex_offset(address_value(function.getEntryPoint())), str(function.getName()))
            for function in program.getFunctionManager().getFunctions(True)
            if not function.isExternal()
        ],
        "Bookmarks": bookmark_evidence_rows(program),
        "Options": [(ANALYZER_NAME, str(analysis_options.getBoolean(ANALYZER_NAME, False)).lower())],
    }


def report(input_path: Path, enabled, entries, before, after, before_evidence, after_evidence) -> str:
    """Render external entries and the analyzer's before/after function state."""
    created = sorted(set(after) - set(before))
    lines = [
        "# External Entry References Behavioral Fixture",
        "",
        "> Generated from a saved MSVC x64 PE program with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "",
        "## Analysis Configuration",
        "",
        f"- **Enabled analyzer:** `{', '.join(enabled)}`",
        "",
        "## PE External Entry References",
        "",
        "| Entry offset | Function after analysis |",
        "| --- | --- |",
    ]
    for entry in entries:
        lines.append(f"| `{hex_offset(entry)}` | `{str(entry in after).lower()}` |")
    lines.extend(["", "## Function Entries Created", "", "| Entry offset |", "| --- |"])
    lines.extend(f"| `{hex_offset(entry)}` |" for entry in created)
    lines.extend(["", *render_evidence(before_evidence, after_evidence), "## Fixture Assertions", "", f"- **External entries:** `{len(entries)}`.", f"- **Functions before analysis:** `{len(before)}`.", f"- **Functions created by the analyzer:** `{len(created)}`.", ""])
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, analyze, save, and extract the fixture program."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_external_entry_references.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_external_entry_references.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_external_entries_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "external_entry_references", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        prepare_disassembly(program)
        entries = external_entries(program)
        enabled = configure_analysis(project, program)
        before = function_entries(program)
        before_evidence = evidence_snapshot(project, program)
        project.analyze(program)
        after = function_entries(program)
        after_evidence = evidence_snapshot(project, program)
        project.save(program)
        output_path.write_text(report(input_path, enabled, entries, before, after, before_evidence, after_evidence), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
