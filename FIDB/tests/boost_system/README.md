# Boost.System Test

[`test.cpp`](test.cpp) creates real Boost.System error codes, categories, messages, and conditions. Boost 1.86 exposes only a dummy compiled function while these APIs are header-only, so the original Ghidra pipeline intentionally emits and verifies a valid empty FIDB with zero false matches.
