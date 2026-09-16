import std;
import function_id;
import sleigh_runtime;

namespace {

/// Stores command-line inputs for one autonomous FunctionID query.
struct Options {
    std::vector<std::string> database_patterns;
    std::filesystem::path sla_path{"x86-64.sla"};
    std::string hex_bytes;
    std::uint64_t address{0x140000000ULL};
    std::string language_id{"x86:LE:64:default"};
    std::optional<std::string> compiler_spec;
    float score_threshold{14.6F};
    sleigh_runtime::ProcessorContext context{
        {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}, {"bit64", 1}, {"protectedMode", 1}}};
};

/// Prints the supported FunctionID query syntax.
void print_usage(std::ostream& output) {
    output
        << "Usage: function_id_cli --hex <bytes> [--fidb <name|path|glob>] [options]\n\n"
        << "Options:\n"
        << "  --hex <bytes>         Function bytes, for example: \"48 8b d9 c3\"\n"
        << "  --fidb <name|path|glob> Database name, path, or glob; repeatable; empty selects all bundled databases\n"
        << "  --sla <path>          Compiled Sleigh file (default: x86-64 test SLA)\n"
        << "  --address <value>     Function address (hex or decimal)\n"
        << "  --language <id>       Language ID (default: x86:LE:64:default)\n"
        << "  --compiler <spec>     Compiler specification filter\n"
        << "  --threshold <value>   Match score threshold (default: 14.6)\n"
        << "  --context <name=value> Override a Sleigh context field; repeatable\n"
        << "  --help                Show this help\n";
}

/// Parses an unsigned integer in hexadecimal or decimal notation.
std::uint64_t parse_integer(std::string_view text, std::string_view option) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        text.remove_prefix(2);
        base = 16;
    }
    if (text.empty())
        throw std::invalid_argument(std::format("{} requires a value", option));
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument(std::format("invalid integer '{}' for {}", text, option));
    }
    return value;
}

/// Converts whitespace-, comma-, or colon-separated hexadecimal text into bytes.
std::vector<std::uint8_t> parse_hex_bytes(std::string_view text) {
    std::string compact;
    for (const char character : text) {
        if (std::isxdigit(static_cast<unsigned char>(character)))
            compact.push_back(character);
    }
    if (compact.empty() || compact.size() % 2 != 0) {
        throw std::invalid_argument("--hex must contain one or more complete byte pairs");
    }
    std::vector<std::uint8_t> bytes;
    bytes.reserve(compact.size() / 2);
    for (std::size_t index = 0; index < compact.size(); index += 2) {
        std::uint32_t value = 0;
        const auto pair = std::string_view(compact).substr(index, 2);
        const auto [end, error] = std::from_chars(pair.data(), pair.data() + 2, value, 16);
        if (error != std::errc{} || end != pair.data() + 2) {
            throw std::invalid_argument(std::format("invalid byte '{}' in --hex", pair));
        }
        bytes.push_back(static_cast<std::uint8_t>(value));
    }
    return bytes;
}

/// Parses command-line arguments and repeatable Sleigh context overrides.
Options parse_options(int argc, char* argv[]) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        const auto require_value = [&](std::string_view option) {
            if (index + 1 >= argc)
                throw std::invalid_argument(std::format("{} requires a value", option));
            return std::string_view(argv[++index]);
        };
        if (argument == "--help" || argument == "-h") {
            print_usage(std::cout);
            std::exit(0);
        } else if (argument == "--hex") {
            options.hex_bytes = require_value(argument);
        } else if (argument == "--fidb") {
            options.database_patterns.emplace_back(require_value(argument));
        } else if (argument.starts_with("--fidb=")) {
            options.database_patterns.emplace_back(argument.substr(std::string_view("--fidb=").size()));
        } else if (argument == "--sla") {
            options.sla_path = require_value(argument);
        } else if (argument == "--address") {
            options.address = parse_integer(require_value(argument), argument);
        } else if (argument == "--language") {
            options.language_id = require_value(argument);
        } else if (argument == "--compiler") {
            options.compiler_spec = std::string(require_value(argument));
        } else if (argument == "--threshold") {
            options.score_threshold = std::stof(std::string(require_value(argument)));
        } else if (argument == "--context") {
            const auto assignment = require_value(argument);
            const auto separator = assignment.find('=');
            if (separator == std::string_view::npos || separator == 0 || separator + 1 == assignment.size()) {
                throw std::invalid_argument("--context expects name=value");
            }
            options.context.values.emplace_back(std::string(assignment.substr(0, separator)),
                                                parse_integer(assignment.substr(separator + 1), argument));
        } else {
            throw std::invalid_argument(std::format("unknown argument '{}'", argument));
        }
    }
    if (options.hex_bytes.empty()) {
        throw std::invalid_argument("--hex is required");
    }
    return options;
}

/// Matches one path component against the shell-style '*' and '?' wildcard syntax.
bool wildcard_match(std::string_view pattern, std::string_view value) {
    std::size_t pattern_index = 0;
    std::size_t value_index = 0;
    std::size_t star = std::string_view::npos;
    std::size_t star_value = 0;
    while (value_index < value.size()) {
        if (pattern_index < pattern.size() &&
            (pattern[pattern_index] == '?' || pattern[pattern_index] == value[value_index])) {
            ++pattern_index;
            ++value_index;
        } else if (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
            star = pattern_index++;
            star_value = value_index;
        } else if (star != std::string_view::npos) {
            pattern_index = star + 1;
            value_index = ++star_value;
        } else {
            return false;
        }
    }
    while (pattern_index < pattern.size() && pattern[pattern_index] == '*')
        ++pattern_index;
    return pattern_index == pattern.size();
}

