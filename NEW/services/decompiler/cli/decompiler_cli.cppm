import std;

import decompiler;
import sleigh_runtime;

namespace {

using newghidra::decompiler::ArchitectureDescription;
using newghidra::decompiler::ConstantFormatDescription;
using newghidra::decompiler::DecompilationResult;
using newghidra::decompiler::Decompiler;
using newghidra::decompiler::DisplayFormat;
using newghidra::decompiler::FlowDescription;
using newghidra::decompiler::FlowProvider;
using newghidra::decompiler::FunctionDescription;
using newghidra::decompiler::FunctionProvider;
using newghidra::decompiler::Instruction;
using newghidra::decompiler::JumpTableDescription;
using newghidra::decompiler::MemoryRangeDescription;
using newghidra::decompiler::PcodeOperation;
using newghidra::decompiler::PrototypeDescription;
using newghidra::decompiler::PrototypeParameterDescription;
using newghidra::decompiler::ProviderContext;
using newghidra::decompiler::ProviderError;
using newghidra::decompiler::RegisterDescription;
using newghidra::decompiler::SpaceDescription;
using newghidra::decompiler::Storage;
using newghidra::decompiler::SymbolDescription;
using newghidra::decompiler::SymbolKind;
using newghidra::decompiler::TypeBitFieldDescription;
using newghidra::decompiler::TypeDescription;
using newghidra::decompiler::TypeEnumValueDescription;
using newghidra::decompiler::TypeFieldDescription;
using newghidra::decompiler::TypeKind;
using newghidra::decompiler::VariableDescription;

/// Selects one or more rendered artifacts from a decompilation result.
enum class OutputSection { summary, assembly, raw_pcode, high_pcode, data_flow, control_flow, ast, c_source };

/// Owns all command-line values and provider metadata for one CLI invocation.
/// Repeated metadata options are intentionally represented as vectors so the
/// command line can describe the complete public provider contract.
struct Options {
    std::filesystem::path sla_path;
    std::string hex;
    std::vector<std::pair<std::uint64_t, std::string>> data_chunks;
    std::uint64_t address = 0x140000000ULL;
    std::optional<std::uint64_t> end;
    std::string function_name = "function";
    std::vector<OutputSection> output{OutputSection::summary,    OutputSection::assembly,  OutputSection::raw_pcode,
                                      OutputSection::high_pcode, OutputSection::data_flow, OutputSection::control_flow,
                                      OutputSection::ast,        OutputSection::c_source};
    ArchitectureDescription architecture;
    std::vector<std::pair<std::string, std::uint64_t>> context{
        {"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}};
    std::vector<SymbolDescription> symbols;
    std::vector<TypeDescription> types;
    std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes;
    std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> variables;
    std::vector<FunctionDescription> functions;
    std::vector<std::pair<std::uint64_t, FlowDescription>> flows;
    std::vector<MemoryRangeDescription> volatile_ranges;
    bool readonly_propagate = false;
};

/// Parses an unsigned decimal or `0x` hexadecimal integer.
std::uint64_t parse_integer(std::string_view text, std::string_view option) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        base = 16;
        text.remove_prefix(2);
    }
    if (text.empty()) {
        throw std::invalid_argument(std::format("{} requires a non-empty integer", option));
    }
    std::uint64_t result = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result, base);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument(std::format("invalid integer '{}' for {}", text, option));
    }
    return result;
}

/// Parses a signed integer used by relative offsets and enum values.
std::int64_t parse_signed_integer(std::string_view text, std::string_view option) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        base = 16;
        text.remove_prefix(2);
    }
    std::int64_t result = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result, base);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument(std::format("invalid signed integer '{}' for {}", text, option));
    }
    return result;
}

/// Parses a boolean command-line value accepted by metadata options.
bool parse_bool(std::string_view text, std::string_view option) {
    if (text == "1" || text == "true" || text == "on" || text == "yes") {
        return true;
    }
    if (text == "0" || text == "false" || text == "off" || text == "no") {
        return false;
    }
    throw std::invalid_argument(std::format("invalid boolean '{}' for {}", text, option));
}

/// Splits a comma-separated `key=value` record without trimming user names.
std::map<std::string, std::string> assignments(std::string_view text, std::string_view option) {
    std::map<std::string, std::string> result;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        const std::size_t end = text.find(',', begin);
        const std::string_view item = text.substr(begin, end == std::string_view::npos ? end : end - begin);
        const std::size_t separator = item.find('=');
        if (separator == std::string_view::npos || separator == 0 || separator + 1 == item.size()) {
            throw std::invalid_argument(std::format("{} expects comma-separated key=value fields", option));
        }
        result.emplace(std::string(item.substr(0, separator)), std::string(item.substr(separator + 1)));
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1;
    }
    return result;
}

