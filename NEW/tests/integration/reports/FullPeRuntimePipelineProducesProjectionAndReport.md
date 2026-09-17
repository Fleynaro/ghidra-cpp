# Full Runtime PE Pipeline Report

- **Status:** PASS
- **Fixture:** `services/analyzers/tests/data/test_analyzers_integration.exe`
- **SQLite projection:** `build/test-reports/project/projection.sqlite`
- **Load revision:** 401
- **Analysis revision:** 403
- **SQLite checkpoint:** 403

## Pipeline

1. `ProjectFacade::open` created the project and event/projection stores.
2. `ProjectFacade::load` parsed the PE, materialized PE regions/symbols, decoded entry/export seeds, and committed listing/function events.
3. `ProjectFacade::analyze` executed the registered runtime analyzer(s).
4. Two `Task<Result<Decompilation>>` operations completed through the shared worker pool.

### Executed Analyzers

- `runtime.entry_materialization`

### Analysis Diagnostics

- _none_

## Coverage Notes

- The current native runtime registry executes `runtime.entry_materialization`; the broader legacy analyzer suite is validated separately by `analyzer_global_integration_tests` and is not implicitly claimed by this facade test.
- References and data objects are represented by durable tables and are currently empty for this runtime event set.

## In-Memory Query Projection

- Revision: 403
- Functions: 31
- Instructions: 308
- Memory regions: 7
- Symbols: 53
- References: unavailable/empty in the current runtime contract implementation
- Data objects: 0

### SQLite Event Types

| Event type | Rows |
| --- | --- |
| AnalysisRunStateChanged | 2 |
| FunctionStateChanged | 31 |
| ListingStateChanged | 308 |
| MemoryStateChanged | 7 |
| ProjectCreated | 1 |
| ProjectInputsChanged | 1 |
| SymbolStateChanged | 53 |

### SQLite Memory Regions

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

