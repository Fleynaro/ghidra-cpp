# Analyzer Core Source

This directory contains the shared analysis model and scheduler implementation. [`analyzer.cppm`](analyzer.cppm) exports only shared state, events, the base `Analyzer` contract, and `AutoAnalysisManager`; feature-specific analyzer declarations and implementations are exported by sibling modules under [`../../`](../../). [`analyzer.cpp`](analyzer.cpp) provides the corresponding shared definitions.
