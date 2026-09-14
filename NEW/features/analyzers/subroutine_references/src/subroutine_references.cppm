export module analyzer_subroutine_references;

import analyzer;
import std;

/// Owns the direct-call function discovery analyzer declaration and implementation.
export namespace ghidra::analyzer {
class SubroutineReferencesAnalyzer final : public Analyzer {
public:
    /// Returns the FunctionAnalyzer-compatible priority contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Creates missing direct-call targets without scanning raw bytes.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {

/// Returns the Subroutine References priority and code-event contract.
AnalyzerDescriptor SubroutineReferencesAnalyzer::descriptor() const {
    return {"Subroutine References", 399, {EventKind::code_added}, {}};
}

/// Creates functions only from already materialized direct CALL references.
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
        if (event.kind == EventKind::code_added) {
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
        if ((!changed_sources.empty() && !changed_sources.contains(reference.source)) ||
            (reference.kind != ReferenceKind::unconditional_call &&
             reference.kind != ReferenceKind::conditional_call)) {
            continue;
        }
        if ((reference.kind == ReferenceKind::unconditional_call ||
             reference.kind == ReferenceKind::conditional_call) &&
            reference.target != reference.fallthrough.value_or(0)) {
            targets.insert(reference.target);
        }
    }
    for (const Address target : targets) {
        if (const auto existing = context.function_at(target); existing) {
            // FunctionAnalyzer repairs importer-created one-instruction
            // placeholders when a real call reference supplies the target.
            if (existing->body.size() <= 1U) {
                static_cast<void>(context.disassemble_flow(target));
                static_cast<void>(context.rebuild_function_body(target));
            }
            continue;
        }
        if (context.executable_region(target)) {
            static_cast<void>(context.create_function(target));
        }
    }
}

} // namespace ghidra::analyzer