/// Returns a required field from a metadata record.
std::string field(const std::map<std::string, std::string>& values, std::string_view name, std::string_view option,
                  bool required = true) {
    const auto iterator = values.find(std::string(name));
    if (iterator == values.end()) {
        if (required) {
            throw std::invalid_argument(std::format("{} is missing '{}'", option, name));
        }
        return {};
    }
    return iterator->second;
}

/// Converts whitespace, comma, and `0x`-free hexadecimal text to bytes.
std::vector<std::uint8_t> parse_hex(std::string_view text, std::string_view option = "--hex") {
    std::string compact;
    for (const char character : text) {
        if (std::isxdigit(static_cast<unsigned char>(character)) != 0) {
            compact.push_back(character);
        } else if (!std::isspace(static_cast<unsigned char>(character)) && character != ',') {
            throw std::invalid_argument(std::format("invalid character '{}' in {}", character, option));
        }
    }
    if (compact.empty() || compact.size() % 2 != 0) {
        throw std::invalid_argument(std::format("{} must contain complete hexadecimal byte pairs", option));
    }
    std::vector<std::uint8_t> result;
    result.reserve(compact.size() / 2);
    for (std::size_t index = 0; index < compact.size(); index += 2) {
        std::uint32_t value = 0;
        const auto pair = std::string_view(compact).substr(index, 2);
        const auto [end, error] = std::from_chars(pair.data(), pair.data() + pair.size(), value, 16);
        if (error != std::errc{} || end != pair.data() + pair.size()) {
            throw std::invalid_argument(std::format("invalid byte '{}' in {}", pair, option));
        }
        result.push_back(static_cast<std::uint8_t>(value));
    }
    return result;
}

/// Parses one provider storage location written as `space:offset:size`.
Storage parse_storage(std::string_view text, std::string_view option) {
    const std::size_t first = text.find(':');
    const std::size_t second = first == std::string_view::npos ? std::string_view::npos : text.find(':', first + 1);
    if (first == std::string_view::npos || second == std::string_view::npos || first == 0 ||
        second + 1 == text.size()) {
        throw std::invalid_argument(std::format("{} expects space:offset:size", option));
    }
    return Storage{std::string(text.substr(0, first)),
                   parse_integer(text.substr(first + 1, second - first - 1), option),
                   static_cast<std::uint32_t>(parse_integer(text.substr(second + 1), option))};
}

/// Maps a public type-kind spelling to the native provider enum.
TypeKind parse_type_kind(std::string_view text) {
    static const std::map<std::string_view, TypeKind> kinds{{"void", TypeKind::void_type},
                                                            {"bool", TypeKind::boolean},
                                                            {"boolean", TypeKind::boolean},
                                                            {"signed", TypeKind::signed_integer},
                                                            {"signed_integer", TypeKind::signed_integer},
                                                            {"unsigned", TypeKind::unsigned_integer},
                                                            {"unsigned_integer", TypeKind::unsigned_integer},
                                                            {"float", TypeKind::floating_point},
                                                            {"floating", TypeKind::floating_point},
                                                            {"char", TypeKind::unicode_character},
                                                            {"pointer", TypeKind::pointer},
                                                            {"array", TypeKind::array},
                                                            {"struct", TypeKind::structure},
                                                            {"structure", TypeKind::structure},
                                                            {"union", TypeKind::union_type},
                                                            {"typedef", TypeKind::typedef_type},
                                                            {"enum", TypeKind::enumeration},
                                                            {"enumeration", TypeKind::enumeration}};
    const auto iterator = kinds.find(text);
    if (iterator == kinds.end()) {
        throw std::invalid_argument(std::format("unknown type kind '{}'", text));
    }
    return iterator->second;
}

/// Maps output names accepted by `--show` and `--only`.
OutputSection parse_output_section(std::string_view text) {
    static const std::map<std::string_view, OutputSection> names{
        {"summary", OutputSection::summary},      {"assembly", OutputSection::assembly},
        {"asm", OutputSection::assembly},         {"raw-pcode", OutputSection::raw_pcode},
        {"raw", OutputSection::raw_pcode},        {"high-pcode", OutputSection::high_pcode},
        {"high", OutputSection::high_pcode},      {"data-flow", OutputSection::data_flow},
        {"data", OutputSection::data_flow},       {"control-flow", OutputSection::control_flow},
        {"control", OutputSection::control_flow}, {"ast", OutputSection::ast},
        {"c", OutputSection::c_source},           {"c-source", OutputSection::c_source},
    };
    const auto iterator = names.find(text);
    if (iterator == names.end()) {
        throw std::invalid_argument(std::format("unknown output section '{}'", text));
    }
    return iterator->second;
}

/// Appends comma-separated output section names while preserving their order.
void set_output(Options& options, std::string_view value) {
    if (value == "all") {
        options.output = {OutputSection::summary,    OutputSection::assembly,  OutputSection::raw_pcode,
                          OutputSection::high_pcode, OutputSection::data_flow, OutputSection::control_flow,
                          OutputSection::ast,        OutputSection::c_source};
        return;
    }
    options.output.clear();
    std::size_t begin = 0;
    while (begin < value.size()) {
        const std::size_t end = value.find(',', begin);
        options.output.push_back(
            parse_output_section(value.substr(begin, end == std::string_view::npos ? end : end - begin)));
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1;
    }
}