| Space | Address | Length | Mnemonic | Assembly | Producer |
| --- | --- | --- | --- | --- | --- |
| ram | 5368713216 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713222 | 3 | ADD | EAX,0x11 | sleigh |
| ram | 5368713225 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368713231 | 1 | RET |  | sleigh |
| ram | 5368713248 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713254 | 3 | ADD | EAX,0x22 | sleigh |
| ram | 5368713257 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368713263 | 1 | RET |  | sleigh |
| ram | 5368713280 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713286 | 3 | ADD | EAX,0x33 | sleigh |
| ram | 5368713289 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368713295 | 1 | RET |  | sleigh |
| ram | 5368713312 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713318 | 3 | ADD | EAX,0x44 | sleigh |
| ram | 5368713321 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368713327 | 1 | RET |  | sleigh |
| ram | 5368713328 | 4 | LEA | EAX,[RDX + R8*0x1] | sleigh |
| ram | 5368713332 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368713338 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713344 | 1 | RET |  | sleigh |
| ram | 5368713360 | 3 | LEA | EAX,[RCX + RCX*0x2] | sleigh |
| ram | 5368713363 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368713369 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713375 | 1 | RET |  | sleigh |
| ram | 5368713392 | 3 | LEA | EAX,[RDX + 0x7] | sleigh |
| ram | 5368713395 | 2 | ADD | EAX,ECX | sleigh |
| ram | 5368713397 | 1 | RET |  | sleigh |
| ram | 5368713408 | 8 | MULSS | XMM1, dword ptr [0x140002130] | sleigh |
| ram | 5368713416 | 4 | ADDSS | XMM0, XMM0 | sleigh |
| ram | 5368713420 | 4 | ADDSS | XMM0, XMM1 | sleigh |
| ram | 5368713424 | 1 | RET |  | sleigh |
| ram | 5368713440 | 3 | LEA | EAX,[RCX + 0x11] | sleigh |
| ram | 5368713443 | 1 | RET |  | sleigh |
| ram | 5368713456 | 3 | ROL | ECX,0x3 | sleigh |
| ram | 5368713459 | 2 | MOV | EAX,ECX | sleigh |
| ram | 5368713461 | 1 | RET |  | sleigh |
| ram | 5368713472 | 6 | XOR | ECX,0xffa5a5a5 | sleigh |
| ram | 5368713478 | 6 | AND | ECX,0xffffff | sleigh |
| ram | 5368713484 | 2 | MOV | EAX,ECX | sleigh |
| ram | 5368713486 | 1 | RET |  | sleigh |
| ram | 5368713504 | 3 | LEA | EAX,[RDX + 0x21] | sleigh |
| ram | 5368713507 | 2 | ADD | EAX,ECX | sleigh |
| ram | 5368713509 | 1 | RET |  | sleigh |
| ram | 5368713520 | 8 | MULSS | XMM0, dword ptr [0x140002138] | sleigh |
| ram | 5368713528 | 8 | MULSS | XMM1, dword ptr [0x140002134] | sleigh |
| ram | 5368713536 | 4 | ADDSS | XMM0, XMM1 | sleigh |
| ram | 5368713540 | 1 | RET |  | sleigh |
| ram | 5368713552 | 2 | PUSH | RBX | sleigh |
| ram | 5368713554 | 4 | SUB | RSP,0x20 | sleigh |
| ram | 5368713558 | 2 | MOV | EBX,ECX | sleigh |
| ram | 5368713560 | 3 | CMP | ECX,0x1 | sleigh |
| ram | 5368713563 | 2 | JG | 0x140001168 | sleigh |
| ram | 5368713565 | 5 | MOV | EAX,0x1 | sleigh |
| ram | 5368713570 | 4 | ADD | RSP,0x20 | sleigh |
| ram | 5368713574 | 1 | POP | RBX | sleigh |
| ram | 5368713575 | 1 | RET |  | sleigh |
| ram | 5368713600 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368713604 | 2 | TEST | ECX,ECX | sleigh |
| ram | 5368713606 | 2 | JG | 0x140001192 | sleigh |
| ram | 5368713608 | 5 | MOV | EAX,0x2 | sleigh |
| ram | 5368713613 | 4 | ADD | RSP,0x28 | sleigh |
| ram | 5368713617 | 1 | RET |  | sleigh |
| ram | 5368713648 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368713652 | 2 | TEST | ECX,ECX | sleigh |
| ram | 5368713654 | 2 | JG | 0x1400011c2 | sleigh |
| ram | 5368713656 | 5 | MOV | EAX,0x3 | sleigh |
| ram | 5368713661 | 4 | ADD | RSP,0x28 | sleigh |
| ram | 5368713665 | 1 | RET |  | sleigh |
| ram | 5368713696 | 3 | CMP | ECX,0x7 | sleigh |
| ram | 5368713699 | 2 | JA | 0x14000122b | sleigh |
| ram | 5368713701 | 3 | MOVSXD | RAX,ECX | sleigh |
| ram | 5368713704 | 7 | LEA | RDX,[0x140000000] | sleigh |
| ram | 5368713711 | 7 | MOV | ECX,dword ptr [RDX + RAX*0x4 + 0x1234] | sleigh |
| ram | 5368713718 | 3 | ADD | RCX,RDX | sleigh |
| ram | 5368713721 | 2 | JMP | RCX | sleigh |
| ram | 5368713824 | 3 | CMP | ECX,-0x64 | sleigh |
| ram | 5368713827 | 2 | JZ | 0x140001281 | sleigh |
| ram | 5368713829 | 3 | CMP | ECX,0x4d | sleigh |
| ram | 5368713832 | 2 | JZ | 0x14000127b | sleigh |
| ram | 5368713834 | 2 | XOR | EDX,EDX | sleigh |
| ram | 5368713836 | 5 | MOV | EAX,0x3 | sleigh |
| ram | 5368713841 | 6 | CMP | ECX,0x3e8 | sleigh |
| ram | 5368713847 | 3 | CMOVNZ | EAX,EDX | sleigh |
| ram | 5368713850 | 1 | RET |  | sleigh |
| ram | 5368713872 | 2 | MOV | EAX,ECX | sleigh |
| ram | 5368713874 | 7 | LEA | RCX,[0x1400021b0] | sleigh |
| ram | 5368713881 | 3 | AND | EAX,0x3 | sleigh |
| ram | 5368713884 | 4 | MOV | RAX,qword ptr [RCX + RAX*0x8] | sleigh |
| ram | 5368713888 | 2 | MOV | ECX,EDX | sleigh |
| ram | 5368713890 | 3 | JMP | RAX | sleigh |
| ram | 5368713904 | 5 | MOV | qword ptr [RSP + 0x8],RBX | sleigh |
| ram | 5368713909 | 1 | PUSH | RDI | sleigh |
| ram | 5368713910 | 4 | SUB | RSP,0x20 | sleigh |
| ram | 5368713914 | 3 | MOV | RAX,qword ptr [RCX] | sleigh |
| ram | 5368713917 | 3 | MOV | RBX,RCX | sleigh |
| ram | 5368713920 | 2 | CALL | qword ptr [RAX] | sleigh |
| ram | 5368713922 | 3 | MOV | RDX,qword ptr [RBX] | sleigh |
| ram | 5368713925 | 3 | MOV | RCX,RBX | sleigh |
| ram | 5368713928 | 2 | MOV | EDI,EAX | sleigh |
| ram | 5368713930 | 3 | CALL | qword ptr [RDX + 0x8] | sleigh |
| ram | 5368713933 | 6 | MOV | EDX,dword ptr [0x14000302c] | sleigh |
| ram | 5368713939 | 5 | MOV | RBX,qword ptr [RSP + 0x30] | sleigh |
| ram | 5368713944 | 3 | MOVSX | ECX,byte ptr [RAX] | sleigh |
| ram | 5368713947 | 2 | MOV | EAX,EDI | sleigh |
| ram | 5368713949 | 2 | XOR | EDX,ECX | sleigh |
| ram | 5368713951 | 6 | MOV | dword ptr [0x14000302c],EDX | sleigh |
| ram | 5368713957 | 4 | ADD | RSP,0x20 | sleigh |
| ram | 5368713961 | 1 | POP | RDI | sleigh |
| ram | 5368713962 | 1 | RET |  | sleigh |
| ram | 5368713984 | 3 | TEST | RCX,RCX | sleigh |
| ram | 5368713987 | 2 | JNZ | 0x140001309 | sleigh |
| ram | 5368713989 | 3 | LEA | EAX,[RCX + -0x1] | sleigh |
| ram | 5368713992 | 1 | RET |  | sleigh |
| ram | 5368714016 | 2 | NOP |  | sleigh |
| ram | 5368714018 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368714024 | 5 | XOR | EAX,0xdeadbeef | sleigh |
| ram | 5368714029 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368714035 | 2 | JMP | 0x140001322 | sleigh |
| ram | 5368714048 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368714052 | 6 | MOV | EAX,dword ptr [0x140003000] | sleigh |
| ram | 5368714058 | 3 | CMP | EAX,-0x1 | sleigh |
| ram | 5368714061 | 2 | JZ | 0x140001362 | sleigh |
| ram | 5368714063 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368714069 | 2 | INC | EAX | sleigh |
| ram | 5368714071 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368714077 | 4 | ADD | RSP,0x28 | sleigh |
| ram | 5368714081 | 1 | RET |  | sleigh |
| ram | 5368714096 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368714100 | 6 | MOV | EAX,dword ptr [0x140003000] | sleigh |
| ram | 5368714106 | 3 | CMP | EAX,-0x2 | sleigh |
| ram | 5368714109 | 2 | JZ | 0x140001393 | sleigh |
| ram | 5368714111 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368714117 | 3 | ADD | EAX,0x2 | sleigh |
| ram | 5368714120 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368714126 | 4 | ADD | RSP,0x28 | sleigh |
| ram | 5368714130 | 1 | RET |  | sleigh |
| ram | 5368714144 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368714148 | 6 | MOV | EAX,dword ptr [0x140003000] | sleigh |
| ram | 5368714154 | 3 | CMP | EAX,-0x3 | sleigh |
| ram | 5368714157 | 2 | JZ | 0x1400013c3 | sleigh |
| ram | 5368714159 | 6 | MOV | EAX,dword ptr [0x14000302c] | sleigh |
| ram | 5368714165 | 3 | ADD | EAX,0x3 | sleigh |
| ram | 5368714168 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368714174 | 4 | ADD | RSP,0x28 | sleigh |
| ram | 5368714178 | 1 | RET |  | sleigh |
| ram | 5368714192 | 3 | MOV | RAX,RSP | sleigh |
| ram | 5368714195 | 7 | SUB | RSP,0xa8 | sleigh |
| ram | 5368714202 | 3 | XORPS | XMM0, XMM0 | sleigh |
| ram | 5368714205 | 5 | LEA | R8,[RSP + 0x20] | sleigh |
| ram | 5368714210 | 5 | MOVUPS | xmmword ptr [RSP + 0x20], XMM0 | sleigh |
| ram | 5368714215 | 6 | MOV | R9D,0x40 | sleigh |
| ram | 5368714221 | 2 | XOR | ECX,ECX | sleigh |
| ram | 5368714223 | 4 | MOVUPS | xmmword ptr [RAX + -0x78], XMM0 | sleigh |
| ram | 5368714227 | 4 | MOVUPS | xmmword ptr [RAX + -0x68], XMM0 | sleigh |
| ram | 5368714231 | 4 | LEA | EDX,[R9 + 0x25] | sleigh |
| ram | 5368714235 | 4 | MOVUPS | xmmword ptr [RAX + -0x58], XMM0 | sleigh |
| ram | 5368714239 | 4 | MOVUPS | xmmword ptr [RAX + -0x48], XMM0 | sleigh |
| ram | 5368714243 | 4 | MOVUPS | xmmword ptr [RAX + -0x38], XMM0 | sleigh |
| ram | 5368714247 | 4 | MOVUPS | xmmword ptr [RAX + -0x28], XMM0 | sleigh |
| ram | 5368714251 | 4 | MOVUPS | xmmword ptr [RAX + -0x18], XMM0 | sleigh |
| ram | 5368714255 | 6 | CALL | qword ptr [0x140002020] | sleigh |
| ram | 5368714261 | 6 | MOV | R9D,0x40 | sleigh |
| ram | 5368714267 | 5 | LEA | R8,[RSP + 0x20] | sleigh |
| ram | 5368714272 | 2 | XOR | ECX,ECX | sleigh |
| ram | 5368714274 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368714280 | 4 | LEA | EDX,[R9 + 0x26] | sleigh |
| ram | 5368714284 | 6 | CALL | qword ptr [0x140002020] | sleigh |
| ram | 5368714290 | 6 | MOV | ECX,dword ptr [0x14000302c] | sleigh |
| ram | 5368714296 | 2 | ADD | EAX,ECX | sleigh |
| ram | 5368714298 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368714304 | 7 | ADD | RSP,0xa8 | sleigh |
| ram | 5368714311 | 1 | RET |  | sleigh |
| ram | 5368714320 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368714324 | 7 | LEA | RCX,[0x140002030] | sleigh |
| ram | 5368714331 | 6 | CALL | qword ptr [0x140002010] | sleigh |
| ram | 5368714337 | 6 | CALL | qword ptr [0x140002000] | sleigh |
| ram | 5368714343 | 7 | MOV | RCX,qword ptr [0x140003030] | sleigh |
| ram | 5368714350 | 2 | MOV | EAX,EAX | sleigh |
| ram | 5368714352 | 3 | ADD | RCX,RAX | sleigh |
| ram | 5368714355 | 7 | MOV | qword ptr [0x140003030],RCX | sleigh |
| ram | 5368714362 | 4 | ADD | RSP,0x28 | sleigh |
| ram | 5368714366 | 1 | RET |  | sleigh |
| ram | 5368714384 | 4 | SUB | RSP,0x28 | sleigh |
| ram | 5368714388 | 6 | MOV | ECX,dword ptr [0x14000302c] | sleigh |
| ram | 5368714394 | 6 | CALL | qword ptr [0x140002008] | sleigh |
| ram | 5368714400 | 1 | INT3 |  | sleigh |
| ram | 5368714401 | 1 | INT3 |  | sleigh |
| ram | 5368714402 | 1 | INT3 |  | sleigh |
| ram | 5368714403 | 1 | INT3 |  | sleigh |
| ram | 5368714404 | 1 | INT3 |  | sleigh |
| ram | 5368714405 | 1 | INT3 |  | sleigh |
| ram | 5368714406 | 1 | INT3 |  | sleigh |
| ram | 5368714407 | 1 | INT3 |  | sleigh |
| ram | 5368714408 | 1 | INT3 |  | sleigh |
| ram | 5368714409 | 1 | INT3 |  | sleigh |
| ram | 5368714410 | 1 | INT3 |  | sleigh |
| ram | 5368714411 | 1 | INT3 |  | sleigh |
| ram | 5368714412 | 1 | INT3 |  | sleigh |
| ram | 5368714413 | 1 | INT3 |  | sleigh |
| ram | 5368714414 | 1 | INT3 |  | sleigh |
| ram | 5368714415 | 1 | INT3 |  | sleigh |
| ram | 5368714416 | 5 | MOV | qword ptr [RSP + 0x10],RBX | sleigh |
| ram | 5368714421 | 4 | MOV | dword ptr [RSP + 0x8],ECX | sleigh |
| ram | 5368714425 | 1 | PUSH | RBP | sleigh |
| ram | 5368714426 | 1 | PUSH | RSI | sleigh |
| ram | 5368714427 | 1 | PUSH | RDI | sleigh |
| ram | 5368714428 | 2 | PUSH | R12 | sleigh |
| ram | 5368714430 | 2 | PUSH | R13 | sleigh |
| ram | 5368714432 | 2 | PUSH | R14 | sleigh |
| ram | 5368714434 | 2 | PUSH | R15 | sleigh |
| ram | 5368714436 | 7 | SUB | RSP,0xd0 | sleigh |
| ram | 5368714443 | 2 | MOV | EAX,ECX | sleigh |
| ram | 5368714445 | 8 | MOVAPS | xmmword ptr [RSP + 0xc0], XMM6 | sleigh |
| ram | 5368714453 | 5 | XOR | EAX,0x55aa | sleigh |
| ram | 5368714458 | 6 | MOV | R9D,0x5a | sleigh |
| ram | 5368714464 | 7 | MOV | dword ptr [RSP + 0x128],EAX | sleigh |
| ram | 5368714471 | 3 | MOV | RBX,RDX | sleigh |
| ram | 5368714474 | 7 | MOV | EAX,dword ptr [RSP + 0x128] | sleigh |
| ram | 5368714481 | 2 | MOV | EDI,ECX | sleigh |
| ram | 5368714483 | 7 | MOV | R8,qword ptr [0x140003030] | sleigh |
| ram | 5368714490 | 5 | LEA | RCX,[RSP + 0x78] | sleigh |
| ram | 5368714495 | 3 | ADD | R8,RAX | sleigh |
| ram | 5368714498 | 5 | MOV | qword ptr [RSP + 0x30],R8 | sleigh |
| ram | 5368714503 | 3 | MOV | R8,RDX | sleigh |
| ram | 5368714506 | 7 | MOV | EAX,dword ptr [RSP + 0x128] | sleigh |
| ram | 5368714513 | 4 | LEA | EDX,[R9 + -0x4f] | sleigh |
| ram | 5368714517 | 7 | MOV | byte ptr [RSP + 0x120],AL | sleigh |
| ram | 5368714524 | 5 | CALL | 0x140001920 | sleigh |
| ram | 5368714529 | 6 | MOV | R9D,0x64 | sleigh |
| ram | 5368714535 | 8 | MOV | dword ptr [RSP + 0x20],0x1234 | sleigh |
| ram | 5368714543 | 3 | MOV | R8,RBX | sleigh |
| ram | 5368714546 | 8 | LEA | RCX,[RSP + 0x98] | sleigh |
| ram | 5368714554 | 4 | LEA | EDX,[R9 + -0x4e] | sleigh |
| ram | 5368714558 | 5 | CALL | 0x140001880 | sleigh |
| ram | 5368714563 | 6 | MOV | R9D,0x20 | sleigh |
| ram | 5368714569 | 5 | LEA | RCX,[RSP + 0x60] | sleigh |
| ram | 5368714574 | 5 | MOV | EDX,0x7001 | sleigh |
| ram | 5368714579 | 4 | LEA | R8D,[R9 + 0x20] | sleigh |
| ram | 5368714583 | 5 | CALL | 0x1400018e0 | sleigh |
| ram | 5368714588 | 5 | MOV | EDX,0x7002 | sleigh |
| ram | 5368714593 | 5 | LEA | RCX,[RSP + 0x48] | sleigh |
| ram | 5368714598 | 6 | MOV | R8D,0x1000 | sleigh |
| ram | 5368714604 | 5 | CALL | 0x1400018b0 | sleigh |
| ram | 5368714609 | 2 | MOV | ECX,EDI | sleigh |
| ram | 5368714611 | 5 | LEA | RAX,[RSP + 0x78] | sleigh |
| ram | 5368714616 | 5 | MOV | qword ptr [RSP + 0x38],RAX | sleigh |
| ram | 5368714621 | 3 | AND | ECX,0x1 | sleigh |
| ram | 5368714624 | 8 | LEA | RAX,[RSP + 0x98] | sleigh |
| ram | 5368714632 | 2 | MOV | EDX,EDI | sleigh |
| ram | 5368714634 | 5 | MOV | qword ptr [RSP + 0x40],RAX | sleigh |
| ram | 5368714639 | 5 | MOV | RCX,qword ptr [RSP + RCX*0x8 + 0x38] | sleigh |
| ram | 5368714644 | 5 | CALL | 0x140001300 | sleigh |
| ram | 5368714649 | 5 | MOV | EDX,0x5 | sleigh |
| ram | 5368714654 | 5 | LEA | RCX,[RSP + 0x78] | sleigh |
| ram | 5368714659 | 3 | MOV | R12D,EAX | sleigh |
| ram | 5368714662 | 5 | CALL | 0x140001a50 | sleigh |
| ram | 5368714667 | 5 | MOV | EDX,0x5678 | sleigh |
| ram | 5368714672 | 8 | LEA | RCX,[RSP + 0x98] | sleigh |
| ram | 5368714680 | 5 | CALL | 0x140001a60 | sleigh |
| ram | 5368714685 | 2 | MOV | EBP,EDI | sleigh |
| ram | 5368714687 | 8 | LEA | RCX,[RSP + 0x98] | sleigh |
| ram | 5368714695 | 3 | AND | EBP,0x3 | sleigh |
| ram | 5368714698 | 2 | MOV | EDX,EBP | sleigh |
| ram | 5368714700 | 5 | CALL | 0x140001a00 | sleigh |
| ram | 5368714705 | 7 | MOV | EDX,dword ptr [RSP + 0x128] | sleigh |
| ram | 5368715072 | 3 | IMUL | EAX,ECX,0x7f | sleigh |
| ram | 5368715075 | 5 | ADD | EAX,0x123 | sleigh |
| ram | 5368715080 | 1 | RET |  | sleigh |
| ram | 5368715088 | 5 | MOV | qword ptr [RSP + 0x8],RBX | sleigh |
| ram | 5368715093 | 1 | PUSH | RDI | sleigh |
| ram | 5368715094 | 4 | SUB | RSP,0x20 | sleigh |
| ram | 5368715098 | 5 | CALL | 0x140001000 | sleigh |
| ram | 5368715103 | 5 | CALL | 0x140001020 | sleigh |
| ram | 5368715108 | 5 | CALL | 0x140001040 | sleigh |
| ram | 5368715113 | 5 | CALL | 0x140001060 | sleigh |
| ram | 5368715118 | 5 | CALL | 0x1400013d0 | sleigh |
| ram | 5368715123 | 5 | CALL | 0x140001450 | sleigh |
| ram | 5368715128 | 5 | CALL | 0x140001340 | sleigh |
| ram | 5368715133 | 5 | CALL | 0x140001370 | sleigh |
| ram | 5368715138 | 5 | CALL | 0x1400013a0 | sleigh |
| ram | 5368715143 | 6 | MOV | ECX,dword ptr [0x140003000] | sleigh |
| ram | 5368715149 | 7 | LEA | RDI,[0x140002030] | sleigh |
| ram | 5368715156 | 3 | MOV | RDX,RDI | sleigh |
| ram | 5368715159 | 5 | CALL | 0x1400014b0 | sleigh |
| ram | 5368715164 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368715170 | 2 | XOR | EBX,EBX | sleigh |
| ram | 5368715172 | 4 | NOP | dword ptr [RAX] | sleigh |
| ram | 5368715176 | 8 | NOP | dword ptr [RAX + RAX*0x1] | sleigh |
| ram | 5368715184 | 6 | MOV | EDX,dword ptr [0x14000302c] | sleigh |
| ram | 5368715190 | 2 | MOV | ECX,EBX | sleigh |
| ram | 5368715192 | 5 | CALL | 0x140001290 | sleigh |
| ram | 5368715197 | 6 | MOV | ECX,dword ptr [0x14000302c] | sleigh |
| ram | 5368715203 | 2 | INC | EBX | sleigh |
| ram | 5368715205 | 2 | XOR | EAX,ECX | sleigh |
| ram | 5368715207 | 6 | MOV | dword ptr [0x14000302c],EAX | sleigh |
| ram | 5368715213 | 3 | CMP | EBX,0x4 | sleigh |
| ram | 5368715216 | 2 | JC | 0x1400017b0 | sleigh |
| ram | 5368715218 | 7 | MOV | RAX,qword ptr [0x140003030] | sleigh |
| ram | 5368715225 | 10 | MOV | RCX,0x102030405060708 | sleigh |
| ram | 5368715235 | 5 | MOV | RBX,qword ptr [RSP + 0x30] | sleigh |
| ram | 5368715240 | 3 | XOR | RAX,RCX | sleigh |
| ram | 5368715243 | 7 | MOV | qword ptr [0x140003030],RAX | sleigh |
| ram | 5368715250 | 7 | MOV | RAX,qword ptr [0x140003030] | sleigh |
| ram | 5368715257 | 3 | XOR | RAX,RDI | sleigh |
| ram | 5368715260 | 7 | MOV | qword ptr [0x140003030],RAX | sleigh |
| ram | 5368715267 | 4 | ADD | RSP,0x20 | sleigh |
| ram | 5368715271 | 1 | POP | RDI | sleigh |
| ram | 5368715272 | 1 | RET |  | sleigh |

### SQLite Symbols

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

| Run | Status | Sequence |
| --- | --- | --- |
| analysis-3 | completed | 403 |

### SQLite References

| Source | Target | Kind |
| --- | --- | --- |
| _none_ |

### SQLite Data Objects

| Address | Type | Value |
| --- | --- | --- |
| _none_ |

## Decompilations

### `0x1400010e0` callback_add

- **Status:** complete
- **Read revision:** 403
- **Raw instructions:** 2

```c

int4 default callback_add(int4 param_1)

{
  return param_1 + 0x11;
}

```

### `0x1400010f0` callback_rotate

- **Status:** complete
- **Read revision:** 403
- **Raw instructions:** 3

```c

uint4 default callback_rotate(uint4 param_1)

{
  return param_1 << 3 | param_1 >> 0x1d;
}

```
