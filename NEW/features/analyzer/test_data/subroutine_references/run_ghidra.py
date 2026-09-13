#!/usr/bin/env python3
"""Generate the Ghidra behavioral report for the Subroutine References fixture."""

from __future__ import annotations

import sys
import shutil
import tempfile
from pathlib import Path


ANALYZER_NAME = "Subroutine References"


def address_text(address) -> str:
    """Return a fixed-width, image-relative-independent Ghidra address string."""
    return f"0x{int(address.getOffset()):016X}"


def sorted_functions(program):
    """Return all non-external functions sorted by their entry offsets."""
    functions = [
        function
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal()
    ]
    return sorted(functions, key=lambda function: int(function.getEntryPoint().getOffset()))


def function_entries(program) -> set[int]:
    """Capture non-external function entries for the before/after comparison."""
    return {
        int(function.getEntryPoint().getOffset())
        for function in sorted_functions(program)
    }


def function_rows(program) -> list[tuple[int, str]]:
    """Snapshot non-external function entries and bodies at the target boundary."""
    return [
        (int(function.getEntryPoint().getOffset()), body_ranges(function))
        for function in sorted_functions(program)
    ]


def body_ranges(function) -> str:
    """Render every address range in a function body in stable ascending order."""
    ranges = []
    for address_range in function.getBody().getAddressRanges():
        ranges.append(
            f"{address_text(address_range.getMinAddress())}-"
            f"{address_text(address_range.getMaxAddress())}"
        )
    return ", ".join(sorted(ranges))


def direct_calls(program) -> list[tuple[int, int, str, str, bool]]:
    """Extract only direct call references and their fall-through classification."""
    listing = program.getListing()
    reference_manager = program.getReferenceManager()
    executable = program.getMemory().getExecuteSet()
    rows = []
    instructions = listing.getInstructions(executable, True)
    while instructions.hasNext():
        instruction = instructions.next()
        if not instruction.getFlowType().isCall():
            continue
        source = instruction.getMinAddress()
        fall_through = instruction.getFallThrough()
        for reference in reference_manager.getFlowReferencesFrom(source):
            if not reference.getReferenceType().isCall():
                continue
            target = reference.getToAddress()
            rows.append(
                (
                    int(source.getOffset()),
                    int(target.getOffset()),
                    str(reference.getReferenceType()),
                    address_text(fall_through) if fall_through is not None else "none",
                    fall_through is not None and fall_through == target,
                )
            )
    return sorted(rows)


def address_set_for_executable(program):
    """Return all executable memory ranges used as explicit disassembly seeds."""
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    return executable


