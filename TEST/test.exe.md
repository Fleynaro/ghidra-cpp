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

| Address | Name | Symbol type | Library |
| --- | --- | --- | --- |
| EXTERNAL:00000001 | MessageBeep | Label | USER32.DLL |
| EXTERNAL:00000002 | RegOpenKeyExA | Label | ADVAPI32.DLL |
| EXTERNAL:00000003 | RegCloseKey | Label | ADVAPI32.DLL |
| EXTERNAL:00000004 | BCryptGenRandom | Label | BCRYPT.DLL |
| EXTERNAL:00000005 | CreateFileW | Label | KERNEL32.DLL |
| EXTERNAL:00000006 | ExitProcess | Label | KERNEL32.DLL |
| EXTERNAL:00000007 | ReadConsoleW | Label | KERNEL32.DLL |
| EXTERNAL:00000008 | ReadFile | Label | KERNEL32.DLL |
| EXTERNAL:00000009 | GetTickCount | Label | KERNEL32.DLL |
| EXTERNAL:0000000a | LoadResource | Label | KERNEL32.DLL |
| EXTERNAL:0000000b | LockResource | Label | KERNEL32.DLL |
| EXTERNAL:0000000c | SizeofResource | Label | KERNEL32.DLL |
| EXTERNAL:0000000d | FindResourceA | Label | KERNEL32.DLL |
| EXTERNAL:0000000e | SetConsoleTitleA | Label | KERNEL32.DLL |
| EXTERNAL:0000000f | CloseHandle | Label | KERNEL32.DLL |
| EXTERNAL:00000010 | QueryPerformanceCounter | Label | KERNEL32.DLL |
| EXTERNAL:00000011 | GetCurrentProcessId | Label | KERNEL32.DLL |
| EXTERNAL:00000012 | GetCurrentThreadId | Label | KERNEL32.DLL |
| EXTERNAL:00000013 | GetSystemTimeAsFileTime | Label | KERNEL32.DLL |
| EXTERNAL:00000014 | InitializeSListHead | Label | KERNEL32.DLL |
| EXTERNAL:00000015 | RtlCaptureContext | Label | KERNEL32.DLL |
| EXTERNAL:00000016 | RtlLookupFunctionEntry | Label | KERNEL32.DLL |
| EXTERNAL:00000017 | RtlVirtualUnwind | Label | KERNEL32.DLL |
| EXTERNAL:00000018 | IsDebuggerPresent | Label | KERNEL32.DLL |
| EXTERNAL:00000019 | UnhandledExceptionFilter | Label | KERNEL32.DLL |
| EXTERNAL:0000001a | SetUnhandledExceptionFilter | Label | KERNEL32.DLL |
| EXTERNAL:0000001b | GetStartupInfoW | Label | KERNEL32.DLL |
| EXTERNAL:0000001c | IsProcessorFeaturePresent | Label | KERNEL32.DLL |
| EXTERNAL:0000001d | GetModuleHandleW | Label | KERNEL32.DLL |
| EXTERNAL:0000001e | GetCurrentProcess | Label | KERNEL32.DLL |
| EXTERNAL:0000001f | TerminateProcess | Label | KERNEL32.DLL |
| EXTERNAL:00000020 | RtlPcToFileHeader | Label | KERNEL32.DLL |
| EXTERNAL:00000021 | RaiseException | Label | KERNEL32.DLL |
| EXTERNAL:00000022 | RtlUnwindEx | Label | KERNEL32.DLL |
| EXTERNAL:00000023 | InterlockedPushEntrySList | Label | KERNEL32.DLL |
| EXTERNAL:00000024 | InterlockedFlushSList | Label | KERNEL32.DLL |
| EXTERNAL:00000025 | GetLastError | Label | KERNEL32.DLL |
| EXTERNAL:00000026 | SetLastError | Label | KERNEL32.DLL |
| EXTERNAL:00000027 | EncodePointer | Label | KERNEL32.DLL |
| EXTERNAL:00000028 | EnterCriticalSection | Label | KERNEL32.DLL |
| EXTERNAL:00000029 | LeaveCriticalSection | Label | KERNEL32.DLL |
| EXTERNAL:0000002a | DeleteCriticalSection | Label | KERNEL32.DLL |
| EXTERNAL:0000002b | InitializeCriticalSectionAndSpinCount | Label | KERNEL32.DLL |
| EXTERNAL:0000002c | TlsAlloc | Label | KERNEL32.DLL |
| EXTERNAL:0000002d | TlsGetValue | Label | KERNEL32.DLL |
| EXTERNAL:0000002e | TlsSetValue | Label | KERNEL32.DLL |
| EXTERNAL:0000002f | TlsFree | Label | KERNEL32.DLL |
| EXTERNAL:00000030 | FreeLibrary | Label | KERNEL32.DLL |
| EXTERNAL:00000031 | GetProcAddress | Label | KERNEL32.DLL |
| EXTERNAL:00000032 | LoadLibraryExW | Label | KERNEL32.DLL |
| EXTERNAL:00000033 | RtlUnwind | Label | KERNEL32.DLL |
| EXTERNAL:00000034 | GetStdHandle | Label | KERNEL32.DLL |
| EXTERNAL:00000035 | WriteFile | Label | KERNEL32.DLL |
| EXTERNAL:00000036 | GetModuleFileNameW | Label | KERNEL32.DLL |
| EXTERNAL:00000037 | WriteConsoleW | Label | KERNEL32.DLL |
| EXTERNAL:00000038 | GetModuleHandleExW | Label | KERNEL32.DLL |
| EXTERNAL:00000039 | GetCommandLineA | Label | KERNEL32.DLL |
| EXTERNAL:0000003a | GetCommandLineW | Label | KERNEL32.DLL |
| EXTERNAL:0000003b | GetCurrentThread | Label | KERNEL32.DLL |
| EXTERNAL:0000003c | HeapAlloc | Label | KERNEL32.DLL |
| EXTERNAL:0000003d | HeapFree | Label | KERNEL32.DLL |
| EXTERNAL:0000003e | GetTempPathW | Label | KERNEL32.DLL |
| EXTERNAL:0000003f | FlsAlloc | Label | KERNEL32.DLL |
| EXTERNAL:00000040 | FlsGetValue | Label | KERNEL32.DLL |
| EXTERNAL:00000041 | FlsSetValue | Label | KERNEL32.DLL |
| EXTERNAL:00000042 | FlsFree | Label | KERNEL32.DLL |
| EXTERNAL:00000043 | IsThreadAFiber | Label | KERNEL32.DLL |
| EXTERNAL:00000044 | InitializeCriticalSectionEx | Label | KERNEL32.DLL |
| EXTERNAL:00000045 | VirtualProtect | Label | KERNEL32.DLL |
| EXTERNAL:00000046 | GetDateFormatW | Label | KERNEL32.DLL |
| EXTERNAL:00000047 | GetTimeFormatW | Label | KERNEL32.DLL |
| EXTERNAL:00000048 | CompareStringW | Label | KERNEL32.DLL |
| EXTERNAL:00000049 | LCMapStringW | Label | KERNEL32.DLL |
| EXTERNAL:0000004a | GetLocaleInfoW | Label | KERNEL32.DLL |
| EXTERNAL:0000004b | IsValidLocale | Label | KERNEL32.DLL |
| EXTERNAL:0000004c | GetUserDefaultLCID | Label | KERNEL32.DLL |
| EXTERNAL:0000004d | EnumSystemLocalesW | Label | KERNEL32.DLL |
| EXTERNAL:0000004e | GetFileType | Label | KERNEL32.DLL |
| EXTERNAL:0000004f | OutputDebugStringW | Label | KERNEL32.DLL |
| EXTERNAL:00000050 | FindClose | Label | KERNEL32.DLL |
| EXTERNAL:00000051 | FindFirstFileExW | Label | KERNEL32.DLL |
| EXTERNAL:00000052 | FindNextFileW | Label | KERNEL32.DLL |
| EXTERNAL:00000053 | IsValidCodePage | Label | KERNEL32.DLL |
| EXTERNAL:00000054 | GetACP | Label | KERNEL32.DLL |
| EXTERNAL:00000055 | GetOEMCP | Label | KERNEL32.DLL |
| EXTERNAL:00000056 | GetCPInfo | Label | KERNEL32.DLL |
| EXTERNAL:00000057 | MultiByteToWideChar | Label | KERNEL32.DLL |
| EXTERNAL:00000058 | WideCharToMultiByte | Label | KERNEL32.DLL |
| EXTERNAL:00000059 | GetEnvironmentStringsW | Label | KERNEL32.DLL |
| EXTERNAL:0000005a | FreeEnvironmentStringsW | Label | KERNEL32.DLL |
| EXTERNAL:0000005b | SetEnvironmentVariableW | Label | KERNEL32.DLL |
| EXTERNAL:0000005c | SetStdHandle | Label | KERNEL32.DLL |
| EXTERNAL:0000005d | GetStringTypeW | Label | KERNEL32.DLL |
| EXTERNAL:0000005e | GetProcessHeap | Label | KERNEL32.DLL |
| EXTERNAL:0000005f | SetConsoleCtrlHandler | Label | KERNEL32.DLL |
| EXTERNAL:00000060 | FlushFileBuffers | Label | KERNEL32.DLL |
| EXTERNAL:00000061 | GetConsoleOutputCP | Label | KERNEL32.DLL |
| EXTERNAL:00000062 | GetConsoleMode | Label | KERNEL32.DLL |
| EXTERNAL:00000063 | GetFileSizeEx | Label | KERNEL32.DLL |
| EXTERNAL:00000064 | SetFilePointerEx | Label | KERNEL32.DLL |
| EXTERNAL:00000065 | HeapSize | Label | KERNEL32.DLL |
| EXTERNAL:00000066 | HeapReAlloc | Label | KERNEL32.DLL |

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
