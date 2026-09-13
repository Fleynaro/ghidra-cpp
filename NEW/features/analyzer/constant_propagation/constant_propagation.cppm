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
        std::map<std::string, std::uint64_t> values;
        std::map<std::string, std::uint64_t> memory;
        for (std::size_t pass = 0; pass < 8; ++pass) {
            bool changed = false;
            for (const Address address : function.body) {
                if (cancellation.is_cancelled()) {
                    return;
                }
                const auto instruction = context.instructions().find(address);
                if (instruction == context.instructions().end()) {
                    continue;
                }
                for (const auto& operation : instruction->second.instruction.pcode) {
                    if (operation.opcode == sleigh_runtime::PcodeOpcode::store) {
                        if (const auto stored = store_value(operation, values)) {
                            auto [location, value] = *stored;
                            if (memory[location] != value) {
                                memory[location] = value;
                                changed = true;
                            }
                        }
                        continue;
                    }
                    if (!operation.output) {
                        continue;
                    }
                    const auto value = evaluate_operation(operation, values, memory);
                    if (!value) {
                        continue;
                    }
                    const auto normalized = truncate_value(*value, operation.output->size);
                    const auto key = location_key(*operation.output);
                    if (!values.contains(key) || values[key] != normalized) {
                        values[key] = normalized;
                        changed = true;
                    }
                    context.add_constant_fact(ConstantFact{address, *operation.output, normalized});
                }
            }
            if (!changed) {
                break;
            }
        }
    }
}

} // namespace ghidra::analyzer
