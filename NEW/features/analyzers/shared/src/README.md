# Analyzer Core Source

This directory contains the shared analysis model and scheduler implementation. [`analyzer.cppm`](analyzer.cppm) exports the shared state, events, the base `Analyzer` contract, and `AutoAnalysisManager`, followed by their implementations in the same C++23 module; feature-specific analyzer declarations and implementations are exported by sibling modules under [`../../`](../../).
