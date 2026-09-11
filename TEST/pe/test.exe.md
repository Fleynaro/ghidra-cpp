# Ghidra PE Loader Reference Data

> Generated automatically from Ghidra using PyGhidra.
> Analysis was disabled intentionally: this document describes the loader/import result rather than later analyzer output.


## Input

- **File:** `C:\Users\Fleynaro\Desktop\GTA-5-Android\ghidra\TEST\test.exe`
- **File size:** `689664`
- **File size (hex):** `0xA8600`

## Program

- **Name:** `test.exe`
- **Executable path:** `/C:/Users/Fleynaro/Desktop/GTA-5-Android/ghidra/TEST/test.exe`
- **Executable format:** `Portable Executable (PE)`
- **Language:** `x86:LE:64:default`
- **Compiler specification:** `windows`
- **Image base:** `0x140000000`

## Address Spaces

| Name | Type | Word size | Min | Max |
| --- | --- | --- | --- | --- |
| const | 0 | 1 | 0x-8000000000000000 | 0x7FFFFFFFFFFFFFFF |
| unique | 3 | 1 | 0x0 | 0xFFFFFFFF |
| ram | 1 | 1 | 0x0 | 0x-1 |
| register | 4 | 1 | 0x0 | 0xFFFFFFFF |
| OTHER | 7 | 1 | 0x0 | 0x-1 |
| EXTERNAL | 10 | 1 | 0x0 | 0xFFFFFFFF |
| stack | 5 | 1 | 0x-8000000000000000 | 0x7FFFFFFFFFFFFFFF |
| HASH | 14 | 1 | 0x0 | 0xFFFFFFFFFFFFFFF |

## PE Loader / Parser Objects

- **PortableExecutable class:** `ghidra.app.util.bin.format.pe.PortableExecutable`
- **Loader class:** `ghidra.app.util.opinion.PeLoader`
- **Loader name:** `Portable Executable (PE)`

## DOS Header

- **Present:** `True`
- **e_magic:** `0x5A4D`
- **e_cblp:** `144`
- **e_cp:** `3`
- **e_crlc:** `0`
- **e_cparhdr:** `4`
- **e_minalloc:** `0`
- **e_maxalloc:** `65535`
- **e_ss:** `0x0`
- **e_sp:** `0xB8`
- **e_csum:** `0x0`
- **e_ip:** `0x0`
- **e_cs:** `0x0`
- **e_lfarlc:** `0x40`
- **e_ovno:** `0`
- **e_res:** `[0, 0, 0, 0]`
- **e_oemid:** `0x0`
- **e_oeminfo:** `0x0`
- **e_res2:** `[0, 0, 0, 0, 0, 0, 0, 0, 0, 0]`
- **e_lfanew:** `0x110`
- **DOS stub size:** `64`

## Rich Header

- **Present:** `True`
- **Offset:** `0x80`
- **Size:** `0x80`
- **Mask:** `0x163115C6`
| Index | Comp ID | Product ID | Product | Build | Object count |
| --- | --- | --- | --- | --- | --- |
| 0 | 0x1058179 | 0x105 | C++ Compiler from VS2015 | 33145 | 142 |
| 1 | 0x1048179 | 0x104 | C Compiler from VS2015 | 33145 | 12 |
| 2 | 0x1038179 | 0x103 | Assembler from VS2015 | 33145 | 6 |
| 3 | 0x0 | 0x0 | Unmarked objects (old) | 0 | 1 |
| 4 | 0x1057C4F | 0x105 | C++ Compiler from VS2015 | 31823 | 42 |
| 5 | 0x1047C4F | 0x104 | C Compiler from VS2015 | 31823 | 16 |
| 6 | 0x1037C4F | 0x103 | Assembler from VS2015 | 31823 | 9 |
| 7 | 0x1018179 | 0x101 | Linker from VS2015 | 33145 | 9 |
| 8 | 0x10000 | 0x1 | Unmarked objects | 0 | 102 |
| 9 | 0x1057CC1 | 0x105 | C++ Compiler from VS2015 | 31937 | 1 |
| 10 | 0x1007CC1 | 0x100 | Linker from VS2015 | 31937 | 1 |
| 11 | 0xFF7CC1 | 0xFF | CVTRes from VS2015 | 31937 | 1 |
| 12 | 0x1027CC1 | 0x102 | Linker from VS2015 | 31937 | 1 |

## NT File Header

