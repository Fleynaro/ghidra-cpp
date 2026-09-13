module analyzer;

import std;

namespace ghidra::analyzer {
namespace {

/// Accepts provider memory/address kinds and textual x86 memory qualifiers.
[[nodiscard]] bool is_memory_operand(const sleigh_runtime::Operand& operand) {
    if (operand.kind == sleigh_runtime::OperandKind::memory || operand.kind == sleigh_runtime::OperandKind::address)
        return true;
    std::string text = operand.text;
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text.find('[') != std::string::npos || text.find(" ptr") != std::string::npos;
}

/// Resolves a p-code address operand into a mapped preferred virtual address.
[[nodiscard]] std::optional<Address> resolve_memory_value(const AnalysisContext& context, Address source,
                                                          const sleigh_runtime::Varnode& value) {
    if (value.space == "const") {
        const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(value.offset));
        if (translated && context.image().find_memory_region(*translated)) {
            return *translated;
        }
    }
    if (context.image().find_memory_region(value.offset)) {
        return value.offset;
    }
    const auto function = context.function_containing(source);
    for (auto fact = context.constant_facts().rbegin(); fact != context.constant_facts().rend(); ++fact) {
        if (fact->path_stable && function && fact->function_entry == function->entry && fact->instruction <= source &&
            fact->location == value) {
            if (context.image().find_memory_region(fact->value)) {
                return fact->value;
            }
            const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(fact->value));
            if (translated && context.image().find_memory_region(*translated)) {
                return *translated;
            }
        }
    }
    return std::nullopt;
}

/// Returns the address input of a LOAD or STORE p-code operation.
[[nodiscard]] std::optional<Address> memory_operand(const AnalysisContext& context, Address source,
                                                    const sleigh_runtime::PcodeOp& operation) {
    if (operation.inputs.empty()) {
        return std::nullopt;
    }
    const auto index = operation.opcode == sleigh_runtime::PcodeOpcode::store && operation.inputs.size() >= 2
                           ? operation.inputs.size() - 2
                           : operation.inputs.size() - 1;
    return resolve_memory_value(context, source, operation.inputs[index]);
}

/// Resolves an explicit instruction operand address as VA or image-relative RVA.
[[nodiscard]] std::optional<Address> operand_address(const AnalysisContext& context, std::uint64_t value) {
    if (context.image().find_memory_region(value)) {
        return value;
    }
    const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(value));
    if (translated && context.image().find_memory_region(*translated)) {
        return *translated;
    }
    return std::nullopt;
}

/// Resolves a concrete hexadecimal address embedded in a printed memory operand.
[[nodiscard]] std::optional<Address> printed_operand_address(const AnalysisContext& context, std::string_view text) {
    const auto start = text.find("0x");
    if (start == std::string_view::npos)
        return std::nullopt;
    auto end = start + 2;
    while (end < text.size() && std::isxdigit(static_cast<unsigned char>(text[end])) != 0)
        ++end;
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(text.data() + start + 2, text.data() + end, value, 16);
    if (parsed.ec != std::errc{})
        return std::nullopt;
    return operand_address(context, value);
}

} // namespace

/// Returns the Reference analyzer priority and code-event contract.
AnalyzerDescriptor ReferenceAnalyzer::descriptor() const {
    return {"Reference",
            600,
            {EventKind::code_added, EventKind::constant_added, EventKind::external_added},
            {"Constant Propagation"}};
}

/// Materializes direct memory references represented by decoded p-code.
void ReferenceAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java
    // Relevant methods: added(), createDisassemblyCommandsForAddress(), and flow-reference handling.
    if (!context.options().reference) {
        return;
    }
    for (const auto& [address, record] : context.instructions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        for (std::size_t operand_index = 0; operand_index < record.instruction.operands.size(); ++operand_index) {
            const auto& operand = record.instruction.operands[operand_index];
            if (is_memory_operand(operand)) {
                const auto target = operand.value ? operand_address(context, *operand.value)
                                                  : printed_operand_address(context, operand.text);
                if (target) {
                    const bool external =
                        std::any_of(context.external_symbols().begin(), context.external_symbols().end(),
                                    [&](const ExternalSymbol& symbol) { return symbol.iat_address == *target; });
                    std::optional<Address> fallthrough;
                    if (external && (record.instruction.flow.kind == sleigh_runtime::FlowKind::call ||
                                     record.instruction.flow.kind == sleigh_runtime::FlowKind::indirect_call)) {
                        fallthrough = address + record.instruction.length;
                    }
                    static_cast<void>(context.add_reference(
                        Reference{address, *target, external ? ReferenceKind::external : ReferenceKind::data,
                                  operand_index, fallthrough, FlowOverride::none, true}));
                }
            }
        }
        for (const auto& operation : record.instruction.pcode) {
            if (operation.opcode != sleigh_runtime::PcodeOpcode::load &&
                operation.opcode != sleigh_runtime::PcodeOpcode::store) {
                continue;
            }
            const auto target = memory_operand(context, address, operation);
            if (!target || !context.image().find_memory_region(*target)) {
                continue;
            }
            const auto operand_index = operation.opcode == sleigh_runtime::PcodeOpcode::load ? 1U : 0U;
            const bool external =
                std::any_of(context.external_symbols().begin(), context.external_symbols().end(),
                            [&](const ExternalSymbol& symbol) { return symbol.iat_address == *target; });
            std::optional<Address> fallthrough;
            if (external && (record.instruction.flow.kind == sleigh_runtime::FlowKind::call ||
                             record.instruction.flow.kind == sleigh_runtime::FlowKind::indirect_call)) {
                fallthrough = address + record.instruction.length;
            }
            static_cast<void>(context.add_reference(Reference{address, *target,
                                                              external ? ReferenceKind::external : ReferenceKind::data,
                                                              operand_index, fallthrough, FlowOverride::none, true}));
        }
    }
}

} // namespace ghidra::analyzer
