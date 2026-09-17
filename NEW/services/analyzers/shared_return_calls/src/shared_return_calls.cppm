export module analyzer_shared_return_calls;

import analyzer;
import std;

/// Owns the Shared Return Calls analyzer declaration and implementation.
export namespace recode::analyzer {
class SharedReturnCallsAnalyzer final : public Analyzer {
public:
    /// Returns the shared-return analyzer priority and event contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Converts eligible jumps to existing functions into CALL_RETURN flows.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Reports whether a reference is one of the jump categories considered by Ghidra.
[[nodiscard]] bool is_jump_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_jump || kind == ReferenceKind::conditional_jump;
}

/// Returns the single direct flow reference emitted by one instruction.
[[nodiscard]] const Reference* single_flow_reference(const AnalysisContext& context, Address source) {
    const Reference* result = nullptr;
    for (const auto& reference : context.references()) {
        if (reference.source != source || !is_jump_reference(reference.kind)) {
            continue;
        }
        if (result != nullptr) {
            return nullptr;
        }
        result = &reference;
    }
    return result;
}

/// Applies the SharedReturnAnalysisCmd destination-function guards to one jump.
void process_jump_to_function(AnalysisContext& context, const Reference& jump, CancellationToken& cancellation) {
    if (cancellation.is_cancelled() ||
        (jump.kind == ReferenceKind::conditional_jump && !context.options().shared_return_allow_conditional_jumps)) {
        // SharedReturnAnalyzer's default option excludes conditional branches.
        return;
    }
    const auto instruction = context.instructions().find(jump.source);
    if (instruction == context.instructions().end()) {
        return;
    }
    if (context.function_at(jump.source) != nullptr) {
        // A jump at a function entry is a thunk candidate, not a shared return.
        return;
    }
    const auto containing = context.function_containing(jump.source);
    if (containing != nullptr && containing->entry == jump.target) {
        // Internal jumps to the top of the same function are not shared returns.
        return;
    }
    if (context.function_at(jump.target) == nullptr) {
        return;
    }
    if (const auto flow = single_flow_reference(context, jump.source); flow == nullptr || flow->target != jump.target) {
        return;
    }
    if (instruction->second.instruction.flow.kind != sleigh_runtime::FlowKind::branch) {
        return;
    }
    if (context.set_flow_override(jump.source, FlowOverride::call_return, jump.target)) {
        if (containing != nullptr) {
            static_cast<void>(context.rebuild_function_body(containing->entry));
        }
    }
}

/// Returns neighboring function entries used by the contiguous-function heuristic.
[[nodiscard]] std::pair<std::optional<Address>, std::optional<Address>>
neighboring_functions(const AnalysisContext& context, Address source) {
    std::optional<Address> before;
    std::optional<Address> after;
    for (const auto& [entry, function] : context.functions()) {
        static_cast<void>(function);
        if (entry < source) {
            before = entry;
        } else if (entry > source && !after) {
            after = entry;
        }
    }
    return {before, after};
}

/// Creates functions for unconditional jumps that cross a known contiguous function boundary.
void discover_contiguous_targets(AnalysisContext& context, CancellationToken& cancellation) {
    std::vector<Reference> candidates;
    for (const auto& reference : context.references()) {
        if (reference.kind == ReferenceKind::unconditional_jump && context.function_at(reference.target) == nullptr) {
            candidates.push_back(reference);
        }
    }
    for (const auto& jump : candidates) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (jump.source == jump.target || context.image().find_memory_region(jump.target) == std::nullopt) {
            continue;
        }
        const auto [before, after] = neighboring_functions(context, jump.source);
        const bool crosses_forward = after && jump.target >= *after;
        const bool crosses_backward = before && jump.target < *before;
        if (!crosses_forward && !crosses_backward) {
            continue;
        }
        if (context.create_function(jump.target)) {
            process_jump_to_function(context, jump, cancellation);
        }
    }
}

} // namespace

/// Returns the Shared Return Calls priority and its code/function event contract.
AnalyzerDescriptor SharedReturnCallsAnalyzer::descriptor() const {
    return {"Shared Return Calls",
            398,
            {EventKind::memory_added, EventKind::code_added, EventKind::reference_added, EventKind::function_added},
            {}};
}

/// Finds jump references to functions and applies the CALL_RETURN flow override.
void SharedReturnCallsAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                        CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/SharedReturnJumpAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/app/cmd/analysis/SharedReturnAnalysisCmd.java
    // Relevant methods: added(), processFunctionJumpReferences(), getJumpRefsToFunction(),
    // getSingleFlowReferenceFrom(), and the contiguous-function scan.
    if (!context.options().shared_return_calls) {
        return;
    }
    std::vector<Reference> jumps;
    for (const auto& reference : context.references()) {
        if (is_jump_reference(reference.kind)) {
            jumps.push_back(reference);
        }
    }
    for (const auto& jump : jumps) {
        if (cancellation.is_cancelled()) {
            return;
        }
        process_jump_to_function(context, jump, cancellation);
    }
    // The Java default assumes contiguous functions, but still requires a known
    // function boundary before it creates a missing target. This pass mirrors that
    // conservative above/below-function scan without inventing targets in gaps.
    if (context.options().shared_return_assume_contiguous_functions_only) {
        discover_contiguous_targets(context, cancellation);
    }
}

} // namespace recode::analyzer