- **Machine:** `0x8664`
- **Machine name/value:** `34404`
- **Machine architecture:** `AMD64 / x86-64`
- **Number of sections:** `15`
- **TimeDateStamp:** `0x6AA3DE2E`
- **PointerToSymbolTable:** `0x0`
- **NumberOfSymbols:** `0`
- **SizeOfOptionalHeader:** `240`
- **Characteristics:** `0x22`
- **Decoded characteristics:** `['IMAGE_FILE_EXECUTABLE_IMAGE', 'IMAGE_FILE_LARGE_ADDRESS_AWARE']`
- **Section table file offset:** `0x218`

## NT Optional Header

- **Magic:** `0x20B`
- **PE32+:** `True`
- **Linker version:** `14.34`
- **SizeOfCode:** `0x84000`
- **SizeOfInitializedData:** `0x27200`
- **SizeOfUninitializedData:** `0x0`
- **AddressOfEntryPoint RVA:** `0x2EEB`
- **AddressOfEntryPoint VA:** `0x140002EEB`
- **BaseOfCode RVA:** `0x1000`
- **BaseOfData RVA:** `0xFFFFFFFF`
- **ImageBase:** `0x140000000`
- **SectionAlignment:** `0x1000`
- **FileAlignment:** `0x200`
- **OS version:** `6.0`
- **Image version:** `0.0`
- **Subsystem version:** `6.0`
- **Win32VersionValue:** `0x0`
- **SizeOfImage:** `0xB6000`
- **SizeOfHeaders:** `0x600`
- **CheckSum:** `0x0`
- **Subsystem:** `0x3`
- **DllCharacteristics:** `0x8160`
- **Stack reserve:** `0x100000`
- **Stack commit:** `0x1000`
- **Heap reserve:** `0x100000`
- **Heap commit:** `0x1000`
- **LoaderFlags:** `0x0`
- **NumberOfRvaAndSizes:** `16`
- **CLI/.NET image:** `False`
- **Decoded DLL characteristics:** `['HIGH_ENTROPY_VA', 'DYNAMIC_BASE', 'NX_COMPAT', 'TERMINAL_SERVER_AWARE']`

## PE Data Directories

| Index | Directory | Parser class | RVA | VA | Raw offset | Size | Parsed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | IMAGE_DIRECTORY_ENTRY_EXPORT | ExportDataDirectory | 0x9C8B0 | 0x14009C8B0 | 0x9BEB0 | 0x18D | True |
| 1 | IMAGE_DIRECTORY_ENTRY_IMPORT | ImportDataDirectory | 0xA8528 | 0x1400A8528 | 0xA2F28 | 0x64 | True |
| 2 | IMAGE_DIRECTORY_ENTRY_RESOURCE | ResourceDataDirectory | 0xB3000 | 0x1400B3000 | 0xA6E00 | 0x4A9 | True |
| 3 | IMAGE_DIRECTORY_ENTRY_EXCEPTION | ExceptionDataDirectory | 0xA2000 | 0x1400A2000 | 0x9D600 | 0x4AE8 | True |
| 4 | IMAGE_DIRECTORY_ENTRY_SECURITY | SecurityDataDirectory | 0x0 | N/A | N/A | 0x0 | False |
| 5 | IMAGE_DIRECTORY_ENTRY_BASERELOC | BaseRelocationDataDirectory | 0xB4000 | 0x1400B4000 | 0xA7400 | 0x898 | True |
| 6 | IMAGE_DIRECTORY_ENTRY_DEBUG | DebugDataDirectory | 0x930C0 | 0x1400930C0 | 0x926C0 | 0x38 | True |
| 7 | IMAGE_DIRECTORY_ENTRY_ARCHITECTURE | ArchitectureDataDirectory | 0x0 | N/A | N/A | 0x0 | False |
| 8 | IMAGE_DIRECTORY_ENTRY_GLOBALPTR | GlobalPointerDataDirectory | 0x0 | N/A | N/A | 0x0 | False |
| 9 | IMAGE_DIRECTORY_ENTRY_TLS | TLSDataDirectory | 0x93A20 | 0x140093A20 | 0x93020 | 0x28 | True |
| 10 | IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG | LoadConfigDataDirectory | 0x92F40 | 0x140092F40 | 0x92540 | 0x140 | True |
| 11 | IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT | BoundImportDataDirectory | 0x0 | N/A | N/A | 0x0 | False |
| 12 | IMAGE_DIRECTORY_ENTRY_IAT | ImportAddressTableDataDirectory | 0xA8000 | 0x1400A8000 | 0xA2A00 | 0x528 | True |
| 13 | IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT | DelayImportDataDirectory | 0x0 | N/A | N/A | 0x0 | False |
| 14 | IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR | COMDescriptorDataDirectory | 0x0 | N/A | N/A | 0x0 | False |
| 15 | RESERVED | null | 0 | N/A | N/A | 0 | False |

