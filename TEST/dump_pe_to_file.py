#!/usr/bin/env python3

"""
Dump the result of Ghidra's PE loader into a deterministic Markdown file.

This script is intended to generate golden/reference data for porting
Ghidra's PE loader to an autonomous C++23 implementation.

    Usage:

        python dump_pe_to_file.py <input.exe> [output.md] [--verbose]

    Example:

        python dump_pe_to_file.py GTA5.exe GTA5_PE_LOADER.md --verbose

The script uses the current PyGhidra + ProgramLoader API and explicitly
requests Ghidra's PeLoader.

Important:
    Analysis is disabled. We want to capture the loader result itself,
    not artifacts produced later by Ghidra analyzers.
"""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Any


def hx(value: Any) -> str:
    """Format Java/Python integer-like values as hexadecimal."""
    if value is None:
        return "null"

    try:
        return f"0x{int(value):X}"
    except Exception:
        return str(value)


def dec(value: Any) -> str:
    """Format integer-like values as decimal."""
    if value is None:
        return "null"

    try:
        return str(int(value))
    except Exception:
        return str(value)


def safe(callable_obj, default="N/A"):
    """Call a Java/Python API and prevent one broken field from killing the dump."""
    try:
        return callable_obj()
    except Exception as exc:
        return f"{default} ({type(exc).__name__}: {exc})"


def md_escape(value: Any) -> str:
    text = str(value)
    return text.replace("|", "\\|").replace("\n", " ")


def section(out: list[str], title: str) -> None:
    out.append("")
    out.append(f"## {title}")
    out.append("")


def kv(out: list[str], name: str, value: Any) -> None:
    out.append(f"- **{name}:** `{md_escape(value)}`")


def table_header(out: list[str], columns: list[str]) -> None:
    out.append("| " + " | ".join(columns) + " |")
    out.append("| " + " | ".join("---" for _ in columns) + " |")


def table_row(out: list[str], values: list[Any]) -> None:
    out.append("| " + " | ".join(md_escape(v) for v in values) + " |")


def dump_program(program, out: list[str]) -> None:
    section(out, "Program")

    kv(out, "Name", safe(program.getName))
    kv(out, "Executable path", safe(program.getExecutablePath))
    kv(out, "Executable format", safe(program.getExecutableFormat))
    kv(out, "Language", safe(lambda: program.getLanguage().getLanguageID()))
    kv(
        out,
        "Compiler specification",
        safe(lambda: program.getCompilerSpec().getCompilerSpecID()),
    )
    kv(out, "Image base", safe(lambda: hx(program.getImageBase().getOffset())))

    # Address factory
    section(out, "Address Spaces")

    factory = program.getAddressFactory()

    table_header(
        out,
        [
            "Name",
            "Type",
            "Word size",
            "Min",
            "Max",
        ],
    )

    for space in factory.getAllAddressSpaces():
        table_row_values = [
            space.getName(),
            space.getType(),
            dec(space.getAddressableUnitSize()),
            hx(space.getMinAddress().getOffset()),
            hx(space.getMaxAddress().getOffset()),
        ]

        table_row(out, table_row_values)


def dump_memory(program, out: list[str]) -> None:
    section(out, "Memory Blocks")

    memory = program.getMemory()

    table_header(
        out,
        [
            "Name",
            "Start",
            "End",
            "Size",
            "Read",
            "Write",
            "Execute",
            "Volatile",
            "Initialized",
            "Overlay",
        ],
    )

    for block in memory.getBlocks():
        table_row(
            out,
            [
                block.getName(),
                block.getStart(),
                block.getEnd(),
                hx(block.getSize()),
                block.isRead(),
                block.isWrite(),
                block.isExecute(),
                block.isVolatile(),
                block.isInitialized(),
                block.isOverlay(),
            ],
        )


def dump_sections_from_memory(program, out: list[str]) -> None:
    """
    Dump section-like memory information.

    The PE loader turns PE sections into Ghidra memory blocks.
    This is one of the most important outputs for the C++ port.
    """
    section(out, "Loaded Image / Section Mapping")

    memory = program.getMemory()

    table_header(
        out,
        [
            "Block",
            "Start VA",
            "End VA",
            "Size",
            "Permissions",
            "Initialized",
        ],
    )

    for block in memory.getBlocks():
        permissions = (
            ("R" if block.isRead() else "-")
            + ("W" if block.isWrite() else "-")
            + ("X" if block.isExecute() else "-")
        )

        table_row(
            out,
            [
                block.getName(),
                block.getStart(),
                block.getEnd(),
                hx(block.getSize()),
                permissions,
                block.isInitialized(),
            ],
        )


