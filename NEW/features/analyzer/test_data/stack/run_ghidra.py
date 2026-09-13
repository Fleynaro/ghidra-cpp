"""Generate a report of stack references and variables created by the Stack analyzer."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Stack"
DEPENDENCY_NAME = "Subroutine References"


def import_and_reopen(project, input_path: Path):
    """Import, persist, close, and reopen through GhidraProject.openProgram."""
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError("Ghidra failed to import the executable")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    return project.openProgram("/", input_path.name, False)


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes while suppressing unrelated code analysis."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare stack fixture disassembly")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def configure_analysis(project, program) -> list[str]:
    """Enable Stack, local variables, and no stack parameters while disabling other analyzers."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in list(options.getOptionNames()):
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, False)
    options.setBoolean(ANALYZER_NAME, True)
    options.setBoolean(DEPENDENCY_NAME, True)
    local_option = "Stack.Create Local Variables"
    if local_option in list(options.getOptionNames()):
        options.setBoolean(local_option, True)
    parameter_option = "Stack.Create Param Variables"
    if parameter_option in list(options.getOptionNames()):
        options.setBoolean(parameter_option, False)
    return sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    )


def function_rows(program):
    """Extract actual functions, local variables, and stack references after analysis."""
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        entry = int(function.getEntryPoint().getOffset())
        variables = []
        for variable in function.getAllVariables():
            variables.append((str(variable.getName()), str(variable.getVariableStorage()), str(variable.getDataType().getDisplayName())))
        stack_refs = []
        iterator = program.getListing().getInstructions(function.getBody(), True)
        while iterator.hasNext():
            instruction = iterator.next()
            for reference in instruction.getReferencesFrom():
                if reference.isStackReference():
                    stack_refs.append((int(instruction.getMinAddress().getOffset()), str(reference.getToAddress())))
        rows.append((entry, str(function.getName()), sorted(variables), sorted(stack_refs)))
    return sorted(rows)


def flatten_rows(rows):
    """Flatten function snapshots into function, variable, and reference evidence."""
    functions = [(entry, name) for entry, name, _, _ in rows]
    variables = [(entry, function, variable, storage, data_type) for entry, function, variable_rows, _ in rows for variable, storage, data_type in variable_rows]
    references = [(entry, name, source, target) for entry, name, _, reference_rows in rows for source, target in reference_rows]
    return functions, variables, references


def keyed_delta(before, after, key_size: int):
    """Return exact added, removed, and changed rows keyed by their stable prefix."""
    before_map = {row[:key_size]: row for row in before}
    after_map = {row[:key_size]: row for row in after}
    added = sorted(after_map[key] for key in set(after_map) - set(before_map))
    removed = sorted(before_map[key] for key in set(before_map) - set(after_map))
    changed = sorted(
        (before_map[key], after_map[key])
        for key in set(before_map) & set(after_map)
        if before_map[key] != after_map[key]
    )
    return added, removed, changed


def render_snapshot(title: str, rows, provenance: str) -> list[str]:
    """Render functions, stack variables, and stack references for one phase."""
    functions, variables, references = flatten_rows(rows)
    lines = [f"### {title}", "", "#### Functions", "", "| Entry | Name | Provenance |", "| --- | --- | --- |"]
    lines.extend(f"| `0x{entry:016X}` | `{name}` | {provenance} |" for entry, name in functions)
    if not functions:
        lines.append("| _(none)_ | | |")
    lines.extend(["", "#### Stack variables", "", "| Function entry | Function | Name | Storage | Data type | Provenance |", "| --- | --- | --- | --- | --- | --- |"])
    lines.extend(f"| `0x{entry:016X}` | `{function}` | `{name}` | `{storage}` | `{data_type}` | {provenance} |" for entry, function, name, storage, data_type in variables)
    if not variables:
        lines.append("| _(none)_ | | | | | |")
    lines.extend(["", "#### Stack references", "", "| Function entry | Function | Source | Target | Provenance |", "| --- | --- | --- | --- | --- |"])
    lines.extend(f"| `0x{entry:016X}` | `{function}` | `0x{source:016X}` | `{target}` | {provenance} |" for entry, function, source, target in references)
    if not references:
        lines.append("| _(none)_ | | | | |")
    return lines