## PE Section Headers

| # | Name | Raw offset | Raw size | Virtual RVA | Virtual VA | Virtual size | Characteristics | Flags | Reloc ptr/count | Line ptr/count |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | .text | 0x600 | 0x84000 | 0x1000 | 0x140001000 | 0x84000 | 0x60000020 | CODE, EXECUTE, READ | 0x0/0 | 0x0/0 |
| 1 | .rdata | 0x84600 | 0x17C00 | 0x85000 | 0x140085000 | 0x17C00 | 0x40000040 | INITIALIZED_DATA, READ | 0x0/0 | 0x0/0 |
| 2 | .data | 0x9C200 | 0x1400 | 0x9D000 | 0x14009D000 | 0x45D9 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 3 | .pdata | 0x9D600 | 0x5400 | 0xA2000 | 0x1400A2000 | 0x5400 | 0x40000040 | INITIALIZED_DATA, READ | 0x0/0 | 0x0/0 |
| 4 | .idata | 0xA2A00 | 0x1600 | 0xA8000 | 0x1400A8000 | 0x1600 | 0x40000040 | INITIALIZED_DATA, READ | 0x0/0 | 0x0/0 |
| 5 | .neon | 0xA4000 | 0x400 | 0xAA000 | 0x1400AA000 | 0x400 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 6 | .bss | 0xA4400 | 0x1400 | 0xAB000 | 0x1400AB000 | 0x1400 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 7 | .tls | 0xA5800 | 0x400 | 0xAD000 | 0x1400AD000 | 0x400 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 8 | .00cfg | 0xA5C00 | 0x200 | 0xAE000 | 0x1400AE000 | 0x200 | 0x40000040 | INITIALIZED_DATA, READ | 0x0/0 | 0x0/0 |
| 9 | _RDATA | 0xA5E00 | 0x400 | 0xAF000 | 0x1400AF000 | 0x400 | 0x40000040 | INITIALIZED_DATA, READ | 0x0/0 | 0x0/0 |
| 10 | .fptable | 0xA6200 | 0x400 | 0xB0000 | 0x1400B0000 | 0x400 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 11 | _guard_c | 0xA6600 | 0x400 | 0xB1000 | 0x1400B1000 | 0x400 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 12 | _guard_d | 0xA6A00 | 0x400 | 0xB2000 | 0x1400B2000 | 0x400 | 0xC0000040 | INITIALIZED_DATA, READ, WRITE | 0x0/0 | 0x0/0 |
| 13 | .rsrc | 0xA6E00 | 0x600 | 0xB3000 | 0x1400B3000 | 0x600 | 0x40000040 | INITIALIZED_DATA, READ | 0x0/0 | 0x0/0 |
| 14 | .reloc | 0xA7400 | 0x1200 | 0xB4000 | 0x1400B4000 | 0x1200 | 0x42000040 | INITIALIZED_DATA, DISCARDABLE, READ | 0x0/0 | 0x0/0 |

## PE Export Directory

- **Export name RVA:** `0x9C8EC`
- **Export name:** `neon_heist.exe`
- **Characteristics:** `0x0`
- **TimeDateStamp:** `0xFFFFFFFF`
- **Version:** `0.0`
- **Ordinal base:** `1`
- **Number of functions:** `2`
- **Number of names:** `2`
- **AddressOfFunctions RVA:** `0x9C8D8`
- **AddressOfFunctions VA:** `0x14009C8D8`
- **AddressOfNames RVA:** `0x9C8E0`
- **AddressOfNames VA:** `0x14009C8E0`
- **AddressOfNameOrdinals RVA:** `0x9C8E8`
- **AddressOfNameOrdinals VA:** `0x14009C8E8`

## Exported Functions

| Ordinal | Name | Address VA | Address RVA | Raw offset | Forwarded | Comment |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | NeonExportedChecksum | 0x140002AF9 | 0x2AF9 | 0x20F9 | False | 0x2af9  1  NeonExportedChecksum |
| 2 | NeonExportedTransform | 0x140003DA0 | 0x3DA0 | 0x33A0 | False | 0x3da0  2  NeonExportedTransform |

