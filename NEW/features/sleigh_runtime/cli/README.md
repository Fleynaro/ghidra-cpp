# Sleigh Runtime CLI

`sleigh_runtime_decode.cpp` builds `sleigh_runtime_decode.exe`, a diagnostic console frontend for the static [`sleigh_runtime`](../CMakeLists.txt) library. It accepts raw machine-code bytes and a compiled Ghidra `.sla` processor specification, then prints assembly and materialized p-code.

## Basic Usage

Run from the `NEW` directory:

```powershell
.\build\features\sleigh_runtime\sleigh_runtime_decode.exe `
    --sla .\features\sleigh_runtime\test_data\x86-64.sla `
    --hex "48 8b d9"
```

The default x86-64 context is `addrsize=2`, `opsize=1`, `rexprefix=0`, and `longMode=1`. Context fields can be overridden with repeated `--context name=value` options.

## Multiple Instructions

The `--hex` value may contain a complete byte sequence rather than one instruction:

```powershell
.\build\features\sleigh_runtime\sleigh_runtime_decode.exe `
    --sla .\features\sleigh_runtime\test_data\x86-64.sla `
    --address 0x140000000 `
    --hex "48 8b d9 48 83 ec 40"
```

The CLI decodes one instruction, advances by its decoded length, updates the address, and repeats until the input is exhausted. Each iteration prints only the bytes consumed by that instruction. Internally, it passes no more than 16 bytes to the library because the Sleigh runtime instruction window is limited to 16 bytes.

## Input Rules

- `--sla` is required and must point to a binary compiled `.sla` file.
- `--hex` is required and must contain complete two-digit hexadecimal byte pairs.
- Spaces, tabs, newlines, and commas are accepted between byte pairs.
- `--address` accepts decimal values and `0x`-prefixed hexadecimal values.
- `--context` accepts `name=value`, where the value is decimal or `0x`-prefixed hexadecimal.

## Output

Each instruction block contains its sequence number, address, consumed bytes, length, mnemonic with operands, control-flow classification, and numbered p-code operations. Varnodes are printed as `space[offset:size]`, for example `register[0x18:8]`.

## Exit Codes

- `0`: all input bytes decoded successfully.
- `1`: invalid arguments or a failure while loading the SLA file.
- `2`: an instruction could not be decoded or returned an invalid length.
