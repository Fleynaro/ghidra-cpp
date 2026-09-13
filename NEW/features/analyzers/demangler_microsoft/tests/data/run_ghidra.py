#!/usr/bin/env python3
"""Run Microsoft Demangler and extract only decorated-symbol transformations."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER = "Demangler Microsoft"
DEPENDENCIES: tuple[str, ...] = ()


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and explicitly enable Microsoft demangling."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    enabled_names = {ANALYZER, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in enabled_names)
    return sorted(str(name) for name in options.getOptionNames()
                  if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def prepare_code(program) -> None:
    """Disassemble executable bytes and create functions at PE function symbols."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet
    from ghidra.program.model.symbol import SymbolType

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    tx = program.startTransaction("Prepare fixture symbols")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        for symbol in program.getSymbolTable().getAllSymbols(True):
            address = symbol.getAddress()
            if symbol.getSymbolType() == SymbolType.FUNCTION and executable.contains(address):
                if program.getFunctionManager().getFunctionAt(address) is None:
                    CreateFunctionCmd(address).applyTo(program)
        committed = True
    finally:
        program.endTransaction(tx, committed)


def demangler_facts(program):
    """Pair original Microsoft-decorated names with stable post-analysis symbols/signatures."""
    rows = []
    for symbol in program.getSymbolTable().getAllSymbols(True):
        name = symbol.getName(False)
        if not name.startswith("?"):
            continue
        function = program.getFunctionManager().getFunctionAt(symbol.getAddress())
        rows.append((name, symbol.getAddress(), function))
    result = []
    for mangled, address, function in sorted(rows, key=lambda row: row[0]):
        symbols = sorted(str(item.getName(False)) for item in program.getSymbolTable().getSymbols(address)
                         if not item.getName(False).startswith("?"))
        signature = str(function.getSignature()) if function is not None else "none"
        result.append((mangled, symbols, signature))
    return result


def fact_delta(before, after):
    """Compare demangler rows by mangled name and classify symbol/signature changes."""
    before_by_name = {row[0]: row for row in before}
    after_by_name = {row[0]: row for row in after}
    added = [after_by_name[key] for key in sorted(set(after_by_name) - set(before_by_name))]
    removed = [before_by_name[key] for key in sorted(set(before_by_name) - set(after_by_name))]
    changed = [(before_by_name[key], after_by_name[key])
               for key in sorted(set(before_by_name) & set(after_by_name))
               if before_by_name[key] != after_by_name[key]]
    return added, removed, changed


def row_text(row) -> str:
    """Render one decorated-symbol fact for a delta list."""
    mangled, symbols, signature = row
    return f"`{mangled}` -> `{', '.join(symbols)}` | `{signature}`"


def delta_lines(before, after) -> list[str]:
    """Render explicit demangler delta rows, including the no-change case."""
    added, removed, changed = fact_delta(before, after)
    if not added and not removed and not changed:
        return ["## Delta", "", "No changes observed", ""]
    lines = ["## Delta", "", "### Added rows", ""]
    if added:
        lines.extend(f"- {row_text(row)}" for row in added)
    else:
        lines.append("- None")
    lines.extend(["", "### Removed rows", ""])
    if removed:
        lines.extend(f"- {row_text(row)}" for row in removed)
    else:
        lines.append("- None")
    lines.extend(["", "### Changed rows", ""])
    if changed:
        lines.extend(f"- {row_text(old)} -> {row_text(new)}" for old, new in changed)
    else:
        lines.append("- None")
    lines.append("")
    return lines


def demangler_table(lines: list[str], phase: str, rows) -> None:
    """Append one snapshot table containing decorated names and signatures."""
    lines.extend([f"## {phase}", "", "| Mangled symbol | Demangled symbols | Function signature |", "| --- | --- | --- |"])
    lines.extend(f"| `{mangled}` | `{', '.join(symbols)}` | `{signature}` |" for mangled, symbols, signature in rows)


def report(input_path: Path, enabled: list[str], before, after) -> str:
    """Render phase decorated-name facts and an explicit demangler delta."""
    lines = ["# Microsoft Demangler Behavioral Fixture", "",
             "> Generated by PyGhidra from genuine MSVC decorated symbols.", "", "## Analysis Configuration", "",
             f"- **Input:** `{input_path.name}`", f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", ""]
    demangler_table(lines, "Before target analysis", before)
    lines.append("")
    demangler_table(lines, "After target analysis", after)
    lines.extend(["", "## Fixture Assertions", "", f"- **Decorated symbols before target analysis:** `{len(before)}`", f"- **Decorated symbols after target analysis:** `{len(after)}`", ""])
    lines.extend(delta_lines(before, after))
    return "\n".join(lines)


def main() -> int:
    """Create, save, reopen, analyze, save, and close an isolated Ghidra project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_demangler_microsoft.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_demangler_microsoft.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_demangler_microsoft_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "demangler_microsoft", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = None
        project = GhidraProject.openProject(str(project_parent), "demangler_microsoft", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        prepare_code(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        before = demangler_facts(program)
        project.analyze(program)
        after = demangler_facts(program)
        output_path.write_text(report(input_path, enabled, before, after), encoding="utf-8", newline="\n")
        project.save(program)
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            if "program" in locals():
                project.save(program)
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
