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
import struct
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


def u16(value: Any) -> int:
    """Return a Java/Python integer normalized to an unsigned 16-bit value."""
    return int(value) & 0xFFFF


def u32(value: Any) -> int:
    """Return a Java/Python integer normalized to an unsigned 32-bit value."""
    return int(value) & 0xFFFFFFFF


def u64(value: Any) -> int:
    """Return a Java/Python integer normalized to an unsigned 64-bit value."""
    return int(value) & 0xFFFFFFFFFFFFFFFF


def rva_to_va(optional_header, rva: Any) -> str:
    """Convert a PE RVA to its preferred-image virtual address."""
    try:
        return hx(u64(optional_header.getImageBase()) + u32(rva))
    except Exception as exc:
        return f"N/A ({type(exc).__name__}: {exc})"


def rva_to_file_offset(nt_header, rva: Any) -> Any:
    """Convert a PE RVA to a raw file offset using Ghidra's section mapping."""
    try:
        pointer = int(nt_header.rvaToPointer(u32(rva)))
        return hx(pointer) if pointer >= 0 else "unmapped"
    except Exception as exc:
        return f"N/A ({type(exc).__name__}: {exc})"


def parser_section(out: list[str], title: str, callback) -> None:
    """Run one parser dump section without losing the rest of the report on failure."""
    section(out, title)
    try:
        callback()
    except Exception as exc:
        out.append(f"`Unable to dump {title}: {type(exc).__name__}: {exc}`")


def optional_method(obj, *names):
    """Call the first available Java getter from a list of version-specific names."""
    for name in names:
        method = getattr(obj, name, None)
        if method is not None:
            return method()
    return None


def dump_dos_header(pe, out: list[str]) -> None:
    """Dump the complete IMAGE_DOS_HEADER and DOS-stub metadata."""
    dos = pe.getDOSHeader()
    if dos is None:
        kv(out, "Present", False)
        return

    kv(out, "Present", dos.isDosSignature())
    fields = [
        ("e_magic", lambda: hx(u16(dos.e_magic()))),
        ("e_cblp", lambda: dec(u16(dos.e_cblp()))),
        ("e_cp", lambda: dec(u16(dos.e_cp()))),
        ("e_crlc", lambda: dec(u16(dos.e_crlc()))),
        ("e_cparhdr", lambda: dec(u16(dos.e_cparhdr()))),
        ("e_minalloc", lambda: dec(u16(dos.e_minalloc()))),
        ("e_maxalloc", lambda: dec(u16(dos.e_maxalloc()))),
        ("e_ss", lambda: hx(u16(dos.e_ss()))),
        ("e_sp", lambda: hx(u16(dos.e_sp()))),
        ("e_csum", lambda: hx(u16(dos.e_csum()))),
        ("e_ip", lambda: hx(u16(dos.e_ip()))),
        ("e_cs", lambda: hx(u16(dos.e_cs()))),
        ("e_lfarlc", lambda: hx(u16(dos.e_lfarlc()))),
        ("e_ovno", lambda: dec(u16(dos.e_ovno()))),
        ("e_res", lambda: [u16(value) for value in dos.e_res()]),
        ("e_oemid", lambda: hx(u16(dos.e_oemid()))),
        ("e_oeminfo", lambda: hx(u16(dos.e_oeminfo()))),
        ("e_res2", lambda: [u16(value) for value in dos.e_res2()]),
        ("e_lfanew", lambda: hx(u32(dos.e_lfanew()))),
        ("DOS stub size", dos.getProgramLen),
    ]

    for name, getter in fields:
        kv(out, name, safe(getter))


def dump_rich_header(pe, out: list[str]) -> None:
    """Dump Microsoft Rich-header location, mask, compiler IDs, and object counts."""
    rich = pe.getRichHeader()
    if rich is None or int(rich.getSize()) == 0:
        kv(out, "Present", False)
        return

    kv(out, "Present", True)
    kv(out, "Offset", hx(u32(rich.getOffset())))
    kv(out, "Size", hx(u32(rich.getSize())))
    kv(out, "Mask", hx(u32(rich.getMask())))

    table_header(out, ["Index", "Comp ID", "Product ID", "Product", "Build", "Object count"])
    for record in rich.getRecords():
        comp_id = record.getCompId()
        table_row(
            out,
            [
                record.getIndex(),
                hx(u32(comp_id.getValue())),
                hx(u32(comp_id.getProductId())),
                comp_id.getProductDescription(),
                comp_id.getBuildNumber(),
                record.getObjectCount(),
            ],
        )


