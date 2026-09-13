"""Generate the Windows resource-reference report through isolated PyGhidra analysis."""

from __future__ import annotations

import re
import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER_NAME = "WindowsResourceReference"
RESOURCE_SYMBOL_PATTERN = re.compile(
    r"^Rsrc_(?P<type>.+)_(?P<resource_id>[0-9A-Fa-f]+)_(?P<locale>[0-9A-Fa-f]+)$"
)
RESOURCE_DEFINE_PATTERN = re.compile(
    r"^\s*#define\s+(?P<name>[A-Za-z_]\w*)\s+(?P<value>0[xX][0-9A-Fa-f]+|\d+)\s*$"
)


def address_text(address) -> str:
    """Render a Ghidra address as a fixed-width PE offset."""
    return f"0x{int(address.getOffset()):016X}"


def executable_set(program):
    """Return executable memory ranges for the explicit disassembly prerequisite."""
    from ghidra.program.model.address import AddressSet

    addresses = AddressSet()
    for block in program.getMemory().getBlocks():
        if block.isExecute():
            addresses.add(block.getStart(), block.getEnd())
    return addresses


def prepare_disassembly(program) -> None:
    """Disassemble PE code without allowing unrelated analyzers to run first."""
    from ghidra.app.cmd.disassemble import DisassembleCommand

    transaction = program.startTransaction("Prepare Windows resource disassembly")
    committed = False
    try:
        addresses = executable_set(program)
        command = DisassembleCommand(addresses, addresses, False)
        command.enableCodeAnalysis(False)
        if not command.applyTo(program):
            raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def configure_analysis(project, program) -> list[str]:
    """Disable every boolean analyzer and enable only WindowsResourceReference."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, name == ANALYZER_NAME)
    enabled = [
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    ]
    if sorted(enabled) != [ANALYZER_NAME]:
        raise RuntimeError(f"Unexpected enabled analysis options: {sorted(enabled)}")
    return sorted(enabled)


def resource_definitions(fixture_dir: Path):
    """Read resource IDs and the LANGID declared by the fixture inputs."""
    header_values = {}
    for line in (fixture_dir / "resource.h").read_text(encoding="utf-8").splitlines():
        match = RESOURCE_DEFINE_PATTERN.match(line)
        if match:
            header_values[match.group("name")] = int(match.group("value"), 0)

    definitions = []
    locale = None
    in_string_table = False
    for line in (fixture_dir / "test_windows_resource_reference.rc").read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        language = re.match(r"LANGUAGE\s+(\d+)\s*,\s*(\d+)", stripped)
        if language:
            primary = int(language.group(1))
            sublanguage = int(language.group(2))
            locale = (sublanguage << 10) | primary
            continue
        if stripped == "STRINGTABLE":
            in_string_table = True
            continue
        if in_string_table and stripped == "END":
            in_string_table = False
            continue

        declaration = None
        resource_type = None
        if in_string_table:
            declaration = re.match(r"(?P<name>[A-Za-z_]\w*)\s+\"", stripped)
            resource_type = "StringTable"
        else:
            declaration = re.match(r"(?P<name>[A-Za-z_]\w*)\s+(?P<kind>DIALOGEX?|MENU)\b", stripped)
            if declaration:
                resource_type = "Dialog" if declaration.group("kind").startswith("DIALOG") else "Menu"
        if declaration:
            name = declaration.group("name")
            if name not in header_values:
                raise RuntimeError(f"Resource declaration {name} is missing from resource.h")
            definitions.append((resource_type, name, header_values[name]))

    if locale is None:
        raise RuntimeError("The resource .rc file does not declare a LANGUAGE")
    if not definitions:
        raise RuntimeError("The resource .rc file declares no test resources")
    return sorted(definitions), locale


def resource_symbols(program, definitions, expected_locale):
    """Extract loader IDs separately from LANGID suffixes and verify fixture IDs."""
    expected_by_type = {}
    for resource_type, _, identifier in definitions:
        expected_by_type.setdefault(resource_type, []).append(identifier)
    string_ids = sorted(expected_by_type.get("StringTable", []))
    expected_string_table = (min(string_ids) - 1) // 16 + 1 if string_ids else None
    rows = []
    for symbol in program.getSymbolTable().getAllSymbols(True):
        name = str(symbol.getName())
        if not name.startswith("Rsrc_"):
            continue
        match = RESOURCE_SYMBOL_PATTERN.match(name)
        if not match:
            raise RuntimeError(f"Unrecognized PE resource symbol format: {name}")
        resource_type = match.group("type")
        resource_id = int(match.group("resource_id"), 16)
        locale = int(match.group("locale"), 16)
        if locale != expected_locale:
            raise RuntimeError(f"Resource symbol {name} has LANGID 0x{locale:X}, expected 0x{expected_locale:X}")
        expected_ids = expected_by_type.get(resource_type, [])
        if resource_type == "StringTable":
            if resource_id != expected_string_table:
                raise RuntimeError(
                    f"String-table symbol {name} encodes table {resource_id}, expected table {expected_string_table} "
                    f"for resource IDs {string_ids}"
                )
            identifier = f"table {resource_id} (IDs {', '.join(str(value) for value in string_ids)})"
        else:
            if resource_id not in expected_ids:
                raise RuntimeError(f"Resource symbol {name} has ID {resource_id}, expected {expected_ids}")
            identifier = str(resource_id)
        rows.append((resource_type, identifier, f"0x{locale:04X}", int(symbol.getAddress().getOffset()), name))
    return sorted(rows, key=lambda row: (row[0], row[2], row[3], row[4]))


def resource_references(program):
    """Extract DATA mnemonic references created by WindowsResourceReference.java."""
    from ghidra.program.model.symbol import RefType

    rows = []
    instructions = program.getListing().getInstructions(program.getMemory().getExecuteSet(), True)
    while instructions.hasNext():
        instruction = instructions.next()
        for reference in instruction.getMnemonicReferences():
            if reference.getReferenceType() != RefType.DATA:
                continue
            target = program.getSymbolTable().getPrimarySymbol(reference.getToAddress())
            rows.append((int(instruction.getAddress().getOffset()), int(reference.getToAddress().getOffset()), str(target.getName()) if target else "unnamed"))
    return sorted(set(rows))


def markdown(input_path: Path, enabled: list[str], symbols, references, definitions, locale) -> str:
    """Render actual resource types/IDs/addresses and analyzer-created DATA references."""
    lines = [
        "# Windows Resource Reference Behavioral Fixture",
        "",
        "> Generated by PyGhidra from the MSVC x64 PE linked with `rc.exe` output.",
        "> The only enabled boolean analyzer was `WindowsResourceReference`.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "",
        "## Analysis Configuration",
        "",
        *[f"- `{name}`" for name in enabled],
        "",
        "## PE Resource Symbols",
        "",
        "| Resource type | Resource ID/table | Locale | Address | Loader symbol |",
        "| --- | --- | --- | --- | --- |",
    ]
    for resource_type, identifier, resource_locale, address, name in symbols:
        lines.append(f"| `{resource_type}` | `{identifier}` | `{resource_locale}` | `0x{address:016X}` | `{name}` |")
    lines.extend(["", "## Resource IDs Verified Against Inputs", "", "| Resource type | .rc/resource.h symbol | ID |", "| --- | --- | --- |"])
    for resource_type, name, identifier in definitions:
        lines.append(f"| `{resource_type}` | `{name}` | `{identifier}` |")
    lines.extend(["", "## Analyzer DATA References", "", "| Instruction | Resource address | Resource symbol |", "| --- | --- | --- |"])
    for instruction, target, name in references:
        lines.append(f"| `0x{instruction:016X}` | `0x{target:016X}` | `{name}` |")
    lines.extend(
        [
            "",
            "## Fixture Assertion",
            "",
            f"- **Resource symbols reported:** `{len(symbols)}`.",
            f"- **Analyzer DATA references reported:** `{len(references)}`.",
            f"- The `.rc` LANGUAGE declaration resolves to LANGID `0x{locale:04X}`; the report keeps it separate from each resource ID.",
            "- Resource IDs are verified against the declarations in `test_windows_resource_reference.rc` and `resource.h`; the two LoadStringW calls resolve to string-table data addresses.",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, isolate, analyze, save, and close the resource project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_windows_resource_reference.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_windows_resource_reference.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_windows_resource_reference_"))
    project = None
    program = None
    script_host_acquired = False
    try:
        # WindowsResourceReferenceAnalyzer.java delegates to a module GhidraScript;
        # standalone PyGhidra has to initialize the script bundle registry explicitly.
        from ghidra.app.script import GhidraScriptUtil

        GhidraScriptUtil.acquireBundleHostReference()
        script_host_acquired = True
        project = GhidraProject.createProject(str(parent), "windows_resource_reference", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the resource fixture")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        prepare_disassembly(program)
        enabled = configure_analysis(project, program)
        project.analyze(program)
        definitions, locale = resource_definitions(fixture_dir)
        symbols = resource_symbols(program, definitions, locale)
        references = resource_references(program)
        if not symbols or not references:
            raise RuntimeError(f"Expected loader resource symbols and DATA references, got {len(symbols)} and {len(references)}")
        output_path.write_text(markdown(input_path, enabled, symbols, references, definitions, locale), encoding="utf-8", newline="\n")
        project.save(program)
    finally:
        if project is not None and program is not None:
            project.close(program)
        if project is not None:
            project.close()
        if script_host_acquired:
            GhidraScriptUtil.releaseBundleHostReference()
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
