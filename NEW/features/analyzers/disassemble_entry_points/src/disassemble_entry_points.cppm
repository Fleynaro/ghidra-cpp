export module analyzer_disassemble_entry_points;

import analyzer;
import std;

/// Owns the entry-point disassembly analyzer declaration and implementation.
export namespace ghidra::analyzer {
class DisassembleEntryPointsAnalyzer final : public Analyzer {
public:
    /// Returns the Ghidra analyzer name and block-analysis priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Processes new memory and external-entry events.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {

/// Returns the EntryPointAnalyzer name and its byte-analysis priority.
AnalyzerDescriptor DisassembleEntryPointsAnalyzer::descriptor() const {
    return {"Disassemble Entry Points", 200, {EventKind::memory_added, EventKind::external_added}, {}};
}

/// Disassembles PE entry points and metadata-derived executable seeds.
void DisassembleEntryPointsAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                             CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java
    // Relevant methods: added(), doDisassembly(), processDoLaterSet().
    if (!context.options().disassemble_entry_points) {
        return;
    }
    std::set<Address> seeds;
    // AutoAnalysisManager converts PE entry metadata and explicit caller seeds
    // into memory events. Consuming those addresses, rather than rebuilding a
    // global metadata set here, preserves EntryPointAnalyzer's event scope.
    for (const auto& event : events) {
        if (event.kind != EventKind::memory_added) {
            continue;
        }
        seeds.insert(event.addresses.begin(), event.addresses.end());
    }
    for (const Address seed : seeds) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (context.can_disassemble(seed)) {
            static_cast<void>(context.disassemble_flow(seed));
        }
    }
}

} // namespace ghidra::analyzer
