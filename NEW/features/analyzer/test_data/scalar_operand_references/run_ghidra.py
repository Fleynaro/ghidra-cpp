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
    """Extract scalar values, references, and the observed acceptance outcome."""
    from ghidra.program.model.scalar import Scalar

    rows = []
    iterator = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while iterator.hasNext():
        instruction = iterator.next()
        for operand_index in range(instruction.getNumOperands()):
            objects = instruction.getOpObjects(operand_index)
            if not any(isinstance(obj, Scalar) for obj in objects):
                continue
            scalar = next(obj for obj in objects if isinstance(obj, Scalar))
            refs = instruction.getOperandReferences(operand_index)
            targets = sorted(f"0x{int(ref.getToAddress().getOffset()):016X}" for ref in refs)
            value = int(scalar.getUnsignedValue())
            if targets:
                outcome = "Analyzer-created address reference"
            elif value < 4096:
                outcome = "Rejected small numeric control"
            else:
                outcome = "Rejected address-like control"
            rows.append(
                (
                    int(instruction.getMinAddress().getOffset()),
                    operand_index,
                    str(instruction.getDefaultOperandRepresentation(operand_index)),
                    value,
                    ", ".join(targets),
                    outcome,
                )
            )
    return sorted(rows)


def render(input_path: Path, enabled: list[str], before, after) -> str:
    """Render before/after scalar outcomes, including positive and negative controls."""
    lines = [
        "# Scalar Operand References Behavioral Fixture",
        "",
        "> Generated from actual Ghidra instruction and operand-reference snapshots.",
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
    lines.extend([
        "",
        "## Scalar Operands",
        "",
        "| Phase | Instruction | Operand index | Text | Unsigned scalar | Operand references | Outcome |",
        "| --- | --- | --- | --- | ---: | --- | --- |",
    ])
    for phase, rows in (("Before", before), ("After", after)):
        lines.extend(
            f"| `{phase}` | `0x{address:016X}` | `{index}` | `{text}` | `0x{value:016X}` | `{targets}` | `{outcome}` |"
            for address, index, text, value, targets, outcome in rows
        )
    positive = sum(1 for row in after if row[4])
    negative = sum(1 for row in after if not row[4])
    lines.extend([
        "",
        "## Fixture Assertions",
        "",
        f"- **Positive analyzer references after analysis:** `{positive}`.",
        f"- **Negative controls without references after analysis:** `{negative}`.",
        "- Positive values are fixed image addresses; negative rows retain the rejected address-like and small numeric controls.",
        "",
    ])
    return "\n".join(lines).rstrip() + "\n"


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
        before = scalar_rows(program)
        project.analyze(program)
        after = scalar_rows(program)
        if not any(row[4] for row in after):
            raise RuntimeError(f"No positive scalar operand reference was created: before={before}, after={after}")
        if not any(not row[4] for row in after):
            raise RuntimeError(f"No negative scalar control was retained: before={before}, after={after}")
        project.save(program)
        output_path.write_text(render(input_path, enabled, before, after), encoding="utf-8", newline="\n")
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
