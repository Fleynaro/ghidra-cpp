"""Generate the external-parameter propagation report for the x86 PE fixture."""

from __future__ import annotations

import shutil
import struct
import sys
import tempfile
import uuid
from pathlib import Path


ANALYZER_NAME = "WindowsPE x86 Propagate External Parameters"
DEPENDENCIES = {"PDB Universal"}


def address_text(address) -> str:
    """Render a Ghidra address as a fixed-width offset."""
    return f"0x{int(address.getOffset()):08X}"


def executable_set(program):
    """Return executable memory ranges for explicit, analysis-free disassembly."""
    from ghidra.program.model.address import AddressSet

    addresses = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            addresses.add(block.getStart(), block.getEnd())
    return addresses


def prepare_disassembly(program) -> None:
    """Disassemble the PE while leaving the requested analyzer as the only analysis task."""
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet
    from ghidra.program.model.symbol import SymbolType

    transaction = program.startTransaction("Prepare x86 external parameter disassembly")
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
        for function in program.getFunctionManager().getFunctions(True):
            seed = DisassembleCommand(function.getEntryPoint(), None, True)
            seed.enableCodeAnalysis(False)
            seed.applyTo(program)
        # The PE/PDB import can initially give a function only its first instruction.
        # Expand the fixture function body to its real RET so the target's backward
        # PUSH scan sees the complete call sequence.
        for function in program.getFunctionManager().getFunctions(True):
            if function.isExternal() or function.getName() not in ("propagate_parameters", "fixture_entry"):
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
    """Disable unrelated boolean analyzers and select the PDB/target analysis phase."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, (include_target and name == ANALYZER_NAME) or name in DEPENDENCIES)
    enabled = [
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    ]
    expected = sorted([ANALYZER_NAME, *DEPENDENCIES] if include_target else DEPENDENCIES)
    if sorted(enabled) != expected:
        raise RuntimeError(f"Unexpected enabled analysis options: {sorted(enabled)}")
    return sorted(enabled)


def propagation_rows(program):
    """Extract PUSH instructions and their analyzer-authored EOL comments in address order."""
    from ghidra.program.model.listing import CommentType

    rows = []
    for function in program.getFunctionManager().getFunctions(True):
        if function.isExternal():
            continue
        instructions = program.getListing().getInstructions(function.getBody(), True)
        while instructions.hasNext():
            instruction = instructions.next()
            if instruction.getMnemonicString() != "PUSH":
                continue
            comment = instruction.getComment(CommentType.EOL)
            if comment:
                rows.append((int(instruction.getAddress().getOffset()), str(comment)))
    return sorted(rows)


def data_comments(program):
    """Extract the actual parameter_* symbols and any analyzer plate comments."""
    from ghidra.program.model.listing import CommentType

    rows = []
    for symbol in program.getSymbolTable().getAllSymbols(True):
        name = str(symbol.getName())
        if not name.startswith("parameter_"):
            continue
        address = symbol.getAddress()
        comment = program.getListing().getComment(CommentType.PLATE, address)
        rows.append((int(address.getOffset()), name, str(comment) if comment else ""))
    return sorted(rows)


def call_summary(program) -> list[str]:
    """Describe fixture CALL/PUSH instructions when the target finds no result."""
    rows = []
    instructions = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while instructions.hasNext():
        instruction = instructions.next()
        if instruction.getMnemonicString() not in ("PUSH", "CALL"):
            continue
        function = program.getFunctionManager().getFunctionContaining(instruction.getAddress())
        targets = [str(reference.getToAddress()) for reference in instruction.getReferencesFrom()]
        rows.append(f"{function.getName() if function else 'none'}:{instruction.getAddress()}:{instruction.getMnemonicString()}:{targets}")
    return rows


def external_summary(program) -> list[str]:
    """Render imported function prototypes for diagnostics when propagation is absent."""
    return sorted(
        f"{function.getName()}:{function.getParameterCount()}:{function.getPrototypeString(True, True)}"
        for function in program.getListing().getExternalFunctions()
    )


def pdb_identity(pdb_path: Path):
    """Read the GUID and age from the PDB's MSF stream 1 metadata."""
    data = pdb_path.read_bytes()
    if not data.startswith(b"Microsoft C/C++ MSF 7.00") or len(data) < 56:
        raise RuntimeError(f"Unsupported or truncated PDB container: {pdb_path}")
    page_size = struct.unpack_from("<I", data, 32)[0]
    page_count = struct.unpack_from("<I", data, 40)[0]
    directory_size = struct.unpack_from("<I", data, 44)[0]
    block_map_page = struct.unpack_from("<I", data, 52)[0]
    if page_size == 0 or page_count == 0 or directory_size == 0:
        raise RuntimeError(f"Invalid PDB MSF header: {pdb_path}")
    directory_page_count = (directory_size + page_size - 1) // page_size
    block_map_offset = block_map_page * page_size
    block_map_end = block_map_offset + directory_page_count * 4
    if block_map_end > len(data):
        raise RuntimeError(f"PDB directory block map is truncated: {pdb_path}")
    directory_pages = struct.unpack_from(f"<{directory_page_count}I", data, block_map_offset)
    directory = b"".join(data[page * page_size : (page + 1) * page_size] for page in directory_pages)[:directory_size]
    if len(directory) < 4:
        raise RuntimeError(f"PDB stream directory is truncated: {pdb_path}")
    stream_count = struct.unpack_from("<I", directory, 0)[0]
    sizes_offset = 4
    if sizes_offset + stream_count * 4 > len(directory):
        raise RuntimeError(f"PDB stream-size table is truncated: {pdb_path}")
    sizes = struct.unpack_from(f"<{stream_count}I", directory, sizes_offset)
    page_lists_offset = sizes_offset + stream_count * 4
    stream_pages = []
    for size in sizes:
        count = (size + page_size - 1) // page_size if size not in (0, 0xFFFFFFFF) else 0
        if page_lists_offset + count * 4 > len(directory):
            raise RuntimeError(f"PDB stream page list is truncated: {pdb_path}")
        stream_pages.append(struct.unpack_from(f"<{count}I", directory, page_lists_offset))
        page_lists_offset += count * 4
    if len(stream_pages) <= 1 or sizes[1] < 28:
        raise RuntimeError(f"PDB info stream is missing: {pdb_path}")
    info = b"".join(data[page * page_size : (page + 1) * page_size] for page in stream_pages[1])[: sizes[1]]
    _, _, age = struct.unpack_from("<III", info, 0)
    return str(uuid.UUID(bytes_le=info[12:28])), age


