# ReCode Integration Report

- **Status:** PASS
- **Test:** `ArchitectureIntegrationTest.FullPeRuntimePipelineProducesProjectionAndReport`
- **Fixture:** `services/analyzers/tests/data/test_analyzers_integration.exe`
- **SQLite projection:** `build/test-reports/project/projection.sqlite`
- **Sampling seed:** `0x415243485f504531`
- **Limits:** functions=50, decompilations=10, instructions=1000, sqlite_instructions=200, text_chars=100
- **Load revision:** 401
- **Analysis revision:** 403
- **SQLite checkpoint:** 403

## Pipeline

1. `ProjectFacade::open` created the project and event/projection stores.
2. `ProjectFacade::load` parsed the PE, materialized regions/symbols, decoded entry/export seeds, and committed events.
3. `ProjectFacade::analyze` executed the registered runtime analyzers.
4. Selected asynchronous `Task<Result<Decompilation>>` operations completed through the worker pool.

### Executed Analyzers

- `runtime.entry_materialization`

#### Analysis Diagnostics

- Total items: 0
- Items shown: 0

- _none_

## Coverage Contract

- All sections use deterministic SQL/order plus stable-random sampling when a limit is exceeded.
- Text cells and diagnostic/source fields are truncated to the configured limit; omitted content is not a runtime failure.
- References and data objects are reported even when their current event set is empty.

## In-Memory Projection

- Revision: 403
- Functions: 31
- Instructions: 308
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
| ram | 5368713552 | recursive_score |  | false | false | decoded | 9 | ram:5368713552-5368713575 | C source: <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713600 | mutual_alpha |  | false | false | decoded | 6 | ram:5368713600-5368713617 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713648 | mutual_beta |  | false | false | decoded | 6 | ram:5368713648-5368713665 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713696 | switch_mode |  | false | false | decoded | 7 | ram:5368713696-5368713722 | C source: <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713824 | sparse_mode |  | false | false | decoded | 9 | ram:5368713824-5368713850 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713872 | invoke_callback |  | false | false | decoded | 6 | ram:5368713872-5368713892 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713904 | update_entity |  | false | false | decoded | 19 | ram:5368713904-5368713962 | C source: uint8 default update_entity(int8 *param_1) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368713984 | update_entity_pointer |  | false | false | decoded | 4 | ram:5368713984-5368713992 | C source: <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714016 | engine_abort |  | false | false | decoded | 5 | ram:5368714016-5368714036 | C source: void default engine_abort(void) | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714048 | abort_path_one |  | false | false | decoded | 9 | ram:5368714048-5368714081 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714096 | abort_path_two |  | false | false | decoded | 9 | ram:5368714096-5368714130 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714144 | abort_path_three |  | false | false | decoded | 9 | ram:5368714144-5368714178 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714192 | resource_lookup |  | false | false | decoded | 27 | ram:5368714192-5368714311 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714320 | system_calls |  | false | false | decoded | 10 | ram:5368714320-5368714366 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714384 | shutdown_engine |  | false | false | decoded | 64 | ram:5368714384-5368714620 | <none> | blocks=0, thunk=<none>, variables=0 |
| ram | 5368714416 | engine_tick |  | false | false | decoded | 64 | ram:5368714416-5368714711 | <none> | blocks=0, thunk=<none>, variables=0 |
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
| ListingStateChanged | 308 |
| MemoryStateChanged | 7 |
| ProjectCreated | 1 |
| ProjectInputsChanged | 1 |
| SymbolStateChanged | 53 |

### SQLite Event Records

- Total rows: 403
- Rows shown: 100
- Sampling: stable-random