## PE Import Directory

| DLL | Descriptor name RVA | Descriptor raw offset | OriginalFirstThunk RVA | FirstThunk/IAT RVA | TimeDateStamp | Bound |
| --- | --- | --- | --- | --- | --- | --- |
| USER32.dll | 0xA8AC6 | 0xA34C6 | 0xA89F8 | 0xA8468 | 0x0 | False |
| ADVAPI32.dll | 0xA8AF0 | 0xA34F0 | 0xA8590 | 0xA8000 | 0x0 | False |
| bcrypt.dll | 0xA8B10 | 0xA3510 | 0xA8A58 | 0xA84C8 | 0x0 | False |
| KERNEL32.dll | 0xA8B82 | 0xA3582 | 0xA85F8 | 0xA8068 | 0x0 | False |

## Imported Functions / IAT Addresses

> Random sample: showing 20 of 102 rows from `Imported Functions / IAT Addresses`. Use `--verbose` to include every row.

| DLL | Name / ordinal | Hint | INT slot RVA | IAT slot RVA | IAT slot VA | IAT slot raw offset | Thunk value | ImportByName RVA | ImportByName raw offset |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| KERNEL32.dll | GetProcessHeap | 740 | 0xA88C0 | 0xA8330 | 0x1400A8330 | 0xA2D30 | 0xA915A | 0xA915A | 0xA3B5A |
| KERNEL32.dll | GetCommandLineA | 511 | 0xA8798 | 0xA8208 | 0x1400A8208 | 0xA2C08 | 0xA8ECE | 0xA8ECE | 0xA38CE |
| KERNEL32.dll | GetOEMCP | 710 | 0xA8878 | 0xA82E8 | 0x1400A82E8 | 0xA2CE8 | 0xA90A6 | 0xA90A6 | 0xA3AA6 |
| KERNEL32.dll | GetFileType | 634 | 0xA8840 | 0xA82B0 | 0x1400A82B0 | 0xA2CB0 | 0xA9036 | 0xA9036 | 0xA3A36 |
| KERNEL32.dll | MultiByteToWideChar | 1059 | 0xA8888 | 0xA82F8 | 0x1400A82F8 | 0xA2CF8 | 0xA90BE | 0xA90BE | 0xA3ABE |
| KERNEL32.dll | FlsGetValue | 452 | 0xA87D0 | 0xA8240 | 0x1400A8240 | 0xA2C40 | 0xA8F3A | 0xA8F3A | 0xA393A |
| KERNEL32.dll | FlsSetValue | 454 | 0xA87D8 | 0xA8248 | 0x1400A8248 | 0xA2C48 | 0xA8F48 | 0xA8F48 | 0xA3948 |
| KERNEL32.dll | WriteConsoleW | 1631 | 0xA8788 | 0xA81F8 | 0x1400A81F8 | 0xA2BF8 | 0xA9236 | 0xA9236 | 0xA3C36 |
| KERNEL32.dll | TlsFree | 1515 | 0xA8748 | 0xA81B8 | 0x1400A81B8 | 0xA2BB8 | 0xA8E30 | 0xA8E30 | 0xA3830 |
| KERNEL32.dll | ReadConsoleW | 1190 | 0xA8608 | 0xA8078 | 0x1400A8078 | 0xA2A78 | 0xA9218 | 0xA9218 | 0xA3C18 |
| KERNEL32.dll | TlsAlloc | 1514 | 0xA8730 | 0xA81A0 | 0x1400A81A0 | 0xA2BA0 | 0xA8E08 | 0xA8E08 | 0xA3808 |
| KERNEL32.dll | SetEnvironmentVariableW | 1370 | 0xA88A8 | 0xA8318 | 0x1400A8318 | 0xA2D18 | 0xA911E | 0xA911E | 0xA3B1E |
| KERNEL32.dll | RtlCaptureContext | 1289 | 0xA8678 | 0xA80E8 | 0x1400A80E8 | 0xA2AE8 | 0xA8C06 | 0xA8C06 | 0xA3606 |
| KERNEL32.dll | GetConsoleMode | 549 | 0xA88E0 | 0xA8350 | 0x1400A8350 | 0xA2D50 | 0xA91AE | 0xA91AE | 0xA3BAE |
| KERNEL32.dll | UnhandledExceptionFilter | 1531 | 0xA8698 | 0xA8108 | 0x1400A8108 | 0xA2B08 | 0xA8C5C | 0xA8C5C | 0xA365C |
| KERNEL32.dll | HeapFree | 896 | 0xA87B8 | 0xA8228 | 0x1400A8228 | 0xA2C28 | 0xA8F12 | 0xA8F12 | 0xA3912 |
| KERNEL32.dll | RtlVirtualUnwind | 1304 | 0xA8688 | 0xA80F8 | 0x1400A80F8 | 0xA2AF8 | 0xA8C34 | 0xA8C34 | 0xA3634 |
| KERNEL32.dll | GetCurrentProcess | 577 | 0xA86C0 | 0xA8130 | 0x1400A8130 | 0xA2B30 | 0xA8CD8 | 0xA8CD8 | 0xA36D8 |
| KERNEL32.dll | SetFilePointerEx | 1385 | 0xA88F0 | 0xA8360 | 0x1400A8360 | 0xA2D60 | 0xA91D0 | 0xA91D0 | 0xA3BD0 |
| KERNEL32.dll | GetConsoleOutputCP | 553 | 0xA88D8 | 0xA8348 | 0x1400A8348 | 0xA2D48 | 0xA9198 | 0xA9198 | 0xA3B98 |

