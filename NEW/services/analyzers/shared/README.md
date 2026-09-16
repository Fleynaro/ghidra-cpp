# Shared Analyzer Runtime

This module owns the common analysis model and event-driven scheduler used by every analyzer submodule. The historical [`analyzer`](src/analyzer.cppm) module is a compatibility umbrella that re-exports the split modules below; each runtime class keeps its declaration and implementation together in one `.cppm` file.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) contributes all shared modules to `NewGhidra::AnalyzerShared` and includes [`test_support/`](test_support/) and [`tests/`](tests/).
- [`src/analyzer_types.cppm`](src/analyzer_types.cppm) defines the observable address, listing, function, event, option, and result value types and re-exports [`../../pe_loader`](../../pe_loader) and [`../../sleigh_runtime`](../../sleigh_runtime).
- [`src/analyzer_cancellation_token.cppm`](src/analyzer_cancellation_token.cppm) defines `CancellationToken` with atomic cooperative cancellation state.
- [`src/analyzer_base.cppm`](src/analyzer_base.cppm) defines the abstract `Analyzer` callback contract.
- [`src/analyzer_context.cppm`](src/analyzer_context.cppm) defines `AnalysisContext`, including PE/Sleigh ownership, disassembly, references, functions, CFGs, strings, symbols, datatype archives, address tables, embedded media, PDB records, signatures, and observable state mutation.
- [`src/analyzer_registry.cppm`](src/analyzer_registry.cppm) defines `AnalyzerRegistry` and duplicate-name validation.
- [`src/analyzer_manager.cppm`](src/analyzer_manager.cppm) defines `AutoAnalysisManager` and its deterministic event scheduler. Built-in feature registration is supplied by [`../analyzer_builtin.cpp`](../analyzer_builtin.cpp) through the aggregate target adapter to avoid a module import cycle.
- [`test_support/analyzer_test_support.cppm`](test_support/analyzer_test_support.cppm) contains shared GoogleTest fixture helpers and test analyzers.
- [`tests/function_body_tests.cppm`](tests/function_body_tests.cppm) verifies shared `AnalysisContext` body, CFG, overlap, thunk, flow-repair, string, signature, and shared-return behavior; its dedicated PE/oracle is under [`tests/data/`](tests/data/).

Feature-specific implementations are sibling directories under [`../`](../), and their tests and fixture data remain isolated from this shared runtime.
