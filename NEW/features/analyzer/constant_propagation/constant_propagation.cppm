module analyzer;

import std;

namespace ghidra::analyzer {
namespace {

/// Builds a stable map key for one p-code storage location.
[[nodiscard]] std::string location_key(const sleigh_runtime::Varnode& location) {
    return location.space + ":" + std::to_string(location.offset) + ":" + std::to_string(location.size);
}

/// Truncates a value to the width of its p-code destination.
[[nodiscard]] std::uint64_t truncate_value(std::uint64_t value, std::uint32_t size) {
    if (size == 0 || size >= 8) {
        return value;
    }
    return value & ((std::uint64_t{1} << (size * 8U)) - 1U);
}

/// Evaluates one arithmetic operation when every input is known.
[[nodiscard]] std::optional<std::uint64_t> evaluate_operation(const sleigh_runtime::PcodeOp& operation,
                                                              const std::map<std::string, std::uint64_t>& values,
                                                              const std::map<std::string, std::uint64_t>& memory) {
    const auto value_of = [&](const sleigh_runtime::Varnode& varnode) -> std::optional<std::uint64_t> {
        if (varnode.space == "const") {
            return truncate_value(varnode.offset, varnode.size);
        }
        const auto value = values.find(location_key(varnode));
        if (value != values.end()) {
            return value->second;
        }
        return std::nullopt;
    };
    if (operation.opcode == sleigh_runtime::PcodeOpcode::copy ||
        operation.opcode == sleigh_runtime::PcodeOpcode::cast ||
        operation.opcode == sleigh_runtime::PcodeOpcode::int_zext) {
        if (operation.inputs.empty()) {
            return std::nullopt;
        }
        return value_of(operation.inputs.back());
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::int_sext) {
        if (operation.inputs.empty()) {
            return std::nullopt;
        }
        const auto value = value_of(operation.inputs.back());
        if (!value || !operation.output || operation.inputs.back().size >= operation.output->size ||
            operation.inputs.back().size == 0 || operation.inputs.back().size >= 8) {
            return value;
        }
        const auto source_bits = operation.inputs.back().size * 8U;
        const auto source_mask = (std::uint64_t{1} << source_bits) - 1U;
        const auto sign_bit = std::uint64_t{1} << (source_bits - 1U);
        return (*value & sign_bit) == 0 ? *value : (*value | ~source_mask);
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::int_two_comp ||
        operation.opcode == sleigh_runtime::PcodeOpcode::int_negate ||
        operation.opcode == sleigh_runtime::PcodeOpcode::bool_negate) {
        if (operation.inputs.empty())
            return std::nullopt;
        const auto value = value_of(operation.inputs.back());
        if (!value)
            return std::nullopt;
        if (operation.opcode == sleigh_runtime::PcodeOpcode::bool_negate)
            return *value == 0;
        return static_cast<std::uint64_t>(-static_cast<std::int64_t>(*value));
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::load) {
        if (operation.inputs.empty()) {
            return std::nullopt;
        }
        const auto address = value_of(operation.inputs.back());
        if (!address) {
            return std::nullopt;
        }
        const auto key = (operation.memory_space.value_or("ram")) + ":" + std::to_string(*address);
        const auto stored = memory.find(key);
        return stored == memory.end() ? std::nullopt : std::optional<std::uint64_t>{stored->second};
    }
    if (operation.inputs.size() < 2) {
        return std::nullopt;
    }
    const auto left = value_of(operation.inputs[operation.inputs.size() - 2]);
    const auto right = value_of(operation.inputs.back());
    if (!left || !right) {
        return std::nullopt;
    }
    switch (operation.opcode) {
        case sleigh_runtime::PcodeOpcode::int_add:
            return *left + *right;
        case sleigh_runtime::PcodeOpcode::int_sub:
            return *left - *right;
        case sleigh_runtime::PcodeOpcode::int_mult:
            return *left * *right;
        case sleigh_runtime::PcodeOpcode::int_and:
            return *left & *right;
        case sleigh_runtime::PcodeOpcode::int_or:
            return *left | *right;
        case sleigh_runtime::PcodeOpcode::int_xor:
            return *left ^ *right;
        case sleigh_runtime::PcodeOpcode::int_left:
            return *left << (*right & 63U);
        case sleigh_runtime::PcodeOpcode::int_right:
            return *left >> (*right & 63U);
        case sleigh_runtime::PcodeOpcode::int_div:
            return *right == 0 ? std::nullopt : std::optional<std::uint64_t>{*left / *right};
        case sleigh_runtime::PcodeOpcode::int_rem:
            return *right == 0 ? std::nullopt : std::optional<std::uint64_t>{*left % *right};
        case sleigh_runtime::PcodeOpcode::int_equal:
            return *left == *right;
        case sleigh_runtime::PcodeOpcode::int_not_equal:
            return *left != *right;
        case sleigh_runtime::PcodeOpcode::int_less:
            return *left < *right;
        case sleigh_runtime::PcodeOpcode::int_less_equal:
            return *left <= *right;
        case sleigh_runtime::PcodeOpcode::int_sless:
        case sleigh_runtime::PcodeOpcode::int_sless_equal:
        case sleigh_runtime::PcodeOpcode::int_sdiv:
        case sleigh_runtime::PcodeOpcode::int_srem:
        case sleigh_runtime::PcodeOpcode::int_sright: {
            const auto as_signed = [](std::uint64_t value, std::uint32_t size) -> std::int64_t {
                if (size == 0 || size >= 8)
                    return static_cast<std::int64_t>(value);
                const auto bits = size * 8U;
                const auto mask = (std::uint64_t{1} << bits) - 1U;
                value &= mask;
                if ((value & (std::uint64_t{1} << (bits - 1U))) != 0)
                    value |= ~mask;
                return static_cast<std::int64_t>(value);
            };
            const auto left_signed = as_signed(*left, operation.inputs[operation.inputs.size() - 2].size);
            const auto right_signed = as_signed(*right, operation.inputs.back().size);
            if (operation.opcode == sleigh_runtime::PcodeOpcode::int_sless)
                return left_signed < right_signed;
            if (operation.opcode == sleigh_runtime::PcodeOpcode::int_sless_equal)
                return left_signed <= right_signed;
            if (operation.opcode == sleigh_runtime::PcodeOpcode::int_sdiv) {
                if (right_signed == 0 ||
                    (left_signed == std::numeric_limits<std::int64_t>::min() && right_signed == -1))
                    return std::nullopt;
                return static_cast<std::uint64_t>(left_signed / right_signed);
            }
            if (operation.opcode == sleigh_runtime::PcodeOpcode::int_srem) {
                if (right_signed == 0 ||
                    (left_signed == std::numeric_limits<std::int64_t>::min() && right_signed == -1))
                    return std::nullopt;
                return static_cast<std::uint64_t>(left_signed % right_signed);
            }
            return static_cast<std::uint64_t>(left_signed >> (static_cast<unsigned>(right_signed) & 63U));
        }
        case sleigh_runtime::PcodeOpcode::int_carry: {
            const auto bits = operation.inputs.back().size * 8U;
            if (bits == 0 || bits > 64)
                return std::nullopt;
            if (bits == 64)
                return *left > std::numeric_limits<std::uint64_t>::max() - *right;
            return (*left & ((std::uint64_t{1} << bits) - 1U)) + (*right & ((std::uint64_t{1} << bits) - 1U)) >=
                   (std::uint64_t{1} << bits);
        }
        default:
            return std::nullopt;
    }
}

/// Returns the concrete memory value represented by a STORE operation.
[[nodiscard]] std::optional<std::pair<std::string, std::uint64_t>>
store_value(const sleigh_runtime::PcodeOp& operation, const std::map<std::string, std::uint64_t>& values) {
    if (operation.inputs.size() < 2) {
        return std::nullopt;
    }
    const auto address = operation.inputs[operation.inputs.size() - 2];
    const auto value = operation.inputs.back();
    const auto address_it = address.space == "const" ? std::optional<std::uint64_t>{address.offset}
                                                     : [&]() -> std::optional<std::uint64_t> {
        const auto found = values.find(location_key(address));
        return found == values.end() ? std::nullopt : std::optional{found->second};
    }();
    const auto value_it =
        value.space == "const" ? std::optional<std::uint64_t>{value.offset} : [&]() -> std::optional<std::uint64_t> {
        const auto found = values.find(location_key(value));
        return found == values.end() ? std::nullopt : std::optional{found->second};
    }();
    if (!address_it || !value_it) {
        return std::nullopt;
    }
    return std::pair{operation.memory_space.value_or("ram") + ":" + std::to_string(*address_it), *value_it};
}

} // namespace

/// Returns the Constant Propagation priority and code-event contract.
AnalyzerDescriptor ConstantPropagationAnalyzer::descriptor() const {
    return {"Constant Propagation", 596, {EventKind::code_added, EventKind::function_added}, {"Function Body"}};
}

/// Runs bounded p-code symbolic propagation through all affected functions.
void ConstantPropagationAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                          CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/program/util/SymbolicPropogator.java
    // Relevant methods: added(), flowConstants(), applyPcode(), and evaluator reference hooks.
    if (!context.options().constant_propagation) {
        return;
    }
    for (const auto& [entry, function] : context.functions()) {
        if (function.blocks.empty())
            continue;
        struct FlowState {
            std::map<std::string, std::uint64_t> values;
            std::map<std::string, std::uint64_t> memory;
        };
        std::map<Address, FlowState> incoming;
        std::deque<Address> worklist;
        incoming.emplace(function.blocks.front().start, FlowState{});
        worklist.push_back(function.blocks.front().start);
        std::size_t iterations = 0;
        while (!worklist.empty() && iterations++ < 4096) {
            if (cancellation.is_cancelled())
                return;
            const Address block_start = worklist.front();
            worklist.pop_front();
            const auto block_it = std::find_if(function.blocks.begin(), function.blocks.end(),
                                               [&](const BasicBlock& block) { return block.start == block_start; });
            if (block_it == function.blocks.end())
                continue;
            FlowState state = incoming.at(block_start);
            for (const Address address : block_it->instructions) {
                const auto instruction = context.instructions().find(address);
                if (instruction == context.instructions().end())
                    continue;
                for (const auto& operation : instruction->second.instruction.pcode) {
                    if (operation.opcode == sleigh_runtime::PcodeOpcode::call ||
                        operation.opcode == sleigh_runtime::PcodeOpcode::call_ind ||
                        operation.opcode == sleigh_runtime::PcodeOpcode::call_other) {
                        // Without a recovered prototype, a call may clobber any
                        // register but does not invalidate known memory facts.
                        std::erase_if(state.values,
                                      [](const auto& item) { return item.first.starts_with("register:"); });
                        continue;
                    }
                    if (operation.opcode == sleigh_runtime::PcodeOpcode::store) {
                        if (const auto stored = store_value(operation, state.values)) {
                            state.memory[stored->first] = stored->second;
                        }
                        continue;
                    }
                    if (!operation.output)
                        continue;
                    const auto value = evaluate_operation(operation, state.values, state.memory);
                    if (!value)
                        continue;
                    const auto normalized = truncate_value(*value, operation.output->size);
                    state.values[location_key(*operation.output)] = normalized;
                    static_cast<void>(context.add_constant_fact(
                        ConstantFact{address, *operation.output, normalized, function.blocks.size() == 1}));
                }
            }
            for (const Address successor : block_it->successors) {
                auto target = incoming.find(successor);
                if (target == incoming.end()) {
                    incoming.emplace(successor, state);
                    worklist.push_back(successor);
                    continue;
                }
                FlowState joined = target->second;
                std::erase_if(joined.values, [&](const auto& item) {
                    const auto other = state.values.find(item.first);
                    return other == state.values.end() || other->second != item.second;
                });
                std::erase_if(joined.memory, [&](const auto& item) {
                    const auto other = state.memory.find(item.first);
                    return other == state.memory.end() || other->second != item.second;
                });
                if (joined.values != target->second.values || joined.memory != target->second.memory) {
                    target->second = std::move(joined);
                    worklist.push_back(successor);
                }
            }
        }
    }
}

} // namespace ghidra::analyzer
