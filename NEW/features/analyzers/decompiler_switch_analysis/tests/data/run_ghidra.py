#!/usr/bin/env python3
"""Run Decompiler Switch Analysis and extract recovered switch labels/references."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER = "Decompiler Switch Analysis"
DEPENDENCIES: tuple[str, ...] = ()


def address_text(address) -> str:
    """Render a Ghidra address as a stable offset."""
    return f"0x{int(address.getOffset()):016X}"


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and enable switch analysis explicitly."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    enabled_names = {ANALYZER, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in enabled_names)
    return sorted(str(name) for name in options.getOptionNames()
                  if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def prepare_code(program) -> None:
    """Disassemble executable bytes and create function objects for exported code symbols."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet
    from ghidra.program.model.symbol import SymbolType

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    tx = program.startTransaction("Prepare fixture functions")
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


def switch_facts(program):
    """Extract switchD symbols and computed-jump references without decompiler text."""
    symbols = []
    for symbol in program.getSymbolTable().getAllSymbols(True):
        if symbol.getName().startswith("switchD"):
            symbols.append((address_text(symbol.getAddress()), symbol.getName()))
    computed = []
    for instruction in program.getListing().getInstructions(program.getMemory().getExecuteSet(), True):
        if instruction.getFlowType().isJump() and instruction.getFlowType().isComputed():
            targets = sorted(address_text(ref.getToAddress()) for ref in instruction.getReferencesFrom())
            computed.append((address_text(instruction.getAddress()), targets))
    return sorted(symbols), sorted(computed)


def keyed_delta(before, after, key):
    """Compare rows by a stable key and classify additions, removals, and changes."""
    before_by_key = {key(row): row for row in before}
    after_by_key = {key(row): row for row in after}
    added = [after_by_key[item] for item in sorted(set(after_by_key) - set(before_by_key))]
    removed = [before_by_key[item] for item in sorted(set(before_by_key) - set(after_by_key))]
    changed = [(before_by_key[item], after_by_key[item])
               for item in sorted(set(before_by_key) & set(after_by_key))
               if before_by_key[item] != after_by_key[item]]
    return added, removed, changed


def switch_delta_lines(before, after) -> list[str]:
    """Render explicit label and computed-reference deltas, including no-change output."""
    before_labels, before_computed = before
    after_labels, after_computed = after
    label_delta = keyed_delta(before_labels, after_labels, lambda row: row[0])
    computed_delta = keyed_delta(before_computed, after_computed, lambda row: row[0])
    if not any(label_delta) and not any(computed_delta):
        return ["## Delta", "", "No changes observed", ""]

    def label_text(row) -> str:
        """Render one switch-label fact for a delta list."""
        return f"`{row[0]}` `{row[1]}`"

    def computed_text(row) -> str:
        """Render one computed-jump fact for a delta list."""
        return f"`{row[0]}` -> `{', '.join(row[1])}`"

    lines = ["## Delta", ""]
    for title, rows, renderer in (
        ("Added rows", (*label_delta[0], *computed_delta[0]),
         lambda row: f"label {label_text(row)}" if not isinstance(row[1], (list, tuple)) else f"computed-reference {computed_text(row)}"),
        ("Removed rows", (*label_delta[1], *computed_delta[1]),
         lambda row: f"label {label_text(row)}" if not isinstance(row[1], (list, tuple)) else f"computed-reference {computed_text(row)}"),
        ("Changed rows", (*label_delta[2], *computed_delta[2]),
         lambda pair: (f"label {label_text(pair[0])} -> {label_text(pair[1])}"
                       if not isinstance(pair[0][1], (list, tuple))
                       else f"computed-reference {computed_text(pair[0])} -> {computed_text(pair[1])}")),
    ):
        lines.extend([f"### {title}", ""])
        if rows:
            lines.extend(f"- {renderer(row)}" for row in rows)
        else:
            lines.append("- None")
        lines.append("")
    return lines


def switch_tables(lines: list[str], phase: str, facts) -> None:
    """Append switch-label and computed-reference tables for one snapshot phase."""
    labels, computed = facts
    lines.extend([f"## {phase}", "", "### Switch Labels", "", "| Address | Symbol |", "| --- | --- |"])
    lines.extend(f"| `{address}` | `{name}` |" for address, name in labels)
    lines.extend(["", "### Computed Jump References", "", "| Dispatch | Case references |", "| --- | --- |"])
    lines.extend(f"| `{address}` | `{', '.join(targets)}` |" for address, targets in computed)


def report(input_path: Path, enabled: list[str], before, after) -> str:
    """Render phase switch facts and an explicit switch-analysis delta."""
    lines = ["# Decompiler Switch Analysis Behavioral Fixture", "",
             "> Generated by PyGhidra; only stable switch labels and references are extracted.", "",
             "## Analysis Configuration", "", f"- **Input:** `{input_path.name}`",
             f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", ""]
    switch_tables(lines, "Before target analysis", before)
    lines.append("")
    switch_tables(lines, "After target analysis", after)
    lines.extend(["", "## Fixture Assertions", "", f"- **Switch labels before target analysis:** `{len(before[0])}`", f"- **Switch labels after target analysis:** `{len(after[0])}`", ""])
    lines.extend(switch_delta_lines(before, after))
    return "\n".join(lines)


def main() -> int:
    """Create, save, reopen, analyze, save, and close an isolated Ghidra project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_decompiler_switch_analysis.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_decompiler_switch_analysis.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_decompiler_switch_analysis_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "decompiler_switch_analysis", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = None
        project = GhidraProject.openProject(str(project_parent), "decompiler_switch_analysis", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        prepare_code(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        before = switch_facts(program)
        project.analyze(program)
        after = switch_facts(program)
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
