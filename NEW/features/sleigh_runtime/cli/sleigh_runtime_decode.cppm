#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

import sleigh_runtime;

namespace {

/// Stores command-line values required to decode one instruction.
struct Options {
    std::filesystem::path sla_path;
    std::string hex_bytes;
    std::uint64_t address = 0x140000000ULL;
    sleigh_runtime::ProcessorContext context{{{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}}};
};

/// Prints the command-line syntax and the supported processor context options.
void print_usage(std::ostream& output) {
    output << "Usage: sleigh_runtime_decode --sla <file.sla> --hex <bytes> [options]\n"
           << "\nOptions:\n"
           << "  --sla <path>          Compiled Sleigh specification\n"
           << "  --hex <bytes>         Machine code, for example: \"48 8b d9\"\n"
           << "  --address <value>     Instruction address (hex or decimal)\n"
           << "  --context <name=value> Override a Sleigh context field; repeatable\n"
           << "  --help                Show this help\n";
}

/// Parses an unsigned integer in hexadecimal or decimal notation.
std::uint64_t parse_integer(std::string_view text, std::string_view option_name) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        text.remove_prefix(2);
        base = 16;
    }
    if (text.empty()) {
        throw std::invalid_argument(std::format("{} requires a value", option_name));
    }
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument(std::format("invalid integer '{}' for {}", text, option_name));
    }
    return value;
}

/// Converts whitespace-separated machine-code text into bytes.
std::vector<std::uint8_t> parse_hex_bytes(std::string_view text) {
    std::string compact;
    for (const char character : text) {
        if (character != ' ' && character != '\t' && character != '\r' && character != '\n' && character != ',') {
            compact.push_back(character);
        }
    }
    if (compact.empty() || compact.size() % 2 != 0) {
        throw std::invalid_argument("--hex must contain one or more complete byte pairs");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(compact.size() / 2);
    for (std::size_t index = 0; index < compact.size(); index += 2) {
        const std::string_view pair = std::string_view(compact).substr(index, 2);
        std::uint32_t value = 0;
        const auto [end, error] = std::from_chars(pair.data(), pair.data() + pair.size(), value, 16);
        if (error != std::errc{} || end != pair.data() + pair.size()) {
            throw std::invalid_argument(std::format("invalid byte '{}' in --hex", pair));
        }
        bytes.push_back(static_cast<std::uint8_t>(value));
    }
    return bytes;
}

/// Parses command-line arguments and applies optional context overrides.
Options parse_options(int argc, char* argv[]) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        const auto require_value = [&](std::string_view option) -> std::string_view {
            if (index + 1 >= argc) {
                throw std::invalid_argument(std::format("{} requires a value", option));
            }
            return argv[++index];
        };

        if (argument == "--help" || argument == "-h") {
            print_usage(std::cout);
            std::exit(0);
        }
        if (argument == "--sla") {
            options.sla_path = require_value(argument);
        } else if (argument == "--hex") {
            options.hex_bytes = require_value(argument);
        } else if (argument == "--address") {
            options.address = parse_integer(require_value(argument), argument);
        } else if (argument == "--context") {
            const std::string_view assignment = require_value(argument);
            const std::size_t separator = assignment.find('=');
            if (separator == std::string_view::npos || separator == 0 || separator + 1 == assignment.size()) {
                throw std::invalid_argument("--context expects name=value");
            }
            options.context.values.emplace_back(std::string(assignment.substr(0, separator)),
                                                parse_integer(assignment.substr(separator + 1), "--context"));
        } else {
            throw std::invalid_argument(std::format("unknown argument '{}'", argument));
        }
    }
    if (options.sla_path.empty() || options.hex_bytes.empty()) {
        throw std::invalid_argument("both --sla and --hex are required");
    }
    return options;
}

/// Formats a concrete p-code varnode for stable console output.
std::string format_varnode(const sleigh_runtime::Varnode& varnode) {
    return std::format("{}[0x{:x}:{}]", varnode.space, varnode.offset, varnode.size);
}

