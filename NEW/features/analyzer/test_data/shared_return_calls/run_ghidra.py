"""Generate a flow-override report for Shared Return Calls."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Shared Return Calls"


def import_and_reopen(project, input_path: Path):
    """Import and save once, then reopen the executable via project.openProgram."""
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError("Ghidra failed to import the executable")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    return project.openProgram("/", input_path.name, False)


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes without submitting ordinary code-analysis work."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare shared-return fixture disassembly")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def seed_destination_functions(program) -> None:
    """Create functions at actual unconditional jump targets required by the analyzer contract."""
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet

    targets = set()
    iterator = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while iterator.hasNext():
        instruction = iterator.next()
        if not instruction.getFlowType().isJump() or instruction.getFlowType().isConditional():
            continue
        for reference in instruction.getReferencesFrom():
            if reference.getReferenceType().isJump():
                targets.add(reference.getToAddress())
    for target in sorted(targets, key=lambda address: int(address.getOffset())):
        if program.getFunctionManager().getFunctionAt(target) is None:
            CreateFunctionCmd(target).applyTo(program)


def configure_analysis(project, program) -> list[str]:
    """Disable every boolean analyzer and explicitly enable Shared Return Calls."""
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


def flow_rows(program):
    """Capture jump targets and flow overrides before/after analysis for comparison."""
    rows = []
    iterator = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while iterator.hasNext():
        instruction = iterator.next()
        override = str(instruction.getFlowOverride())
        if not instruction.getFlowType().isJump() and "CALL_RETURN" not in override:
            continue
        for reference in instruction.getReferencesFrom():
            if reference.getReferenceType().isJump() or reference.getReferenceType().isCall():
                rows.append(
                    (
                        int(instruction.getMinAddress().getOffset()),
                        int(reference.getToAddress().getOffset()),
                        override,
                    )
                )
    return sorted(rows)


def render(input_path: Path, enabled: list[str], before, after) -> str:
    """Render observed jump/override state and identify actual CALL_RETURN changes."""
    before_map = {(source, target): override for source, target, override in before}
    lines = [
        "# Shared Return Calls Behavioral Fixture",
        "",
        "> Generated from actual Ghidra flow state. Destination functions were seeded only because the Java contract requires existing functions.",
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
    lines.extend(["", "## Jump Flow Before and After", "", "| Source | Target | Before | After |", "| --- | --- | --- | --- |"])
    for source, target, override in after:
        lines.append(f"| `0x{source:016X}` | `0x{target:016X}` | `{before_map.get((source, target), 'not present')}` | `{override}` |")
    changed = sum(1 for source, target, override in after if before_map.get((source, target)) != override and "CALL_RETURN" in override)
    lines.extend(["", "## Observed Analyzer Effect", "", f"- **Jump rows changed to CALL_RETURN:** `{changed}`", ""])
    return "\n".join(lines)


def main() -> int:
    """Run the isolated analyzer and cleanly save and close its temporary project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_shared_return_calls.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_shared_return_calls.md"
    if not input_path.is_file():
        print(f"ERROR: executable does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_shared_return_calls_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "shared_return_calls", False)
        program = import_and_reopen(project, input_path)
        prepare_disassembly(program)
        seed_destination_functions(program)
        before = flow_rows(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        project.analyze(program)
        after = flow_rows(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, before, after), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    except Exception as error:
        print(f"ERROR: Shared Return Calls analysis failed: {error}", file=sys.stderr)
        return 1
    finally:
        if project is not None:
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