## PE Debug Directory

| Characteristics | TimeDateStamp | Version | Type | Description | SizeOfData | AddressOfRawData RVA | PointerToRawData |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0x0 | 0x6AA3DE2E | 0.0 | 2 | CodeView | 0x70 | 0x946D8 | 0x93CD8 |
| 0x0 | 0x6AA3DE2E | 0.0 | 12 | DebugType-12 | 0x14 | 0x94748 | 0x93D48 |
- **CodeView parser present:** `True`
- **CodeView PDB info present:** `False`
- **CodeView PDB valid:** `False`
- **.NET PDB info present:** `True`
- **.NET PDB valid:** `True`

## PE Exception / Runtime Function Directory

> Random sample: showing 20 of 1598 rows from `PE Exception / Runtime Function Directory`. Use `--verbose` to include every row.

| Entry | Begin RVA | Begin VA | End RVA | End VA | Unwind RVA | Unwind VA | Unwind raw offset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 615 | 0x3B658 | 0x14003B658 | 0x3B781 | 0x14003B781 | 0x96F44 | 0x140096F44 | 0x96544 |
| 2 | 0x7730 | 0x140007730 | 0x776C | 0x14000776C | 0x94D08 | 0x140094D08 | 0x94308 |
| 228 | 0x12C2C | 0x140012C2C | 0x12C78 | 0x140012C78 | 0x960F0 | 0x1400960F0 | 0x956F0 |
| 40 | 0x94E8 | 0x1400094E8 | 0x9521 | 0x140009521 | 0x94FA0 | 0x140094FA0 | 0x945A0 |
| 1125 | 0x60AA0 | 0x140060AA0 | 0x60BD5 | 0x140060BD5 | 0x9A15C | 0x14009A15C | 0x9975C |
| 296 | 0x1BD58 | 0x14001BD58 | 0x1C02C | 0x14001C02C | 0x95CF0 | 0x140095CF0 | 0x952F0 |
| 962 | 0x56A40 | 0x140056A40 | 0x56A75 | 0x140056A75 | 0x994B8 | 0x1400994B8 | 0x98AB8 |
| 1516 | 0x7D450 | 0x14007D450 | 0x7D517 | 0x14007D517 | 0x9C028 | 0x14009C028 | 0x9B628 |
| 819 | 0x50FD8 | 0x140050FD8 | 0x510B6 | 0x1400510B6 | 0x98BB8 | 0x140098BB8 | 0x981B8 |
| 200 | 0x11CB0 | 0x140011CB0 | 0x11D45 | 0x140011D45 | 0x961C4 | 0x1400961C4 | 0x957C4 |
| 574 | 0x34EA0 | 0x140034EA0 | 0x34FDB | 0x140034FDB | 0x975B0 | 0x1400975B0 | 0x96BB0 |
| 1569 | 0x83A80 | 0x140083A80 | 0x83A9D | 0x140083A9D | 0x99C04 | 0x140099C04 | 0x99204 |
| 1421 | 0x766C4 | 0x1400766C4 | 0x76791 | 0x140076791 | 0x9B810 | 0x14009B810 | 0x9AE10 |
| 9 | 0x7A30 | 0x140007A30 | 0x7A5B | 0x140007A5B | 0x94DE4 | 0x140094DE4 | 0x943E4 |
| 560 | 0x34954 | 0x140034954 | 0x34977 | 0x140034977 | 0x97C80 | 0x140097C80 | 0x97280 |
| 672 | 0x44788 | 0x140044788 | 0x449E1 | 0x1400449E1 | 0x96CAC | 0x140096CAC | 0x962AC |
| 655 | 0x42028 | 0x140042028 | 0x4259D | 0x14004259D | 0x9846C | 0x14009846C | 0x97A6C |
| 1401 | 0x75BA0 | 0x140075BA0 | 0x75BB3 | 0x140075BB3 | 0x9B6D0 | 0x14009B6D0 | 0x9ACD0 |
| 1568 | 0x83A5F | 0x140083A5F | 0x83A7A | 0x140083A7A | 0x99BD0 | 0x140099BD0 | 0x991D0 |
| 1509 | 0x7CC70 | 0x14007CC70 | 0x7CE33 | 0x14007CE33 | 0x9BF90 | 0x14009BF90 | 0x9B590 |

