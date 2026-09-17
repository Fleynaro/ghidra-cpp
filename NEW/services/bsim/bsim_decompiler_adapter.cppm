export module recode.service.bsim.decompiler_adapter;

import std;
import recode.core;
import decompiler;

// Adaptation of Ghidra/Features/Decompiler/src/decompile/cpp/signature_ghidra.cc
// and SignatureResult.java compact value fields into the generic graph.

export namespace recode::services::bsim {

/// Converts value-owned native decompiler provenance into the generic graph input.
/// Native Funcdata and Architecture objects remain private to the decompiler frontend.
[[nodiscard]] inline recode::core::NormalizedFunction
make_normalized_function(const recode::core::FunctionSnapshot& snapshot,
                         const recode::decompiler::DecompilationResult& decompilation) {
    recode::core::NormalizedFunction result;
    result.identity = snapshot.key;
    result.name = snapshot.name;

    std::unordered_map<std::uint64_t, std::uint32_t> operation_ids;
    operation_ids.reserve(decompilation.pcode_provenance.size());
    for (std::uint32_t index = 0; index < decompilation.pcode_provenance.size(); ++index)
        operation_ids.emplace(decompilation.pcode_provenance[index].sequence, index);

    result.varnodes.reserve(decompilation.varnode_provenance.size());
    for (const auto& source : decompilation.varnode_provenance) {
        recode::core::NormalizedVarnode varnode;
        varnode.id = source.create_index;
        varnode.storage = recode::core::StorageLocation{source.space, source.offset, source.size};
        varnode.size = source.size;
        varnode.constant = source.space == "const" || source.space == "constant";
        varnode.constant_value = source.offset;
        varnode.input = !source.defining_op.has_value();
        varnode.persistent = source.space == "ram" || source.space == "data";
        varnode.annotation = source.space == "iop" || source.space == "join";
        varnode.written = source.defining_op.has_value();
        if (source.defining_op) {
            const auto operation = operation_ids.find(*source.defining_op);
            if (operation != operation_ids.end())
                varnode.defining_operation = operation->second;
        }
        result.varnodes.push_back(std::move(varnode));
    }

    result.operations.reserve(decompilation.pcode_provenance.size());
    for (std::uint32_t index = 0; index < decompilation.pcode_provenance.size(); ++index) {
        const auto& source = decompilation.pcode_provenance[index];
        recode::core::NormalizedOperation operation;
        operation.id = index;
        operation.opcode = static_cast<recode::core::PcodeOpcode>(source.opcode_value);
        if (source.output_varnode)
            operation.output = *source.output_varnode;
        operation.inputs = source.input_varnodes;
        operation.marker = operation.opcode == recode::core::PcodeOpcode::multiequal;
        operation.commutative = operation.opcode == recode::core::PcodeOpcode::multiequal;
        result.operations.push_back(std::move(operation));
    }

    result.blocks.reserve(snapshot.blocks.size());
    std::unordered_map<recode::core::EntityId, std::uint32_t> block_ids;
    block_ids.reserve(snapshot.blocks.size());
    for (std::uint32_t index = 0; index < snapshot.blocks.size(); ++index)
        block_ids.emplace(snapshot.blocks[index].id, index);
    for (std::uint32_t index = 0; index < snapshot.blocks.size(); ++index) {
        recode::core::NormalizedBasicBlock block;
        block.id = index;
        for (const auto& predecessor : snapshot.blocks[index].predecessors) {
            const auto mapped = block_ids.find(predecessor);
            if (mapped != block_ids.end())
                block.predecessors.push_back(mapped->second);
        }
        for (const auto& successor : snapshot.blocks[index].successors) {
            const auto mapped = block_ids.find(successor);
            if (mapped != block_ids.end())
                block.successors.push_back(mapped->second);
        }
        result.blocks.push_back(std::move(block));
    }
    for (auto& operation : result.operations) {
        const auto source = decompilation.pcode_provenance[operation.id].address;
        std::uint32_t selected = 0;
        for (std::uint32_t index = 0; index < snapshot.blocks.size(); ++index) {
            if (snapshot.blocks[index].ranges.contains(recode::core::Address{snapshot.key.entry.space, source})) {
                selected = index;
                break;
            }
        }
        operation.block_id = selected;
        if (!result.blocks.empty())
            result.blocks[selected].operation_ids.push_back(operation.id);
    }
    result.has_unimplemented = false;
    result.has_bad_data = false;
    return result;
}

} // namespace recode::services::bsim
