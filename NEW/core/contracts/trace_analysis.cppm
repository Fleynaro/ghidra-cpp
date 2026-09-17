export module recode.core.contracts.trace_analysis;

import std;
import recode.core.trace_analysis;
import recode.core.contracts.operation;
import recode.core.diagnostics;

export namespace recode::core::contracts {

namespace trace_analysis = recode::core;

/// Performs one offline bulk pass over a replayable execution trace.
class ITraceAnalyzer {
public:
    /// Releases the analyzer and any implementation-owned replay resources.
    virtual ~ITraceAnalyzer() = default;

    /// Computes function-entry statistics without mutating an interactive replay session.
    [[nodiscard]] virtual Task<Result<trace_analysis::FunctionCallStatistics>>
    analyze(trace_analysis::TraceAnalysisRequest request, OperationContext context) = 0;
};

} // namespace recode::core::contracts
