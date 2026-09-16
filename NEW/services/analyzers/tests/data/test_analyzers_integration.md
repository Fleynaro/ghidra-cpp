# All Analyzer Integration Behavioral Oracle

> Generated automatically with PyGhidra for development comparison only.
> The native Google Test does not read or parse this Markdown file.

## Input And Limits

- File: `test_analyzers_integration.exe`
- Size: `10240` bytes
- Functions before analysis: `22`; after: `64`
- Maximum total characters: `240000`
- Maximum functions: `120`
- Maximum instructions/function: `24`
- Maximum references/function: `20`

## Analysis Configuration

### Enabled options
- `ASCII Strings`
- `Aggressive Instruction Finder`
- `Apply Data Archives`
- `Call Convention ID`
- `Call-Fixup Installer`
- `Condense Filler Bytes`
- `Create Address Tables`
- `Data Reference`
- `Decompiler Parameter ID`
- `Decompiler Switch Analysis`
- `Demangler Microsoft`
- `Disassemble Entry Points`
- `Embedded Media`
- `External Entry References`
- `Function ID`
- `Function Start Search`
- `Non-Returning Functions - Discovered`
- `Non-Returning Functions - Known`
- `PDB MSDIA`
- `PDB Universal`
- `Reference`
- `Scalar Operand References`
- `Shared Return Calls`
- `Stack`
- `Subroutine References`
- `Variadic Function Signature Override`
- `WindowsPE x86 Propagate External Parameters`
- `WindowsResourceReference`
- `x86 Constant Reference Analyzer`

### Requested but unavailable in this Ghidra installation
- `Constant Propagation`
- `Function Start Pre Search`
- `Function Start Search After Code`
- `Function Start Search After Data`
- `Function Start Search In Functions`

## Functions

Observed local functions: `64`; maximum rendered: `120`.
### 1. `filler_engine_a` at `0x0000000140001000`
- Body: `0x0000000140001000-0x000000014000100F`
- Comment: ``
- Signature: `void __cdecl filler_engine_a(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001000` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001006` `ADD EAX,0x11` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x11, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x11, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x11, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001009` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000100F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001000` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001009` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x000000014000010C` -> `0x0000000140001000` type=`DATA`
  - `0x00000001400001F4` -> `0x0000000140001000` type=`DATA`
  - `0x00000001400027B4` -> `0x0000000140001000` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001000` type=`EXTERNAL`
  - `0x000000014000175A` -> `0x0000000140001000` type=`UNCONDITIONAL_CALL`
### 2. `filler_engine_b` at `0x0000000140001020`
- Body: `0x0000000140001020-0x000000014000102F`
- Comment: ``
- Signature: `void __cdecl filler_engine_b(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001020` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001026` `ADD EAX,0x22` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x22, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x22, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x22, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001029` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000102F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001020` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001029` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x00000001400027B8` -> `0x0000000140001020` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001020` type=`EXTERNAL`
  - `0x000000014000175F` -> `0x0000000140001020` type=`UNCONDITIONAL_CALL`
### 3. `filler_engine_c` at `0x0000000140001040`
- Body: `0x0000000140001040-0x000000014000104F`
- Comment: ``
- Signature: `void __cdecl filler_engine_c(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001040` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001046` `ADD EAX,0x33` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x33, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x33, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x33, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001049` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000104F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001040` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001049` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x00000001400027BC` -> `0x0000000140001040` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001040` type=`EXTERNAL`
  - `0x0000000140001764` -> `0x0000000140001040` type=`UNCONDITIONAL_CALL`
### 4. `filler_engine_d` at `0x0000000140001060`
- Body: `0x0000000140001060-0x000000014000106F`
- Comment: ``
- Signature: `void __cdecl filler_engine_d(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001060` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001066` `ADD EAX,0x44` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x44, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x44, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x44, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001069` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000106F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001060` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001069` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x00000001400027C0` -> `0x0000000140001060` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001060` type=`EXTERNAL`
  - `0x0000000140001769` -> `0x0000000140001060` type=`UNCONDITIONAL_CALL`
### 5. `add` at `0x0000000140001070`
- Body: `0x0000000140001070-0x0000000140001080`
- Comment: ``
- Signature: `int __thiscall add(Calculator * this, int param_1, int param_2); return=int; convention=__thiscall; parameters=['this:Calculator *', 'param_1:int', 'param_2:int']`
- Stack frame: `frame=40, local=40, parameters=3,  variables=['this:Calculator *:RCX:8 (auto)', 'param_1:int:EDX:4', 'param_2:int:R8D:4']`
- Instructions:
  - `0x0000000140001070` `LEA EAX,[RDX + R8*0x1]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9300, 8) INT_MULT (register, 0x80, 8) , (const, 0x1, 8)', '(unique, 0x9500, 8) INT_ADD (register, 0x10, 8) , (unique, 0x9300, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x9500, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001074` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000107A` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001080` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001074` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x000000014000107A` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
- Callers:
  - `0x0000000140002738` -> `0x0000000140001070` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001070` type=`EXTERNAL`
  - `0x0000000140001620` -> `0x0000000140001070` type=`UNCONDITIONAL_CALL`
### 6. `scale` at `0x0000000140001090`
- Body: `0x0000000140001090-0x000000014000109F`
- Comment: ``
- Signature: `int __cdecl scale(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x0000000140001090` `LEA EAX,[RCX + RCX*0x2]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9300, 8) INT_MULT (register, 0x8, 8) , (const, 0x2, 8)', '(unique, 0x9500, 8) INT_ADD (register, 0x8, 8) , (unique, 0x9300, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x9500, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001093` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x0000000140001099` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x000000014000109F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001093` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x0000000140001099` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
- Callers:
  - `0x0000000140002744` -> `0x0000000140001090` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001090` type=`EXTERNAL`
  - `0x000000014000162D` -> `0x0000000140001090` type=`UNCONDITIONAL_CALL`
### 7. `combine` at `0x00000001400010B0`
- Body: `0x00000001400010B0-0x00000001400010B5`
- Comment: ``
- Signature: `int __cdecl combine(int param_1, int param_2); return=int; convention=__cdecl; parameters=['param_1:int', 'param_2:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:int:ECX:4', 'param_2:int:EDX:4']`
- Instructions:
  - `0x00000001400010B0` `LEA EAX,[RDX + 0x7]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x10, 8) , (const, 0x7, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x8f00, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400010B3` `ADD EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400010B5` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x000000014000273C` -> `0x00000001400010B0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400010B0` type=`EXTERNAL`
  - `0x0000000140001693` -> `0x00000001400010B0` type=`UNCONDITIONAL_CALL`
### 8. `combine` at `0x00000001400010C0`
- Body: `0x00000001400010C0-0x00000001400010D0`
- Comment: ``
- Signature: `float __cdecl combine(float param_1, float param_2); return=float; convention=__cdecl; parameters=['param_1:float', 'param_2:float']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:float:XMM0_Da:4', 'param_2:float:XMM1_Da:4']`
- Instructions:
  - `0x00000001400010C0` `MULSS XMM1,dword ptr [0x140002130]` flow=`FALL_THROUGH` pcode=`['(register, 0x1240, 4) FLOAT_MULT (register, 0x1240, 4) , (ram, 0x140002130, 4)']`
  - `0x00000001400010C8` `ADDSS XMM0,XMM0` flow=`FALL_THROUGH` pcode=`['(register, 0x1200, 4) FLOAT_ADD (register, 0x1200, 4) , (register, 0x1200, 4)']`
  - `0x00000001400010CC` `ADDSS XMM0,XMM1` flow=`FALL_THROUGH` pcode=`['(register, 0x1200, 4) FLOAT_ADD (register, 0x1200, 4) , (register, 0x1240, 4)']`
  - `0x00000001400010D0` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x00000001400010C0` -> `0x0000000140002130` type=`READ` source=`DEFAULT`
- Callers:
  - `0x0000000140002740` -> `0x00000001400010C0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400010C0` type=`EXTERNAL`
### 9. `callback_add` at `0x00000001400010E0`
- Body: `0x00000001400010E0-0x00000001400010E3`
- Comment: ``
- Signature: `uint __cdecl callback_add(uint param_1); return=uint; convention=__cdecl; parameters=['param_1:uint']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:uint:ECX:4']`
- Instructions:
  - `0x00000001400010E0` `LEA EAX,[RCX + 0x11]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x11, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x8f00, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400010E3` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x0000000140002754` -> `0x00000001400010E0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400010E0` type=`EXTERNAL`
  - `0x00000001400021C8` -> `0x00000001400010E0` type=`DATA`
  - `0x00000001400012A2` -> `0x00000001400010E0` type=`COMPUTED_JUMP`
  - `0x00000001400021B0` -> `0x00000001400010E0` type=`DATA`
### 10. `callback_rotate` at `0x00000001400010F0`
- Body: `0x00000001400010F0-0x00000001400010F5`
- Comment: ``
- Signature: `uint __cdecl callback_rotate(uint param_1); return=uint; convention=__cdecl; parameters=['param_1:uint']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:uint:ECX:4']`
- Instructions:
  - `0x00000001400010F0` `ROL ECX,0x3` flow=`FALL_THROUGH` pcode=`['(unique, 0xb9200, 4) INT_AND (const, 0x3, 4) , (const, 0x1f, 4)', '(unique, 0xb9300, 4) INT_LEFT (register, 0x8, 4) , (unique, 0xb9200, 4)', '(unique, 0xb9400, 4) INT_SUB (const, 0x20, 4) , (unique, 0xb9200, 4)', '(unique, 0xb9500, 4) INT_RIGHT (register, 0x8, 4) , (unique, 0xb9400, 4)', '(register, 0x8, 4) INT_OR (unique, 0xb9300, 4) , (unique, 0xb9500, 4)', '(unique, 0x5a500, 1) INT_EQUAL (unique, 0xb9200, 4) , (const, 0x0, 4)', '(unique, 0x5a600, 4) INT_AND (register, 0x8, 4) , (const, 0x1, 4)', '(unique, 0x5a700, 1) INT_NOTEQUAL (unique, 0x5a600, 4) , (const, 0x0, 4)', '(unique, 0x0, 1) COPY (unique, 0x5a500, 1)', '(unique, 0x100, 1) INT_MULT (unique, 0x0, 1) , (register, 0x200, 1)', '(unique, 0x200, 1) BOOL_NEGATE (unique, 0x5a500, 1)', '(unique, 0x300, 1) COPY (unique, 0x200, 1)', '(unique, 0x400, 1) INT_MULT (unique, 0x300, 1) , (unique, 0x5a700, 1)', '(register, 0x200, 1) INT_OR (unique, 0x100, 1) , (unique, 0x400, 1)', '(unique, 0x5a800, 1) INT_EQUAL (unique, 0xb9200, 4) , (const, 0x1, 4)', '(unique, 0x5a900, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x5aa00, 1) INT_XOR (register, 0x200, 1) , (unique, 0x5a900, 1)', '(unique, 0x0, 1) COPY (unique, 0x5a800, 1)', '(unique, 0x100, 1) INT_MULT (unique, 0x0, 1) , (unique, 0x5aa00, 1)', '(unique, 0x200, 1) BOOL_NEGATE (unique, 0x5a800, 1)', '(unique, 0x300, 1) COPY (unique, 0x200, 1)', '(unique, 0x400, 1) INT_MULT (unique, 0x300, 1) , (register, 0x20b, 1)', '(register, 0x20b, 1) INT_OR (unique, 0x100, 1) , (unique, 0x400, 1)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x00000001400010F3` `MOV EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400010F5` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x000000014000275C` -> `0x00000001400010F0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400010F0` type=`EXTERNAL`
  - `0x00000001400021B8` -> `0x00000001400010F0` type=`DATA`
  - `0x00000001400012A2` -> `0x00000001400010F0` type=`COMPUTED_JUMP`
### 11. `callback_mask` at `0x0000000140001100`
- Body: `0x0000000140001100-0x000000014000110E`
- Comment: ``
- Signature: `uint __cdecl callback_mask(uint param_1); return=uint; convention=__cdecl; parameters=['param_1:uint']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:uint:ECX:4']`
- Instructions:
  - `0x0000000140001100` `XOR ECX,0xffa5a5a5` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x8, 4) INT_XOR (register, 0x8, 4) , (const, 0xffa5a5a5, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001106` `AND ECX,0xffffff` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x8, 4) INT_AND (register, 0x8, 4) , (const, 0xffffff, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000110C` `MOV EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x000000014000110E` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x0000000140002758` -> `0x0000000140001100` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001100` type=`EXTERNAL`
  - `0x00000001400021C0` -> `0x0000000140001100` type=`DATA`
  - `0x00000001400012A2` -> `0x0000000140001100` type=`COMPUTED_JUMP`
### 12. `overloaded_sum` at `0x0000000140001120`
- Body: `0x0000000140001120-0x0000000140001125`
- Comment: ``
- Signature: `int __cdecl overloaded_sum(int param_1, int param_2); return=int; convention=__cdecl; parameters=['param_1:int', 'param_2:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:int:ECX:4', 'param_2:int:EDX:4']`
- Instructions:
  - `0x0000000140001120` `LEA EAX,[RDX + 0x21]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x10, 8) , (const, 0x21, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x8f00, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001123` `ADD EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001125` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x00000001400027D4` -> `0x0000000140001120` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001120` type=`EXTERNAL`
  - `0x0000000140001685` -> `0x0000000140001120` type=`UNCONDITIONAL_CALL`
### 13. `overloaded_sum_float` at `0x0000000140001130`
- Body: `0x0000000140001130-0x0000000140001144`
- Comment: ``
- Signature: `float __cdecl overloaded_sum_float(float param_1, float param_2); return=float; convention=__cdecl; parameters=['param_1:float', 'param_2:float']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:float:XMM0_Da:4', 'param_2:float:XMM1_Da:4']`
- Instructions:
  - `0x0000000140001130` `MULSS XMM0,dword ptr [0x140002138]` flow=`FALL_THROUGH` pcode=`['(register, 0x1200, 4) FLOAT_MULT (register, 0x1200, 4) , (ram, 0x140002138, 4)']`
  - `0x0000000140001138` `MULSS XMM1,dword ptr [0x140002134]` flow=`FALL_THROUGH` pcode=`['(register, 0x1240, 4) FLOAT_MULT (register, 0x1240, 4) , (ram, 0x140002134, 4)']`
  - `0x0000000140001140` `ADDSS XMM0,XMM1` flow=`FALL_THROUGH` pcode=`['(register, 0x1200, 4) FLOAT_ADD (register, 0x1200, 4) , (register, 0x1240, 4)']`
  - `0x0000000140001144` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001130` -> `0x0000000140002138` type=`READ` source=`DEFAULT`
  - `0x0000000140001138` -> `0x0000000140002134` type=`READ` source=`DEFAULT`
- Callers:
  - `0x00000001400027D8` -> `0x0000000140001130` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001130` type=`EXTERNAL`
  - `0x0000000140001678` -> `0x0000000140001130` type=`UNCONDITIONAL_CALL`
### 14. `recursive_score` at `0x0000000140001150`
- Body: `0x0000000140001150-0x0000000140001176`
- Comment: ``
- Signature: `int __cdecl recursive_score(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x0000000140001150` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001152` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001156` `MOV EBX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 4) COPY (register, 0x8, 4)', '(register, 0x18, 8) INT_ZEXT (register, 0x18, 4)']`
  - `0x0000000140001158` `CMP ECX,0x1` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x8, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0x1, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0x1, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0x1, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000115B` `JG 0x140001168` flow=`CONDITIONAL_JUMP` pcode=`['(unique, 0x26000, 1) BOOL_NEGATE (register, 0x206, 1)', '(unique, 0x26100, 1) INT_EQUAL (register, 0x20b, 1) , (register, 0x207, 1)', '(unique, 0x26300, 1) BOOL_AND (unique, 0x26000, 1) , (unique, 0x26100, 1)', ' ---  CBRANCH (ram, 0x140001168, 8) , (unique, 0x26300, 1)']`
  - `0x000000014000115D` `MOV EAX,0x1` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1, 8)']`
  - `0x0000000140001162` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001166` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001167` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001168` `DEC ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x20b, 1) INT_SBORROW (register, 0x8, 4) , (const, 0x1, 4)', '(register, 0x8, 4) INT_SUB (register, 0x8, 4) , (const, 0x1, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000116A` `CALL 0x140001150` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x14000116f, 8)', ' ---  CALL (ram, 0x140001150, 8)']`
  - `0x000000014000116F` `ADD EAX,EBX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x18, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x18, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x18, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001171` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001175` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001176` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x000000014000115B` -> `0x0000000140001168` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x000000014000116A` -> `0x0000000140001150` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400027DC` -> `0x0000000140001150` type=`DATA`
  - `0x0000000140004000` -> `0x0000000140001150` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001150` type=`EXTERNAL`
  - `0x0000000140001652` -> `0x0000000140001150` type=`UNCONDITIONAL_CALL`
  - `0x000000014000116A` -> `0x0000000140001150` type=`UNCONDITIONAL_CALL`
### 15. `mutual_alpha` at `0x0000000140001180`
- Body: `0x0000000140001180-0x000000014000119F`
- Comment: ``
- Signature: `int __cdecl mutual_alpha(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x0000000140001180` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001184` `TEST ECX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(unique, 0xd3300, 4) INT_AND (register, 0x8, 4) , (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0xd3300, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd3300, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0xd3300, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001186` `JG 0x140001192` flow=`CONDITIONAL_JUMP` pcode=`['(unique, 0x26000, 1) BOOL_NEGATE (register, 0x206, 1)', '(unique, 0x26100, 1) INT_EQUAL (register, 0x20b, 1) , (register, 0x207, 1)', '(unique, 0x26300, 1) BOOL_AND (unique, 0x26000, 1) , (unique, 0x26100, 1)', ' ---  CBRANCH (ram, 0x140001192, 8) , (unique, 0x26300, 1)']`
  - `0x0000000140001188` `MOV EAX,0x2` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x2, 8)']`
  - `0x000000014000118D` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001191` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001192` `DEC ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x20b, 1) INT_SBORROW (register, 0x8, 4) , (const, 0x1, 4)', '(register, 0x8, 4) INT_SUB (register, 0x8, 4) , (const, 0x1, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001194` `CALL 0x1400011b0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001199, 8)', ' ---  CALL (ram, 0x1400011b0, 8)']`
  - `0x0000000140001199` `INC EAX` flow=`FALL_THROUGH` pcode=`['(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x1, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x1, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000119B` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000119F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001186` -> `0x0000000140001192` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x0000000140001194` -> `0x00000001400011B0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400027CC` -> `0x0000000140001180` type=`DATA`
  - `0x000000014000400C` -> `0x0000000140001180` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001180` type=`EXTERNAL`
  - `0x000000014000165B` -> `0x0000000140001180` type=`UNCONDITIONAL_CALL`
  - `0x00000001400011C4` -> `0x0000000140001180` type=`UNCONDITIONAL_CALL`
### 16. `mutual_beta` at `0x00000001400011B0`
- Body: `0x00000001400011B0-0x00000001400011D0`
- Comment: ``
- Signature: `int __cdecl mutual_beta(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x00000001400011B0` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011B4` `TEST ECX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(unique, 0xd3300, 4) INT_AND (register, 0x8, 4) , (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0xd3300, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd3300, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0xd3300, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011B6` `JG 0x1400011c2` flow=`CONDITIONAL_JUMP` pcode=`['(unique, 0x26000, 1) BOOL_NEGATE (register, 0x206, 1)', '(unique, 0x26100, 1) INT_EQUAL (register, 0x20b, 1) , (register, 0x207, 1)', '(unique, 0x26300, 1) BOOL_AND (unique, 0x26000, 1) , (unique, 0x26100, 1)', ' ---  CBRANCH (ram, 0x1400011c2, 8) , (unique, 0x26300, 1)']`
  - `0x00000001400011B8` `MOV EAX,0x3` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x3, 8)']`
  - `0x00000001400011BD` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011C1` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x00000001400011C2` `DEC ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x20b, 1) INT_SBORROW (register, 0x8, 4) , (const, 0x1, 4)', '(register, 0x8, 4) INT_SUB (register, 0x8, 4) , (const, 0x1, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011C4` `CALL 0x140001180` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400011c9, 8)', ' ---  CALL (ram, 0x140001180, 8)']`
  - `0x00000001400011C9` `ADD EAX,0x2` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x2, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x2, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x2, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011CC` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011D0` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x00000001400011B6` -> `0x00000001400011C2` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x00000001400011C4` -> `0x0000000140001180` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400027D0` -> `0x00000001400011B0` type=`DATA`
  - `0x0000000140004018` -> `0x00000001400011B0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400011B0` type=`EXTERNAL`
  - `0x0000000140001194` -> `0x00000001400011B0` type=`UNCONDITIONAL_CALL`
### 17. `switch_mode` at `0x00000001400011E0`
- Body: `0x00000001400011E0-0x0000000140001230`
- Comment: ``
- Signature: `int __cdecl switch_mode(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x00000001400011E0` `CMP ECX,0x7` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x8, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0x7, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0x7, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0x7, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011E3` `JA 0x14000122b` flow=`CONDITIONAL_JUMP` pcode=`['(unique, 0x25200, 1) BOOL_OR (register, 0x200, 1) , (register, 0x206, 1)', '(unique, 0x25400, 1) BOOL_NEGATE (unique, 0x25200, 1)', ' ---  CBRANCH (ram, 0x14000122b, 8) , (unique, 0x25400, 1)']`
  - `0x00000001400011E5` `MOVSXD RAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) INT_SEXT (register, 0x8, 4)']`
  - `0x00000001400011E8` `LEA RDX,[0x140000000]` flow=`FALL_THROUGH` pcode=`['(register, 0x10, 8) COPY (const, 0x140000000, 8)']`
  - `0x00000001400011EF` `MOV ECX,dword ptr [RDX + RAX*0x4 + 0x1234]` flow=`FALL_THROUGH` pcode=`['(unique, 0xa700, 8) INT_ADD (const, 0x1234, 8) , (register, 0x10, 8)', '(unique, 0xa800, 8) INT_MULT (register, 0x0, 8) , (const, 0x4, 8)', '(unique, 0xaa00, 8) INT_ADD (unique, 0xa700, 8) , (unique, 0xa800, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0xaa00, 8)', '(register, 0x8, 4) COPY (unique, 0x23d00, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x00000001400011F6` `ADD RCX,RDX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x8, 8) , (register, 0x10, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x8, 8) , (register, 0x10, 8)', '(register, 0x8, 8) INT_ADD (register, 0x8, 8) , (register, 0x10, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x8, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400011F9` `JMP RCX` flow=`COMPUTED_JUMP` pcode=`[' ---  BRANCHIND (register, 0x8, 8)']`
  - `0x00000001400011FB` `MOV EAX,0xa` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0xa, 8)']`
  - `0x0000000140001200` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001201` `MOV EAX,0x14` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x14, 8)']`
  - `0x0000000140001206` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001207` `MOV EAX,0x1e` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1e, 8)']`
  - `0x000000014000120C` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x000000014000120D` `MOV EAX,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x28, 8)']`
  - `0x0000000140001212` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001213` `MOV EAX,0x32` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x32, 8)']`
  - `0x0000000140001218` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001219` `MOV EAX,0x3c` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x3c, 8)']`
  - `0x000000014000121E` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x000000014000121F` `MOV EAX,0x46` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x46, 8)']`
  - `0x0000000140001224` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001225` `MOV EAX,0x50` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x50, 8)']`
  - `0x000000014000122A` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x000000014000122B` `MOV EAX,0xffffffff` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0xffffffff, 8)']`
  - [TRUNCATED: maximum `24` instructions]