def dump_file_header(nt_header, out: list[str]) -> None:
    """Dump IMAGE_FILE_HEADER values and decoded machine/characteristic flags."""
    header = nt_header.getFileHeader()
    machine = u16(header.getMachine())
    kv(out, "Machine", hx(machine))
    kv(out, "Machine name/value", header.getMachineName())
    kv(
        out,
        "Machine architecture",
        {
            0x014C: "Intel 386 (I386)",
            0x8664: "AMD64 / x86-64",
            0x01C0: "ARM",
            0x01C4: "ARMNT",
            0xAA64: "ARM64",
            0x0200: "IA64",
        }.get(machine, "Unknown"),
    )
    kv(out, "Number of sections", header.getNumberOfSections())
    kv(out, "TimeDateStamp", hx(u32(header.getTimeDateStamp())))
    kv(out, "PointerToSymbolTable", hx(u32(header.getPointerToSymbolTable())))
    kv(out, "NumberOfSymbols", dec(u32(header.getNumberOfSymbols())))
    kv(out, "SizeOfOptionalHeader", dec(u16(header.getSizeOfOptionalHeader())))
    characteristics = u16(header.getCharacteristics())
    kv(out, "Characteristics", hx(characteristics))

    flags = [
        (0x0001, "IMAGE_FILE_RELOCS_STRIPPED"),
        (0x0002, "IMAGE_FILE_EXECUTABLE_IMAGE"),
        (0x0004, "IMAGE_FILE_LINE_NUMS_STRIPPED"),
        (0x0008, "IMAGE_FILE_LOCAL_SYMS_STRIPPED"),
        (0x0020, "IMAGE_FILE_LARGE_ADDRESS_AWARE"),
        (0x0100, "IMAGE_FILE_32BIT_MACHINE"),
        (0x0200, "IMAGE_FILE_DEBUG_STRIPPED"),
        (0x1000, "IMAGE_FILE_SYSTEM"),
        (0x2000, "IMAGE_FILE_DLL"),
    ]
    kv(out, "Decoded characteristics", [name for mask, name in flags if characteristics & mask])
    kv(out, "Section table file offset", hx(u32(header.getPointerToSections())))


def dump_optional_header(nt_header, out: list[str]) -> None:
    """Dump all scalar IMAGE_OPTIONAL_HEADER fields exposed by Ghidra."""
    header = nt_header.getOptionalHeader()
    fields = [
        ("Magic", lambda: hx(0x20B if header.is64bit() else 0x10B)),
        ("PE32+", header.is64bit),
        ("Linker version", lambda: f"{u16(header.getMajorLinkerVersion())}.{u16(header.getMinorLinkerVersion())}"),
        ("SizeOfCode", lambda: hx(u64(header.getSizeOfCode()))),
        ("SizeOfInitializedData", lambda: hx(u64(header.getSizeOfInitializedData()))),
        ("SizeOfUninitializedData", lambda: hx(u64(header.getSizeOfUninitializedData()))),
        ("AddressOfEntryPoint RVA", lambda: hx(u32(header.getAddressOfEntryPoint()))),
        ("AddressOfEntryPoint VA", lambda: rva_to_va(header, header.getAddressOfEntryPoint())),
        ("BaseOfCode RVA", lambda: hx(u32(header.getBaseOfCode()))),
        ("BaseOfData RVA", lambda: hx(u32(header.getBaseOfData()))),
        ("ImageBase", lambda: hx(u64(header.getImageBase()))),
        ("SectionAlignment", lambda: hx(u32(header.getSectionAlignment()))),
        ("FileAlignment", lambda: hx(u32(header.getFileAlignment()))),
        ("OS version", lambda: f"{u16(header.getMajorOperatingSystemVersion())}.{u16(header.getMinorOperatingSystemVersion())}"),
        ("Image version", lambda: f"{u16(header.getMajorImageVersion())}.{u16(header.getMinorImageVersion())}"),
        ("Subsystem version", lambda: f"{u16(header.getMajorSubsystemVersion())}.{u16(header.getMinorSubsystemVersion())}"),
        ("Win32VersionValue", lambda: hx(u32(header.getWin32VersionValue()))),
        ("SizeOfImage", lambda: hx(u64(header.getSizeOfImage()))),
        ("SizeOfHeaders", lambda: hx(u32(header.getSizeOfHeaders()))),
        ("CheckSum", lambda: hx(u32(header.getChecksum()))),
        ("Subsystem", lambda: hx(u16(header.getSubsystem()))),
        ("DllCharacteristics", lambda: hx(u16(header.getDllCharacteristics()))),
        ("Stack reserve", lambda: hx(u64(header.getSizeOfStackReserve()))),
        ("Stack commit", lambda: hx(u64(header.getSizeOfStackCommit()))),
        ("Heap reserve", lambda: hx(u64(header.getSizeOfHeapReserve()))),
        ("Heap commit", lambda: hx(u64(header.getSizeOfHeapCommit()))),
        ("LoaderFlags", lambda: hx(u32(header.getLoaderFlags()))),
        ("NumberOfRvaAndSizes", lambda: dec(u64(header.getNumberOfRvaAndSizes()))),
        ("CLI/.NET image", header.isCLI),
    ]

    for name, getter in fields:
        kv(out, name, safe(getter))

    dll_characteristics = u16(header.getDllCharacteristics())
    dll_flags = [
        (0x0020, "HIGH_ENTROPY_VA"),
        (0x0040, "DYNAMIC_BASE"),
        (0x0080, "FORCE_INTEGRITY"),
        (0x0100, "NX_COMPAT"),
        (0x0200, "NO_ISOLATION"),
        (0x0400, "NO_SEH"),
        (0x0800, "NO_BIND"),
        (0x1000, "APPCONTAINER"),
        (0x2000, "WDM_DRIVER"),
        (0x4000, "GUARD_CF"),
        (0x8000, "TERMINAL_SERVER_AWARE"),
    ]
    kv(out, "Decoded DLL characteristics", [name for mask, name in dll_flags if dll_characteristics & mask])


