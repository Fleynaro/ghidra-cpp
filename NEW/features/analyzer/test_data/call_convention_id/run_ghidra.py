#!/usr/bin/env python3
"""Generate the behavioral report for the Call Convention ID fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "Call Convention ID"
DEPENDENCIES: tuple[str, ...] = ()
TARGET_NAME = "convention_target"


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated boolean analyzers and enable the decompiler target explicitly."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    wanted = {ANALYZER_NAME, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in wanted)
    options.setInt(ANALYZER_NAME + ".Analysis Decompiler Timeout (sec)", 30)
    return sorted(str(name) for name in options.getOptionNames() if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def find_target(program):
    """Find the exported fixture function by symbol name and require its function body."""
    symbols = program.getSymbolTable().getAllSymbols(True)
    names = []
    while symbols.hasNext():
        symbol = symbols.next()
        names.append(str(symbol.getName(False)))
        if str(symbol.getName(False)) == TARGET_NAME:
            function = program.getFunctionManager().getFunctionAt(symbol.getAddress())
            if function is None:
                from ghidra.app.cmd.function import CreateFunctionCmd
                CreateFunctionCmd(symbol.getAddress()).applyTo(program)
                function = program.getFunctionManager().getFunctionAt(symbol.getAddress())
            if function is not None:
                return function
    raise RuntimeError(f"Exported target function was not found: {TARGET_NAME}; symbols={names[:30]}")


def prepare_exported_functions(program) -> None:
    """Disassemble executable bytes and create functions at PE function exports."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet
    from ghidra.program.model.symbol import SymbolType

    executable = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            executable.add(block.getStart(), block.getEnd())
    transaction = program.startTransaction("Prepare convention fixture functions")
    committed = False
    try:
        command = DisassembleCommand(executable, executable, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Fixture disassembly failed: {command.getStatusMsg()}")
        symbols = program.getSymbolTable().getAllSymbols(True)
        while symbols.hasNext():
            symbol = symbols.next()
            address = symbol.getAddress()
            if symbol.getSymbolType() == SymbolType.FUNCTION and executable.contains(address):
                if program.getFunctionManager().getFunctionAt(address) is None:
                    CreateFunctionCmd(address).applyTo(program)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def prepare_unknown_signature(program) -> None:
    """Make the target eligible using the exact unknown-convention and defined-type predicates."""
    from ghidra.program.model.data import DWordDataType
    from ghidra.program.model.listing import ParameterImpl
    from ghidra.program.model.symbol import SourceType
    from ghidra.program.model.listing import Function

    function = find_target(program)
    transaction = program.startTransaction("Prepare unknown calling convention fixture")
    committed = False
    try:
        function.setCallingConvention(Function.UNKNOWN_CALLING_CONVENTION_STRING)
        function.addParameter(ParameterImpl("first", DWordDataType.dataType, program), SourceType.USER_DEFINED)
        function.addParameter(ParameterImpl("second", DWordDataType.dataType, program), SourceType.USER_DEFINED)
        function.addParameter(ParameterImpl("third", DWordDataType.dataType, program), SourceType.USER_DEFINED)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def function_state(program) -> tuple[str, str, int, str]:
    """Return the target's stable name, convention, parameter count, and signature source."""
    function = find_target(program)
    return (str(function.getName()), str(function.getCallingConventionName()), int(function.getParameterCount()), str(function.getSignatureSource()))


def write_report(output_path: Path, input_path: Path, enabled: list[str], before, after) -> None:
    """Write the configured eligibility and recovered convention observation."""
    lines = [
        "# Call Convention ID Behavioral Fixture", "", "> Generated automatically with PyGhidra.", "",
        "## Input", "", f"- **File:** `{input_path.name}`", f"- **File size:** `{input_path.stat().st_size}` bytes", "",
        "## Analysis Configuration", "", f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", "- **Target eligibility:** `unknown convention, three defined DWord parameters, non-custom storage`", "",
        "## Convention Observation", "", "| Stage | Function | Calling convention | Parameter count | Signature source |", "| --- | --- | --- | --- | --- |",
        f"| Before analysis | `{before[0]}` | `{before[1]}` | `{before[2]}` | `{before[3]}` |",
        f"| After analysis | `{after[0]}` | `{after[1]}` | `{after[2]}` | `{after[3]}` |",
        "", f"- **Convention changed:** `{str(before[1] != after[1]).lower()}`.", "",
    ]
    output_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Create, save, reopen, prepare, analyze, report, and cleanly close the project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_call_convention_id.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_call_convention_id.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File
    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_call_convention_id_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "call_convention_id", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = GhidraProject.openProject(str(project_parent), "call_convention_id", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        prepare_exported_functions(program)
        prepare_unknown_signature(program)
        before = function_state(program)
        enabled = configure_analysis(project, program)
        if ANALYZER_NAME not in enabled:
            raise RuntimeError(f"Target analyzer was not enabled: {enabled}")
        project.analyze(program)
        write_report(output_path, input_path, enabled, before, function_state(program))
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
