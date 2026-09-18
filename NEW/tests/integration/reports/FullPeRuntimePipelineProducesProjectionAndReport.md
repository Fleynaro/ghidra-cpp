# ReCode Integration Report

- **Status:** PARTIAL
- **Test:** `ArchitectureIntegrationTest.FullPeRuntimePipelineProducesProjectionAndReport`
- **Fixture:** `services/analyzers/tests/data/test_analyzers_integration.exe`
- **SQLite projection:** `build/test-reports/project/projection.sqlite`
- **Sampling seed:** `0x415243485f504531`
- **Limits:** functions=50, decompilations=10, instructions=1000, sqlite_instructions=200, text_chars=100
- **Load revision:** 537
- **Analysis revision:** 539
- **Analyzer profile:** full (35 registered)
- **Entity profile:** missing references/data
- **SQLite checkpoint:** 539

## Pipeline

1. `ProjectFacade::open` created the project and event/projection stores.
2. `ProjectFacade::load` parsed the PE, materialized regions/symbols, decoded entry/export seeds, and committed events.
3. `ProjectFacade::analyze` executed the registered runtime analyzers.
4. Selected asynchronous `Task<Result<Decompilation>>` operations completed through the worker pool.

### Executed Analyzers

- `ASCII Strings`
- `Aggressive Instruction Finder`
- `Apply Data Archives`
- `Call Convention ID`
- `Call-Fixup Installer`
- `Condense Filler Bytes`
- `Constant Propagation`
- `Create Address Tables`
- `Data Reference`
- `Decompiler Parameter ID`
- `Decompiler Switch Analysis`
- `Demangler Microsoft`
- `Disassemble Entry Points`
- `Embedded Media`
- `External Entry References`
- `Function ID`
- `Function Start Pre Search`
- `Function Start Search`
- `Function Start Search After Code`
- `Function Start Search After Data`
- `Function Start Search In Functions`
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
- `runtime.entry_materialization`
- `x86 Constant Reference Analyzer`

#### Analysis Diagnostics

- Total items: 0
- Items shown: 0

- _none_

## Coverage Contract

- All sections use deterministic SQL/order plus stable-random sampling when a limit is exceeded.
- Text cells and diagnostic/source fields are truncated to the configured limit; omitted content is not a runtime failure.
- References and data objects are reported even when their current event set is empty.

## In-Memory Projection

- Revision: 539
- Functions: 31
- Instructions: 444
- Memory regions: 7
- Symbols: 53
- Data objects: 0

### Functions and Signatures

- Total rows: 31
- Rows shown: 31
- Sampling: all

| Space | Entry | Name | Namespace | External | No return | Status | Instructions | Body | Signature (return, parameters, ABI) | Structure |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ram | 5368713216 | filler_engine_a |  | false | false | decoded | 4 | ram:5368713216-5368713231 | C source: void default filler_engine_a(void) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713248 | filler_engine_b |  | false | false | decoded | 4 | ram:5368713248-5368713263 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713280 | filler_engine_c |  | false | false | decoded | 4 | ram:5368713280-5368713295 | C source: void default filler_engine_c(void) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713312 | filler_engine_d |  | false | false | decoded | 4 | ram:5368713312-5368713327 | C source: void default filler_engine_d(void) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713328 | ?add@Calculator@demangle_fixture@@QEAAHHH@Z |  | false | false | decoded | 4 | ram:5368713328-5368713344 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713360 | ?scale@Calculator@demangle_fixture@@SAHH@Z |  | false | false | decoded | 4 | ram:5368713360-5368713375 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713392 | ?combine@overload_fixture@@YAHHH@Z |  | false | false | decoded | 3 | ram:5368713392-5368713397 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713408 | ?combine@overload_fixture@@YAMMM@Z |  | false | false | decoded | 4 | ram:5368713408-5368713424 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713440 | callback_add |  | false | false | decoded | 2 | ram:5368713440-5368713443 | C source: int4 default callback_add(int4 param_1) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713456 | callback_rotate |  | false | false | decoded | 3 | ram:5368713456-5368713461 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713472 | callback_mask |  | false | false | decoded | 4 | ram:5368713472-5368713486 | C source: uint4 default callback_mask(uint4 param_1) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713504 | overloaded_sum |  | false | false | decoded | 3 | ram:5368713504-5368713509 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713520 | overloaded_sum_float |  | false | false | decoded | 4 | ram:5368713520-5368713540 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713552 | recursive_score |  | false | false | decoded | 15 | ram:5368713552-5368713590 | C source: int4 default recursive_score(int4 param_1) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713600 | mutual_alpha |  | false | false | decoded | 11 | ram:5368713600-5368713631 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713648 | mutual_beta |  | false | false | decoded | 11 | ram:5368713648-5368713680 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713696 | switch_mode |  | false | false | decoded | 9 | ram:5368713696-5368713776 | C source: undefined8 default switch_mode(undefined4 param_1) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713824 | sparse_mode |  | false | false | decoded | 13 | ram:5368713824-5368713862 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713872 | invoke_callback |  | false | false | decoded | 6 | ram:5368713872-5368713892 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713904 | update_entity |  | false | false | decoded | 19 | ram:5368713904-5368713962 | C source: uint8 default update_entity(int8 *param_1) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713984 | update_entity_pointer |  | false | false | decoded | 6 | ram:5368713984-5368713999 | C source: undefined8 default update_entity_pointer(int8 param_1,int4 param_2) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714016 | engine_abort |  | false | false | decoded | 5 | ram:5368714016-5368714036 | C source: void default engine_abort(void) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714048 | abort_path_one |  | false | false | decoded | 19 | ram:5368714048-5368714095 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714096 | abort_path_two |  | false | false | decoded | 18 | ram:5368714096-5368714143 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714144 | abort_path_three |  | false | false | decoded | 18 | ram:5368714144-5368714191 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714192 | resource_lookup |  | false | false | decoded | 27 | ram:5368714192-5368714311 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714320 | system_calls |  | false | false | decoded | 10 | ram:5368714320-5368714366 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714384 | shutdown_engine |  | false | false | decoded | 19 | ram:5368714384-5368714415 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714416 | engine_tick |  | false | false | decoded | 148 | ram:5368714416-5368715053 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368715072 | engine_unused_dead_code |  | false | false | decoded | 3 | ram:5368715072-5368715080 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368715088 | fixture_entry |  | false | false | decoded | 40 | ram:5368715088-5368715272 | <none> | blocks=0, thunk=<none>, variables=0 |

### SQLite Event Types

- Total rows: 7
- Rows shown: 7
- Sampling: all

| Event type | Rows |
| --- | --- |
| AnalysisRunStateChanged | 2 |
| FunctionStateChanged | 31 |
| ListingStateChanged | 444 |
| MemoryStateChanged | 7 |
| ProjectCreated | 1 |
| ProjectInputsChanged | 1 |
| SymbolStateChanged | 53 |

### SQLite Event Records

- Total rows: 539
- Rows shown: 100
- Sampling: stable-random

