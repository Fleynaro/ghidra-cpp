# Decompiler Tests

This directory contains Google Test coverage for the standalone provider frontend and the
ported native engine. [`decompiler_tests.cppm`](decompiler_tests.cppm) uses a deterministic
provider to exercise raw p-code materialization. `Example1StringLengthWorkerWEndToEnd`,
`Example2BsearchEndToEnd`, `Example3TypedChildCallEndToEnd`,
`Example4VectorConstructorIteratorEndToEnd`, `Example5SlotAppendEndToEnd`, and
`Example6MetricSpentOnBankInterestEndToEnd` through `Example11GlobalSlotRegistrationEndToEnd`
use explicit machine-code bytes and expected C from their corresponding examples without
reading Markdown at runtime. The tests feed those bytes through the compiled Sleigh fixture at
[`../../sleigh_runtime/test_data/x86-64.sla`](../../sleigh_runtime/test_data/x86-64.sla), then
run the real native flow, SSA, actions, and C printer.

[`native_funcproto_tests.cppm`](native_funcproto_tests.cppm) independently initializes a Toy-like
native `Architecture`, decodes the three compiler-spec models used by
[`Ghidra/Features/Decompiler/src/decompile/unittests/testfuncproto.cc`](../../../../Ghidra/Features/Decompiler/src/decompile/unittests/testfuncproto.cc),
and drives the real `ghidra.decompiler.grammar`, `ProtoModel`, and `ParamActive` algorithms.
Its table-driven cases preserve the original storage-assignment and input-recovery scenarios
without depending on the legacy test harness.

[`native_paramstore_tests.cppm`](native_paramstore_tests.cppm) ports
`Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc` without its XML
capability or test harness. It supplies processor spaces and register aliases through a
provider translator, loads the corresponding authoritative compiler specifications from
[`../../../../Ghidra/Processors`](../../../../Ghidra/Processors), and invokes the exported
`ghidra::parse_C`, `ghidra::parse_protopieces`, and `ghidra::ProtoModel::assignParameterStorage`
algorithms directly. The x64, PPC64 big-endian, MIPS32 big-endian, and AArch64 cases are
kept as exact storage assertions; endian-aware subregister justification and architecture-
specific joins are not skipped or weakened.

[`decompiler_architecture_tests.cppm`](decompiler_architecture_tests.cppm) adds a focused
provider-boundary inventory for MIPS32, ARM32, AArch64, PPC32, 68000, 8051, Toy, and x86-32.
It constructs every `ArchitectureDescription` without external files, uses a provider-only
Toy context because the installed distribution has no Toy SLA, and decodes one real installed
SLA per remaining family when `GHIDRA_INSTALL_DIR` is set. The test resolves only the relative
paths under that environment variable; it does not embed a workstation path or copy every SLA
variant. The selected source mappings are recorded in
[`../../sleigh_runtime/test_data/README.md`](../../sleigh_runtime/test_data/README.md).

The target is registered by [`CMakeLists.txt`](CMakeLists.txt) and is invoked from the parent
feature build.

The additional native ports are split by responsibility: [`native_scalar_tests.cppm`](native_scalar_tests.cppm)
covers `testmultiprec.cc` and `testfloatemu.cc`; [`native_marshal_tests.cppm`](native_marshal_tests.cppm)
covers `testmarshal.cc`; [`native_circlerange_tests.cppm`](native_circlerange_tests.cppm) covers
`testcirclerange.cc`; and [`native_type_tests.cppm`](native_type_tests.cppm) covers `testtypes.cc`.
The shared decompiler target also includes [`decompiler_datatests.cppm`](decompiler_datatests.cppm),
which uses embedded x86-64 bytes and the real `Decompiler`, `SleighPcodeProvider`, SSA, and C printer.
Its focused provider-metadata cases additionally exercise recursive structure fields and fixed arrays,
overlapping unions, named enum constants, packed bitfields, provider volatile ranges, counted-loop
restructuring, and ordered aggregate storage. The aggregate cases cover `concatsplit.xml`,
`piecestruct.xml`, `multiret.xml`, `stackreturn.xml`, and `mixfloatint.xml` through real x86 Sleigh
decoding and the native join, call, SSA, and C-printer algorithms. The volatile case uses the production
`MemoryProvider::volatile_ranges()` contract and keeps an unused LOAD alive as required by the native
side-effect model.

[`metadata_provider_tests.cppm`](metadata_provider_tests.cppm) drives the same native pipeline with a
provider-only p-code body. It verifies recursive typedef, array, structure, union, enum, and bitfield
materialization, a namespaced address-tied data symbol with a forced hexadecimal display format, and
an offcut load through that mapped data object.

`decompiler_datatests.cppm` contains the executable category ports and a manifest of all 89 original
XML datatests. The manifest records the remaining cases that require legacy XML commands, non-x86
processor contexts, injection/override commands, or database-backed global mappings; aggregate fields,
arrays, unions, bitfields, enums, volatile ranges, ordered ABI pieces, and control flow now have focused
embedded-provider coverage where the public boundary can express their semantics. No original XML
fixture or legacy test runner is loaded at runtime.

The compiled SLA files are binary artifacts and are marked `binary` in the repository root
`.gitattributes`; this prevents Windows line-ending conversion from corrupting zlib streams. The
fixture paths are resolved relative to the CTest working directory configured in [`../CMakeLists.txt`](../CMakeLists.txt).
The feature-level build configuration is defined by [`../../CMakeLists.txt`](../../CMakeLists.txt), while the
Google Test dependency comes from [`../../../vcpkg.json`](../../../vcpkg.json).

The focused string ports in `decompiler_datatests.cppm` cover the narrow and UTF-16 heap-store
patterns from `heapstring.xml` cases #1 and #6, plus the narrow and native UTF-32 stack-array
patterns from `stackstring.xml` cases #2 and #6. Each case feeds compact x86-64 bytes through
`SleighPcodeProvider` and checks the real `StringManager`/`HeapSequence` result, including
`builtin_strncpy`, `builtin_memcpy`, or `builtin_wcsncpy`, the recovered literal, and its count.
