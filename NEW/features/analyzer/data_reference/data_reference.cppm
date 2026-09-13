module analyzer;

import std;

namespace ghidra::analyzer {

/// Returns the Data Reference analyzer priority and data-event contract.
AnalyzerDescriptor DataReferenceAnalyzer::descriptor() const {
    return {"Data Reference", 602, {EventKind::data_added, EventKind::memory_added}, {"Reference"}};
}

/// Follows pointer-valued data without promoting data origins to functions.
void DataReferenceAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                    CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/DataOperandReferenceAnalyzer.java
    // Relevant method: createFunctions(), intentionally a no-op for data-origin pointers.
    if (!context.options().data_reference) {
        return;
    }
    const std::uint32_t pointer_size = context.image().optional_header().pe32_plus ? 8U : 4U;
    for (const auto& region : context.image().memory_regions()) {
        if (!region.executable && region.size != 0 && !context.data().contains(region.start)) {
            const auto size = static_cast<std::uint32_t>(
                std::min<std::uint64_t>(region.size, std::numeric_limits<std::uint32_t>::max()));
            context.add_data(DataObject{region.start, size, "PE data section"});
        }
    }
    const auto process_pointer = [&](Address source) {
        const auto value = context.image().read_memory(source, pointer_size);
        if (!value) {
            return;
        }
        std::uint64_t target = 0;
        for (std::size_t index = 0; index < value->size(); ++index) {
            target |= static_cast<std::uint64_t>((*value)[index]) << (index * 8U);
        }
        if (context.image().find_memory_region(target)) {
            context.add_reference(
                Reference{source, target, ReferenceKind::data, std::nullopt, std::nullopt, FlowOverride::none, true});
        }
    };
    for (const auto& [address, data] : context.data()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (data.size < pointer_size) {
            continue;
        }
        for (std::uint64_t offset = 0; offset + pointer_size <= data.size; offset += pointer_size) {
            if (cancellation.is_cancelled()) {
                return;
            }
            process_pointer(address + offset);
        }
    }
}

} // namespace ghidra::analyzer
