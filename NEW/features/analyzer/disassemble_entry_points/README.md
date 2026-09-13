# Disassemble Entry Points

Ports `EntryPointAnalyzer.added()`, `doDisassembly()`, and deferred entry
processing from `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`.

## Contract

- Required state: PE executable regions and entry metadata.
- Consumes: `memory_added`, `external_added`.
- Produces: decoded instructions, direct flow references, and `code_added` events.
- Priority: `200`.
- Consumers: function discovery, reference, scalar, propagation, and no-return analyzers.

The implementation seeds the PE entry point, non-forwarded exports, TLS
callbacks, and runtime-function starts, filters to executable regions, and
follows direct flow and fall-through while preserving calls as references.
`data_only_marker`-style non-executable symbols are not decoded. No function is
created by this analyzer alone, matching the original analyzer boundary.

Tests: [`../tests/analyzer_tests.cppm`](../tests/analyzer_tests.cppm) and
[`../test_data/disassemble_entry_points/`](../test_data/disassemble_entry_points/).
