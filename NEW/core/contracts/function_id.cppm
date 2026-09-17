export module recode.core.contracts.function_id;

import std;
import recode.core.diagnostics;
import recode.core.function;
import recode.core.function_id;
import recode.core.contracts.operation;

export namespace recode::core::contracts {

/// Matches one immutable function snapshot against configured FID resources.
class IFunctionIdMatcher {
public:
    /// Releases a Function ID matcher through its contract.
    virtual ~IFunctionIdMatcher() = default;

    /// Queues relation-aware matching work and returns scored candidates.
    [[nodiscard]] virtual Task<Result<FunctionIdResult>> identify(FunctionSnapshot function, FunctionIdOptions options,
                                                                  OperationContext context) = 0;
};

} // namespace recode::core::contracts