## PE Base Relocation Directory

> Random sample: showing 20 of 1036 rows from `PE Base Relocation Directory`. Use `--verbose` to include every row.

| Block RVA | Block VA | Block size | Entry index | Type | Type name | Offset | Target RVA | Target VA |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0x8D000 | 0x14008D000 | 0x80 | 41 | 10 | DIR64 | 0x298 | 0x8D298 | 0x14008D298 |
| 0x86000 | 0x140086000 | 0x140 | 16 | 10 | DIR64 | 0xA0 | 0x860A0 | 0x1400860A0 |
| 0x8E000 | 0x14008E000 | 0x1A8 | 46 | 10 | DIR64 | 0x5F0 | 0x8E5F0 | 0x14008E5F0 |
| 0x8B000 | 0x14008B000 | 0xE0 | 54 | 10 | DIR64 | 0x490 | 0x8B490 | 0x14008B490 |
| 0x89000 | 0x140089000 | 0xB8 | 44 | 10 | DIR64 | 0xDD8 | 0x89DD8 | 0x140089DD8 |
| 0x8E000 | 0x14008E000 | 0x1A8 | 56 | 10 | DIR64 | 0x690 | 0x8E690 | 0x14008E690 |
| 0x86000 | 0x140086000 | 0x140 | 70 | 10 | DIR64 | 0x7E0 | 0x867E0 | 0x1400867E0 |
| 0x86000 | 0x140086000 | 0x140 | 83 | 10 | DIR64 | 0x8B0 | 0x868B0 | 0x1400868B0 |
| 0x86000 | 0x140086000 | 0x140 | 81 | 10 | DIR64 | 0x890 | 0x86890 | 0x140086890 |
| 0x92000 | 0x140092000 | 0x48 | 12 | 10 | DIR64 | 0xC48 | 0x92C48 | 0x140092C48 |
| 0x86000 | 0x140086000 | 0x140 | 140 | 10 | DIR64 | 0xC40 | 0x86C40 | 0x140086C40 |
| 0x89000 | 0x140089000 | 0xB8 | 16 | 10 | DIR64 | 0xCF0 | 0x89CF0 | 0x140089CF0 |
| 0x89000 | 0x140089000 | 0xB8 | 62 | 10 | DIR64 | 0xE68 | 0x89E68 | 0x140089E68 |
| 0x8B000 | 0x14008B000 | 0xE0 | 99 | 10 | DIR64 | 0x840 | 0x8B840 | 0x14008B840 |
| 0x8A000 | 0x14008A000 | 0x3C | 18 | 10 | DIR64 | 0x4E0 | 0x8A4E0 | 0x14008A4E0 |
| 0x8B000 | 0x14008B000 | 0xE0 | 39 | 10 | DIR64 | 0x3A0 | 0x8B3A0 | 0x14008B3A0 |
| 0x87000 | 0x140087000 | 0x54 | 23 | 10 | DIR64 | 0xC68 | 0x87C68 | 0x140087C68 |
| 0x8F000 | 0x14008F000 | 0x34 | 7 | 10 | DIR64 | 0x70 | 0x8F070 | 0x14008F070 |
| 0x8E000 | 0x14008E000 | 0x1A8 | 42 | 10 | DIR64 | 0x5B0 | 0x8E5B0 | 0x14008E5B0 |
| 0x92000 | 0x140092000 | 0x48 | 23 | 10 | DIR64 | 0xCF8 | 0x92CF8 | 0x140092CF8 |

