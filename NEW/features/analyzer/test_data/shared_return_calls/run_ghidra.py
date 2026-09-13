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


def function_rows(program):
    """Capture the functions relevant to shared-return target resolution."""
    return sorted(
        (int(function.getEntryPoint().getOffset()), str(function.getName()))
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal()
    )


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


def render_functions(title: str, rows, provenance: str) -> list[str]:
    """Render one function snapshot with provenance."""
    lines = [f"### {title}", "", "| Entry | Name | Provenance |", "| --- | --- | --- |"]
    lines.extend(f"| `0x{entry:016X}` | `{name}` | {provenance} |" for entry, name in rows)
    if not rows:
        lines.append("| _(none)_ | | |")
    return lines


def render_flows(title: str, rows, provenance: str) -> list[str]:
    """Render one jump-flow snapshot with provenance."""
    lines = [f"### {title}", "", "| Source | Target | Flow override | Provenance |", "| --- | --- | --- | --- |"]
    lines.extend(f"| `0x{source:016X}` | `0x{target:016X}` | `{override}` | {provenance} |" for source, target, override in rows)
    if not rows:
        lines.append("| _(none)_ | | | |")
    return lines


def render(input_path: Path, enabled: list[str], before_functions, after_functions, before, after) -> str:
    """Render before/after function and flow state with exact analyzer deltas."""
    function_added, function_removed, function_changed = keyed_delta(before_functions, after_functions, 1)
    flow_added, flow_removed, flow_changed = keyed_delta(before, after, 2)
    lines = [
        "# Shared Return Calls Behavioral Fixture",
        "",
        "> Generated from actual Ghidra function and flow-state snapshots. Destination functions were seeded only because the Java contract requires existing functions.",
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
    lines.extend(render_functions("Functions visible before target analysis", before_functions, "Disassembler/fixture prerequisite"))
    lines.extend(render_flows("Jump flow visible before target analysis", before, "Pre-existing flow state"))
    lines.extend(["", "## After target analysis"])
    lines.extend(render_functions("Functions visible after target analysis", after_functions, "Post-target function state"))
    lines.extend(render_flows("Jump flow visible after target analysis", after, "Post-target flow state"))
    lines.extend(["", "## Delta", "", "Seeded destination functions are prerequisites and are classified as pre-existing. Only the exact function/flow changes below are attributed to Shared Return Calls.", "", "### Function delta", "", "| Change | Entry | Before name | After name |", "| --- | --- | --- | --- |"])
    lines.extend(f"| Added | `0x{entry:016X}` | | `{name}` |" for entry, name in function_added)
    lines.extend(f"| Removed | `0x{entry:016X}` | `{name}` | |" for entry, name in function_removed)
    lines.extend(f"| Changed | `0x{before_entry:016X}` | `{before_name}` | `{after_name}` |" for (before_entry, before_name), (_, after_name) in function_changed)
    if not function_added and not function_removed and not function_changed:
        lines.append("| _(none)_ | | | |")
    lines.extend(["", "### Flow delta", "", "| Change | Source | Target | Before override | After override |", "| --- | --- | --- | --- | --- |"])
    lines.extend(f"| Added | `0x{source:016X}` | `0x{target:016X}` | | `{override}` |" for source, target, override in flow_added)
    lines.extend(f"| Removed | `0x{source:016X}` | `0x{target:016X}` | `{override}` | |" for source, target, override in flow_removed)
    lines.extend(f"| Changed | `0x{source:016X}` | `0x{target:016X}` | `{before_override}` | `{after_override}` |" for (source, target, before_override), (_, _, after_override) in flow_changed)
    if not flow_added and not flow_removed and not flow_changed:
        lines.append("| _(none)_ | | | | |")
    call_return_changes = sum(1 for _, after_row in flow_changed if "CALL_RETURN" in after_row[2])
    lines.extend(["", "### Delta conclusion", "", f"- **Functions added:** `{len(function_added)}`; removed: `{len(function_removed)}`; changed: `{len(function_changed)}`.", f"- **Flow rows added:** `{len(flow_added)}`; removed: `{len(flow_removed)}`; changed: `{len(flow_changed)}`.", f"- **Flow rows changed to `CALL_RETURN`:** `{call_return_changes}`.", ""])
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
        before_functions = function_rows(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        project.analyze(program)
        after = flow_rows(program)
        after_functions = function_rows(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, before_functions, after_functions, before, after), encoding="utf-8", newline="\n")
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