| Sequence | Event type | Service | Payload |
| --- | --- | --- | --- |
| 9 | MemoryStateChanged | pe_loader | id=memory-6;space=ram;start=5368733696;end=5368734207;name=.reloc;r=1;w=0;x=0; |
| 10 | SymbolStateChanged | symbol | id=export-1;address=5368713328;name=?add@Calculator@demangle_fixture@@QEAAHHH@Z;namespace=;priori... |
| 11 | SymbolStateChanged | symbol | id=export-2;address=5368713392;name=?combine@overload_fixture@@YAHHH@Z;namespace=;priority=0; |
| 15 | SymbolStateChanged | symbol | id=export-6;address=5368714144;name=abort_path_three;namespace=;priority=0; |
| 25 | SymbolStateChanged | symbol | id=export-16;address=5368717472;name=engine_difficulty_levels;namespace=;priority=0; |
| 33 | SymbolStateChanged | symbol | id=export-24;address=5368721408;name=engine_selector;namespace=;priority=0; |
| 36 | SymbolStateChanged | symbol | id=export-27;address=5368717776;name=engine_table_sentinel;namespace=;priority=0; |
| 38 | SymbolStateChanged | symbol | id=export-29;address=5368717448;name=engine_unterminated;namespace=;priority=0; |
| 45 | SymbolStateChanged | symbol | id=export-36;address=5368715088;name=fixture_entry;namespace=;priority=0; |
| 52 | SymbolStateChanged | symbol | id=export-43;address=5368714192;name=resource_lookup;namespace=;priority=0; |
| 56 | SymbolStateChanged | symbol | id=export-47;address=5368714320;name=system_calls;namespace=;priority=0; |
| 74 | ListingStateChanged | sleigh | id=instruction-ram-5368715138;space=ram;address=5368715138;length=5;mnemonic=CALL;assembly=0x1400... |
| 78 | ListingStateChanged | sleigh | id=instruction-ram-5368715159;space=ram;address=5368715159;length=5;mnemonic=CALL;assembly=0x1400... |
| 83 | ListingStateChanged | sleigh | id=instruction-ram-5368715184;space=ram;address=5368715184;length=6;mnemonic=MOV;assembly=EDX,dwo... |
| 101 | ListingStateChanged | sleigh | id=instruction-ram-5368715271;space=ram;address=5368715271;length=1;mnemonic=POP;assembly=RDI;byt... |
| 106 | ListingStateChanged | sleigh | id=instruction-ram-5368713338;space=ram;address=5368713338;length=6;mnemonic=MOV;assembly=EAX,dwo... |
| 114 | ListingStateChanged | sleigh | id=instruction-ram-5368713416;space=ram;address=5368713416;length=4;mnemonic=ADDSS;assembly=XMM0,... |
| 117 | FunctionStateChanged | sleigh | id=function-5368713408;space=ram;entry=5368713408;name=?combine@overload_fixture@@YAMMM@Z;end=536... |
| 118 | ListingStateChanged | sleigh | id=instruction-ram-5368713360;space=ram;address=5368713360;length=3;mnemonic=LEA;assembly=EAX,[RC... |
| 120 | ListingStateChanged | sleigh | id=instruction-ram-5368713369;space=ram;address=5368713369;length=6;mnemonic=MOV;assembly=EAX,dwo... |
| 125 | ListingStateChanged | sleigh | id=instruction-ram-5368714058;space=ram;address=5368714058;length=3;mnemonic=CMP;assembly=EAX,-0x... |
| 136 | ListingStateChanged | sleigh | id=instruction-ram-5368714090;space=ram;address=5368714090;length=1;mnemonic=INT3;assembly=;bytes... |
| 138 | ListingStateChanged | sleigh | id=instruction-ram-5368714092;space=ram;address=5368714092;length=1;mnemonic=INT3;assembly=;bytes... |
| 142 | FunctionStateChanged | sleigh | id=function-5368714048;space=ram;entry=5368714048;name=abort_path_one;end=5368714095;status=decoded; |
| 146 | ListingStateChanged | sleigh | id=instruction-ram-5368714157;space=ram;address=5368714157;length=2;mnemonic=JZ;assembly=0x140001... |
| 157 | ListingStateChanged | sleigh | id=instruction-ram-5368714188;space=ram;address=5368714188;length=1;mnemonic=INT3;assembly=;bytes... |
| 163 | ListingStateChanged | sleigh | id=instruction-ram-5368714100;space=ram;address=5368714100;length=6;mnemonic=MOV;assembly=EAX,dwo... |
| 164 | ListingStateChanged | sleigh | id=instruction-ram-5368714106;space=ram;address=5368714106;length=3;mnemonic=CMP;assembly=EAX,-0x... |
| 167 | ListingStateChanged | sleigh | id=instruction-ram-5368714117;space=ram;address=5368714117;length=3;mnemonic=ADD;assembly=EAX,0x2... |
| 186 | ListingStateChanged | sleigh | id=instruction-ram-5368713484;space=ram;address=5368713484;length=2;mnemonic=MOV;assembly=EAX,ECX... |
| 190 | ListingStateChanged | sleigh | id=instruction-ram-5368713459;space=ram;address=5368713459;length=2;mnemonic=MOV;assembly=EAX,ECX... |
| 195 | ListingStateChanged | sleigh | id=instruction-ram-5368714024;space=ram;address=5368714024;length=5;mnemonic=XOR;assembly=EAX,0xd... |
| 197 | ListingStateChanged | sleigh | id=instruction-ram-5368714035;space=ram;address=5368714035;length=2;mnemonic=JMP;assembly=0x14000... |
| 212 | ListingStateChanged | sleigh | id=instruction-ram-5368714458;space=ram;address=5368714458;length=6;mnemonic=MOV;assembly=R9D,0x5... |
| 221 | ListingStateChanged | sleigh | id=instruction-ram-5368714503;space=ram;address=5368714503;length=3;mnemonic=MOV;assembly=R8,RDX;... |
| 232 | ListingStateChanged | sleigh | id=instruction-ram-5368714563;space=ram;address=5368714563;length=6;mnemonic=MOV;assembly=R9D,0x2... |
| 233 | ListingStateChanged | sleigh | id=instruction-ram-5368714569;space=ram;address=5368714569;length=5;mnemonic=LEA;assembly=RCX,[RS... |
| 236 | ListingStateChanged | sleigh | id=instruction-ram-5368714583;space=ram;address=5368714583;length=5;mnemonic=CALL;assembly=0x1400... |
| 241 | ListingStateChanged | sleigh | id=instruction-ram-5368714609;space=ram;address=5368714609;length=2;mnemonic=MOV;assembly=ECX,EDI... |
| 242 | ListingStateChanged | sleigh | id=instruction-ram-5368714611;space=ram;address=5368714611;length=5;mnemonic=LEA;assembly=RAX,[RS... |
| 243 | ListingStateChanged | sleigh | id=instruction-ram-5368714616;space=ram;address=5368714616;length=5;mnemonic=MOV;assembly=qword p... |
| 252 | ListingStateChanged | sleigh | id=instruction-ram-5368714659;space=ram;address=5368714659;length=3;mnemonic=MOV;assembly=R12D,EA... |
| 256 | ListingStateChanged | sleigh | id=instruction-ram-5368714680;space=ram;address=5368714680;length=5;mnemonic=CALL;assembly=0x1400... |
| 257 | ListingStateChanged | sleigh | id=instruction-ram-5368714685;space=ram;address=5368714685;length=2;mnemonic=MOV;assembly=EBP,EDI... |
| 263 | ListingStateChanged | sleigh | id=instruction-ram-5368714712;space=ram;address=5368714712;length=5;mnemonic=LEA;assembly=RCX,[RS... |
| 274 | ListingStateChanged | sleigh | id=instruction-ram-5368714760;space=ram;address=5368714760;length=5;mnemonic=CALL;assembly=0x1400... |
| 279 | ListingStateChanged | sleigh | id=instruction-ram-5368714784;space=ram;address=5368714784;length=5;mnemonic=CALL;assembly=0x1400... |
| 283 | ListingStateChanged | sleigh | id=instruction-ram-5368714802;space=ram;address=5368714802;length=6;mnemonic=MOV;assembly=ECX,dwo... |
| 294 | ListingStateChanged | sleigh | id=instruction-ram-5368714843;space=ram;address=5368714843;length=5;mnemonic=CALL;assembly=0x1400... |
| 295 | ListingStateChanged | sleigh | id=instruction-ram-5368714848;space=ram;address=5368714848;length=7;mnemonic=MOV;assembly=EBP,dwo... |
| 298 | ListingStateChanged | sleigh | id=instruction-ram-5368714865;space=ram;address=5368714865;length=4;mnemonic=MOVD;assembly=XMM0, ... |
| 300 | ListingStateChanged | sleigh | id=instruction-ram-5368714872;space=ram;address=5368714872;length=5;mnemonic=CALL;assembly=0x1400... |
| 302 | ListingStateChanged | sleigh | id=instruction-ram-5368714879;space=ram;address=5368714879;length=3;mnemonic=MOV;assembly=ECX,R12... |
| 305 | ListingStateChanged | sleigh | id=instruction-ram-5368714890;space=ram;address=5368714890;length=5;mnemonic=MOV;assembly=EDX,0x3... |
| 309 | ListingStateChanged | sleigh | id=instruction-ram-5368714904;space=ram;address=5368714904;length=8;mnemonic=MOVZX;assembly=ECX,b... |
| 317 | ListingStateChanged | sleigh | id=instruction-ram-5368714936;space=ram;address=5368714936;length=2;mnemonic=ADD;assembly=EDI,ESI... |
| 330 | ListingStateChanged | sleigh | id=instruction-ram-5368714989;space=ram;address=5368714989;length=5;mnemonic=CALL;assembly=0x1400... |
| 331 | ListingStateChanged | sleigh | id=instruction-ram-5368714994;space=ram;address=5368714994;length=8;mnemonic=LEA;assembly=RCX,[RS... |
| 338 | ListingStateChanged | sleigh | id=instruction-ram-5368715035;space=ram;address=5368715035;length=7;mnemonic=ADD;assembly=RSP,0xd... |
| 354 | ListingStateChanged | sleigh | id=instruction-ram-5368713225;space=ram;address=5368713225;length=6;mnemonic=MOV;assembly=dword p... |
| 358 | ListingStateChanged | sleigh | id=instruction-ram-5368713254;space=ram;address=5368713254;length=3;mnemonic=ADD;assembly=EAX,0x2... |
| 370 | ListingStateChanged | sleigh | id=instruction-ram-5368713327;space=ram;address=5368713327;length=1;mnemonic=RET;assembly=;bytes=... |
| 376 | ListingStateChanged | sleigh | id=instruction-ram-5368713888;space=ram;address=5368713888;length=2;mnemonic=MOV;assembly=ECX,EDX... |
| 386 | ListingStateChanged | sleigh | id=instruction-ram-5368713620;space=ram;address=5368713620;length=5;mnemonic=CALL;assembly=0x1400... |
| 390 | FunctionStateChanged | sleigh | id=function-5368713600;space=ram;entry=5368713600;name=mutual_alpha;end=5368713631;status=decoded; |
| 396 | ListingStateChanged | sleigh | id=instruction-ram-5368713665;space=ram;address=5368713665;length=1;mnemonic=RET;assembly=;bytes=... |
| 403 | ListingStateChanged | sleigh | id=instruction-ram-5368713504;space=ram;address=5368713504;length=3;mnemonic=LEA;assembly=EAX,[RD... |
| 404 | ListingStateChanged | sleigh | id=instruction-ram-5368713507;space=ram;address=5368713507;length=2;mnemonic=ADD;assembly=EAX,ECX... |
| 406 | FunctionStateChanged | sleigh | id=function-5368713504;space=ram;entry=5368713504;name=overloaded_sum;end=5368713509;status=decoded; |
| 416 | ListingStateChanged | sleigh | id=instruction-ram-5368713563;space=ram;address=5368713563;length=2;mnemonic=JG;assembly=0x140001... |
| 417 | ListingStateChanged | sleigh | id=instruction-ram-5368713565;space=ram;address=5368713565;length=5;mnemonic=MOV;assembly=EAX,0x1... |
| 423 | ListingStateChanged | sleigh | id=instruction-ram-5368713583;space=ram;address=5368713583;length=2;mnemonic=ADD;assembly=EAX,EBX... |
| 425 | ListingStateChanged | sleigh | id=instruction-ram-5368713589;space=ram;address=5368713589;length=1;mnemonic=POP;assembly=RBX;byt... |
| 428 | ListingStateChanged | sleigh | id=instruction-ram-5368714192;space=ram;address=5368714192;length=3;mnemonic=MOV;assembly=RAX,RSP... |
| 429 | ListingStateChanged | sleigh | id=instruction-ram-5368714195;space=ram;address=5368714195;length=7;mnemonic=SUB;assembly=RSP,0xa... |
| 432 | ListingStateChanged | sleigh | id=instruction-ram-5368714210;space=ram;address=5368714210;length=5;mnemonic=MOVUPS;assembly=xmmw... |
| 443 | ListingStateChanged | sleigh | id=instruction-ram-5368714255;space=ram;address=5368714255;length=6;mnemonic=CALL;assembly=qword ... |
| 452 | ListingStateChanged | sleigh | id=instruction-ram-5368714298;space=ram;address=5368714298;length=6;mnemonic=MOV;assembly=dword p... |
| 454 | ListingStateChanged | sleigh | id=instruction-ram-5368714311;space=ram;address=5368714311;length=1;mnemonic=RET;assembly=;bytes=... |
| 457 | ListingStateChanged | sleigh | id=instruction-ram-5368714388;space=ram;address=5368714388;length=6;mnemonic=MOV;assembly=ECX,dwo... |
| 462 | ListingStateChanged | sleigh | id=instruction-ram-5368714403;space=ram;address=5368714403;length=1;mnemonic=INT3;assembly=;bytes... |
| 463 | ListingStateChanged | sleigh | id=instruction-ram-5368714404;space=ram;address=5368714404;length=1;mnemonic=INT3;assembly=;bytes... |
| 471 | ListingStateChanged | sleigh | id=instruction-ram-5368714412;space=ram;address=5368714412;length=1;mnemonic=INT3;assembly=;bytes... |
| 472 | ListingStateChanged | sleigh | id=instruction-ram-5368714413;space=ram;address=5368714413;length=1;mnemonic=INT3;assembly=;bytes... |
| 477 | ListingStateChanged | sleigh | id=instruction-ram-5368713827;space=ram;address=5368713827;length=2;mnemonic=JZ;assembly=0x140001... |
| 480 | ListingStateChanged | sleigh | id=instruction-ram-5368713834;space=ram;address=5368713834;length=2;mnemonic=XOR;assembly=EDX,EDX... |
| 485 | ListingStateChanged | sleigh | id=instruction-ram-5368713851;space=ram;address=5368713851;length=5;mnemonic=MOV;assembly=EAX,0x2... |
| 499 | FunctionStateChanged | sleigh | id=function-5368713696;space=ram;entry=5368713696;name=switch_mode;end=5368713776;status=decoded; |
| 501 | ListingStateChanged | sleigh | id=instruction-ram-5368714324;space=ram;address=5368714324;length=7;mnemonic=LEA;assembly=RCX,[0x... |
| 505 | ListingStateChanged | sleigh | id=instruction-ram-5368714350;space=ram;address=5368714350;length=2;mnemonic=MOV;assembly=EAX,EAX... |
| 508 | ListingStateChanged | sleigh | id=instruction-ram-5368714362;space=ram;address=5368714362;length=4;mnemonic=ADD;assembly=RSP,0x2... |
| 512 | ListingStateChanged | sleigh | id=instruction-ram-5368713909;space=ram;address=5368713909;length=1;mnemonic=PUSH;assembly=RDI;by... |
| 518 | ListingStateChanged | sleigh | id=instruction-ram-5368713925;space=ram;address=5368713925;length=3;mnemonic=MOV;assembly=RCX,RBX... |
| 520 | ListingStateChanged | sleigh | id=instruction-ram-5368713930;space=ram;address=5368713930;length=3;mnemonic=CALL;assembly=qword ... |
| 522 | ListingStateChanged | sleigh | id=instruction-ram-5368713939;space=ram;address=5368713939;length=5;mnemonic=MOV;assembly=RBX,qwo... |
| 526 | ListingStateChanged | sleigh | id=instruction-ram-5368713951;space=ram;address=5368713951;length=6;mnemonic=MOV;assembly=dword p... |
| 531 | ListingStateChanged | sleigh | id=instruction-ram-5368713984;space=ram;address=5368713984;length=3;mnemonic=TEST;assembly=RCX,RC... |
| 534 | ListingStateChanged | sleigh | id=instruction-ram-5368713992;space=ram;address=5368713992;length=1;mnemonic=RET;assembly=;bytes=... |
| 535 | ListingStateChanged | sleigh | id=instruction-ram-5368713993;space=ram;address=5368713993;length=2;mnemonic=INC;assembly=EDX;byt... |
| 536 | ListingStateChanged | sleigh | id=instruction-ram-5368713995;space=ram;address=5368713995;length=5;mnemonic=JMP;assembly=0x14000... |

### SQLite Memory Regions

- Total rows: 7
- Rows shown: 7
- Sampling: all

| Space | Start | End | Name | Read | Write | Execute |
| --- | --- | --- | --- | --- | --- | --- |
| ram | 5368709120 | 5368710143 | Headers | 1 | 0 | 0 |
| ram | 5368713216 | 5368716287 | .text | 1 | 0 | 1 |
| ram | 5368717312 | 5368720895 | .rdata | 1 | 0 | 0 |
| ram | 5368721408 | 5368721919 | .data | 1 | 1 | 0 |
| ram | 5368725504 | 5368726015 | .pdata | 1 | 0 | 0 |
| ram | 5368729600 | 5368730623 | .rsrc | 1 | 0 | 0 |
| ram | 5368733696 | 5368734207 | .reloc | 1 | 0 | 0 |

### SQLite Functions

- Total rows: 31
- Rows shown: 31
- Sampling: all

| Space | Entry | End | Name | Status | Producer |
| --- | --- | --- | --- | --- | --- |
| ram | 5368713216 | 5368713231 | filler_engine_a | decoded | sleigh |
| ram | 5368713248 | 5368713263 | filler_engine_b | decoded | sleigh |
| ram | 5368713280 | 5368713295 | filler_engine_c | decoded | sleigh |
| ram | 5368713312 | 5368713327 | filler_engine_d | decoded | sleigh |
| ram | 5368713328 | 5368713344 | ?add@Calculator@demangle_fixture@@QEAAHHH@Z | decoded | sleigh |
| ram | 5368713360 | 5368713375 | ?scale@Calculator@demangle_fixture@@SAHH@Z | decoded | sleigh |
| ram | 5368713392 | 5368713397 | ?combine@overload_fixture@@YAHHH@Z | decoded | sleigh |
| ram | 5368713408 | 5368713424 | ?combine@overload_fixture@@YAMMM@Z | decoded | sleigh |
| ram | 5368713440 | 5368713443 | callback_add | decoded | sleigh |
| ram | 5368713456 | 5368713461 | callback_rotate | decoded | sleigh |
| ram | 5368713472 | 5368713486 | callback_mask | decoded | sleigh |
| ram | 5368713504 | 5368713509 | overloaded_sum | decoded | sleigh |
| ram | 5368713520 | 5368713540 | overloaded_sum_float | decoded | sleigh |
| ram | 5368713552 | 5368713590 | recursive_score | decoded | sleigh |
| ram | 5368713600 | 5368713631 | mutual_alpha | decoded | sleigh |
| ram | 5368713648 | 5368713680 | mutual_beta | decoded | sleigh |
| ram | 5368713696 | 5368713776 | switch_mode | decoded | sleigh |
| ram | 5368713824 | 5368713862 | sparse_mode | decoded | sleigh |
| ram | 5368713872 | 5368713892 | invoke_callback | decoded | sleigh |
| ram | 5368713904 | 5368713962 | update_entity | decoded | sleigh |
| ram | 5368713984 | 5368713999 | update_entity_pointer | decoded | sleigh |
| ram | 5368714016 | 5368714036 | engine_abort | decoded | sleigh |
| ram | 5368714048 | 5368714095 | abort_path_one | decoded | sleigh |
| ram | 5368714096 | 5368714143 | abort_path_two | decoded | sleigh |
| ram | 5368714144 | 5368714191 | abort_path_three | decoded | sleigh |
| ram | 5368714192 | 5368714311 | resource_lookup | decoded | sleigh |
| ram | 5368714320 | 5368714366 | system_calls | decoded | sleigh |
| ram | 5368714384 | 5368714415 | shutdown_engine | decoded | sleigh |
| ram | 5368714416 | 5368715053 | engine_tick | decoded | sleigh |
| ram | 5368715072 | 5368715080 | engine_unused_dead_code | decoded | sleigh |
| ram | 5368715088 | 5368715272 | fixture_entry | decoded | sleigh |

### SQLite Instructions

- Total rows: 444
- Rows shown: 200
- Sampling: stable-random

| Space | Address | Length | Mnemonic | Assembly | Bytes | Mask | Flow | Fallthrough | Terminal | Target | P-code | Producer | Metadata |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ram | 5368713225 | 6 | MOV | dword ptr [0x14000302c],EAX | 89051d200000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713248 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b0506200000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713280 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05e61f0000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713286 | 3 | ADD | EAX,0x33 | 83c033 | fff800 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713289 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905dd1f0000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713295 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713327 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713332 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905b21f0000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713344 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713360 | 3 | LEA | EAX,[RCX + RCX*0x2] | 8d0449 | ffc700 | 0 | 1 | 0 |  | 4 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713375 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713392 | 3 | LEA | EAX,[RDX + 0x7] | 8d4207 | ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713420 | 4 | ADDSS | XMM0, XMM1 | f30f58c1 | ffffffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713424 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713440 | 3 | LEA | EAX,[RCX + 0x11] | 8d4111 | ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713443 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713461 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713484 | 2 | MOV | EAX,ECX | 8bc1 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713504 | 3 | LEA | EAX,[RDX + 0x21] | 8d4221 | ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713509 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713536 | 4 | ADDSS | XMM0, XMM1 | f30f58c1 | ffffffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713540 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713563 | 2 | JG | 0x140001168 | 7f0b | ff00 | 2 | 1 | 0 | 5368713576 | 4 | sleigh | operands=1, flow=2, target=5368713576, pcode=0 |
| ram | 5368713565 | 5 | MOV | EAX,0x1 | b801000000 | f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713570 | 4 | ADD | RSP,0x20 | 4883c420 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713585 | 4 | ADD | RSP,0x20 | 4883c420 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713589 | 1 | POP | RBX | 5b | f8 | 0 | 1 | 0 |  | 4 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713604 | 2 | TEST | ECX,ECX | 85c9 | ffc0 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713608 | 5 | MOV | EAX,0x2 | b802000000 | f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713613 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713617 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713618 | 2 | DEC | ECX | ffc9 | fff8 | 0 | 1 | 0 |  | 9 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713620 | 5 | CALL | 0x1400011b0 | e817000000 | ff00000000 | 4 | 1 | 0 | 5368713648 | 3 | sleigh | operands=1, flow=4, target=5368713648, pcode=0 |
| ram | 5368713627 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713631 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713656 | 5 | MOV | EAX,0x3 | b803000000 | f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713673 | 3 | ADD | EAX,0x2 | 83c002 | fff800 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713676 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713699 | 2 | JA | 0x14000122b | 7746 | ff00 | 2 | 1 | 0 | 5368713771 | 3 | sleigh | operands=1, flow=2, target=5368713771, pcode=0 |
| ram | 5368713718 | 3 | ADD | RCX,RDX | 4803ca | f8ffc0 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713771 | 5 | MOV | EAX,0xffffffff | b8ffffffff | f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713836 | 5 | MOV | EAX,0x3 | b803000000 | f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713841 | 6 | CMP | ECX,0x3e8 | 81f9e8030000 | fff800000000 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713847 | 3 | CMOVNZ | EAX,EDX | 0f45c2 | ffffc0 | 2 | 1 | 0 | 5368713850 | 6 | sleigh | operands=2, flow=2, target=5368713850, pcode=0 |
| ram | 5368713850 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713851 | 5 | MOV | EAX,0x2 | b802000000 | f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713862 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368713872 | 2 | MOV | EAX,ECX | 8bc1 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713874 | 7 | LEA | RCX,[0x1400021b0] | 488d0d170f0000 | f8ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713888 | 2 | MOV | ECX,EDX | 8bca | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713890 | 3 | JMP | RAX | 48ffe0 | f8fff8 | 5 | 0 | 1 |  | 1 | sleigh | operands=1, flow=5, target=<none>, pcode=0 |
| ram | 5368713917 | 3 | MOV | RBX,RCX | 488bd9 | f8ffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713920 | 2 | CALL | qword ptr [RAX] | ff10 | fff8 | 6 | 1 | 0 |  | 5 | sleigh | operands=1, flow=6, target=<none>, pcode=0 |
| ram | 5368713925 | 3 | MOV | RCX,RBX | 488bcb | f8ffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713933 | 6 | MOV | EDX,dword ptr [0x14000302c] | 8b15591d0000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713947 | 2 | MOV | EAX,EDI | 8bc7 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713949 | 2 | XOR | EDX,ECX | 33d1 | ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713951 | 6 | MOV | dword ptr [0x14000302c],EDX | 8915471d0000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713957 | 4 | ADD | RSP,0x20 | 4883c420 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713987 | 2 | JNZ | 0x140001309 | 7504 | ff00 | 2 | 1 | 0 | 5368713993 | 2 | sleigh | operands=1, flow=2, target=5368713993, pcode=0 |
| ram | 5368713993 | 2 | INC | EDX | ffc2 | fff8 | 0 | 1 | 0 |  | 9 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714016 | 2 | NOP |  | 6690 | ffff | 0 | 1 | 0 |  | 0 | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714024 | 5 | XOR | EAX,0xdeadbeef | 35efbeadde | ff00000000 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714058 | 3 | CMP | EAX,-0x1 | 83f8ff | fff800 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714063 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05d71c0000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714081 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368714082 | 5 | CALL | 0x140001320 | e8b9ffffff | ff00000000 | 4 | 1 | 0 | 5368714016 | 3 | sleigh | operands=1, flow=4, target=5368714016, pcode=0 |
| ram | 5368714089 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714090 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714093 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714094 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714096 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714100 | 6 | MOV | EAX,dword ptr [0x140003000] | 8b05861c0000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714109 | 2 | JZ | 0x140001393 | 7414 | ff00 | 2 | 1 | 0 | 5368714131 | 1 | sleigh | operands=1, flow=2, target=5368714131, pcode=0 |
| ram | 5368714130 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368714137 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714139 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714140 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714141 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714143 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714144 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714154 | 3 | CMP | EAX,-0x3 | 83f8fd | fff800 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714165 | 3 | ADD | EAX,0x3 | 83c003 | fff800 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714168 | 6 | MOV | dword ptr [0x14000302c],EAX | 89056e1c0000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714184 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714185 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714188 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714192 | 3 | MOV | RAX,RSP | 488bc4 | f8ffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714202 | 3 | XORPS | XMM0, XMM0 | 0f57c0 | ffffc0 | 0 | 1 | 0 |  | 4 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714223 | 4 | MOVUPS | xmmword ptr [RAX + -0x78], XMM0 | 0f114088 | ffffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714227 | 4 | MOVUPS | xmmword ptr [RAX + -0x68], XMM0 | 0f114098 | ffffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714239 | 4 | MOVUPS | xmmword ptr [RAX + -0x48], XMM0 | 0f1140b8 | ffffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714261 | 6 | MOV | R9D,0x40 | 41b940000000 | f8f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714267 | 5 | LEA | R8,[RSP + 0x20] | 4c8d442420 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714280 | 4 | LEA | EDX,[R9 + 0x26] | 418d5126 | f8ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714290 | 6 | MOV | ECX,dword ptr [0x14000302c] | 8b0df41b0000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714298 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905ec1b0000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714304 | 7 | ADD | RSP,0xa8 | 4881c4a8000000 | f8fff800000000 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714331 | 6 | CALL | qword ptr [0x140002010] | ff15af0b0000 | ffff00000000 | 6 | 1 | 0 |  | 4 | sleigh | operands=1, flow=6, target=<none>, pcode=0 |
| ram | 5368714337 | 6 | CALL | qword ptr [0x140002000] | ff15990b0000 | ffff00000000 | 6 | 1 | 0 |  | 4 | sleigh | operands=1, flow=6, target=<none>, pcode=0 |
| ram | 5368714355 | 7 | MOV | qword ptr [0x140003030],RCX | 48890db61b0000 | f8ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714366 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368714388 | 6 | MOV | ECX,dword ptr [0x14000302c] | 8b0d921b0000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714394 | 6 | CALL | qword ptr [0x140002008] | ff15680b0000 | ffff00000000 | 6 | 1 | 0 |  | 4 | sleigh | operands=1, flow=6, target=<none>, pcode=0 |
| ram | 5368714400 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714402 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714403 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714409 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714410 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714411 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714413 | 1 | INT3 |  | cc | ff | 6 | 1 | 0 |  | 3 | sleigh | operands=0, flow=6, target=<none>, pcode=0 |
| ram | 5368714416 | 5 | MOV | qword ptr [RSP + 0x10],RBX | 48895c2410 | f8ffc73800 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714421 | 4 | MOV | dword ptr [RSP + 0x8],ECX | 894c2408 | ffc73800 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714426 | 1 | PUSH | RSI | 56 | f8 | 0 | 1 | 0 |  | 3 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714427 | 1 | PUSH | RDI | 57 | f8 | 0 | 1 | 0 |  | 3 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714428 | 2 | PUSH | R12 | 4154 | f8f8 | 0 | 1 | 0 |  | 3 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714432 | 2 | PUSH | R14 | 4156 | f8f8 | 0 | 1 | 0 |  | 3 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714443 | 2 | MOV | EAX,ECX | 8bc1 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714458 | 6 | MOV | R9D,0x5a | 41b95a000000 | f8f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714464 | 7 | MOV | dword ptr [RSP + 0x128],EAX | 89842428010000 | ffc73800000000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714495 | 3 | ADD | R8,RAX | 4c03c0 | f8ffc0 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714503 | 3 | MOV | R8,RDX | 4c8bc2 | f8ffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714513 | 4 | LEA | EDX,[R9 + -0x4f] | 418d51b1 | f8ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714543 | 3 | MOV | R8,RBX | 4c8bc3 | f8ffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714546 | 8 | LEA | RCX,[RSP + 0x98] | 488d8c2498000000 | f8ffc73800000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714554 | 4 | LEA | EDX,[R9 + -0x4e] | 418d51b2 | f8ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714569 | 5 | LEA | RCX,[RSP + 0x60] | 488d4c2460 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714583 | 5 | CALL | 0x1400018e0 | e884030000 | ff00000000 | 4 | 1 | 0 | 5368715488 | 3 | sleigh | operands=1, flow=4, target=5368715488, pcode=0 |
| ram | 5368714593 | 5 | LEA | RCX,[RSP + 0x48] | 488d4c2448 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714598 | 6 | MOV | R8D,0x1000 | 41b800100000 | f8f800000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714604 | 5 | CALL | 0x1400018b0 | e83f030000 | ff00000000 | 4 | 1 | 0 | 5368715440 | 3 | sleigh | operands=1, flow=4, target=5368715440, pcode=0 |
| ram | 5368714616 | 5 | MOV | qword ptr [RSP + 0x38],RAX | 4889442438 | f8ffc73800 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714634 | 5 | MOV | qword ptr [RSP + 0x40],RAX | 4889442440 | f8ffc73800 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714644 | 5 | CALL | 0x140001300 | e867fdffff | ff00000000 | 4 | 1 | 0 | 5368713984 | 3 | sleigh | operands=1, flow=4, target=5368713984, pcode=0 |
| ram | 5368714659 | 3 | MOV | R12D,EAX | 448be0 | f8ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714662 | 5 | CALL | 0x140001a50 | e8a5040000 | ff00000000 | 4 | 1 | 0 | 5368715856 | 3 | sleigh | operands=1, flow=4, target=5368715856, pcode=0 |
| ram | 5368714672 | 8 | LEA | RCX,[RSP + 0x98] | 488d8c2498000000 | f8ffc73800000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714680 | 5 | CALL | 0x140001a60 | e8a3040000 | ff00000000 | 4 | 1 | 0 | 5368715872 | 3 | sleigh | operands=1, flow=4, target=5368715872, pcode=0 |
| ram | 5368714685 | 2 | MOV | EBP,EDI | 8bef | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714695 | 3 | AND | EBP,0x3 | 83e503 | fff800 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714700 | 5 | CALL | 0x140001a00 | e82f040000 | ff00000000 | 4 | 1 | 0 | 5368715776 | 3 | sleigh | operands=1, flow=4, target=5368715776, pcode=0 |
| ram | 5368714717 | 5 | CALL | 0x1400019f0 | e80e040000 | ff00000000 | 4 | 1 | 0 | 5368715760 | 3 | sleigh | operands=1, flow=4, target=5368715760, pcode=0 |
| ram | 5368714727 | 3 | MOV | R13D,EAX | 448be8 | f8ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714730 | 5 | CALL | 0x140001aa0 | e8b1040000 | ff00000000 | 4 | 1 | 0 | 5368715936 | 3 | sleigh | operands=1, flow=4, target=5368715936, pcode=0 |
| ram | 5368714735 | 5 | LEA | RCX,[RSP + 0x48] | 488d4c2448 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714740 | 3 | ADD | R13D,EAX | 4403e8 | f8ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714743 | 5 | CALL | 0x140001a80 | e884040000 | ff00000000 | 4 | 1 | 0 | 5368715904 | 3 | sleigh | operands=1, flow=4, target=5368715904, pcode=0 |
| ram | 5368714757 | 3 | ADD | R13D,EAX | 4403e8 | f8ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714771 | 8 | LEA | RCX,[RSP + 0x128] | 488d8c2428010000 | f8ffc73800000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714779 | 2 | MOV | EDX,EDI | 8bd7 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714781 | 3 | MOV | R15D,EAX | 448bf8 | f8ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714784 | 5 | CALL | 0x140001070 | e84bfaffff | ff00000000 | 4 | 1 | 0 | 5368713328 | 3 | sleigh | operands=1, flow=4, target=5368713328, pcode=0 |
| ram | 5368714794 | 3 | MOV | R14D,EAX | 448bf0 | f8ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714797 | 5 | CALL | 0x140001090 | e85efaffff | ff00000000 | 4 | 1 | 0 | 5368713360 | 3 | sleigh | operands=1, flow=4, target=5368713360, pcode=0 |
| ram | 5368714808 | 3 | ADD | R14D,EAX | 4403f0 | f8ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714822 | 2 | MOV | EDI,EAX | 8bf8 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714831 | 3 | LEA | ECX,[RBP + 0x2] | 8d4d02 | ffc000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714839 | 2 | MOV | ECX,EBP | 8bcd | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714843 | 5 | CALL | 0x140001180 | e820fbffff | ff00000000 | 4 | 1 | 0 | 5368713600 | 3 | sleigh | operands=1, flow=4, target=5368713600, pcode=0 |
| ram | 5368714855 | 2 | ADD | ESI,EAX | 03f0 | ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714865 | 4 | MOVD | XMM0, EBP | 660f6ec5 | ffffffc0 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714877 | 2 | MOV | EDX,EDI | 8bd7 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714879 | 3 | MOV | ECX,R12D | 418bcc | f8ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714882 | 3 | MOVAPS | XMM6, XMM0 | 0f28f0 | ffffc0 | 0 | 1 | 0 |  | 4 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714885 | 5 | CALL | 0x140001120 | e896faffff | ff00000000 | 4 | 1 | 0 | 5368713504 | 3 | sleigh | operands=1, flow=4, target=5368713504, pcode=0 |
| ram | 5368714897 | 2 | MOV | EBX,EAX | 8bd8 | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714919 | 2 | ADD | EBX,ECX | 03d9 | ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714936 | 2 | ADD | EDI,ESI | 03fe | ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714938 | 3 | ADD | EDI,R14D | 4103fe | f8ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714955 | 7 | MOV | RDX,qword ptr [0x140003030] | 488b155e190000 | f8ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714962 | 2 | MOV | ECX,EDI | 8bcf | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714964 | 3 | ADD | RDX,RCX | 4803d1 | f8ffc0 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714967 | 5 | LEA | RCX,[RSP + 0x48] | 488d4c2448 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714972 | 7 | MOV | qword ptr [0x140003030],RDX | 4889154d190000 | f8ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714984 | 5 | LEA | RCX,[RSP + 0x60] | 488d4c2460 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714994 | 8 | LEA | RCX,[RSP + 0x98] | 488d8c2498000000 | f8ffc73800000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715007 | 5 | LEA | RCX,[RSP + 0x78] | 488d4c2478 | f8ffc73800 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715012 | 5 | CALL | 0x1400019d0 | e8c7020000 | ff00000000 | 4 | 1 | 0 | 5368715728 | 3 | sleigh | operands=1, flow=4, target=5368715728, pcode=0 |
| ram | 5368715017 | 8 | MOV | RBX,qword ptr [RSP + 0x118] | 488b9c2418010000 | f8ffc73800000000 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715048 | 2 | POP | R12 | 415c | f8f8 | 0 | 1 | 0 |  | 4 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715051 | 1 | POP | RSI | 5e | f8 | 0 | 1 | 0 |  | 4 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715053 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368715080 | 1 | RET |  | c3 | ff | 7 | 0 | 1 |  | 3 | sleigh | operands=0, flow=7, target=<none>, pcode=0 |
| ram | 5368715093 | 1 | PUSH | RDI | 57 | f8 | 0 | 1 | 0 |  | 3 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715094 | 4 | SUB | RSP,0x20 | 4883ec20 | f8fff800 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715113 | 5 | CALL | 0x140001060 | e8f2f8ffff | ff00000000 | 4 | 1 | 0 | 5368713312 | 3 | sleigh | operands=1, flow=4, target=5368713312, pcode=0 |
| ram | 5368715118 | 5 | CALL | 0x1400013d0 | e85dfcffff | ff00000000 | 4 | 1 | 0 | 5368714192 | 3 | sleigh | operands=1, flow=4, target=5368714192, pcode=0 |
| ram | 5368715123 | 5 | CALL | 0x140001450 | e8d8fcffff | ff00000000 | 4 | 1 | 0 | 5368714320 | 3 | sleigh | operands=1, flow=4, target=5368714320, pcode=0 |
| ram | 5368715128 | 5 | CALL | 0x140001340 | e8c3fbffff | ff00000000 | 4 | 1 | 0 | 5368714048 | 3 | sleigh | operands=1, flow=4, target=5368714048, pcode=0 |
| ram | 5368715138 | 5 | CALL | 0x1400013a0 | e819fcffff | ff00000000 | 4 | 1 | 0 | 5368714144 | 3 | sleigh | operands=1, flow=4, target=5368714144, pcode=0 |
| ram | 5368715164 | 6 | MOV | dword ptr [0x14000302c],EAX | 89058a180000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715170 | 2 | XOR | EBX,EBX | 33db | ffc0 | 0 | 1 | 0 |  | 10 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715184 | 6 | MOV | EDX,dword ptr [0x14000302c] | 8b1576180000 | ffc700000000 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715190 | 2 | MOV | ECX,EBX | 8bcb | ffc0 | 0 | 1 | 0 |  | 2 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715192 | 5 | CALL | 0x140001290 | e8d3faffff | ff00000000 | 4 | 1 | 0 | 5368713872 | 3 | sleigh | operands=1, flow=4, target=5368713872, pcode=0 |
| ram | 5368715207 | 6 | MOV | dword ptr [0x14000302c],EAX | 89055f180000 | ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715218 | 7 | MOV | RAX,qword ptr [0x140003030] | 488b0557180000 | f8ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715235 | 5 | MOV | RBX,qword ptr [RSP + 0x30] | 488b5c2430 | f8ffc73800 | 0 | 1 | 0 |  | 3 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715243 | 7 | MOV | qword ptr [0x140003030],RAX | 4889053e180000 | f8ffc700000000 | 0 | 1 | 0 |  | 1 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715257 | 3 | XOR | RAX,RDI | 4833c7 | f8ffc0 | 0 | 1 | 0 |  | 9 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |

### SQLite Symbols

- Total rows: 53
- Rows shown: 53
- Sampling: all

| Space | Address | Name | Namespace | Priority |
| --- | --- | --- | --- | --- |
| ram | 5368713328 | ?add@Calculator@demangle_fixture@@QEAAHHH@Z |  | 0 |
| ram | 5368713456 | callback_rotate |  | 0 |
| ram | 5368714016 | engine_abort |  | 0 |
| ram | 5368718008 | engine_au |  | 0 |
| ram | 5368717360 | engine_banner |  | 0 |
| ram | 5368717744 | engine_callbacks |  | 0 |
| ram | 5368721416 | engine_config |  | 0 |
| ram | 5368717472 | engine_difficulty_levels |  | 0 |
| ram | 5368717408 | engine_format |  | 0 |
| ram | 5368717808 | engine_gif |  | 0 |
| ram | 5368721456 | engine_global_ticks |  | 0 |
| ram | 5368713392 | ?combine@overload_fixture@@YAHHH@Z |  | 0 |
| ram | 5368717976 | engine_midi |  | 0 |
| ram | 5368717460 | engine_non_ascii |  | 0 |
| ram | 5368717856 | engine_png |  | 0 |
| ram | 5368717784 | engine_pointer_data |  | 0 |
| ram | 5368721408 | engine_selector |  | 0 |
| ram | 5368717436 | engine_short |  | 0 |
| ram | 5368721452 | engine_state |  | 0 |
| ram | 5368717776 | engine_table_sentinel |  | 0 |
| ram | 5368714416 | engine_tick |  | 0 |
| ram | 5368717448 | engine_unterminated |  | 0 |
| ram | 5368713408 | ?combine@overload_fixture@@YAMMM@Z |  | 0 |
| ram | 5368715072 | engine_unused_dead_code |  | 0 |
| ram | 5368717928 | engine_wave |  | 0 |
| ram | 5368713216 | filler_engine_a |  | 0 |
| ram | 5368713248 | filler_engine_b |  | 0 |
| ram | 5368713280 | filler_engine_c |  | 0 |
| ram | 5368713312 | filler_engine_d |  | 0 |
| ram | 5368715088 | fixture_entry |  | 0 |
| ram | 5368713872 | invoke_callback |  | 0 |
| ram | 5368713600 | mutual_alpha |  | 0 |
| ram | 5368713648 | mutual_beta |  | 0 |
| ram | 5368713360 | ?scale@Calculator@demangle_fixture@@SAHH@Z |  | 0 |
| ram | 5368713504 | overloaded_sum |  | 0 |
| ram | 5368713520 | overloaded_sum_float |  | 0 |
| ram | 5368713552 | recursive_score |  | 0 |
| ram | 5368714192 | resource_lookup |  | 0 |
| ram | 5368714384 | shutdown_engine |  | 0 |
| ram | 5368713824 | sparse_mode |  | 0 |
| ram | 5368713696 | switch_mode |  | 0 |
| ram | 5368714320 | system_calls |  | 0 |
| ram | 5368713904 | update_entity |  | 0 |
| ram | 5368713984 | update_entity_pointer |  | 0 |
| ram | 5368714048 | abort_path_one |  | 0 |
| ram | 5368714144 | abort_path_three |  | 0 |
| ram | 5368714096 | abort_path_two |  | 0 |
| ram | 5368713440 | callback_add |  | 0 |
| ram | 5368713472 | callback_mask |  | 0 |
| ram | 5368717312 | GetTickCount | KERNEL32.dll | 0 |
| ram | 5368717320 | ExitProcess | KERNEL32.dll | 0 |
| ram | 5368717328 | OutputDebugStringA | KERNEL32.dll | 0 |
| ram | 5368717344 | LoadStringW | USER32.dll | 0 |

### SQLite Analysis Runs

- Total rows: 1
- Rows shown: 1
- Sampling: all

| Run | Status | Sequence |
| --- | --- | --- |
| analysis-3 | completed | 539 |

### SQLite References

- Total rows: 0
- Rows shown: 0
- Sampling: all

| Source | Target | Kind |
| --- | --- | --- |
| _none_ |

### SQLite Data Objects

- Total rows: 0
- Rows shown: 0
- Sampling: all

| Address | Type | Value |
| --- | --- | --- |
| _none_ |

## Decompilations

- Total results: 10
- Results shown: 10

### `0x140001000` filler_engine_a

- Status: `complete`
- Read revision: 539
- Signature: `C source: void default filler_engine_a(void)`
- Raw instructions: 4
- Control-flow chars: 48
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

void default filler_engine_a(void)

{
  iRam000000014000302c = iRam000000014000302c + 0x11;
  return;
}

```

```text
0
  Basic Block 0 0x000140001000-0x00014000100f

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x140001040` filler_engine_c

- Status: `complete`
- Read revision: 539
- Signature: `C source: void default filler_engine_c(void)`
- Raw instructions: 4
- Control-flow chars: 48
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

void default filler_engine_c(void)

{
  iRam000000014000302c = iRam000000014000302c + 0x33;
  return;
}

```

```text
0
  Basic Block 0 0x000140001040-0x00014000104f

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x140001060` filler_engine_d

- Status: `complete`
- Read revision: 539
- Signature: `C source: void default filler_engine_d(void)`
- Raw instructions: 4
- Control-flow chars: 48
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

void default filler_engine_d(void)

{
  iRam000000014000302c = iRam000000014000302c + 0x44;
  return;
}

```

```text
0
  Basic Block 0 0x000140001060-0x00014000106f

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x1400010e0` callback_add

- Status: `complete`
- Read revision: 539
- Signature: `C source: int4 default callback_add(int4 param_1)`
- Raw instructions: 2
- Control-flow chars: 48
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

int4 default callback_add(int4 param_1)

{
  return param_1 + 0x11;
}

```

```text
0
  Basic Block 0 0x0001400010e0-0x0001400010e3

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x140001100` callback_mask

- Status: `complete`
- Read revision: 539
- Signature: `C source: uint4 default callback_mask(uint4 param_1)`
- Raw instructions: 4
- Control-flow chars: 48
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

uint4 default callback_mask(uint4 param_1)

{
  return (param_1 ^ 0xffa5a5a5) & 0xffffff;
}

```

```text
0
  Basic Block 0 0x000140001100-0x00014000110e

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x140001150` recursive_score

- Status: `complete`
- Read revision: 539
- Signature: `C source: int4 default recursive_score(int4 param_1)`
- Raw instructions: 15
- Control-flow chars: 169
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

int4 default recursive_score(int4 param_1)

{
  int4 iVar1;

  if (param_1 < 2) {
    return 1;
  }
  iVar1 = recursive_score(param_1 + -1);
  return iVar1 + param_1;
}

```

```text
0
  If (no exit) block 0
    Basic Block 0 0x000140001150-0x00014000115b
    Basic Block 2 0x00014000115d-0x000140001167
    Basic Block 1 0x000140001168-0x000140001176

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x1400011e0` switch_mode

- Status: `complete`
- Read revision: 539
- Signature: `C source: undefined8 default switch_mode(undefined4 param_1)`
- Raw instructions: 9
- Control-flow chars: 499
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

undefined8 default switch_mode(undefined4 param_1)

{
  switch(param_1) {
  case 0:
    return 10;
  case 1:
    return 0x14;
  case 2:
    return 0x1e;
  case 3:
    return 0x28;
  case 4:
    return 0x32;
  case 5:
    return 0x3c;
  case 6:
    return 0x46;
  case 7:
    return 0x50;
  default:
    return 0xffffffff;
  }
}

```

```text
0
  Switch block 0
    Basic Block 0 0x0001400011e0-0x0001400011f9
    Basic Block 9 0x0001400011fb-0x000140001200
    Basic Block 8 0x000140001201-0x000140001206
    Basic Block 7 0x000140001207-0x00014000120c
    Basic Block 6 0x00014000120d-0x000140001212
    Basic Block 5 0x000140001213-0x000140001218
    Basic Block 4 0x000140001219-0x00014000121e
    Basic Block 3 0x00014000121f-0x000140001224
    Basic Block 2 0x000140001225-0x00014000122a
    Basic Block 1 0x00014000122b-0x000140001230

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x1400012b0` update_entity

- Status: `complete`
- Read revision: 539
- Signature: `C source: uint8 default update_entity(int8 *param_1)`
- Raw instructions: 19
- Control-flow chars: 48
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

uint8 default update_entity(int8 *param_1)

{
  uint4 uVar1;
  char *pcVar2;
  uint8 uVar3;

  uVar1 = (**(code **)*param_1)();
  uVar3 = (uint8)uVar1;
  pcVar2 = (char *)(**(code **)(*param_1 + 8))(param_1);
  uRam000000014000302c = uRam000000014000302c ^ (int4)*pcVar2;
  return uVar3 & 0xffffffff;
}

```

```text
0
  Basic Block 0 0x0001400012b0-0x0001400012ea

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x140001300` update_entity_pointer

- Status: `complete`
- Read revision: 539
- Signature: `C source: undefined8 default update_entity_pointer(int8 param_1,int4 param_2)`
- Raw instructions: 6
- Control-flow chars: 169
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

undefined8 default update_entity_pointer(int8 param_1,int4 param_2)

{
  undefined8 xVar1;

  if (param_1 == 0) {
    return 0xffffffff;
  }
  xVar1 = update_entity(param_1,param_2 + 1);
  return xVar1;
}

```

```text
0
  If (no exit) block 0
    Basic Block 0 0x000140001300-0x000140001303
    Basic Block 2 0x000140001305-0x000140001308
    Basic Block 1 0x000140001309-0x00014000130b

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_

### `0x140001320` engine_abort

- Status: `complete`
- Read revision: 539
- Signature: `C source: void default engine_abort(void)`
- Raw instructions: 5
- Control-flow chars: 141
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c

void default engine_abort(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}

```

```text
0
  List block 0
    Basic Block 0 0x000140001320-0x000140001320
    Infinite loop block 1
      Basic Block 1 0x000140001322-0x000140001333

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 0
- Items shown: 0

- _none_