/// Returns the x86-64 architecture used by the checked-in Sleigh fixture and
/// by the portable machine-code datatests.
ArchitectureDescription default_architecture() {
    ArchitectureDescription architecture;
    architecture.name = "x86-64";
    architecture.calling_convention = "__cdecl";
    architecture.spaces = {SpaceDescription{"ram", 8, 1, false, 2, 0, true},
                           SpaceDescription{"register", 8, 1, false, 3, 0, true}};
    architecture.registers = {
        RegisterDescription{"RAX", Storage{"register", 0, 8}},
        RegisterDescription{"RCX", Storage{"register", 8, 8}},
        RegisterDescription{"RDX", Storage{"register", 0x10, 8}},
        RegisterDescription{"RBX", Storage{"register", 0x18, 8}},
        RegisterDescription{"RSP", Storage{"register", 0x20, 8}},
        RegisterDescription{"RBP", Storage{"register", 0x28, 8}},
        RegisterDescription{"RSI", Storage{"register", 0x30, 8}},
        RegisterDescription{"RDI", Storage{"register", 0x38, 8}},
        RegisterDescription{"R8", Storage{"register", 0x80, 8}},
        RegisterDescription{"R9", Storage{"register", 0x88, 8}},
        RegisterDescription{"R10", Storage{"register", 0x90, 8}},
        RegisterDescription{"R11", Storage{"register", 0x98, 8}},
        RegisterDescription{"XMM0", Storage{"register", 0x1200, 16}},
        RegisterDescription{"XMM1", Storage{"register", 0x1210, 16}},
    };
    return architecture;
}

