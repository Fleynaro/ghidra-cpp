export module analyzer_function_start_search;

import analyzer;
import std;

/// Owns the six delayed Function Start Search phase declarations and implementations.
export namespace recode::analyzer {
class FunctionStartPreAnalyzer final : public Analyzer {
public:
    /// Returns the pre-function pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Searches executable boundaries before ordinary function creation.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

class FunctionStartAnalyzer final : public Analyzer {
public:
    /// Returns the byte-analysis pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Validates candidates, disassembles them, and creates functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

class FunctionStartFunctionAnalyzer final : public Analyzer {
public:
    /// Returns the function-constrained pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Re-evaluates retained candidates after function events.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

class FunctionStartPostAnalyzer final : public Analyzer {
public:
    /// Returns the post-code pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Rechecks unresolved candidates after code analysis.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

class FunctionStartDataPostAnalyzer final : public Analyzer {
public:
    /// Returns the post-data pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Rechecks unresolved candidates after data analysis.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Stores one masked byte from a Ghidra pattern rule.
struct MaskedByte {
    std::uint8_t value{};
    std::uint8_t mask{};
};

/// Stores a complete marked byte pattern and its Function Start action.
struct BytePattern {
    std::vector<MaskedByte> bytes;
    std::size_t mark_offset{};
    FunctionStartProperties properties;
};

/// Parses one hexadecimal or binary token with bit/nibble wildcards.
[[nodiscard]] std::vector<MaskedByte> parse_pattern_token(std::string token) {
    std::vector<MaskedByte> bytes;
    if (token.starts_with("0x") || token.starts_with("0X")) {
        token.erase(0, 2);
        if (std::any_of(token.begin(), token.end(), [](char value) {
                return value != '.' && std::isxdigit(static_cast<unsigned char>(value)) == 0;
            }))
            return {};
        if (token.size() % 2 != 0) {
            token.insert(token.begin(), '0');
        }
        for (std::size_t index = 0; index < token.size(); index += 2) {
            MaskedByte byte;
            const auto nibble = [](char value) -> std::pair<std::uint8_t, std::uint8_t> {
                if (value == '.') {
                    return {0, 0};
                }
                const auto digit =
                    std::isdigit(static_cast<unsigned char>(value))
                        ? static_cast<std::uint8_t>(value - '0')
                        : static_cast<std::uint8_t>(std::tolower(static_cast<unsigned char>(value)) - 'a' + 10);
                return {digit, 0xf};
            };
            const auto high = nibble(token[index]);
            const auto low = nibble(token[index + 1]);
            byte.value = static_cast<std::uint8_t>((high.first << 4U) | low.first);
            byte.mask = static_cast<std::uint8_t>((high.second << 4U) | low.second);
            bytes.push_back(byte);
        }
        return bytes;
    }
    if (token.empty() || token.size() % 8 != 0 || std::any_of(token.begin(), token.end(), [](char value) {
            return value != '0' && value != '1' && value != '.';
        })) {
        return {};
    }
    for (std::size_t index = 0; index < token.size(); index += 8) {
        MaskedByte byte;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            if (token[index + bit] != '.') {
                byte.mask |= static_cast<std::uint8_t>(1U << (7U - bit));
                if (token[index + bit] == '1') {
                    byte.value |= static_cast<std::uint8_t>(1U << (7U - bit));
                }
            }
        }
        bytes.push_back(byte);
    }
    return bytes;
}

/// Parses one `<data>` body and returns bytes plus the post-wildcard mark offset.
[[nodiscard]] std::pair<std::vector<MaskedByte>, std::size_t> parse_data_body(std::string_view body) {
    std::vector<MaskedByte> bytes;
    std::size_t mark = 0;
    std::istringstream tokens{std::string(body)};
    std::string token;
    while (tokens >> token) {
        if (token == "*") {
            mark = bytes.size();
            continue;
        }
        const auto parsed = parse_pattern_token(token);
        bytes.insert(bytes.end(), parsed.begin(), parsed.end());
    }
    return {std::move(bytes), mark};
}

/// Reads one XML attribute without imposing an XML-library dependency.
[[nodiscard]] std::optional<std::string> xml_attribute(std::string_view tag, std::string_view name) {
    const std::string key = std::string(name) + "=";
    const auto start = tag.find(key);
    if (start == std::string_view::npos || start + key.size() >= tag.size()) {
        return std::nullopt;
    }
    const char quote = tag[start + key.size()];
    if (quote != '\'' && quote != '"') {
        return std::nullopt;
    }
    const auto end = tag.find(quote, start + key.size() + 1);
    if (end == std::string_view::npos) {
        return std::nullopt;
    }
    return std::string(tag.substr(start + key.size() + 1, end - start - key.size() - 1));
}

/// Converts Ghidra's after attribute into the native action enum.
[[nodiscard]] FunctionStartAfter parse_after(std::string_view value) {
    if (value == "function")
        return FunctionStartAfter::function;
    if (value == "instruction")
        return FunctionStartAfter::instruction;
    if (value == "data")
        return FunctionStartAfter::data;
    if (value == "ptr")
        return FunctionStartAfter::pointer;
    if (value == "defined")
        return FunctionStartAfter::defined;
    return FunctionStartAfter::none;
}

/// Parses a funcstart or possiblefuncstart XML action and its constraints.
[[nodiscard]] FunctionStartProperties parse_action(std::string_view tag, std::size_t mark_offset,
                                                   std::optional<std::pair<std::int64_t, std::uint32_t>> alignment) {
    FunctionStartProperties properties;
    properties.pattern_mark_offset = mark_offset;
    if (tag.starts_with("<possiblefuncstart")) {
        properties.possible = true;
    }
    if (const auto value = xml_attribute(tag, "after")) {
        properties.after = parse_after(*value);
    }
    if (const auto value = xml_attribute(tag, "section"))
        properties.section = *value;
    if (const auto value = xml_attribute(tag, "label"))
        properties.label = *value;
    properties.thunk = xml_attribute(tag, "thunk").has_value();
    properties.no_return = xml_attribute(tag, "noreturn").has_value();
    if (const auto value = xml_attribute(tag, "validcode")) {
        if (*value == "function") {
            properties.valid_code.existing_function = true;
        } else if (*value == "true" || *value == "subroutine") {
            properties.valid_code.subroutine = true;
        } else if (const auto parsed = std::from_chars(value->data(), value->data() + value->size(),
                                                       properties.valid_code.minimum_instructions);
                   parsed.ec == std::errc{}) {
            properties.valid_code.contiguous = true;
        }
    }
    if (const auto value = xml_attribute(tag, "validcodemax")) {
        std::uint32_t maximum = 0;
        const auto parsed = std::from_chars(value->data(), value->data() + value->size(), maximum);
        if (parsed.ec == std::errc{})
            properties.valid_code.maximum_instructions = maximum;
    }
    if (const auto value = xml_attribute(tag, "contiguous"))
        properties.valid_code.contiguous = *value != "false";
    if (alignment) {
        properties.alignment_mark = alignment->first;
        properties.alignment_bits = alignment->second;
    }
    return properties;
}

/// Loads marked byte patterns and action attributes from Ghidra XML files.
[[nodiscard]] std::vector<BytePattern> load_patterns(const std::filesystem::path& root, bool pre_patterns) {
    std::vector<BytePattern> patterns;
    if (root.empty() || !std::filesystem::is_directory(root)) {
        return patterns;
    }
    for (const auto& file : std::filesystem::recursive_directory_iterator(root)) {
        if (!file.is_regular_file() || file.path().extension() != ".xml") {
            continue;
        }
        const auto filename = file.path().filename().string();
        const bool is_pre_pattern_file = filename.ends_with("_prepatterns.xml");
        if (is_pre_pattern_file != pre_patterns) {
            continue;
        }
        // The current PE context is x86-64. Ordinary corpora are selected by
        // the x86-64 Windows constraints; the shared x86 pre-corpus is also
        // the source selected for the x86-64 GCC pre-search constraints.
        if (!pre_patterns && filename.find("x86") != std::string::npos &&
            filename.find("x86-64win") == std::string::npos) {
            continue;
        }
        std::ifstream input(file.path());
        const std::string xml{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        std::vector<std::pair<std::size_t, std::size_t>> pair_ranges;
        std::size_t pair_cursor = 0;
        while ((pair_cursor = xml.find("<patternpairs", pair_cursor)) != std::string::npos) {
            const auto pair_end = xml.find("</patternpairs>", pair_cursor);
            if (pair_end == std::string::npos)
                break;
            pair_ranges.emplace_back(pair_cursor, pair_end + 15);
            const auto pre_begin = xml.find("<prepatterns>", pair_cursor);
            const auto pre_end = xml.find("</prepatterns>", pre_begin);
            const auto post_begin = xml.find("<postpatterns>", pair_cursor);
            const auto post_end = xml.find("</postpatterns>", post_begin);
            if (pre_begin != std::string::npos && pre_begin < pair_end && post_begin != std::string::npos &&
                post_begin < pair_end && pre_end != std::string::npos && post_end != std::string::npos) {
                std::vector<std::pair<std::vector<MaskedByte>, std::size_t>> prefixes;
                std::vector<std::pair<std::vector<MaskedByte>, std::size_t>> suffixes;
                for (std::size_t data = pre_begin; data < pre_end;) {
                    const auto begin = xml.find("<data", data);
                    if (begin == std::string::npos || begin >= pre_end)
                        break;
                    const auto start = xml.find('>', begin);
                    const auto close = xml.find("</data>", start);
                    if (start == std::string::npos || close == std::string::npos || close > pre_end)
                        break;
                    prefixes.push_back(parse_data_body(std::string_view(xml).substr(start + 1, close - start - 1)));
                    data = close + 7;
                }
                for (std::size_t data = post_begin; data < post_end;) {
                    const auto begin = xml.find("<data", data);
                    if (begin == std::string::npos || begin >= post_end)
                        break;
                    const auto start = xml.find('>', begin);
                    const auto close = xml.find("</data>", start);
                    if (start == std::string::npos || close == std::string::npos || close > post_end)
                        break;
                    suffixes.push_back(parse_data_body(std::string_view(xml).substr(start + 1, close - start - 1)));
                    data = close + 7;
                }
                const auto action = xml.find("<funcstart", post_end);
                const auto possible = xml.find("<possiblefuncstart", post_end);
                const auto action_begin =
                    action != std::string::npos && action < pair_end
                        ? action
                        : (possible != std::string::npos && possible < pair_end ? possible : std::string::npos);
                if (action_begin != std::string::npos) {
                    const auto action_end = xml.find('>', action_begin);
                    if (action_end != std::string::npos) {
                        for (const auto& prefix : prefixes) {
                            for (const auto& suffix : suffixes) {
                                if (prefix.first.empty() || suffix.first.empty())
                                    continue;
                                BytePattern pattern;
                                pattern.bytes = prefix.first;
                                pattern.mark_offset = pattern.bytes.size() + suffix.second;
                                pattern.bytes.insert(pattern.bytes.end(), suffix.first.begin(), suffix.first.end());
                                pattern.properties = parse_action(
                                    std::string_view(xml).substr(action_begin, action_end - action_begin + 1),
                                    pattern.mark_offset, std::nullopt);
                                patterns.push_back(std::move(pattern));
                            }
                        }
                    }
                }
            }
            pair_cursor = pair_end + 15;
        }
        std::size_t cursor = 0;
        while ((cursor = xml.find("<data", cursor)) != std::string::npos) {
            const auto data_start = xml.find('>', cursor);
            if (data_start == std::string::npos)
                break;
            const auto end = xml.find("</data>", cursor + 6);
            if (end == std::string::npos) {
                break;
            }
            if (std::any_of(pair_ranges.begin(), pair_ranges.end(),
                            [&](const auto& range) { return cursor >= range.first && cursor < range.second; })) {
                cursor = end + 7;
                continue;
            }
            BytePattern pattern;
            auto [bytes, mark] = parse_data_body(std::string_view(xml).substr(data_start + 1, end - data_start - 1));
            pattern.bytes = std::move(bytes);
            pattern.mark_offset = mark;
            const auto pattern_end = xml.find("</pattern>", end);
            const auto action_begin = xml.find("<funcstart", end);
            const auto possible_begin = xml.find("<possiblefuncstart", end);
            const auto action =
                action_begin != std::string::npos && action_begin < pattern_end
                    ? action_begin
                    : (possible_begin != std::string::npos && possible_begin < pattern_end ? possible_begin
                                                                                           : std::string::npos);
            if (action != std::string::npos) {
                const auto action_end = xml.find('>', action);
                std::optional<std::pair<std::int64_t, std::uint32_t>> alignment;
                const auto align_begin = xml.find("<align", end);
                if (align_begin != std::string::npos && align_begin < action &&
                    (pattern_end == std::string::npos || align_begin < pattern_end)) {
                    const auto align_end = xml.find('>', align_begin);
                    if (align_end != std::string::npos) {
                        const auto mark = xml_attribute(
                            std::string_view(xml).substr(align_begin, align_end - align_begin + 1), "mark");
                        const auto bits = xml_attribute(
                            std::string_view(xml).substr(align_begin, align_end - align_begin + 1), "bits");
                        if (mark && bits) {
                            std::int64_t mark_value = 0;
                            std::uint32_t bit_value = 0;
                            const auto mark_result =
                                std::from_chars(mark->data(), mark->data() + mark->size(), mark_value);
                            const auto bit_result =
                                std::from_chars(bits->data(), bits->data() + bits->size(), bit_value);
                            if (mark_result.ec == std::errc{} && bit_result.ec == std::errc{}) {
                                alignment = std::pair{mark_value, bit_value};
                            }
                        }
                    }
                }
                if (action_end != std::string::npos) {
                    pattern.properties = parse_action(std::string_view(xml).substr(action, action_end - action + 1),
                                                      pattern.mark_offset, alignment);
                }
            }
            if (!pattern.bytes.empty())
                patterns.push_back(std::move(pattern));
            cursor = end + 7;
        }
    }
    return patterns;
}

/// Resolves the processor pattern directory when callers did not override it.
/// The corpus is shipped with this feature, so production analysis never needs
/// an installed Ghidra tree or an environment variable at runtime. The source
/// location is used instead of a repository-root name so the module remains
/// relocatable.
[[nodiscard]] std::filesystem::path resolve_pattern_root(const AnalysisContext& context) {
    if (!context.options().pattern_root.empty()) {
        return context.options().pattern_root;
    }
    const auto bundled = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "patterns";
    if (std::filesystem::is_directory(bundled)) {
        return bundled;
    }
    std::vector<std::filesystem::path> candidates;
    candidates.emplace_back(std::filesystem::current_path() / "services" / "analyzers" / "function_start_search" /
                            "data" / "patterns");
    for (const auto& base : candidates) {
        if (std::filesystem::is_directory(base))
            return base;
    }
    return {};
}

/// Tests one masked byte pattern against mapped PE bytes.
[[nodiscard]] bool matches_pattern(const pe::LoadedPeImage& image, Address address, const BytePattern& pattern) {
    if (pattern.bytes.empty()) {
        return false;
    }
    const auto bytes = image.read_memory(address, pattern.bytes.size());
    if (!bytes) {
        return false;
    }
    for (std::size_t index = 0; index < pattern.bytes.size(); ++index) {
        if (((*bytes)[index] & pattern.bytes[index].mask) != (pattern.bytes[index].value & pattern.bytes[index].mask)) {
            return false;
        }
    }
    return true;
}

/// Returns whether bytes preceding a candidate are recognized pattern filler.
[[nodiscard]] bool has_pattern_boundary(const pe::LoadedPeImage& image, Address address) {
    if (const auto region = image.find_memory_region(address); region && address == region->start) {
        return true;
    }
    if (address == 0) {
        return false;
    }
    const auto previous = image.read_byte(address - 1);
    return previous &&
           (*previous == 0xcc || *previous == 0x90 || *previous == 0xc3 || *previous == 0xe9 || *previous == 0xeb);
}

/// Returns whether a candidate begins with a valid instruction pattern.
[[nodiscard]] bool matches_function_pattern(AnalysisContext& context, Address address) {
    if (!has_pattern_boundary(context.image(), address)) {
        return false;
    }
    const auto decoded = context.decode(address);
    if (!decoded || decoded->mnemonic.empty()) {
        return false;
    }
    return decoded->length != 0 && decoded->length <= 15;
}

/// Collects the selected pre- or ordinary-pattern corpus without reading raw
/// data as code. The split corresponds to Ghidra's two constraints files.
void collect_candidates(AnalysisContext& context, std::span<const AnalysisEvent> events,
                        CancellationToken& cancellation, bool pre_patterns) {
    std::set<Address> candidates;
    static_cast<void>(events);
    const auto patterns = load_patterns(resolve_pattern_root(context), pre_patterns);
    if (!patterns.empty()) {
        for (const auto& region : context.image().memory_regions()) {
            if (!region.executable) {
                continue;
            }
            if (!events.empty() && !std::any_of(events.begin(), events.end(), [&](const AnalysisEvent& event) {
                    return std::any_of(event.addresses.begin(), event.addresses.end(), [&](Address address) {
                        return address >= region.start && address < region.start + region.size;
                    });
                })) {
                continue;
            }
            for (std::uint64_t offset = 0; offset < region.size; ++offset) {
                if (cancellation.is_cancelled()) {
                    return;
                }
                const Address address = region.start + offset;
                for (std::size_t index = 0; index < patterns.size(); ++index) {
                    const auto mark = patterns[index].mark_offset;
                    if (!matches_pattern(context.image(), address, patterns[index])) {
                        continue;
                    }
                    const Address effective = address + mark;
                    if (effective < region.start || effective >= region.start + region.size) {
                        continue;
                    }
                    auto properties = patterns[index].properties;
                    properties.pattern_mark_offset = mark;
                    if (candidates.insert(effective).second) {
                        static_cast<void>(
                            context.mark_potential_function_start(effective, index, std::move(properties)));
                        break;
                    }
                }
            }
        }
    }
    std::size_t pattern_index = 0;
    for (const Address candidate : candidates) {
        if (!context.functions().contains(candidate) && context.executable_region(candidate)) {
            static_cast<void>(context.mark_potential_function_start(candidate, pattern_index++));
        }
    }
}

/// Tests whether a candidate's preceding program state satisfies `after`.
[[nodiscard]] bool satisfies_after(const AnalysisContext& context, Address address, FunctionStartAfter after) {
    if (after == FunctionStartAfter::none)
        return true;
    if (const auto region = context.image().find_memory_region(address); region && address == region->start) {
        return true;
    }
    if (address == 0)
        return false;
    const Address previous = address - 1;
    const auto ends_at = [&](Address start) {
        const auto instruction = context.instructions().find(start);
        if (instruction == context.instructions().end() || instruction->second.instruction.length == 0) {
            return false;
        }
        return previous >= start && previous < start + instruction->second.instruction.length;
    };
    const bool instruction = std::any_of(context.instructions().begin(), context.instructions().end(),
                                         [&](const auto& item) { return ends_at(item.first); });
    const bool function = std::any_of(context.functions().begin(), context.functions().end(), [&](const auto& item) {
        return item.second.body.contains(previous) ||
               std::any_of(item.second.instruction_starts.begin(), item.second.instruction_starts.end(),
                           [&](Address start) { return ends_at(start); });
    });
    const bool data = std::any_of(context.data().begin(), context.data().end(), [&](const auto& item) {
        return previous >= item.first && previous < item.first + item.second.size;
    });
    const auto incoming = std::count_if(context.references().begin(), context.references().end(),
                                        [&](const Reference& ref) { return ref.target == address; });
    const bool pointer =
        incoming != 0 &&
        std::all_of(context.references().begin(), context.references().end(),
                    [&](const Reference& ref) { return ref.target != address || ref.kind == ReferenceKind::data; });
    switch (after) {
        case FunctionStartAfter::function:
            return function;
        case FunctionStartAfter::instruction:
            return instruction;
        case FunctionStartAfter::data:
            return data;
        case FunctionStartAfter::pointer:
            return pointer;
        case FunctionStartAfter::defined:
            return instruction || data || pointer;
        default:
            return true;
    }
}

/// Tests section, alignment, valid-code, and delayed-function constraints.
[[nodiscard]] bool satisfies_properties(AnalysisContext& context, Address address,
                                        const FunctionStartProperties& properties) {
    if (!satisfies_after(context, address, properties.after))
        return false;
    if (properties.section) {
        const auto region = context.image().find_memory_region(address);
        if (!region)
            return false;
        std::string expression = *properties.section;
        bool ignore_case = expression.starts_with("(?i)");
        if (ignore_case)
            expression.erase(0, 4);
        try {
            auto flags = std::regex::ECMAScript;
            if (ignore_case)
                flags |= std::regex::icase;
            if (!std::regex_match(region->name, std::regex(expression, flags)))
                return false;
        } catch (const std::regex_error&) {
            return false;
        }
    }
    if (properties.alignment_bits != 0) {
        const auto mask = (std::uint64_t{1} << std::min(properties.alignment_bits, 63U)) - 1U;
        const Address raw = address - properties.pattern_mark_offset;
        if (((raw + static_cast<Address>(properties.alignment_mark)) & mask) != 0)
            return false;
    }
    // Function Start Search uses a pseudo-disassembler and must never turn a
    // byte-pattern match in the middle of an existing instruction into code.
    for (const auto& [instruction_start, record] : context.instructions()) {
        if (address > instruction_start && address - instruction_start < record.instruction.length)
            return false;
    }
    if (properties.valid_code.existing_function)
        return context.function_at(address) != nullptr;
    const bool has_valid_code_constraint = properties.valid_code.minimum_instructions != 0 ||
                                           properties.valid_code.maximum_instructions.has_value() ||
                                           properties.valid_code.subroutine;
    if (!has_valid_code_constraint)
        return true;
    if (!context.instructions().contains(address)) {
        static_cast<void>(context.disassemble_flow(address));
    }
    if (!context.instructions().contains(address))
        return false;
    std::uint32_t count = 0;
    bool terminal = false;
    std::deque<Address> work{address};
    std::set<Address> visited;
    const auto maximum = properties.valid_code.maximum_instructions.value_or(
        properties.valid_code.minimum_instructions == 0 ? 256U : properties.valid_code.minimum_instructions);
    while (!work.empty() && count < maximum) {
        const Address cursor = work.front();
        work.pop_front();
        if (!visited.insert(cursor).second)
            continue;
        const auto instruction = context.instructions().find(cursor);
        if (instruction == context.instructions().end())
            continue;
        ++count;
        terminal = instruction->second.instruction.flow.terminal ||
                   instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::return_op;
        if (terminal)
            break;
        if (properties.valid_code.contiguous &&
            instruction->second.instruction.flow.kind != sleigh_runtime::FlowKind::none &&
            !instruction->second.instruction.flow.has_fallthrough)
            continue;
        if (const auto next = cursor + instruction->second.instruction.length;
            instruction->second.instruction.flow.has_fallthrough ||
            instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::none) {
            if (context.can_disassemble(next))
                work.push_back(next);
        }
        if (!properties.valid_code.contiguous && instruction->second.instruction.flow.target) {
            const auto target = instruction->second.instruction.flow.target->offset;
            if (context.can_disassemble(target))
                work.push_back(target);
        }
    }
    if (count < properties.valid_code.minimum_instructions)
        return false;
    return !properties.valid_code.subroutine || terminal;
}

/// Creates a validated candidate and its Ghidra-compatible action effects.
void materialize_candidates(AnalysisContext& context, CancellationToken& cancellation, bool allow_possible = false,
                            bool only_existing_function = false) {
    for (const auto& [address, pattern_index] : context.potential_function_starts()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        const auto properties_it = context.potential_function_properties().find(address);
        const FunctionStartProperties properties = properties_it == context.potential_function_properties().end()
                                                       ? FunctionStartProperties{}
                                                       : properties_it->second;
        if (properties.possible && !allow_possible)
            continue;
        if (only_existing_function && !properties.valid_code.existing_function)
            continue;
        if (context.function_containing(address) && !context.function_at(address))
            continue;
        if (!satisfies_properties(context, address, properties))
            continue;
        if (properties.valid_code.existing_function) {
            if (properties.no_return && context.set_function_no_return(address, true)) {
                static_cast<void>(
                    context.add_bookmark(Bookmark{address, "Function Start Search", "Existing function action"}));
            }
            if (properties.thunk)
                static_cast<void>(context.set_function_thunk(address, true));
            continue;
        }
        static_cast<void>(context.disassemble_flow(address));
        if (!context.instructions().contains(address)) {
            continue;
        }
        if (context.create_function(address, properties.label.value_or(std::string{}))) {
            if (properties.no_return)
                static_cast<void>(context.set_function_no_return(address, true));
            if (properties.thunk)
                static_cast<void>(context.set_function_thunk(address, true));
            if (context.options().create_analysis_bookmarks) {
                static_cast<void>(context.add_bookmark(
                    Bookmark{address, "Function Start Search", "Match pattern " + std::to_string(pattern_index)}));
            }
        }
    }
}

} // namespace

/// Returns the pre-function pattern analyzer contract.
AnalyzerDescriptor FunctionStartPreAnalyzer::descriptor() const {
    return {"Function Start Pre Search", 199, {EventKind::memory_added}, {}};
}

/// Collects pre-function candidates while preserving delayed constraints.
void FunctionStartPreAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                       CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartPreFuncAnalyzer.java
    // Relevant method: added(). Candidate state mirrors the original property map.
    if (context.options().function_start_search) {
        collect_candidates(context, events, cancellation, true);
    }
}

/// Returns the ordinary Function Start Search analyzer contract.
AnalyzerDescriptor FunctionStartAnalyzer::descriptor() const {
    return {"Function Start Search", 402, {EventKind::memory_added}, {"Function Start Pre Search"}};
}

/// Validates delayed candidates, disassembles them, and creates function bodies.
void FunctionStartAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                    CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartAnalyzer.java
    // Relevant methods: added(), FunctionStartAction.applyActionToSet(), and checkPreRequisites().
    if (!context.options().function_start_search) {
        return;
    }
    if (context.potential_function_starts().empty()) {
        collect_candidates(context, events, cancellation, false);
    }
    materialize_candidates(context, cancellation);
}

/// Returns the function-constrained Function Start Search contract.
AnalyzerDescriptor FunctionStartFunctionAnalyzer::descriptor() const {
    return {"Function Start Search In Functions", 498, {EventKind::function_added}, {"Function Start Search"}};
}

/// Rechecks candidates after a function exists, matching delayed validFunction rules.
void FunctionStartFunctionAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                            CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartFuncAnalyzer.java
    // Relevant method: added().
    if (context.options().function_start_search) {
        // FunctionStartFuncAnalyzer intersects the pre-search property map
        // and only rechecks actions declared with validcode="function".
        materialize_candidates(context, cancellation, true, true);
    }
}

/// Returns the post-code Function Start Search contract.
AnalyzerDescriptor FunctionStartPostAnalyzer::descriptor() const {
    return {"Function Start Search After Code", 898, {EventKind::code_added}, {"Function Start Search"}};
}

/// Runs the post-code candidate pass when explicitly enabled.
void FunctionStartPostAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                        CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartPostAnalyzer.java
    // Relevant method: added().
    if (context.options().function_start_after_code) {
        materialize_candidates(context, cancellation, true);
    }
}

/// Returns the post-data Function Start Search contract.
AnalyzerDescriptor FunctionStartDataPostAnalyzer::descriptor() const {
    return {"Function Start Search After Data", 898, {EventKind::data_added}, {"Function Start Search"}};
}

/// Runs the post-data candidate pass when explicitly enabled.
void FunctionStartDataPostAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                            CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartDataPostAnalyzer.java
    // Relevant method: added().
    if (context.options().function_start_after_data) {
        materialize_candidates(context, cancellation, true);
    }
}

} // namespace recode::analyzer
