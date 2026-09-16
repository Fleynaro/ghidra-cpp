export module ghidra.core.contracts.function_id;

import std;
import ghidra.core.diagnostics;
import ghidra.core.function;
import ghidra.core.function_id;
import ghidra.core.contracts.operation;

export namespace ghidra::core::contracts {

/// Matches one immutable function snapshot against configured FID resources.
class IFunctionIdMatcher {
public:
    /// Releases a Function ID matcher through its contract.
    virtual ~IFunctionIdMatcher() = default;

    /// Queues relation-aware matching work and returns scored candidates.
    [[nodiscard]] virtual Task<Result<FunctionIdResult>> identify(FunctionSnapshot function, FunctionIdOptions options,
                                                                  OperationContext context) = 0;
};

} // namespace ghidra::core::contracts