- References:
  - `0x00000001400011E3` -> `0x000000014000122B` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x00000001400011E8` -> `0x0000000140000000` type=`DATA` source=`ANALYSIS`
  - `0x00000001400011EF` -> `0x0000000140001234` type=`DATA` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x00000001400011FB` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x0000000140001201` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x0000000140001207` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x000000014000120D` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x0000000140001213` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x0000000140001219` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x000000014000121F` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400011F9` -> `0x0000000140001225` type=`COMPUTED_JUMP` source=`ANALYSIS`
- Callers:
  - `0x00000001400027EC` -> `0x00000001400011E0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400011E0` type=`EXTERNAL`
  - `0x000000014000163B` -> `0x00000001400011E0` type=`UNCONDITIONAL_CALL`
### 18. `sparse_mode` at `0x0000000140001260`
- Body: `0x0000000140001260-0x0000000140001286`
- Comment: ``
- Signature: `int __cdecl sparse_mode(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x0000000140001260` `CMP ECX,-0x64` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x8, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0xffffff9c, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0xffffff9c, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0xffffff9c, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001263` `JZ 0x140001281` flow=`CONDITIONAL_JUMP` pcode=`[' ---  CBRANCH (ram, 0x140001281, 8) , (register, 0x206, 1)']`
  - `0x0000000140001265` `CMP ECX,0x4d` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x8, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0x4d, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0x4d, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0x4d, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001268` `JZ 0x14000127b` flow=`CONDITIONAL_JUMP` pcode=`[' ---  CBRANCH (ram, 0x14000127b, 8) , (register, 0x206, 1)']`
  - `0x000000014000126A` `XOR EDX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x10, 4) INT_XOR (register, 0x10, 4) , (register, 0x10, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x10, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x10, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x10, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000126C` `MOV EAX,0x3` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x3, 8)']`
  - `0x0000000140001271` `CMP ECX,0x3e8` flow=`FALL_THROUGH` pcode=`['(unique, 0x78200, 4) COPY (register, 0x8, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78200, 4) , (const, 0x3e8, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78200, 4) , (const, 0x3e8, 4)', '(unique, 0x78400, 4) INT_SUB (unique, 0x78200, 4) , (const, 0x3e8, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78400, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78400, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78400, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001277` `CMOVNZ EAX,EDX` flow=`FALL_THROUGH` pcode=`['(unique, 0x24f00, 1) BOOL_NEGATE (register, 0x206, 1)', '(unique, 0x77000, 4) COPY (register, 0x10, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(unique, 0x77100, 1) BOOL_NEGATE (unique, 0x24f00, 1)', ' ---  CBRANCH (ram, 0x14000127a, 8) , (unique, 0x77100, 1)', '(register, 0x0, 4) COPY (unique, 0x77000, 4)']`
  - `0x000000014000127A` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x000000014000127B` `MOV EAX,0x2` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x2, 8)']`
  - `0x0000000140001280` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001281` `MOV EAX,0x1` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1, 8)']`
  - `0x0000000140001286` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001263` -> `0x0000000140001281` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x0000000140001268` -> `0x000000014000127B` type=`CONDITIONAL_JUMP` source=`DEFAULT`
- Callers:
  - `0x00000001400027E8` -> `0x0000000140001260` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001260` type=`EXTERNAL`
  - `0x0000000140001648` -> `0x0000000140001260` type=`UNCONDITIONAL_CALL`
### 19. `invoke_callback` at `0x0000000140001290`
- Body: `0x0000000140001290-0x00000001400012A4`
- Comment: ``
- Signature: `uint __cdecl invoke_callback(uint param_1, uint param_2); return=uint; convention=__cdecl; parameters=['param_1:uint', 'param_2:uint']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:uint:ECX:4', 'param_2:uint:EDX:4']`
- Instructions:
  - `0x0000000140001290` `MOV EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001292` `LEA RCX,[0x1400021b0]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 8) COPY (const, 0x1400021b0, 8)']`
  - `0x0000000140001299` `AND EAX,0x3` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x0, 4) INT_AND (register, 0x0, 4) , (const, 0x3, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000129C` `MOV RAX,qword ptr [RCX + RAX*0x8]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9300, 8) INT_MULT (register, 0x0, 8) , (const, 0x8, 8)', '(unique, 0x9500, 8) INT_ADD (register, 0x8, 8) , (unique, 0x9300, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9500, 8)', '(register, 0x0, 8) COPY (unique, 0x23e00, 8)']`
  - `0x00000001400012A0` `MOV ECX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 4) COPY (register, 0x10, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x00000001400012A2` `JMP RAX` flow=`COMPUTED_JUMP` pcode=`[' ---  BRANCHIND (register, 0x0, 8)']`
- References:
  - `0x0000000140001292` -> `0x00000001400021B0` type=`DATA` source=`ANALYSIS`
  - `0x000000014000129C` -> `0x00000001400021B0` type=`DATA` source=`ANALYSIS`
  - `0x00000001400012A2` -> `0x00000001400010E0` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400012A2` -> `0x00000001400010F0` type=`COMPUTED_JUMP` source=`ANALYSIS`
  - `0x00000001400012A2` -> `0x0000000140001100` type=`COMPUTED_JUMP` source=`ANALYSIS`
- Callers:
  - `0x00000001400027C8` -> `0x0000000140001290` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001290` type=`EXTERNAL`
  - `0x00000001400017B8` -> `0x0000000140001290` type=`UNCONDITIONAL_CALL`
  - `0x0000000140001608` -> `0x0000000140001290` type=`UNCONDITIONAL_CALL`
### 20. `update_entity` at `0x00000001400012B0`
- Body: `0x00000001400012B0-0x00000001400012EA`
- Comment: ``
- Signature: `int __cdecl update_entity(Entity * param_1, int param_2); return=int; convention=__cdecl; parameters=['param_1:Entity *', 'param_2:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:Entity *:RCX:8', 'param_2:int:EDX:4']`
- Instructions:
  - `0x00000001400012B0` `MOV qword ptr [RSP + 0x8],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400012B5` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400012B6` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400012BA` `MOV RAX,qword ptr [RCX]` flow=`FALL_THROUGH` pcode=`['(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (register, 0x8, 8)', '(register, 0x0, 8) COPY (unique, 0x23e00, 8)']`
  - `0x00000001400012BD` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x00000001400012C0` `CALL qword ptr [RAX]` flow=`COMPUTED_CALL` pcode=`['(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (register, 0x0, 8)', '(unique, 0x75000, 8) COPY (unique, 0x23e00, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400012c2, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
  - `0x00000001400012C2` `MOV RDX,qword ptr [RBX]` flow=`FALL_THROUGH` pcode=`['(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (register, 0x18, 8)', '(register, 0x10, 8) COPY (unique, 0x23e00, 8)']`
  - `0x00000001400012C5` `MOV RCX,RBX` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 8) COPY (register, 0x18, 8)']`
  - `0x00000001400012C8` `MOV EDI,EAX` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 4) COPY (register, 0x0, 4)', '(register, 0x38, 8) INT_ZEXT (register, 0x38, 4)']`
  - `0x00000001400012CA` `CALL qword ptr [RDX + 0x8]` flow=`COMPUTED_CALL` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x10, 8) , (const, 0x8, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x75000, 8) COPY (unique, 0x23e00, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400012cd, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
  - `0x00000001400012CD` `MOV EDX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x10, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)']`
  - `0x00000001400012D3` `MOV RBX,qword ptr [RSP + 0x30]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x30, 8) , (register, 0x20, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x18, 8) COPY (unique, 0x23e00, 8)']`
  - `0x00000001400012D8` `MOVSX ECX,byte ptr [RAX]` flow=`FALL_THROUGH` pcode=`['(unique, 0x23b00, 1) LOAD (const, 0x1b1, 4) , (register, 0x0, 8)', '(register, 0x8, 4) INT_SEXT (unique, 0x23b00, 1)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x00000001400012DB` `MOV EAX,EDI` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x38, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400012DD` `XOR EDX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x10, 4) INT_XOR (register, 0x10, 4) , (register, 0x8, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x10, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x10, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x10, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400012DF` `MOV dword ptr [0x14000302c],EDX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x10, 4)']`
  - `0x00000001400012E5` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400012E9` `POP RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x38, 8) COPY (unique, 0xa7200, 8)']`
  - `0x00000001400012EA` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x00000001400012B0` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400012CD` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x00000001400012D3` -> `0x0000000000000008` type=`READ` source=`ANALYSIS`
  - `0x00000001400012DF` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x00000001400027F4` -> `0x00000001400012B0` type=`DATA`
  - `0x0000000140004024` -> `0x00000001400012B0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400012B0` type=`EXTERNAL`
  - `0x000000014000130B` -> `0x00000001400012B0` type=`UNCONDITIONAL_CALL`
