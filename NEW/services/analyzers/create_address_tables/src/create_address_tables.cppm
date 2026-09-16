export module analyzer_create_address_tables;

import analyzer;
import std;

/// Owns the Create Address Tables analyzer declaration and implementation.
export namespace ghidra::analyzer {
class CreateAddressTablesAnalyzer final : public Analyzer {
public:
    /// Returns the AddressTableAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Finds contiguous valid pointer runs and materializes their table records.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Reads one little-endian PE pointer from a mapped address.
[[nodiscard]] std::optional<std::uint64_t> read_pointer(const AnalysisContext& context, Address address,
                                                        std::uint32_t size) {
    const auto bytes = context.image().read_memory(address, size);
    if (!bytes || bytes->size() != size) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < bytes->size(); ++index) {
        value |= static_cast<std::uint64_t>((*bytes)[index]) << (index * 8U);
    }
    return value;
}

/// Resolves an absolute pointer or an image-relative PE pointer into mapped memory.
[[nodiscard]] std::optional<Address> resolve_target(const AnalysisContext& context, std::uint64_t value) {
    if (const auto region = context.image().find_memory_region(value)) {
        static_cast<void>(region);
        return value;
    }
    const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(value));
    if (translated && context.image().find_memory_region(*translated)) {
        return *translated;
    }
    return std::nullopt;
}

/// Reports whether an address is inside an existing instruction or non-pointer data object.
[[nodiscard]] bool conflicts_with_listing(const AnalysisContext& context, Address address, bool allow_data_pointer) {
    for (const auto& [start, record] : context.instructions()) {
        if (address >= start && address < start + record.instruction.length) {
            return true;
        }
    }
    for (const auto& [start, data] : context.data()) {
        if (address < start || address >= start + data.size) {
            continue;
        }
        if (!allow_data_pointer || data.type != "pointer") {
            return true;
        }
    }
    return false;
}

/// Rejects pointers into the middle of an instruction unless offcut references are enabled.
[[nodiscard]] bool invalid_offcut_target(const AnalysisContext& context, Address target, bool allow_offcut) {
    if (allow_offcut) {
        return false;
    }
    for (const auto& [start, record] : context.instructions()) {
        if (target > start && target < start + record.instruction.length) {
            return true;
        }
    }
    return false;
}

/// Reports whether a relocation exists at a pointer cell.
[[nodiscard]] bool has_relocation_at(const AnalysisContext& context, Address address) {
    for (const auto& block : context.image().relocations()) {
        for (const auto& relocation : block.entries) {
            if (relocation.target_va == address) {
                return true;
            }
        }
    }
    return false;
}

/// Converts an address to a deterministic table-label suffix.
[[nodiscard]] std::string address_suffix(Address address) {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << address;
    return stream.str();
}

/// Scans one table candidate and returns the valid entries before its first break.
[[nodiscard]] std::vector<Address> read_table(const AnalysisContext& context, Address start,
                                              const pe::MemoryRegion& region, std::uint32_t pointer_size,
                                              const AnalysisOptions& options, bool relocation_backed,
                                              CancellationToken& cancellation) {
    std::vector<Address> targets;
    Address cursor = start;
    std::optional<Address> previous;
    while (!cancellation.is_cancelled()) {
        const auto offset = cursor - region.start;
        if (offset > region.size || pointer_size > region.size - offset) {
            break;
        }
        if (conflicts_with_listing(context, cursor, true)) {
            break;
        }
        if (options.address_table_relocation_guide && relocation_backed && !has_relocation_at(context, cursor)) {
            break;
        }
        const auto raw = read_pointer(context, cursor, pointer_size);
        if (!raw || *raw == 0U) {
            break;
        }
        const auto target = resolve_target(context, *raw);
        if (!target || *target < options.address_table_minimum_pointer_address ||
            *target % std::max<std::uint32_t>(1U, options.address_table_pointer_alignment) != 0U ||
            invalid_offcut_target(context, *target, options.address_table_allow_offcuts)) {
            break;
        }
        if (previous) {
            const auto difference = *previous >= *target ? *previous - *target : *target - *previous;
            if (difference > options.address_table_maximum_distance) {
                break;
            }
        }
        targets.push_back(*target);
        previous = *target;
        if (cursor > std::numeric_limits<Address>::max() - pointer_size) {
            break;
        }
        cursor += pointer_size;
    }
    return targets;
}

} // namespace

/// Returns the Create Address Tables priority and memory/data-event contract.
AnalyzerDescriptor CreateAddressTablesAnalyzer::descriptor() const {
    return {"Create Address Tables", 899, {EventKind::memory_added, EventKind::data_added, EventKind::code_added}, {}};
}

/// Finds pointer runs, records tables, adds bookmarks/labels, and disassembles all-code tables.
void CreateAddressTablesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                          CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTableAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/AddressTable.java
    // Relevant methods: canAnalyze(), added(), processAddressTable(), getEntry(), makeTable(), and
    // getFunctionEntries().
    if (!context.options().create_address_tables) {
        return;
    }
    const std::uint32_t pointer_size = context.image().optional_header().pe32_plus ? 8U : 4U;
    const bool relocation_backed =
        std::any_of(context.image().relocations().begin(), context.image().relocations().end(),
                    [](const auto& block) { return !block.entries.empty(); });
    const std::uint32_t table_alignment = std::max<std::uint32_t>(1U, context.options().address_table_alignment);
    const std::uint32_t minimum_entries = std::max<std::uint32_t>(1U, context.options().address_table_minimum_entries);
    for (const auto& region : context.image().memory_regions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!region.initialized || (!region.readable && !region.writable && !region.executable) ||
            region.size < pointer_size) {
            continue;
        }
        const Address end = region.start + region.size - pointer_size;
        for (Address start = region.start; start <= end; ++start) {
            if (cancellation.is_cancelled()) {
                return;
            }
            if (start % table_alignment != 0U || conflicts_with_listing(context, start, true)) {
                continue;
            }
            const auto targets =
                read_table(context, start, region, pointer_size, context.options(), relocation_backed, cancellation);
            if (targets.size() < minimum_entries) {
                continue;
            }
            if (!context.add_address_table(
                    AddressTableRecord{start, pointer_size, targets, false, relocation_backed, false})) {
                continue;
            }
            if (context.options().create_analysis_bookmarks) {
                static_cast<void>(context.add_bookmark(
                    Bookmark{start, "Address Table", "Address table[" + std::to_string(targets.size()) + "] created"}));
            }
            if (context.options().address_table_auto_label) {
                const std::string prefix = "AddrTable" + address_suffix(start);
                static_cast<void>(
                    context.add_symbol(SymbolRecord{start, {}, prefix, {}, "address_table", false, true}));
                for (std::size_t index = 0; index < targets.size(); ++index) {
                    static_cast<void>(context.add_symbol(SymbolRecord{targets[index],
                                                                      {},
                                                                      prefix + "Element" + std::to_string(index),
                                                                      {},
                                                                      "address_table_element",
                                                                      false,
                                                                      true}));
                }
            }
            const bool all_code = std::all_of(targets.begin(), targets.end(),
                                              [&](Address target) { return context.image().is_executable(target); });
            if (all_code) {
                for (const Address target : targets) {
                    if (cancellation.is_cancelled()) {
                        return;
                    }
                    static_cast<void>(context.disassemble_flow(target));
                }
            }
            if (start > std::numeric_limits<Address>::max() - pointer_size) {
                break;
            }
            start += pointer_size - 1U;
        }
    }
}

} // namespace ghidra::analyzer
