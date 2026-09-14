export module analyzer_data_reference;

import analyzer;
import std;

/// Owns the data-origin reference analyzer declaration and implementation.
export namespace ghidra::analyzer {
class DataReferenceAnalyzer final : public Analyzer {
public:
    /// Returns the DataOperandReferenceAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Resolves mapped pointer values without creating functions from them.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

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
    // data cells. The target does not have to be relocated as well: a pointer
    // may legitimately reference a string, code, import, or ordinary data.
    std::map<Address, std::uint32_t> relocated_data_cells;
    for (const auto& block : context.image().relocations()) {
        for (const auto& relocation : block.entries) {
            if (relocation.type == 0 || !relocation.target_file_offset) {
                continue;
            }
            const std::uint32_t cell_size = relocation.type == 3 ? 4U : pointer_size;
            if (!context.image().is_executable(relocation.target_va)) {
                relocated_data_cells.emplace(relocation.target_va, cell_size);
            }
        }
    }
    const auto process_pointer = [&](Address source, std::uint32_t size) {
        const auto value = context.image().read_memory(source, size);
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
    for (const auto& [source, size] : relocated_data_cells) {
        if (cancellation.is_cancelled())
            return;
        const auto bytes = context.image().read_memory(source, size);
        if (!bytes)
            continue;
        std::uint64_t target = 0;
        for (std::size_t index = 0; index < bytes->size(); ++index)
            target |= static_cast<std::uint64_t>((*bytes)[index]) << (index * 8U);
        std::optional<Address> target_address;
        if (context.image().find_memory_region(target)) {
            target_address = target;
        } else if (const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(target)); translated) {
            target_address = *translated;
        }
        if (target_address && context.image().find_memory_region(*target_address)) {
            static_cast<void>(context.add_data(DataObject{source, size, "relocated pointer"}));
            static_cast<void>(context.add_reference(Reference{source, *target_address, ReferenceKind::data,
                                                              std::nullopt, std::nullopt, FlowOverride::none, true}));
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
            process_pointer(address + offset, pointer_size);
        }
    }
}

} // namespace ghidra::analyzer