### 21. `update_entity_pointer` at `0x0000000140001300`
- Body: `0x0000000140001300-0x000000014000130F`
- Comment: ``
- Signature: `int __cdecl update_entity_pointer(Entity * param_1, int param_2); return=int; convention=__cdecl; parameters=['param_1:Entity *', 'param_2:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:Entity *:RCX:8', 'param_2:int:EDX:4']`
- Instructions:
  - `0x0000000140001300` `TEST RCX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(unique, 0xd3500, 8) INT_AND (register, 0x8, 8) , (register, 0x8, 8)', '(register, 0x207, 1) INT_SLESS (unique, 0xd3500, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd3500, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (unique, 0xd3500, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001303` `JNZ 0x140001309` flow=`CONDITIONAL_JUMP` pcode=`['(unique, 0x24f00, 1) BOOL_NEGATE (register, 0x206, 1)', ' ---  CBRANCH (ram, 0x140001309, 8) , (unique, 0x24f00, 1)']`
  - `0x0000000140001305` `LEA EAX,[RCX + -0x1]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0xffffffffffffffff, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x8f00, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001308` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001309` `INC EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x20b, 1) INT_SCARRY (register, 0x10, 4) , (const, 0x1, 4)', '(register, 0x10, 4) INT_ADD (register, 0x10, 4) , (const, 0x1, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x10, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x10, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x10, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000130B` `JMP 0x1400012b0` flow=`CALL_TERMINATOR` pcode=`[' ---  BRANCH (ram, 0x1400012b0, 8)']`
- References:
  - `0x0000000140001303` -> `0x0000000140001309` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x000000014000130B` -> `0x00000001400012B0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400027F8` -> `0x0000000140001300` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001300` type=`EXTERNAL`
  - `0x0000000140001594` -> `0x0000000140001300` type=`UNCONDITIONAL_CALL`
### 22. `engine_abort` at `0x0000000140001320`
- Body: `0x0000000140001320-0x0000000140001334`
- Comment: ``
- Signature: `noreturn void __cdecl engine_abort(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001320` `NOP` flow=`FALL_THROUGH` pcode=`[]`
  - `0x0000000140001322` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001328` `XOR EAX,0xdeadbeef` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x0, 4) INT_XOR (register, 0x0, 4) , (const, 0xdeadbeef, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000132D` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x0000000140001333` `JMP 0x140001322` flow=`UNCONDITIONAL_JUMP` pcode=`[' ---  BRANCH (ram, 0x140001322, 8)']`
- References:
  - `0x0000000140001322` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x000000014000132D` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x0000000140001333` -> `0x0000000140001322` type=`UNCONDITIONAL_JUMP` source=`DEFAULT`
- Callers:
  - `0x0000000140002760` -> `0x0000000140001320` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001320` type=`EXTERNAL`
  - `0x0000000140001362` -> `0x0000000140001320` type=`UNCONDITIONAL_CALL`
  - `0x0000000140001393` -> `0x0000000140001320` type=`UNCONDITIONAL_CALL`
  - `0x00000001400013C3` -> `0x0000000140001320` type=`UNCONDITIONAL_CALL`
### 23. `abort_path_one` at `0x0000000140001340`
- Body: `0x0000000140001340-0x0000000140001366`
- Comment: ``
- Signature: `void __cdecl abort_path_one(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001340` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001344` `MOV EAX,dword ptr [0x140003000]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x140003000, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x000000014000134A` `CMP EAX,-0x1` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x0, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0xffffffff, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0xffffffff, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0xffffffff, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000134D` `JZ 0x140001362` flow=`CONDITIONAL_JUMP` pcode=`[' ---  CBRANCH (ram, 0x140001362, 8) , (register, 0x206, 1)']`
  - `0x000000014000134F` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001355` `INC EAX` flow=`FALL_THROUGH` pcode=`['(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x1, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x1, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001357` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000135D` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001361` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001362` `CALL 0x140001320` flow=`CALL_TERMINATOR` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001367, 8)', ' ---  CALL (ram, 0x140001320, 8)']`
- References:
  - `0x0000000140001344` -> `0x0000000140003000` type=`READ` source=`DEFAULT`
  - `0x000000014000134D` -> `0x0000000140001362` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x000000014000134F` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001357` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x0000000140001362` -> `0x0000000140001320` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x0000000140002748` -> `0x0000000140001340` type=`DATA`
  - `0x0000000140004030` -> `0x0000000140001340` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001340` type=`EXTERNAL`
  - `0x0000000140001778` -> `0x0000000140001340` type=`UNCONDITIONAL_CALL`
### 24. `abort_path_two` at `0x0000000140001370`
- Body: `0x0000000140001370-0x0000000140001397`
- Comment: ``
- Signature: `void __cdecl abort_path_two(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001370` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001374` `MOV EAX,dword ptr [0x140003000]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x140003000, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x000000014000137A` `CMP EAX,-0x2` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x0, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0xfffffffe, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0xfffffffe, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0xfffffffe, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000137D` `JZ 0x140001393` flow=`CONDITIONAL_JUMP` pcode=`[' ---  CBRANCH (ram, 0x140001393, 8) , (register, 0x206, 1)']`
  - `0x000000014000137F` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001385` `ADD EAX,0x2` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x2, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x2, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x2, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001388` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x000000014000138E` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001392` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x0000000140001393` `CALL 0x140001320` flow=`CALL_TERMINATOR` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001398, 8)', ' ---  CALL (ram, 0x140001320, 8)']`
- References:
  - `0x0000000140001374` -> `0x0000000140003000` type=`READ` source=`DEFAULT`
  - `0x000000014000137D` -> `0x0000000140001393` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x000000014000137F` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001388` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x0000000140001393` -> `0x0000000140001320` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x0000000140002750` -> `0x0000000140001370` type=`DATA`
  - `0x000000014000403C` -> `0x0000000140001370` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001370` type=`EXTERNAL`
  - `0x000000014000177D` -> `0x0000000140001370` type=`UNCONDITIONAL_CALL`
### 25. `abort_path_three` at `0x00000001400013A0`
- Body: `0x00000001400013A0-0x00000001400013C7`
- Comment: ``
- Signature: `void __cdecl abort_path_three(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x00000001400013A0` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400013A4` `MOV EAX,dword ptr [0x140003000]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x140003000, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400013AA` `CMP EAX,-0x3` flow=`FALL_THROUGH` pcode=`['(unique, 0x78b00, 4) COPY (register, 0x0, 4)', '(register, 0x200, 1) INT_LESS (unique, 0x78b00, 4) , (const, 0xfffffffd, 4)', '(register, 0x20b, 1) INT_SBORROW (unique, 0x78b00, 4) , (const, 0xfffffffd, 4)', '(unique, 0x78d00, 4) INT_SUB (unique, 0x78b00, 4) , (const, 0xfffffffd, 4)', '(register, 0x207, 1) INT_SLESS (unique, 0x78d00, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (unique, 0x78d00, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (unique, 0x78d00, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400013AD` `JZ 0x1400013c3` flow=`CONDITIONAL_JUMP` pcode=`[' ---  CBRANCH (ram, 0x1400013c3, 8) , (register, 0x206, 1)']`
  - `0x00000001400013AF` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400013B5` `ADD EAX,0x3` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x3, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x3, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x3, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400013B8` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x00000001400013BE` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400013C2` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
  - `0x00000001400013C3` `CALL 0x140001320` flow=`CALL_TERMINATOR` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400013c8, 8)', ' ---  CALL (ram, 0x140001320, 8)']`
- References:
  - `0x00000001400013A4` -> `0x0000000140003000` type=`READ` source=`DEFAULT`
  - `0x00000001400013AD` -> `0x00000001400013C3` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x00000001400013AF` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x00000001400013B8` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x00000001400013C3` -> `0x0000000140001320` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x000000014000274C` -> `0x00000001400013A0` type=`DATA`
  - `0x0000000140004048` -> `0x00000001400013A0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400013A0` type=`EXTERNAL`
  - `0x0000000140001782` -> `0x00000001400013A0` type=`UNCONDITIONAL_CALL`
### 26. `resource_lookup` at `0x00000001400013D0`
- Body: `0x00000001400013D0-0x0000000140001447`
- Comment: ``
- Signature: `void __cdecl resource_lookup(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x00000001400013D0` `MOV RAX,RSP` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x20, 8)']`
  - `0x00000001400013D3` `SUB RSP,0xa8` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0xa8, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0xa8, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0xa8, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400013DA` `XORPS XMM0,XMM0` flow=`FALL_THROUGH` pcode=`['(register, 0x1200, 4) INT_XOR (register, 0x1200, 4) , (register, 0x1200, 4)', '(register, 0x1204, 4) INT_XOR (register, 0x1204, 4) , (register, 0x1204, 4)', '(register, 0x1208, 4) INT_XOR (register, 0x1208, 4) , (register, 0x1208, 4)', '(register, 0x120c, 4) INT_XOR (register, 0x120c, 4) , (register, 0x120c, 4)']`
  - `0x00000001400013DD` `LEA R8,[RSP + 0x20]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x20, 8) , (register, 0x20, 8)', '(register, 0x80, 8) COPY (unique, 0x9d00, 8)']`
  - `0x00000001400013E2` `MOVUPS xmmword ptr [RSP + 0x20],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x20, 8) , (register, 0x20, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd700, 16)']`
  - `0x00000001400013E7` `MOV R9D,0x40` flow=`FALL_THROUGH` pcode=`['(register, 0x88, 8) COPY (const, 0x40, 8)']`
  - `0x00000001400013ED` `XOR ECX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x8, 4) INT_XOR (register, 0x8, 4) , (register, 0x8, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400013EF` `MOVUPS xmmword ptr [RAX + -0x78],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffff88, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x00000001400013F3` `MOVUPS xmmword ptr [RAX + -0x68],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffff98, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x00000001400013F7` `LEA EDX,[R9 + 0x25]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x88, 8) , (const, 0x25, 8)', '(register, 0x10, 4) SUBPIECE (unique, 0x8f00, 8) , (const, 0x0, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)']`
  - `0x00000001400013FB` `MOVUPS xmmword ptr [RAX + -0x58],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffffa8, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x00000001400013FF` `MOVUPS xmmword ptr [RAX + -0x48],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffffb8, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x0000000140001403` `MOVUPS xmmword ptr [RAX + -0x38],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffffc8, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x0000000140001407` `MOVUPS xmmword ptr [RAX + -0x28],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffffd8, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x000000014000140B` `MOVUPS xmmword ptr [RAX + -0x18],XMM0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x0, 8) , (const, 0xffffffffffffffe8, 8)', '(unique, 0xd700, 16) COPY (register, 0x1200, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd700, 16)']`
  - `0x000000014000140F` `CALL qword ptr [0x140002020]` flow=`COMPUTED_CALL` pcode=`['(unique, 0x75000, 8) COPY (ram, 0x140002020, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001415, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
  - `0x0000000140001415` `MOV R9D,0x40` flow=`FALL_THROUGH` pcode=`['(register, 0x88, 8) COPY (const, 0x40, 8)']`
  - `0x000000014000141B` `LEA R8,[RSP + 0x20]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x20, 8) , (register, 0x20, 8)', '(register, 0x80, 8) COPY (unique, 0x9d00, 8)']`
  - `0x0000000140001420` `XOR ECX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x8, 4) INT_XOR (register, 0x8, 4) , (register, 0x8, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x8, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001422` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x0000000140001428` `LEA EDX,[R9 + 0x26]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x88, 8) , (const, 0x26, 8)', '(register, 0x10, 4) SUBPIECE (unique, 0x8f00, 8) , (const, 0x0, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)']`
  - `0x000000014000142C` `CALL qword ptr [0x140002020]` flow=`COMPUTED_CALL` pcode=`['(unique, 0x75000, 8) COPY (ram, 0x140002020, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001432, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
  - `0x0000000140001432` `MOV ECX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x0000000140001438` `ADD EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - [TRUNCATED: maximum `24` instructions]
- References:
  - `0x00000001400013DD` -> `0x-000000000000088` type=`DATA` source=`ANALYSIS`
  - `0x00000001400013E2` -> `0x-000000000000088` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400013EF` -> `0x-000000000000078` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400013F3` -> `0x-000000000000068` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400013FB` -> `0x-000000000000058` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400013FF` -> `0x-000000000000048` type=`WRITE` source=`ANALYSIS`
  - `0x0000000140001403` -> `0x-000000000000038` type=`WRITE` source=`ANALYSIS`
  - `0x0000000140001407` -> `0x-000000000000028` type=`WRITE` source=`ANALYSIS`
  - `0x000000014000140B` -> `0x-000000000000018` type=`WRITE` source=`ANALYSIS`
  - `0x000000014000140F` -> `0x0000000140002020` type=`READ` source=`DEFAULT`
  - `0x000000014000140F` -> `0x0000000000000004` type=`COMPUTED_CALL` source=`ANALYSIS`
  - `0x000000014000141B` -> `0x-000000000000088` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001422` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x000000014000142C` -> `0x0000000140002020` type=`READ` source=`DEFAULT`
  - `0x000000014000142C` -> `0x0000000000000004` type=`COMPUTED_CALL` source=`ANALYSIS`
  - `0x0000000140001432` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x000000014000143A` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x00000001400027E0` -> `0x00000001400013D0` type=`DATA`
  - `0x0000000140004054` -> `0x00000001400013D0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400013D0` type=`EXTERNAL`
  - `0x000000014000176E` -> `0x00000001400013D0` type=`UNCONDITIONAL_CALL`