/// Parses all CLI options, including repeated provider metadata records.
Options parse_options(int argc, char* argv[]) {
    Options options;
    options.architecture = default_architecture();
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        const auto require_value = [&](std::string_view option) -> std::string_view {
            if (index + 1 >= argc) {
                throw std::invalid_argument(std::format("{} requires a value", option));
            }
            return argv[++index];
        };
        if (argument == "--help" || argument == "-h") {
            throw std::runtime_error("help");
        }
        if (argument == "--sla") {
            options.sla_path = require_value(argument);
        } else if (argument == "--hex") {
            options.hex = require_value(argument);
        } else if (argument == "--hex-file") {
            const std::filesystem::path path(require_value(argument));
            std::ifstream input(path);
            if (!input) {
                throw std::invalid_argument("cannot open --hex-file: " + path.string());
            }
            options.hex.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        } else if (argument == "--data") {
            const std::string_view value = require_value(argument);
            const std::size_t separator = value.find(':');
            if (separator == std::string_view::npos) {
                throw std::invalid_argument("--data expects address:hex");
            }
            options.data_chunks.emplace_back(parse_integer(value.substr(0, separator), argument),
                                             std::string(value.substr(separator + 1)));
        } else if (argument == "--address") {
            options.address = parse_integer(require_value(argument), argument);
        } else if (argument == "--end") {
            options.end = parse_integer(require_value(argument), argument);
        } else if (argument == "--size") {
            const std::uint64_t size = parse_integer(require_value(argument), argument);
            if (options.address > std::numeric_limits<std::uint64_t>::max() - size) {
                throw std::invalid_argument("--size overflows the function address");
            }
            options.end = options.address + size;
        } else if (argument == "--name") {
            options.function_name = require_value(argument);
        } else if (argument == "--context") {
            const auto value = require_value(argument);
            const std::size_t separator = value.find('=');
            if (separator == std::string_view::npos || separator == 0) {
                throw std::invalid_argument("--context expects name=value");
            }
            options.context.emplace_back(std::string(value.substr(0, separator)),
                                         parse_integer(value.substr(separator + 1), argument));
        } else if (argument == "--show" || argument == "--only") {
            set_output(options, require_value(argument));
        } else if (argument == "--arch-name") {
            options.architecture.name = require_value(argument);
        } else if (argument == "--code-space") {
            options.architecture.code_space = require_value(argument);
        } else if (argument == "--data-space") {
            options.architecture.data_space = require_value(argument);
        } else if (argument == "--stack-register") {
            options.architecture.stack_register = require_value(argument);
        } else if (argument == "--pointer-size") {
            options.architecture.pointer_size =
                static_cast<std::uint32_t>(parse_integer(require_value(argument), argument));
        } else if (argument == "--space") {
            const auto values = assignments(require_value(argument), argument);
            SpaceDescription space;
            space.name = field(values, "name", argument);
            space.address_size =
                static_cast<std::uint32_t>(parse_integer(field(values, "address-size", argument), argument));
            space.word_size = static_cast<std::uint32_t>(parse_integer(
                field(values, "word-size", argument, false).empty() ? "1" : field(values, "word-size", argument),
                argument));
            space.big_endian = parse_bool(
                field(values, "big-endian", argument, false).empty() ? "false" : field(values, "big-endian", argument),
                argument);
            space.index = static_cast<std::int32_t>(parse_signed_integer(
                field(values, "index", argument, false).empty() ? "-1" : field(values, "index", argument), argument));
            space.physical = parse_bool(
                field(values, "physical", argument, false).empty() ? "true" : field(values, "physical", argument),
                argument);
            space.overlay_base = field(values, "overlay", argument, false);
            options.architecture.spaces.push_back(std::move(space));
        } else if (argument == "--register") {
            const auto values = assignments(require_value(argument), argument);
            options.architecture.registers.push_back(RegisterDescription{
                field(values, "name", argument), parse_storage(field(values, "storage", argument), argument)});
        } else if (argument == "--symbol") {
            const auto values = assignments(require_value(argument), argument);
            SymbolDescription symbol;
            symbol.address = parse_integer(field(values, "address", argument), argument);
            symbol.name = field(values, "name", argument);
            symbol.namespace_name = field(values, "namespace", argument, false);
            symbol.kind = field(values, "kind", argument, false) == "data" ? SymbolKind::data : SymbolKind::function;
            symbol.size = static_cast<std::uint32_t>(parse_integer(
                field(values, "size", argument, false).empty() ? "0" : field(values, "size", argument), argument));
            symbol.type_name = field(values, "type", argument, false);
            symbol.read_only = parse_bool(
                field(values, "readonly", argument, false).empty() ? "false" : field(values, "readonly", argument),
                argument);
            symbol.space = field(values, "space", argument, false);
            symbol.identity = parse_integer(
                field(values, "identity", argument, false).empty() ? "0" : field(values, "identity", argument),
                argument);
            symbol.alias_identity = parse_integer(
                field(values, "alias", argument, false).empty() ? "0" : field(values, "alias", argument), argument);
            options.symbols.push_back(std::move(symbol));
        } else if (argument == "--type") {
            const auto values = assignments(require_value(argument), argument);
            TypeDescription type;
            type.name = field(values, "name", argument);
            type.kind = parse_type_kind(field(values, "kind", argument));
            type.size = static_cast<std::uint32_t>(parse_integer(field(values, "size", argument), argument));
            type.signed_value = parse_bool(field(values, "signed", argument, false).empty()
                                               ? (type.kind == TypeKind::unsigned_integer ? "false" : "true")
                                               : field(values, "signed", argument),
                                           argument);
            type.element_type = field(values, "element", argument, false);
            type.element_count = static_cast<std::uint32_t>(parse_integer(
                field(values, "count", argument, false).empty() ? "0" : field(values, "count", argument), argument));
            type.declaration = field(values, "declaration", argument, false);
            options.types.push_back(std::move(type));
        } else if (argument == "--type-field" || argument == "--bitfield" || argument == "--enum-value") {
            const auto values = assignments(require_value(argument), argument);
            const std::string type_name = field(values, "type", argument);
            auto iterator = std::find_if(options.types.begin(), options.types.end(),
                                         [&](const TypeDescription& type) { return type.name == type_name; });
            if (iterator == options.types.end()) {
                throw std::invalid_argument("metadata field references an unknown --type: " + type_name);
            }
            if (argument == "--type-field") {
                iterator->fields.push_back(TypeFieldDescription{
                    field(values, "name", argument), field(values, "value", argument),
                    static_cast<std::uint32_t>(parse_integer(field(values, "offset", argument), argument))});
            } else if (argument == "--bitfield") {
                iterator->bitfields.push_back(TypeBitFieldDescription{
                    field(values, "name", argument), field(values, "value", argument),
                    static_cast<std::uint32_t>(parse_integer(field(values, "bits", argument), argument)),
                    static_cast<std::uint32_t>(parse_integer(
                        field(values, "group", argument, false).empty() ? "0" : field(values, "group", argument),
                        argument))});
            } else {
                iterator->enum_values.push_back(TypeEnumValueDescription{
                    field(values, "name", argument), parse_signed_integer(field(values, "value", argument), argument)});
            }
        } else if (argument == "--type-declaration") {
            const auto values = assignments(require_value(argument), argument);
            const std::string type_name = field(values, "name", argument);
            auto iterator = std::find_if(options.types.begin(), options.types.end(),
                                         [&](const TypeDescription& type) { return type.name == type_name; });
            if (iterator == options.types.end()) {
                throw std::invalid_argument("--type-declaration references an unknown --type: " + type_name);
            }
            iterator->declaration = field(values, "value", argument);
        } else if (argument == "--prototype") {
            const auto values = assignments(require_value(argument), argument);
            const std::uint64_t address = parse_integer(field(values, "address", argument), argument);
            auto iterator = std::find_if(options.prototypes.begin(), options.prototypes.end(),
                                         [address](const auto& item) { return item.first == address; });
            if (iterator == options.prototypes.end()) {
                iterator = options.prototypes.emplace(options.prototypes.end(), address, PrototypeDescription{});
            }
            iterator->second.calling_convention = field(values, "cc", argument, false).empty()
                                                      ? iterator->second.calling_convention
                                                      : field(values, "cc", argument);
            iterator->second.return_type = field(values, "return", argument, false).empty()
                                               ? iterator->second.return_type
                                               : field(values, "return", argument);
            const std::string return_storage = field(values, "return-storage", argument, false);
            if (!return_storage.empty()) {
                iterator->second.return_storage = parse_storage(return_storage, argument);
            }
            iterator->second.no_return = parse_bool(
                field(values, "no-return", argument, false).empty() ? "false" : field(values, "no-return", argument),
                argument);
            iterator->second.inline_function = parse_bool(
                field(values, "inline", argument, false).empty() ? "false" : field(values, "inline", argument),
                argument);
        } else if (argument == "--param") {
            const auto values = assignments(require_value(argument), argument);
            const std::uint64_t address = parse_integer(field(values, "address", argument), argument);
            auto iterator = std::find_if(options.prototypes.begin(), options.prototypes.end(),
                                         [address](const auto& item) { return item.first == address; });
            if (iterator == options.prototypes.end()) {
                iterator = options.prototypes.emplace(options.prototypes.end(), address, PrototypeDescription{});
            }
            iterator->second.parameters.push_back(
                PrototypeParameterDescription{field(values, "name", argument), field(values, "type", argument),
                                              parse_storage(field(values, "storage", argument), argument)});
        } else if (argument == "--variable") {
            const auto values = assignments(require_value(argument), argument);
            const std::uint64_t address = parse_integer(field(values, "address", argument), argument);
            auto iterator = std::find_if(options.variables.begin(), options.variables.end(),
                                         [address](const auto& item) { return item.first == address; });
            if (iterator == options.variables.end()) {
                iterator =
                    options.variables.emplace(options.variables.end(), address, std::vector<VariableDescription>{});
            }
            iterator->second.push_back(VariableDescription{
                field(values, "name", argument), field(values, "type", argument),
                parse_storage(field(values, "storage", argument), argument),
                parse_integer(field(values, "identity", argument, false).empty() ? "0"
                                                                                 : field(values, "identity", argument),
                              argument),
                parse_bool(field(values, "isolated", argument, false).empty() ? "false"
                                                                              : field(values, "isolated", argument),
                           argument)});
        } else if (argument == "--function") {
            const auto values = assignments(require_value(argument), argument);
            options.functions.push_back(FunctionDescription{field(values, "name", argument),
                                                            parse_integer(field(values, "address", argument), argument),
                                                            parse_integer(field(values, "end", argument), argument)});
        } else if (argument == "--jump-table") {
            const auto values = assignments(require_value(argument), argument);
            const std::uint64_t function_address = parse_integer(field(values, "function", argument), argument);
            auto iterator =
                std::find_if(options.flows.begin(), options.flows.end(),
                             [function_address](const auto& item) { return item.first == function_address; });
            if (iterator == options.flows.end()) {
                iterator = options.flows.emplace(options.flows.end(), function_address, FlowDescription{});
            }
            JumpTableDescription table;
            table.branch_address = parse_integer(field(values, "branch", argument), argument);
            table.starting_value = parse_integer(
                field(values, "start", argument, false).empty() ? "0" : field(values, "start", argument), argument);
            const std::string targets = field(values, "targets", argument);
            std::size_t begin = 0;
            while (begin < targets.size()) {
                const std::size_t end = targets.find('|', begin);
                table.target_addresses.push_back(
                    parse_integer(targets.substr(begin, end == std::string::npos ? end : end - begin), argument));
                if (end == std::string::npos) {
                    break;
                }
                begin = end + 1;
            }
            iterator->second.jump_tables.push_back(std::move(table));
        } else if (argument == "--readonly") {
            options.readonly_propagate = true;
        } else if (argument == "--volatile") {
            const auto values = assignments(require_value(argument), argument);
            options.volatile_ranges.push_back(MemoryRangeDescription{
                field(values, "space", argument, false).empty() ? "ram" : field(values, "space", argument),
                parse_integer(field(values, "address", argument), argument),
                parse_integer(field(values, "size", argument), argument)});
        } else {
            throw std::invalid_argument(std::format("unknown argument '{}'", argument));
        }
    }
    return options;
}

