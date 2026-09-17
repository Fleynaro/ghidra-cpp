export module recode.service.bsim.graph_signature;

import std;
import recode.core.contracts.bsim;
import recode.core.diagnostics;
import recode.core.normalized_function;
import recode.service.bsim.block_signature;
import recode.service.bsim.signature;
import recode.service.bsim.signature_entry;

export namespace recode::services::bsim {

/// Port of Ghidra/Features/Decompiler/src/decompile/cpp/signature.cc:
/// GraphSigManager over the independent normalized graph boundary.
class GraphSignatureGenerator final {
public:
    static constexpr std::uint32_t sig_collapse_size = 0x1U;
    static constexpr std::uint32_t sig_collapse_indirect_noise = 0x2U;
    static constexpr std::uint32_t sig_do_not_use_const = 0x10U;
    static constexpr std::uint32_t sig_do_not_use_input = 0x20U;
    static constexpr std::uint32_t sig_do_not_use_persist = 0x40U;

    /// Generates the complete sorted feature sequence for one normalized function.
    [[nodiscard]] recode::core::Result<recode::core::contracts::FunctionSimilarityFeatures>
    generate(const recode::core::NormalizedFunction& function, std::uint32_t settings, std::int32_t max_iterations,
             std::int32_t max_block_iterations, std::uint32_t max_varnodes) const;

private:
    using Index = SignatureEntry::Index;

    /// Builds all signature overlays and performs initial local hashing.
    [[nodiscard]] recode::core::Result<void> prepare(const recode::core::NormalizedFunction& function,
                                                     std::uint32_t settings, std::uint32_t max_varnodes,
                                                     std::unordered_map<std::uint32_t, Index>& varnode_map,
                                                     std::unordered_map<std::uint32_t, Index>& operation_map,
                                                     std::vector<SignatureEntry>& entries) const;

    /// Performs one synchronous data-flow hash iteration.
    static void iterate_varnodes(const recode::core::NormalizedFunction& function,
                                 const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                 std::vector<SignatureEntry>& entries);

    /// Performs one synchronous control-flow hash iteration.
    static void iterate_blocks(const recode::core::NormalizedFunction& function,
                               const std::unordered_map<std::uint32_t, Index>& block_map,
                               std::vector<BlockSignatureEntry>& entries);

    /// Emits block-root, call, COPY, and control-flow feature hashes.
    static void collect_block_features(const recode::core::NormalizedFunction& function,
                                       const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                       const std::unordered_map<std::uint32_t, Index>& operation_map,
                                       const std::unordered_map<std::uint32_t, Index>& block_map,
                                       const std::vector<SignatureEntry>& varnodes,
                                       const std::vector<BlockSignatureEntry>& blocks,
                                       std::vector<std::uint32_t>& features);

    /// Emits all non-suppressed final Varnode hashes, retaining duplicates.
    static void collect_varnode_features(const std::vector<SignatureEntry>& entries,
                                         std::vector<std::uint32_t>& features);

    /// Finds a normalized operation by ID without exposing implementation details.
    [[nodiscard]] static const recode::core::NormalizedOperation&
    operation(const recode::core::NormalizedFunction& function,
              const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t id);

