export module analyzer_variadic_function_signature_override;

import analyzer;
import std;

// Ported behavior is traced to
// Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/
// FormatStringAnalyzer.java and FormatStringParser.java.

export namespace recode::analyzer {

/// Describes one argument introduced by a printf/scanf conversion.
struct FormatArgument {
    std::string type;
    bool indirect{};
};

/// Reports malformed format syntax without accepting a partial signature.
struct FormatStringError {
    std::string message;
};

/// Parses printf/scanf conversions into decompiler type spellings.
[[nodiscard]] std::expected<std::vector<FormatArgument>, FormatStringError> parse_format_string(std::string_view format,
                                                                                                bool scanf_output);

/// Applies representable variadic call-site evidence from format strings.
class VariadicFunctionSignatureOverrideAnalyzer final : public Analyzer {
public:
    /// Returns the low-priority one-time analyzer scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Finds format-string calls and records exact call-site override evidence.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Skips one bounded decimal field.
void skip_digits(std::string_view value, std::size_t& position) {
    while (position < value.size() && value[position] >= '0' && value[position] <= '9')
        ++position;
}

/// Returns a C string at a concrete VA or RVA present in the loaded image.
[[nodiscard]] std::optional<std::string> read_string(const AnalysisContext& context, std::uint64_t candidate) {
    std::uint64_t address = candidate;
    if (!context.image().find_memory_region(address)) {
        if (const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(candidate)))
            address = *translated;
    }
    if (!context.image().find_memory_region(address))
        return std::nullopt;
    std::string result;
    for (std::size_t count = 0; count < 4096U; ++count) {
        const auto byte = context.image().read_memory(address + count, 1U);
        if (!byte)
            return std::nullopt;
        if ((*byte)[0] == 0U)
            return result;
        if ((*byte)[0] < 0x20U && (*byte)[0] != '\t' && (*byte)[0] != '\n')
            return std::nullopt;
        result.push_back(static_cast<char>((*byte)[0]));
    }
    return std::nullopt;
}

/// Finds a literal format-string pointer in p-code emitted before a call.
[[nodiscard]] std::optional<std::string> format_at_call(const AnalysisContext& context, Address call) {
    const auto instruction = context.instructions().find(call);
    if (instruction == context.instructions().end())
        return std::nullopt;
    for (const auto& operation : instruction->second.instruction.pcode)
        for (const auto& input : operation.inputs)
            if (input.space == "const" || input.space == "ram")
                if (const auto value = read_string(context, input.offset))
                    if (value->find('%') != std::string::npos)
                        return value;
    return std::nullopt;
}

/// Returns whether a function has the exact variadic format-string shape supported by Ghidra.
[[nodiscard]] bool is_variadic_format_function(const Function& function) {
    if (!function.variadic || function.parameters.empty())
        return false;
    const auto type = function.parameters.back().type;
    const auto normalized = type.find("char") != std::string::npos || type.find("wchar") != std::string::npos;
    return normalized && type.find('*') != std::string::npos;
}

} // namespace

