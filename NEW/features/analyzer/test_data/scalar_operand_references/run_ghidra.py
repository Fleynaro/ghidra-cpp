"""Generate an observed scalar-operand reference report."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Scalar Operand References"


def import_and_reopen(project, input_path: Path):
    """Import and persist the executable, then reopen it with project.openProgram."""
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError("Ghidra failed to import the executable")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    return project.openProgram("/", input_path.name, False)


def prepare_disassembly(program) -> None:
    """Create instruction units only; scalar analysis is the sole enabled analyzer."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare scalar fixture disassembly")
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
    """Disable all boolean analyzers and enable Scalar Operand References explicitly."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in list(options.getOptionNames()):
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, False)
    options.setBoolean(ANALYZER_NAME, True)
    return sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    )


def scalar_rows(program):
    """Extract scalar operands and any operand references present after analysis."""
    from ghidra.program.model.scalar import Scalar

    rows = []
    iterator = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while iterator.hasNext():
        instruction = iterator.next()
        for operand_index in range(instruction.getNumOperands()):
            objects = instruction.getOpObjects(operand_index)
            if not any(isinstance(obj, Scalar) for obj in objects):
                continue
            refs = instruction.getOperandReferences(operand_index)
            targets = sorted(f"0x{int(ref.getToAddress().getOffset()):016X}" for ref in refs)
            rows.append(
                (
                    int(instruction.getMinAddress().getOffset()),
                    operand_index,
                    str(instruction.getDefaultOperandRepresentation(operand_index)),
                    ", ".join(targets),
                )
            )
    return sorted(rows)


def render(input_path: Path, enabled: list[str], rows) -> str:
    """Render observed scalar operands and references without hard-coded addresses."""
    lines = [
        "# Scalar Operand References Behavioral Fixture",
        "",
        "> Generated from actual Ghidra instruction and operand-reference state.",
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
    lines.extend(["", "## Scalar Operands", "", "| Instruction | Operand index | Text | Operand references |", "| --- | --- | --- | --- |"])
    lines.extend(f"| `0x{address:016X}` | `{index}` | `{text}` | `{targets}` |" for address, index, text, targets in rows)
    return "\n".join(lines) + "\n"


def main() -> int:
    """Run the analysis in a temporary project and cleanly save and close it."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_scalar_operand_references.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_scalar_operand_references.md"
    if not input_path.is_file():
        print(f"ERROR: executable does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_scalar_operand_references_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "scalar_operand_references", False)
        program = import_and_reopen(project, input_path)
        prepare_disassembly(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        project.analyze(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, scalar_rows(program)), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    except Exception as error:
        print(f"ERROR: Scalar operand analysis failed: {error}", file=sys.stderr)
        return 1
    finally:
        if project is not None:
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
