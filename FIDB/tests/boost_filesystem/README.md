# Boost.Filesystem Test

[`test.cpp`](test.cpp) performs real directory creation, file output, status, recursive iteration, and removal operations. [`../run_boost.py`](../run_boost.py) builds it against the pinned Boost.Filesystem static library and validates its Ghidra Function ID matches plus the negative control.