def dump_pe_sections(nt_header, out: list[str]) -> None:
    """Dump raw and virtual IMAGE_SECTION_HEADER fields and section flags."""
    optional = nt_header.getOptionalHeader()
    table_header(
        out,
        [
            "#",
            "Name",
            "Raw offset",
            "Raw size",
            "Virtual RVA",
            "Virtual VA",
            "Virtual size",
            "Characteristics",
            "Flags",
            "Reloc ptr/count",
            "Line ptr/count",
        ],
    )

    section_flags = [
        (0x00000020, "CODE"),
        (0x00000040, "INITIALIZED_DATA"),
        (0x00000080, "UNINITIALIZED_DATA"),
        (0x02000000, "DISCARDABLE"),
        (0x04000000, "NOT_CACHED"),
        (0x08000000, "NOT_PAGED"),
        (0x10000000, "SHARED"),
        (0x20000000, "EXECUTE"),
        (0x40000000, "READ"),
        (0x80000000, "WRITE"),
    ]

    for index, header in enumerate(nt_header.getFileHeader().getSectionHeaders()):
        characteristics = u32(header.getCharacteristics())
        rva = u32(header.getVirtualAddress())
        table_row(
            out,
            [
                index,
                header.getReadableName(),
                hx(u32(header.getPointerToRawData())),
                hx(u32(header.getSizeOfRawData())),
                hx(rva),
                rva_to_va(optional, rva),
                hx(u32(header.getVirtualSize())),
                hx(characteristics),
                ", ".join(name for mask, name in section_flags if characteristics & mask),
                f"{hx(u32(header.getPointerToRelocations()))}/{u16(header.getNumberOfRelocations())}",
                f"{hx(u32(header.getPointerToLinenumbers()))}/{u16(header.getNumberOfLinenumbers())}",
            ],
        )


def dump_data_directories(nt_header, out: list[str]) -> None:
    """Dump every IMAGE_DATA_DIRECTORY RVA, size, raw offset, VA, and parse status."""
    optional = nt_header.getOptionalHeader()
    directories = optional.getDataDirectories()
    names = [
        "EXPORT", "IMPORT", "RESOURCE", "EXCEPTION", "SECURITY", "BASERELOC",
        "DEBUG", "ARCHITECTURE", "GLOBALPTR", "TLS", "LOAD_CONFIG", "BOUND_IMPORT",
        "IAT", "DELAY_IMPORT", "COM_DESCRIPTOR", "RESERVED",
    ]
    table_header(out, ["Index", "Directory", "Parser class", "RVA", "VA", "Raw offset", "Size", "Parsed"])
    if directories is None:
        return

    for index, directory in enumerate(directories):
        if directory is None:
            table_row(out, [index, names[index] if index < len(names) else "UNKNOWN", "null", "0", "N/A", "N/A", "0", False])
            continue

        rva = u32(directory.getVirtualAddress())
        virtual_address = "N/A" if rva == 0 else rva_to_va(optional, rva)
        raw_offset = "N/A" if rva == 0 else rva_to_file_offset(nt_header, rva)
        table_row(
            out,
            [
                index,
                safe(directory.getDirectoryName),
                directory.getClass().getSimpleName(),
                hx(rva),
                virtual_address,
                raw_offset,
                hx(u32(directory.getSize())),
                safe(directory.hasParsedCorrectly),
            ],
        )


