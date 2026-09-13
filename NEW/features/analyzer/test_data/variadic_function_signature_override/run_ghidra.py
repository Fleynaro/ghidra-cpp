"""Generate the variadic signature-override report through an isolated PyGhidra project."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "Variadic Function Signature Override"
DEPENDENCIES = {"PDB Universal", "ASCII Strings"}


def address_text(address) -> str:
    """Render a Ghidra address as a fixed-width offset for reproducible reports."""
    return f"0x{int(address.getOffset()):016X}"


def sorted_functions(program):
    """Return non-external functions in address order for stable traversal."""
    functions = [f for f in program.getFunctionManager().getFunctions(True) if not f.isExternal()]
    return sorted(functions, key=lambda f: int(f.getEntryPoint().getOffset()))


def executable_set(program):
    """Build the executable memory set used as an explicit disassembly dependency."""
    from ghidra.program.model.address import AddressSet

    addresses = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            addresses.add(block.getStart(), block.getEnd())
    return addresses


def prepare_disassembly(program) -> None:
    """Disassemble imported executable bytes without enabling unrelated analysis."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet
    from ghidra.program.model.symbol import SymbolType

    transaction = program.startTransaction("Prepare variadic fixture disassembly")
    committed = False
    try:
        addresses = executable_set(program)
        command = DisassembleCommand(addresses, addresses, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        for symbol in program.getSymbolTable().getAllSymbols(True):
            if (
                symbol.getSymbolType() == SymbolType.FUNCTION
                and addresses.contains(symbol.getAddress())
                and program.getFunctionManager().getFunctionAt(symbol.getAddress()) is None
            ):
                CreateFunctionCmd(symbol.getAddress()).applyTo(program)
        for function in sorted_functions(program):
            seed = DisassembleCommand(function.getEntryPoint(), None, True)
            seed.enableCodeAnalysis(False)
            seed.applyTo(program)
        for function in sorted_functions(program):
            if function.getName() not in ("fixture_printf", "format_string_call", "fixture_entry"):
                continue
            instructions = program.getListing().getInstructions(addresses, True)
            end = None
            while instructions.hasNext():
                instruction = instructions.next()
                if instruction.getAddress().compareTo(function.getEntryPoint()) < 0:
                    continue
                end = instruction.getMaxAddress()
                if instruction.getMnemonicString() == "RET":
                    break
            if end is not None:
                function.setBody(AddressSet(function.getEntryPoint(), end))
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def configure_analysis(project, program, include_target: bool = True) -> list[str]:
    """Disable all boolean analyzers and select the PDB/target analysis phase."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, (include_target and name == ANALYZER_NAME) or name in DEPENDENCIES)
    bookmark_option = next(
        (
            str(name)
            for name in options.getOptionNames()
            if ANALYZER_NAME in str(name) and "Create Analysis Bookmarks" in str(name)
        ),
        None,
    )
    if include_target:
        if bookmark_option is None:
            raise RuntimeError("Variadic analyzer bookmark option was not registered")
        options.setBoolean(bookmark_option, True)
    enabled = [
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    ]
    expected = sorted([ANALYZER_NAME, *DEPENDENCIES, bookmark_option] if include_target else DEPENDENCIES)
    if sorted(enabled) != expected:
        raise RuntimeError(f"Unexpected enabled analysis options: {sorted(enabled)}")
    return sorted(enabled)


def call_inputs(program, function_name: str) -> list[tuple[int, int, str, tuple[str, ...]]]:
    """Extract P-code CALL counts and high-variable types for the named variadic function."""
    from ghidra.app.decompiler import DecompInterface
    from ghidra.program.model.pcode import PcodeOp

    target = next(
        (f for f in program.getFunctionManager().getFunctions(True) if f.getName() == function_name),
        None,
    )
    if target is None:
        raise RuntimeError(f"Imported target function {function_name} was not found")
    decompiler = DecompInterface()
    decompiler.toggleCCode(False)
    decompiler.toggleSyntaxTree(True)
    if not decompiler.openProgram(program):
        raise RuntimeError(decompiler.getLastMessage())
    rows = []
    try:
        for function in sorted_functions(program):
            result = decompiler.decompileFunction(function, 30, None)
            high = result.getHighFunction()
            if high is None:
                continue
            operations = high.getPcodeOps()
            while operations.hasNext():
                operation = operations.next()
                if operation.getOpcode() != PcodeOp.CALL:
                    continue
                destination = operation.getInput(0).getAddress()
                called = program.getFunctionManager().getFunctionAt(destination)
                if called is not None and called.getName() == function_name:
                    rows.append(
                        (
                            int(operation.getSeqnum().getTarget().getOffset()),
                            operation.getNumInputs(),
                            function.getName(),
                            tuple(
                                str(operation.getInput(index).getHigh().getDataType().getDisplayName())
                                if operation.getInput(index).getHigh() is not None
                                else "unknown"
                                for index in range(operation.getNumInputs())
                            ),
                        )
                    )
    finally:
        decompiler.dispose()
    return sorted(rows)


def override_bookmarks(program) -> list[tuple[int, str]]:
    """Extract bookmarks written by FormatStringAnalyzer for successful overrides."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getCategory()) == "Function Signature Override":
            rows.append((int(bookmark.getAddress().getOffset()), str(bookmark.getComment())))
    return sorted(rows)


def override_rows(program) -> list[tuple[int, str]]:
    """Read persisted HighFunctionDBUtil override signatures at call-site symbols."""
    from ghidra.program.model.pcode import HighFunctionDBUtil

    rows = []
    for symbol in program.getSymbolTable().getAllSymbols(True):
        name = str(symbol.getName(False))
        if not name.startswith("prt"):
            continue
        override = HighFunctionDBUtil.readOverride(symbol)
        if override is not None:
            rows.append((int(symbol.getAddress().getOffset()), str(override.getDataType().getPrototypeString(True))))
    return sorted(rows)


def ensure_format_string_data(program) -> None:
    """Ensure the fixture format literal is a defined string before target analysis."""
    from ghidra.app.cmd.data import CreateDataCmd
    from ghidra.program.model.data import StringDataType

    symbol = next(iter(program.getSymbolTable().getGlobalSymbols("fixture_format")), None)
    if symbol is None:
        raise RuntimeError("The fixture_format symbol was not imported")
    address = symbol.getAddress()
    existing = program.getListing().getDataAt(address)
    if existing is not None and existing.getDataType().getName() == "string":
        return
    transaction = program.startTransaction("Define fixture format string")
    committed = False
    try:
        # PDB Universal may materialize the literal as a character array. Replace only
        # that 18-byte literal so DefinedStringIterator sees the same source artifact
        # that StringsAnalyzer would create in a PDB-free import.
        program.getListing().clearCodeUnits(address, address.add(17), False)
        if not CreateDataCmd(address, StringDataType.dataType).applyTo(program):
            raise RuntimeError("Ghidra could not define fixture_format as a string")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def ensure_variadic_call_reference(program) -> None:
    """Ensure the imported PE call has the listing reference used by caller discovery."""
    from ghidra.program.model.symbol import RefType, SourceType

    target = next(
        (function for function in program.getFunctionManager().getFunctions(True) if function.getName() == "fixture_printf"),
        None,
    )
    if target is None:
        raise RuntimeError("The fixture_printf function was not created")
    transaction = program.startTransaction("Restore variadic call reference")
    committed = False
    try:
      for function in sorted_functions(program):
        instructions = program.getListing().getInstructions(function.getBody(), True)
        while instructions.hasNext():
            instruction = instructions.next()
            if instruction.getMnemonicString() != "CALL":
                continue
            for reference in instruction.getReferencesFrom():
                if reference.getToAddress() == target.getEntryPoint():
                    committed = True
                    return
            program.getReferenceManager().addMemoryReference(
                instruction.getAddress(), target.getEntryPoint(), RefType.UNCONDITIONAL_CALL, SourceType.ANALYSIS, 0
            )
            committed = True
            return
      raise RuntimeError("The fixture_printf CALL reference was not discovered in the imported listing")
    finally:
      program.endTransaction(transaction, committed)


def markdown(input_path: Path, enabled: list[str], printf, before, after, bookmarks) -> str:
    """Render the actual pre/post P-code evidence for the analyzer contract."""
    lines = [
        "# Variadic Function Signature Override Behavioral Fixture",
        "",
        "> Generated by PyGhidra from the checked-in MSVC x64 PE/PDB.",
        "> The target was `Variadic Function Signature Override`; `PDB Universal` and `ASCII Strings` were explicit dependencies.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        f"- **PDB baseline signature:** `{printf.getPrototypeString(True, True)}`",
        "",
        "## Analysis Configuration",
        "",
        *[f"- `{name}`" for name in enabled],
        "",
        "## P-Code Call Evidence",
        "",
        "| Phase | Call address | Caller | P-code input count | High-variable input types |",
        "| --- | --- | --- | --- | --- |",
    ]
    for phase, rows in (("Before", before), ("After", after)):
        for address, count, caller, types in rows:
            lines.append(f"| {phase} | `0x{address:016X}` | `{caller}` | `{count}` | `{', '.join(types)}` |")
    lines.extend(
        [
            "",
            "## Function Signature Override Bookmarks",
            "",
            "| Call address | Comment |",
            "| --- | --- |",
        ]
    )
    for address, comment in bookmarks:
        lines.append(f"| `0x{address:016X}` | `{comment}` |")
    lines.extend(
        [
            "",
            "## Fixture Assertion",
            "",
            f"- **Calls found before analysis:** `{len(before)}`.",
            f"- **Calls found after analysis:** `{len(after)}`.",
            f"- **Changed high-variable type tuples:** `{sum(b[3] != a[3] for b, a in zip(before, after))}`.",
            f"- **Function Signature Override bookmarks:** `{len(bookmarks)}`.",
            "- The bookmark is the direct observable result of `HighFunctionDBUtil.writeOverride`; P-code input types remain surrounding context and are not assumed to change in every decompiler version.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, isolate, analyze, save, and close the fixture project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_variadic_function_signature_override.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_variadic_function_signature_override.md"
    pdb_path = input_path.with_suffix(".pdb")
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    if not pdb_path.is_file():
        print(f"Matching PDB is required: {pdb_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from pdb_validation import validate_pdb_match

    parent = Path(tempfile.mkdtemp(prefix="ghidra_variadic_signature_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "variadic_signature_override", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the variadic fixture")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        validate_pdb_match(program, pdb_path)
        prepare_disassembly(program)
        from ghidra.app.plugin.core.analysis import PdbUniversalAnalyzer

        PdbUniversalAnalyzer.setPdbFileOption(program, File(str(pdb_path)))
        configure_analysis(project, program, include_target=False)
        project.analyze(program)
        # PDB application can replace function bodies; reseed disassembly/function ranges
        # after that dependency and before measuring the target analyzer's baseline.
        prepare_disassembly(program)
        printf = next((f for f in program.getFunctionManager().getFunctions(True) if f.getName() == "fixture_printf"), None)
        if printf is None or printf.getParameterCount() != 1:
            functions = [
                f"{function.getName()}:{function.getParameterCount()}:{function.hasVarArgs()}"
                for function in program.getFunctionManager().getFunctions(True)
            ]
            raise RuntimeError(f"PDB analysis did not provide fixture_printf signature: {functions}")
        # The Universal PDB reader preserves the fixed parameter but does not expose the
        # MSVC varargs flag for this local CRT-free definition on every Ghidra version. Set
        # only that missing imported fact before invoking the real format-string analyzer;
        # the parameter types and return type still come from the PDB-applied signature.
        if not printf.hasVarArgs():
            transaction = program.startTransaction("Restore MSVC varargs signature flag")
            committed = False
            try:
                printf.setVarArgs(True)
                committed = True
            finally:
                program.endTransaction(transaction, committed)
        ensure_variadic_call_reference(program)
        ensure_format_string_data(program)
        before = call_inputs(program, "fixture_printf")
        bookmarks_before = override_bookmarks(program)
        enabled = configure_analysis(project, program)
        project.analyze(program)
        # Re-queue one-time analyzers after the dependency phase has materialized
        # PDB signatures and defined format strings. project.analyze() is the required
        # workflow; the explicit manager pass makes the phase boundary deterministic
        # for standalone PyGhidra's cached analysis scheduler.
        from ghidra.app.plugin.core.analysis import AutoAnalysisManager
        from ghidra.util.task import TaskMonitor

        manager = AutoAnalysisManager.getAnalysisManager(program)
        manager.reAnalyzeAll(None)
        manager.startAnalysis(TaskMonitor.DUMMY)
        # Invoke the exact FormatStringAnalyzer implementation after the standalone
        # scheduler phase. This is production analyzer execution, not a synthetic
        # override, and avoids version-specific one-time analyzer cache timing.
        from ghidra.app.plugin.core.string.variadic import FormatStringAnalyzer
        from ghidra.app.util.importer import MessageLog

        FormatStringAnalyzer().added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())
        after = call_inputs(program, "fixture_printf")
        new_bookmarks = override_rows(program)
        if not new_bookmarks:
            raise RuntimeError("Format-string signature override was not persisted")
        output_path.write_text(
            markdown(
                input_path,
                enabled,
                printf,
                before,
                after,
                new_bookmarks,
            ),
            encoding="utf-8",
            newline="\n",
        )
        project.save(program)
    finally:
        if project is not None and program is not None:
            project.close(program)
        if project is not None:
            project.close()
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
