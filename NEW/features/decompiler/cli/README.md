# Native Decompiler CLI

`decompiler_cli` is the standalone console frontend for [`NewGhidra::DecompilerFrontend`](../CMakeLists.txt). The built executable is named `new_ghidra_decompiler.exe` and accepts a compiled Sleigh `.sla`, machine-code bytes, a function range, and provider metadata. It has no dependency on GoogleTest or the decompiler test executable.

The CLI is intentionally built on the same public provider boundary as the library. It does not access Java, the original database, a running Ghidra process, or private Sleigh classes.

## Parameters

All parameters are listed below. Metadata options use comma-separated `key=value` records and can be repeated.

| Parameter | Value | Description |
|---|---|---|
| `--help`, `-h` | none | Print the complete usage reference. |
| `--sla` | path | Path to the compiled Sleigh decoder specification. Required in direct mode. |
| `--hex` | hex bytes | Machine-code bytes. Spaces, commas, and newlines are accepted. |
| `--hex-file` | path | Read hexadecimal machine-code bytes from a text file. |
| `--data` | `address:hex` | Map an additional data chunk. Repeatable. |
| `--address` | integer | Function entry address. Decimal or `0x` hexadecimal. |
| `--size` | integer | Function size in bytes; sets the exclusive end address. |
| `--end` | integer | Explicit exclusive function end address. |
| `--name` | string | Function name used by the native symbol/printer pipeline. |
| `--context` | `name=value` | Override one Sleigh processor-context field. Repeatable. |
| `--show` | section list | Select output sections. Use `all`, or comma-separated section names. |
| `--only` | section list | Alias for `--show`; useful for concise test checks. |
| `--arch-name` | string | Architecture label. |
| `--space` | assignments | Add an address space: `name`, `address-size`, `word-size`, `big-endian`, `index`, `physical`, `overlay`. |
| `--register` | assignments | Add a register with `name` and `storage=space:offset:size`. |
| `--code-space` | string | Set the architecture code space. |
| `--data-space` | string | Set the architecture data space. |
| `--stack-register` | string | Set the architecture stack-pointer register. |
| `--pointer-size` | integer | Set pointer width in bytes. |
| `--symbol` | assignments | Add a function/data symbol: `address`, `name`, `kind`, `size`, `type`, `namespace`, `readonly`, `space`, `identity`, `alias`. |
| `--type` | assignments | Declare a type: `name`, `kind`, `size`, `signed`, `element`, `count`, `declaration`. |
| `--type-field` | assignments | Add a structure/union field: `type`, `name`, `value`, `offset`. |
| `--bitfield` | assignments | Add a bitfield: `type`, `name`, `value`, `bits`, optional `group`. |
| `--enum-value` | assignments | Add an enumeration member: `type`, `name`, `value`. |
| `--type-declaration` | assignments | Replace a declared type's printer declaration using `name` and `value`. |
| `--prototype` | assignments | Set a function signature: `address`, `cc`, `return`, `return-storage`, `no-return`, `inline`. |
| `--param` | assignments | Add a parameter: `address`, `name`, `type`, `storage`. |
| `--variable` | assignments | Add a local: `address`, `name`, `type`, `storage`, `identity`, `isolated`. |
| `--function` | assignments | Add a bounded child function with `name`, `address`, and `end`. Repeatable. |
| `--jump-table` | assignments | Add a flow jump table: `function`, `branch`, pipe-separated `targets`, optional `start`. |
| `--volatile` | assignments | Mark a memory range volatile using `space`, `address`, and `size`. |
| `--readonly` | none | Enable propagation of immutable mapped memory values. |

Available output sections are `summary`, `assembly`, `raw-pcode`, `high-pcode`, `data-flow`, `control-flow`, `ast`, and `c`.

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

The CLI starts with a practical x86-64 architecture containing `ram` and `register` spaces and common registers. Override or extend it when using a different provider model:

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

Supported type kinds are `void`, `boolean`, `signed_integer`, `unsigned_integer`, `floating_point`, `char`, `pointer`, `array`, `structure`, `union`, `typedef`, and `enumeration`. Pointer and array shape is described with `element` and `count`.

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

## Manual Test Reproduction

The CLI does not load, link, or execute test code. To reproduce a test, copy its input bytes and metadata into ordinary CLI arguments. The following reproduces the important shape of the bitfield case using the same kind of explicit type and field records:

```powershell
& $cli --sla $sla --address 0x480000 --size 0x20 `
  --hex "488b c1 83e0 07 c3" `
  --name bitfield_extract `
  --type "name=uint32,kind=unsigned_integer,size=4,signed=false" `
  --type "name=PackedFlags,kind=structure,size=4,declaration=struct PackedFlags" `
  --bitfield "type=PackedFlags,name=low,value=uint32,bits=3,group=0" `
  --bitfield "type=PackedFlags,name=mode,value=uint32,bits=5,group=0" `
  --prototype "address=0x480000,cc=__cdecl,return=uint32,return-storage=register:0:4" `
  --only c
```