def dump_symbols(program, out: list[str]) -> None:
    section(out, "Symbols")

    symbol_table = program.getSymbolTable()

    table_header(
        out,
        [
            "Address",
            "Name",
            "Symbol type",
            "Source",
            "Primary",
        ],
    )

    for symbol in symbol_table.getAllSymbols(True):
        table_row(
            out,
            [
                symbol.getAddress(),
                symbol.getName(),
                symbol.getSymbolType(),
                symbol.getSource(),
                symbol.isPrimary(),
            ],
        )


def dump_external_symbols(program, out: list[str]) -> None:
    section(out, "External Symbols")

    symbol_table = program.getSymbolTable()

    table_header(
        out,
        [
            "Address",
            "Name",
            "Symbol type",
            "Library",
        ],
    )

    for symbol in symbol_table.getExternalSymbols():
        library = safe(lambda: symbol.getParentSymbol().getName())

        table_row(
            out,
            [
                symbol.getAddress(),
                symbol.getName(),
                symbol.getSymbolType(),
                library,
            ],
        )


def dump_entry_points(program, out: list[str]) -> None:
    section(out, "Entry Points")

    symbol_table = program.getSymbolTable()

    table_header(
        out,
        [
            "Address",
            "Name",
        ],
    )

    for address in symbol_table.getExternalEntryPointIterator():
        table_row(out, [address, "external"])

    # Primary entry point
    try:
        entry = program.getSymbolTable().getPrimarySymbol(
            program.getListing().getCodeUnitAt(program.getMinAddress()).getAddress()
        )
        if entry is not None:
            table_row(out, [entry.getAddress(), entry.getName()])
    except Exception:
        pass


def dump_references(program, out: list[str]) -> None:
    section(out, "References")

    reference_manager = program.getReferenceManager()

    table_header(
        out,
        [
            "From",
            "To",
            "Reference type",
            "Operand",
            "Primary",
        ],
    )

    iterator = reference_manager.getReferenceIterator(program.getMinAddress())

    # ReferenceManager iterators are not guaranteed to expose every address
    # in the way we want, so iterate memory/code units and inspect references.
    listing = program.getListing()

    for code_unit in listing.getCodeUnits(True):
        refs = reference_manager.getReferencesFrom(code_unit.getMinAddress())

        for ref in refs:
            table_row(
                out,
                [
                    ref.getFromAddress(),
                    ref.getToAddress(),
                    ref.getReferenceType(),
                    ref.getOperandIndex(),
                    ref.isPrimary(),
                ],
            )


def dump_properties(program, out: list[str]) -> None:
    section(out, "Program Properties")

    try:
        options = program.getOptions("Program Information")

        table_header(out, ["Property", "Value"])

        # Ghidra Options does not provide a simple universal "dump all"
        # API, so inspect known PE-related properties explicitly.
        known = [
            "Executable Format",
            "Compiler",
            "Language",
            "Image Base",
            "Original Image Base",
            "PE File Type",
            "PE Original Image Base",
            "PE Subsystem",
            "PE Machine",
        ]

        for name in known:
            value = safe(lambda n=name: options.getString(n, None), None)

            if value is not None:
                table_row(out, [name, value])

    except Exception as exc:
        out.append(
            f"`Unable to enumerate Program Information options: "
            f"{type(exc).__name__}: {exc}`"
        )


def dump_pe_header_objects(program, out: list[str]) -> None:
    """
    Try to obtain the PE parser objects associated with the loaded program.

    Ghidra's exact internal object exposure varies between versions, so this
    section intentionally uses defensive introspection.

    The primary source of truth remains the Program + memory state.
    """

    section(out, "PE Loader / Parser Objects")

    try:
        from ghidra.app.util.bin.format.pe import PortableExecutable

        kv(out, "PortableExecutable class", PortableExecutable.class_.getName())
    except Exception as exc:
        out.append(f"- PortableExecutable import: `{type(exc).__name__}: {exc}`")

    # The Program loader itself
    try:
        from ghidra.app.util.opinion import PeLoader

        kv(out, "Loader class", PeLoader.class_.getName())
        kv(out, "Loader name", PeLoader.PE_NAME)
    except Exception as exc:
        out.append(f"- PeLoader import: `{type(exc).__name__}: {exc}`")