/// Prints the complete command syntax and the provider metadata grammar.
void print_usage(std::ostream& output) {
    output << R"(Usage:
  new_ghidra_decompiler --sla <name-or-path> --hex <bytes> [options]

Input:
  --sla <name-or-path>         SLA filename or explicit relative/absolute path, required in direct mode
  --hex <bytes>                Code bytes; spaces, commas, and newlines are accepted
  --hex-file <path>            Read code bytes from a text file
  --data <address:hex>         Add a mapped data chunk; repeatable
  --address <value>            Function entry address, decimal or 0x hexadecimal
  --size <bytes>               Function body size (alternative to --end)
  --end <address>              Exclusive function end address
  --name <name>                Function name
  --context <name=value>       Sleigh context override; repeatable

Output:
  --show <list>                all,summary,assembly,raw-pcode,high-pcode,data-flow,
                               control-flow,ast,c (comma-separated)
  --only <list>                Alias for --show

Architecture and metadata (repeatable where noted):
  --arch-name <name>           Architecture label
  --space name=...,address-size=N[,word-size=N,big-endian=BOOL,index=N,physical=BOOL,overlay=NAME]
  --register name=...,storage=space:offset:size
  --code-space <name>          Default code address space
  --data-space <name>          Default data address space
  --stack-register <name>      Stack pointer register
  --pointer-size <bytes>       Pointer width
  --symbol address=...,name=...[,kind=function|data,size=N,type=...,namespace=...,readonly=BOOL,space=...]
  --type name=...,kind=...,size=N[,signed=BOOL,element=...,count=N,declaration=...]
  --type-field type=...,name=...,value=...,offset=N
  --bitfield type=...,name=...,value=...,bits=N[,group=N]
  --enum-value type=...,name=...,value=N
  --prototype address=...[,cc=...,return=...,return-storage=space:offset:size,no-return=BOOL,inline=BOOL]
  --param address=...,name=...,type=...,storage=space:offset:size
  --variable address=...,name=...,type=...,storage=space:offset:size[,identity=N,isolated=BOOL]
  --function name=...,address=...,end=...  Additional bounded child function; repeatable
  --jump-table function=...,branch=...,targets=A|B|C[,start=N]
  --volatile space=...,address=...,size=N
  --readonly                    Propagate immutable mapped memory constants

  --help                       Show this help
)" << std::flush;
}

