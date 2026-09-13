# Native Decompiler CLI

`decompiler_cli` is the console frontend for [`NewGhidra::DecompilerFrontend`](../CMakeLists.txt). The built executable is named `new_ghidra_decompiler.exe` and exposes two input paths:

- direct mode accepts a compiled Sleigh `.sla`, machine-code bytes, a function range, and provider metadata;
- datatest mode executes a registered `DecompilerDatatests` case and prints the exact `DecompilationResult` captured by the test fixture.

The CLI is intentionally built on the same public provider boundary as the library. It does not access Java, the original database, a running Ghidra process, or private Sleigh classes.

## Build

Run the repository build entry point from `NEW`:

```powershell
cmd /c .\build.bat
```

The executable is generated at:

```text
NEW/build/features/decompiler/new_ghidra_decompiler.exe
```

The logical CMake target is `decompiler_cli`; the output name is set to `new_ghidra_decompiler` so the binary name describes its purpose.

## Output Model

The default output is a readable report containing all artifacts available from [`DecompilationResult`](../src/decompiler.cppm):

1. Summary and decoded instructions.
2. Assembly reconstructed by the selected provider.
3. Raw p-code emitted before native simplification.
4. High p-code / SSA after heritage and action passes.
5. Data-flow representation.
6. Control-flow block tree.
7. Native AST/local-range diagnostic when available.
8. Generated C source.

Every artifact has a boxed heading. Empty artifacts are printed as `(empty)` instead of silently disappearing, which makes incomplete provider input immediately visible.

Use `--show` or `--only` to select a subset. Names are comma-separated:

```text
summary, assembly, raw-pcode, high-pcode, data-flow, control-flow, ast, c
```

For example, `--only c` prints only the generated C source, while `--only high-pcode` prints only the transformed high-level p-code.

## Direct Mode

The smallest complete invocation uses a `.sla`, bytes, and an entry point. The default entry point is `0x140000000`; the default function end is the first byte after `--hex`.

```powershell
$cli = ".\build\features\decompiler\new_ghidra_decompiler.exe"
$sla = ".\features\sleigh_runtime\test_data\x86-64.sla"

& $cli --sla $sla --address 0x140000000 `
    --hex "b8 2a 00 00 00 c3" `
    --name answer
```

Representative output has this shape; instruction addresses and p-code details depend on the selected SLA:

```text
+======================================================================+
| DECOMPILATION SUMMARY                                               |
+----------------------------------------------------------------------+
Function : answer
Result   : artifact set 1/1
Decoded  : 2 instruction(s)
  0x0000000140000000  + 5  MOV EAX,0x2a
  0x0000000140000005  + 1  RET

+======================================================================+
| RAW P-CODE                                                          |
+----------------------------------------------------------------------+
...

+======================================================================+
| HIGH P-CODE / SSA                                                   |
+----------------------------------------------------------------------+
...

+======================================================================+
| C SOURCE                                                             |
+----------------------------------------------------------------------+
int answer(void)
{
  return 0x2a;
}
```

The exact C formatting is produced by the native C printer, not by the CLI.

### Function Range

`--size N` sets the exclusive end to `address + N`. `--end ADDRESS` sets the exclusive end explicitly. These options are important when the byte image contains data, a jump table, or adjacent child functions:

```powershell
& $cli --sla $sla --address 0x401000 --size 0x24 `
    --hex "48 83 ec 18 ..." --name switch_return
```

The CLI rejects zero-length and overflowing ranges before constructing the native decompiler.

### Context

Sleigh context values are repeatable and may be decimal or hexadecimal:

```powershell
& $cli --sla $sla --hex "48 8b d9" `
    --context addrsize=2 --context opsize=1 `
    --context rexprefix=0 --context longMode=1
```

The default context matches the checked-in x86-64 fixture. Other `.sla` files must receive the field names and values required by their processor specification.

### Mapped Data

`--data ADDRESS:HEX` adds an additional mapped chunk to the sparse image. It can be repeated and is useful for RIP-relative globals, strings, and jump tables:

```powershell
& $cli --sla $sla --address 0x401000 --size 0x12 `
    --hex "488b0509000000c3" `
    --data 0x401010:"2a 00 00 00" `
    --symbol "address=0x401010,name=global_value,kind=data,size=4,type=int32,readonly=true" `
    --type "name=int32,kind=signed_integer,size=4,signed=true" `
    --readonly --only c
```

PowerShell quoting is recommended for metadata records so commas remain in one argument.

## Metadata Grammar

Metadata records use comma-separated `key=value` fields. Repeat the option for multiple records. The grammar maps directly to the public provider structs in [`decompiler.cppm`](../src/decompiler.cppm).

### Architecture

The CLI starts with a practical x86-64 architecture containing `ram`, `register`, and `stack` spaces and common registers. Override or extend it when using a different provider model:

