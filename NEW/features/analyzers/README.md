# Analyzer Engine

This directory contains the provider-backed C++23 analysis engine. It owns the
native program/listing state, the event queue, the deterministic priority
scheduler, and the first ten analyzer families. It does not parse PE files or
decode instructions itself.

## Navigation

- [`CMakeLists.txt`](CMakeLists.txt) builds `NewGhidra::Analyzer` and its tests.
- Each implemented analyzer `CMakeLists.txt` defines a separate static `analyzer_<name>` library; `analyzer` is only the aggregate application target.
- [`build.bat`](build.bat) builds and tests only this module through the parent script; tests are enabled by default.
- [`shared/src/analyzer.cppm`](shared/src/analyzer.cppm) defines `AnalysisContext`, state entities, events, the base `Analyzer` contract, and `AutoAnalysisManager`; concrete analyzer declarations are exported by their own feature modules.
- [`shared/src/analyzer.cpp`](shared/src/analyzer.cpp) implements provider-backed state mutation, flow/body construction, and scheduling.
- [`shared/test_support/`](shared/test_support/) contains shared C++23 test helpers and [`shared/tests/`](shared/tests/) contains shared infrastructure tests and fixtures.
- [`disassemble_entry_points/`](disassemble_entry_points/) ports entry-point disassembly.
- [`function_start_search/`](function_start_search/) ports the pre/function/post pattern phases.
- [`subroutine_references/`](subroutine_references/) ports call-driven function discovery.
- [`function_body/`](function_body/) contains focused tests for the `CreateFunctionCmd`/`FollowFlow` body logic implemented in shared context; it is not a separate upstream-style analyzer.
- [`reference/`](reference/) ports instruction operand-reference creation.
- [`data_reference/`](data_reference/) ports data-origin pointer references.
- [`scalar_operand_references/`](scalar_operand_references/) ports scalar address-reference filtering.
- [`stack/`](stack/) ports stack-frame/local-variable discovery.
- [`constant_propagation/`](constant_propagation/) ports the p-code symbolic propagation pass.
- [`non_returning_functions/`](non_returning_functions/) ports known/evidence-backed no-return analysis.
- Each concrete analyzer source is an independent exported module, such as [`constant_propagation/src/constant_propagation.cppm`](constant_propagation/src/constant_propagation.cppm) exporting `analyzer_constant_propagation`; consumers must import the feature module rather than relying on `analyzer` to expose concrete classes.
- Each implemented analyzer owns its Google Test module under its own `tests/` directory; for example [`disassemble_entry_points/tests/disassemble_entry_points_tests.cppm`](disassemble_entry_points/tests/disassemble_entry_points_tests.cppm). Function body/CFG tests are the documented exception because Ghidra creates bodies synchronously in shared `AnalysisContext`; they live under [`shared/tests/function_body_tests.cppm`](shared/tests/function_body_tests.cppm).
- Each analyzer directory contains its own `src/` when implemented, `CMakeLists.txt`, `build.bat`, and `tests/data/` for executable fixtures and Ghidra golden reports. Additional fixture sets can be added as files or subdirectories there.

Run `build.bat` from this directory for the focused analyzer build and test. Use `build.bat --no-test` for a compile-only check and `build.bat --clean` only when a clean rebuild is required.

## Contracts

`AnalysisContext` owns a `pe::LoadedPeImage`, a `sleigh_runtime::Decoder`, decoded
instructions, flow/data references, functions, bodies, blocks, CFG edges, data,
stack variables, constants, bookmarks, and pending events. Mutations emit
events only when observable state changes. `Analyzer` implementations declare a
name, numeric priority, event triggers, and prerequisites. `AutoAnalysisManager`
coalesces events per analyzer, executes lower priorities first, breaks ties by
name, validates prerequisite priorities, limits runaway custom event producers,
and dispatches events produced by a callback only after that callback returns.
This mirrors the important `AnalysisTaskList` and `AnalysisScheduler`
invariants from Ghidra.

The providers are [`../pe_loader/`](../pe_loader/) and
[`../sleigh_runtime/`](../sleigh_runtime/). The loader supplies sections,
permissions, entry points, exports, TLS, runtime-function starts, and mapped
bytes. Sleigh supplies instruction lengths, operands, flow, and p-code. No
second PE parser or instruction decoder is present here.

## Pipeline

The built-in priorities are: known no-return names `97`, pre-function patterns
`199`, entry disassembly `200`, discovered no-return detection `302`,
subroutine/function creation `399`, ordinary pattern search `402`,
function-constrained patterns `498`, constant propagation `596`, scalar
references `598`, references `600`, data references `602`, post-code/data
pattern phases `898`, and stack `903`. Function bodies and CFGs are constructed
synchronously by function creation rather than by a priority-400 analyzer.
Options can disable any phase without changing registration or scheduling.
Known and discovered no-return phases can also be disabled independently. Entry
disassembly honors `AnalysisOptions::respect_execute_flag`; the no-return phase
loads `Ghidra/Features/Base/data/PEFunctionsThatDoNotReturn` or the explicit
`AnalysisOptions::no_return_names_file` path.

## Provenance and fidelity

Each implementation file names the original Ghidra class and methods it ports.
The source behavior is based on:

- `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java`
- `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`
- `Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartAnalyzer.java`
- `Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java`
- `Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java`
- `Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/FollowFlow.java`
- `Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/SimpleBlockModel.java`
- `Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java`

The current native implementation intentionally exposes unresolved indirect
flow as unresolved Sleigh flow and supports the compiled SLA profiles present
in the repository (`x86-64.sla` and `ARM8_le.sla`). It does not claim to support
architectures for which no provider profile exists. Ghidra's Java database,
GUI, options service, cancellation monitor, and transaction layer are replaced
by the explicit C++ contracts above.

Known fidelity boundaries are explicit: XML loading handles masked
hex/binary patterns, marked patternpairs, section/alignment, after/valid-code,
thunk/no-return, and delayed phases, but does not yet expose every Sleigh
context-register assignment as a public pattern property. Constant propagation
uses CFG block worklists and conservative joins;
unresolved architecture - specific p - code operations remain unknown.The x86 - 32 golden constant -
    reference fixture is not executed because the repository does not contain an x86 -
    32 SLA profile.These cases are documented limitations,
    not silently reported as equivalent behavior.

        ##Fixtures and
        validation

                The checked -
            in fixture reports are behavioral evidence used during port development.The Google Tests do not read report
                files at runtime : expected function body ranges,
    references, stack offsets,
    and negative outcomes are copied into typed C++ test constants and compared structurally.The current tests cover
            entry disassembly,
    direct - call function creation / CFG, shared bodies, pattern constraints, scalar filtering, imports,
    removal / end lifecycle, registry behavior, event generation, priority order, downstream scheduling, stack,
    constant, data, and no - return analysis.