/// Implements the provider contracts used by the standalone command line.
class CliProviders final {
public:
    /// Creates immutable provider objects from parsed command-line records.
    explicit CliProviders(const Options& options) {
        if (!options.symbols.empty()) {
            symbols_ = std::make_shared<Symbols>(options.symbols);
        }
        if (!options.types.empty()) {
            types_ = std::make_shared<Types>(options.types);
        }
        if (!options.prototypes.empty()) {
            prototypes_ = std::make_shared<Prototypes>(options.prototypes);
        }
        if (!options.variables.empty()) {
            variables_ = std::make_shared<Variables>(options.variables);
        }
        if (!options.functions.empty()) {
            functions_ = std::make_shared<Functions>(options.functions);
        }
        if (!options.flows.empty()) {
            flow_ = std::make_shared<Flows>(options.flows);
        }
    }

    /// Returns the symbol provider, or null when no symbols were supplied.
    [[nodiscard]] std::shared_ptr<newghidra::decompiler::SymbolProvider> symbols() const {
        return symbols_;
    }

    /// Returns the type provider, or null when no types were supplied.
    [[nodiscard]] std::shared_ptr<newghidra::decompiler::TypeProvider> types() const {
        return types_;
    }

    /// Returns the prototype provider, or null when no prototypes were supplied.
    [[nodiscard]] std::shared_ptr<newghidra::decompiler::PrototypeProvider> prototypes() const {
        return prototypes_;
    }

    /// Returns the local-variable provider, or null when no locals were supplied.
    [[nodiscard]] std::shared_ptr<newghidra::decompiler::VariableProvider> variables() const {
        return variables_;
    }

    /// Returns bounded child-function metadata supplied by the command line.
    [[nodiscard]] std::shared_ptr<FunctionProvider> functions() const {
        return functions_;
    }

