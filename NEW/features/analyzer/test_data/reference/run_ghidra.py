"""Generate a reference-analyzer report from observed Ghidra references."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Reference"


def import_and_reopen(project, input_path: Path):
    """Import, save, close, and reopen the executable through project.openProgram."""
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError("Ghidra failed to import the executable")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    return project.openProgram("/", input_path.name, False)


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes without allowing unrelated code analysis."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare reference fixture disassembly")
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
    """Select Reference while explicitly disabling every other boolean analyzer."""
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


def references(program):
    """Extract actual memory and operand references in deterministic source order."""
    rows = []
    iterator = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while iterator.hasNext():
        instruction = iterator.next()
        for reference in instruction.getReferencesFrom():
            if reference.isMemoryReference() and not reference.getReferenceType().isFlow():
                rows.append(
                    (
                        int(instruction.getMinAddress().getOffset()),
                        int(reference.getToAddress().getOffset()),
                        int(reference.getOperandIndex()),
                        str(reference.getReferenceType()),
                        str(reference.getSource()),
                    )
                )
    return sorted(rows)


def render(input_path: Path, enabled: list[str], rows) -> str:
    """Render only observed references and configuration, never guessed expectations."""
    lines = [
        "# Reference Behavioral Fixture",
        "",
        "> Generated from actual Ghidra reference-manager state after analysis.",
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
    lines.extend(["", "## Analyzer-Created Memory References", "", "| Source | Target | Operand | Type | Source kind |", "| --- | --- | --- | --- | --- |"])
    lines.extend(
        f"| `0x{source:016X}` | `0x{target:016X}` | `{operand}` | `{kind}` | `{source_kind}` |"
        for source, target, operand, kind, source_kind in rows
    )
    return "\n".join(lines) + "\n"


def main() -> int:
    """Run the isolated analysis and cleanly save and close its temporary project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_reference.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_reference.md"
    if not input_path.is_file():
        print(f"ERROR: executable does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_reference_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "reference", False)
        program = import_and_reopen(project, input_path)
        prepare_disassembly(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        project.analyze(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, references(program)), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    except Exception as error:
        print(f"ERROR: Reference analysis failed: {error}", file=sys.stderr)
        return 1
    finally:
        if project is not None:
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
