export module analyzer_demangler_microsoft;

import analyzer;
import std;

// Original sources:
// Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/util/demangler/microsoft/MicrosoftDemangler.java
// Ghidra/Features/MicrosoftDemangler/src/main/java/ghidra/app/util/demangler/microsoft/MicrosoftDemangledFunction.java
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AbstractDemanglerAnalyzer.java

/// Describes a parsed Microsoft decorated symbol and its recoverable function contract.
export struct MicrosoftDemangledSymbol {
    std::string mangled;
    std::string qualified_name;
    std::string short_name;
    std::string namespace_name;
    std::string class_name;
    std::string calling_convention;
    std::string return_type;
    std::vector<std::string> parameter_types;
    bool function = false;
    bool non_static_member = false;
    bool valid = false;
};

/// Implements the Microsoft decorated-name grammar used by the analyzer and direct clients.
export class MicrosoftDemangler final {
public:
    /// Parses a decorated name, including nested scopes, templates, pointers, references, and functions.
    [[nodiscard]] MicrosoftDemangledSymbol demangle(std::string_view name) const;
};

/// Applies Microsoft demangled names and recoverable signatures to native symbols and functions.
export namespace ghidra::analyzer {
class DemanglerMicrosoftAnalyzer final : public Analyzer {
public:
    /// Returns the demangler analyzer's late code priority and symbol events.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Demangles local/exported and external symbols without replacing the existing PE parser.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace {

/// Returns the first Microsoft type token and advances the decorated-name cursor.
[[nodiscard]] std::string parse_type(std::string_view encoded, std::size_t& cursor, std::vector<std::string>& previous);

/// Parses a qualified class/struct/union/enum name ending at the Microsoft `@@` terminator.
[[nodiscard]] std::string parse_tagged_type(std::string_view encoded, std::size_t& cursor, std::string_view prefix) {
    const auto end = encoded.find("@@", cursor);
    if (end == std::string_view::npos) {
        cursor = encoded.size();
        return std::string(prefix) + "<unknown>";
    }
    std::string name(encoded.substr(cursor, end - cursor));
    cursor = end + 2U;
    std::vector<std::string> pieces;
    std::size_t start = 0;
    while (start <= name.size()) {
        const auto separator = name.find('@', start);
        pieces.push_back(name.substr(start, separator == std::string::npos ? std::string::npos : separator - start));
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1U;
    }
    std::reverse(pieces.begin(), pieces.end());
    std::string qualified;
    for (const auto& piece : pieces) {
        if (!qualified.empty()) {
            qualified += "::";
        }
        qualified += piece;
    }
    return std::string(prefix) + qualified;
}

/// Parses one template name from the compact name component representation.
[[nodiscard]] std::string parse_name_component(std::string component) {
    if (!component.starts_with("?$")) {
        return component;
    }
    component.erase(0, 2U);
    const auto separator = component.find('@');
    if (separator == std::string::npos) {
        return component;
    }
    const auto base = component.substr(0, separator);
    std::string result = base + "<";
    std::size_t argument_start = separator + 1U;
    bool first = true;
    while (argument_start < component.size()) {
        const auto next = component.find('@', argument_start);
        const auto argument =
            component.substr(argument_start, next == std::string::npos ? std::string::npos : next - argument_start);
        if (!argument.empty()) {
            if (!first) {
                result += ", ";
            }
            result += argument;
            first = false;
        }
        if (next == std::string::npos) {
            break;
        }
        argument_start = next + 1U;
    }
    return result + ">";
}

/// Converts a Microsoft calling-convention code to the public signature spelling.
[[nodiscard]] std::string calling_convention(char code) {
    switch (code) {
        case 'A':
            return "__cdecl";
        case 'E':
            return "__thiscall";
        case 'G':
            return "__stdcall";
        case 'I':
            return "__clrcall";
        case 'J':
            return "__vectorcall";
        case 'M':
            return "__thiscall";
        default:
            return "unknown";
    }
}

/// Returns the type spelling for one Microsoft primitive or compound type code.
[[nodiscard]] std::string parse_type(std::string_view encoded, std::size_t& cursor,
                                     std::vector<std::string>& previous) {
    if (cursor >= encoded.size()) {
        return "void";
    }
    const auto original = cursor;
    const char code = encoded[cursor++];
    if (code >= '0' && code <= '9') {
        const auto index = static_cast<std::size_t>(code - '0');
        return index < previous.size() ? previous[index] : "unknown";
    }
    std::string result;
    switch (code) {
        case 'X':
            result = "void";
            break;
        case 'C':
            result = "signed char";
            break;
        case 'D':
            result = "char";
            break;
        case 'E':
            result = "unsigned char";
            break;
        case 'F':
            result = "short";
            break;
        case 'G':
            result = "unsigned short";
            break;
        case 'H':
            result = "int";
            break;
        case 'I':
            result = "unsigned int";
            break;
        case 'J':
            result = "long";
            break;
        case 'K':
            result = "unsigned long";
            break;
        case 'M':
            result = "float";
            break;
        case 'N':
            result = "double";
            break;
        case 'O':
            result = "long double";
            break;
        case 'P': {
            while (cursor < encoded.size() &&
                   std::string_view{"AEIG"}.find(encoded[cursor]) != std::string_view::npos) {
                ++cursor;
            }
            result = parse_type(encoded, cursor, previous) + " *";
            break;
        }
        case 'Q': {
            while (cursor < encoded.size() &&
                   std::string_view{"AEIG"}.find(encoded[cursor]) != std::string_view::npos) {
                ++cursor;
            }
            result = parse_type(encoded, cursor, previous) + " * const";
            break;
        }
        case 'A':
            result = parse_type(encoded, cursor, previous) + " &";
            break;
        case 'V':
            result = parse_tagged_type(encoded, cursor, "struct ");
            break;
        case 'U':
            result = parse_tagged_type(encoded, cursor, "union ");
            break;
        case 'T':
            result = parse_tagged_type(encoded, cursor, "union ");
            break;
        case 'W':
            result = (cursor < encoded.size() && std::isdigit(static_cast<unsigned char>(encoded[cursor])) != 0)
                         ? parse_tagged_type(encoded, cursor, "enum ")
                         : "wchar_t";
            break;
        case '_':
            if (cursor < encoded.size()) {
                const auto extended = encoded[cursor++];
                result = extended == 'J'   ? "__int64"
                         : extended == 'K' ? "unsigned __int64"
                         : extended == 'N' ? "bool"
                                           : "unknown";
            } else {
                result = "unknown";
            }
            break;
        default:
            result = "unknown";
            break;
    }
    if (result == "unknown" && cursor == original) {
        ++cursor;
    }
    if (result != "void" && result != "unknown") {
        previous.push_back(result);
    }
    return result;
}

/// Parses a decorated function encoding following the terminating name `@@`.
void parse_function_encoding(std::string_view encoded, MicrosoftDemangledSymbol& result, std::string_view class_name) {
    if (encoded.size() < 3U) {
        return;
    }
    std::size_t cursor = 0;
    const char family = encoded[cursor++];
    char convention = '\0';
    if (family == 'Y' || family == 'S') {
        convention = cursor < encoded.size() ? encoded[cursor++] : '\0';
    } else if (family == 'Q' || family == 'R' || family == 'U' || family == 'V' || family == 'I') {
        convention = cursor < encoded.size() ? encoded[cursor] : '\0';
        cursor = std::min(encoded.size(), cursor + 3U);
        result.non_static_member = family != 'S';
    } else {
        return;
    }
    std::vector<std::string> previous;
    result.calling_convention = calling_convention(convention);
    result.return_type = parse_type(encoded, cursor, previous);
    while (cursor < encoded.size() && encoded[cursor] != 'Z') {
        if (encoded[cursor] == '@') {
            ++cursor;
            continue;
        }
        const auto type = parse_type(encoded, cursor, previous);
        if (type == "void" && cursor + 1U < encoded.size() && encoded[cursor] == '@' && encoded[cursor + 1U] == 'Z') {
            break;
        }
        result.parameter_types.push_back(type);
    }
    if (result.non_static_member) {
        result.parameter_types.insert(result.parameter_types.begin(), std::string(class_name) + " *");
    }
    result.function = true;
}

} // namespace

/// Parses Microsoft scopes, special members, template components, and broad function type grammar.
MicrosoftDemangledSymbol MicrosoftDemangler::demangle(std::string_view name) const {
    MicrosoftDemangledSymbol result;
    result.mangled = std::string(name);
    if (!name.starts_with('?')) {
        if (name.starts_with('_') && name.size() > 1U) {
            result.short_name = std::string(name.substr(1));
            result.qualified_name = result.short_name;
            result.valid = true;
        }
        return result;
    }
    std::size_t special_offset = 1U;
    if (name.starts_with("??0")) {
        special_offset = 3U;
    } else if (name.starts_with("??1")) {
        special_offset = 3U;
    } else if (name.starts_with("??_")) {
        special_offset = 4U;
    }
    const auto name_end = name.find("@@", special_offset);
    if (name_end == std::string_view::npos) {
        return result;
    }
    std::string name_part(name.substr(special_offset, name_end - special_offset));
    std::vector<std::string> components;
    std::size_t start = 0;
    while (start <= name_part.size()) {
        const auto separator = name_part.find('@', start);
        components.push_back(parse_name_component(
            name_part.substr(start, separator == std::string::npos ? std::string::npos : separator - start)));
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1U;
    }
    std::string special_name;
    if (name.starts_with("??0") && !components.empty()) {
        special_name = components.front();
        components.front() = special_name;
    } else if (name.starts_with("??1") && !components.empty()) {
        special_name = "~" + components.front();
        components.front() = special_name;
    } else if (name.starts_with("??_7") && !components.empty()) {
        special_name = "`vftable'";
        components.front() = special_name;
    }
    std::reverse(components.begin(), components.end());
    result.short_name = components.empty() ? std::string{} : components.back();
    if (components.size() > 1U) {
        for (std::size_t index = 0; index + 1U < components.size(); ++index) {
            if (!result.namespace_name.empty()) {
                result.namespace_name += "::";
            }
            result.namespace_name += components[index];
        }
    }
    result.qualified_name =
        result.namespace_name.empty() ? result.short_name : result.namespace_name + "::" + result.short_name;
    result.class_name = components.size() > 1U ? components[components.size() - 2U] : std::string{};
    parse_function_encoding(name.substr(name_end + 2U), result, result.class_name);
    result.valid = true;
    return result;
}

namespace ghidra::analyzer {

/// Returns the Microsoft demangler's late code-analysis descriptor.
AnalyzerDescriptor DemanglerMicrosoftAnalyzer::descriptor() const {
    return {"Demangler Microsoft",
            897,
            {EventKind::external_added, EventKind::function_added, EventKind::function_changed},
            {}};
}

/// Applies broad Microsoft demangled names and signatures through the public context API.
void DemanglerMicrosoftAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                         CancellationToken& cancellation) {
    if (!context.options().demangler_microsoft) {
        return;
    }
    const MicrosoftDemangler demangler;
    const auto apply = [&](std::string_view mangled, std::optional<Address> address, bool external) {
        const auto parsed = demangler.demangle(mangled);
        if (!parsed.valid || parsed.short_name.empty()) {
            return;
        }
        const auto display_name = context.options().demangler_apply_namespaces ? parsed.short_name : parsed.short_name;
        static_cast<void>(
            context.add_symbol(SymbolRecord{address.value_or(0), std::string(mangled), display_name,
                                            parsed.namespace_name, "demangler_microsoft", external, true}));
        if (external) {
            static_cast<void>(context.set_external_demangled_name(address.value_or(0), display_name));
            return;
        }
        if (!address || !context.function_at(*address)) {
            return;
        }
        static_cast<void>(context.set_function_name(*address, display_name));
        if (!context.options().demangler_apply_function_signatures || !parsed.function) {
            return;
        }
        std::vector<FunctionParameter> parameters;
        for (std::size_t index = 0; index < parsed.parameter_types.size(); ++index) {
            parameters.push_back(FunctionParameter{
                index == 0U && parsed.non_static_member ? "this" : "param_" + std::to_string(index),
                parsed.parameter_types[index], "register", static_cast<std::int64_t>(index * 8U), 8U, false});
        }
        static_cast<void>(
            context.set_function_signature(*address, parsed.calling_convention, parsed.return_type, parameters, false));
    };
    for (const auto& symbol : context.external_symbols()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        apply(symbol.name, symbol.iat_address, true);
    }
    for (const auto& symbol : context.image().exported_symbols()) {
        if (cancellation.is_cancelled() || symbol.forwarded || !symbol.name) {
            continue;
        }
        apply(*symbol.name, symbol.address_va, false);
    }
}

} // namespace ghidra::analyzer