/// Returns the directory containing the FunctionID databases shipped with this feature.
std::filesystem::path database_directory() {
    const std::filesystem::path directory(FUNCTION_ID_DATABASE_DIRECTORY);
    if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory))
        return directory;
    throw std::runtime_error(std::format("FunctionID database directory does not exist: {}", directory.string()));
}

/// Resolves a simple database name while leaving paths and path-like globs untouched.
std::string resolve_database_pattern(std::string_view pattern_text) {
    if (pattern_text.empty())
        return (database_directory() / "*.fidb").string();

    const std::filesystem::path pattern(pattern_text);
    if (!pattern.parent_path().empty())
        return pattern.string();

    if (std::filesystem::exists(pattern))
        return pattern.string();

    auto name = pattern;
    if (name.extension() != ".fidb")
        name.replace_extension(".fidb");
    return (database_directory() / name).string();
}

/// Expands database names, direct paths, or single-directory wildcards into sorted database paths.
std::vector<std::filesystem::path> expand_databases(const std::vector<std::string>& patterns) {
    std::vector<std::filesystem::path> result;
    const auto requested_patterns = patterns.empty() ? std::vector<std::string>{""} : patterns;
    for (const auto& raw_pattern : requested_patterns) {
        const auto pattern_text = resolve_database_pattern(raw_pattern);
        const std::filesystem::path pattern(pattern_text);
        const auto filename = pattern.filename().string();
        const auto directory = pattern.parent_path().empty() ? std::filesystem::path(".") : pattern.parent_path();
        if (filename.find_first_of("*?") == std::string::npos) {
            result.push_back(pattern);
            continue;
        }
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            throw std::runtime_error(std::format("database glob directory does not exist: {}", directory.string()));
        }
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && wildcard_match(filename, entry.path().filename().string())) {
                result.push_back(entry.path());
            }
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    if (result.empty())
        throw std::runtime_error("the --fidb patterns matched no files");
    return result;
}

/// Decodes all instructions in one function extent through the existing Sleigh runtime.
std::vector<sleigh_runtime::Instruction> decode_function(sleigh_runtime::Decoder& decoder,
                                                         std::span<const std::uint8_t> bytes, std::uint64_t address,
                                                         const sleigh_runtime::ProcessorContext& context) {
    std::vector<sleigh_runtime::Instruction> instructions;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto window = bytes.subspan(offset, std::min<std::size_t>(16, bytes.size() - offset));
        const auto decoded = decoder.decode(address + offset, window, context);
        if (!decoded)
            throw std::runtime_error(std::format("decode failed at byte {}: {}", offset, decoded.error().message));
        if (decoded->length == 0 || decoded->length > bytes.size() - offset) {
            throw std::runtime_error("Sleigh returned an invalid instruction length");
        }
        instructions.push_back(*decoded);
        offset += decoded->length;
    }
    return instructions;
}

/// Resolves the default test SLA from either the NEW directory or its repository root.
std::filesystem::path resolve_sla_path(const std::filesystem::path& requested) {
    if (std::filesystem::exists(requested))
        return requested;
    const auto repository_relative = std::filesystem::path("NEW") / requested;
    if (std::filesystem::exists(repository_relative))
        return repository_relative;
    return requested;
}

/// Prints one structured FunctionID match.
void print_match(const std::filesystem::path& database, const fid::Match& match) {
    std::cout << std::format("{}: {}  library='{} {}'  score={:.2f} (function={:.2f}, child={:.2f}, parent={:.2f})\n",
                             database.string(), match.function.name, match.library.family_name, match.library.version,
                             match.overall_score(), match.function_score, match.child_score, match.parent_score);
}

/// Runs one raw-byte FunctionID query across all expanded databases.
int run(const Options& options) {
    const auto bytes = parse_hex_bytes(options.hex_bytes);
    sleigh_runtime::Decoder decoder(resolve_sla_path(options.sla_path));
    const auto instructions = decode_function(decoder, bytes, options.address, options.context);
    const auto hash = fid::Hasher::hash_sleigh(instructions);
    if (!hash)
        throw std::runtime_error(hash.error().message);
    std::cout << std::format("fullHash=0x{:016x} specificHash=0x{:016x} codeUnits={} specificUnits={}\n",
                             hash->full_hash, hash->specific_hash, hash->code_unit_size,
                             static_cast<int>(hash->specific_hash_additional_size));

    const auto databases = expand_databases(options.database_patterns);
    const fid::FunctionContext context{*hash, {}, {}};
    const fid::ProgramInfo program{options.language_id, options.compiler_spec, std::nullopt, false};
    bool found = false;
    for (const auto& database_path : databases) {
        auto database = fid::Database::open(database_path);
        if (!database) {
            std::cerr << std::format("{}: database error: {}\n", database_path.string(), database.error().message);
            continue;
        }
        const auto result = database->identify(context, program, options.score_threshold);
        if (!result) {
            std::cerr << std::format("{}: identification error: {}\n", database_path.string(), result.error().message);
            continue;
        }
        for (const auto& match : result->matches) {
            found = true;
            print_match(database_path, match);
        }
    }
    if (!found)
        std::cout << "No FunctionID matches.\n";
    return found ? 0 : 3;
}

} // namespace

/// Parses arguments, runs FunctionID, and prints actionable user-facing errors.
int main(int argc, char* argv[]) {
    try {
        return run(parse_options(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        print_usage(std::cerr);
        return 1;
    }
}
