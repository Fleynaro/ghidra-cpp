#!/usr/bin/env python3
"""Extract the known-name non-returning analyzer result from Ghidra."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "test_support"))
from evidence import render_evidence

ANALYZER_NAME = "Non-Returning Functions - Known"


def value(address) -> int:
    """Return an image-relative address offset."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Close the saved analyzed program through the GhidraProject lifecycle API."""
    if program is not None:
        project.close(program)
    else:
        project.close()


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated analyzers and enable the known-name analyzer/bookmarks."""
    from ghidra.framework.options import OptionType

    analysis_options = project.getAnalysisOptions(program)
    for name in list(analysis_options.getOptionNames()):
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE:
            analysis_options.setBoolean(name, str(name) == ANALYZER_NAME)
    analysis_options.getOptions(ANALYZER_NAME).setBoolean("Create Analysis Bookmarks", True)
    enabled = [
        str(name)
        for name in analysis_options.getOptionNames()
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE
        and analysis_options.getBoolean(name, False)
    ]
    if ANALYZER_NAME not in enabled:
        raise RuntimeError(f"Known no-return analyzer is not enabled: {sorted(enabled)}")
    return [ANALYZER_NAME]


def prepare_disassembly(program) -> None:
    """Disassemble loaded executable bytes without enabling other analyzers."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    executable = program.getMemory().getExecuteSet()
    transaction = program.startTransaction("Prepare known no-return fixture")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def function_rows(program):
    """Extract every local function's no-return state and stable name."""
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if not function.isExternal():
            rows.append((value(function.getEntryPoint()), str(function.getName()), bool(function.hasNoReturn())))
    return sorted(rows)


def bookmarks(program):
    """Extract bookmarks created by the known-name analyzer."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getTypeString()) == "Analysis" and str(bookmark.getCategory()) == "Non-Returning Function":
            rows.append((value(bookmark.getAddress()), str(bookmark.getComment())))
    return sorted(rows)


def evidence_snapshot(project, program):
    """Capture no-return functions, bookmarks, and known-name analyzer options."""
    analysis_options = project.getAnalysisOptions(program)
    analyzer_options = analysis_options.getOptions(ANALYZER_NAME)
    return {
        "Data": [],
        "Functions": [
            (f"0x{address:016X}", f"{name} | no-return: {str(no_return).lower()}")
            for address, name, no_return in function_rows(program)
        ],
        "Bookmarks": [
            (f"0x{address:016X} Non-Returning Function", comment)
            for address, comment in bookmarks(program)
        ],
        "Options": [
            (ANALYZER_NAME, str(analysis_options.getBoolean(ANALYZER_NAME, False)).lower()),
            (
                "Create Analysis Bookmarks",
                str(analyzer_options.getBoolean("Create Analysis Bookmarks", False)).lower(),
            ),
        ],
    }


def report(input_path: Path, enabled, functions, marks, before, after) -> str:
    """Render the actual known-name no-return state."""
    lines = [
        "# Non-Returning Functions Known Behavioral Fixture",
        "",
        "> Generated from the saved MSVC x64 PE program with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "- **Authoritative name:** `abort` from `PEFunctionsThatDoNotReturn`",
        "",
        "## Analysis Configuration",
        "",
        f"- **Enabled analyzer:** `{enabled[0]}`",
        "- **Create Analysis Bookmarks:** `true`",
        "",
        "## Function No-Return State",
        "",
        "| Entry offset | Name | No Return |",
        "| --- | --- | --- |",
    ]
    lines.extend(f"| `0x{address:016X}` | `{name}` | `{str(no_return).lower()}` |" for address, name, no_return in functions)
    lines.extend(["", "## Non-Returning Function Bookmarks", "", "| Entry offset | Comment |", "| --- | --- |"])
    lines.extend(f"| `0x{address:016X}` | `{comment}` |" for address, comment in marks)
    abort_rows = [row for row in functions if row[1] == "abort"]
    lines.extend(["", *render_evidence(before, after), "## Fixture Assertions", "", f"- **Exact `abort` functions observed:** `{len(abort_rows)}`.", f"- **Exact `abort` functions marked no-return:** `{sum(1 for _, _, state in abort_rows if state)}`.", f"- **Known-name bookmarks:** `{len(marks)}`.", ""])
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, analyze, save, and extract the known-name fixture."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_non_returning_functions_known.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_non_returning_functions_known.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_known_noreturn_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "non_returning_functions_known", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        prepare_disassembly(program)
        enabled = configure_analysis(project, program)
        before = evidence_snapshot(project, program)
        project.analyze(program)
        # Re-run the authoritative one-shot analyzer with its option state
        # explicitly synchronized so the default analysis bookmark is visible
        # even when the scheduled pass already marked the function.
        from ghidra.app.plugin.core.analysis import NoReturnFunctionAnalyzer
        from ghidra.app.util.importer import MessageLog
        from ghidra.util.task import TaskMonitor
        known_analyzer = NoReturnFunctionAnalyzer()
        known_analyzer.optionsChanged(
            project.getAnalysisOptions(program).getOptions(ANALYZER_NAME), program
        )
        known_analyzer.added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())
        functions = function_rows(program)
        marks = bookmarks(program)
        after = evidence_snapshot(project, program)
        project.save(program)
        output_path.write_text(report(input_path, enabled, functions, marks, before, after), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