### 27. `system_calls` at `0x0000000140001450`
- Body: `0x0000000140001450-0x000000014000147E`
- Comment: ``
- Signature: `void __cdecl system_calls(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001450` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001454` `LEA RCX,[0x140002030]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 8) COPY (const, 0x140002030, 8)']`
  - `0x000000014000145B` `CALL qword ptr [0x140002010]` flow=`COMPUTED_CALL` pcode=`['(unique, 0x75000, 8) COPY (ram, 0x140002010, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001461, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
  - `0x0000000140001461` `CALL qword ptr [0x140002000]` flow=`COMPUTED_CALL` pcode=`['(unique, 0x75000, 8) COPY (ram, 0x140002000, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001467, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
  - `0x0000000140001467` `MOV RCX,qword ptr [0x140003030]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 8) COPY (ram, 0x140003030, 8)']`
  - `0x000000014000146E` `MOV EAX,EAX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001470` `ADD RCX,RAX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x8, 8) , (register, 0x0, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x8, 8) , (register, 0x0, 8)', '(register, 0x8, 8) INT_ADD (register, 0x8, 8) , (register, 0x0, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x8, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x8, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x8, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001473` `MOV qword ptr [0x140003030],RCX` flow=`FALL_THROUGH` pcode=`['(ram, 0x140003030, 8) COPY (register, 0x8, 8)']`
  - `0x000000014000147A` `ADD RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000147E` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001454` -> `0x0000000140002030` type=`DATA` source=`ANALYSIS`
  - `0x000000014000145B` -> `0x0000000140002010` type=`READ` source=`DEFAULT`
  - `0x000000014000145B` -> `0x0000000000000003` type=`COMPUTED_CALL` source=`ANALYSIS`
  - `0x0000000140001461` -> `0x0000000140002000` type=`READ` source=`DEFAULT`
  - `0x0000000140001461` -> `0x0000000000000001` type=`COMPUTED_CALL` source=`ANALYSIS`
  - `0x0000000140001467` -> `0x0000000140003030` type=`READ` source=`DEFAULT`
  - `0x0000000140001473` -> `0x0000000140003030` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x00000001400027F0` -> `0x0000000140001450` type=`DATA`
  - `0x0000000140004060` -> `0x0000000140001450` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001450` type=`EXTERNAL`
  - `0x0000000140001773` -> `0x0000000140001450` type=`UNCONDITIONAL_CALL`
### 28. `shutdown_engine` at `0x0000000140001490`
- Body: `0x0000000140001490-0x000000014000149F`
- Comment: ``
- Signature: `noreturn void __cdecl shutdown_engine(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001490` `SUB RSP,0x28` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x28, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001494` `MOV ECX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x000000014000149A` `CALL qword ptr [0x140002008]` flow=`COMPUTED_CALL_TERMINATOR` pcode=`['(unique, 0x75000, 8) COPY (ram, 0x140002008, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400014a0, 8)', ' ---  CALLIND (unique, 0x75000, 8)']`
- References:
  - `0x0000000140001494` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x000000014000149A` -> `0x0000000140002008` type=`READ` source=`DEFAULT`
  - `0x000000014000149A` -> `0x0000000000000002` type=`COMPUTED_CALL_TERMINATOR` source=`ANALYSIS`
- Callers:
  - `0x00000001400027E4` -> `0x0000000140001490` type=`DATA`
  - `0x000000014000406C` -> `0x0000000140001490` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001490` type=`EXTERNAL`
### 29. `engine_tick` at `0x00000001400014B0`
- Body: `0x00000001400014B0-0x000000014000172D`
- Comment: ``
- Signature: `uint __cdecl engine_tick(int param_1, char * param_2); return=uint; convention=__cdecl; parameters=['param_1:int', 'param_2:char *']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['param_1:int:ECX:4', 'param_2:char *:RDX:8']`
- Instructions:
  - `0x00000001400014B0` `MOV qword ptr [RSP + 0x10],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x10, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400014B5` `MOV dword ptr [RSP + 0x8],ECX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd400, 4) COPY (register, 0x8, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd400, 4)']`
  - `0x00000001400014B9` `PUSH RBP` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x28, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014BA` `PUSH RSI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x30, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014BB` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014BC` `PUSH R12` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0xa0, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014BE` `PUSH R13` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0xa8, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014C0` `PUSH R14` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0xb0, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014C2` `PUSH R15` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0xb8, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400014C4` `SUB RSP,0xd0` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0xd0, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0xd0, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0xd0, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400014CB` `MOV EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400014CD` `MOVAPS xmmword ptr [RSP + 0xc0],XMM6` flow=`FALL_THROUGH` pcode=`['(unique, 0xa600, 8) INT_ADD (const, 0xc0, 8) , (register, 0x20, 8)', '(unique, 0xd700, 16) COPY (register, 0x1380, 16)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0xa600, 8) , (unique, 0xd700, 16)']`
  - `0x00000001400014D5` `XOR EAX,0x55aa` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x0, 4) INT_XOR (register, 0x0, 4) , (const, 0x55aa, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400014DA` `MOV R9D,0x5a` flow=`FALL_THROUGH` pcode=`['(register, 0x88, 8) COPY (const, 0x5a, 8)']`
  - `0x00000001400014E0` `MOV dword ptr [RSP + 0x128],EAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa600, 8) INT_ADD (const, 0x128, 8) , (register, 0x20, 8)', '(unique, 0xd400, 4) COPY (register, 0x0, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0xa600, 8) , (unique, 0xd400, 4)']`
  - `0x00000001400014E7` `MOV RBX,RDX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x10, 8)']`
  - `0x00000001400014EA` `MOV EAX,dword ptr [RSP + 0x128]` flow=`FALL_THROUGH` pcode=`['(unique, 0xa600, 8) INT_ADD (const, 0x128, 8) , (register, 0x20, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0xa600, 8)', '(register, 0x0, 4) COPY (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400014F1` `MOV EDI,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 4) COPY (register, 0x8, 4)', '(register, 0x38, 8) INT_ZEXT (register, 0x38, 4)']`
  - `0x00000001400014F3` `MOV R8,qword ptr [0x140003030]` flow=`FALL_THROUGH` pcode=`['(register, 0x80, 8) COPY (ram, 0x140003030, 8)']`
  - `0x00000001400014FA` `LEA RCX,[RSP + 0x78]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x78, 8) , (register, 0x20, 8)', '(register, 0x8, 8) COPY (unique, 0x9d00, 8)']`
  - `0x00000001400014FF` `ADD R8,RAX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x80, 8) , (register, 0x0, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x80, 8) , (register, 0x0, 8)', '(register, 0x80, 8) INT_ADD (register, 0x80, 8) , (register, 0x0, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x80, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x80, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x80, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001502` `MOV qword ptr [RSP + 0x30],R8` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x30, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x80, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001507` `MOV R8,RDX` flow=`FALL_THROUGH` pcode=`['(register, 0x80, 8) COPY (register, 0x10, 8)']`
  - `0x000000014000150A` `MOV EAX,dword ptr [RSP + 0x128]` flow=`FALL_THROUGH` pcode=`['(unique, 0xa600, 8) INT_ADD (const, 0x128, 8) , (register, 0x20, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0xa600, 8)', '(register, 0x0, 4) COPY (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - [TRUNCATED: maximum `24` instructions]
- References:
  - `0x00000001400014B0` -> `0x0000000000000010` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400014B5` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400014CD` -> `0x-000000000000048` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400014E0` -> `0x0000000000000020` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400014EA` -> `0x0000000000000020` type=`READ` source=`ANALYSIS`
  - `0x00000001400014F3` -> `0x0000000140003030` type=`READ` source=`DEFAULT`
  - `0x00000001400014FA` -> `0x-000000000000090` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001502` -> `0x-0000000000000D8` type=`WRITE` source=`ANALYSIS`
  - `0x000000014000150A` -> `0x0000000000000020` type=`READ` source=`ANALYSIS`
  - `0x0000000140001515` -> `0x0000000000000018` type=`WRITE` source=`ANALYSIS`
  - `0x000000014000151C` -> `0x0000000140001920` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001527` -> `0x-0000000000000E8` type=`WRITE` source=`ANALYSIS`
  - `0x0000000140001532` -> `0x-000000000000070` type=`DATA` source=`ANALYSIS`
  - `0x000000014000153E` -> `0x0000000140001880` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001549` -> `0x-0000000000000A8` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001557` -> `0x00000001400018E0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001561` -> `0x-0000000000000C0` type=`DATA` source=`ANALYSIS`
  - `0x000000014000156C` -> `0x00000001400018B0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001573` -> `0x-000000000000090` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001578` -> `0x-0000000000000D0` type=`WRITE` source=`ANALYSIS`
  - [TRUNCATED: maximum `20` references]
- Callers:
  - `0x00000001400027A4` -> `0x00000001400014B0` type=`DATA`
  - `0x0000000140004078` -> `0x00000001400014B0` type=`DATA`
  - `0x0000000000000000` -> `0x00000001400014B0` type=`EXTERNAL`
  - `0x0000000140001797` -> `0x00000001400014B0` type=`UNCONDITIONAL_CALL`
### 30. `engine_unused_dead_code` at `0x0000000140001740`
- Body: `0x0000000140001740-0x0000000140001748`
- Comment: ``
- Signature: `int __cdecl engine_unused_dead_code(int param_1); return=int; convention=__cdecl; parameters=['param_1:int']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['param_1:int:ECX:4']`
- Instructions:
  - `0x0000000140001740` `IMUL EAX,ECX,0x7f` flow=`FALL_THROUGH` pcode=`['(unique, 0x93a00, 8) INT_SEXT (register, 0x8, 4)', '(unique, 0x93b00, 8) INT_SEXT (const, 0x7f, 4)', '(unique, 0x93d00, 8) INT_MULT (unique, 0x93a00, 8) , (unique, 0x93b00, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x93d00, 8) , (const, 0x0, 4)', '(unique, 0x94000, 4) SUBPIECE (unique, 0x93d00, 8) , (const, 0x4, 4)', '(unique, 0x5a200, 8) INT_SEXT (register, 0x0, 4)', '(register, 0x200, 1) INT_NOTEQUAL (unique, 0x5a200, 8) , (unique, 0x93d00, 8)', '(register, 0x20b, 1) COPY (register, 0x200, 1)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001743` `ADD EAX,0x123` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (const, 0x123, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (const, 0x123, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (const, 0x123, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001748` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x00000001400027AC` -> `0x0000000140001740` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001740` type=`EXTERNAL`
### 31. `fixture_entry` at `0x0000000140001750`
- Body: `0x0000000140001750-0x0000000140001808`
- Comment: ``
- Signature: `void __cdecl fixture_entry(void); return=void; convention=__cdecl; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001750` `MOV qword ptr [RSP + 0x8],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001755` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001756` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000175A` `CALL 0x140001000` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x14000175f, 8)', ' ---  CALL (ram, 0x140001000, 8)']`
  - `0x000000014000175F` `CALL 0x140001020` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001764, 8)', ' ---  CALL (ram, 0x140001020, 8)']`
  - `0x0000000140001764` `CALL 0x140001040` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001769, 8)', ' ---  CALL (ram, 0x140001040, 8)']`
  - `0x0000000140001769` `CALL 0x140001060` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x14000176e, 8)', ' ---  CALL (ram, 0x140001060, 8)']`
  - `0x000000014000176E` `CALL 0x1400013d0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001773, 8)', ' ---  CALL (ram, 0x1400013d0, 8)']`
  - `0x0000000140001773` `CALL 0x140001450` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001778, 8)', ' ---  CALL (ram, 0x140001450, 8)']`
  - `0x0000000140001778` `CALL 0x140001340` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x14000177d, 8)', ' ---  CALL (ram, 0x140001340, 8)']`
  - `0x000000014000177D` `CALL 0x140001370` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001782, 8)', ' ---  CALL (ram, 0x140001370, 8)']`
  - `0x0000000140001782` `CALL 0x1400013a0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001787, 8)', ' ---  CALL (ram, 0x1400013a0, 8)']`
  - `0x0000000140001787` `MOV ECX,dword ptr [0x140003000]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 4) COPY (ram, 0x140003000, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x000000014000178D` `LEA RDI,[0x140002030]` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 8) COPY (const, 0x140002030, 8)']`
  - `0x0000000140001794` `MOV RDX,RDI` flow=`FALL_THROUGH` pcode=`['(register, 0x10, 8) COPY (register, 0x38, 8)']`
  - `0x0000000140001797` `CALL 0x1400014b0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x14000179c, 8)', ' ---  CALL (ram, 0x1400014b0, 8)']`
  - `0x000000014000179C` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x00000001400017A2` `XOR EBX,EBX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x18, 4) INT_XOR (register, 0x18, 4) , (register, 0x18, 4)', '(register, 0x18, 8) INT_ZEXT (register, 0x18, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x18, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x18, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x18, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400017A4` `NOP dword ptr [RAX]` flow=`FALL_THROUGH` pcode=`[]`
  - `0x00000001400017A8` `NOP dword ptr [RAX + RAX*0x1]` flow=`FALL_THROUGH` pcode=`['(unique, 0xab00, 8) INT_MULT (register, 0x0, 8) , (const, 0x1, 8)', '(unique, 0xad00, 8) INT_ADD (register, 0x0, 8) , (unique, 0xab00, 8)']`
  - `0x00000001400017B0` `MOV EDX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x10, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)']`
  - `0x00000001400017B6` `MOV ECX,EBX` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 4) COPY (register, 0x18, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x00000001400017B8` `CALL 0x140001290` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400017bd, 8)', ' ---  CALL (ram, 0x140001290, 8)']`
  - `0x00000001400017BD` `MOV ECX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x8, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - [TRUNCATED: maximum `24` instructions]
- References:
  - `0x0000000140001750` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x000000014000175A` -> `0x0000000140001000` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x000000014000175F` -> `0x0000000140001020` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001764` -> `0x0000000140001040` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001769` -> `0x0000000140001060` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x000000014000176E` -> `0x00000001400013D0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001773` -> `0x0000000140001450` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001778` -> `0x0000000140001340` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x000000014000177D` -> `0x0000000140001370` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001782` -> `0x00000001400013A0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001787` -> `0x0000000140003000` type=`READ` source=`DEFAULT`
  - `0x000000014000178D` -> `0x0000000140002030` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001797` -> `0x00000001400014B0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x000000014000179C` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x00000001400017B0` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x00000001400017B8` -> `0x0000000140001290` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x00000001400017BD` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x00000001400017C7` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x00000001400017D0` -> `0x00000001400017B0` type=`CONDITIONAL_JUMP` source=`DEFAULT`
  - `0x00000001400017D2` -> `0x0000000140003030` type=`READ` source=`DEFAULT`
  - [TRUNCATED: maximum `20` references]
- Callers:
  - `0x0000000140000108` -> `0x0000000140001750` type=`DATA`
  - `0x00000001400027C4` -> `0x0000000140001750` type=`DATA`
  - `0x0000000140004084` -> `0x0000000140001750` type=`DATA`
  - `0x0000000000000000` -> `0x0000000140001750` type=`EXTERNAL`
### 32. `Asset` at `0x0000000140001810`
- Body: `0x0000000140001810-0x0000000140001820`
- Comment: ``
- Signature: `Asset * __thiscall Asset(Asset * this, uint param_1); return=Asset *; convention=__thiscall; parameters=['this:Asset *', 'param_1:uint']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Asset *:RCX:8 (auto)', 'param_1:uint:EDX:4']`
- Instructions:
  - `0x0000000140001810` `LEA RAX,[0x140002118]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002118, 8)']`
  - `0x0000000140001817` `MOV dword ptr [RCX + 0x8],EDX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(unique, 0xd400, 4) COPY (register, 0x10, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000181A` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x000000014000181D` `MOV RAX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001820` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001810` -> `0x0000000140002118` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x00000001400018C0` -> `0x0000000140001810` type=`UNCONDITIONAL_CALL`
  - `0x00000001400018F8` -> `0x0000000140001810` type=`UNCONDITIONAL_CALL`
### 33. `Entity` at `0x0000000140001830`
- Body: `0x0000000140001830-0x0000000140001844`
- Comment: ``
- Signature: `Entity * __thiscall Entity(Entity * this, int param_1, char * param_2); return=Entity *; convention=__thiscall; parameters=['this:Entity *', 'param_1:int', 'param_2:char *']`
- Stack frame: `frame=40, local=40, parameters=3,  variables=['this:Entity *:RCX:8 (auto)', 'param_1:int:EDX:4', 'param_2:char *:R8:8']`
- Instructions:
  - `0x0000000140001830` `LEA RAX,[0x1400020b8]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020b8, 8)']`
  - `0x0000000140001837` `MOV dword ptr [RCX + 0x8],EDX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(unique, 0xd400, 4) COPY (register, 0x10, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000183A` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x000000014000183D` `MOV RAX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001840` `MOV qword ptr [RCX + 0x10],R8` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x10, 8)', '(unique, 0xd500, 8) COPY (register, 0x80, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001844` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001830` -> `0x00000001400020B8` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x0000000140001860` -> `0x0000000140001830` type=`UNCONDITIONAL_CALL`
  - `0x0000000140001930` -> `0x0000000140001830` type=`UNCONDITIONAL_CALL`
