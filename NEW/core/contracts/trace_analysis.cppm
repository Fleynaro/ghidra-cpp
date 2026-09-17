export module ghidra.core.contracts.trace_analysis;

import std;
import ghidra.core.trace_analysis;
import ghidra.core.contracts.operation;
import ghidra.core.diagnostics;

export namespace ghidra::core::contracts {

namespace trace_analysis = ghidra::core;

/// Performs one offline bulk pass over a replayable execution trace.
class ITraceAnalyzer {
public:
    /// Releases the analyzer and any implementation-owned replay resources.
    virtual ~ITraceAnalyzer() = default;

    /// Computes function-entry statistics without mutating an interactive replay session.
    [[nodiscard]] virtual Task<Result<trace_analysis::FunctionCallStatistics>> analyze(
        trace_analysis::TraceAnalysisRequest request, OperationContext context) = 0;
};

} // namespace ghidra::core::contracts