def prepare_disassembly(program) -> None:
    """Disassemble executable bytes without submitting code-analysis work."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    transaction = program.startTransaction("Prepare fixture disassembly")
    committed = False
    try:
        executable = address_set_for_executable(program)
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        for function in sorted_functions(program):
            seed_command = DisassembleCommand(function.getEntryPoint(), None, True)
            seed_command.enableCodeAnalysis(False)
            seed_command.applyTo(program)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def configure_analysis(project, program) -> list[str]:
    """Disable every boolean analysis option and enable only the target analyzer."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for option_name in options.getOptionNames():
        if options.getType(option_name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(option_name, option_name == ANALYZER_NAME)

    enabled = []
    for option_name in options.getOptionNames():
        if options.getType(option_name) == OptionType.BOOLEAN_TYPE and options.getBoolean(
            option_name, False
        ):
            enabled.append(str(option_name))
    return sorted(enabled)


def delta_section(before, after) -> list[str]:
    """Describe function rows added, removed, or changed by Subroutine References."""
    before_map = {row[0]: row for row in before}
    after_map = {row[0]: row for row in after}
    added = [after_map[key] for key in sorted(set(after_map) - set(before_map))]
    removed = [before_map[key] for key in sorted(set(before_map) - set(after_map))]
    changed = [
        (before_map[key], after_map[key])
        for key in sorted(set(before_map) & set(after_map))
        if before_map[key] != after_map[key]
    ]
    if not added and not removed and not changed:
        return ["## Delta", "", "No changes observed", ""]

    def row_text(row) -> str:
        """Render one function snapshot row for delta evidence."""
        return f"`0x{row[0]:016X}` body `{row[1]}`"

    lines = ["## Delta", "", "**Added rows**", ""]
    if added:
        lines.extend(f"- {row_text(row)}" for row in added)
    else:
        lines.append("- None")
    lines.extend(["", "**Removed rows**", ""])
    if removed:
        lines.extend(f"- {row_text(row)}" for row in removed)
    else:
        lines.append("- None")
    lines.extend(["", "**Changed rows**", ""])
    if changed:
        lines.extend(f"- Before {row_text(old)}; after {row_text(new)}" for old, new in changed)
    else:
        lines.append("- None")
    lines.append("")
    return lines


def markdown(program, input_path: Path, enabled: list[str], before: set[int], before_rows, after_rows, calls) -> str:
    """Render only call references and function state relevant to the analyzer."""
    after_functions = sorted_functions(program)
    after = {entry for entry, _ in after_rows}
    created = sorted(after - before)

    lines = [
        "# Subroutine References Behavioral Fixture",
        "",
        "> Generated automatically from the MSVC x64 fixture using PyGhidra.",
        "> All analysis options were disabled except `Subroutine References`.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean analysis options |",
        "| --- |",
    ]
    for option_name in enabled:
        lines.append(f"| `{option_name}` |")

    target_counts = {}
    for _, target, _, _, _ in calls:
        target_counts[target] = target_counts.get(target, 0) + 1
    created_set = set(created)
    max_callers = max(target_counts.values(), default=0)

    lines.extend(
        [
            "",
            "## Direct Call References",
            "",
            "| Call site | Call target | Reference type | Fall-through | Function before analysis |",
            "| --- | --- | --- | --- | --- |",
        ]
    )
    for source, target, reference_type, fall_through, _ in calls:
        lines.append(
            f"| `{address_text(program.getAddressFactory().getDefaultAddressSpace().getAddress(source))}` "
            f"| `{address_text(program.getAddressFactory().getDefaultAddressSpace().getAddress(target))}` "
            f"| `{reference_type}` | `{fall_through}` | `{str(target in before).lower()}` |"
        )

    lines.extend(
        [
            "",
            "## Before target analysis",
            "",
            "| Entry address | Body ranges |",
            "| --- | --- |",
        ]
    )
    for entry, body in before_rows:
        lines.append(
            f"| `{address_text(program.getAddressFactory().getDefaultAddressSpace().getAddress(entry))}` | `{body}` |"
        )

    lines.extend(
        [
            "",
            "## Functions Created By Subroutine References",
            "",
            "| Entry address | Body ranges |",
            "| --- | --- |",
        ]
    )
    for entry in created:
        function = program.getFunctionManager().getFunctionAt(
            program.getAddressFactory().getDefaultAddressSpace().getAddress(entry)
        )
        lines.append(f"| `{address_text(function.getEntryPoint())}` | `{body_ranges(function)}` |")

    lines.extend(
        [
            "",
            "## After target analysis",
            "",
            "| Entry address | Body ranges |",
            "| --- | --- |",
        ]
    )
    for entry, body in after_rows:
        lines.append(
            f"| `{address_text(program.getAddressFactory().getDefaultAddressSpace().getAddress(entry))}` | `{body}` |"
        )

    lines.extend([""] + delta_section(before_rows, after_rows))

    lines.extend(
        [
            "",
            "## Fixture Assertions",
            "",
            f"- **Initial non-external functions:** `{len(before)}`; **created entries:** `{len(created)}`.",
            f"- **Maximum direct callers of one target:** `{max_callers}`.",
            f"- **Known direct-call targets not newly created:** `{len({target for _, target, _, _, _ in calls if target in before and target not in created_set})}`.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    """Import the fixture into a temporary project, analyze it, and write the report."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_subroutine_references.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_subroutine_references.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_function_analyzer_"))
    project_name = "subroutine_references"
    try:
        project = GhidraProject.createProject(str(project_parent), project_name, True)
        program = project.importProgram(File(str(input_path)))
        if program is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(program, "/", input_path.name, True)
        project.close(program)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        prepare_disassembly(program)
        before = function_entries(program)
        enabled = configure_analysis(project, program)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        calls_before_analysis = direct_calls(program)
        print(f"[+] Imported {input_path}")
        print(f"[+] Enabled analysis: {', '.join(enabled)}")
        print(f"[+] Direct calls before analysis: {len(calls_before_analysis)}")
        # Disassembly, call inventory, and target option configuration are complete at this boundary.
        before_rows = function_rows(program)
        project.analyze(program)
        after_rows = function_rows(program)
        report = markdown(program, input_path, enabled, before, before_rows, after_rows, calls_before_analysis)
        project.save(program)
        output_path.write_text(report, encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if "project" in locals():
            if "program" in locals() and program is not None:
                project.close(program)
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