    /// Returns function-local flow corrections supplied by the command line.
    [[nodiscard]] std::shared_ptr<FlowProvider> flow() const {
        return flow_;
    }

private:
    /// Implements lookup for externally supplied symbols.
    class Symbols final : public newghidra::decompiler::SymbolProvider {
    public:
        /// Stores immutable symbols for one invocation.
        explicit Symbols(std::vector<SymbolDescription> values) : values_(std::move(values)) {}
        /// Returns the exact symbol at an address.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            const auto iterator = std::find_if(values_.begin(), values_.end(),
                                               [address](const auto& value) { return value.address == address; });
            return iterator == values_.end() ? std::nullopt : std::optional<SymbolDescription>(*iterator);
        }
        /// Returns every symbol for pre-analysis installation.
        [[nodiscard]] std::vector<SymbolDescription> symbols() const override {
            return values_;
        }

    private:
        std::vector<SymbolDescription> values_;
    };

    /// Implements named type lookup.
    class Types final : public newghidra::decompiler::TypeProvider {
    public:
        /// Stores immutable types for one invocation.
        explicit Types(std::vector<TypeDescription> values) : values_(std::move(values)) {}
        /// Returns a type declaration by provider name.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            const auto iterator =
                std::find_if(values_.begin(), values_.end(), [name](const auto& value) { return value.name == name; });
            return iterator == values_.end() ? std::nullopt : std::optional<TypeDescription>(*iterator);
        }

    private:
        std::vector<TypeDescription> values_;
    };

    /// Implements address-keyed function prototypes.
    class Prototypes final : public newghidra::decompiler::PrototypeProvider {
    public:
        /// Stores immutable prototypes for one invocation.
        explicit Prototypes(std::vector<std::pair<std::uint64_t, PrototypeDescription>> values)
            : values_(std::move(values)) {}
        /// Returns the prototype associated with a function entry.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            const auto iterator = std::find_if(values_.begin(), values_.end(),
                                               [address](const auto& value) { return value.first == address; });
            return iterator == values_.end() ? std::nullopt : std::optional<PrototypeDescription>(iterator->second);
        }

    private:
        std::vector<std::pair<std::uint64_t, PrototypeDescription>> values_;
    };

    /// Implements address-keyed local-variable lookup.
    class Variables final : public newghidra::decompiler::VariableProvider {
    public:
        /// Stores immutable local-variable sets for one invocation.
        explicit Variables(std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> values)
            : values_(std::move(values)) {}
        /// Returns locals associated with a function entry.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            const auto iterator = std::find_if(values_.begin(), values_.end(),
                                               [address](const auto& value) { return value.first == address; });
            return iterator == values_.end() ? std::vector<VariableDescription>{} : iterator->second;
        }

    private:
        std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> values_;
    };

    /// Implements the public child-function provider contract.
    class Functions final : public FunctionProvider {
    public:
        /// Stores immutable child-function ranges.
        explicit Functions(std::vector<FunctionDescription> values) : values_(std::move(values)) {}
        /// Returns every child function supplied by the invocation.
        [[nodiscard]] std::vector<FunctionDescription> functions() const override {
            return values_;
        }

    private:
        std::vector<FunctionDescription> values_;
    };

    /// Implements address-keyed flow and jump-table metadata.
    class Flows final : public FlowProvider {
    public:
        /// Stores immutable flow descriptions keyed by root function address.
        explicit Flows(std::vector<std::pair<std::uint64_t, FlowDescription>> values) : values_(std::move(values)) {}
        /// Returns the flow description for one function, when supplied.
        [[nodiscard]] std::optional<FlowDescription> flow_at(std::uint64_t address) const override {
            const auto iterator = std::find_if(values_.begin(), values_.end(),
                                               [address](const auto& item) { return item.first == address; });
            return iterator == values_.end() ? std::nullopt : std::optional<FlowDescription>(iterator->second);
        }

    private:
        std::vector<std::pair<std::uint64_t, FlowDescription>> values_;
    };

    std::shared_ptr<Symbols> symbols_ = nullptr;
    std::shared_ptr<Types> types_ = nullptr;
    std::shared_ptr<Prototypes> prototypes_ = nullptr;
    std::shared_ptr<Variables> variables_ = nullptr;
    std::shared_ptr<Functions> functions_ = nullptr;
    std::shared_ptr<Flows> flow_ = nullptr;
};

/// Builds a contiguous image from the code and optional mapped data chunks.
std::pair<std::uint64_t, std::vector<std::uint8_t>> make_image(const Options& options) {
    std::vector<std::pair<std::uint64_t, std::vector<std::uint8_t>>> chunks;
    chunks.emplace_back(options.address, parse_hex(options.hex));
    for (const auto& [address, text] : options.data_chunks) {
        chunks.emplace_back(address, parse_hex(text, "--data"));
    }
    const auto first =
        std::ranges::min_element(chunks, {}, &std::pair<std::uint64_t, std::vector<std::uint8_t>>::first);
    std::uint64_t end = first->first;
    for (const auto& [address, bytes] : chunks) {
        if (address > std::numeric_limits<std::uint64_t>::max() - bytes.size()) {
            throw std::invalid_argument("mapped image chunk overflows its address");
        }
        end = std::max(end, address + bytes.size());
    }
    std::vector<std::uint8_t> image(static_cast<std::size_t>(end - first->first), 0);
    for (const auto& [address, bytes] : chunks) {
        std::ranges::copy(bytes, image.begin() + static_cast<std::ptrdiff_t>(address - first->first));
    }
    return {first->first, std::move(image)};
}

/// Formats one provider storage location for a readable raw-p-code listing.
std::string format_storage(const Storage& storage) {
    return std::format("{}[0x{:x}:{}]", storage.space.name(), storage.offset, storage.size);
}

