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
    const auto relocation_contains = [&](Address address, std::size_t length) {
        return std::any_of(context.image().relocations().begin(), context.image().relocations().end(),
                           [&](const pe::RelocationBlock& block) {
                               return std::any_of(block.entries.begin(), block.entries.end(),
                                                  [&](const pe::RelocationEntry& relocation) {
                                                      const auto end = address + length;
                                                      return relocation.target_va >= address &&
                                                             relocation.target_va < end;
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
            const bool relocation_backed = relocation_contains(address, record.instruction.length);
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
            if (!target || !context.image().find_memory_region(*target) || offcut_instruction(*target)) {
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
