| Hex bytes | x86-64 instruction | p-code |
|---|---|---|
| `48 89 5c 24 08` | `MOV qword ptr [RSP + local_res8], RBX` | `$U9d00 :8 = INT_ADD 8:8, RSP`<br>`$Ud500 :8 = COPY RBX`<br>`STORE ram ($U9d00 :8), $Ud500 :8` |
| `57` | `PUSH RDI` | `$U4f900 :8 = COPY RDI`<br>`RSP = INT_SUB RSP, 8:8`<br>`STORE ram (RSP), $U4f900 :8` |
| `48 83 ec 40` | `SUB RSP, 0x40` | `CF = INT_LESS RSP, 64:8`<br>`OF = INT_SBORROW RSP, 64:8`<br>`RSP = INT_SUB RSP, 64:8`<br>`SF = INT_SLESS RSP, 0:8`<br>`ZF = INT_EQUAL RSP, 0:8`<br>`$U58300 :8 = INT_AND RSP, 0xff:8`<br>`$U58400 :1 = POPCOUNT $U58300 :8`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `0f 29 74 24 30` | `MOVAPS xmmword ptr [RSP + local_18[0]], XMM6` | `$U9d00 :8 = INT_ADD 48:8, RSP`<br>`$Ud700 :16 = COPY XMM6`<br>`STORE ram ($U9d00 :8), $Ud700 :16` |
| `8b fa` | `MOV EDI, param_2` | `EDI = COPY EDX`<br>`RDI = INT_ZEXT EDI` |
| `48 8b d9` | `MOV RBX, param_1` | `RBX = COPY RCX` |
| `0f 29 7c 24 20` | `MOVAPS xmmword ptr [RSP + local_28[0]], XMM7` | `$U9d00 :8 = INT_ADD 32:8, RSP`<br>`$Ud700 :16 = COPY XMM7`<br>`STORE ram ($U9d00 :8), $Ud700 :16` |
| `e8 c6 84 ff ff` | `CALL FUN_14032b6f4` | `RSP = INT_SUB RSP, 8:8`<br>`STORE ram (RSP), 0x14033322e:8`<br>`CALL *[ram ]0x14032b6f4:8` |
| `48 8b cb` | `MOV param_1, RBX` | `RCX = COPY RBX` |
| `0f 28 f8` | `MOVAPS XMM7, XMM0` | `XMM7_Da = COPY XMM0_Da`<br>`XMM7_Db = COPY XMM0_Db`<br>`XMM7_Dc = COPY XMM0_Dc`<br>`XMM7_Dd = COPY XMM0_Dd` |
| `e8 a7 83 ff ff` | `CALL FUN_14032b5e0` | `RSP = INT_SUB RSP, 8:8`<br>`STORE ram (RSP), 0x140333239:8`<br>`CALL *[ram ]0x14032b5e0:8` |
| `0f 28 f0` | `MOVAPS XMM6, XMM0` | `XMM6_Da = COPY XMM0_Da`<br>`XMM6_Db = COPY XMM0_Db`<br>`XMM6_Dc = COPY XMM0_Dc`<br>`XMM6_Dd = COPY XMM0_Dd` |
| `85 ff` | `TEST EDI, EDI` | `CF = COPY 0:1`<br>`OF = COPY 0:1`<br>`$Ud3300 :4 = INT_AND EDI, EDI`<br>`SF = INT_SLESS $Ud3300 :4, 0:4`<br>`ZF = INT_EQUAL $Ud3300 :4, 0:4`<br>`$U58300 :4 = INT_AND $Ud3300 :4, 0xff:4`<br>`$U58400 :1 = POPCOUNT $U58300 :4`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `74 24` | `JZ LAB_140333264` | `CBRANCH *[ram ]0x140333264:8, ZF` |
| `f3 0f 5c f7` | `SUBSS XMM6, XMM7` | `XMM6_Da = FLOAT_SUB XMM6_Da, XMM7_Da` |
| `40 0f b6 c7` | `MOVZX EAX, DIL` | `EAX = INT_ZEXT DIL`<br>`RAX = INT_ZEXT EAX` |
| `66 0f 6e c8` | `MOVD XMM1, EAX` | `XMM1 = INT_ZEXT EAX` |
| `0f 5b c9` | `CVTDQ2PS XMM1, XMM1` | `XMM1_Da = INT2FLOAT XMM1_Da`<br>`XMM1_Db = INT2FLOAT XMM1_Db`<br>`XMM1_Dc = INT2FLOAT XMM1_Dc`<br>`XMM1_Dd = INT2FLOAT XMM1_Dd` |
| `f3 0f 59 0d 79 fd 50 01` | `MULSS XMM1, dword ptr [DAT_141842fd0]` | `XMM1_Da = FLOAT_MULT XMM1_Da, *[ram ]0x141842fd0:4` |
| `f3 0f 59 f1` | `MULSS XMM6, XMM1` | `XMM6_Da = FLOAT_MULT XMM6_Da, XMM1_Da` |
| `f3 0f 58 f7` | `ADDSS XMM6, XMM7` | `XMM6_Da = FLOAT_ADD XMM6_Da, XMM7_Da` |
| `0f 28 c6` | `MOVAPS XMM0, XMM6` | `XMM0_Da = COPY XMM6_Da`<br>`XMM0_Db = COPY XMM6_Db`<br>`XMM0_Dc = COPY XMM6_Dc`<br>`XMM0_Dd = COPY XMM6_Dd` |
| `eb 20` | `JMP LAB_140333284` | `BRANCH *[ram ]0x140333284:8` |
| `e8 a7 27 3d 01` | `CALL rand` | `RSP = INT_SUB RSP, 8:8`<br>`STORE ram (RSP), 0x140333269:8`<br>`CALL *[ram ]0x141705a10:8` |
| `f3 0f 5c f7` | `SUBSS XMM6, XMM7` | `XMM6_Da = FLOAT_SUB XMM6_Da, XMM7_Da` |
| `66 0f 6e c0` | `MOVD XMM0, EAX` | `XMM0 = INT_ZEXT EAX` |
| `0f 5b c0` | `CVTDQ2PS XMM0, XMM0` | `XMM0_Da = INT2FLOAT XMM0_Da`<br>`XMM0_Db = INT2FLOAT XMM0_Db`<br>`XMM0_Dc = INT2FLOAT XMM0_Dc`<br>`XMM0_Dd = INT2FLOAT XMM0_Dd` |
| `f3 0f 59 05 fc d1 53 01` | `MULSS XMM0, dword ptr [DAT_141870478]` | `XMM0_Da = FLOAT_MULT XMM0_Da, *[ram ]0x141870478:4` |
| `f3 0f 59 c6` | `MULSS XMM0, XMM6` | `XMM0_Da = FLOAT_MULT XMM0_Da, XMM6_Da` |
| `f3 0f 58 c7` | `ADDSS XMM0, XMM7` | `XMM0_Da = FLOAT_ADD XMM0_Da, XMM7_Da` |
| `48 8b 5c 24 50` | `MOV RBX, qword ptr [RSP + local_res8]` | `$U9d00 :8 = INT_ADD 0x50:8, RSP`<br>`$U23e00 :8 = LOAD ram ($U9d00 :8)`<br>`RBX = COPY $U23e00 :8` |
| `0f 28 74 24 30` | `MOVAPS XMM6, xmmword ptr [RSP + local_18[0]]` | `$U9d00 :8 = INT_ADD 48:8, RSP`<br>`$Ud700 :16 = LOAD ram ($U9d00 :8)`<br>`$U10e300 :16 = COPY $Ud700 :16`<br>`XMM6_Da = COPY $U10e300 :4`<br>`XMM6_Db = COPY $U10e304 :4`<br>`XMM6_Dc = COPY $U10e308 :4`<br>`XMM6_Dd = COPY $U10e30c :4` |
| `0f 28 7c 24 20` | `MOVAPS XMM7, xmmword ptr [RSP + local_28[0]]` | `$U9d00 :8 = INT_ADD 32:8, RSP`<br>`$Ud700 :16 = LOAD ram ($U9d00 :8)`<br>`$U10e300 :16 = COPY $Ud700 :16`<br>`XMM7_Da = COPY $U10e300 :4`<br>`XMM7_Db = COPY $U10e304 :4`<br>`XMM7_Dc = COPY $U10e308 :4`<br>`XMM7_Dd = COPY $U10e30c :4` |
| `48 83 c4 40` | `ADD RSP, 0x40` | `CF = INT_CARRY RSP, 64:8`<br>`OF = INT_SCARRY RSP, 64:8`<br>`RSP = INT_ADD RSP, 64:8`<br>`SF = INT_SLESS RSP, 0:8`<br>`ZF = INT_EQUAL RSP, 0:8`<br>`$U58300 :8 = INT_AND RSP, 0xff:8`<br>`$U58400 :1 = POPCOUNT $U58300 :8`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `5f` | `POP RDI` | `$Ua7200 :8 = COPY 0:8`<br>`$Ua7200 :8 = LOAD ram (RSP)`<br>`RSP = INT_ADD RSP, 8:8`<br>`RDI = COPY $Ua7200 :8` |
| `c3` | `RET` | `RIP = LOAD ram (RSP)`<br>`RSP = INT_ADD RSP, 8:8`<br>`RETURN RIP` |
| `48 89 5c 24 08` | `MOV qword ptr [RSP + local_res8], RBX` | `$U9d00 :8 = INT_ADD 8:8, RSP`<br>`$Ud500 :8 = COPY RBX`<br>`STORE ram ($U9d00 :8), $Ud500 :8` |
| `48 89 6c 24 10` | `MOV qword ptr [RSP + local_res10], RBP` | `$U9d00 :8 = INT_ADD 16:8, RSP`<br>`$Ud500 :8 = COPY RBP`<br>`STORE ram ($U9d00 :8), $Ud500 :8` |
| `48 89 74 24 18` | `MOV qword ptr [RSP + local_res18], RSI` | `$U9d00 :8 = INT_ADD 24:8, RSP`<br>`$Ud500 :8 = COPY RSI`<br>`STORE ram ($U9d00 :8), $Ud500 :8` |
| `57` | `PUSH RDI` | `$U4f900 :8 = COPY RDI`<br>`RSP = INT_SUB RSP, 8:8`<br>`STORE ram (RSP), $U4f900 :8` |
| `48 83 ec 20` | `SUB RSP, 0x20` | `CF = INT_LESS RSP, 32:8`<br>`OF = INT_SBORROW RSP, 32:8`<br>`RSP = INT_SUB RSP, 32:8`<br>`SF = INT_SLESS RSP, 0:8`<br>`ZF = INT_EQUAL RSP, 0:8`<br>`$U58300 :8 = INT_AND RSP, 0xff:8`<br>`$U58400 :1 = POPCOUNT $U58300 :8`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `49 8b f1` | `MOV RSI, param_4` | `RSI = COPY R9` |
| `41 8b d8` | `MOV EBX, param_3` | `EBX = COPY R8D`<br>`RBX = INT_ZEXT EBX` |
| `48 8b ea` | `MOV RBP, param_2` | `RBP = COPY RDX` |
| `48 8b f9` | `MOV RDI, param_1` | `RDI = COPY RCX` |
| `eb 08` | `JMP LAB_14000153a` | `BRANCH *[ram ]0x14000153a:8` |
| `48 8b cf` | `MOV param_1, RDI` | `RCX = COPY RDI` |
| `ff d6` | `CALL RSI` | `$U75000 :8 = COPY RSI`<br>`RSP = INT_SUB RSP, 8:8`<br>`STORE ram (RSP), 0x140001537:8`<br>`CALLIND $U75000 :8` |
| `48 03 fd` | `ADD RDI, RBP` | `CF = INT_CARRY RDI, RBP`<br>`OF = INT_SCARRY RDI, RBP`<br>`RDI = INT_ADD RDI, RBP`<br>`SF = INT_SLESS RDI, 0:8`<br>`ZF = INT_EQUAL RDI, 0:8`<br>`$U58300 :8 = INT_AND RDI, 0xff:8`<br>`$U58400 :1 = POPCOUNT $U58300 :8`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `ff cb` | `DEC EBX` | `OF = INT_SBORROW EBX, 1:4`<br>`EBX = INT_SUB EBX, 1:4`<br>`RBX = INT_ZEXT EBX`<br>`SF = INT_SLESS EBX, 0:4`<br>`ZF = INT_EQUAL EBX, 0:4`<br>`$U58300 :4 = INT_AND EBX, 0xff:4`<br>`$U58400 :1 = POPCOUNT $U58300 :4`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `79 f4` | `JNS LAB_140001532` | `$U25600 :1 = BOOL_NEGATE SF`<br>`CBRANCH *[ram ]0x140001532:8, $U25600 :1` |
| `48 8b 5c 24 30` | `MOV RBX, qword ptr [RSP + local_res8]` | `$U9d00 :8 = INT_ADD 48:8, RSP`<br>`$U23e00 :8 = LOAD ram ($U9d00 :8)`<br>`RBX = COPY $U23e00 :8` |
| `48 8b 6c 24 38` | `MOV RBP, qword ptr [RSP + local_res10]` | `$U9d00 :8 = INT_ADD 56:8, RSP`<br>`$U23e00 :8 = LOAD ram ($U9d00 :8)`<br>`RBP = COPY $U23e00 :8` |
| `48 8b 74 24 40` | `MOV RSI, qword ptr [RSP + local_res18]` | `$U9d00 :8 = INT_ADD 64:8, RSP`<br>`$U23e00 :8 = LOAD ram ($U9d00 :8)`<br>`RSI = COPY $U23e00 :8` |
| `48 83 c4 20` | `ADD RSP, 0x20` | `CF = INT_CARRY RSP, 32:8`<br>`OF = INT_SCARRY RSP, 32:8`<br>`RSP = INT_ADD RSP, 32:8`<br>`SF = INT_SLESS RSP, 0:8`<br>`ZF = INT_EQUAL RSP, 0:8`<br>`$U58300 :8 = INT_AND RSP, 0xff:8`<br>`$U58400 :1 = POPCOUNT $U58300 :8`<br>`$U58500 :1 = INT_AND $U58400 :1, 1:1`<br>`PF = INT_EQUAL $U58500 :1, 0:1` |
| `5f` | `POP RDI` | `$Ua7200 :8 = COPY 0:8`<br>`$Ua7200 :8 = LOAD ram (RSP)`<br>`RSP = INT_ADD RSP, 8:8`<br>`RDI = COPY $Ua7200 :8` |
| `c3` | `RET` | `RIP = LOAD ram (RSP)`<br>`RSP = INT_ADD RSP, 8:8`<br>`RETURN RIP` |
