module analyzer;

import std;

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
    for (const auto& event : events) {
        seeds.insert(event.addresses.begin(), event.addresses.end());
    }
    if (const auto entry = context.image().entry_point_va(); entry) {
        seeds.insert(*entry);
    }
    for (const auto& symbol : context.image().exported_symbols()) {
        if (!symbol.forwarded) {
            seeds.insert(symbol.address_va);
        }
    }
    if (const auto& tls = context.image().tls(); tls) {
        seeds.insert(tls->callback_addresses.begin(), tls->callback_addresses.end());
    }
    for (const auto& runtime : context.image().exception_functions()) {
        seeds.insert(runtime.begin_va);
    }
    for (const Address seed : seeds) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (context.image().is_executable(seed)) {
            static_cast<void>(context.disassemble_flow(seed));
        }
    }
}

} // namespace ghidra::analyzer
