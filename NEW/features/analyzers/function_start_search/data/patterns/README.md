# X86 Pattern Corpus

These XML files define the masked byte patterns and action attributes consumed
by the function-start-search implementation. They are loaded recursively by
[`function_start_search.cppm`](../../src/function_start_search.cppm).

The corpus is intentionally stored beside the analyzer rather than resolved
from an installed Ghidra directory. Keep the XML files and the parser contract
in sync when adding or changing pattern behavior; validate changes with the
focused target from [`build.bat`](../../build.bat).
