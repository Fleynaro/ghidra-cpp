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

The target is registered by [`CMakeLists.txt`](CMakeLists.txt) and is invoked from the parent
feature build.
