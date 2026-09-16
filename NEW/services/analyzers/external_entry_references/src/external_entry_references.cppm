export module analyzer_external_entry_references;

import analyzer;
import std;

// Original source:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/ExternalEntryFunctionAnalyzer.java

/// Creates functions at executable PE exports that are already decoded and are not fall-through targets.
export namespace ghidra::analyzer {
class ExternalEntryReferencesAnalyzer final : public Analyzer {
public:
    /// Returns the original name and CODE_ANALYSIS.before().before() priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Converts eligible exported external entries into named functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Returns whether an instruction immediately before an address falls through into it.
[[nodiscard]] bool falls_through_to(const AnalysisContext& context, Address address) {
    for (const auto& [start, record] : context.instructions()) {
        if (record.instruction.length != 0 && start + record.instruction.length == address &&
            record.instruction.flow.has_fallthrough) {
            return true;
        }
    }
    return false;
}

} // namespace

/// Returns the ExternalEntryFunctionAnalyzer priority and memory/code event contract.
AnalyzerDescriptor ExternalEntryReferencesAnalyzer::descriptor() const {
    return {"External Entry References", 398, {EventKind::memory_added, EventKind::code_added}, {}};
}

/// Applies Ghidra's instruction-start and no-fall-through eligibility checks before function creation.
void ExternalEntryReferencesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                              CancellationToken& cancellation) {
    if (!context.options().external_entry_references) {
        return;
    }
    for (const auto& exported : context.image().exported_symbols()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (exported.forwarded || !exported.name) {
            continue;
        }
        static_cast<void>(context.add_external_entry(exported.address_va));
        if (!context.image().is_executable(exported.address_va)) {
            continue;
        }
        if (!context.instructions().contains(exported.address_va)) {
            static_cast<void>(context.disassemble_flow(exported.address_va));
        }
        if (!context.instructions().contains(exported.address_va)) {
            continue;
        }
        if (context.function_at(exported.address_va) || falls_through_to(context, exported.address_va)) {
            continue;
        }
        static_cast<void>(context.create_function(exported.address_va, *exported.name));
    }

    // The PE loader exposes the image entry point separately from the named export table,
    // while Ghidra's symbol table presents both as external entry points.
    if (const auto entry = context.image().entry_point_va(); entry && !cancellation.is_cancelled() &&
                                                             context.image().is_executable(*entry) &&
                                                             context.instructions().contains(*entry)) {
        static_cast<void>(context.add_external_entry(*entry));
        if (!context.function_at(*entry) && !falls_through_to(context, *entry)) {
            static_cast<void>(context.create_function(*entry));
        }
    }
}

} // namespace ghidra::analyzer
