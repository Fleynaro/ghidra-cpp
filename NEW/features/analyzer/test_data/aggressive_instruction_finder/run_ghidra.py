#!/usr/bin/env python3
"""Generate the behavioral report for the Aggressive Instruction Finder fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "Aggressive Instruction Finder"
DEPENDENCIES: tuple[str, ...] = ()
TARGET_NAME = "aggressive_candidate"


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and enable the heuristic target explicitly."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    wanted = {ANALYZER_NAME, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in wanted)
    options.setBoolean(ANALYZER_NAME + ".Create Analysis Bookmarks", True)
    return sorted(str(name) for name in options.getOptionNames() if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def find_target(program):
    """Find the exported candidate function before its code is cleared."""
    symbols = program.getSymbolTable().getAllSymbols(True)
    names = []
    while symbols.hasNext():
        symbol = symbols.next()
        names.append(str(symbol.getName(False)))
        if str(symbol.getName(False)) == TARGET_NAME:
            function = program.getFunctionManager().getFunctionAt(symbol.getAddress())
            if function is None:
                from ghidra.app.cmd.function import CreateFunctionCmd
                CreateFunctionCmd(symbol.getAddress()).applyTo(program)
                function = program.getFunctionManager().getFunctionAt(symbol.getAddress())
            if function is not None:
                return function
    raise RuntimeError(f"Exported candidate function was not found: {TARGET_NAME}; symbols={names[:30]}")


def prepare_undefined_candidate(program) -> None:
    """Disassemble first, then clear only the candidate body so its bytes become undefined."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare aggressive undefined candidate")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Fixture disassembly failed: {command.getStatusMsg()}")
        from ghidra.app.cmd.function import CreateFunctionCmd
        symbols = program.getSymbolTable().getAllSymbols(True)
        while symbols.hasNext():
            symbol = symbols.next()
            address = symbol.getAddress()
            name = str(symbol.getName(False))
            if executable.contains(address) and (name.startswith("seed_") or name == TARGET_NAME):
                if program.getFunctionManager().getFunctionAt(address) is None:
                    CreateFunctionCmd(address).applyTo(program)
        function = find_target(program)
        start = function.getEntryPoint()
        end = function.getBody().getMaxAddress()
        program.getListing().clearCodeUnits(start, end, False)
        program.getFunctionManager().removeFunction(start)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def function_count(program) -> int:
    """Return the current total function count used by the analyzer's guard."""
    return int(program.getFunctionManager().getFunctionCount())


def bookmark_rows(program) -> list[tuple[str, str, str]]:
    """Extract analysis bookmarks produced by the target analyzer."""
    rows = []
    manager = program.getBookmarkManager()
    iterator = manager.getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getCategory()) != ANALYZER_NAME:
            continue
        rows.append((str(bookmark.getAddress()), str(bookmark.getCategory()), str(bookmark.getComment())))
    return sorted(rows)


def write_report(output_path: Path, input_path: Path, enabled: list[str], before_count: int, after_count: int, bookmarks) -> None:
    """Write actual function and bookmark observations without claiming heuristic success."""
    lines = [
        "# Aggressive Instruction Finder Behavioral Fixture", "", "> Generated automatically with PyGhidra.", "",
        "## Input", "", f"- **File:** `{input_path.name}`", f"- **File size:** `{input_path.stat().st_size}` bytes", "",
        "## Analysis Configuration", "", f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", "- **Create Analysis Bookmarks:** `true`", "- **Minimum function count required by Java:** `20`", "",
        "## Discovery Observation", "", f"- **Functions before analysis:** `{before_count}`", f"- **Functions after analysis:** `{after_count}`", "",
        "| Bookmark address | Category | Comment |", "| --- | --- | --- |",
    ]
    lines.extend(f"| `{address}` | `{category}` | `{comment}` |" for address, category, comment in bookmarks)
    lines.extend(["", f"- **Aggressive-discovery bookmarks:** `{len(bookmarks)}`.", ""])
    output_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Create, save, reopen, prepare, analyze, report, and cleanly close the project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_aggressive_instruction_finder.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_aggressive_instruction_finder.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File
    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_aggressive_instruction_finder_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "aggressive_instruction_finder", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = GhidraProject.openProject(str(project_parent), "aggressive_instruction_finder", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        prepare_undefined_candidate(program)
        before_count = function_count(program)
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Target analyzer was not enabled: {enabled}")
        project.analyze(program)
        write_report(output_path, input_path, enabled, before_count, function_count(program), bookmark_rows(program))
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