def directory_at(optional_header, index: int):
    """Return a parsed data directory by index, or None when it is absent."""
    directories = optional_header.getDataDirectories()
    if directories is None or index < 0 or index >= len(directories):
        return None
    return directories[index]


def dump_import_directory(nt_header, out: list[str]) -> None:
    """Dump import descriptors, INT/IAT slots, hints, ordinals, RVAs, VAs, and file offsets."""
    optional = nt_header.getOptionalHeader()
    directory = directory_at(optional, 1)
    if directory is None:
        kv(out, "Present", False)
        return

    descriptors = directory.getImportDescriptors()
    table_header(
        out,
        [
            "DLL",
            "Descriptor name RVA",
            "Descriptor raw offset",
            "OriginalFirstThunk RVA",
            "FirstThunk/IAT RVA",
            "TimeDateStamp",
            "Bound",
        ],
    )
    for descriptor in descriptors:
        table_row(
            out,
            [
                descriptor.getDLL(),
                hx(u32(descriptor.getName())),
                rva_to_file_offset(nt_header, descriptor.getName()),
                hx(u32(descriptor.getOriginalFirstThunk())),
                hx(u32(descriptor.getFirstThunk())),
                hx(u32(descriptor.getTimeDateStamp())),
                descriptor.isBound(),
            ],
        )

    section(out, "Imported Functions / IAT Addresses")
    table_header(
        out,
        [
            "DLL",
            "Name / ordinal",
            "Hint",
            "INT slot RVA",
            "IAT slot RVA",
            "IAT slot VA",
            "IAT slot raw offset",
            "Thunk value",
            "ImportByName RVA",
            "ImportByName raw offset",
        ],
    )
    for descriptor in descriptors:
        int_thunks = descriptor.getImportNameTableThunkData()
        iat_thunks = descriptor.getImportAddressTableThunkData()
        int_rva = u32(descriptor.getOriginalFirstThunk() or descriptor.getFirstThunk())
        iat_rva = u32(descriptor.getFirstThunk())
        slot_size = 8 if optional.is64bit() else 4

        for index, thunk in enumerate(int_thunks):
            import_by_name = thunk.getImportByName()
            ordinal = thunk.getOrdinal() if thunk.isOrdinal() else ""
            name = f"Ordinal #{ordinal}" if thunk.isOrdinal() else import_by_name.getName()
            hint = "" if thunk.isOrdinal() else u16(import_by_name.getHint())
            iat_thunk = iat_thunks[index] if index < len(iat_thunks) else None
            table_row(
                out,
                [
                    descriptor.getDLL(),
                    name,
                    hint,
                    hx(int_rva + index * slot_size),
                    hx(iat_rva + index * slot_size),
                    rva_to_va(optional, iat_rva + index * slot_size),
                    rva_to_file_offset(nt_header, iat_rva + index * slot_size),
                    "N/A" if iat_thunk is None else hx(u64(iat_thunk.getFunction())),
                    "" if thunk.isOrdinal() else hx(u32(thunk.getAddressOfData())),
                    "" if thunk.isOrdinal() else rva_to_file_offset(nt_header, thunk.getAddressOfData()),
                ],
            )