```powershell
& $cli --sla $sla --hex "c3" --address 0x5000 `
    --space "name=ram,address-size=8,word-size=1,big-endian=false,index=2,physical=true" `
    --space "name=register,address-size=8,word-size=1,big-endian=false,index=3,physical=true" `
    --register "name=RSP,storage=register:0x20:8" `
    --code-space ram --data-space ram --stack-register RSP --pointer-size 8
```

`--space` supports `name`, `address-size`, `word-size`, `big-endian`, `index`, `physical`, and `overlay`. `--register` supports `name` and `storage=space:offset:size`.

### Symbols

Symbols may represent functions or data and may carry names, namespaces, types, identities, aliases, and read-only state:

```powershell
--symbol "address=0x401000,name=demo,kind=function,namespace=sample"
--symbol "address=0x402000,name=flags,kind=data,size=4,type=PackedFlags,readonly=true,space=ram"
```

### Types, Structures, Bitfields, and Enums

Declare a type first, then attach fields. This ordering makes malformed references fail early:

```powershell
--type "name=uint32,kind=unsigned_integer,size=4,signed=false"
--type "name=PackedFlags,kind=structure,size=4,declaration=struct PackedFlags"
--bitfield "type=PackedFlags,name=low,value=uint32,bits=3,group=0"
--bitfield "type=PackedFlags,name=mode,value=uint32,bits=5,group=0"
--type "name=Status,kind=enumeration,size=4,signed=true"
--enum-value "type=Status,name=Ready,value=1"
```

Supported type kinds are `void`, `boolean`, `signed_integer`, `unsigned_integer`, `floating_point`, `unicode_character`, `pointer`, `array`, `structure`, `union`, `typedef`, and `enumeration`. Pointer and array shape is described with `element` and `count`.

### Prototypes and Locals

Prototype storage is explicit because it is part of decompiler behavior, not merely presentation:

```powershell
--prototype "address=0x401000,cc=__cdecl,return=int32,return-storage=register:0:4"
--param "address=0x401000,name=flags,type=PackedFlags *,storage=register:8:8"
--variable "address=0x401000,name=local,type=int32,storage=stack:0x10:4,isolated=true"
```

`--prototype` also supports `no-return=true` and `inline=true`. Split ABI values are represented in the library contract by ordered pieces; the CLI currently exposes the common single-storage form directly and reports unsupported future fields instead of silently guessing.

### Volatile Memory

Mark a range as volatile to retain unused loads with side effects:

```powershell
--volatile "space=ram,address=0x2000,size=4"
```

This is distinct from `--readonly`, which enables propagation of immutable mapped values into constants.

## Datatest Mode

The `--test` path is the important reproducibility feature. It executes the actual registered C++ test, not a hand-written approximation. The test's `expect_complete_analysis` helper captures each `DecompilationResult`; the CLI then renders those same objects using the selected sections.

List available cases:

```powershell
& $cli --list-tests
```

Run the bitfield test and print the C output only:

```powershell
& $cli --test ProviderBitfieldExtraction --only c
```

Representative semantic output:

```text
Function : ProviderBitfieldExtraction

| C SOURCE |
uint32 bitfield_extract(PackedFlags *flags)
{
  return (uint32)flags->mode;
}
```

The real output is generated by the current native port. It must not be copied into this README as a golden string because compiler-printer improvements intentionally change formatting while preserving the semantic assertions.

Run switch coverage with all diagnostics:

```powershell
& $cli --test PortedSwitchIndirectMultiFunctionBodies
& $cli --test PortedSwitchMaskAndMultiCaseSemantics --only high-pcode,control-flow,c
```

The switch result includes the same recovered cases and named child targets asserted by the test, for example `case 0`, `case 1`, `switch_case_0`, and no synthetic `goto LAB_` in the generated C where the fixture requires structured output.

Names may also include the suite prefix:

```powershell
& $cli --test DecompilerDatatests.ProviderBitfieldExtraction --only high-pcode
```

Cases with multiple decompilations produce `artifact set N/M` headings. This is used by fixtures such as `PortedModulo2` and `PortedSwitchMaskAndMultiCaseSemantics`.

This bridge covers the executable cases in `NEW/features/decompiler/tests/decompiler_datatests.cppm`. The original 89 XML manifest entries are not all portable in the current provider boundary; the manifest and its reasons remain authoritative in that test source. The CLI does not claim to run unsupported processor-specific XML cases automatically.

## Exit Codes

- `0`: successful direct decompilation, test execution, or test listing;
- `1`: malformed options, missing files, provider validation failure, unknown test, or failed test assertions;
- decoder failures are reported with the native provider diagnostic and use the same non-zero CLI failure path.

## Source Mapping

- [`decompiler_cli.cppm`](decompiler_cli.cppm) parses arguments, constructs providers, runs the native pipeline, and renders all result artifacts.
- [`../src/decompiler.cppm`](../src/decompiler.cppm) defines the public provider and result contracts.
- [`../tests/decompiler_datatests.cppm`](../tests/decompiler_datatests.cppm) remains the fixture authority and exports the narrow capture/list bridge used only by this executable.
- [`../CMakeLists.txt`](../CMakeLists.txt) builds the CLI alongside the frontend and test targets.
