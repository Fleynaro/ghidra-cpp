export module analyzer_stack;

import analyzer;
import std;

/// Owns the stack variable analyzer declaration and implementation.
export namespace ghidra::analyzer {
class StackAnalyzer final : public Analyzer {
public:
    /// Returns the StackVariableAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Performs stack-frame discovery for newly created functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Parses a signed stack/frame-pointer displacement from Sleigh operand text.
[[nodiscard]] std::optional<std::pair<std::string, std::int64_t>> parse_stack_operand(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    const std::array<std::string_view, 4> registers{"rsp", "esp", "rbp", "ebp"};
    std::size_t register_position = std::string::npos;
    std::string register_name;
    for (const auto register_value : registers) {
        const auto position = text.find(register_value);
        if (position != std::string::npos && position < register_position) {
            register_position = position;
            register_name = std::string(register_value);
        }
    }
    if (register_position == std::string::npos)
        return std::nullopt;
    const auto plus = text.find('+', register_position);
    const auto minus = text.find('-', register_position);
    if (plus == std::string::npos && minus == std::string::npos) {
        // A bare register operand (PUSH RBP, SUB RSP, ...) is not a stack
        // storage reference; only [rsp]/[rbp] without displacement denotes 0.
        if (text.find('[', register_position) == std::string::npos &&
            text.find(']', register_position) == std::string::npos)
            return std::nullopt;
        return std::pair{register_name, 0LL};
    }
    const bool negative = minus != std::string::npos && (plus == std::string::npos || minus < plus);
    auto start = (negative ? minus : plus) + 1;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0)
        ++start;
    auto end = start;
    while (end < text.size() && (std::isxdigit(static_cast<unsigned char>(text[end])) != 0 || text[end] == 'x'))
        ++end;
    std::string value = text.substr(start, end - start);
    if (value.ends_with('h')) {
        value.pop_back();
        value = "0x" + value;
    }
    const bool hexadecimal = value.starts_with("0x");
    if (hexadecimal)
        value.erase(0, 2);
    std::uint64_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed, hexadecimal ? 16 : 10);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
        return std::nullopt;
    return std::pair{register_name, negative ? -static_cast<std::int64_t>(parsed) : static_cast<std::int64_t>(parsed)};
}

/// Infers storage width from an x86 operand-size qualifier.
[[nodiscard]] std::uint32_t operand_size(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (text.find("byte ptr") != std::string::npos)
        return 1;
    if (text.find("qword ptr") != std::string::npos)
        return 8;
    if (text.find("word ptr") != std::string::npos)
        return 2;
    return 4;
}

/// Parses an immediate after a stack-pointer allocation instruction.
[[nodiscard]] std::optional<std::uint32_t> frame_immediate(std::string text) {
    const auto comma = text.find(',');
    if (comma == std::string::npos)
        return std::nullopt;
    text = text.substr(comma + 1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0)
        text.erase(text.begin());
    if (text.ends_with('h')) {
        text.pop_back();
        text = "0x" + text;
    }
    const bool hexadecimal = text.starts_with("0x");
    if (hexadecimal)
        text.erase(0, 2);
    std::uint32_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, hexadecimal ? 16 : 10);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() ? std::optional{value} : std::nullopt;
}

} // namespace

/// Returns the Stack analyzer priority and function-event contract.
AnalyzerDescriptor StackAnalyzer::descriptor() const {
    return {"Stack", 903, {EventKind::function_added, EventKind::function_changed}, {"Function Body"}};
}

/// Creates stack-frame metadata, typed locals/parameters, and stack references.
void StackAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>, CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/StackVariableAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/NewFunctionStackAnalysisCmd.java
    // Relevant methods: added(), analyzeLocation(), stack-frame construction, and local-variable creation.
    if (!context.options().stack)
        return;
    for (const auto& [entry, function] : context.functions()) {
        std::uint32_t frame_size = 0;
        std::int64_t stack_pointer_delta = 0;
        std::int64_t frame_pointer_delta = 0;
        std::optional<std::string> frame_pointer;
        for (const Address address : function.instruction_starts) {
            if (cancellation.is_cancelled())
                return;
            const auto instruction = context.instructions().find(address);
            if (instruction == context.instructions().end())
                continue;
            // Sleigh stores the mnemonic and operand body separately. The
            // original NewFunctionStackAnalysisCmd consumes instruction
            // semantics, so combine both fields before recognizing prologue
            // and epilogue instructions.
            std::string assembly =
                instruction->second.instruction.mnemonic + " " + instruction->second.instruction.assembly;
            std::transform(assembly.begin(), assembly.end(), assembly.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            if (assembly.find("sub rsp") != std::string::npos || assembly.find("sub esp") != std::string::npos) {
                if (const auto immediate = frame_immediate(assembly)) {
                    frame_size += *immediate;
                    stack_pointer_delta -= *immediate;
                }
            }
            if (assembly.find("add rsp") != std::string::npos || assembly.find("add esp") != std::string::npos) {
                if (const auto immediate = frame_immediate(assembly))
                    stack_pointer_delta += *immediate;
            }
            if (assembly.find("push rbp") != std::string::npos)
                stack_pointer_delta -= 8;
            if (assembly.find("pop rbp") != std::string::npos)
                stack_pointer_delta += 8;
            if (assembly.find("mov rbp, rsp") != std::string::npos) {
                frame_pointer = "rbp";
                frame_pointer_delta = stack_pointer_delta;
            }
            bool found_stack_storage = false;
            const auto record_storage = [&](std::string_view storage_text,
                                            const std::pair<std::string, std::int64_t>& parsed) {
                const auto& [register_name, offset] = parsed;
                static_cast<void>(register_name);
                if (offset >= 2048 || offset < -65536) {
                    return;
                }
                const bool parameter = offset >= 0 && context.options().create_stack_parameters;
                std::ostringstream name;
                if (parameter)
                    name << "param_" << std::hex << offset;
                else
                    name << "local_res" << std::hex << std::nouppercase << std::llabs(offset);
                const auto size = operand_size(std::string(storage_text));
                static_cast<void>(
                    context.add_stack_variable(entry, StackVariable{offset, size, name.str(), parameter}));
                static_cast<void>(context.add_reference(Reference{address, 0, ReferenceKind::stack, std::nullopt,
                                                                  std::nullopt, FlowOverride::none, true, offset}));
                found_stack_storage = true;
            };
            for (const auto& operand : instruction->second.instruction.operands) {
                const auto parsed = parse_stack_operand(operand.text);
                if (!parsed)
                    continue;
                auto adjusted = *parsed;
                adjusted.second += adjusted.first.starts_with("rbp") || adjusted.first.starts_with("ebp")
                                       ? frame_pointer_delta
                                       : stack_pointer_delta;
                record_storage(operand.text, adjusted);
            }
            if (!found_stack_storage) {
                if (const auto parsed = parse_stack_operand(assembly)) {
                    auto adjusted = *parsed;
                    adjusted.second += adjusted.first.starts_with("rbp") || adjusted.first.starts_with("ebp")
                                           ? frame_pointer_delta
                                           : stack_pointer_delta;
                    record_storage(assembly, adjusted);
                }
            }
        }
        static_cast<void>(context.set_function_stack_frame(entry, frame_size, stack_pointer_delta, frame_pointer));
    }
}

} // namespace ghidra::analyzer
