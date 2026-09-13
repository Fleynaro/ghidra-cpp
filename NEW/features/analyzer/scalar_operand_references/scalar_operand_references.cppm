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
    for (const auto& [address, record] : context.instructions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (record.instruction.flow.kind == sleigh_runtime::FlowKind::branch ||
            record.instruction.flow.kind == sleigh_runtime::FlowKind::conditional_branch ||
            record.instruction.flow.kind == sleigh_runtime::FlowKind::call ||
            record.instruction.flow.kind == sleigh_runtime::FlowKind::indirect_call ||
            record.instruction.flow.kind == sleigh_runtime::FlowKind::indirect_branch) {
            // Flow operands already have a typed code reference and are not
            // scalar data references in ScalarOperandAnalyzer.
            continue;
        }
        for (std::size_t operand_index = 0; operand_index < record.instruction.operands.size(); ++operand_index) {
            const auto& operand = record.instruction.operands[operand_index];
            if (!operand.value || *operand.value < 0x1000) {
                continue;
            }
            const Address target = *operand.value;
            if (!context.image().find_memory_region(target)) {
                continue;
            }
            const auto duplicate =
                std::find_if(context.references().begin(), context.references().end(), [&](const Reference& reference) {
                    return reference.source == address && reference.operand_index == operand_index &&
                           reference.kind == ReferenceKind::scalar;
                });
            if (duplicate == context.references().end()) {
                static_cast<void>(context.add_reference(Reference{address, target, ReferenceKind::scalar, operand_index,
                                                                  std::nullopt, FlowOverride::none, true}));
            }
            if (!context.instructions().contains(target) && !context.data().contains(target) &&
                !context.image().is_executable(target)) {
                static_cast<void>(context.add_data(DataObject{target, 1, "address"}));
            }
        }
    }
}

} // namespace ghidra::analyzer
