#!/usr/bin/env python3
"""Extract discovered non-returning evidence from the real Ghidra listing."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Non-Returning Functions - Discovered"
EXPORTED_PREFIX = "discovered_"


def value(address) -> int:
    """Return an image-relative address offset."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Release the saved analyzed program and close the underlying project."""
    if program is not None:
        program.release(project)
    underlying = project.getProject()
    if underlying is not None:
        underlying.close()


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes without running code analysis."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    executable = program.getMemory().getExecuteSet()
    transaction = program.startTransaction("Prepare discovered no-return fixture")
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
    """Create only functions at real PE export entry points."""
    from ghidra.app.cmd.function import CreateFunctionCmd

    iterator = program.getSymbolTable().getExternalEntryPointIterator()
    while iterator.hasNext():
        address = iterator.next()
        command = CreateFunctionCmd(address)
        if not command.applyTo(program):
            raise RuntimeError(f"Could not seed exported function at {address}")


def configure_analysis(project, program) -> list[str]:
    """Enable only discovered no-return analysis and set its stable threshold/options."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) == ANALYZER_NAME)
    options.setInt("Function Non-return Threshold", 3)
    options.setBoolean("Repair Flow Damage", False)
    options.setBoolean("Create Analysis Bookmarks", True)
    enabled = [
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    ]
    if ANALYZER_NAME not in enabled:
        raise RuntimeError(f"Discovered no-return analyzer is not enabled: {sorted(enabled)}")
    return [ANALYZER_NAME]


def function_rows(program):
    """Extract exported fixture function states after discovery."""
    rows = []
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        name = str(symbol.getName(False))
        if not name.startswith(EXPORTED_PREFIX):
            continue
        function = program.getFunctionManager().getFunctionAt(symbol.getAddress())
        if function is not None:
            rows.append((value(symbol.getAddress()), name, bool(function.hasNoReturn())))
    return sorted(set(rows))


def call_rows(program):
    """Extract call targets and post-analysis flow overrides from instructions."""
    rows = []
    instructions = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while instructions.hasNext():
        instruction = instructions.next()
        if not instruction.getFlowType().isCall():
            continue
        flows = instruction.getFlows()
        if flows is None or len(flows) == 0:
            continue
        rows.append((value(instruction.getAddress()), value(flows[0]), str(instruction.getFlowOverride())))
    return sorted(rows)


def bookmarks(program):
    """Extract bookmarks emitted by the discovered analyzer."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getTypeString()) == "Analysis" and str(bookmark.getCategory()) == "Non-Returning Function":
            rows.append((value(bookmark.getAddress()), str(bookmark.getComment())))
    return sorted(rows)


def report(input_path: Path, enabled, functions, calls, marks) -> str:
    """Render actual evidence-derived no-return state."""
    target_rows = [row for row in functions if row[1] == "discovered_target"]
    lines = [
        "# Non-Returning Functions Discovered Behavioral Fixture",
        "",
        "> Generated from the saved MSVC x64 PE program with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "- **Evidence:** three real `INT3` instructions emitted by `__debugbreak` after calls to `discovered_target`",
        "",
        "## Analysis Configuration",
        "",
        f"- **Enabled analyzer:** `{enabled[0]}`",
        "- **Function Non-return Threshold:** `3`",
        "- **Repair Flow Damage:** `false`",
        "- **Create Analysis Bookmarks:** `true`",
        "",
        "## Exported Function State",
        "",
        "| Entry offset | Name | No Return |",
        "| --- | --- | --- |",
    ]
    lines.extend(f"| `0x{address:016X}` | `{name}` | `{str(no_return).lower()}` |" for address, name, no_return in functions)
    lines.extend(["", "## Call Evidence And Flow Overrides", "", "| Call offset | Target offset | Flow override |", "| --- | --- | --- |"])
    lines.extend(f"| `0x{source:016X}` | `0x{target:016X}` | `{override}` |" for source, target, override in calls)
    lines.extend(["", "## Non-Returning Function Bookmarks", "", "| Entry offset | Comment |", "| --- | --- |"])
    lines.extend(f"| `0x{address:016X}` | `{comment}` |" for address, comment in marks)
    lines.extend(["", "## Fixture Assertions", "", f"- **Discovered target rows:** `{len(target_rows)}`.", f"- **Discovered target marked no-return:** `{sum(1 for _, _, state in target_rows if state)}`.", f"- **Analyzer bookmarks:** `{len(marks)}`.", ""])
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, seed only fixture functions, analyze, save, and extract."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_non_returning_functions_discovered.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_non_returning_functions_discovered.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_discovered_noreturn_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "non_returning_functions_discovered", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        prepare_disassembly(program)
        seed_export_functions(program)
        enabled = configure_analysis(project, program)
        # The analyzer supports one-time execution; synchronize its options and
        # invoke the same added() implementation before the scheduled pass so
        # its evidence and bookmark side effects are observable.
        from ghidra.app.plugin.core.analysis import FindNoReturnFunctionsAnalyzer
        from ghidra.app.util.importer import MessageLog
        from ghidra.util.task import TaskMonitor
        discovered_analyzer = FindNoReturnFunctionsAnalyzer()
        discovered_analyzer.optionsChanged(project.getAnalysisOptions(program), program)
        discovered_analyzer.added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())
        project.analyze(program)
        functions = function_rows(program)
        calls = call_rows(program)
        marks = bookmarks(program)
        project.save(program)
        output_path.write_text(report(input_path, enabled, functions, calls, marks), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