def dump_export_directory(nt_header, out: list[str]) -> None:
    """Dump the export directory arrays and every named or ordinal export address."""
    optional = nt_header.getOptionalHeader()
    directory = directory_at(optional, 0)
    if directory is None:
        kv(out, "Present", False)
        return

    fields = [
        ("Export name RVA", lambda: hx(u32(directory.getName()))),
        ("Export name", directory.getExportName),
        ("Characteristics", lambda: hx(u32(directory.getCharacteristics()))),
        ("TimeDateStamp", lambda: hx(u32(directory.getTimeDateStamp()))),
        ("Version", lambda: f"{u16(directory.getMajorVersion())}.{u16(directory.getMinorVersion())}"),
        ("Ordinal base", lambda: dec(u32(directory.getBase()))),
        ("Number of functions", lambda: dec(u32(directory.getNumberOfFunctions()))),
        ("Number of names", lambda: dec(u32(directory.getNumberOfNames()))),
        ("AddressOfFunctions RVA", lambda: hx(u32(directory.getAddressOfFunctions()))),
        ("AddressOfFunctions VA", lambda: rva_to_va(optional, directory.getAddressOfFunctions())),
        ("AddressOfNames RVA", lambda: hx(u32(directory.getAddressOfNames()))),
        ("AddressOfNames VA", lambda: rva_to_va(optional, directory.getAddressOfNames())),
        ("AddressOfNameOrdinals RVA", lambda: hx(u32(directory.getAddressOfNameOrdinals()))),
        ("AddressOfNameOrdinals VA", lambda: rva_to_va(optional, directory.getAddressOfNameOrdinals())),
    ]
    for name, getter in fields:
        kv(out, name, safe(getter))

    section(out, "Exported Functions")
    table_header(out, ["Ordinal", "Name", "Address VA", "Address RVA", "Raw offset", "Forwarded", "Comment"])
    for export in directory.getExports():
        address = u64(export.getAddress())
        rva = address - u64(optional.getImageBase())
        table_row(
            out,
            [
                export.getOrdinal(),
                export.getName(),
                hx(address),
                hx(rva),
                rva_to_file_offset(nt_header, rva),
                export.isForwarded(),
                export.getComment(),
            ],
        )


def dump_relocation_directory(nt_header, out: list[str]) -> None:
    """Dump base-relocation blocks and each relocation type/offset/target RVA."""
    directory = directory_at(nt_header.getOptionalHeader(), 5)
    if directory is None:
        kv(out, "Present", False)
        return

    table_header(out, ["Block RVA", "Block VA", "Block size", "Entry index", "Type", "Type name", "Offset", "Target RVA", "Target VA"])
    for block in directory.getBaseRelocations():
        block_rva = u32(block.getVirtualAddress())
        for index in range(block.getCount()):
            relocation_type = block.getType(index)
            target_rva = block_rva + u32(block.getOffset(index))
            table_row(
                out,
                [
                    hx(block_rva),
                    rva_to_va(nt_header.getOptionalHeader(), block_rva),
                    hx(u32(block.getSizeOfBlock())),
                    index,
                    relocation_type,
                    block.getName(relocation_type),
                    hx(u32(block.getOffset(index))),
                    hx(target_rva),
                    rva_to_va(nt_header.getOptionalHeader(), target_rva),
                ],
            )


def dump_tls_directory(nt_header, out: list[str]) -> None:
    """Dump TLS raw-data range, index, callback array, zero-fill, and characteristics."""
    directory = directory_at(nt_header.getOptionalHeader(), 9)
    if directory is None or directory.getTLSDirectory() is None:
        kv(out, "Present", False)
        return

    tls = directory.getTLSDirectory()
    table_header(out, ["Start raw VA", "End raw VA", "AddressOfIndex VA", "AddressOfCallbacks VA", "Zero fill", "Characteristics"])
    table_row(
        out,
        [
            hx(u64(tls.getStartAddressOfRawData())),
            hx(u64(tls.getEndAddressOfRawData())),
            hx(u64(tls.getAddressOfIndex())),
            hx(u64(tls.getAddressOfCallBacks())),
            hx(u32(tls.getSizeOfZeroFill())),
            hx(u32(tls.getCharacteristics())),
        ],
    )