/// Formats one p-code operation as an assignment-like textual expression.
std::string format_pcode(const sleigh_runtime::PcodeOp& operation) {
    std::ostringstream output;
    if (operation.output.has_value()) {
        output << format_varnode(*operation.output) << " = ";
    }
    output << sleigh_runtime::opcode_name(operation.opcode);
    for (const auto& input : operation.inputs) {
        output << ' ' << format_varnode(input);
    }
    return output.str();
}

/// Returns the stable textual name of a control-flow classification.
std::string_view flow_name(sleigh_runtime::FlowKind flow) {
    switch (flow) {
        case sleigh_runtime::FlowKind::none:
            return "none";
        case sleigh_runtime::FlowKind::branch:
            return "branch";
        case sleigh_runtime::FlowKind::conditional_branch:
            return "conditional branch";
        case sleigh_runtime::FlowKind::call:
            return "call";
        case sleigh_runtime::FlowKind::indirect_branch:
            return "indirect branch";
        case sleigh_runtime::FlowKind::indirect_call:
            return "indirect call";
        case sleigh_runtime::FlowKind::return_op:
            return "return";
    }
    return "unknown";
}

/// Prints one decoded instruction and every materialized p-code operation.
void print_instruction(std::size_t index, const sleigh_runtime::Instruction& instruction,
                       std::span<const std::uint8_t> bytes) {
    std::cout << "\n+--------------------------------------------------------------+\n"
              << "| SLEIGH RUNTIME DECODE                                       |\n"
              << std::format("| Instruction: #{:<45}|\n", index + 1)
              << "+--------------------------------------------------------------+\n"
              << std::format("| Address : 0x{:016x}\n", instruction.address) << std::format("| Bytes   : ");
    for (const std::uint8_t byte : bytes) {
        std::cout << std::format("{:02x} ", byte);
    }
    std::cout << "\n"
              << std::format("| Length  : {} byte(s)\n", instruction.length)
              << std::format("| ASM     : {}{}\n", instruction.mnemonic,
                             instruction.assembly.empty() ? "" : " " + instruction.assembly)
              << std::format("| Flow    : {}\n", flow_name(instruction.flow.kind))
              << "+--------------------------------------------------------------+\n"
              << "| P-CODE                                                       |\n"
              << "+--------------------------------------------------------------+\n";
    for (std::size_t index = 0; index < instruction.pcode.size(); ++index) {
        std::cout << std::format("| {:>3}: {}\n", index, format_pcode(instruction.pcode[index]));
    }
    std::cout << "+--------------------------------------------------------------+\n";
}

/// Runs the decoder and returns a process exit code suitable for a console tool.
int run(int argc, char* argv[]) {
    const Options options = parse_options(argc, argv);
    const std::vector<std::uint8_t> input = parse_hex_bytes(options.hex_bytes);
    sleigh_runtime::Decoder decoder(options.sla_path);
    std::span<const std::uint8_t> remaining(input);
    std::uint64_t address = options.address;
    std::size_t instruction_index = 0;
    while (!remaining.empty()) {
        // The public runtime intentionally rejects buffers larger than the architectural 16-byte instruction window.
        const auto decode_window = remaining.first(std::min<std::size_t>(remaining.size(), 16));
        const auto result = decoder.decode(address, decode_window, options.context);
        if (!result.has_value()) {
            std::cerr << std::format("Decode error at instruction #{} (address 0x{:x}): {}\n", instruction_index + 1,
                                     address, result.error().message);
            return 2;
        }
        if (result->length == 0 || result->length > remaining.size()) {
            std::cerr << std::format("Decode error at instruction #{}: decoder returned invalid length {}\n",
                                     instruction_index + 1, result->length);
            return 2;
        }

        print_instruction(instruction_index, *result, remaining.first(result->length));
        remaining = remaining.subspan(result->length);
        address += result->length;
        ++instruction_index;
    }
    return 0;
}

} // namespace

/// Parses arguments, decodes one instruction, and reports user-facing errors.
int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        print_usage(std::cerr);
        return 1;
    }
}
