# Sleigh Runtime Decoder

`sleigh_runtime` is a standalone C++23 runtime for decoding machine-code bytes with a compiled Ghidra Sleigh processor specification. It exposes a small owning data model instead of leaking the legacy Ghidra runtime classes to callers.

The runtime returns:

- The decoded instruction length and address.
- The mnemonic and rendered operands.
- Operand kind and immediate/address values when available.
- Control-flow classification and a concrete p-code target when available.
- Materialized p-code operations with concrete input and output varnodes.

The implementation intentionally does not parse `.slaspec`. All instruction patterns, operand renderers, context fields, and p-code templates come from the compiled binary `.sla` file supplied by the caller.

## Directory Layout

- [`CMakeLists.txt`](CMakeLists.txt) builds the static runtime library, CLI, and tests.
- [`sleigh_runtime.cppm`](sleigh_runtime.cppm) exports the public C++23 module named `sleigh_runtime` and its owning result model.
- [`sleigh_runtime_adapter.cppm`](sleigh_runtime_adapter.cppm) adapts the reference runtime to in-memory instruction bytes and owns decode state.
- [`src/README.md`](src/README.md) documents the runtime implementation sources.
- [`src/README.md`](src/README.md) documents the private runtime module implementation.
- [`cli/sleigh_runtime_decode.cppm`](cli/sleigh_runtime_decode.cppm) implements the console decoder executable.
- [`cli/README.md`](cli/README.md) documents the CLI in detail.
- [`tests/sleigh_runtime_tests.cppm`](tests/sleigh_runtime_tests.cppm) contains end-to-end runtime tests.
- [`tests/README.md`](tests/README.md) documents the test target and fixtures.
- [`test_data/x86-64.sla`](test_data/x86-64.sla) is the module-local compiled x86-64 processor specification.
- [`test_data/README.md`](test_data/README.md) documents the processor fixture.

## Library API

Create a `sleigh_runtime::Decoder` with a compiled `.sla` path and call `decode` with an instruction address, a byte span, and optional processor context values:

```cpp
#include <array>
#include <cstdint>

import sleigh_runtime;

const sleigh_runtime::ProcessorContext context{
    {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}}};
sleigh_runtime::Decoder decoder("x86-64.sla");
const std::array<std::uint8_t, 3> bytes{0x48, 0x8b, 0xd9};
const auto result = decoder.decode(0x140000000ULL, bytes, context);
```

`ProcessorContext::values` contains named `ContextValue` objects and owns their strings. `decode` processes one instruction and returns `std::expected<Instruction, DecodeError>`. The runtime requires at most 16 input bytes per call; callers decoding a stream should advance by `Instruction::length` and pass the remaining bytes in windows no larger than 16 bytes. The CLI performs this loop automatically.

For x86-64, the module-local SLA expects `addrsize=2`, `opsize=1`, `rexprefix=0`, and `longMode=1` for the default 64-bit mode. Other processor specifications can require different context field names and values.

## Console Tool

The module builds `sleigh_runtime_decode.exe`. It statically links the `sleigh_runtime` library and accepts a compiled SLA file plus one or more machine instructions represented as hexadecimal bytes.

Run it from the `NEW` directory:

```powershell
.\build\features\sleigh_runtime\sleigh_runtime_decode.exe `
    --sla .\features\sleigh_runtime\test_data\x86-64.sla `
    --hex "48 8b d9 48 83 ec 40"
```

The CLI decodes the complete input until all bytes are consumed. Every decoded instruction receives a separate output block containing:

- Sequential instruction number.
- Effective address.
- Bytes consumed by that instruction.
- Instruction length and assembly text.
- Control-flow kind.
- Numbered materialized p-code operations.

The command accepts these options:

| Option | Description |
| --- | --- |
| `--sla <path>` | Path to the compiled binary Sleigh specification. Required. |
| `--hex <bytes>` | One or more bytes written as two hexadecimal digits, separated by spaces, tabs, newlines, or commas. Required. |
| `--address <value>` | Starting instruction address in decimal or `0x` hexadecimal notation. Defaults to `0x140000000`. |
| `--context <name=value>` | Overrides a Sleigh processor context field. May be repeated. |
| `--help` | Prints command-line help. |

Examples:

```powershell
# Decode one instruction.
.\build\features\sleigh_runtime\sleigh_runtime_decode.exe `
    --sla .\features\sleigh_runtime\test_data\x86-64.sla `
    --hex "48 8b d9"

# Decode a sequence at a custom address.
.\build\features\sleigh_runtime\sleigh_runtime_decode.exe `
    --sla .\features\sleigh_runtime\test_data\x86-64.sla `
    --address 0x140010000 `
    --hex "57 48 83 ec 40 c3"

# Override processor context values.
.\build\features\sleigh_runtime\sleigh_runtime_decode.exe `
    --sla .\features\sleigh_runtime\test_data\x86-64.sla `
    --hex "48 8b d9" `
    --context longMode=1 --context addrsize=2
```

The CLI returns exit code `0` when the complete input is decoded, `1` for invalid command-line input or an unavailable SLA file, and `2` when instruction decoding fails.

## Build And Tests

The repository build uses MSVC, CMake, Ninja, vcpkg, and C++23. From [`NEW/build.bat`](../../build.bat), the standard workflow configures the build directory, builds all targets, and runs CTest:

```powershell
cd NEW
.\build.bat
```

The main generated artifacts are:

- `NEW/build/features/sleigh_runtime/sleigh_runtime.lib`: static runtime library.
- `NEW/build/features/sleigh_runtime/sleigh_runtime_decode.exe`: console decoder.
- `NEW/build/features/sleigh_runtime/tests/sleigh_runtime_tests.exe`: runtime test executable.

The tests cover SLA loading, instruction matching, operand classification, p-code materialization, control flow, context isolation, malformed input, and instruction-size guards.

## Limitations

- A compiled `.sla` file is required; the runtime does not compile Sleigh source files.
- The processor context must match the fields expected by the selected `.sla` specification.
- The library API decodes one instruction per call and limits each call to 16 bytes. The CLI hides this limitation by repeatedly decoding 16-byte windows.
- The CLI prints p-code in a diagnostic textual form; it is not intended to be an assembler or a decompiler.
