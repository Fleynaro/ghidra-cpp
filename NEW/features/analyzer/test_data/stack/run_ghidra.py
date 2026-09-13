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


def render(input_path: Path, enabled: list[str], rows) -> str:
    """Render only stack artifacts observed in the analyzed program."""
    lines = [
        "# Stack Behavioral Fixture",
        "",
        "> Generated from actual Ghidra stack-variable and reference state.",
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
    lines.extend(["", "## Functions", "", "| Entry | Name |", "| --- | --- |"])
    lines.extend(f"| `0x{entry:016X}` | `{name}` |" for entry, name, _, _ in rows)
    lines.extend(["", "## Stack Variables", "", "| Function | Name | Storage | Data type |", "| --- | --- | --- | --- |"])
    for _, name, variables, _ in rows:
        lines.extend(f"| `{name}` | `{variable}` | `{storage}` | `{data_type}` |" for variable, storage, data_type in variables)
    lines.extend(["", "## Stack References", "", "| Function | Source | Target |", "| --- | --- | --- |"])
    for _, name, _, references in rows:
        lines.extend(f"| `{name}` | `0x{source:016X}` | `{target}` |" for source, target in references)
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
        project.analyze(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, function_rows(program)), encoding="utf-8", newline="\n")
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
