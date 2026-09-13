# Analyzer Sources

The [`analyzer.cppm`](analyzer.cppm) module interface exports the native
analysis contracts and observable entities. [`analyzer.cpp`](analyzer.cpp)
implements state mutation, provider-backed decoding, function-body/CFG
construction, and the event scheduler. The parent build is
[`../CMakeLists.txt`](../CMakeLists.txt); implementation units for individual
analyzers live in the sibling directories under [`..`](..).

The source preserves the relevant Ghidra `AutoAnalysisManager`,
`CreateFunctionCmd`, `FollowFlow`, `SimpleBlockModel`, and `BasicBlockModel`
boundaries without depending on the Java database or GUI. It imports the PE
and Sleigh provider modules declared by the parent feature build.
