export module analyzer_constant_propagation;

import analyzer;
import std;

/// Owns the p-code constant propagation analyzer declaration and implementation.
export namespace ghidra::analyzer {
class ConstantPropagationAnalyzer final : public Analyzer {
public:
    /// Returns the ConstantPropagationAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Runs bounded fixed-point symbolic propagation over affected functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Builds a stable map key for one p-code storage location.
[[nodiscard]] std::string location_key(const sleigh_runtime::Varnode& location) {
    return location.space.name() + ":" + std::to_string(location.offset) + ":" + std::to_string(location.size);
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
                                                              const std::map<std::string, std::uint64_t>& memory,
                                                              const AnalysisContext& context) {
    const auto value_of = [&](const sleigh_runtime::Varnode& varnode) -> std::optional<std::uint64_t> {
        if (varnode.space.name() == "const") {
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
        if (operation.opcode == sleigh_runtime::PcodeOpcode::int_negate)
            return ~*value;
        return std::uint64_t{0} - *value;
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::load) {
        if (operation.inputs.empty()) {
            return std::nullopt;
        }
        const auto address = value_of(operation.inputs.back());
        if (!address) {
            return std::nullopt;
        }
        const auto key = (operation.memory_space ? operation.memory_space->name() : std::string{"ram"}) + ":" +
                         std::to_string(*address);
        if (const auto stored = memory.find(key); stored != memory.end()) {
            return stored->second;
        }
        std::optional<Address> mapped;
        if (context.image().find_memory_region(*address)) {
            mapped = *address;
        } else if (const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(*address)); translated) {
            mapped = *translated;
        }
        if (!mapped || !operation.output) {
            return std::nullopt;
        }
        const auto bytes = context.image().read_memory(*mapped, operation.output->size);
        if (!bytes) {
            return std::nullopt;
        }
        std::uint64_t loaded = 0;
        for (std::size_t index = 0; index < bytes->size() && index < sizeof(loaded); ++index) {
            loaded |= static_cast<std::uint64_t>((*bytes)[index]) << (index * 8U);
        }
        return loaded;
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::multiequal) {
        if (operation.inputs.empty()) {
            return std::nullopt;
        }
        const auto first = value_of(operation.inputs.front());
        if (!first) {
            return std::nullopt;
        }
        for (const auto& input : operation.inputs) {
            if (value_of(input) != first) {
                return std::nullopt;
            }
        }
        return first;
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::piece) {
        if (operation.inputs.size() < 2) {
            return std::nullopt;
        }
        const auto high = value_of(operation.inputs[operation.inputs.size() - 2]);
        const auto low = value_of(operation.inputs.back());
        if (!high || !low) {
            return std::nullopt;
        }
        const auto low_bits = operation.inputs.back().size * 8U;
        return low_bits >= 64 ? *low : (*high << low_bits) | *low;
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::subpiece) {
        if (operation.inputs.size() < 2) {
            return std::nullopt;
        }
        const auto value = value_of(operation.inputs.front());
        const auto offset = value_of(operation.inputs.back());
        if (!value || !offset || *offset >= 8) {
            return std::nullopt;
        }
        return *value >> (*offset * 8U);
    }
    if (operation.opcode == sleigh_runtime::PcodeOpcode::ptradd ||
        operation.opcode == sleigh_runtime::PcodeOpcode::ptrsub) {
        if (operation.inputs.size() < 3) {
            return std::nullopt;
        }
        const auto base = value_of(operation.inputs[0]);
        const auto index = value_of(operation.inputs[1]);
        const auto element_size = value_of(operation.inputs[2]);
        if (!base || !index || !element_size) {
            return std::nullopt;
        }
        const auto displacement = *index * *element_size;
        return operation.opcode == sleigh_runtime::PcodeOpcode::ptradd ? *base + displacement : *base - displacement;
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
        case sleigh_runtime::PcodeOpcode::int_right: {
            const auto width = operation.inputs[operation.inputs.size() - 2].size * 8U;
            if (width == 0 || *right >= width)
                return std::uint64_t{0};
            return operation.opcode == sleigh_runtime::PcodeOpcode::int_left ? *left << *right : *left >> *right;
        }
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
            const auto width = operation.inputs[operation.inputs.size() - 2].size * 8U;
            if (width == 0 || *right >= width)
                return left_signed < 0 ? std::numeric_limits<std::uint64_t>::max() : std::uint64_t{0};
            return static_cast<std::uint64_t>(left_signed >> *right);
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
        case sleigh_runtime::PcodeOpcode::bool_xor:
            return ((*left != 0) != (*right != 0));
        case sleigh_runtime::PcodeOpcode::bool_and:
            return ((*left != 0) && (*right != 0));
        case sleigh_runtime::PcodeOpcode::bool_or:
            return ((*left != 0) || (*right != 0));
        case sleigh_runtime::PcodeOpcode::int_scarry:
        case sleigh_runtime::PcodeOpcode::int_sborrow: {
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
            if (operation.opcode == sleigh_runtime::PcodeOpcode::int_scarry) {
                return (right_signed > 0 && left_signed > std::numeric_limits<std::int64_t>::max() - right_signed) ||
                       (right_signed < 0 && left_signed < std::numeric_limits<std::int64_t>::min() - right_signed);
            }
            return (right_signed < 0 && left_signed > std::numeric_limits<std::int64_t>::max() + right_signed) ||
                   (right_signed > 0 && left_signed < std::numeric_limits<std::int64_t>::min() + right_signed);
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
    const auto address_it = address.space.name() == "const" ? std::optional<std::uint64_t>{address.offset}
                                                            : [&]() -> std::optional<std::uint64_t> {
        const auto found = values.find(location_key(address));
        return found == values.end() ? std::nullopt : std::optional{found->second};
    }();
    const auto value_it = value.space.name() == "const" ? std::optional<std::uint64_t>{value.offset}
                                                        : [&]() -> std::optional<std::uint64_t> {
        const auto found = values.find(location_key(value));
        return found == values.end() ? std::nullopt : std::optional{found->second};
    }();
    if (!address_it || !value_it) {
        return std::nullopt;
    }
    return std::pair{(operation.memory_space ? operation.memory_space->name() : std::string{"ram"}) + ":" +
                         std::to_string(*address_it),
                     *value_it};
}

} // namespace

/// Returns the Constant Propagation priority and code-event contract.
AnalyzerDescriptor ConstantPropagationAnalyzer::descriptor() const {
    // Function bodies are available synchronously from CreateFunctionCmd's
    // native context implementation; original Ghidra has no body analyzer
    // prerequisite to schedule here.
    return {"Constant Propagation", 596, {EventKind::code_added, EventKind::function_added}, {}};
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
            bool path_stable{true};
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
                    const auto value = evaluate_operation(operation, state.values, state.memory, context);
                    if (!value) {
                        // An unknown/unsupported write invalidates the prior
                        // fact. Retaining it would let a stale constant flow
                        // through a divide-by-zero, unresolved load, or other
                        // non-evaluable operation, unlike SymbolicPropogator.
                        if (operation.output->space == "ram")
                            state.memory.erase(location_key(*operation.output));
                        else
                            state.values.erase(location_key(*operation.output));
                        continue;
                    }
                    const auto normalized = truncate_value(*value, operation.output->size);
                    state.values[location_key(*operation.output)] = normalized;
                    static_cast<void>(context.add_constant_fact(
                        ConstantFact{address, *operation.output, normalized, state.path_stable, entry}));
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
                const bool paths_match = target->second.values == state.values && target->second.memory == state.memory;
                std::erase_if(joined.values, [&](const auto& item) {
                    const auto other = state.values.find(item.first);
                    return other == state.values.end() || other->second != item.second;
                });
                std::erase_if(joined.memory, [&](const auto& item) {
                    const auto other = state.memory.find(item.first);
                    return other == state.memory.end() || other->second != item.second;
                });
                joined.path_stable = target->second.path_stable && state.path_stable && paths_match;
                if (joined.values != target->second.values || joined.memory != target->second.memory ||
                    joined.path_stable != target->second.path_stable) {
                    target->second = std::move(joined);
                    worklist.push_back(successor);
                }
            }
        }
    }
}

} // namespace ghidra::analyzer
