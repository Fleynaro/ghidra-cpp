# Decompiler Tests

This directory contains Google Test coverage for the standalone provider frontend and the
ported native engine. [`decompiler_tests.cppm`](decompiler_tests.cppm) uses a deterministic
provider to exercise raw p-code materialization, and `Example1StringLengthWorkerWEndToEnd`
uses the explicit machine-code bytes and expected C from `TEST/dec_code_examples/1.md` without
reading Markdown at runtime. The test feeds those bytes through the compiled Sleigh fixture at
[`../../sleigh_runtime/test_data/x86-64.sla`](../../sleigh_runtime/test_data/x86-64.sla), then
runs the real native flow, SSA, actions, and C printer.

The target is registered by [`CMakeLists.txt`](CMakeLists.txt) and is invoked from the parent
feature build.
