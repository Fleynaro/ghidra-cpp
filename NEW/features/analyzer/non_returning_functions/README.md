# Non-Returning Functions

Ports `FindNoReturnFunctionsAnalyzer.added()`, `detectNoReturn()`,
`targetOnlyCallsNoReturn()`, `setNoFallThru()`, and caller repair behavior from
`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/FindNoReturnFunctionsAnalyzer.java`.

## Contract

- Required state: call references, fall-through instructions, and functions.
- Consumes: code, function, and flow events.
- Produces: no-return functions, CALL_RETURN overrides, bookmarks, and flow events.
- Priority: `302`.
- Consumers: Function Body and control-flow clients.

Known PE/runtime and imported names are normalized before matching. Discovered
no-return functions require the configurable default threshold of three
independent post-call INT3 or UD2 indicators. Explicit call fall-through
metadata, including external IAT references, is converted to a `CALL_RETURN`
override, preserving the observable Ghidra state.

Golden evidence: [`../test_data/non_returning_functions_known/`](../test_data/non_returning_functions_known/)
and [`../test_data/non_returning_functions_discovered/`](../test_data/non_returning_functions_discovered/).
