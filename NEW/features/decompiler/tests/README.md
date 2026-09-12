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

The target is registered by [`CMakeLists.txt`](CMakeLists.txt) and is invoked from the parent
feature build.

The additional native ports are split by responsibility: [`native_scalar_tests.cppm`](native_scalar_tests.cppm)
covers `testmultiprec.cc` and `testfloatemu.cc`; [`native_marshal_tests.cppm`](native_marshal_tests.cppm)
covers `testmarshal.cc`; [`native_circlerange_tests.cppm`](native_circlerange_tests.cppm) covers
`testcirclerange.cc`; and [`native_type_tests.cppm`](native_type_tests.cppm) covers `testtypes.cc`.
The shared decompiler target also includes [`decompiler_datatests.cppm`](decompiler_datatests.cppm),
which uses embedded x86-64 bytes and the real `Decompiler`, `SleighPcodeProvider`, SSA, and C printer.

`decompiler_datatests.cppm` contains 16 executable category ports and a manifest of all 89 original
XML datatests. The manifest records cases that require legacy XML commands or provider metadata not
yet exposed by the standalone boundary, including non-x86 processor contexts, injection/override
commands, volatile ranges, bitfield/enum metadata, and database-backed global mappings. No original
XML fixture or legacy test runner is loaded at runtime.

The compiled SLA files are binary artifacts and are marked `binary` in the repository root
`.gitattributes`; this prevents Windows line-ending conversion from corrupting zlib streams. The
fixture paths are resolved relative to the CTest working directory configured in [`../CMakeLists.txt`](../CMakeLists.txt).
The feature-level build configuration is defined by [`../../CMakeLists.txt`](../../CMakeLists.txt), while the
Google Test dependency comes from [`../../../vcpkg.json`](../../../vcpkg.json).