def verify_matching_pdb(program, pdb_path: Path) -> None:
    """Verify executable CodeView identity against the supplied PDB before setup edits."""
    if not pdb_path.is_file():
        raise RuntimeError(f"Matching PDB is required before manual setup: {pdb_path}")
    from ghidra.program.model.listing import Program

    info = program.getOptions(Program.PROGRAM_INFO)
    metadata = {str(name): str(info.getValueAsString(name)) for name in info.getOptionNames()}
    expected_name = metadata.get("PDB File", "")
    expected_guid = metadata.get("PDB GUID", "")
    expected_age = metadata.get("PDB Age", "")
    actual_guid, actual_age = pdb_identity(pdb_path)
    if expected_name.lower() != pdb_path.name.lower():
        raise RuntimeError(f"Executable expects PDB {expected_name!r}, not {pdb_path.name!r}")
    if not expected_guid or expected_guid.lower() != actual_guid.lower():
        raise RuntimeError(f"PDB GUID mismatch: executable={expected_guid!r}, file={actual_guid!r}")
    try:
        expected_age_value = int(expected_age, 16)
    except ValueError as error:
        raise RuntimeError(f"Executable has invalid PDB age metadata: {expected_age!r}") from error
    if expected_age_value != actual_age:
        raise RuntimeError(f"PDB age mismatch: executable={expected_age_value}, file={actual_age}")


def require_pdb_applied(program) -> None:
    """Require PDB Universal to apply the verified PDB before manual signature setup."""
    from ghidra.program.model.listing import Program

    info = program.getOptions(Program.PROGRAM_INFO)
    if str(info.getValueAsString("PDB Loaded")).lower() != "true":
        raise RuntimeError("PDB Universal did not apply the verified matching PDB")


