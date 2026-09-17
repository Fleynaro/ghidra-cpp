export module analyzer_windows_resource_reference;

import analyzer;
import std;

// Original sources:
// Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/WindowsResourceReferenceAnalyzer.java
// Ghidra/Features/Decompiler/ghidra_scripts/WindowsResourceReference.java

/// Adds DATA references from constant Windows resource IDs to PE resource payloads.
export namespace recode::analyzer {
class WindowsResourceReferenceAnalyzer final : public Analyzer {
public:
    /// Returns the one-time analyzer's data-type propagation priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Resolves supported Windows API calls through existing constants and resource leaves.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Returns a case-insensitive copy of a symbol name.
[[nodiscard]] std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

/// Returns the resource family associated with a supported Windows lookup API.
[[nodiscard]] std::optional<std::string> resource_family(std::string_view name) {
    auto lower = lower_copy(std::string{name});
    for (const auto prefix : {std::string_view{"__imp_"}, std::string_view{"_imp_"}, std::string_view{"imp_"}}) {
        if (lower.starts_with(prefix)) {
            lower.erase(0, prefix.size());
            break;
        }
    }
    if (lower == "loadstringa" || lower == "loadstringw" || lower == "afxmessagebox") {
        return "StringTable";
    }
    if (lower == "loadacceleratorsa" || lower == "loadacceleratorsw") {
        return "Accelerator";
    }
    if (lower == "loadbitmapa" || lower == "loadbitmapw") {
        return "Bitmap";
    }
    if (lower == "loadicona" || lower == "loadiconw") {
        return "GroupIcon";
    }
    if (lower == "regloadmuistringw") {
        return "MUI";
    }
    if (lower == "playsoundw") {
        return "WAVE";
    }
    if (lower.find("dialog") != std::string::npos) {
        return "Dialog";
    }
    if (lower.find("menu") != std::string::npos) {
        return "Menu";
    }
    if (lower.find("cursor") != std::string::npos || lower.find("findresource") != std::string::npos ||
        lower.find("loadimage") != std::string::npos) {
        return std::string{};
    }
    if (lower == "loadmenua" || lower == "loadmenuw") {
        return "Menu";
    }
    if (lower == "loadregtypelib" || lower == "loadtypelib" || lower == "loadtypelibex") {
        return std::string{};
    }
    return std::nullopt;
}

/// Returns the resource leaf's preferred virtual address.
[[nodiscard]] std::optional<Address> resource_address(const AnalysisContext& context, const pe::ResourceLeaf& leaf) {
    const auto address = context.image().rva_to_va(leaf.data_rva);
    return address && context.image().find_memory_region(*address, leaf.size) ? std::optional{*address} : std::nullopt;
}

/// Returns the decoded UTF-16 string-table item address for one resource ID.
[[nodiscard]] std::optional<Address> string_table_address(const AnalysisContext& context, const pe::ResourceLeaf& leaf,
                                                          std::uint64_t resource_id) {
    if (resource_id == 0U || leaf.type.named || leaf.type.id != 6U || leaf.name.named ||
        leaf.name.id != (resource_id - 1U) / 16U + 1U) {
        return std::nullopt;
    }
    const auto base = resource_address(context, leaf);
    const auto payload = context.image().read_resource_payload(leaf);
    if (!base || !payload) {
        return std::nullopt;
    }
    std::size_t offset = 0;
    const auto wanted = (resource_id - 1U) % 16U;
    for (std::uint64_t index = 0; index <= wanted; ++index) {
        if (offset + 2U > payload->size()) {
            return std::nullopt;
        }
        const auto length =
            static_cast<std::size_t>((*payload)[offset]) | (static_cast<std::size_t>((*payload)[offset + 1U]) << 8U);
        if (index == wanted) {
            return *base + offset;
        }
        if (length > (payload->size() - offset - 2U) / 2U) {
            return std::nullopt;
        }
        offset += 2U + length * 2U;
    }
    return std::nullopt;
}

/// Finds the payload corresponding to a resource API family and numeric ID.
[[nodiscard]] std::optional<Address> find_resource(const AnalysisContext& context, std::string_view family,
                                                   std::uint64_t resource_id) {
    if (!context.image().resources()) {
        return std::nullopt;
    }
    for (const auto& leaf : context.image().resources()->leaves) {
        if (family == "StringTable") {
            if (const auto address = string_table_address(context, leaf, resource_id)) {
                return address;
            }
            continue;
        }
        if (!leaf.type.named &&
            ((family == "" && leaf.type.id != 6U) || (family == "Dialog" && leaf.type.id == 5U) ||
             (family == "Menu" && leaf.type.id == 4U) || (family == "Accelerator" && leaf.type.id == 9U) ||
             (family == "GroupIcon" && leaf.type.id == 14U) || (family == "Bitmap" && leaf.type.id == 2U) ||
             (family == "WAVE" && leaf.type.id == 11U) || (family == "MUI" && leaf.type.id == 240U)) &&
            !leaf.name.named && leaf.name.id == resource_id) {
            if (const auto address = resource_address(context, leaf)) {
                return address;
            }
        }
    }
    return std::nullopt;
}

/// Collects immediate and propagated values before a resource call in deterministic order.
[[nodiscard]] std::vector<std::uint64_t> candidate_ids(const AnalysisContext& context, Address source) {
    std::vector<std::uint64_t> result;
    const auto function = context.function_containing(source);
    std::vector<Address> instruction_addresses;
    if (function) {
        instruction_addresses.assign(function->instruction_starts.begin(), function->instruction_starts.end());
    } else {
        // The original helper obtains a HighFunction. During the native migration, use the
        // closest decoded instructions when the caller function has not been materialized yet.
        for (const auto& instruction_record : context.instructions()) {
            const auto address = instruction_record.first;
            if (address < source) {
                instruction_addresses.push_back(address);
            }
        }
        if (instruction_addresses.size() > 64U) {
            instruction_addresses.erase(instruction_addresses.begin(), instruction_addresses.end() - 64);
        }
    }
    for (const auto address : instruction_addresses) {
        if (address >= source) {
            break;
        }
        const auto instruction = context.instructions().find(address);
        if (instruction == context.instructions().end()) {
            continue;
        }
        for (const auto& operand : instruction->second.instruction.operands) {
            if (operand.value && (operand.kind == sleigh_runtime::OperandKind::immediate ||
                                  operand.kind == sleigh_runtime::OperandKind::address)) {
                result.push_back(*operand.value);
                continue;
            }
            const auto marker = operand.text.find("0x");
            if (marker != std::string::npos) {
                std::uint64_t value = 0;
                const auto parsed = std::from_chars(operand.text.data() + marker + 2,
                                                    operand.text.data() + operand.text.size(), value, 16);
                if (parsed.ec == std::errc{}) {
                    result.push_back(value);
                }
            }
        }
    }
    for (const auto& fact : context.constant_facts()) {
        if (fact.instruction < source && fact.value < 0x100000U) {
            result.push_back(fact.value);
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

/// Returns the external symbol whose IAT address is the supplied call target.
[[nodiscard]] const ExternalSymbol* external_at(const AnalysisContext& context, Address target) {
    const auto symbol = std::find_if(context.external_symbols().begin(), context.external_symbols().end(),
                                     [&](const ExternalSymbol& item) { return item.iat_address == target; });
    return symbol == context.external_symbols().end() ? nullptr : &*symbol;
}

} // namespace

/// Returns the WindowsResourceReference analyzer priority and late-code triggers.
AnalyzerDescriptor WindowsResourceReferenceAnalyzer::descriptor() const {
    return {"WindowsResourceReference",
            900,
            {EventKind::code_added, EventKind::constant_added, EventKind::external_added},
            {}};
}

/// Reproduces the script's resource-family and constant-ID lookup without a decompiler dependency.
void WindowsResourceReferenceAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                               CancellationToken& cancellation) {
    if (!context.options().windows_resource_reference || !context.image().resources()) {
        return;
    }
    const auto references = context.references();
    for (const auto& reference : references) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (reference.kind != ReferenceKind::external) {
            continue;
        }
        const auto* external = external_at(context, reference.target);
        if (!external) {
            continue;
        }
        const auto family = resource_family(external->name);
        if (!family) {
            continue;
        }
        for (const auto id : candidate_ids(context, reference.source)) {
            const auto target = find_resource(context, *family, id);
            if (!target) {
                continue;
            }
            static_cast<void>(context.add_reference(Reference{reference.source, *target, ReferenceKind::data,
                                                              std::nullopt, std::nullopt, FlowOverride::none, true}));
            const std::string symbol_name = "Rsrc_" + (family->empty() ? std::string{"resource"} : *family);
            static_cast<void>(
                context.add_symbol(SymbolRecord{*target, {}, symbol_name, {}, "windows_resource", false, true}));
            if (context.options().create_analysis_bookmarks) {
                static_cast<void>(context.add_bookmark(
                    Bookmark{reference.source, "WindowsResourceReference", "Added Resource Reference"}));
            }
            break;
        }
    }
}

} // namespace recode::analyzer