def render_delta(title: str, added, removed, changed, columns: str, hex_indexes=()) -> list[str]:
    """Render exact added, removed, and changed rows for one stack artifact kind."""
    def format_value(index: int, value) -> str:
        """Format address-valued fields in the same stable form as phase snapshots."""
        return f"0x{value:016X}" if index in hex_indexes and isinstance(value, int) else str(value)

    lines = [f"### {title}", "", f"| Change | {columns} |", f"| --- | {' | '.join('---' for _ in columns.split(' | '))} |"]
    lines.extend("| Added | " + " | ".join(f"`{format_value(index, value)}`" for index, value in enumerate(row)) + " |" for row in added)
    lines.extend("| Removed | " + " | ".join(f"`{format_value(index, value)}`" for index, value in enumerate(row)) + " |" for row in removed)
    lines.extend(
        "| Changed | " + " | ".join(f"`{format_value(index, before_value)} -> {format_value(index, after_value)}`" for index, (before_value, after_value) in enumerate(zip(before_row, after_row))) + " |"
        for before_row, after_row in changed
    )
    if not added and not removed and not changed:
        lines.append("| _(none)_ | " + " | ".join("" for _ in columns.split(" | ")) + " |")
    return lines


def render(input_path: Path, enabled: list[str], before, after) -> str:
    """Render before/after stack artifacts and exact function/variable/reference deltas."""
    before_functions, before_variables, before_references = flatten_rows(before)
    after_functions, after_variables, after_references = flatten_rows(after)
    function_delta = keyed_delta(before_functions, after_functions, 1)
    variable_delta = keyed_delta(before_variables, after_variables, 2)
    reference_delta = keyed_delta(before_references, after_references, 3)
    lines = [
        "# Stack Behavioral Fixture",
        "",
        "> Generated from actual Ghidra stack-variable and reference snapshots around the target analyzer.",
        "",
        "## Input",
        "",
        f"- **Executable:** `{input_path.name}`",
        f"- **Executable bytes:** `{input_path.stat().st_size}`",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean option |",
        "| --- |",
    ]
    lines.extend(f"| `{name}` |" for name in enabled)
    lines.extend(["", "## Before target analysis"])
    lines.extend(render_snapshot("Stack artifacts visible before target analysis", before, "Disassembly/fixture prerequisite"))
    lines.extend(["", "## After target analysis"])
    lines.extend(render_snapshot("Stack artifacts visible after target analysis", after, "Post-target stack state"))
    lines.extend(["", "## Delta", "", "The target boundary is `project.analyze(program)`. Existing functions, variables, and references are retained as before-state artifacts; only these exact keyed changes are attributed to Stack.", ""])
    lines.extend(render_delta("Function delta", *function_delta, "Entry | Name", (0,)))
    lines.extend(render_delta("Stack variable delta", *variable_delta, "Function entry | Function | Name | Storage | Data type", (0,)))
    lines.extend(render_delta("Stack reference delta", *reference_delta, "Function entry | Function | Source | Target", (0, 2)))
    delta_count = sum(len(rows) for delta in (function_delta, variable_delta, reference_delta) for rows in delta)
    lines.extend(["", "### Delta conclusion", "", f"- **Exact stack delta rows:** `{delta_count}`.", "- Stack references and variables are reported from Ghidra's listing and function model; source-level local names are not assumed."])
    return "\n".join(lines) + "\n"


def main() -> int:
    """Run the stack analyzer in a temporary project and cleanly save and close it."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_stack.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_stack.md"
    if not input_path.is_file():
        print(f"ERROR: executable does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_stack_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "stack", False)
        program = import_and_reopen(project, input_path)
        prepare_disassembly(program)
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Stack analyzer was not enabled: {enabled}")
        before = function_rows(program)
        project.analyze(program)
        after = function_rows(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, before, after), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    except Exception as error:
        print(f"ERROR: Stack analysis failed: {error}", file=sys.stderr)
        return 1
    finally:
        if project is not None:
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