/// Formats one materialized provider p-code operation.
std::string format_operation(const PcodeOperation& operation) {
    std::ostringstream output;
    if (operation.output) {
        output << format_storage(*operation.output) << " = ";
    }
    output << sleigh_runtime::opcode_name(operation.opcode);
    for (const Storage& input : operation.inputs) {
        output << ' ' << format_storage(input);
    }
    if (operation.memory_space) {
        output << " {" << operation.memory_space->name() << '}';
    }
    return output.str();
}

/// Prints one labeled section with a consistent visual hierarchy.
void print_section(std::ostream& output, std::string_view title, std::string_view text) {
    output << "\n+======================================================================+\n"
           << std::format("| {:<68} |\n", title)
           << "+----------------------------------------------------------------------+\n";
    if (text.empty()) {
        output << "(empty)\n";
    } else {
        output << text;
        if (!text.ends_with('\n')) {
            output << '\n';
        }
    }
}

/// Prints the summary and every selected native decompiler artifact.
void print_result(std::ostream& output, const DecompilationResult& result, std::string_view name,
                  const std::vector<OutputSection>& sections, std::size_t ordinal = 0, std::size_t total = 1) {
    for (const OutputSection section : sections) {
        switch (section) {
            case OutputSection::summary: {
                std::ostringstream summary;
                summary << "Function : " << name << '\n'
                        << "Result   : artifact set " << ordinal + 1 << '/' << total << '\n'
                        << "Decoded  : " << result.raw_instructions.size() << " instruction(s)\n";
                for (const Instruction& instruction : result.raw_instructions) {
                    summary << "  0x" << std::hex << std::setw(16) << std::setfill('0') << instruction.address
                            << std::dec << "  +" << std::setw(2) << std::setfill(' ') << instruction.length << "  "
                            << instruction.mnemonic;
                    if (!instruction.assembly.empty()) {
                        summary << ' ' << instruction.assembly;
                    }
                    summary << '\n';
                }
                print_section(output, "DECOMPILATION SUMMARY", summary.str());
                break;
            }
            case OutputSection::assembly: {
                std::ostringstream assembly;
                for (const Instruction& instruction : result.raw_instructions) {
                    assembly << "0x" << std::hex << std::setw(16) << std::setfill('0') << instruction.address
                             << std::dec << ": " << std::left << std::setw(12) << std::setfill(' ')
                             << instruction.mnemonic << std::right;
                    if (!instruction.assembly.empty()) {
                        assembly << ' ' << instruction.assembly;
                    }
                    assembly << '\n';
                }
                print_section(output, "DISASSEMBLY", assembly.str());
                break;
            }
            case OutputSection::raw_pcode:
                print_section(output, "RAW P-CODE", result.raw_pcode);
                break;
            case OutputSection::high_pcode:
                print_section(output, "HIGH P-CODE / SSA", result.high_pcode);
                break;
            case OutputSection::data_flow:
                print_section(output, "DATA FLOW", result.data_flow);
                break;
            case OutputSection::control_flow:
                print_section(output, "CONTROL FLOW", result.control_flow);
                break;
            case OutputSection::ast:
                print_section(output, "C AST", result.ast);
                break;
            case OutputSection::c_source:
                print_section(output, "C SOURCE", result.c_source);
                break;
        }
    }
}

/// Runs one direct `.sla`-backed decompilation using the parsed providers.
int run_direct(const Options& options) {
    if (options.sla_path.empty() || options.hex.empty()) {
        throw std::invalid_argument("direct mode requires both --sla and --hex (or --hex-file)");
    }
    const auto [image_base, image] = make_image(options);
    const std::uint64_t function_end = options.end.value_or(options.address + parse_hex(options.hex).size());
    if (function_end <= options.address) {
        throw std::invalid_argument("function end must be greater than function address");
    }
    auto memory = std::make_shared<newghidra::decompiler::SparseMemory>(image_base, image, options.volatile_ranges);
    auto pcode =
        std::make_shared<newghidra::decompiler::SleighPcodeProvider>(options.sla_path, memory, options.context);
    CliProviders providers(options);
    ProviderContext context;
    context.pcode = std::move(pcode);
    context.memory = memory;
    context.symbols = providers.symbols();
    context.types = providers.types();
    context.prototypes = providers.prototypes();
    context.variables = providers.variables();
    context.functions = providers.functions();
    context.flow = providers.flow();
    context.analysis_options.readonly_propagate = options.readonly_propagate;
    Decompiler decompiler(options.architecture, std::move(context));
    const DecompilationResult result =
        decompiler.decompile(FunctionDescription{options.function_name, options.address, function_end});
    print_result(std::cout, result, options.function_name, options.output);
    return 0;
}

/// Dispatches direct mode and returns a shell-friendly exit code.
int run(int argc, char* argv[]) {
    Options options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) == "help") {
            print_usage(std::cout);
            return 0;
        }
        throw;
    }
    return run_direct(options);
}

} // namespace

/// Parses CLI arguments, runs the selected decompilation, and reports failures
/// without hiding the remediation details needed to correct an input record.
int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        print_usage(std::cerr);
        return 1;
    }
}
