# Ghidra Port Evidence

The scheduler maps lifecycle and dependency behavior from Ghidra `AutoAnalysisManager`, `Analyzer`, `AnalyzerType`, and `AnalysisOptions` under `Ghidra/Framework/Project/src/main/java/ghidra/app/plugin/core/analysis`. Trigger sets and prerequisites are explicit values rather than mutable manager callbacks.

The current scheduler executes canonical `IAnalyzer` services and returns mutation proposals for the project commit lane. Legacy feature analyzers remain available and fully tested while their `AnalysisContext` state is extracted incrementally.
