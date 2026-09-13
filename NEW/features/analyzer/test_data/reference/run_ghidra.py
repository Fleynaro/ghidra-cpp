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


def keyed_delta(before, after):
    """Return exact added, removed, and changed reference rows by stable identity."""
    before_map = {(row[0], row[1], row[2], row[3]): row for row in before}
    after_map = {(row[0], row[1], row[2], row[3]): row for row in after}
    added = sorted(after_map[key] for key in set(after_map) - set(before_map))
    removed = sorted(before_map[key] for key in set(before_map) - set(after_map))
    changed = sorted(
        (before_map[key], after_map[key])
        for key in set(before_map) & set(after_map)
        if before_map[key] != after_map[key]
    )
    return added, removed, changed


def render_reference_rows(title: str, rows, provenance: str) -> list[str]:
    """Render one deterministic reference snapshot with its phase provenance."""
    lines = [f"### {title}", "", "| Source | Target | Operand | Type | Ghidra source | Provenance |", "| --- | --- | --- | --- | --- | --- |"]
    lines.extend(
        f"| `0x{source:016X}` | `0x{target:016X}` | `{operand}` | `{kind}` | `{source_kind}` | {provenance} |"
        for source, target, operand, kind, source_kind in rows
    )
    if not rows:
        lines.append("| _(none)_ | | | | | |")
    return lines


def render(input_path: Path, enabled: list[str], before, after) -> str:
    """Render before/after references and exact reference-manager deltas."""
    added, removed, changed = keyed_delta(before, after)
    lines = [
        "# Reference Behavioral Fixture",
        "",
        "> Generated from actual Ghidra reference-manager snapshots around the target analyzer.",
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
    lines.extend(render_reference_rows("References visible before target analysis", before, "Pre-existing disassembler reference"))
    lines.extend(["", "## After target analysis"])
    lines.extend(render_reference_rows("References visible after target analysis", after, "Post-target reference state"))
    lines.extend(["", "## Delta", "", "The delta is keyed by source, target, operand, and reference type; a changed Ghidra source is reported as changed rather than silently merged.", "", "| Change | Source | Target | Operand | Type | Before source | After source |", "| --- | --- | --- | --- | --- | --- | --- |"])
    for row in added:
        source, target, operand, kind, source_kind = row
        lines.append(f"| Added | `0x{source:016X}` | `0x{target:016X}` | `{operand}` | `{kind}` | | `{source_kind}` |")
    for row in removed:
        source, target, operand, kind, source_kind = row
        lines.append(f"| Removed | `0x{source:016X}` | `0x{target:016X}` | `{operand}` | `{kind}` | `{source_kind}` | |")
    for before_row, after_row in changed:
        source, target, operand, kind, before_source = before_row
        after_source = after_row[4]
        lines.append(f"| Changed | `0x{source:016X}` | `0x{target:016X}` | `{operand}` | `{kind}` | `{before_source}` | `{after_source}` |")
    if not added and not removed and not changed:
        lines.append("| _(none)_ | | | | | | |")
    lines.extend([
        "",
        "### Delta conclusion",
        "",
        f"- **References before target analysis:** `{len(before)}`.",
        f"- **References after target analysis:** `{len(after)}`.",
        f"- **References added:** `{len(added)}`; removed: `{len(removed)}`; changed: `{len(changed)}`.",
        "- `DEFAULT` rows present in both snapshots are loader/disassembler artifacts, not Reference-analyzer deltas.",
    ])
    return "\n".join(lines).rstrip() + "\n"


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
        before = references(program)
        project.analyze(program)
        after = references(program)
        project.save(program)
        output_path.write_text(render(input_path, enabled, before, after), encoding="utf-8", newline="\n")
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