## PE Resource Directory

- **Characteristics:** `0x0`
- **TimeDateStamp:** `0x0`
- **Version:** `0.0`
- **Named entries:** `0`
- **ID entries:** `2`

## Resource Leaves

| Type ID | Type | ID | Name | RVA/VA | Raw offset | Size |
| --- | --- | --- | --- | --- | --- | --- |
| 10 | Rsrc_RC_Data_65_409 | 101 | Rsrc_RC_Data_65_409 | 0xB3338 / 0x1400B3338 | 0xA7138 | 0x20 |
| 24 | Rsrc_Manifest_1_409 | 1 | Rsrc_Manifest_1_409 | 0xB31C0 / 0x1400B31C0 | 0xA6FC0 | 0x173 |

## PE Security / Authenticode Directory

| Index | Length | Revision | Type | Type name | Certificate bytes |
| --- | --- | --- | --- | --- | --- |

## PE TLS Directory

| Start raw VA | End raw VA | AddressOfIndex VA | AddressOfCallbacks VA | Zero fill | Characteristics |
| --- | --- | --- | --- | --- | --- |
| 0x1400AD000 | 0x1400AD101 | 0x14009F678 | 0x1400858B8 | 0x0 | 0x100000 |

## PE Load Config Directory

- **Size:** `0x140`
- **CriticalSectionDefaultTimeout:** `0x0`
- **SEHandlerTable:** `0x0`
- **SEHandlerCount:** `0`
- **CFG guard flags:** `0x100`
- **CFG check function:** `0x1400AE000`
- **CFG dispatch function:** `0x1400AE020`
- **CFG function table:** `0x0`
- **CFG function count:** `0`
- **Guard address-taken IAT table:** `0x0`
- **Guard address-taken IAT count:** `0`
- **CHPE metadata:** `None`
- **ARM64EC metadata:** `None`
- **RFG failure routine:** `0x0`
- **RFG failure routine pointer:** `0x0`
- **RFG verify stack pointer:** `0x0`
- **Dynamic relocation table object:** `null`

## PE Bound Import Directory

| Module | TimeDateStamp | Name offset | Forwarder refs |
| --- | --- | --- | --- |

## PE Delay Import Directory

| DLL | Valid | Uses RVA | Attributes | Module handle | IAT | INT | Bound IAT | Unload IAT | TimeDateStamp |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## Delay Imported Functions

| DLL | Name | Address RVA/VA | Bound | Comment |
| --- | --- | --- | --- | --- |

## PE COM/.NET Descriptor

- **Present:** `False`

## Memory Blocks

| Name | Start | End | Size | Read | Write | Execute | Volatile | Initialized | Overlay |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Headers | 140000000 | 1400005ff | 0x600 | True | False | False | False | True | False |
| .text | 140001000 | 140084fff | 0x84000 | True | False | True | False | True | False |
| .rdata | 140085000 | 14009cbff | 0x17C00 | True | False | False | False | True | False |
| .data | 14009d000 | 1400a15d8 | 0x45D9 | True | True | False | False | True | False |
| .pdata | 1400a2000 | 1400a73ff | 0x5400 | True | False | False | False | True | False |
| .idata | 1400a8000 | 1400a95ff | 0x1600 | True | False | False | False | True | False |
| .neon | 1400aa000 | 1400aa3ff | 0x400 | True | True | False | False | True | False |
| .bss | 1400ab000 | 1400ac3ff | 0x1400 | True | True | False | False | True | False |
| .tls | 1400ad000 | 1400ad3ff | 0x400 | True | True | False | False | True | False |
| .00cfg | 1400ae000 | 1400ae1ff | 0x200 | True | False | False | False | True | False |
| _RDATA | 1400af000 | 1400af3ff | 0x400 | True | False | False | False | True | False |
| .fptable | 1400b0000 | 1400b03ff | 0x400 | True | True | False | False | True | False |
| _guard_c | 1400b1000 | 1400b13ff | 0x400 | True | True | False | False | True | False |
| _guard_d | 1400b2000 | 1400b23ff | 0x400 | True | True | False | False | True | False |
| .rsrc | 1400b3000 | 1400b35ff | 0x600 | True | False | False | False | True | False |
| .reloc | 1400b4000 | 1400b51ff | 0x1200 | True | False | False | False | True | False |