Representative output:

```text
Function : bitfield_extract

| C SOURCE |
uint32 bitfield_extract(PackedFlags *flags)
{
  return (uint32)flags->mode;
}
```

The real output is generated by the current native port. It must not be copied into this README as a golden string because compiler-printer improvements intentionally change formatting while preserving the semantic assertions.

### Executed `PortedForloop1` Input

This is the direct CLI equivalent of the `PortedForloop1` datatest. The command supplies the same entry address and bytes, then describes the function symbol, `int32` type, return storage, and the `max` parameter in `RCX`-equivalent register storage:

```powershell
& $cli --sla $sla --address 0x400517 `
  --hex "b8 00 00 00 00 83 f9 05 7d 05 83 c0 01 eb f6 c3" `
  --name forloop1 `
  --symbol "address=0x400517,name=forloop1,kind=function" `
  --type "name=int32,kind=signed_integer,size=4,signed=true" `
  --prototype "address=0x400517,cc=__cdecl,return=int32,return-storage=register:0:4" `
  --param "address=0x400517,name=max,type=int32,storage=register:8:4" `
  --only c,control-flow
```

The generated result was:

```c
int32 __cdecl forloop1(int32 max)

{
  int4 iVar1;

  iVar1 = 0;
  while (max < 5) {
    iVar1 = iVar1 + 1;
  }
  return iVar1;
}
```

The same invocation also produced this control-flow artifact:

```text
0
  List block 0
    Basic Block 0 0x00400517-0x00400517
    Whiledo block 1
      Basic Block 1 0x0040051c-0x0040051f
      Basic Block 3 0x00400521-0x00400524
    Basic Block 2 0x00400526-0x00400526
```

The control-flow artifact contained a `List` block with a nested `Whiledo` block. The example demonstrates that `--hex` is the executable input, while `--prototype`, `--param`, and `--symbol` provide source-level meaning.

### Executed `PortedSwitchIndirectMultiFunctionBodies` Input

This switch example requires three kinds of input: the root function and its `CALL` stubs in `--hex`, the read-only 64-bit jump-table entries in `--data` at `0x480200`, and the three bounded child bodies in additional `--data` chunks. The `--jump-table` record tells the native flow engine that the indirect branch at `0x480006` has three targets. The `--function` records prevent child decoding from running through neighboring image bytes.

```powershell
& $cli --sla $sla --address 0x480000 --size 0x25 `
  --hex "48 83 f9 02 77 19 ff 24 cd 00 02 48 00 e8 ee 02 00 00 c3 e8 f8 02 00 00 c3 e8 02 03 00 00 c3 b8 ff ff ff ff c3" `
  --data 0x480200:"0d 00 48 00 00 00 00 00 13 00 48 00 00 00 00 00 19 00 48 00 00 00 00 00" `
  --data 0x480300:"b8 10 00 00 00 c3" `
  --data 0x480310:"b8 11 00 00 00 c3" `
  --data 0x480320:"b8 12 00 00 00 c3" `
  --name switchind_root `
  --symbol "address=0x480000,name=switchind_root,kind=function" `
  --symbol "address=0x480300,name=switch_case_0,kind=function" `
  --symbol "address=0x480310,name=switch_case_1,kind=function" `
  --symbol "address=0x480320,name=switch_case_2,kind=function" `
  --type "name=int32,kind=signed_integer,size=4,signed=true" `
  --prototype "address=0x480000,cc=__cdecl,return=int32,return-storage=register:0:4" `
  --prototype "address=0x480300,cc=__cdecl,return=int32,return-storage=register:0:4" `
  --prototype "address=0x480310,cc=__cdecl,return=int32,return-storage=register:0:4" `
  --prototype "address=0x480320,cc=__cdecl,return=int32,return-storage=register:0:4" `
  --param "address=0x480000,name=selector,type=int32,storage=register:8:4" `
  --function "name=switch_case_0,address=0x480300,end=0x480306" `
  --function "name=switch_case_1,address=0x480310,end=0x480316" `
  --function "name=switch_case_2,address=0x480320,end=0x480326" `
  --jump-table "function=0x480000,branch=0x480006,targets=0x48000d|0x480013|0x480019,start=0" `
  --only c,control-flow
```

The CLI restored a structured switch:

```c
int32 __cdecl switchind_root(int32 selector)

{
  int32 iVar1;
  undefined4 in_RCX;

  if (2 < CONCAT44(in_RCX,selector)) {
    return 0xffffffff;
  }
                    /* WARNING: Switch is manually overridden */
  switch(CONCAT44(in_RCX,selector)) {
  case 0:
    iVar1 = switch_case_0();
    return iVar1;
  case 1:
    iVar1 = switch_case_1();
    return iVar1;
  case 2:
    iVar1 = switch_case_2();
    return iVar1;
  }
}
```

The corresponding control-flow artifact was:

