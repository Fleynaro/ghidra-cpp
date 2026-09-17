# Function Start Search Data

This directory contains runtime data owned by the function-start-search
analyzer. The [`patterns/`](patterns/) directory is the bundled x86 pattern
corpus loaded by [`src/function_start_search.cppm`](../src/function_start_search.cppm)
when `AnalysisOptions::pattern_root` is not set.

The data is packaged inside this project, so analysis and tests do not depend on an
external installation. The feature target declares
the data location in [`CMakeLists.txt`](../CMakeLists.txt), and the focused
test consumes the same repository-local corpus.