def dump_load_config_directory(nt_header, out: list[str]) -> None:
    """Dump Load Config security-hardening pointers and dynamic relocation metadata."""
    directory = directory_at(nt_header.getOptionalHeader(), 10)
    if directory is None:
        kv(out, "Present", False)
        return

    config = directory.getLoadConfigDirectory()
    if config is None:
        kv(out, "Parsed", False)
        return

    fields = [
        ("Size", lambda: hx(u32(config.getSize()))),
        ("CriticalSectionDefaultTimeout", lambda: hx(u32(config.getCriticalSectionDefaultTimeout()))),
        ("SEHandlerTable", lambda: hx(u64(config.getSeHandlerTable()))),
        ("SEHandlerCount", lambda: dec(u64(config.getSeHandlerCount()))),
        ("CFG guard flags", lambda: hx(u32(config.getCfgGuardFlags().getFlags()))),
        ("CFG check function", lambda: hx(u64(config.getCfgCheckFunctionPointer()))),
        ("CFG dispatch function", lambda: hx(u64(config.getCfgDispatchFunctionPointer()))),
        ("CFG function table", lambda: hx(u64(config.getCfgFunctionTablePointer()))),
        ("CFG function count", lambda: dec(u64(config.getCfgFunctionCount()))),
        ("Guard address-taken IAT table", lambda: hx(u64(config.getGuardAddressIatTableTablePointer()))),
        ("Guard address-taken IAT count", lambda: dec(u64(config.getGuardAddressIatTableCount()))),
        ("CHPE metadata", lambda: optional_method(config, "getChpeMetadataX86", "getChpeMetadataPointer")),
        ("ARM64EC metadata", lambda: optional_method(config, "getArm64ecMetadata")),
        ("RFG failure routine", lambda: hx(u64(config.getRfgFailureRoutine()))),
        ("RFG failure routine pointer", lambda: hx(u64(config.getRfgFailureRoutineFunctionPointer()))),
        ("RFG verify stack pointer", lambda: hx(u64(config.getRfgVerifyStackPointerFunctionPointer()))),
    ]
    for name, getter in fields:
        kv(out, name, safe(getter))

    dynamic = config.getDynamicRelocationTable()
    kv(out, "Dynamic relocation table object", "null" if dynamic is None else dynamic)


def dump_delay_import_directory(nt_header, out: list[str]) -> None:
    """Dump delay-load DLL descriptors, thunk addresses, and imported symbols."""
    directory = directory_at(nt_header.getOptionalHeader(), 13)
    if directory is None:
        kv(out, "Present", False)
        return

    descriptors = directory.getDelayImportDescriptors()
    table_header(
        out,
        [
            "DLL",
            "Valid",
            "Uses RVA",
            "Attributes",
            "Module handle",
            "IAT",
            "INT",
            "Bound IAT",
            "Unload IAT",
            "TimeDateStamp",
        ],
    )
    for descriptor in descriptors:
        table_row(
            out,
            [
                descriptor.getDLLName(),
                descriptor.isValid(),
                descriptor.isUsingRVA(),
                hx(u32(descriptor.getAttibutes())),
                hx(u64(descriptor.getAddressOfModuleHandle())),
                hx(u64(descriptor.getAddressOfIAT())),
                hx(u64(descriptor.getAddressOfINT())),
                hx(u64(descriptor.getAddressOfBoundIAT())),
                hx(u64(descriptor.getAddressOfOriginalIAT())),
                hx(u32(descriptor.getTimeStamp())),
            ],
        )

    section(out, "Delay Imported Functions")
    table_header(out, ["DLL", "Name", "Address RVA/VA", "Bound", "Comment"])
    for descriptor in descriptors:
        for info in descriptor.getImportList():
            address = u32(info.getAddress())
            table_row(
                out,
                [
                    info.getDLL(),
                    info.getName(),
                    f"{hx(address)} / {rva_to_va(nt_header.getOptionalHeader(), address)}",
                    info.isBound(),
                    info.getComment(),
                ],
            )


def dump_bound_import_directory(nt_header, out: list[str]) -> None:
    """Dump bound-import DLL names, timestamps, and forwarder-reference counts."""
    directory = directory_at(nt_header.getOptionalHeader(), 11)
    if directory is None:
        kv(out, "Present", False)
        return

    table_header(out, ["Module", "TimeDateStamp", "Name offset", "Forwarder refs"])
    for descriptor in directory.getBoundImportDescriptors():
        table_row(
            out,
            [
                descriptor.getModuleName(),
                hx(u32(descriptor.getTimeDateStamp())),
                hx(u16(descriptor.getOffsetModuleName())),
                descriptor.getNumberOfModuleForwarderRefs(),
            ],
        )