    /// Finds a normalized block by ID without exposing implementation details.
    [[nodiscard]] static const recode::core::NormalizedBasicBlock&
    block(const recode::core::NormalizedFunction& function, const std::unordered_map<std::uint32_t, Index>& block_map,
          std::uint32_t id);
};

inline const recode::core::NormalizedOperation&
GraphSignatureGenerator::operation(const recode::core::NormalizedFunction& function,
                                   const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t id) {
    return function.operations.at(operation_map.at(id));
}

inline const recode::core::NormalizedBasicBlock&
GraphSignatureGenerator::block(const recode::core::NormalizedFunction& function,
                               const std::unordered_map<std::uint32_t, Index>& block_map, std::uint32_t id) {
    return function.blocks.at(block_map.at(id));
}

inline recode::core::Result<void>
GraphSignatureGenerator::prepare(const recode::core::NormalizedFunction& function, std::uint32_t settings,
                                 std::uint32_t max_varnodes, std::unordered_map<std::uint32_t, Index>& varnode_map,
                                 std::unordered_map<std::uint32_t, Index>& operation_map,
                                 std::vector<SignatureEntry>& entries) const {
    if (!valid_signature_settings(settings))
        return std::unexpected(recode::core::Error::make(recode::core::DiagnosticCode::invalid_argument,
                                                         "BSim signature settings contain unsupported bits",
                                                         "Use the Ghidra settings encoding, for example 0x49."));
    if (max_varnodes != 0U && function.varnodes.size() > max_varnodes)
        return std::unexpected(recode::core::Error::make(recode::core::DiagnosticCode::invalid_argument,
                                                         "Function exceeds the configured BSim Varnode limit",
                                                         "Increase max_varnodes or analyze a smaller function."));
    varnode_map.reserve(function.varnodes.size());
    operation_map.reserve(function.operations.size());
    for (Index index = 0; index < function.varnodes.size(); ++index)
        varnode_map.emplace(function.varnodes[index].id, index);
    for (Index index = 0; index < function.operations.size(); ++index)
        operation_map.emplace(function.operations[index].id, index);
    entries.reserve(function.varnodes.size() + 1U);
    const auto modifiers = settings >> 2U;
    for (const auto& varnode : function.varnodes)
        entries.emplace_back(function, varnode, varnode_map, operation_map, modifiers);
    if ((modifiers & sig_collapse_indirect_noise) != 0U)
        SignatureEntry::remove_noise(function, varnode_map, operation_map, entries);
    else
        for (auto& entry : entries)
            entry.calculate_shadow(function, varnode_map, operation_map);
    for (auto& entry : entries)
        entry.local_hash(function, varnode_map, operation_map, modifiers);
    return {};
}

inline void GraphSignatureGenerator::iterate_varnodes(const recode::core::NormalizedFunction& function,
                                                      const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                                      std::vector<SignatureEntry>& entries) {
    for (auto& entry : entries)
        entry.flip();
    for (auto& entry : entries) {
        if (entry.not_emitted() || entry.terminal())
            continue;
        std::vector<const SignatureEntry*> neighbors;
        neighbors.reserve(entry.input_count());
        for (std::size_t input = 0; input < entry.input_count(); ++input) {
            const auto input_index = entry.input_entry(function, varnode_map, input, entries);
            neighbors.push_back(&entries[input_index]);
        }
        entry.hash_in(neighbors);
    }
}

inline void GraphSignatureGenerator::iterate_blocks(const recode::core::NormalizedFunction& function,
                                                    const std::unordered_map<std::uint32_t, Index>& block_map,
                                                    std::vector<BlockSignatureEntry>& entries) {
    for (auto& entry : entries)
        entry.flip();
    for (auto& entry : entries) {
        const auto& current = block(function, block_map, entry.block_id());
        std::vector<const BlockSignatureEntry*> incoming;
        std::vector<const recode::core::NormalizedBasicBlock*> incoming_blocks;
        incoming.reserve(current.predecessors.size());
        incoming_blocks.reserve(current.predecessors.size());
        for (const auto predecessor_id : current.predecessors) {
            incoming.push_back(&entries.at(block_map.at(predecessor_id)));
            incoming_blocks.push_back(&block(function, block_map, predecessor_id));
        }
        entry.hash_in(current, incoming, incoming_blocks);
    }
}

inline void GraphSignatureGenerator::collect_varnode_features(const std::vector<SignatureEntry>& entries,
                                                              std::vector<std::uint32_t>& features) {
    for (const auto& entry : entries)
        if (!entry.not_emitted())
            features.push_back(entry.hash());
}

inline void GraphSignatureGenerator::collect_block_features(
    const recode::core::NormalizedFunction& function, const std::unordered_map<std::uint32_t, Index>& varnode_map,
    const std::unordered_map<std::uint32_t, Index>& operation_map,
    const std::unordered_map<std::uint32_t, Index>& block_map, const std::vector<SignatureEntry>& varnodes,
    const std::vector<BlockSignatureEntry>& blocks, std::vector<std::uint32_t>& features) {
    using Opcode = recode::core::PcodeOpcode;
    for (const auto& block_entry : blocks) {
        const auto& current = block(function, block_map, block_entry.block_id());
        std::uint64_t last_hash = 0;
        bool have_last = false;
        std::uint64_t call_hash = 0;
        std::uint64_t copy_hash = 0;
        for (const auto operation_id : current.operation_ids) {
            const auto& operation_value = operation(function, operation_map, operation_id);
            std::size_t start = 0;
            std::size_t stop = 0;
            switch (operation_value.opcode) {
                case Opcode::call:
                    call_hash += 100001ULL;
                    call_hash *= 0x78abbfULL;
                    start = 1;
                    stop = operation_value.inputs.size();
                    break;
                case Opcode::call_ind:
                    call_hash += 123451ULL;
                    call_hash *= 0x78abbfULL;
                    start = 1;
                    stop = operation_value.inputs.size();
                    break;
                case Opcode::call_other:
                    start = 1;
                    stop = operation_value.inputs.size();
                    break;
                case Opcode::store:
                    start = 1;
                    stop = operation_value.inputs.size();
                    break;
                case Opcode::cbranch:
                    start = 1;
                    stop = std::min<std::size_t>(2, operation_value.inputs.size());
                    break;
                case Opcode::branch_ind:
                    start = 0;
                    stop = std::min<std::size_t>(1, operation_value.inputs.size());
                    break;
                case Opcode::return_op:
                    start = 1;
                    stop = operation_value.inputs.size();
                    break;
                case Opcode::indirect:
                case Opcode::copy:
                    if (operation_value.output) {
                        const auto output = varnodes.at(varnode_map.at(*operation_value.output));
                        if (output.standalone_copy())
                            copy_hash += output.hash();
                    }
                    continue;
                default:
                    continue;
            }
            if (stop == 0U &&
                (!operation_value.output || std::ranges::none_of(function.operations, [&](const auto& candidate) {
                    return std::ranges::find(candidate.inputs, *operation_value.output) != candidate.inputs.end();
                })))
                continue;
            std::uint64_t value = 0;
            if (operation_value.output) {
                const auto& output = varnodes.at(varnode_map.at(*operation_value.output));
                if (output.not_emitted())
                    continue;
                value = output.hash();
            } else {
                value = static_cast<std::uint64_t>(operation_value.opcode);
                value ^= value << 9U;
                value ^= value << 18U;
                std::uint64_t accumulated = 0;
                for (std::size_t input = start; input < stop; ++input) {
                    const auto input_index = varnode_map.at(operation_value.inputs[input]);
                    const auto& input_entry = varnodes[input_index];
                    const auto collapsed =
                        input_entry.shadow() == SignatureEntry::no_index ? input_index : input_entry.shadow();
                    accumulated += hash_mixin(value, varnodes[collapsed].hash());
                }
                value ^= accumulated;
            }
            const auto final_hash = hash_mixin(value, have_last ? last_hash : block_entry.hash());
            features.push_back(static_cast<std::uint32_t>(final_hash));
            have_last = true;
            last_hash = value;
        }
        auto final_hash = hash_mixin(block_entry.hash(), 0x9b1c5fULL);
        if (call_hash != 0U)
            final_hash = hash_mixin(final_hash, call_hash);
        features.push_back(static_cast<std::uint32_t>(final_hash));
        if (copy_hash != 0U)
            features.push_back(static_cast<std::uint32_t>(hash_mixin(copy_hash, 0xa2de3cULL)));
    }
}

inline recode::core::Result<recode::core::contracts::FunctionSimilarityFeatures>
GraphSignatureGenerator::generate(const recode::core::NormalizedFunction& function, std::uint32_t settings,
                                  std::int32_t max_iterations, std::int32_t max_block_iterations,
                                  std::uint32_t max_varnodes) const {
    if (max_iterations <= 0)
        return std::unexpected(recode::core::Error::make(recode::core::DiagnosticCode::invalid_argument,
                                                         "BSim max_iterations must be positive"));
    std::unordered_map<std::uint32_t, Index> varnode_map;
    std::unordered_map<std::uint32_t, Index> operation_map;
    std::vector<SignatureEntry> varnodes;
    auto prepared = prepare(function, settings, max_varnodes, varnode_map, operation_map, varnodes);
    if (!prepared)
        return std::unexpected(prepared.error());
    const auto minus_one = max_iterations - 1;
    const auto first_half = minus_one / 2;
    const auto second_half = minus_one - first_half;
    iterate_varnodes(function, varnode_map, varnodes);
    for (std::int32_t iteration = 0; iteration < first_half; ++iteration)
        iterate_varnodes(function, varnode_map, varnodes);

    std::vector<std::uint32_t> features;
    if (max_block_iterations >= 0 && !function.blocks.empty()) {
        std::unordered_map<std::uint32_t, Index> block_map;
        std::vector<BlockSignatureEntry> blocks;
        block_map.reserve(function.blocks.size());
        blocks.reserve(function.blocks.size());
        for (Index index = 0; index < function.blocks.size(); ++index) {
            block_map.emplace(function.blocks[index].id, index);
            blocks.emplace_back(function.blocks[index].id);
            blocks.back().local_hash(function.blocks[index]);
        }
        for (std::int32_t iteration = 0; iteration < max_block_iterations; ++iteration)
            iterate_blocks(function, block_map, blocks);
        collect_block_features(function, varnode_map, operation_map, block_map, varnodes, blocks, features);
    }
    for (std::int32_t iteration = 0; iteration < second_half; ++iteration)
        iterate_varnodes(function, varnode_map, varnodes);
    collect_varnode_features(varnodes, features);
    std::sort(features.begin(), features.end());

    std::uint64_t overall = 0x12349876abacabULL;
    for (const auto feature : features)
        overall = hash_mixin(overall, feature);
    recode::core::contracts::FunctionSimilarityFeatures result;
    result.function = function.identity;
    result.hashes = std::move(features);
    result.direct_call_addresses = function.direct_call_addresses;
    result.has_unimplemented = function.has_unimplemented;
    result.has_bad_data = function.has_bad_data;
    result.settings = settings;
    result.overall_hash = overall;
    return result;
}

} // namespace recode::services::bsim
