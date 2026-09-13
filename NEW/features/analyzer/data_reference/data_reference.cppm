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
    // IMAGE_BASE_RELOCATION entries are the loader's concrete evidence for
    // pointer-sized data cells; arbitrary non-executable bytes are not data.
    std::set<Address> relocated_data_cells;
    for (const auto& block : context.image().relocations()) {
        for (const auto& relocation : block.entries) {
            if (relocation.type == 0)
                continue;
            const auto rva = block.page_rva + (relocation.raw_value & 0x0fffU);
            const auto address = context.image().rva_to_va(rva);
            if (address && !context.image().is_executable(*address))
                relocated_data_cells.insert(*address);
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
            static_cast<void>(context.add_reference(
                Reference{source, target, ReferenceKind::data, std::nullopt, std::nullopt, FlowOverride::none, true}));
        }
    };
    for (const Address source : relocated_data_cells) {
        if (cancellation.is_cancelled())
            return;
        const auto bytes = context.image().read_memory(source, pointer_size);
        if (!bytes)
            continue;
        std::uint64_t target = 0;
        for (std::size_t index = 0; index < bytes->size(); ++index)
            target |= static_cast<std::uint64_t>((*bytes)[index]) << (index * 8U);
        if (relocated_data_cells.contains(target)) {
            static_cast<void>(context.add_data(DataObject{source, pointer_size, "relocated pointer"}));
            static_cast<void>(context.add_data(DataObject{target, pointer_size, "relocated pointer target"}));
            static_cast<void>(context.add_reference(
                Reference{source, target, ReferenceKind::data, std::nullopt, std::nullopt, FlowOverride::none, true}));
        }
    }
    for (const auto& [address, data] : context.data()) {
        if (relocated_data_cells.contains(address))
            continue;
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
