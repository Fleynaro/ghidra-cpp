module analyzer;

import std;

namespace ghidra::analyzer {

/// Returns the Subroutine References priority and code-event contract.
AnalyzerDescriptor SubroutineReferencesAnalyzer::descriptor() const {
    return {"Subroutine References", 399, {EventKind::code_added, EventKind::reference_added}, {}};
}

/// Creates functions only from already materialized direct CALL references.
void SubroutineReferencesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                           CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java
    // Relevant methods: added() and fallthroughCall(). Raw PE bytes are never scanned here.
    if (!context.options().subroutine_references) {
        return;
    }
    std::set<Address> targets;
    for (const auto& reference : context.references()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if ((reference.kind == ReferenceKind::unconditional_call ||
             reference.kind == ReferenceKind::conditional_call) &&
            reference.target != reference.fallthrough.value_or(0)) {
            targets.insert(reference.target);
        }
    }
    for (const Address target : targets) {
        if (!context.functions().contains(target) && !context.function_containing(target) &&
            context.executable_region(target)) {
            static_cast<void>(context.create_function(target));
        }
    }
}

} // namespace ghidra::analyzer
