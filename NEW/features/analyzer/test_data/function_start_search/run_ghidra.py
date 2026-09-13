#!/usr/bin/env python3
"""Extract Function Start Search behavior from the saved Ghidra program."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZERS = ["Function Start Search", "Function Start Search After Code", "Function Start Search After Data"]
POSITIVE_SYMBOL = "function_start_positive_pattern"
POSITIVE_MARK_OFFSET = 3


def value(address) -> int:
    """Return the stable numeric address offset used by the report."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Close the saved analyzed program through the GhidraProject lifecycle API."""
    if program is not None:
        project.close(program)
    else:
        project.close()


def configure_analysis(project, program) -> list[str]:
    """Enable the main and registered post-search phases explicitly."""
    from ghidra.framework.options import OptionType

    analysis_options = project.getAnalysisOptions(program)
    for name in list(analysis_options.getOptionNames()):
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE:
            analysis_options.setBoolean(name, str(name) in ANALYZERS)
    function_start_options = analysis_options.getOptions(ANALYZERS[0])
    function_start_options.setBoolean("Bookmark Functions", True)
    enabled = sorted(
        str(name)
        for name in analysis_options.getOptionNames()
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE
        and analysis_options.getBoolean(name, False)
    )
    analyzer_enabled = sorted(name for name in ANALYZERS if name in enabled)
    expected = sorted(ANALYZERS[:1])
    if analyzer_enabled != expected:
        raise RuntimeError(f"Unexpected enabled analyzers: {enabled}; expected {expected}")
    return analyzer_enabled


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes without code-analysis side effects."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    executable = program.getMemory().getExecuteSet()
    transaction = program.startTransaction("Prepare function-start fixture")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def names_and_entries(program):
    """Return exported candidate names and all non-external function entries."""
    candidates = {}
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        name = str(symbol.getName(False))
        if name == POSITIVE_SYMBOL:
            candidates[name] = value(symbol.getAddress()) + POSITIVE_MARK_OFFSET
        elif name.startswith("function_start_candidate_"):
            candidates[name] = value(symbol.getAddress())
    entries = sorted(
        value(function.getEntryPoint())
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal()
    )
    return candidates, entries


def remove_positive_function(program, candidates) -> None:
    """Remove only the positive candidate function so the negative remains a control."""
    address_space = program.getAddressFactory().getDefaultAddressSpace()
    manager = program.getFunctionManager()
    transaction = program.startTransaction("Remove pre-existing candidate functions")
    committed = False
    try:
        manager.removeFunction(address_space.getAddress(candidates[POSITIVE_SYMBOL]))
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def bookmarks(program) -> list[tuple[int, str, str]]:
    """Extract bookmarks created by the Function Start Search family."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        category = str(bookmark.getCategory())
        if "Function Start Search" in category:
            rows.append((value(bookmark.getAddress()), category, str(bookmark.getComment())))
    return sorted(rows)


def run_authoritative_search(program) -> None:
    """Run the registered FunctionStartAnalyzer action on the complete loaded set."""
    from ghidra.app.analyzers import FunctionStartAnalyzer
    from ghidra.app.util.importer import MessageLog
    from ghidra.util.task import TaskMonitor

    analyzer = FunctionStartAnalyzer()
    analyzer.optionsChanged(
        program.getOptions(program.ANALYSIS_PROPERTIES).getOptions(ANALYZERS[0]), program
    )
    analyzer.added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())


def report(input_path: Path, enabled, candidates, before, after, marks_before, marks_after) -> str:
    """Render before/after candidate evidence and the rejected negative control."""
    created = sorted(set(after) - set(before))
    lines = [
        "# Function Start Search Behavioral Fixture",
        "",
        "> Generated from the saved MSVC x64 PE program with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean analyzer |",
        "| --- |",
        *[f"| `{name}` |" for name in enabled],
        "",
        "## Candidate Export Offsets",
        "",
        "| Symbol | Offset | Before function | After function | Created by target |",
        "| --- | --- | --- | --- | --- |",
    ]
    for name, address in sorted(candidates.items()):
        lines.append(
            f"| `{name}` | `0x{address:016X}` | `{str(address in before).lower()}` | `{str(address in after).lower()}` | `{str(address in after and address not in before).lower()}` |"
        )
    lines.extend(["", "## Functions Created By Pattern Search", "", "| Offset |", "| --- |"])
    lines.extend(f"| `0x{address:016X}` |" for address in created)
    lines.extend(["", "## Function Start Search Bookmarks Before Analysis", "", "| Offset | Category | Comment |", "| --- | --- | --- |"])
    lines.extend(f"| `0x{address:016X}` | `{category}` | `{comment}` |" for address, category, comment in marks_before)
    lines.extend(["", "## Function Start Search Bookmarks After Analysis", "", "| Offset | Category | Comment |", "| --- | --- | --- |"])
    lines.extend(f"| `0x{address:016X}` | `{category}` | `{comment}` |" for address, category, comment in marks_after)
    lines.extend([
        "",
        "## Fixture Assertions",
        "",
        f"- **Candidate exports:** `{len(candidates)}`.",
        f"- **Functions created:** `{len(created)}`.",
        f"- **Pattern bookmarks before/after:** `{len(marks_before)}` / `{len(marks_after)}`.",
        f"- **Positive candidate discovered:** `{str(candidates[POSITIVE_SYMBOL] in after and candidates[POSITIVE_SYMBOL] not in before).lower()}`.",
        "- The ordinary exported candidate remains a rejected negative control because it is already a function before the target analyzer runs.",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, analyze, save, and extract the fixture program."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_function_start_search.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_function_start_search.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_function_start_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "function_start_search", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        prepare_disassembly(program)
        candidates, before = names_and_entries(program)
        remove_positive_function(program, candidates)
        before = names_and_entries(program)[1]
        marks_before = bookmarks(program)
        enabled = configure_analysis(project, program)
        project.analyze(program)
        run_authoritative_search(program)
        project.analyze(program)
        candidates, after = names_and_entries(program)
        marks_after = bookmarks(program)
        if candidates[POSITIVE_SYMBOL] in before or candidates[POSITIVE_SYMBOL] not in after:
            raise RuntimeError(f"Positive Function Start candidate was not discovered: before={before}, after={after}")
        negative = [address for name, address in candidates.items() if name.startswith("function_start_candidate_")]
        if not negative or not all(address in before and address in after for address in negative):
            raise RuntimeError(f"Negative Function Start candidate evidence is incomplete: candidates={candidates}, before={before}, after={after}")
        project.save(program)
        output_path.write_text(report(input_path, enabled, candidates, before, after, marks_before, marks_after), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