| Sequence | Event type | Service | Payload |
| --- | --- | --- | --- |
| 2 | ProjectInputsChanged | pe_loader | artifact=integration-input;name=fixture.exe;format=PE;locator=<services/analyzers/tests/data/test... |
| 5 | MemoryStateChanged | pe_loader | id=memory-2;space=ram;start=5368717312;end=5368720895;name=.rdata;r=1;w=0;x=0; |
| 6 | MemoryStateChanged | pe_loader | id=memory-3;space=ram;start=5368721408;end=5368721919;name=.data;r=1;w=1;x=0; |
| 7 | MemoryStateChanged | pe_loader | id=memory-4;space=ram;start=5368725504;end=5368726015;name=.pdata;r=1;w=0;x=0; |
| 8 | MemoryStateChanged | pe_loader | id=memory-5;space=ram;start=5368729600;end=5368730623;name=.rsrc;r=1;w=0;x=0; |
| 10 | SymbolStateChanged | symbol | id=export-1;address=5368713328;name=?add@Calculator@demangle_fixture@@QEAAHHH@Z;namespace=;priori... |
| 14 | SymbolStateChanged | symbol | id=export-5;address=5368714048;name=abort_path_one;namespace=;priority=0; |
| 21 | SymbolStateChanged | symbol | id=export-12;address=5368718008;name=engine_au;namespace=;priority=0; |
| 23 | SymbolStateChanged | symbol | id=export-14;address=5368717744;name=engine_callbacks;namespace=;priority=0; |
| 26 | SymbolStateChanged | symbol | id=export-17;address=5368717408;name=engine_format;namespace=;priority=0; |
| 35 | SymbolStateChanged | symbol | id=export-26;address=5368721452;name=engine_state;namespace=;priority=0; |
| 36 | SymbolStateChanged | symbol | id=export-27;address=5368717776;name=engine_table_sentinel;namespace=;priority=0; |
| 38 | SymbolStateChanged | symbol | id=export-29;address=5368717448;name=engine_unterminated;namespace=;priority=0; |
| 43 | SymbolStateChanged | symbol | id=export-34;address=5368713280;name=filler_engine_c;namespace=;priority=0; |
| 50 | SymbolStateChanged | symbol | id=export-41;address=5368713520;name=overloaded_sum_float;namespace=;priority=0; |
| 58 | SymbolStateChanged | symbol | id=export-49;address=5368713984;name=update_entity_pointer;namespace=;priority=0; |
| 59 | SymbolStateChanged | symbol | id=import-0;address=5368717312;name=GetTickCount;namespace=KERNEL32.dll;priority=0; |
| 63 | ListingStateChanged | sleigh | id=instruction-ram-5368715088;space=ram;address=5368715088;length=5;mnemonic=MOV;assembly=qword p... |
| 65 | ListingStateChanged | sleigh | id=instruction-ram-5368715094;space=ram;address=5368715094;length=4;mnemonic=SUB;assembly=RSP,0x2... |
| 66 | ListingStateChanged | sleigh | id=instruction-ram-5368715098;space=ram;address=5368715098;length=5;mnemonic=CALL;assembly=0x1400... |
| 67 | ListingStateChanged | sleigh | id=instruction-ram-5368715103;space=ram;address=5368715103;length=5;mnemonic=CALL;assembly=0x1400... |
| 73 | ListingStateChanged | sleigh | id=instruction-ram-5368715133;space=ram;address=5368715133;length=5;mnemonic=CALL;assembly=0x1400... |
| 75 | ListingStateChanged | sleigh | id=instruction-ram-5368715143;space=ram;address=5368715143;length=6;mnemonic=MOV;assembly=ECX,dwo... |
| 78 | ListingStateChanged | sleigh | id=instruction-ram-5368715159;space=ram;address=5368715159;length=5;mnemonic=CALL;assembly=0x1400... |
| 79 | ListingStateChanged | sleigh | id=instruction-ram-5368715164;space=ram;address=5368715164;length=6;mnemonic=MOV;assembly=dword p... |
| 82 | ListingStateChanged | sleigh | id=instruction-ram-5368715176;space=ram;address=5368715176;length=8;mnemonic=NOP;assembly=dword p... |
| 90 | ListingStateChanged | sleigh | id=instruction-ram-5368715213;space=ram;address=5368715213;length=3;mnemonic=CMP;assembly=EBX,0x4... |
| 93 | ListingStateChanged | sleigh | id=instruction-ram-5368715225;space=ram;address=5368715225;length=10;mnemonic=MOV;assembly=RCX,0x... |
| 103 | FunctionStateChanged | sleigh | id=function-5368715088;space=ram;entry=5368715088;name=fixture_entry;end=5368715272;status=decoded; |
| 105 | ListingStateChanged | sleigh | id=instruction-ram-5368713332;space=ram;address=5368713332;length=6;mnemonic=MOV;assembly=dword p... |
| 108 | FunctionStateChanged | sleigh | id=function-5368713328;space=ram;entry=5368713328;name=?add@Calculator@demangle_fixture@@QEAAHHH@... |
| 124 | ListingStateChanged | sleigh | id=instruction-ram-5368714052;space=ram;address=5368714052;length=6;mnemonic=MOV;assembly=EAX,dwo... |
| 125 | ListingStateChanged | sleigh | id=instruction-ram-5368714058;space=ram;address=5368714058;length=3;mnemonic=CMP;assembly=EAX,-0x... |
| 128 | ListingStateChanged | sleigh | id=instruction-ram-5368714069;space=ram;address=5368714069;length=2;mnemonic=INC;assembly=EAX;byt... |
| 129 | ListingStateChanged | sleigh | id=instruction-ram-5368714071;space=ram;address=5368714071;length=6;mnemonic=MOV;assembly=dword p... |
| 132 | FunctionStateChanged | sleigh | id=function-5368714048;space=ram;entry=5368714048;name=abort_path_one;end=5368714081;status=decoded; |
| 142 | FunctionStateChanged | sleigh | id=function-5368714144;space=ram;entry=5368714144;name=abort_path_three;end=5368714178;status=dec... |
| 145 | ListingStateChanged | sleigh | id=instruction-ram-5368714106;space=ram;address=5368714106;length=3;mnemonic=CMP;assembly=EAX,-0x... |
| 148 | ListingStateChanged | sleigh | id=instruction-ram-5368714117;space=ram;address=5368714117;length=3;mnemonic=ADD;assembly=EAX,0x2... |
| 155 | FunctionStateChanged | sleigh | id=function-5368713440;space=ram;entry=5368713440;name=callback_add;end=5368713443;status=decoded; |
| 161 | ListingStateChanged | sleigh | id=instruction-ram-5368713456;space=ram;address=5368713456;length=3;mnemonic=ROL;assembly=ECX,0x3... |
| 166 | ListingStateChanged | sleigh | id=instruction-ram-5368714018;space=ram;address=5368714018;length=6;mnemonic=MOV;assembly=EAX,dwo... |
| 179 | ListingStateChanged | sleigh | id=instruction-ram-5368714434;space=ram;address=5368714434;length=2;mnemonic=PUSH;assembly=R15;by... |
| 181 | ListingStateChanged | sleigh | id=instruction-ram-5368714443;space=ram;address=5368714443;length=2;mnemonic=MOV;assembly=EAX,ECX... |
| 183 | ListingStateChanged | sleigh | id=instruction-ram-5368714453;space=ram;address=5368714453;length=5;mnemonic=XOR;assembly=EAX,0x5... |
| 189 | ListingStateChanged | sleigh | id=instruction-ram-5368714483;space=ram;address=5368714483;length=7;mnemonic=MOV;assembly=R8,qwor... |
| 191 | ListingStateChanged | sleigh | id=instruction-ram-5368714495;space=ram;address=5368714495;length=3;mnemonic=ADD;assembly=R8,RAX;... |
| 193 | ListingStateChanged | sleigh | id=instruction-ram-5368714503;space=ram;address=5368714503;length=3;mnemonic=MOV;assembly=R8,RDX;... |
| 194 | ListingStateChanged | sleigh | id=instruction-ram-5368714506;space=ram;address=5368714506;length=7;mnemonic=MOV;assembly=EAX,dwo... |
| 195 | ListingStateChanged | sleigh | id=instruction-ram-5368714513;space=ram;address=5368714513;length=4;mnemonic=LEA;assembly=EDX,[R9... |
| 202 | ListingStateChanged | sleigh | id=instruction-ram-5368714554;space=ram;address=5368714554;length=4;mnemonic=LEA;assembly=EDX,[R9... |
| 208 | ListingStateChanged | sleigh | id=instruction-ram-5368714583;space=ram;address=5368714583;length=5;mnemonic=CALL;assembly=0x1400... |
| 209 | ListingStateChanged | sleigh | id=instruction-ram-5368714588;space=ram;address=5368714588;length=5;mnemonic=MOV;assembly=EDX,0x7... |
| 210 | ListingStateChanged | sleigh | id=instruction-ram-5368714593;space=ram;address=5368714593;length=5;mnemonic=LEA;assembly=RCX,[RS... |
| 211 | ListingStateChanged | sleigh | id=instruction-ram-5368714598;space=ram;address=5368714598;length=6;mnemonic=MOV;assembly=R8D,0x1... |
| 222 | ListingStateChanged | sleigh | id=instruction-ram-5368714649;space=ram;address=5368714649;length=5;mnemonic=MOV;assembly=EDX,0x5... |
| 224 | ListingStateChanged | sleigh | id=instruction-ram-5368714659;space=ram;address=5368714659;length=3;mnemonic=MOV;assembly=R12D,EA... |
| 225 | ListingStateChanged | sleigh | id=instruction-ram-5368714662;space=ram;address=5368714662;length=5;mnemonic=CALL;assembly=0x1400... |
| 227 | ListingStateChanged | sleigh | id=instruction-ram-5368714672;space=ram;address=5368714672;length=8;mnemonic=LEA;assembly=RCX,[RS... |
| 230 | ListingStateChanged | sleigh | id=instruction-ram-5368714687;space=ram;address=5368714687;length=8;mnemonic=LEA;assembly=RCX,[RS... |
| 231 | ListingStateChanged | sleigh | id=instruction-ram-5368714695;space=ram;address=5368714695;length=3;mnemonic=AND;assembly=EBP,0x3... |
| 234 | ListingStateChanged | sleigh | id=instruction-ram-5368714705;space=ram;address=5368714705;length=7;mnemonic=MOV;assembly=EDX,dwo... |
| 235 | FunctionStateChanged | sleigh | id=function-5368714416;space=ram;entry=5368714416;name=engine_tick;end=5368714711;status=decoded; |
| 249 | FunctionStateChanged | sleigh | id=function-5368713248;space=ram;entry=5368713248;name=filler_engine_b;end=5368713263;status=deco... |
| 256 | ListingStateChanged | sleigh | id=instruction-ram-5368713318;space=ram;address=5368713318;length=3;mnemonic=ADD;assembly=EAX,0x4... |
| 267 | ListingStateChanged | sleigh | id=instruction-ram-5368713600;space=ram;address=5368713600;length=4;mnemonic=SUB;assembly=RSP,0x2... |
| 270 | ListingStateChanged | sleigh | id=instruction-ram-5368713608;space=ram;address=5368713608;length=5;mnemonic=MOV;assembly=EAX,0x2... |
| 275 | ListingStateChanged | sleigh | id=instruction-ram-5368713652;space=ram;address=5368713652;length=2;mnemonic=TEST;assembly=ECX,EC... |
| 278 | ListingStateChanged | sleigh | id=instruction-ram-5368713661;space=ram;address=5368713661;length=4;mnemonic=ADD;assembly=RSP,0x2... |
| 289 | FunctionStateChanged | sleigh | id=function-5368713520;space=ram;entry=5368713520;name=overloaded_sum_float;end=5368713540;status... |
| 293 | ListingStateChanged | sleigh | id=instruction-ram-5368713560;space=ram;address=5368713560;length=3;mnemonic=CMP;assembly=ECX,0x1... |
| 294 | ListingStateChanged | sleigh | id=instruction-ram-5368713563;space=ram;address=5368713563;length=2;mnemonic=JG;assembly=0x140001... |
| 300 | ListingStateChanged | sleigh | id=instruction-ram-5368714192;space=ram;address=5368714192;length=3;mnemonic=MOV;assembly=RAX,RSP... |
| 305 | ListingStateChanged | sleigh | id=instruction-ram-5368714215;space=ram;address=5368714215;length=6;mnemonic=MOV;assembly=R9D,0x4... |
| 311 | ListingStateChanged | sleigh | id=instruction-ram-5368714239;space=ram;address=5368714239;length=4;mnemonic=MOVUPS;assembly=xmmw... |
| 313 | ListingStateChanged | sleigh | id=instruction-ram-5368714247;space=ram;address=5368714247;length=4;mnemonic=MOVUPS;assembly=xmmw... |
| 317 | ListingStateChanged | sleigh | id=instruction-ram-5368714267;space=ram;address=5368714267;length=5;mnemonic=LEA;assembly=R8,[RSP... |
| 321 | ListingStateChanged | sleigh | id=instruction-ram-5368714284;space=ram;address=5368714284;length=6;mnemonic=CALL;assembly=qword ... |
| 322 | ListingStateChanged | sleigh | id=instruction-ram-5368714290;space=ram;address=5368714290;length=6;mnemonic=MOV;assembly=ECX,dwo... |
| 328 | ListingStateChanged | sleigh | id=instruction-ram-5368714384;space=ram;address=5368714384;length=4;mnemonic=SUB;assembly=RSP,0x2... |
| 333 | ListingStateChanged | sleigh | id=instruction-ram-5368714402;space=ram;address=5368714402;length=1;mnemonic=INT3;assembly=;bytes... |
| 343 | ListingStateChanged | sleigh | id=instruction-ram-5368714412;space=ram;address=5368714412;length=1;mnemonic=INT3;assembly=;bytes... |
| 346 | ListingStateChanged | sleigh | id=instruction-ram-5368714415;space=ram;address=5368714415;length=1;mnemonic=INT3;assembly=;bytes... |
| 347 | FunctionStateChanged | sleigh | id=function-5368714384;space=ram;entry=5368714384;name=shutdown_engine;end=5368714620;status=deco... |
| 348 | ListingStateChanged | sleigh | id=instruction-ram-5368713824;space=ram;address=5368713824;length=3;mnemonic=CMP;assembly=ECX,-0x... |
| 355 | ListingStateChanged | sleigh | id=instruction-ram-5368713847;space=ram;address=5368713847;length=3;mnemonic=CMOVNZ;assembly=EAX,... |
| 357 | FunctionStateChanged | sleigh | id=function-5368713824;space=ram;entry=5368713824;name=sparse_mode;end=5368713850;status=decoded; |
| 361 | ListingStateChanged | sleigh | id=instruction-ram-5368713704;space=ram;address=5368713704;length=7;mnemonic=LEA;assembly=RDX,[0x... |
| 365 | FunctionStateChanged | sleigh | id=function-5368713696;space=ram;entry=5368713696;name=switch_mode;end=5368713722;status=decoded; |
| 375 | ListingStateChanged | sleigh | id=instruction-ram-5368714366;space=ram;address=5368714366;length=1;mnemonic=RET;assembly=;bytes=... |
| 378 | ListingStateChanged | sleigh | id=instruction-ram-5368713909;space=ram;address=5368713909;length=1;mnemonic=PUSH;assembly=RDI;by... |
| 382 | ListingStateChanged | sleigh | id=instruction-ram-5368713920;space=ram;address=5368713920;length=2;mnemonic=CALL;assembly=qword ... |
| 384 | ListingStateChanged | sleigh | id=instruction-ram-5368713925;space=ram;address=5368713925;length=3;mnemonic=MOV;assembly=RCX,RBX... |
| 388 | ListingStateChanged | sleigh | id=instruction-ram-5368713939;space=ram;address=5368713939;length=5;mnemonic=MOV;assembly=RBX,qwo... |
| 390 | ListingStateChanged | sleigh | id=instruction-ram-5368713947;space=ram;address=5368713947;length=2;mnemonic=MOV;assembly=EAX,EDI... |
| 391 | ListingStateChanged | sleigh | id=instruction-ram-5368713949;space=ram;address=5368713949;length=2;mnemonic=XOR;assembly=EDX,ECX... |
| 394 | ListingStateChanged | sleigh | id=instruction-ram-5368713961;space=ram;address=5368713961;length=1;mnemonic=POP;assembly=RDI;byt... |
| 397 | ListingStateChanged | sleigh | id=instruction-ram-5368713984;space=ram;address=5368713984;length=3;mnemonic=TEST;assembly=RCX,RC... |
| 400 | ListingStateChanged | sleigh | id=instruction-ram-5368713992;space=ram;address=5368713992;length=1;mnemonic=RET;assembly=;bytes=... |
| 401 | FunctionStateChanged | sleigh | id=function-5368713984;space=ram;entry=5368713984;name=update_entity_pointer;end=5368713992;statu... |

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
| ram | 5368713552 | 5368713575 | recursive_score | decoded | sleigh |
| ram | 5368713600 | 5368713617 | mutual_alpha | decoded | sleigh |
| ram | 5368713648 | 5368713665 | mutual_beta | decoded | sleigh |
| ram | 5368713696 | 5368713722 | switch_mode | decoded | sleigh |
| ram | 5368713824 | 5368713850 | sparse_mode | decoded | sleigh |
| ram | 5368713872 | 5368713892 | invoke_callback | decoded | sleigh |
| ram | 5368713904 | 5368713962 | update_entity | decoded | sleigh |
| ram | 5368713984 | 5368713992 | update_entity_pointer | decoded | sleigh |
| ram | 5368714016 | 5368714036 | engine_abort | decoded | sleigh |
| ram | 5368714048 | 5368714081 | abort_path_one | decoded | sleigh |
| ram | 5368714096 | 5368714130 | abort_path_two | decoded | sleigh |
| ram | 5368714144 | 5368714178 | abort_path_three | decoded | sleigh |
| ram | 5368714192 | 5368714311 | resource_lookup | decoded | sleigh |
| ram | 5368714320 | 5368714366 | system_calls | decoded | sleigh |
| ram | 5368714384 | 5368714620 | shutdown_engine | decoded | sleigh |
| ram | 5368714416 | 5368714711 | engine_tick | decoded | sleigh |
| ram | 5368715072 | 5368715080 | engine_unused_dead_code | decoded | sleigh |
| ram | 5368715088 | 5368715272 | fixture_entry | decoded | sleigh |

### SQLite Instructions

- Total rows: 308
- Rows shown: 200
- Sampling: stable-random

| Space | Address | Length | Mnemonic | Assembly | Bytes | Mask | Producer | Metadata |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ram | 5368713216 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b0526200000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713225 | 6 | MOV | dword ptr [0x14000302c],EAX | 89051d200000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713248 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b0506200000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713254 | 3 | ADD | EAX,0x22 | 83c022 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713257 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905fd1f0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713263 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713280 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05e61f0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713295 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713318 | 3 | ADD | EAX,0x44 | 83c044 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713321 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905bd1f0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713327 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713338 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05ac1f0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713344 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713360 | 3 | LEA | EAX,[RCX + RCX*0x2] | 8d0449 | ffc700 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713363 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905931f0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713375 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713395 | 2 | ADD | EAX,ECX | 03c1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713416 | 4 | ADDSS | XMM0, XMM0 | f30f58c0 | ffffffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713420 | 4 | ADDSS | XMM0, XMM1 | f30f58c1 | ffffffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713424 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713440 | 3 | LEA | EAX,[RCX + 0x11] | 8d4111 | ffc000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713443 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713456 | 3 | ROL | ECX,0x3 | c1c103 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713472 | 6 | XOR | ECX,0xffa5a5a5 | 81f1a5a5a5ff | fff800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713478 | 6 | AND | ECX,0xffffff | 81e1ffffff00 | fff800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713484 | 2 | MOV | EAX,ECX | 8bc1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713486 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713520 | 8 | MULSS | XMM0, dword ptr [0x140002138] | f30f590500100000 | ffffffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713554 | 4 | SUB | RSP,0x20 | 4883ec20 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713560 | 3 | CMP | ECX,0x1 | 83f901 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713563 | 2 | JG | 0x140001168 | 7f0b | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713565 | 5 | MOV | EAX,0x1 | b801000000 | f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713570 | 4 | ADD | RSP,0x20 | 4883c420 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713574 | 1 | POP | RBX | 5b | f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713600 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713604 | 2 | TEST | ECX,ECX | 85c9 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713606 | 2 | JG | 0x140001192 | 7f0a | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713608 | 5 | MOV | EAX,0x2 | b802000000 | f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713613 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713617 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713648 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713652 | 2 | TEST | ECX,ECX | 85c9 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713654 | 2 | JG | 0x1400011c2 | 7f0a | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713656 | 5 | MOV | EAX,0x3 | b803000000 | f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713661 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713665 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713699 | 2 | JA | 0x14000122b | 7746 | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713704 | 7 | LEA | RDX,[0x140000000] | 488d1511eeffff | f8ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713711 | 7 | MOV | ECX,dword ptr [RDX + RAX*0x4 + 0x1234] | 8b8c8234120000 | ffc70000000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713721 | 2 | JMP | RCX | ffe1 | fff8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713827 | 2 | JZ | 0x140001281 | 741c | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713832 | 2 | JZ | 0x14000127b | 7411 | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713834 | 2 | XOR | EDX,EDX | 33d2 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713836 | 5 | MOV | EAX,0x3 | b803000000 | f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713847 | 3 | CMOVNZ | EAX,EDX | 0f45c2 | ffffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713850 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368713872 | 2 | MOV | EAX,ECX | 8bc1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713881 | 3 | AND | EAX,0x3 | 83e003 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713884 | 4 | MOV | RAX,qword ptr [RCX + RAX*0x8] | 488b04c1 | f8ffc700 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713890 | 3 | JMP | RAX | 48ffe0 | f8fff8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713904 | 5 | MOV | qword ptr [RSP + 0x8],RBX | 48895c2408 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713909 | 1 | PUSH | RDI | 57 | f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713910 | 4 | SUB | RSP,0x20 | 4883ec20 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713917 | 3 | MOV | RBX,RCX | 488bd9 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713920 | 2 | CALL | qword ptr [RAX] | ff10 | fff8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713922 | 3 | MOV | RDX,qword ptr [RBX] | 488b13 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713925 | 3 | MOV | RCX,RBX | 488bcb | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713928 | 2 | MOV | EDI,EAX | 8bf8 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713930 | 3 | CALL | qword ptr [RDX + 0x8] | ff5208 | fff800 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713933 | 6 | MOV | EDX,dword ptr [0x14000302c] | 8b15591d0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713944 | 3 | MOVSX | ECX,byte ptr [RAX] | 0fbe08 | ffffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713947 | 2 | MOV | EAX,EDI | 8bc7 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713949 | 2 | XOR | EDX,ECX | 33d1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713957 | 4 | ADD | RSP,0x20 | 4883c420 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713961 | 1 | POP | RDI | 5f | f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368713984 | 3 | TEST | RCX,RCX | 4885c9 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368713987 | 2 | JNZ | 0x140001309 | 7504 | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714016 | 2 | NOP |  | 6690 | ffff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714018 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05041d0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714029 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905f91c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714035 | 2 | JMP | 0x140001322 | ebed | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714048 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714052 | 6 | MOV | EAX,dword ptr [0x140003000] | 8b05b61c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714058 | 3 | CMP | EAX,-0x1 | 83f8ff | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714061 | 2 | JZ | 0x140001362 | 7413 | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714069 | 2 | INC | EAX | ffc0 | fff8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714096 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714100 | 6 | MOV | EAX,dword ptr [0x140003000] | 8b05861c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714111 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05a71c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714120 | 6 | MOV | dword ptr [0x14000302c],EAX | 89059e1c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714126 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714144 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714148 | 6 | MOV | EAX,dword ptr [0x140003000] | 8b05561c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714154 | 3 | CMP | EAX,-0x3 | 83f8fd | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714157 | 2 | JZ | 0x1400013c3 | 7414 | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714159 | 6 | MOV | EAX,dword ptr [0x14000302c] | 8b05771c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714165 | 3 | ADD | EAX,0x3 | 83c003 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714174 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714178 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714202 | 3 | XORPS | XMM0, XMM0 | 0f57c0 | ffffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714205 | 5 | LEA | R8,[RSP + 0x20] | 4c8d442420 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714215 | 6 | MOV | R9D,0x40 | 41b940000000 | f8f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714221 | 2 | XOR | ECX,ECX | 33c9 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714247 | 4 | MOVUPS | xmmword ptr [RAX + -0x28], XMM0 | 0f1140d8 | ffffc000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714267 | 5 | LEA | R8,[RSP + 0x20] | 4c8d442420 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714272 | 2 | XOR | ECX,ECX | 33c9 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714274 | 6 | MOV | dword ptr [0x14000302c],EAX | 8905041c0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714290 | 6 | MOV | ECX,dword ptr [0x14000302c] | 8b0df41b0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714296 | 2 | ADD | EAX,ECX | 03c1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714304 | 7 | ADD | RSP,0xa8 | 4881c4a8000000 | f8fff800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714320 | 4 | SUB | RSP,0x28 | 4883ec28 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714352 | 3 | ADD | RCX,RAX | 4803c8 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714362 | 4 | ADD | RSP,0x28 | 4883c428 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714366 | 1 | RET |  | c3 | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714388 | 6 | MOV | ECX,dword ptr [0x14000302c] | 8b0d921b0000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714403 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714404 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714405 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714406 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714407 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714408 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714410 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714412 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714414 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714415 | 1 | INT3 |  | cc | ff | sleigh | operands=0, flow=0, target=<none>, pcode=0 |
| ram | 5368714425 | 1 | PUSH | RBP | 55 | f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714427 | 1 | PUSH | RDI | 57 | f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714428 | 2 | PUSH | R12 | 4154 | f8f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714432 | 2 | PUSH | R14 | 4156 | f8f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714436 | 7 | SUB | RSP,0xd0 | 4881ecd0000000 | f8fff800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714443 | 2 | MOV | EAX,ECX | 8bc1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714445 | 8 | MOVAPS | xmmword ptr [RSP + 0xc0], XMM6 | 0f29b424c0000000 | ffffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714453 | 5 | XOR | EAX,0x55aa | 35aa550000 | ff00000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714471 | 3 | MOV | RBX,RDX | 488bda | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714474 | 7 | MOV | EAX,dword ptr [RSP + 0x128] | 8b842428010000 | ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714481 | 2 | MOV | EDI,ECX | 8bf9 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714483 | 7 | MOV | R8,qword ptr [0x140003030] | 4c8b05361b0000 | f8ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714490 | 5 | LEA | RCX,[RSP + 0x78] | 488d4c2478 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714498 | 5 | MOV | qword ptr [RSP + 0x30],R8 | 4c89442430 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714506 | 7 | MOV | EAX,dword ptr [RSP + 0x128] | 8b842428010000 | ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714513 | 4 | LEA | EDX,[R9 + -0x4f] | 418d51b1 | f8ffc000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714517 | 7 | MOV | byte ptr [RSP + 0x120],AL | 88842420010000 | ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714524 | 5 | CALL | 0x140001920 | e8ff030000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714529 | 6 | MOV | R9D,0x64 | 41b964000000 | f8f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714543 | 3 | MOV | R8,RBX | 4c8bc3 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714546 | 8 | LEA | RCX,[RSP + 0x98] | 488d8c2498000000 | f8ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714554 | 4 | LEA | EDX,[R9 + -0x4e] | 418d51b2 | f8ffc000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714558 | 5 | CALL | 0x140001880 | e83d030000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714563 | 6 | MOV | R9D,0x20 | 41b920000000 | f8f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714579 | 4 | LEA | R8D,[R9 + 0x20] | 458d4120 | f8ffc000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714583 | 5 | CALL | 0x1400018e0 | e884030000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714588 | 5 | MOV | EDX,0x7002 | ba02700000 | f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714593 | 5 | LEA | RCX,[RSP + 0x48] | 488d4c2448 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714598 | 6 | MOV | R8D,0x1000 | 41b800100000 | f8f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714604 | 5 | CALL | 0x1400018b0 | e83f030000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714611 | 5 | LEA | RAX,[RSP + 0x78] | 488d442478 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714621 | 3 | AND | ECX,0x1 | 83e101 | fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714624 | 8 | LEA | RAX,[RSP + 0x98] | 488d842498000000 | f8ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714632 | 2 | MOV | EDX,EDI | 8bd7 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714634 | 5 | MOV | qword ptr [RSP + 0x40],RAX | 4889442440 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714644 | 5 | CALL | 0x140001300 | e867fdffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714649 | 5 | MOV | EDX,0x5 | ba05000000 | f800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714654 | 5 | LEA | RCX,[RSP + 0x78] | 488d4c2478 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714659 | 3 | MOV | R12D,EAX | 448be0 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714662 | 5 | CALL | 0x140001a50 | e8a5040000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714680 | 5 | CALL | 0x140001a60 | e8a3040000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714685 | 2 | MOV | EBP,EDI | 8bef | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714687 | 8 | LEA | RCX,[RSP + 0x98] | 488d8c2498000000 | f8ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368714700 | 5 | CALL | 0x140001a00 | e82f040000 | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368714705 | 7 | MOV | EDX,dword ptr [RSP + 0x128] | 8b942428010000 | ffc73800000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715072 | 3 | IMUL | EAX,ECX,0x7f | 6bc17f | ffc000 | sleigh | operands=3, flow=0, target=<none>, pcode=0 |
| ram | 5368715075 | 5 | ADD | EAX,0x123 | 0523010000 | ff00000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715088 | 5 | MOV | qword ptr [RSP + 0x8],RBX | 48895c2408 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715093 | 1 | PUSH | RDI | 57 | f8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715094 | 4 | SUB | RSP,0x20 | 4883ec20 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715098 | 5 | CALL | 0x140001000 | e8a1f8ffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715108 | 5 | CALL | 0x140001040 | e8d7f8ffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715113 | 5 | CALL | 0x140001060 | e8f2f8ffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715123 | 5 | CALL | 0x140001450 | e8d8fcffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715128 | 5 | CALL | 0x140001340 | e8c3fbffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715133 | 5 | CALL | 0x140001370 | e8eefbffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715143 | 6 | MOV | ECX,dword ptr [0x140003000] | 8b0d73180000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715149 | 7 | LEA | RDI,[0x140002030] | 488d3d9c080000 | f8ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715159 | 5 | CALL | 0x1400014b0 | e814fdffff | ff00000000 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715164 | 6 | MOV | dword ptr [0x14000302c],EAX | 89058a180000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715170 | 2 | XOR | EBX,EBX | 33db | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715172 | 4 | NOP | dword ptr [RAX] | 0f1f4000 | fffff8ff | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715184 | 6 | MOV | EDX,dword ptr [0x14000302c] | 8b1576180000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715190 | 2 | MOV | ECX,EBX | 8bcb | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715197 | 6 | MOV | ECX,dword ptr [0x14000302c] | 8b0d69180000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715203 | 2 | INC | EBX | ffc3 | fff8 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715205 | 2 | XOR | EAX,ECX | 33c1 | ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715207 | 6 | MOV | dword ptr [0x14000302c],EAX | 89055f180000 | ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715216 | 2 | JC | 0x1400017b0 | 72de | ff00 | sleigh | operands=1, flow=0, target=<none>, pcode=0 |
| ram | 5368715218 | 7 | MOV | RAX,qword ptr [0x140003030] | 488b0557180000 | f8ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715235 | 5 | MOV | RBX,qword ptr [RSP + 0x30] | 488b5c2430 | f8ffc73800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715250 | 7 | MOV | RAX,qword ptr [0x140003030] | 488b0537180000 | f8ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715257 | 3 | XOR | RAX,RDI | 4833c7 | f8ffc0 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715260 | 7 | MOV | qword ptr [0x140003030],RAX | 4889052d180000 | f8ffc700000000 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |
| ram | 5368715267 | 4 | ADD | RSP,0x20 | 4883c420 | f8fff800 | sleigh | operands=2, flow=0, target=<none>, pcode=0 |

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
| analysis-3 | completed | 403 |

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
- Read revision: 403
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
- Read revision: 403
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
- Read revision: 403
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
- Read revision: 403
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
- Read revision: 403
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

- Status: `failed`
- Read revision: 403
- Signature: `C source: <none>`
- Raw instructions: 9
- Control-flow chars: 0
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c
void recursive_score() {
  /* RBX */
  /* RSP,0x20 */
  /* EBX,ECX */
  /* ECX,0x1 */
  /* 0x140001168 */
  /* EAX,0x1 */
  /* RSP,0x20 */
  /* RBX */
  /*  */
}

```

```text

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 1
- Items shown: 1

- `Native decompiler failure: Could not find op at target address: (ram,0x00014000116a)`

### `0x1400011e0` switch_mode

- Status: `failed`
- Read revision: 403
- Signature: `C source: <none>`
- Raw instructions: 7
- Control-flow chars: 0
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c
void switch_mode() {
  /* ECX,0x7 */
  /* 0x14000122b */
  /* RAX,ECX */
  /* RDX,[0x140000000] */
  /* ECX,dword ptr [RDX + RAX*0x4 + 0x1234] */
  /* RCX,RDX */
  /* RCX */
}

```

```text

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 1
- Items shown: 1

- `Native decompiler failure: Could not find op at target address: (ram,0x00014000122b)`

### `0x1400012b0` update_entity

- Status: `complete`
- Read revision: 403
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

- Status: `failed`
- Read revision: 403
- Signature: `C source: <none>`
- Raw instructions: 4
- Control-flow chars: 0
- Switches: 0
- Evidence: 0
- Cache identity: ``

```c
void update_entity_pointer() {
  /* RCX,RCX */
  /* 0x140001309 */
  /* EAX,[RCX + -0x1] */
  /*  */
}

```

```text

```

#### Recovered Variables

- Total items: 0
- Items shown: 0

- _none_

#### Diagnostics

- Total items: 1
- Items shown: 1

- `Native decompiler failure: Could not find op at target address: (ram,0x00014000130b)`

### `0x140001320` engine_abort

- Status: `complete`
- Read revision: 403
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