def dump_listing_summary(program, out: list[str]) -> None:
    section(out, "Listing Summary")

    listing = program.getListing()

    counts = {
        "Instructions": listing.getNumInstructions(),
        "Defined data": listing.getNumDefinedData(),
        "Functions": program.getFunctionManager().getFunctionCount(),
    }

    table_header(out, ["Entity", "Count"])

    for name, count in counts.items():
        table_row(out, [name, count])


def dump_functions(program, out: list[str]) -> None:
    section(out, "Functions Present After Loader")

    fm = program.getFunctionManager()

    table_header(
        out,
        [
            "Entry",
            "Name",
            "Thunk",
            "External",
        ],
    )

    for function in fm.getFunctions(True):
        table_row(
            out,
            [
                function.getEntryPoint(),
                function.getName(),
                function.isThunk(),
                function.isExternal(),
            ],
        )


def generate_markdown(program, input_path: Path, verbose: bool = False) -> str:
    """Build the Markdown report, optionally including verbose listing data."""
    out: list[str] = []

    out.append("# Ghidra PE Loader Reference Data")
    out.append("")
    out.append("> Generated automatically from Ghidra using PyGhidra.")
    out.append(
        "> Analysis was disabled intentionally: this document describes "
        "the loader/import result rather than later analyzer output."
    )
    out.append("")

    section(out, "Input")

    kv(out, "File", input_path)
    kv(out, "File size", input_path.stat().st_size)
    kv(out, "File size (hex)", hx(input_path.stat().st_size))

    dump_program(program, out)
    dump_pe_header_objects(program, out)
    dump_memory(program, out)
    dump_sections_from_memory(program, out)
    dump_listing_summary(program, out)
    dump_external_symbols(program, out)
    dump_entry_points(program, out)
    dump_properties(program, out)

    if verbose:
        dump_symbols(program, out)
        dump_references(program, out)
        dump_functions(program, out)

    return "\n".join(out) + "\n"


def main() -> int:
    """Load a PE file with Ghidra and write its loader report to Markdown."""
    if len(sys.argv) < 2:
        print(
            "Usage: python dump_pe_to_file.py <input.exe> [output.md] [--verbose]",
            file=sys.stderr,
        )
        return 2

    arguments = sys.argv[1:]
    verbose = "--verbose" in arguments
    arguments = [argument for argument in arguments if argument != "--verbose"]

    if not arguments:
        print(
            "Usage: python dump_pe_to_file.py <input.exe> [output.md] [--verbose]",
            file=sys.stderr,
        )
        return 2

    input_path = Path(arguments[0]).resolve()

    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    if len(arguments) >= 2:
        output_path = Path(arguments[1]).resolve()
    else:
        output_path = input_path.with_suffix(input_path.suffix + ".pe-loader.md")

    import pyghidra

    # Current PyGhidra API.
    pyghidra.start()

    from ghidra.app.util.opinion import PeLoader

    print(f"[+] Input : {input_path}")
    print(f"[+] Output: {output_path}")
    print(f"[+] Loader: {PeLoader.PE_NAME}")
    print("[+] Importing with analysis disabled...")

    # open_program() is currently deprecated in favor of the newer
    # ProgramLoader API, but it is still a convenient PyGhidra bridge.
    #
    # We explicitly specify PeLoader so this isn't accidentally handled by
    # another opinion/loader.
    with pyghidra.open_program(
        input_path,
        analyze=False,
        loader=PeLoader,
    ) as flat_api:
        program = flat_api.getCurrentProgram()

        if program is None:
            raise RuntimeError("PyGhidra did not produce a Program")

        print("[+] Program loaded")
        print(f"[+] Format: {program.getExecutableFormat()}")
        print(f"[+] Language: {program.getLanguage().getLanguageID()}")
        print("[+] Dumping loader result...")

        markdown = generate_markdown(program, input_path, verbose=verbose)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(markdown, encoding="utf-8")

    print(f"[+] Done: {output_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
