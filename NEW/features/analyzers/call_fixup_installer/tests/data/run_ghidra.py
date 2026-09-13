#!/usr/bin/env python3
"""Generate the behavioral report for the Call-Fixup Installer fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "Call-Fixup Installer"
DEPENDENCIES: tuple[str, ...] = ()


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and enable the call-fixup target explicitly."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    wanted = {ANALYZER_NAME, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in wanted)
    return sorted(str(name) for name in options.getOptionNames() if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def prepare_exported_functions(program) -> None:
    """Create functions at the compiler-spec target and fixture entry exports."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare call-fixup fixture functions")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Fixture disassembly failed: {command.getStatusMsg()}")
        symbols = program.getSymbolTable().getAllSymbols(True)
        while symbols.hasNext():
            symbol = symbols.next()
            if str(symbol.getName(False)) not in {"__security_check_cookie", "call_fixup_installer_entry"}:
                continue
            address = symbol.getAddress()
            if program.getFunctionManager().getFunctionAt(address) is None:
                CreateFunctionCmd(address).applyTo(program)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def fixup_rows(program) -> list[tuple[str, str, bool]]:
    """Extract each non-external function's installed fixup and no-return state."""
    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        rows.append((str(function.getName()), str(function.getCallFixup()), bool(function.hasNoReturn())))
    return sorted(rows)


def delta_section(before, after) -> list[str]:
    """Describe installed-fixup rows added, removed, or changed by the target."""
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
        """Render one function fixup row for delta evidence."""
        return " | ".join(f"`{value}`" for value in row)

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


def write_report(output_path: Path, input_path: Path, enabled: list[str], before, after) -> None:
    """Write the target mapping observations and the before/after function state."""
    before_map = {row[0]: row for row in before}

    lines = [
        "# Call-Fixup Installer Behavioral Fixture", "", "> Generated automatically with PyGhidra.", "",
        "## Input", "", f"- **File:** `{input_path.name}`", f"- **File size:** `{input_path.stat().st_size}` bytes", "",
        "## Analysis Configuration", "", f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", "- **Compiler-spec target:** `__security_check_cookie`", "- **Expected payload:** `security_check_cookie`", "",
        "## Before target analysis", "", "| Name | Call fixup | No return |", "| --- | --- | --- |",
    ]
    lines.extend(f"| `{name}` | `{fixup}` | `{no_return}` |" for name, fixup, no_return in before)
    lines.extend(["", "## After target analysis", "", "| Name | Call fixup | No return |", "| --- | --- | --- |"])
    lines.extend(f"| `{name}` | `{fixup}` | `{no_return}` |" for name, fixup, no_return in after)
    lines.extend([""] + delta_section(before, after))
    lines.extend(["", "## Function State", "", "| Name | Call fixup | No return | Changed |", "| --- | --- | --- | --- |"])
    lines.extend(f"| `{name}` | `{fixup}` | `{no_return}` | `{str(before_map.get(name) != row).lower()}` |" for row in after for name, fixup, no_return in [row])
    installed = [row for row in after if row[1] not in ("None", "")]
    lines.extend(["", f"- **Functions with installed call fixups:** `{len(installed)}`.", ""])
    output_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Create, save, reopen, analyze, report, and cleanly close the temporary project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_call_fixup_installer.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_call_fixup_installer.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File
    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_call_fixup_installer_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "call_fixup_installer", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = GhidraProject.openProject(str(project_parent), "call_fixup_installer", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        prepare_exported_functions(program)
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Target analyzer was not enabled: {enabled}")
        # Exported functions and target options are prepared; capture state at the target boundary.
        before = fixup_rows(program)
        project.analyze(program)
        after = fixup_rows(program)
        write_report(output_path, input_path, enabled, before, after)
        project.save(program)
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            if "program" in locals():
                project.save(program)
                project.close(program)
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
