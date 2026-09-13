module analyzer;

import std;

namespace ghidra::analyzer {

/// Returns the Stack analyzer priority and function-event contract.
AnalyzerDescriptor StackAnalyzer::descriptor() const {
    return {"Stack", 903, {EventKind::function_added}, {"Function Body"}};
}

/// Creates local stack variables and stack references from decoded operands.
void StackAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>, CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/StackVariableAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/NewFunctionStackAnalysisCmd.java
    // Relevant methods: added(), analyzeLocation(), and local-variable creation.
    if (!context.options().stack) {
        return;
    }
    for (const auto& [entry, function] : context.functions()) {
        for (const Address address : function.body) {
            if (cancellation.is_cancelled()) {
                return;
            }
            const auto instruction = context.instructions().find(address);
            if (instruction == context.instructions().end()) {
                continue;
            }
            for (const auto& operand : instruction->second.instruction.operands) {
                const std::string text = [&] {
                    std::string value = operand.text;
                    std::transform(value.begin(), value.end(), value.begin(),
                                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
                    return value;
                }();
                const auto stack = text.find("rsp") != std::string::npos ? text.find("rsp") : text.find("esp");
                if (stack == std::string::npos) {
                    continue;
                }
                const auto sign =
                    text.find('+', stack) != std::string::npos ? text.find('+', stack) : text.find('-', stack);
                std::int64_t offset = 0;
                if (sign != std::string::npos) {
                    const bool negative = text[sign] == '-';
                    std::string number = text.substr(sign + 1);
                    const auto close = number.find_first_of("])");
                    if (close != std::string::npos) {
                        number.resize(close);
                    }
                    if (number.ends_with('h')) {
                        number.pop_back();
                        number = "0x" + number;
                    }
                    const bool hexadecimal = number.starts_with("0x");
                    if (hexadecimal) {
                        number.erase(0, 2);
                    }
                    std::uint64_t parsed = 0;
                    const auto result =
                        std::from_chars(number.data(), number.data() + number.size(), parsed, hexadecimal ? 16 : 10);
                    if (result.ec != std::errc{} || result.ptr != number.data() + number.size()) {
                        continue;
                    }
                    offset = negative ? -static_cast<std::int64_t>(parsed) : static_cast<std::int64_t>(parsed);
                }
                const std::uint32_t size = 4;
                std::ostringstream name;
                name << "local_res" << std::hex << std::nouppercase << std::llabs(offset);
                context.add_stack_variable(entry, StackVariable{offset, size, name.str()});
                context.add_reference(Reference{address, 0, ReferenceKind::stack, std::nullopt, std::nullopt,
                                                FlowOverride::none, true, offset});
            }
        }
    }
}

} // namespace ghidra::analyzer
