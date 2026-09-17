export module analyzer_subroutine_references;

import analyzer;
import std;

/// Owns the direct-call function discovery analyzer declaration and implementation.
export namespace recode::analyzer {
class SubroutineReferencesAnalyzer final : public Analyzer {
public:
    /// Returns the FunctionAnalyzer-compatible priority contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Creates missing direct-call targets without scanning raw bytes.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {

/// Returns the Subroutine References priority and code/reference event contract.
AnalyzerDescriptor SubroutineReferencesAnalyzer::descriptor() const {
    return {
        "Subroutine References", 399, {EventKind::code_added, EventKind::reference_added, EventKind::flow_changed}, {}};
}

/// Creates functions only from already materialized call references.
void SubroutineReferencesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                           CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java
    // Relevant methods: added() and fallthroughCall(). Raw PE bytes are never scanned here.
    if (!context.options().subroutine_references) {
        return;
    }
    std::set<Address> changed_sources;
    for (const auto& event : events) {
        if (event.kind == EventKind::code_added || event.kind == EventKind::reference_added ||
            event.kind == EventKind::flow_changed) {
            changed_sources.insert(event.addresses.begin(), event.addresses.end());
        }
    }
    if (!events.empty() && changed_sources.empty()) {
        return;
    }
    std::set<Address> targets;
    for (const auto& reference : context.references()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!changed_sources.empty() && !changed_sources.contains(reference.source)) {
            continue;
        }
        const auto instruction = context.instructions().find(reference.source);
        if (instruction == context.instructions().end() ||
            (instruction->second.instruction.flow.kind != sleigh_runtime::FlowKind::conditional_call &&
             instruction->second.instruction.flow.kind != sleigh_runtime::FlowKind::call &&
             instruction->second.instruction.flow.kind != sleigh_runtime::FlowKind::indirect_call)) {
            continue;
        }
        if (reference.kind != ReferenceKind::unconditional_call && reference.kind != ReferenceKind::conditional_call &&
            reference.kind != ReferenceKind::computed_call && reference.kind != ReferenceKind::external) {
            continue;
        }
        // FunctionAnalyzer.fallthroughCall() compares the actual fall-through
        // address.  A missing fall-through is not address zero and must not be
        // converted into a synthetic comparison value.
        if (!reference.fallthrough || reference.target != *reference.fallthrough) {
            targets.insert(reference.target);
        }
    }
    for (const Address target : targets) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (const auto existing = context.function_at(target); existing) {
            // FunctionAnalyzer repairs importer-created one-instruction
            // placeholders when a real call reference supplies the target.
            const auto target_instruction = context.instructions().find(target);
            if (existing->body.size() == 1U && target_instruction != context.instructions().end() &&
                (target_instruction->second.instruction.length > 1U ||
                 !target_instruction->second.instruction.flow.terminal)) {
                // This is FunctionAnalyzer's precise one-address placeholder
                // predicate, not a blanket rewrite of real one-byte returns.
                static_cast<void>(context.disassemble_flow(target));
                static_cast<void>(context.rebuild_function_body(target));
            }
            continue;
        }
        if (context.options().create_only_thunks && !context.thunk_target(target)) {
            // FunctionAnalyzer.createOnlyThunks performs this filter after
            // removing existing function symbols, so placeholder repair above
            // remains active while ordinary missing targets are ignored.
            continue;
        }
        // CreateFunctionCmd only succeeds when a code unit already exists at
        // the target.  Do not guess through an unmapped, data, or merely
        // executable-but-undecoded address.
        if (context.instructions().contains(target) && context.executable_region(target)) {
            static_cast<void>(context.create_function(target));
        }
    }
}

} // namespace recode::analyzer