/// Parses the supported flags, width, precision, length, suppression, and conversion grammar.
std::expected<std::vector<FormatArgument>, FormatStringError> parse_format_string(std::string_view format,
                                                                                  bool scanf_output) {
    std::vector<FormatArgument> arguments;
    for (std::size_t position = 0; position < format.size(); ++position) {
        if (format[position] != '%')
            continue;
        ++position;
        if (position == format.size())
            return std::unexpected(FormatStringError{"format ends after '%'"});
        if (format[position] == '%')
            continue;
        bool suppressed = false;
        if (scanf_output && format[position] == '*') {
            suppressed = true;
            ++position;
        }
        while (position < format.size() && std::string_view("-+ #0'").find(format[position]) != std::string_view::npos)
            ++position;
        if (position < format.size() && format[position] == '*')
            ++position;
        else
            skip_digits(format, position);
        if (position < format.size() && format[position] == '.') {
            ++position;
            if (position < format.size() && format[position] == '*')
                ++position;
            else
                skip_digits(format, position);
        }
        std::string length;
        for (const std::string_view candidate : {"hh", "ll", "I64", "I32", "h", "l", "j", "z", "t", "L"})
            if (format.substr(position).starts_with(candidate)) {
                length = candidate;
                position += candidate.size();
                break;
            }
        if (position >= format.size())
            return std::unexpected(FormatStringError{"format conversion is missing"});
        const char conversion = format[position];
        if (suppressed)
            continue;
        std::string type;
        switch (conversion) {
            case 'd':
            case 'i':
                type = "int";
                if (length == "hh")
                    type = "signed char";
                else if (length == "h")
                    type = "short";
                else if (length == "l")
                    type = "long";
                else if (length == "ll" || length == "I64" || length == "j")
                    type = "long long";
                break;
            case 'o':
            case 'u':
            case 'x':
            case 'X':
                type = "unsigned int";
                if (length == "h")
                    type = "unsigned short";
                else if (length == "l")
                    type = "unsigned long";
                else if (length == "ll" || length == "I64" || length == "j")
                    type = "unsigned long long";
                break;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                if (scanf_output)
                    type = length == "l" || length == "L" ? "double *" : "float *";
                else
                    type = length == "L" ? "long double" : "double";
                break;
            case 'c':
                type = length == "l" ? "wint_t" : "char";
                break;
            case 's':
                type = length == "l" ? "wchar_t *" : "char *";
                break;
            case 'p':
                type = scanf_output ? "void **" : "void *";
                break;
            case 'n':
                if (length == "l")
                    type = "long *";
                else if (length == "ll" || length == "I64")
                    type = "long long *";
                else
                    type = "int *";
                break;
            default:
                return std::unexpected(
                    FormatStringError{"unsupported conversion '%" + std::string(1, conversion) + "'"});
        }
        if (scanf_output && type.find('*') == std::string::npos)
            type += " *";
        arguments.push_back({type, scanf_output});
    }
    return arguments;
}

/// Returns the variadic override scheduling contract.
AnalyzerDescriptor VariadicFunctionSignatureOverrideAnalyzer::descriptor() const {
    return {"Variadic Function Signature Override", 1000, {EventKind::memory_added}, {}};
}

/// Records one call-site's parsed variadic argument contract without corrupting the target function signature.
void VariadicFunctionSignatureOverrideAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                                        CancellationToken& cancellation) {
    if (!context.options().variadic_function_signature_override)
        return;
    std::vector<std::pair<Address, const Function*>> targets;
    for (const auto& [entry, function] : context.functions())
        if (is_variadic_format_function(function) &&
            (function.name.find("printf") != std::string::npos || function.name.find("scanf") != std::string::npos))
            targets.emplace_back(entry, &function);
    for (const auto& [target, function] : targets) {
        if (cancellation.is_cancelled())
            return;
        for (const auto& reference : context.references()) {
            if (reference.target != target || (reference.kind != ReferenceKind::unconditional_call &&
                                               reference.kind != ReferenceKind::conditional_call))
                continue;
            const auto format = format_at_call(context, reference.source);
            if (!format)
                continue;
            const bool scanf_output = function->name.find("scanf") != std::string::npos;
            const auto parsed = parse_format_string(*format, scanf_output);
            if (!parsed) {
                if (context.options().create_analysis_bookmarks)
                    static_cast<void>(context.add_bookmark(Bookmark{
                        reference.source, "Analysis", "Format string parse failed: " + parsed.error().message}));
                continue;
            }
            if (parsed->empty())
                continue;
            if (context.options().create_analysis_bookmarks) {
                std::string comment = "Override for call to " + function->name + ": ";
                for (std::size_t index = 0; index < parsed->size(); ++index) {
                    if (index != 0U)
                        comment += ", ";
                    comment += (*parsed)[index].type;
                }
                static_cast<void>(
                    context.add_bookmark(Bookmark{reference.source, "Function Signature Override", comment}));
            }
            // Ghidra stores this as a call-site signature override. The native
            // context intentionally has no call-site override table, so changing
            // the callee's persistent signature here would be incorrect.
        }
    }
}

} // namespace recode::analyzer