## Loaded Image / Section Mapping

| Block | Start VA | End VA | Size | Permissions | Initialized |
| --- | --- | --- | --- | --- | --- |
| Headers | 140000000 | 1400005ff | 0x600 | R-- | True |
| .text | 140001000 | 140084fff | 0x84000 | R-X | True |
| .rdata | 140085000 | 14009cbff | 0x17C00 | R-- | True |
| .data | 14009d000 | 1400a15d8 | 0x45D9 | RW- | True |
| .pdata | 1400a2000 | 1400a73ff | 0x5400 | R-- | True |
| .idata | 1400a8000 | 1400a95ff | 0x1600 | R-- | True |
| .neon | 1400aa000 | 1400aa3ff | 0x400 | RW- | True |
| .bss | 1400ab000 | 1400ac3ff | 0x1400 | RW- | True |
| .tls | 1400ad000 | 1400ad3ff | 0x400 | RW- | True |
| .00cfg | 1400ae000 | 1400ae1ff | 0x200 | R-- | True |
| _RDATA | 1400af000 | 1400af3ff | 0x400 | R-- | True |
| .fptable | 1400b0000 | 1400b03ff | 0x400 | RW- | True |
| _guard_c | 1400b1000 | 1400b13ff | 0x400 | RW- | True |
| _guard_d | 1400b2000 | 1400b23ff | 0x400 | RW- | True |
| .rsrc | 1400b3000 | 1400b35ff | 0x600 | R-- | True |
| .reloc | 1400b4000 | 1400b51ff | 0x1200 | R-- | True |

## Listing Summary

| Entity | Count |
| --- | --- |
| Instructions | 0 |
| Defined data | 4731 |
| Functions | 1554 |

## External Symbols

> Random sample: showing 20 of 102 rows from `External Symbols`. Use `--verbose` to include every row.

| Address | Name | Symbol type | Library |
| --- | --- | --- | --- |
| EXTERNAL:00000037 | WriteConsoleW | Label | KERNEL32.DLL |
| EXTERNAL:00000016 | RtlLookupFunctionEntry | Label | KERNEL32.DLL |
| EXTERNAL:00000046 | GetDateFormatW | Label | KERNEL32.DLL |
| EXTERNAL:00000026 | SetLastError | Label | KERNEL32.DLL |
| EXTERNAL:0000004b | IsValidLocale | Label | KERNEL32.DLL |
| EXTERNAL:00000053 | IsValidCodePage | Label | KERNEL32.DLL |
| EXTERNAL:00000019 | UnhandledExceptionFilter | Label | KERNEL32.DLL |
| EXTERNAL:00000038 | GetModuleHandleExW | Label | KERNEL32.DLL |
| EXTERNAL:00000004 | BCryptGenRandom | Label | BCRYPT.DLL |
| EXTERNAL:0000005e | GetProcessHeap | Label | KERNEL32.DLL |
| EXTERNAL:00000002 | RegOpenKeyExA | Label | ADVAPI32.DLL |
| EXTERNAL:00000042 | FlsFree | Label | KERNEL32.DLL |
| EXTERNAL:00000017 | RtlVirtualUnwind | Label | KERNEL32.DLL |
| EXTERNAL:00000028 | EnterCriticalSection | Label | KERNEL32.DLL |
| EXTERNAL:0000001d | GetModuleHandleW | Label | KERNEL32.DLL |
| EXTERNAL:00000058 | WideCharToMultiByte | Label | KERNEL32.DLL |
| EXTERNAL:00000009 | GetTickCount | Label | KERNEL32.DLL |
| EXTERNAL:00000033 | RtlUnwind | Label | KERNEL32.DLL |
| EXTERNAL:00000063 | GetFileSizeEx | Label | KERNEL32.DLL |
| EXTERNAL:0000005a | FreeEnvironmentStringsW | Label | KERNEL32.DLL |

## Entry Points

| Address | Name |
| --- | --- |
| 140007520 | external |
| 140002af9 | external |
| 140003da0 | external |
| 140002eeb | external |
| 140000000 | IMAGE_DOS_HEADER_140000000 |

## Program Properties

| Property | Value |
| --- | --- |
| Executable Format | Portable Executable (PE) |
| Compiler | visualstudio:unknown |