def dump_resource_directory(nt_header, out: list[str]) -> None:
    """Dump resource directory metadata and every resource leaf RVA/size/type."""
    directory = directory_at(nt_header.getOptionalHeader(), 2)
    if directory is None or directory.getRootDirectory() is None:
        kv(out, "Present", False)
        return

    root = directory.getRootDirectory()
    fields = [
        ("Characteristics", lambda: hx(u32(root.getCharacteristics()))),
        ("TimeDateStamp", lambda: hx(u32(root.getTimeDataStamp()))),
        ("Version", lambda: f"{u16(root.getMajorVersion())}.{u16(root.getMinorVersion())}"),
        ("Named entries", root.getNumberOfNamedEntries),
        ("ID entries", root.getNumberOfIdEntries),
    ]
    for name, getter in fields:
        kv(out, name, safe(getter))

    section(out, "Resource Leaves")
    table_header(out, ["Type ID", "Type", "ID", "Name", "RVA/VA", "Raw offset", "Size"])
    for resource in directory.getResources():
        address = u32(resource.getAddress())
        table_row(
            out,
            [
                resource.getTypeID(),
                resource.getName(),
                resource.getID(),
                resource.getName(),
                f"{hx(address)} / {rva_to_va(nt_header.getOptionalHeader(), address)}",
                rva_to_file_offset(nt_header, address),
                hx(u32(resource.getSize())),
            ],
        )


def dump_security_directory(nt_header, out: list[str]) -> None:
    """Dump Authenticode certificate table offsets, sizes, revisions, and certificate types."""
    directory = directory_at(nt_header.getOptionalHeader(), 4)
    if directory is None:
        kv(out, "Present", False)
        return

    certificates = directory.getCertificate()
    table_header(out, ["Index", "Length", "Revision", "Type", "Type name", "Certificate bytes"])
    for index, certificate in enumerate(certificates):
        table_row(
            out,
            [
                index,
                hx(u32(certificate.getLength())),
                hx(u16(certificate.getRevision())),
                hx(u16(certificate.getType())),
                certificate.getTypeAsString(),
                len(certificate.getData()),
            ],
        )


def dump_com_descriptor(nt_header, out: list[str]) -> None:
    """Dump CLR/COM descriptor metadata when the PE contains a managed image."""
    directory = directory_at(nt_header.getOptionalHeader(), 14)
    if directory is None or directory.getHeader() is None:
        kv(out, "Present", False)
        return

    header = directory.getHeader()
    fields = [
        ("Header size", lambda: hx(u32(header.getCb()))),
        ("Runtime version", lambda: f"{u16(header.getMajorRuntimeVersion())}.{u16(header.getMinorRuntimeVersion())}"),
        ("Flags", lambda: hx(u32(header.getFlags()))),
        ("EntryPointToken", lambda: hx(u32(header.getEntryPointToken()))),
        ("EntryPointVA", header.getEntryPointVA),
    ]
    for name, getter in fields:
        kv(out, name, safe(getter))

    table_header(out, ["Subdirectory", "RVA", "VA", "Size"])
    for name, getter in [
        ("Metadata", header.getMetadata),
        ("Resources", header.getResources),
        ("StrongNameSignature", header.getStrongNameSignature),
        ("CodeManagerTable", header.getCodeManagerTable),
        ("VTableFixups", header.getVTableFixups),
        ("ExportAddressTableJumps", header.getExportAddressTableJumps),
        ("ManagedNativeHeader", header.getManagedNativeHeader),
    ]:
        child = safe(getter, None)
        if child is None:
            table_row(out, [name, "N/A", "N/A", "N/A"])
            continue
        rva = u32(child.getVirtualAddress())
        table_row(out, [name, hx(rva), rva_to_va(nt_header.getOptionalHeader(), rva), hx(u32(child.getSize()))])


def dump_debug_directory(nt_header, out: list[str]) -> None:
    """Dump debug-directory records and the parser's CodeView/PDB presence indicators."""
    directory = directory_at(nt_header.getOptionalHeader(), 6)
    if directory is None:
        kv(out, "Present", False)
        return

    parser = directory.getParser()
    table_header(
        out,
        [
            "Characteristics",
            "TimeDateStamp",
            "Version",
            "Type",
            "Description",
            "SizeOfData",
            "AddressOfRawData RVA",
            "PointerToRawData",
        ],
    )
    for debug in parser.getDebugDirectories():
        table_row(
            out,
            [
                hx(u32(debug.getCharacteristics())),
                hx(u32(debug.getTimeDateStamp())),
                f"{u16(debug.getMajorVersion())}.{u16(debug.getMinorVersion())}",
                debug.getType(),
                debug.getDescription(),
                hx(u32(debug.getSizeOfData())),
                hx(u32(debug.getAddressOfRawData())),
                hx(u32(debug.getPointerToRawData())),
            ],
        )

    code_view = parser.getDebugCodeView()
    kv(out, "CodeView parser present", code_view is not None)
    if code_view is not None:
        pdb_info = code_view.getPdbInfo()
        dotnet_pdb_info = code_view.getDotNetPdbInfo()
        kv(out, "CodeView PDB info present", pdb_info is not None)
        kv(out, "CodeView PDB valid", safe(pdb_info.isValid) if pdb_info is not None else False)
        kv(out, ".NET PDB info present", dotnet_pdb_info is not None)
        kv(out, ".NET PDB valid", safe(dotnet_pdb_info.isValid) if dotnet_pdb_info is not None else False)


