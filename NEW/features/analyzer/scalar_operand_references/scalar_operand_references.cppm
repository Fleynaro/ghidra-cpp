module analyzer;

import std;

namespace ghidra::analyzer {

/// Returns the Scalar Operand analyzer priority and code-event contract.
AnalyzerDescriptor ScalarOperandReferencesAnalyzer::descriptor() const {
    return {"Scalar Operand References", 598, {EventKind::code_added}, {"Constant Propagation"}};
}

/// Applies the scalar address-reference acceptance and rejection rules.
void ScalarOperandReferencesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                              CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ScalarOperandAnalyzer.java
    // Relevant methods: checkOperands(), addReference(), checkOffcutFuncRef(), and checkForJumpTable().
    if (!context.options().scalar_operand_references) {
        return;
    }
    const auto relocation_matches_scalar = [&](Address address, std::size_t length, std::uint64_t scalar) {
        return std::any_of(context.image().relocations().begin(), context.image().relocations().end(),
                           [&](const pe::RelocationBlock& block) {
                               return std::any_of(
                                   block.entries.begin(), block.entries.end(),
                                   [&](const pe::RelocationEntry& relocation) {
                                       if (relocation.target_va < address || relocation.target_va - address >= length) {
                                           return false;
                                       }
                                       std::size_t width = context.image().optional_header().pe32_plus ? 8U : 4U;
                                       switch (relocation.type) {
                                           case 1: // IMAGE_REL_BASED_HIGH
                                           case 2: // IMAGE_REL_BASED_LOW
                                               width = 2U;
                                               break;
                                           case 3: // IMAGE_REL_BASED_HIGHLOW
                                               width = 4U;
                                               break;
                                           case 10: // IMAGE_REL_BASED_DIR64
                                               width = 8U;
                                               break;
                                           default:
                                               break;
                                       }
                                       const auto bytes = context.image().read_memory(relocation.target_va, width);
                                       if (!bytes || bytes->size() != width) {
                                           return false;
                                       }
                                       std::uint64_t encoded = 0;
                                       for (std::size_t index = 0; index < width; ++index) {
                                           encoded |= static_cast<std::uint64_t>((*bytes)[index]) << (index * 8U);
                                       }
                                       auto expected = scalar;
                                       if (width < sizeof(std::uint64_t)) {
                                           encoded &= (std::uint64_t{1} << (width * 8U)) - 1U;
                                           expected &= (std::uint64_t{1} << (width * 8U)) - 1U;
                                       }
                                       return encoded == expected;
                                   });
                           });
    };
    const auto offcut_instruction = [&](Address target) {
        return std::any_of(context.instructions().begin(), context.instructions().end(), [&](const auto& item) {
            const Address start = item.first;
            const auto end = start + item.second.instruction.length;
            return target > start && target < end;
        });
    };
    const auto offcut_function = [&](Address target) {
        if (const auto* function = context.function_containing(target)) {
            return function->entry != target;
        }
        return false;
    };
    const auto rejected_sentinel = [](std::uint64_t value) {
        static constexpr std::array<std::uint64_t, 9> sentinels{
            0xffffU, 0xff00U, 0xffffffU, 0xff0000U, 0xff00ffU, 0xffffffffU, 0xffffff00U, 0xffff0000U, 0xff000000U};
        return std::ranges::find(sentinels, value) != sentinels.end();
    };
    for (const auto& [address, record] : context.instructions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        for (std::size_t operand_index = 0; operand_index < record.instruction.operands.size(); ++operand_index) {
            const auto& operand = record.instruction.operands[operand_index];
            if (!operand.value ||
                (operand.kind != sleigh_runtime::OperandKind::immediate &&
                 operand.kind != sleigh_runtime::OperandKind::address) ||
                rejected_sentinel(*operand.value)) {
                continue;
            }
            const bool relocation_backed =
                relocation_matches_scalar(address, record.instruction.length, *operand.value);
            if (!relocation_backed && *operand.value < 0x1000) {
                continue;
            }
            std::optional<Address> target;
            if (context.image().find_memory_region(*operand.value)) {
                target = *operand.value;
            } else if (const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(*operand.value));
                       translated) {
                target = *translated;
            }
            if (!target || !context.image().find_memory_region(*target) || offcut_instruction(*target) ||
                offcut_function(*target)) {
                continue;
            }
            const auto duplicate =
                std::find_if(context.references().begin(), context.references().end(), [&](const Reference& reference) {
                    const bool flow_reference = reference.kind == ReferenceKind::conditional_jump ||
                                                reference.kind == ReferenceKind::unconditional_jump ||
                                                reference.kind == ReferenceKind::computed_jump ||
                                                reference.kind == ReferenceKind::conditional_call ||
                                                reference.kind == ReferenceKind::unconditional_call ||
                                                reference.kind == ReferenceKind::computed_call;
                    return reference.source == address && (reference.operand_index == operand_index ||
                                                           (flow_reference && reference.target == *target));
                });
            if (duplicate == context.references().end()) {
                static_cast<void>(context.add_reference(Reference{
                    address, *target, ReferenceKind::scalar, operand_index, std::nullopt, FlowOverride::none, true}));
            }
        }
    }
}

} // namespace ghidra::analyzer