### 34. `Ped` at `0x0000000140001850`
- Body: `0x0000000140001850-0x000000014000187F`
- Comment: ``
- Signature: `Ped * __thiscall Ped(Ped * this, int param_1, char * param_2, int param_3); return=Ped *; convention=__thiscall; parameters=['this:Ped *', 'param_1:int', 'param_2:char *', 'param_3:int']`
- Stack frame: `frame=40, local=40, parameters=4,  variables=['this:Ped *:RCX:8 (auto)', 'param_1:int:EDX:4', 'param_2:char *:R8:8', 'param_3:int:R9D:4']`
- Instructions:
  - `0x0000000140001850` `MOV qword ptr [RSP + 0x8],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001855` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001856` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000185A` `MOV EDI,R9D` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 4) COPY (register, 0x88, 4)', '(register, 0x38, 8) INT_ZEXT (register, 0x38, 4)']`
  - `0x000000014000185D` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001860` `CALL 0x140001830` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001865, 8)', ' ---  CALL (ram, 0x140001830, 8)']`
  - `0x0000000140001865` `LEA RAX,[0x1400020e8]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020e8, 8)']`
  - `0x000000014000186C` `MOV dword ptr [RBX + 0x18],EDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) COPY (register, 0x38, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000186F` `MOV qword ptr [RBX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x18, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001872` `MOV RAX,RBX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x18, 8)']`
  - `0x0000000140001875` `MOV RBX,qword ptr [RSP + 0x30]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x30, 8) , (register, 0x20, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x18, 8) COPY (unique, 0x23e00, 8)']`
  - `0x000000014000187A` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000187E` `POP RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x38, 8) COPY (unique, 0xa7200, 8)']`
  - `0x000000014000187F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001850` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x0000000140001860` -> `0x0000000140001830` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001865` -> `0x00000001400020E8` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001875` -> `0x0000000000000008` type=`READ` source=`ANALYSIS`
- Callers:
  - `0x0000000140004090` -> `0x0000000140001850` type=`DATA`
  - `0x0000000140001889` -> `0x0000000140001850` type=`UNCONDITIONAL_CALL`
### 35. `Player` at `0x0000000140001880`
- Body: `0x0000000140001880-0x00000001400018A7`
- Comment: ``
- Signature: `Player * __thiscall Player(Player * this, int param_1, char * param_2, int param_3, uint param_4); return=Player *; convention=__thiscall; parameters=['this:Player *', 'param_1:int', 'param_2:char *', 'param_3:int', 'param_4:uint']`
- Stack frame: `frame=44, local=40, parameters=5,  variables=['this:Player *:RCX:8 (auto)', 'param_1:int:EDX:4', 'param_2:char *:R8:8', 'param_3:int:R9D:4', 'param_4:uint:Stack[0x28]:4']`
- Instructions:
  - `0x0000000140001880` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001882` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001886` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001889` `CALL 0x140001850` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x14000188e, 8)', ' ---  CALL (ram, 0x140001850, 8)']`
  - `0x000000014000188E` `LEA RAX,[0x140002100]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002100, 8)']`
  - `0x0000000140001895` `MOV qword ptr [RBX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x18, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001898` `MOV EAX,dword ptr [RSP + 0x50]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x50, 8) , (register, 0x20, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x0, 4) COPY (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x000000014000189C` `MOV dword ptr [RBX + 0x20],EAX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x20, 8)', '(unique, 0xd400, 4) COPY (register, 0x0, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000189F` `MOV RAX,RBX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x18, 8)']`
  - `0x00000001400018A2` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400018A6` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x00000001400018A7` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001889` -> `0x0000000140001850` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x000000014000188E` -> `0x0000000140002100` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001898` -> `0x0000000000000028` type=`READ` source=`ANALYSIS`
- Callers:
  - `0x000000014000409C` -> `0x0000000140001880` type=`DATA`
  - `0x000000014000153E` -> `0x0000000140001880` type=`UNCONDITIONAL_CALL`
### 36. `ScriptAsset` at `0x00000001400018B0`
- Body: `0x00000001400018B0-0x00000001400018DF`
- Comment: ``
- Signature: `ScriptAsset * __thiscall ScriptAsset(ScriptAsset * this, uint param_1, uint param_2); return=ScriptAsset *; convention=__thiscall; parameters=['this:ScriptAsset *', 'param_1:uint', 'param_2:uint']`
- Stack frame: `frame=40, local=40, parameters=3,  variables=['this:ScriptAsset *:RCX:8 (auto)', 'param_1:uint:EDX:4', 'param_2:uint:R8D:4']`
- Instructions:
  - `0x00000001400018B0` `MOV qword ptr [RSP + 0x8],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400018B5` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400018B6` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400018BA` `MOV EDI,R8D` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 4) COPY (register, 0x80, 4)', '(register, 0x38, 8) INT_ZEXT (register, 0x38, 4)']`
  - `0x00000001400018BD` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x00000001400018C0` `CALL 0x140001810` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400018c5, 8)', ' ---  CALL (ram, 0x140001810, 8)']`
  - `0x00000001400018C5` `LEA RAX,[0x140002128]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002128, 8)']`
  - `0x00000001400018CC` `MOV dword ptr [RBX + 0x10],EDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x10, 8)', '(unique, 0xd400, 4) COPY (register, 0x38, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x00000001400018CF` `MOV qword ptr [RBX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x18, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400018D2` `MOV RAX,RBX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x18, 8)']`
  - `0x00000001400018D5` `MOV RBX,qword ptr [RSP + 0x30]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x30, 8) , (register, 0x20, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x18, 8) COPY (unique, 0x23e00, 8)']`
  - `0x00000001400018DA` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400018DE` `POP RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x38, 8) COPY (unique, 0xa7200, 8)']`
  - `0x00000001400018DF` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x00000001400018B0` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400018C0` -> `0x0000000140001810` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x00000001400018C5` -> `0x0000000140002128` type=`DATA` source=`ANALYSIS`
  - `0x00000001400018D5` -> `0x0000000000000008` type=`READ` source=`ANALYSIS`
- Callers:
  - `0x00000001400040A8` -> `0x00000001400018B0` type=`DATA`
  - `0x000000014000156C` -> `0x00000001400018B0` type=`UNCONDITIONAL_CALL`
### 37. `TextureAsset` at `0x00000001400018E0`
- Body: `0x00000001400018E0-0x000000014000191F`
- Comment: ``
- Signature: `TextureAsset * __thiscall TextureAsset(TextureAsset * this, uint param_1, uint param_2, uint param_3); return=TextureAsset *; convention=__thiscall; parameters=['this:TextureAsset *', 'param_1:uint', 'param_2:uint', 'param_3:uint']`
- Stack frame: `frame=40, local=40, parameters=4,  variables=['this:TextureAsset *:RCX:8 (auto)', 'param_1:uint:EDX:4', 'param_2:uint:R8D:4', 'param_3:uint:R9D:4']`
- Instructions:
  - `0x00000001400018E0` `MOV qword ptr [RSP + 0x8],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400018E5` `MOV qword ptr [RSP + 0x10],RSI` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x10, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x30, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400018EA` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x00000001400018EB` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400018EF` `MOV ESI,R9D` flow=`FALL_THROUGH` pcode=`['(register, 0x30, 4) COPY (register, 0x88, 4)', '(register, 0x30, 8) INT_ZEXT (register, 0x30, 4)']`
  - `0x00000001400018F2` `MOV EBX,R8D` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 4) COPY (register, 0x80, 4)', '(register, 0x18, 8) INT_ZEXT (register, 0x18, 4)']`
  - `0x00000001400018F5` `MOV RDI,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 8) COPY (register, 0x8, 8)']`
  - `0x00000001400018F8` `CALL 0x140001810` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x1400018fd, 8)', ' ---  CALL (ram, 0x140001810, 8)']`
  - `0x00000001400018FD` `LEA RAX,[0x140002120]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002120, 8)']`
  - `0x0000000140001904` `MOV dword ptr [RDI + 0x10],EBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x38, 8) , (const, 0x10, 8)', '(unique, 0xd400, 4) COPY (register, 0x18, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x0000000140001907` `MOV RBX,qword ptr [RSP + 0x30]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x30, 8) , (register, 0x20, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x18, 8) COPY (unique, 0x23e00, 8)']`
  - `0x000000014000190C` `MOV qword ptr [RDI],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x38, 8) , (unique, 0xd500, 8)']`
  - `0x000000014000190F` `MOV RAX,RDI` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x38, 8)']`
  - `0x0000000140001912` `MOV dword ptr [RDI + 0x14],ESI` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x38, 8) , (const, 0x14, 8)', '(unique, 0xd400, 4) COPY (register, 0x30, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x0000000140001915` `MOV RSI,qword ptr [RSP + 0x38]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x38, 8) , (register, 0x20, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x30, 8) COPY (unique, 0x23e00, 8)']`
  - `0x000000014000191A` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000191E` `POP RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x38, 8) COPY (unique, 0xa7200, 8)']`
  - `0x000000014000191F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x00000001400018E0` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400018E5` -> `0x0000000000000010` type=`WRITE` source=`ANALYSIS`
  - `0x00000001400018F8` -> `0x0000000140001810` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x00000001400018FD` -> `0x0000000140002120` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001907` -> `0x0000000000000008` type=`READ` source=`ANALYSIS`
  - `0x0000000140001915` -> `0x0000000000000010` type=`READ` source=`ANALYSIS`
- Callers:
  - `0x00000001400040B4` -> `0x00000001400018E0` type=`DATA`
  - `0x0000000140001557` -> `0x00000001400018E0` type=`UNCONDITIONAL_CALL`
### 38. `Vehicle` at `0x0000000140001920`
- Body: `0x0000000140001920-0x000000014000194F`
- Comment: ``
- Signature: `Vehicle * __thiscall Vehicle(Vehicle * this, int param_1, char * param_2, int param_3); return=Vehicle *; convention=__thiscall; parameters=['this:Vehicle *', 'param_1:int', 'param_2:char *', 'param_3:int']`
- Stack frame: `frame=40, local=40, parameters=4,  variables=['this:Vehicle *:RCX:8 (auto)', 'param_1:int:EDX:4', 'param_2:char *:R8:8', 'param_3:int:R9D:4']`
- Instructions:
  - `0x0000000140001920` `MOV qword ptr [RSP + 0x8],RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x8, 8) , (register, 0x20, 8)', '(unique, 0xd500, 8) COPY (register, 0x18, 8)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x9d00, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001925` `PUSH RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x38, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001926` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000192A` `MOV EDI,R9D` flow=`FALL_THROUGH` pcode=`['(register, 0x38, 4) COPY (register, 0x88, 4)', '(register, 0x38, 8) INT_ZEXT (register, 0x38, 4)']`
  - `0x000000014000192D` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001930` `CALL 0x140001830` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001935, 8)', ' ---  CALL (ram, 0x140001830, 8)']`
  - `0x0000000140001935` `LEA RAX,[0x1400020d0]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020d0, 8)']`
  - `0x000000014000193C` `MOV dword ptr [RBX + 0x18],EDI` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) COPY (register, 0x38, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000193F` `MOV qword ptr [RBX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x18, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001942` `MOV RAX,RBX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (register, 0x18, 8)']`
  - `0x0000000140001945` `MOV RBX,qword ptr [RSP + 0x30]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9d00, 8) INT_ADD (const, 0x30, 8) , (register, 0x20, 8)', '(unique, 0x23e00, 8) LOAD (const, 0x1b1, 4) , (unique, 0x9d00, 8)', '(register, 0x18, 8) COPY (unique, 0x23e00, 8)']`
  - `0x000000014000194A` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x000000014000194E` `POP RDI` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x38, 8) COPY (unique, 0xa7200, 8)']`
  - `0x000000014000194F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001920` -> `0x0000000000000008` type=`WRITE` source=`ANALYSIS`
  - `0x0000000140001930` -> `0x0000000140001830` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
  - `0x0000000140001935` -> `0x00000001400020D0` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001945` -> `0x0000000000000008` type=`READ` source=`ANALYSIS`
- Callers:
  - `0x00000001400040C0` -> `0x0000000140001920` type=`DATA`
  - `0x000000014000151C` -> `0x0000000140001920` type=`UNCONDITIONAL_CALL`
### 39. `~Asset` at `0x0000000140001950`
- Body: `0x0000000140001950-0x000000014000195A`
- Comment: ``
- Signature: `void __thiscall ~Asset(Asset * this); return=void; convention=__thiscall; parameters=['this:Asset *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Asset *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001950` `LEA RAX,[0x140002118]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002118, 8)']`
  - `0x0000000140001957` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x000000014000195A` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001950` -> `0x0000000140002118` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x00000001400019B0` -> `0x0000000140001950` type=`UNCONDITIONAL_JUMP`
  - `0x00000001400019C0` -> `0x0000000140001950` type=`UNCONDITIONAL_JUMP`
### 40. `~Entity` at `0x0000000140001960`
- Body: `0x0000000140001960-0x000000014000196A`
- Comment: ``
- Signature: `void __thiscall ~Entity(Entity * this); return=void; convention=__thiscall; parameters=['this:Entity *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Entity *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001960` `LEA RAX,[0x1400020b8]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020b8, 8)']`
  - `0x0000000140001967` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x000000014000196A` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001960` -> `0x00000001400020B8` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x0000000140001981` -> `0x0000000140001960` type=`UNCONDITIONAL_CALL`
  - `0x00000001400019E1` -> `0x0000000140001960` type=`UNCONDITIONAL_CALL`
### 41. `~Ped` at `0x0000000140001970`
- Body: `0x0000000140001970-0x0000000140001985`
- Comment: ``
- Signature: `void __thiscall ~Ped(Ped * this); return=void; convention=__thiscall; parameters=['this:Ped *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Ped *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001970` `LEA RAX,[0x1400020e8]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020e8, 8)']`
  - `0x0000000140001977` `MOV dword ptr [RCX + 0x18],0x0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) COPY (const, 0x0, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000197E` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x0000000140001981` `JMP 0x140001960` flow=`CALL_TERMINATOR` pcode=`[' ---  BRANCH (ram, 0x140001960, 8)']`
- References:
  - `0x0000000140001970` -> `0x00000001400020E8` type=`DATA` source=`ANALYSIS`
  - `0x0000000140001981` -> `0x0000000140001960` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400019A1` -> `0x0000000140001970` type=`UNCONDITIONAL_CALL`
### 42. `~Player` at `0x0000000140001990`
- Body: `0x0000000140001990-0x00000001400019A5`
- Comment: ``
- Signature: `void __thiscall ~Player(Player * this); return=void; convention=__thiscall; parameters=['this:Player *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Player *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001990` `LEA RAX,[0x140002100]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002100, 8)']`
  - `0x0000000140001997` `MOV dword ptr [RCX + 0x20],0x0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x20, 8)', '(unique, 0xd400, 4) COPY (const, 0x0, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x000000014000199E` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400019A1` `JMP 0x140001970` flow=`CALL_TERMINATOR` pcode=`[' ---  BRANCH (ram, 0x140001970, 8)']`
- References:
  - `0x0000000140001990` -> `0x0000000140002100` type=`DATA` source=`ANALYSIS`
  - `0x00000001400019A1` -> `0x0000000140001970` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400016FA` -> `0x0000000140001990` type=`UNCONDITIONAL_CALL`
### 43. `~ScriptAsset` at `0x00000001400019B0`
- Body: `0x00000001400019B0-0x00000001400019B4`
- Comment: ``
- Signature: `void __thiscall ~ScriptAsset(ScriptAsset * this); return=void; convention=__thiscall; parameters=['this:ScriptAsset *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:ScriptAsset *:RCX:8 (auto)']`
- Instructions:
  - `0x00000001400019B0` `JMP 0x140001950` flow=`UNCONDITIONAL_JUMP` pcode=`[' ---  BRANCH (ram, 0x140001950, 8)']`
- References:
  - `0x00000001400019B0` -> `0x0000000140001950` type=`UNCONDITIONAL_JUMP` source=`DEFAULT`
- Callers:
  - `0x00000001400016E3` -> `0x00000001400019B0` type=`UNCONDITIONAL_CALL`
### 44. `~TextureAsset` at `0x00000001400019C0`
- Body: `0x00000001400019C0-0x00000001400019C4`
- Comment: ``
- Signature: `void __thiscall ~TextureAsset(TextureAsset * this); return=void; convention=__thiscall; parameters=['this:TextureAsset *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:TextureAsset *:RCX:8 (auto)']`
- Instructions:
  - `0x00000001400019C0` `JMP 0x140001950` flow=`UNCONDITIONAL_JUMP` pcode=`[' ---  BRANCH (ram, 0x140001950, 8)']`
- References:
  - `0x00000001400019C0` -> `0x0000000140001950` type=`UNCONDITIONAL_JUMP` source=`DEFAULT`
- Callers:
  - `0x00000001400016ED` -> `0x00000001400019C0` type=`UNCONDITIONAL_CALL`
### 45. `~Vehicle` at `0x00000001400019D0`
- Body: `0x00000001400019D0-0x00000001400019E5`
- Comment: ``
- Signature: `void __thiscall ~Vehicle(Vehicle * this); return=void; convention=__thiscall; parameters=['this:Vehicle *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Vehicle *:RCX:8 (auto)']`
- Instructions:
  - `0x00000001400019D0` `LEA RAX,[0x1400020d0]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020d0, 8)']`
  - `0x00000001400019D7` `MOV dword ptr [RCX + 0x18],0x0` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) COPY (const, 0x0, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x00000001400019DE` `MOV qword ptr [RCX],RAX` flow=`FALL_THROUGH` pcode=`['(unique, 0xd500, 8) COPY (register, 0x0, 8)', ' ---  STORE (const, 0x1b1, 4) , (register, 0x8, 8) , (unique, 0xd500, 8)']`
  - `0x00000001400019E1` `JMP 0x140001960` flow=`CALL_TERMINATOR` pcode=`[' ---  BRANCH (ram, 0x140001960, 8)']`
- References:
  - `0x00000001400019D0` -> `0x00000001400020D0` type=`DATA` source=`ANALYSIS`
  - `0x00000001400019E1` -> `0x0000000140001960` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x0000000140001704` -> `0x00000001400019D0` type=`UNCONDITIONAL_CALL`
### 46. `checksum` at `0x00000001400019F0`
- Body: `0x00000001400019F0-0x00000001400019F9`
- Comment: ``
- Signature: `uint __thiscall checksum(ScriptAsset * this, uint param_1); return=uint; convention=__thiscall; parameters=['this:ScriptAsset *', 'param_1:uint']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:ScriptAsset *:RCX:8 (auto)', 'param_1:uint:EDX:4']`
- Instructions:
  - `0x00000001400019F0` `IMUL EAX,dword ptr [RCX + 0x10],0x11` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x10, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x93a00, 8) INT_SEXT (unique, 0x23d00, 4)', '(unique, 0x93b00, 8) INT_SEXT (const, 0x11, 4)', '(unique, 0x93d00, 8) INT_MULT (unique, 0x93a00, 8) , (unique, 0x93b00, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x93d00, 8) , (const, 0x0, 4)', '(unique, 0x94000, 4) SUBPIECE (unique, 0x93d00, 8) , (const, 0x4, 4)', '(unique, 0x5a200, 8) INT_SEXT (register, 0x0, 4)', '(register, 0x200, 1) INT_NOTEQUAL (unique, 0x5a200, 8) , (unique, 0x93d00, 8)', '(register, 0x20b, 1) COPY (register, 0x200, 1)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x00000001400019F4` `XOR EDX,dword ptr [RCX + 0x8]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x10, 4) INT_XOR (register, 0x10, 4) , (unique, 0x23d00, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x10, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x10, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x10, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400019F7` `ADD EAX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x00000001400019F9` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x00000001400015DD` -> `0x00000001400019F0` type=`UNCONDITIONAL_CALL`
### 47. `damage` at `0x0000000140001A00`
- Body: `0x0000000140001A00-0x0000000140001A03`
- Comment: ``
- Signature: `void __thiscall damage(Ped * this, int param_1); return=void; convention=__thiscall; parameters=['this:Ped *', 'param_1:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Ped *:RCX:8 (auto)', 'param_1:int:EDX:4']`
- Instructions:
  - `0x0000000140001A00` `SUB dword ptr [RCX + 0x18],EDX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_LESS (unique, 0xd400, 4) , (register, 0x10, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SBORROW (unique, 0xd400, 4) , (register, 0x10, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0xd400, 4) INT_SUB (unique, 0xd400, 4) , (register, 0x10, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x207, 1) INT_SLESS (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x58300, 4) INT_AND (unique, 0xd400, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001A03` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x00000001400015CC` -> `0x0000000140001A00` type=`UNCONDITIONAL_CALL`
### 48. `kind` at `0x0000000140001A10`
- Body: `0x0000000140001A10-0x0000000140001A17`
- Comment: ``
- Signature: `char * __thiscall kind(Entity * this); return=char *; convention=__thiscall; parameters=['this:Entity *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Entity *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001A10` `LEA RAX,[0x1400020c8]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020c8, 8)']`
  - `0x0000000140001A17` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001A10` -> `0x00000001400020C8` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x00000001400020C0` -> `0x0000000140001A10` type=`DATA`
### 49. `kind` at `0x0000000140001A20`
- Body: `0x0000000140001A20-0x0000000140001A27`
- Comment: ``
- Signature: `char * __thiscall kind(Ped * this); return=char *; convention=__thiscall; parameters=['this:Ped *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Ped *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001A20` `LEA RAX,[0x1400020f8]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020f8, 8)']`
  - `0x0000000140001A27` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001A20` -> `0x00000001400020F8` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x00000001400020F0` -> `0x0000000140001A20` type=`DATA`
### 50. `kind` at `0x0000000140001A30`
- Body: `0x0000000140001A30-0x0000000140001A37`
- Comment: ``
- Signature: `char * __thiscall kind(Player * this); return=char *; convention=__thiscall; parameters=['this:Player *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Player *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001A30` `LEA RAX,[0x140002110]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x140002110, 8)']`
  - `0x0000000140001A37` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001A30` -> `0x0000000140002110` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x0000000140002108` -> `0x0000000140001A30` type=`DATA`
### 51. `kind` at `0x0000000140001A40`
- Body: `0x0000000140001A40-0x0000000140001A47`
- Comment: ``
- Signature: `char * __thiscall kind(Vehicle * this); return=char *; convention=__thiscall; parameters=['this:Vehicle *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Vehicle *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001A40` `LEA RAX,[0x1400020e0]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 8) COPY (const, 0x1400020e0, 8)']`
  - `0x0000000140001A47` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001A40` -> `0x00000001400020E0` type=`DATA` source=`ANALYSIS`
- Callers:
  - `0x00000001400020D8` -> `0x0000000140001A40` type=`DATA`
### 52. `refuel` at `0x0000000140001A50`
- Body: `0x0000000140001A50-0x0000000140001A53`
- Comment: ``
- Signature: `void __thiscall refuel(Vehicle * this, int param_1); return=void; convention=__thiscall; parameters=['this:Vehicle *', 'param_1:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Vehicle *:RCX:8 (auto)', 'param_1:int:EDX:4']`
- Instructions:
  - `0x0000000140001A50` `ADD dword ptr [RCX + 0x18],EDX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (unique, 0xd400, 4) , (register, 0x10, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (unique, 0xd400, 4) , (register, 0x10, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0xd400, 4) INT_ADD (unique, 0xd400, 4) , (register, 0x10, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x207, 1) INT_SLESS (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x58300, 4) INT_AND (unique, 0xd400, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001A53` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x00000001400015A6` -> `0x0000000140001A50` type=`UNCONDITIONAL_CALL`
### 53. `select_weapon` at `0x0000000140001A60`
- Body: `0x0000000140001A60-0x0000000140001A63`
- Comment: ``
- Signature: `void __thiscall select_weapon(Player * this, uint param_1); return=void; convention=__thiscall; parameters=['this:Player *', 'param_1:uint']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Player *:RCX:8 (auto)', 'param_1:uint:EDX:4']`
- Instructions:
  - `0x0000000140001A60` `MOV dword ptr [RCX + 0x20],EDX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x20, 8)', '(unique, 0xd400, 4) COPY (register, 0x10, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)']`
  - `0x0000000140001A63` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x00000001400015B8` -> `0x0000000140001A60` type=`UNCONDITIONAL_CALL`
### 54. `size` at `0x0000000140001A70`
- Body: `0x0000000140001A70-0x0000000140001A74`
- Comment: ``
- Signature: `uint __thiscall size(Asset * this); return=uint; convention=__thiscall; parameters=['this:Asset *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:Asset *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001A70` `MOVZX EAX,byte ptr [RCX + 0x8]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(unique, 0x23b00, 1) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_ZEXT (unique, 0x23b00, 1)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001A74` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
- Callers:
  - `0x0000000140001A89` -> `0x0000000140001A70` type=`UNCONDITIONAL_CALL`
  - `0x0000000140001AA9` -> `0x0000000140001A70` type=`UNCONDITIONAL_CALL`
  - `0x0000000140002118` -> `0x0000000140001A70` type=`DATA`
### 55. `size` at `0x0000000140001A80`
- Body: `0x0000000140001A80-0x0000000140001A96`
- Comment: ``
- Signature: `uint __thiscall size(ScriptAsset * this); return=uint; convention=__thiscall; parameters=['this:ScriptAsset *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:ScriptAsset *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001A80` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001A82` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001A86` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001A89` `CALL 0x140001a70` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001a8e, 8)', ' ---  CALL (ram, 0x140001a70, 8)']`
  - `0x0000000140001A8E` `ADD EAX,dword ptr [RBX + 0x10]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x10, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001A91` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001A95` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001A96` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001A89` -> `0x0000000140001A70` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400040CC` -> `0x0000000140001A80` type=`DATA`
  - `0x00000001400015F7` -> `0x0000000140001A80` type=`UNCONDITIONAL_CALL`
  - `0x0000000140002128` -> `0x0000000140001A80` type=`DATA`
### 56. `size` at `0x0000000140001AA0`
- Body: `0x0000000140001AA0-0x0000000140001ABD`
- Comment: ``
- Signature: `uint __thiscall size(TextureAsset * this); return=uint; convention=__thiscall; parameters=['this:TextureAsset *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['this:TextureAsset *:RCX:8 (auto)']`
- Instructions:
  - `0x0000000140001AA0` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001AA2` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AA6` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001AA9` `CALL 0x140001a70` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001aae, 8)', ' ---  CALL (ram, 0x140001a70, 8)']`
  - `0x0000000140001AAE` `MOV EDX,dword ptr [RBX + 0x14]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x14, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x10, 4) COPY (unique, 0x23d00, 4)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)']`
  - `0x0000000140001AB1` `IMUL EDX,dword ptr [RBX + 0x10]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x10, 8)', '(unique, 0x92500, 8) INT_SEXT (register, 0x10, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x92600, 8) INT_SEXT (unique, 0x23d00, 4)', '(unique, 0x92800, 8) INT_MULT (unique, 0x92500, 8) , (unique, 0x92600, 8)', '(register, 0x10, 4) SUBPIECE (unique, 0x92800, 8) , (const, 0x0, 4)', '(unique, 0x92b00, 4) SUBPIECE (unique, 0x92800, 8) , (const, 0x4, 4)', '(unique, 0x5a200, 8) INT_SEXT (register, 0x10, 4)', '(register, 0x200, 1) INT_NOTEQUAL (unique, 0x5a200, 8) , (unique, 0x92800, 8)', '(register, 0x20b, 1) COPY (register, 0x200, 1)', '(register, 0x10, 8) INT_ZEXT (register, 0x10, 4)']`
  - `0x0000000140001AB5` `LEA EAX,[RAX + RDX*0x4]` flow=`FALL_THROUGH` pcode=`['(unique, 0x9300, 8) INT_MULT (register, 0x10, 8) , (const, 0x4, 8)', '(unique, 0x9500, 8) INT_ADD (register, 0x0, 8) , (unique, 0x9300, 8)', '(register, 0x0, 4) SUBPIECE (unique, 0x9500, 8) , (const, 0x0, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001AB8` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001ABC` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001ABD` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001AA9` -> `0x0000000140001A70` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400040D8` -> `0x0000000140001AA0` type=`DATA`
  - `0x00000001400015EA` -> `0x0000000140001AA0` type=`UNCONDITIONAL_CALL`
  - `0x0000000140002120` -> `0x0000000140001AA0` type=`DATA`
### 57. `update` at `0x0000000140001AC0`
- Body: `0x0000000140001AC0-0x0000000140001AD6`
- Comment: ``
- Signature: `int __thiscall update(Entity * this, int param_1); return=int; convention=__thiscall; parameters=['this:Entity *', 'param_1:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Entity *:RCX:8 (auto)', 'param_1:int:EDX:4']`
- Instructions:
  - `0x0000000140001AC0` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001AC6` `ADD EAX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AC8` `ADD EAX,dword ptr [RCX + 0x8]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001ACB` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x0000000140001AD1` `MOV EAX,dword ptr [RCX + 0x8]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) COPY (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001AD4` `ADD EAX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AD6` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001AC0` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001ACB` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
- Callers:
  - `0x0000000140001AF4` -> `0x0000000140001AC0` type=`UNCONDITIONAL_CALL`
  - `0x0000000140001B63` -> `0x0000000140001AC0` type=`UNCONDITIONAL_CALL`
  - `0x00000001400020B8` -> `0x0000000140001AC0` type=`DATA`
### 58. `update` at `0x0000000140001AE0`
- Body: `0x0000000140001AE0-0x0000000140001B01`
- Comment: ``
- Signature: `int __thiscall update(Ped * this, int param_1); return=int; convention=__thiscall; parameters=['this:Ped *', 'param_1:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Ped *:RCX:8 (auto)', 'param_1:int:EDX:4']`
- Instructions:
  - `0x0000000140001AE0` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001AE2` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AE6` `MOV EAX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (register, 0x10, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001AE8` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001AEB` `XOR EAX,dword ptr [RCX + 0x8]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x8, 8)', '(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_XOR (register, 0x0, 4) , (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AEE` `AND EAX,0x7` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x0, 4) INT_AND (register, 0x0, 4) , (const, 0x7, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AF1` `ADD dword ptr [RCX + 0x18],EAX` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (unique, 0xd400, 4) , (register, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (unique, 0xd400, 4) , (register, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0xd400, 4) INT_ADD (unique, 0xd400, 4) , (register, 0x0, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x207, 1) INT_SLESS (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x58300, 4) INT_AND (unique, 0xd400, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AF4` `CALL 0x140001ac0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001af9, 8)', ' ---  CALL (ram, 0x140001ac0, 8)']`
  - `0x0000000140001AF9` `ADD EAX,dword ptr [RBX + 0x18]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x18, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001AFC` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B00` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001B01` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001AF4` -> `0x0000000140001AC0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400040E4` -> `0x0000000140001AE0` type=`DATA`
  - `0x0000000140001B2F` -> `0x0000000140001AE0` type=`UNCONDITIONAL_CALL`
  - `0x00000001400020E8` -> `0x0000000140001AE0` type=`DATA`
### 59. `update` at `0x0000000140001B10`
- Body: `0x0000000140001B10-0x0000000140001B3F`
- Comment: ``
- Signature: `int __thiscall update(Player * this, int param_1); return=int; convention=__thiscall; parameters=['this:Player *', 'param_1:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Player *:RCX:8 (auto)', 'param_1:int:EDX:4']`
- Instructions:
  - `0x0000000140001B10` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001B12` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B16` `MOV EAX,dword ptr [RCX + 0x20]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x20, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) COPY (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001B19` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001B1C` `MOV R8,qword ptr [0x140003030]` flow=`FALL_THROUGH` pcode=`['(register, 0x80, 8) COPY (ram, 0x140003030, 8)']`
  - `0x0000000140001B23` `ADD EAX,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x10, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B25` `XOR R8,RAX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x80, 8) INT_XOR (register, 0x80, 8) , (register, 0x0, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x80, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x80, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x80, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B28` `MOV qword ptr [0x140003030],R8` flow=`FALL_THROUGH` pcode=`['(ram, 0x140003030, 8) COPY (register, 0x80, 8)']`
  - `0x0000000140001B2F` `CALL 0x140001ae0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001b34, 8)', ' ---  CALL (ram, 0x140001ae0, 8)']`
  - `0x0000000140001B34` `MOVZX ECX,byte ptr [RBX + 0x20]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x20, 8)', '(unique, 0x23b00, 1) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x8, 4) INT_ZEXT (unique, 0x23b00, 1)', '(register, 0x8, 8) INT_ZEXT (register, 0x8, 4)']`
  - `0x0000000140001B38` `ADD EAX,ECX` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (register, 0x8, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B3A` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B3E` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001B3F` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001B1C` -> `0x0000000140003030` type=`READ` source=`DEFAULT`
  - `0x0000000140001B28` -> `0x0000000140003030` type=`WRITE` source=`DEFAULT`
  - `0x0000000140001B2F` -> `0x0000000140001AE0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400040F0` -> `0x0000000140001B10` type=`DATA`
  - `0x0000000140002100` -> `0x0000000140001B10` type=`DATA`
### 60. `update` at `0x0000000140001B40`
- Body: `0x0000000140001B40-0x0000000140001B70`
- Comment: ``
- Signature: `int __thiscall update(Vehicle * this, int param_1); return=int; convention=__thiscall; parameters=['this:Vehicle *', 'param_1:int']`
- Stack frame: `frame=40, local=40, parameters=2,  variables=['this:Vehicle *:RCX:8 (auto)', 'param_1:int:EDX:4']`
- Instructions:
  - `0x0000000140001B40` `PUSH RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0x4f900, 8) COPY (register, 0x18, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (unique, 0x4f900, 8)']`
  - `0x0000000140001B42` `SUB RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_LESS (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SBORROW (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B46` `MOV R8D,EDX` flow=`FALL_THROUGH` pcode=`['(register, 0x80, 4) COPY (register, 0x10, 4)', '(register, 0x80, 8) INT_ZEXT (register, 0x80, 4)']`
  - `0x0000000140001B49` `MOV RBX,RCX` flow=`FALL_THROUGH` pcode=`['(register, 0x18, 8) COPY (register, 0x8, 8)']`
  - `0x0000000140001B4C` `AND R8D,0x3` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) COPY (const, 0x0, 1)', '(register, 0x20b, 1) COPY (const, 0x0, 1)', '(register, 0x80, 4) INT_AND (register, 0x80, 4) , (const, 0x3, 4)', '(register, 0x80, 8) INT_ZEXT (register, 0x80, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x80, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x80, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x80, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B50` `SUB dword ptr [RCX + 0x18],R8D` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_LESS (unique, 0xd400, 4) , (register, 0x80, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SBORROW (unique, 0xd400, 4) , (register, 0x80, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0xd400, 4) INT_SUB (unique, 0xd400, 4) , (register, 0x80, 4)', ' ---  STORE (const, 0x1b1, 4) , (unique, 0x8f00, 8) , (unique, 0xd400, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x207, 1) INT_SLESS (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x206, 1) INT_EQUAL (unique, 0xd400, 4) , (const, 0x0, 4)', '(unique, 0xd400, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(unique, 0x58300, 4) INT_AND (unique, 0xd400, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B54` `MOV EAX,dword ptr [0x14000302c]` flow=`FALL_THROUGH` pcode=`['(register, 0x0, 4) COPY (ram, 0x14000302c, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)']`
  - `0x0000000140001B5A` `ADD EAX,dword ptr [RCX + 0x18]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x8, 8) , (const, 0x18, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B5D` `MOV dword ptr [0x14000302c],EAX` flow=`FALL_THROUGH` pcode=`['(ram, 0x14000302c, 4) COPY (register, 0x0, 4)']`
  - `0x0000000140001B63` `CALL 0x140001ac0` flow=`UNCONDITIONAL_CALL` pcode=`['(register, 0x20, 8) INT_SUB (register, 0x20, 8) , (const, 0x8, 8)', ' ---  STORE (const, 0x1b1, 8) , (register, 0x20, 8) , (const, 0x140001b68, 8)', ' ---  CALL (ram, 0x140001ac0, 8)']`
  - `0x0000000140001B68` `ADD EAX,dword ptr [RBX + 0x18]` flow=`FALL_THROUGH` pcode=`['(unique, 0x8f00, 8) INT_ADD (register, 0x18, 8) , (const, 0x18, 8)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x200, 1) INT_CARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x0, 4) , (unique, 0x23d00, 4)', '(unique, 0x23d00, 4) LOAD (const, 0x1b1, 4) , (unique, 0x8f00, 8)', '(register, 0x0, 4) INT_ADD (register, 0x0, 4) , (unique, 0x23d00, 4)', '(register, 0x0, 8) INT_ZEXT (register, 0x0, 4)', '(register, 0x207, 1) INT_SLESS (register, 0x0, 4) , (const, 0x0, 4)', '(register, 0x206, 1) INT_EQUAL (register, 0x0, 4) , (const, 0x0, 4)', '(unique, 0x58300, 4) INT_AND (register, 0x0, 4) , (const, 0xff, 4)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 4)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B6B` `ADD RSP,0x20` flow=`FALL_THROUGH` pcode=`['(register, 0x200, 1) INT_CARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20b, 1) INT_SCARRY (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x20, 8)', '(register, 0x207, 1) INT_SLESS (register, 0x20, 8) , (const, 0x0, 8)', '(register, 0x206, 1) INT_EQUAL (register, 0x20, 8) , (const, 0x0, 8)', '(unique, 0x58300, 8) INT_AND (register, 0x20, 8) , (const, 0xff, 8)', '(unique, 0x58400, 1) POPCOUNT (unique, 0x58300, 8)', '(unique, 0x58500, 1) INT_AND (unique, 0x58400, 1) , (const, 0x1, 1)', '(register, 0x202, 1) INT_EQUAL (unique, 0x58500, 1) , (const, 0x0, 1)']`
  - `0x0000000140001B6F` `POP RBX` flow=`FALL_THROUGH` pcode=`['(unique, 0xa7200, 8) COPY (const, 0x0, 8)', '(unique, 0xa7200, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', '(register, 0x18, 8) COPY (unique, 0xa7200, 8)']`
  - `0x0000000140001B70` `RET` flow=`TERMINATOR` pcode=`['(register, 0x288, 8) LOAD (const, 0x1b1, 8) , (register, 0x20, 8)', '(register, 0x20, 8) INT_ADD (register, 0x20, 8) , (const, 0x8, 8)', ' ---  RETURN (register, 0x288, 8)']`
- References:
  - `0x0000000140001B54` -> `0x000000014000302C` type=`READ` source=`DEFAULT`
  - `0x0000000140001B5D` -> `0x000000014000302C` type=`WRITE` source=`DEFAULT`
  - `0x0000000140001B63` -> `0x0000000140001AC0` type=`UNCONDITIONAL_CALL` source=`DEFAULT`
- Callers:
  - `0x00000001400040FC` -> `0x0000000140001B40` type=`DATA`
  - `0x00000001400020D0` -> `0x0000000140001B40` type=`DATA`
### 61. `OutputDebugStringA` at `0x0000000140001B71`
- Body: `0x0000000140001B71-0x0000000140001B76`
- Comment: ``
- Signature: `void __stdcall OutputDebugStringA(LPCSTR lpOutputString); return=void; convention=__stdcall; parameters=['lpOutputString:typedef LPCSTR CHAR *']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['lpOutputString:typedef LPCSTR CHAR *:RCX:8']`
- Instructions:
  - `0x0000000140001B71` `JMP qword ptr [0x140002010]` flow=`COMPUTED_JUMP` pcode=`[' ---  BRANCHIND (ram, 0x140002010, 8)']`
- References:
  - `0x0000000140001B71` -> `0x0000000140002010` type=`INDIRECTION` source=`DEFAULT`
  - `0x0000000140001B71` -> `0x0000000000000003` type=`COMPUTED_JUMP` source=`ANALYSIS`
- Callers:
### 62. `GetTickCount` at `0x0000000140001B77`
- Body: `0x0000000140001B77-0x0000000140001B7C`
- Comment: ``
- Signature: `DWORD __stdcall GetTickCount(void); return=typedef DWORD ulong; convention=__stdcall; parameters=[]`
- Stack frame: `frame=40, local=40, parameters=0,  variables=[]`
- Instructions:
  - `0x0000000140001B77` `JMP qword ptr [0x140002000]` flow=`COMPUTED_JUMP` pcode=`[' ---  BRANCHIND (ram, 0x140002000, 8)']`
- References:
  - `0x0000000140001B77` -> `0x0000000140002000` type=`INDIRECTION` source=`DEFAULT`
  - `0x0000000140001B77` -> `0x0000000000000001` type=`COMPUTED_JUMP` source=`ANALYSIS`
- Callers:
### 63. `ExitProcess` at `0x0000000140001B7D`
- Body: `0x0000000140001B7D-0x0000000140001B82`
- Comment: ``
- Signature: `noreturn void __stdcall ExitProcess(UINT uExitCode); return=void; convention=__stdcall; parameters=['uExitCode:typedef UINT uint']`
- Stack frame: `frame=40, local=40, parameters=1,  variables=['uExitCode:typedef UINT uint:ECX:4']`
- Instructions:
  - `0x0000000140001B7D` `JMP qword ptr [0x140002008]` flow=`COMPUTED_JUMP` pcode=`[' ---  BRANCHIND (ram, 0x140002008, 8)']`
- References:
  - `0x0000000140001B7D` -> `0x0000000140002008` type=`INDIRECTION` source=`DEFAULT`
  - `0x0000000140001B7D` -> `0x0000000000000002` type=`COMPUTED_JUMP` source=`ANALYSIS`
- Callers:
### 64. `LoadStringW` at `0x0000000140001B83`
- Body: `0x0000000140001B83-0x0000000140001B88`
- Comment: ``
- Signature: `int __stdcall LoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int cchBufferMax); return=int; convention=__stdcall; parameters=['hInstance:typedef HINSTANCE HINSTANCE__ *', 'uID:typedef UINT uint', 'lpBuffer:typedef LPWSTR WCHAR *', 'cchBufferMax:int']`
- Stack frame: `frame=40, local=40, parameters=4,  variables=['hInstance:typedef HINSTANCE HINSTANCE__ *:RCX:8', 'uID:typedef UINT uint:EDX:4', 'lpBuffer:typedef LPWSTR WCHAR *:R8:8', 'cchBufferMax:int:R9D:4']`
- Instructions:
  - `0x0000000140001B83` `JMP qword ptr [0x140002020]` flow=`COMPUTED_JUMP` pcode=`[' ---  BRANCHIND (ram, 0x140002020, 8)']`
- References:
  - `0x0000000140001B83` -> `0x0000000140002020` type=`INDIRECTION` source=`DEFAULT`
  - `0x0000000140001B83` -> `0x0000000000000004` type=`COMPUTED_JUMP` source=`ANALYSIS`
- Callers:

## Strings And Data

| Address | Length | Type | Value |
| --- | ---: | --- | --- |
| `0000000140000000` | `128` | `/DOS/IMAGE_DOS_HEADER<br>pack(disabled)<br>Structure IMAGE_DOS_HEADER {<br>   0   char[2]   2   e_magic   "Magic number"<br>   2   word   2   e_cblp   "Bytes of last page"<br>   4   word   2   e_cp   "Pages in file"<br>   6   word   2   e_crlc   "Relocations"<br>   8   word   2   e_cparhdr   "Size of header in paragraphs"<br>   10   word   2   e_minalloc   "Minimum extra paragraphs needed"<br>   12   word   2   e_maxalloc   "Maximum extra paragraphs needed"<br>   14   word   2   e_ss   "Initial (relative) SS value"<br>   16   word   2   e_sp   "Initial SP value"<br>   18   word   2   e_csum   "Checksum"<br>   20   word   2   e_ip   "Initial IP value"<br>   22   word   2   e_cs   "Initial (relative) CS value"<br>   24   word   2   e_lfarlc   "File address of relocation table"<br>   26   word   2   e_ovno   "Overlay number"<br>   28   word[4]   8   e_res[4]   "Reserved words"<br>   36   word   2   e_oemid   "OEM identifier (for e_oeminfo)"<br>   38   word   2   e_oeminfo   "OEM information; e_oemid specific"<br>   40   word[10]   20   e_res2[10]   "Reserved words"<br>   60   dword   4   e_lfanew   "File address of new exe header"<br>   64   byte[64]   64   e_program   "Actual DOS program"<br>}<br>Length: 128 Alignment: 1<br>` | `None` |
| `00000001400000E0` | `264` | `/PE/IMAGE_NT_HEADERS64<br>pack(disabled)<br>Structure IMAGE_NT_HEADERS64 {<br>   0   char[4]   4   Signature   ""<br>   4   IMAGE_FILE_HEADER   20   FileHeader   ""<br>   24   IMAGE_OPTIONAL_HEADER64   240   OptionalHeader   ""<br>}<br>Length: 264 Alignment: 1<br>` | `None` |
| `00000001400001E8` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0000000140000210` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0000000140000238` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0000000140000260` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0000000140000288` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `00000001400002B0` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0000000140002030` | `45` | `char[45]` | `GTA-like native analysis integration fixture` |
| `0000000140002060` | `27` | `char[27]` | `tick=%d entity=%s score=%u` |
| `000000014000207C` | `5` | `char[5]` | `tiny` |
| `0000000140002088` | `12` | `char[12]` | `unterminated` |
| `0000000140002094` | `7` | `uchar[7]` | `������` |
| `00000001400020C8` | `7` | `TerminatedCString` | `Entity` |
| `00000001400020E0` | `8` | `TerminatedCString` | `Vehicle` |
| `00000001400020F8` | `4` | `TerminatedCString` | `Ped` |
| `0000000140002110` | `7` | `TerminatedCString` | `Player` |
| `0000000140002140` | `28` | `/PE/IMAGE_DEBUG_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_DEBUG_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   dword   4   Type   ""<br>   16   dword   4   SizeOfData   ""<br>   20   dword   4   AddressOfRawData   ""<br>   24   dword   4   PointerToRawData   ""<br>}<br>Length: 28 Alignment: 1<br>` | `None` |
| `000000014000215C` | `28` | `/PE/IMAGE_DEBUG_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_DEBUG_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   dword   4   Type   ""<br>   16   dword   4   SizeOfData   ""<br>   20   dword   4   AddressOfRawData   ""<br>   24   dword   4   PointerToRawData   ""<br>}<br>Length: 28 Alignment: 1<br>` | `None` |
| `0000000140002178` | `28` | `/PE/IMAGE_DEBUG_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_DEBUG_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   dword   4   Type   ""<br>   16   dword   4   SizeOfData   ""<br>   20   dword   4   AddressOfRawData   ""<br>   24   dword   4   PointerToRawData   ""<br>}<br>Length: 28 Alignment: 1<br>` | `None` |
| `0000000140002194` | `28` | `/PE/IMAGE_DEBUG_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_DEBUG_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   dword   4   Type   ""<br>   16   dword   4   SizeOfData   ""<br>   20   dword   4   AddressOfRawData   ""<br>   24   dword   4   PointerToRawData   ""<br>}<br>Length: 28 Alignment: 1<br>` | `None` |
| `00000001400021D8` | `24` | `char *[3]` | `None` |
| `00000001400021F0` | `35` | `uchar[35]` | `GIF89a` |
| `0000000140002220` | `68` | `uchar[68]` | `�PNG<br><br>` |
| `0000000140002268` | `44` | `uchar[44]` | `RIFF$` |
| `0000000140002298` | `26` | `uchar[26]` | `MThd` |
| `00000001400022B8` | `28` | `uchar[28]` | `.snd` |
| `00000001400023EC` | `135` | `/PDB/DotNetPdbInfo<br>pack(disabled)<br>Structure DotNetPdbInfo {<br>   0   string   4   signature   ""<br>   4   GUID   16   guid   ""<br>   20   dword   4   age   ""<br>   24   string   111   pdbpath   ""<br>}<br>Length: 135 Alignment: 1<br>` | `None` |
| `00000001400024CC` | `7` | `string` | `.rdata` |
| `00000001400024DC` | `14` | `string` | `.rdata$engine` |
| `00000001400024F4` | `13` | `string` | `.rdata$media` |
| `000000014000250C` | `14` | `string` | `.rdata$voltmd` |
| `000000014000254C` | `7` | `string` | `.edata` |
| `00000001400025AC` | `6` | `string` | `.data` |
| `00000001400025CC` | `7` | `string` | `.pdata` |
| `0000000140002710` | `40` | `/PE/IMAGE_DIRECTORY_ENTRY_EXPORT<br>pack(disabled)<br>Structure IMAGE_DIRECTORY_ENTRY_EXPORT {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   ImageBaseOffset32   4   Name   ""<br>   16   dword   4   Base   ""<br>   20   dword   4   NumberOfFunctions   ""<br>   24   dword   4   NumberOfNames   ""<br>   28   ImageBaseOffset32   4   AddressOfFunctions   ""<br>   32   ImageBaseOffset32   4   AddressOfNames   ""<br>   36   ImageBaseOffset32   4   AddressOfNameOrdinals   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0000000140002922` | `31` | `TerminatedCString` | `test_analyzers_integration.exe` |
| `0000000140002941` | `44` | `TerminatedCString` | `?add@Calculator@demangle_fixture@@QEAAHHH@Z` |
| `000000014000296D` | `35` | `TerminatedCString` | `?combine@overload_fixture@@YAHHH@Z` |
| `0000000140002990` | `35` | `TerminatedCString` | `?combine@overload_fixture@@YAMMM@Z` |
| `00000001400029B3` | `43` | `TerminatedCString` | `?scale@Calculator@demangle_fixture@@SAHH@Z` |
| `00000001400029DE` | `15` | `TerminatedCString` | `abort_path_one` |
| `00000001400029ED` | `17` | `TerminatedCString` | `abort_path_three` |
| `00000001400029FE` | `15` | `TerminatedCString` | `abort_path_two` |
| `0000000140002A0D` | `13` | `TerminatedCString` | `callback_add` |
| `0000000140002A1A` | `14` | `TerminatedCString` | `callback_mask` |
| `0000000140002A28` | `16` | `TerminatedCString` | `callback_rotate` |
| `0000000140002A38` | `13` | `TerminatedCString` | `engine_abort` |
| `0000000140002A45` | `10` | `TerminatedCString` | `engine_au` |
| `0000000140002A4F` | `14` | `TerminatedCString` | `engine_banner` |
| `0000000140002A5D` | `17` | `TerminatedCString` | `engine_callbacks` |
| `0000000140002A6E` | `14` | `TerminatedCString` | `engine_config` |
| `0000000140002A7C` | `25` | `TerminatedCString` | `engine_difficulty_levels` |
| `0000000140002A95` | `14` | `TerminatedCString` | `engine_format` |
| `0000000140002AA3` | `11` | `TerminatedCString` | `engine_gif` |
| `0000000140002AAE` | `20` | `TerminatedCString` | `engine_global_ticks` |
| `0000000140002AC2` | `12` | `TerminatedCString` | `engine_midi` |
| `0000000140002ACE` | `17` | `TerminatedCString` | `engine_non_ascii` |
| `0000000140002ADF` | `11` | `TerminatedCString` | `engine_png` |
| `0000000140002AEA` | `20` | `TerminatedCString` | `engine_pointer_data` |
| `0000000140002AFE` | `16` | `TerminatedCString` | `engine_selector` |
| `0000000140002B0E` | `13` | `TerminatedCString` | `engine_short` |
| `0000000140002B1B` | `13` | `TerminatedCString` | `engine_state` |
| `0000000140002B28` | `22` | `TerminatedCString` | `engine_table_sentinel` |
| `0000000140002B3E` | `12` | `TerminatedCString` | `engine_tick` |
| `0000000140002B4A` | `20` | `TerminatedCString` | `engine_unterminated` |
| `0000000140002B5E` | `24` | `TerminatedCString` | `engine_unused_dead_code` |
| `0000000140002B76` | `12` | `TerminatedCString` | `engine_wave` |
| `0000000140002B82` | `16` | `TerminatedCString` | `filler_engine_a` |
| `0000000140002B92` | `16` | `TerminatedCString` | `filler_engine_b` |
| `0000000140002BA2` | `16` | `TerminatedCString` | `filler_engine_c` |
| `0000000140002BB2` | `16` | `TerminatedCString` | `filler_engine_d` |
| `0000000140002BC2` | `14` | `TerminatedCString` | `fixture_entry` |
| `0000000140002BD0` | `16` | `TerminatedCString` | `invoke_callback` |
| `0000000140002BE0` | `13` | `TerminatedCString` | `mutual_alpha` |
| `0000000140002BED` | `12` | `TerminatedCString` | `mutual_beta` |
| `0000000140002BF9` | `15` | `TerminatedCString` | `overloaded_sum` |
| `0000000140002C08` | `21` | `TerminatedCString` | `overloaded_sum_float` |
| `0000000140002C1D` | `16` | `TerminatedCString` | `recursive_score` |
| `0000000140002C2D` | `16` | `TerminatedCString` | `resource_lookup` |
| `0000000140002C3D` | `16` | `TerminatedCString` | `shutdown_engine` |
| `0000000140002C4D` | `12` | `TerminatedCString` | `sparse_mode` |
| `0000000140002C59` | `12` | `TerminatedCString` | `switch_mode` |
| `0000000140002C65` | `13` | `TerminatedCString` | `system_calls` |
| `0000000140002C72` | `14` | `TerminatedCString` | `update_entity` |
| `0000000140002C80` | `22` | `TerminatedCString` | `update_entity_pointer` |
| `0000000140002D0A` | `19` | `TerminatedCString` | `OutputDebugStringA` |
| `0000000140002D20` | `13` | `TerminatedCString` | `GetTickCount` |
| `0000000140002D30` | `12` | `TerminatedCString` | `ExitProcess` |
| `0000000140002D3C` | `13` | `TerminatedCString` | `KERNEL32.dll` |
| `0000000140002D4C` | `12` | `TerminatedCString` | `LoadStringW` |
| `0000000140002D58` | `11` | `TerminatedCString` | `USER32.dll` |
| `0000000140003008` | `32` | `/test_analyzers_integration.pdb/EngineConfig<br>pack()<br>Structure EngineConfig {<br>   0   uint   4   world_id   ""<br>   8   char *   8   display_name   ""<br>   16   int *   8   difficulty_levels   ""<br>   24   uint   4   difficulty_count   ""<br>}<br>Length: 32 Alignment: 8<br>` | `None` |
| `0000000140005000` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |
| `0000000140005028` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |
| `0000000140005040` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |
| `0000000140005058` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |
| `0000000140005070` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |
| `0000000140005088` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |
| `00000001400050A0` | `16` | `/PE/IMAGE_RESOURCE_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_RESOURCE_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   word   2   NumberOfNamedEntries   ""<br>   14   word   2   NumberOfIdEntries   ""<br>}<br>Length: 16 Alignment: 1<br>` | `None` |

Defined data entries observed: `449`; maximum rendered: `100`.
| Address | Length | Type | Value |
| --- | ---: | --- | --- |
| `0x0000000140000000` | `128` | `/DOS/IMAGE_DOS_HEADER<br>pack(disabled)<br>Structure IMAGE_DOS_HEADER {<br>   0   char[2]   2   e_magic   "Magic number"<br>   2   word   2   e_cblp   "Bytes of last page"<br>   4   word   2   e_cp   "Pages in file"<br>   6   word   2   e_crlc   "Relocations"<br>   8   word   2   e_cparhdr   "Size of header in paragraphs"<br>   10   word   2   e_minalloc   "Minimum extra paragraphs needed"<br>   12   word   2   e_maxalloc   "Maximum extra paragraphs needed"<br>   14   word   2   e_ss   "Initial (relative) SS value"<br>   16   word   2   e_sp   "Initial SP value"<br>   18   word   2   e_csum   "Checksum"<br>   20   word   2   e_ip   "Initial IP value"<br>   22   word   2   e_cs   "Initial (relative) CS value"<br>   24   word   2   e_lfarlc   "File address of relocation table"<br>   26   word   2   e_ovno   "Overlay number"<br>   28   word[4]   8   e_res[4]   "Reserved words"<br>   36   word   2   e_oemid   "OEM identifier (for e_oeminfo)"<br>   38   word   2   e_oeminfo   "OEM information; e_oemid specific"<br>   40   word[10]   20   e_res2[10]   "Reserved words"<br>   60   dword   4   e_lfanew   "File address of new exe header"<br>   64   byte[64]   64   e_program   "Actual DOS program"<br>}<br>Length: 128 Alignment: 1<br>` | `None` |
| `0x0000000140000080` | `72` | `IMAGE_RICH_HEADER` | `IMAGE_RICH_HEADER[mask=b09f4949h, numRecords=6]` |
| `0x00000001400000E0` | `264` | `/PE/IMAGE_NT_HEADERS64<br>pack(disabled)<br>Structure IMAGE_NT_HEADERS64 {<br>   0   char[4]   4   Signature   ""<br>   4   IMAGE_FILE_HEADER   20   FileHeader   ""<br>   24   IMAGE_OPTIONAL_HEADER64   240   OptionalHeader   ""<br>}<br>Length: 264 Alignment: 1<br>` | `None` |
| `0x00000001400001E8` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0x0000000140000210` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0x0000000140000238` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0x0000000140000260` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0x0000000140000288` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0x00000001400002B0` | `40` | `/PE/IMAGE_SECTION_HEADER<br>pack(disabled)<br>Structure IMAGE_SECTION_HEADER {<br>   0   char[8]   8   Name   ""<br>   8   Misc   4   Misc   ""<br>   12   ImageBaseOffset32   4   VirtualAddress   ""<br>   16   dword   4   SizeOfRawData   ""<br>   20   dword   4   PointerToRawData   ""<br>   24   dword   4   PointerToRelocations   ""<br>   28   dword   4   PointerToLinenumbers   ""<br>   32   word   2   NumberOfRelocations   ""<br>   34   word   2   NumberOfLinenumbers   ""<br>   36   SectionFlags   4   Characteristics   ""<br>}<br>Length: 40 Alignment: 1<br>` | `None` |
| `0x0000000140001010` | `16` | `Alignment` | `align(16)` |
| `0x0000000140001030` | `16` | `Alignment` | `align(16)` |
| `0x0000000140001050` | `16` | `Alignment` | `align(16)` |
| `0x0000000140001081` | `15` | `Alignment` | `align(15)` |
| `0x00000001400010A0` | `16` | `Alignment` | `align(16)` |
| `0x00000001400010B6` | `10` | `Alignment` | `align(10)` |
| `0x00000001400010D1` | `15` | `Alignment` | `align(15)` |
| `0x00000001400010E4` | `12` | `Alignment` | `align(12)` |
| `0x00000001400010F6` | `10` | `Alignment` | `align(10)` |
| `0x000000014000110F` | `17` | `Alignment` | `align(17)` |
| `0x0000000140001126` | `10` | `Alignment` | `align(10)` |
| `0x0000000140001145` | `11` | `Alignment` | `align(11)` |
| `0x0000000140001177` | `9` | `Alignment` | `align(9)` |
| `0x00000001400011A0` | `16` | `Alignment` | `align(16)` |
| `0x00000001400011D1` | `15` | `Alignment` | `align(15)` |
| `0x0000000140001234` | `4` | `uint` | `0x11fb` |
| `0x0000000140001238` | `4` | `uint` | `0x1201` |
| `0x000000014000123C` | `4` | `uint` | `0x1207` |
| `0x0000000140001240` | `4` | `uint` | `0x120d` |
| `0x0000000140001244` | `4` | `uint` | `0x1213` |
| `0x0000000140001248` | `4` | `uint` | `0x1219` |
| `0x000000014000124C` | `4` | `uint` | `0x121f` |
| `0x0000000140001250` | `4` | `uint` | `0x1225` |
| `0x0000000140001287` | `9` | `Alignment` | `align(9)` |
| `0x00000001400012A5` | `11` | `Alignment` | `align(11)` |
| `0x00000001400012EB` | `21` | `Alignment` | `align(21)` |
| `0x0000000140001310` | `16` | `Alignment` | `align(16)` |
| `0x0000000140001335` | `11` | `Alignment` | `align(11)` |
| `0x0000000140001367` | `9` | `Alignment` | `align(9)` |
| `0x0000000140001398` | `8` | `Alignment` | `align(8)` |
| `0x00000001400013C8` | `8` | `Alignment` | `align(8)` |
| `0x0000000140001448` | `8` | `Alignment` | `align(8)` |
| `0x000000014000147F` | `17` | `Alignment` | `align(17)` |
| `0x00000001400014A0` | `16` | `Alignment` | `align(16)` |
| `0x000000014000172E` | `18` | `Alignment` | `align(18)` |
| `0x0000000140001749` | `7` | `Alignment` | `align(7)` |
| `0x0000000140001809` | `7` | `Alignment` | `align(7)` |
| `0x0000000140001821` | `15` | `Alignment` | `align(15)` |
| `0x0000000140001845` | `11` | `Alignment` | `align(11)` |
| `0x00000001400018A8` | `8` | `Alignment` | `align(8)` |
| `0x000000014000195B` | `5` | `Alignment` | `align(5)` |
| `0x000000014000196B` | `5` | `Alignment` | `align(5)` |
| `0x0000000140001986` | `10` | `Alignment` | `align(10)` |
| `0x00000001400019A6` | `10` | `Alignment` | `align(10)` |
| `0x00000001400019B5` | `11` | `Alignment` | `align(11)` |
| `0x00000001400019C5` | `11` | `Alignment` | `align(11)` |
| `0x00000001400019E6` | `10` | `Alignment` | `align(10)` |
| `0x00000001400019FA` | `6` | `Alignment` | `align(6)` |
| `0x0000000140001A04` | `12` | `Alignment` | `align(12)` |
| `0x0000000140001A18` | `8` | `Alignment` | `align(8)` |
| `0x0000000140001A28` | `8` | `Alignment` | `align(8)` |
| `0x0000000140001A38` | `8` | `Alignment` | `align(8)` |
| `0x0000000140001A48` | `8` | `Alignment` | `align(8)` |
| `0x0000000140001A54` | `12` | `Alignment` | `align(12)` |
| `0x0000000140001A64` | `12` | `Alignment` | `align(12)` |
| `0x0000000140001A75` | `11` | `Alignment` | `align(11)` |
| `0x0000000140001A97` | `9` | `Alignment` | `align(9)` |
| `0x0000000140001ABE` | `2` | `Alignment` | `align(2)` |
| `0x0000000140001AD7` | `9` | `Alignment` | `align(9)` |
| `0x0000000140001B02` | `14` | `Alignment` | `align(14)` |
| `0x0000000140002000` | `8` | `pointer` | `00002d1e` |
| `0x0000000140002008` | `8` | `pointer` | `00002d2e` |
| `0x0000000140002010` | `8` | `pointer` | `00002d08` |
| `0x0000000140002020` | `8` | `pointer` | `00002d4a` |
| `0x0000000140002030` | `45` | `char[45]` | `GTA-like native analysis integration fixture` |
| `0x0000000140002060` | `27` | `char[27]` | `tick=%d entity=%s score=%u` |
| `0x000000014000207C` | `5` | `char[5]` | `tiny` |
| `0x0000000140002088` | `12` | `char[12]` | `unterminated` |
| `0x0000000140002094` | `7` | `uchar[7]` | `������` |
| `0x00000001400020A0` | `20` | `int[5]` | `None` |
| `0x00000001400020B8` | `8` | `pointer` | `140001ac0` |
| `0x00000001400020C0` | `8` | `undefined *` | `140001a10` |
| `0x00000001400020C8` | `7` | `TerminatedCString` | `Entity` |
| `0x00000001400020D0` | `8` | `pointer` | `140001b40` |
| `0x00000001400020D8` | `8` | `undefined *` | `140001a40` |
| `0x00000001400020E0` | `8` | `TerminatedCString` | `Vehicle` |
| `0x00000001400020E8` | `8` | `pointer` | `140001ae0` |
| `0x00000001400020F0` | `8` | `undefined *` | `140001a20` |
| `0x00000001400020F8` | `4` | `TerminatedCString` | `Ped` |
| `0x0000000140002100` | `8` | `pointer` | `140001b10` |
| `0x0000000140002108` | `8` | `undefined *` | `140001a30` |
| `0x0000000140002110` | `7` | `TerminatedCString` | `Player` |
| `0x0000000140002118` | `8` | `pointer` | `140001a70` |
| `0x0000000140002120` | `8` | `pointer` | `140001aa0` |
| `0x0000000140002128` | `8` | `pointer` | `140001a80` |
| `0x000000014000213C` | `4` | `undefined4` | `0x40000000` |
| `0x0000000140002140` | `28` | `/PE/IMAGE_DEBUG_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_DEBUG_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   dword   4   Type   ""<br>   16   dword   4   SizeOfData   ""<br>   20   dword   4   AddressOfRawData   ""<br>   24   dword   4   PointerToRawData   ""<br>}<br>Length: 28 Alignment: 1<br>` | `None` |
| `0x000000014000215C` | `28` | `/PE/IMAGE_DEBUG_DIRECTORY<br>pack(disabled)<br>Structure IMAGE_DEBUG_DIRECTORY {<br>   0   dword   4   Characteristics   ""<br>   4   dword   4   TimeDateStamp   ""<br>   8   word   2   MajorVersion   ""<br>   10   word   2   MinorVersion   ""<br>   12   dword   4   Type   ""<br>   16   dword   4   SizeOfData   ""<br>   20   dword   4   AddressOfRawData   ""<br>   24   dword   4   PointerToRawData   ""<br>}<br>Length: 28 Alignment: 1<br>` | `None` |
[TRUNCATED: maximum total report size reached]