def dump_exception_directory(nt_header, provider, out: list[str]) -> None:
    """Dump .pdata runtime-function ranges and unwind-info RVAs from the exception directory."""
    directory = directory_at(nt_header.getOptionalHeader(), 3)
    if directory is None or u32(directory.getSize()) == 0:
        kv(out, "Present", False)
        return

    raw_offset = int(nt_header.rvaToPointer(u32(directory.getVirtualAddress())))
    if raw_offset < 0:
        kv(out, "Raw mapping", "unmapped")
        return

    entry_size = 12
    count = u32(directory.getSize()) // entry_size
    data = bytes(provider.readBytes(raw_offset, count * entry_size))
    table_header(out, ["Entry", "Begin RVA", "Begin VA", "End RVA", "End VA", "Unwind RVA", "Unwind VA", "Unwind raw offset"])
    for index in range(count):
        begin_rva, end_rva, unwind_rva = struct.unpack_from("<III", data, index * entry_size)
        table_row(
            out,
            [
                index,
                hx(begin_rva),
                rva_to_va(nt_header.getOptionalHeader(), begin_rva),
                hx(end_rva),
                rva_to_va(nt_header.getOptionalHeader(), end_rva),
                hx(unwind_rva),
                rva_to_va(nt_header.getOptionalHeader(), unwind_rva),
                rva_to_file_offset(nt_header, unwind_rva),
            ],
        )


def dump_pe_parser_info(input_path: Path, out: list[str]) -> None:
    """Parse the source file with Ghidra's PE model and dump all exposed PE structures."""
    provider = None
    try:
        from java.io import File
        from java.nio.file import AccessMode
        from ghidra.app.util.bin import FileByteProvider
        from ghidra.app.util.bin.format.pe import PortableExecutable

        provider = FileByteProvider(File(str(input_path)), None, AccessMode.READ)
        pe = PortableExecutable(provider, PortableExecutable.SectionLayout.FILE, True, True)
        nt_header = pe.getNTHeader()
        if nt_header is None:
            kv(out, "NT header", "not parsed")
            return

        parser_section(out, "DOS Header", lambda: dump_dos_header(pe, out))
        parser_section(out, "Rich Header", lambda: dump_rich_header(pe, out))
        parser_section(out, "NT File Header", lambda: dump_file_header(nt_header, out))
        parser_section(out, "NT Optional Header", lambda: dump_optional_header(nt_header, out))
        parser_section(out, "PE Data Directories", lambda: dump_data_directories(nt_header, out))
        parser_section(out, "PE Section Headers", lambda: dump_pe_sections(nt_header, out))
        parser_section(out, "PE Export Directory", lambda: dump_export_directory(nt_header, out))
        parser_section(out, "PE Import Directory", lambda: dump_import_directory(nt_header, out))
        parser_section(out, "PE Debug Directory", lambda: dump_debug_directory(nt_header, out))
        parser_section(out, "PE Exception / Runtime Function Directory", lambda: dump_exception_directory(nt_header, provider, out))
        parser_section(out, "PE Base Relocation Directory", lambda: dump_relocation_directory(nt_header, out))
        parser_section(out, "PE Resource Directory", lambda: dump_resource_directory(nt_header, out))
        parser_section(out, "PE Security / Authenticode Directory", lambda: dump_security_directory(nt_header, out))
        parser_section(out, "PE TLS Directory", lambda: dump_tls_directory(nt_header, out))
        parser_section(out, "PE Load Config Directory", lambda: dump_load_config_directory(nt_header, out))
        parser_section(out, "PE Bound Import Directory", lambda: dump_bound_import_directory(nt_header, out))
        parser_section(out, "PE Delay Import Directory", lambda: dump_delay_import_directory(nt_header, out))
        parser_section(out, "PE COM/.NET Descriptor", lambda: dump_com_descriptor(nt_header, out))
    except Exception as exc:
        out.append(f"`Unable to initialize direct PE parser: {type(exc).__name__}: {exc}`")
    finally:
        if provider is not None:
            try:
                provider.close()
            except Exception:
                pass


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
    dump_pe_parser_info(input_path, out)
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