```text
0
  If (no exit) block 0
    Basic Block 0 0x00480000-0x00480004
    Basic Block 1 0x0048001f-0x00480024
    Switch block 2
      Basic Block 2 0x00480006-0x00480006
      Basic Block 5 0x0048000d-0x00480012
      Basic Block 4 0x00480013-0x00480018
      Basic Block 3 0x00480019-0x0048001e
```

The `WARNING: Switch is manually overridden` comment is expected: the command explicitly supplies the provider-level jump-table model. The important result is that the indirect branch is represented as a structured `switch`, with named child calls and no synthetic `goto LAB_` output.

### Executed `SleighProvider` Input

`SleighProvider.DecodesX86BytesIntoProviderPcode` from `decompiler_tests.cppm` checks one instruction rather than a complete function. The CLI is a decompiler, so the command adds a `RET` after the tested `SUB RSP,0x40` instruction to form a bounded function. The first instruction remains exactly the test input:

```powershell
& $cli --sla $sla --address 0x100 `
  --hex "48 83 ec 40 c3" `
  --name provider_decode_adapted `
  --only assembly,raw-pcode
```

The disassembly result was:

```text
0x0000000000000100: SUB          RSP,0x40
0x0000000000000104: RET
```

The raw p-code included the `RSP = RSP - 0x40` operation, flag calculations, stack read-ahead handling, and the final `RETURN`. This is an adapted decoder-only test, not an execution of the GoogleTest case.

### Executed `Example1StringLengthWorkerWEndToEnd` Input

This command manually transfers the complete Example 1 x86-64 byte sequence and its source metadata. It declares the UTF-16 character type, integer types, pointer types, function prototype, and all three register parameters. The complete command is intentionally kept here so the result can be reproduced without linking any test target:

```powershell
& $cli --sla $sla --address 0x1000 `
  --hex "4c89442418488954241048894c24084883ec18c744240800000000488b4424284889042448837c242800742a488b4424200fb70085c0741e488b4424204883c0024889442420488b4424284883e8014889442428ebce48837c2428007508c74424085700078048837c2430007429837c2408007c16488b442428488b0c24482bc8488b442430488908eb0c488b44243048c700000000008b4424084883c418c3" `
  --name StringLengthWorkerW `
  --symbol "address=0x1000,name=StringLengthWorkerW,kind=function" `
  --type "name=wchar_t,kind=char,size=2,signed=false" `
  --type "name=__uint64,kind=unsigned_integer,size=8,signed=false" `
  --type "name=long,kind=signed_integer,size=4,signed=true" `
  --type "name=int,kind=signed_integer,size=4,signed=true" `
  --type "name=wchar_t *,kind=pointer,size=8,element=wchar_t" `
  --type "name=__uint64 *,kind=pointer,size=8,element=__uint64" `
  --prototype "address=0x1000,cc=__cdecl,return=long,return-storage=register:0:4" `
  --param "address=0x1000,name=param_1,type=wchar_t *,storage=register:8:8" `
  --param "address=0x1000,name=param_2,type=__uint64,storage=register:0x10:8" `
  --param "address=0x1000,name=param_3,type=__uint64 *,storage=register:0x80:8" `
  --only c
```

The generated C body was:

```c
long __cdecl StringLengthWorkerW(wchar_t * param_1,__uint64 param_2,__uint64 * param_3)

{
  wchar_t * pwStack0000000000000008;
  __uint64 _Stack0000000000000010;
  int4 iStack_10;

  iStack_10 = 0;
  _Stack0000000000000010 = param_2;
  for (pwStack0000000000000008 = param_1;
      (_Stack0000000000000010 != 0 && (*pwStack0000000000000008 != L'\0'));
      pwStack0000000000000008 = pwStack0000000000000008 + 1) {
    _Stack0000000000000010 = _Stack0000000000000010 - 1;
  }
  if (_Stack0000000000000010 == 0) {
    iStack_10 = -0x7ff8ffa9;
  }
  if (param_3 != (__uint64 *)0x0) {
    if (iStack_10 < 0) {
      *param_3 = 0;
    }
    else {
      *param_3 = param_2 - _Stack0000000000000010;
    }
  }
  return iStack_10;
}
```

The algorithm matches the test: bounded UTF-16 scan, error value `0x80070057`, optional consumed-count output, and return of the status value. Local variable names differ because this CLI invocation intentionally does not add the test's `VariableProvider` records.

## Exit Codes

- `0`: successful direct decompilation;
- `1`: malformed options, missing files, provider validation failure, or decoder failure;
- decoder failures are reported with the native provider diagnostic and use the same non-zero CLI failure path.

## Source Mapping

- [`decompiler_cli.cppm`](decompiler_cli.cppm) parses arguments, constructs providers, runs the native pipeline, and renders all result artifacts.
- [`../src/decompiler.cppm`](../src/decompiler.cppm) defines the public provider and result contracts.
- [`../CMakeLists.txt`](../CMakeLists.txt) builds the CLI alongside the frontend and test targets.