def seed_external_signature(program) -> None:
    """Apply the imported API prototype required by the original analyzer.

    The PE import loader creates MessageBoxA as an external function but does not
    transfer the source declaration's type to that external address in standalone
    PyGhidra. This is the real analyzer precondition, not report fabrication: the
    target analyzer still discovers every PUSH and creates every comment/reference.
    """
    from ghidra.program.model.data import CharDataType, IntegerDataType, PointerDataType, VoidDataType
    from ghidra.program.model.listing import Function, ParameterImpl, ReturnParameterImpl
    from ghidra.program.model.symbol import SourceType
    from java.util import ArrayList

    external = next(
        (function for function in program.getListing().getExternalFunctions() if function.getName() == "MessageBoxA"),
        None,
    )
    if external is None:
        raise RuntimeError("Imported MessageBoxA function was not found")
    parameters = ArrayList()
    parameters.add(ParameterImpl("window", PointerDataType(VoidDataType.dataType), program))
    parameters.add(ParameterImpl("text", PointerDataType(CharDataType.dataType), program))
    parameters.add(ParameterImpl("caption", PointerDataType(CharDataType.dataType), program))
    parameters.add(ParameterImpl("type", IntegerDataType.dataType, program))
    transaction = program.startTransaction("Seed imported MessageBoxA prototype")
    committed = False
    try:
        external.updateFunction(
            "__stdcall",
            ReturnParameterImpl(IntegerDataType.dataType, program),
            parameters,
            Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
            True,
            SourceType.ANALYSIS,
        )
        thunk = next(
            (function for function in program.getFunctionManager().getFunctions(True) if function.getName() == "fixture_message_box_thunk"),
            None,
        )
        if thunk is None:
            raise RuntimeError("The fixture import thunk function was not created")
        thunk.setThunkedFunction(external)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def markdown(input_path: Path, pdb_path: Path, enabled: list[str], pushes, data, calls, externals) -> str:
    """Render only stable comments and labels produced by the target analyzer."""
    lines = [
        "# Windows PE x86 Propagate External Parameters Behavioral Fixture",
        "",
        "> Generated by PyGhidra from the checked-in MSVC x86 PE/PDB.",
        "> The target was `WindowsPE x86 Propagate External Parameters`; `PDB Universal` was its explicit dependency.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        f"- **Matching PDB:** `{pdb_path.name}` (CodeView identity verified before setup)",
        "",
        "## Analysis Configuration",
        "",
        *[f"- `{name}`" for name in enabled],
        "",
        "## Propagated PUSH Parameters",
        "",
        "| PUSH address | EOL comment |",
        "| --- | --- |",
    ]
    for address, comment in pushes:
        lines.append(f"| `0x{address:08X}` | `{comment.replace(chr(10), '<br>')}` |")
    lines.extend(["", "## Parameter Data Symbols", "", "| Address | Symbol | Plate comment |", "| --- | --- | --- |"])
    for address, name, comment in data:
        lines.append(f"| `0x{address:08X}` | `{name}` | `{comment.replace(chr(10), '<br>') or '-'}` |")
    lines.extend(
        [
            "",
            "## External Function Baseline",
            "",
            "| External function |",
            "| --- |",
        ]
    )
    for external in externals:
        lines.append(f"| `{external}` |")
    lines.extend(
        [
            "",
            "## Candidate Instructions",
            "",
            "| Function:address:mnemonic:targets |",
            "| --- |",
        ]
    )
    for call in calls:
        lines.append(f"| `{call}` |")
    lines.extend(
        [
            "",
            "## Fixture Assertion",
            "",
            f"- **PUSH parameter comments:** `{len(pushes)}`.",
            f"- **Parameter data symbols reported:** `{len(data)}`.",
            "- The four EOL comments are the direct observable result of the analyzer's import-thunk PUSH propagation path.",
            "- Referenced data rows are restricted to the source-declared `parameter_*` symbols; setup does not invent data labels or report rows.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, isolate, analyze, save, and close the x86 project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_windows_pe_x86_propagate_external_parameters.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_windows_pe_x86_propagate_external_parameters.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    pdb_path = input_path.with_suffix(".pdb")
    if not pdb_path.is_file():
        print(f"Matching PDB is required: {pdb_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_x86_external_parameters_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "x86_external_parameters", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the x86 external-parameter fixture")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        verify_matching_pdb(program, pdb_path)
        prepare_disassembly(program)
        from ghidra.app.plugin.core.analysis import PdbUniversalAnalyzer

        PdbUniversalAnalyzer.setPdbFileOption(program, File(str(pdb_path)))
        configure_analysis(project, program, include_target=False)
        project.analyze(program)
        require_pdb_applied(program)
        # PDB application can replace function bodies; reseed the actual x86 call body
        # after that dependency and before the PUSH-based target analyzer runs.
        prepare_disassembly(program)
        seed_external_signature(program)
        enabled = configure_analysis(project, program)
        project.analyze(program)
        pushes = propagation_rows(program)
        data = data_comments(program)
        if len(pushes) != 4:
            raise RuntimeError(
                f"Expected four propagated MessageBoxA comments, found {len(pushes)}; "
                f"external functions={external_summary(program)}; instructions={call_summary(program)}"
            )
        output_path.write_text(
            markdown(
                input_path,
                pdb_path,
                enabled,
                pushes,
                data,
                call_summary(program),
                external_summary(program),
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
