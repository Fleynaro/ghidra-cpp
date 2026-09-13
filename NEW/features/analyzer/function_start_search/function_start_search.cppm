module analyzer;

import std;

namespace ghidra::analyzer {
namespace {

/// Stores the concrete byte suffix of one Ghidra XML pattern rule.
struct BytePattern {
    std::vector<std::optional<std::uint8_t>> bytes;
};

/// Parses one hexadecimal pattern token while retaining wildcard nibbles.
[[nodiscard]] std::vector<std::optional<std::uint8_t>> parse_pattern_token(std::string token) {
    if (!token.starts_with("0x") && !token.starts_with("0X")) {
        return {};
    }
    token.erase(0, 2);
    if (token.size() % 2 != 0) {
        token.insert(token.begin(), '0');
    }
    std::vector<std::optional<std::uint8_t>> bytes;
    for (std::size_t index = 0; index < token.size(); index += 2) {
        const char high = token[index];
        const char low = token[index + 1];
        if (high == '.' || low == '.') {
            bytes.push_back(std::nullopt);
            continue;
        }
        std::uint32_t value = 0;
        const auto parsed = std::from_chars(token.data() + index, token.data() + index + 2, value, 16);
        if (parsed.ec != std::errc{}) {
            return {};
        }
        bytes.push_back(static_cast<std::uint8_t>(value));
    }
    return bytes;
}

/// Loads concrete post-wildcard byte suffixes from Ghidra pattern XML files.
[[nodiscard]] std::vector<BytePattern> load_patterns(const std::filesystem::path& root) {
    std::vector<BytePattern> patterns;
    if (root.empty() || !std::filesystem::is_directory(root)) {
        return patterns;
    }
    for (const auto& file : std::filesystem::recursive_directory_iterator(root)) {
        if (!file.is_regular_file() || file.path().extension() != ".xml") {
            continue;
        }
        std::ifstream input(file.path());
        const std::string xml{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        std::size_t cursor = 0;
        while ((cursor = xml.find("<data>", cursor)) != std::string::npos) {
            const auto end = xml.find("</data>", cursor + 6);
            if (end == std::string::npos) {
                break;
            }
            const auto wildcard = xml.find('*', cursor + 6);
            if (wildcard != std::string::npos && wildcard < end) {
                std::istringstream tokens(xml.substr(wildcard + 1, end - wildcard - 1));
                std::string token;
                while (tokens >> token) {
                    const auto parsed = parse_pattern_token(token);
                    if (!parsed.empty()) {
                        patterns.push_back(BytePattern{parsed});
                        break;
                    }
                }
            }
            cursor = end + 7;
        }
    }
    return patterns;
}

/// Tests one concrete pattern suffix against mapped PE bytes.
[[nodiscard]] bool matches_pattern(const pe::LoadedPeImage& image, Address address, const BytePattern& pattern) {
    if (pattern.bytes.empty()) {
        return false;
    }
    const auto bytes = image.read_memory(address, pattern.bytes.size());
    if (!bytes) {
        return false;
    }
    for (std::size_t index = 0; index < pattern.bytes.size(); ++index) {
        if (pattern.bytes[index] && *pattern.bytes[index] != (*bytes)[index]) {
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

/// Collects exported and filler-boundary candidates without reading raw data as code.
void collect_candidates(AnalysisContext& context, std::span<const AnalysisEvent> events,
                        CancellationToken& cancellation) {
    std::set<Address> candidates;
    for (const auto& event : events) {
        candidates.insert(event.addresses.begin(), event.addresses.end());
    }
    for (const auto& symbol : context.image().exported_symbols()) {
        if (!symbol.forwarded) {
            candidates.insert(symbol.address_va);
        }
    }
    const auto patterns = load_patterns(context.options().pattern_root);
    if (!patterns.empty()) {
        for (const auto& region : context.image().memory_regions()) {
            if (!region.executable) {
                continue;
            }
            for (std::uint64_t offset = 0; offset < region.size; ++offset) {
                if (cancellation.is_cancelled()) {
                    return;
                }
                const Address address = region.start + offset;
                for (std::size_t index = 0; index < patterns.size(); ++index) {
                    if (matches_pattern(context.image(), address, patterns[index])) {
                        candidates.insert(address);
                        context.mark_potential_function_start(address, index);
                        break;
                    }
                }
            }
        }
    }
    for (const auto& region : context.image().memory_regions()) {
        if (!region.executable || region.size == 0) {
            continue;
        }
        const auto first = region.start;
        if (matches_function_pattern(context, first)) {
            candidates.insert(first);
        }
        // Pattern search examines filler boundaries, not every byte as code.
        for (std::uint64_t offset = 1; offset < region.size; ++offset) {
            if (cancellation.is_cancelled()) {
                return;
            }
            const Address address = region.start + offset;
            const auto byte = context.image().read_byte(address - 1);
            if (byte && (*byte == 0xcc || *byte == 0x90 || *byte == 0xc3) &&
                matches_function_pattern(context, address)) {
                candidates.insert(address);
            }
        }
    }
    std::size_t pattern_index = 0;
    for (const Address candidate : candidates) {
        if (!context.functions().contains(candidate) && context.executable_region(candidate)) {
            context.mark_potential_function_start(candidate, pattern_index++);
        }
    }
}

/// Creates a validated candidate and its Ghidra-compatible analysis bookmark.
void materialize_candidates(AnalysisContext& context, CancellationToken& cancellation) {
    for (const auto& [address, pattern_index] : context.potential_function_starts()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (context.functions().contains(address)) {
            continue;
        }
        context.disassemble_flow(address);
        if (!context.instructions().contains(address)) {
            continue;
        }
        if (context.create_function(address)) {
            if (context.options().create_analysis_bookmarks) {
                context.add_bookmark(
                    Bookmark{address, "Function Start Search", "Match pattern " + std::to_string(pattern_index)});
            }
        }
    }
}

} // namespace

/// Returns the pre-function pattern analyzer contract.
AnalyzerDescriptor FunctionStartPreAnalyzer::descriptor() const {
    return {"Function Start Search Before Functions", 199, {EventKind::memory_added}, {}};
}

/// Collects pre-function candidates while preserving delayed constraints.
void FunctionStartPreAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                       CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/BytePatterns/src/main/java/ghidra/app/analyzers/FunctionStartPreFuncAnalyzer.java
    // Relevant method: added(). Candidate state mirrors the original property map.
    if (context.options().function_start_search) {
        collect_candidates(context, events, cancellation);
    }
}

/// Returns the ordinary Function Start Search analyzer contract.
AnalyzerDescriptor FunctionStartAnalyzer::descriptor() const {
    return {"Function Start Search",
            402,
            {EventKind::code_added, EventKind::memory_added},
            {"Function Start Search Before Functions"}};
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
        collect_candidates(context, events, cancellation);
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
        materialize_candidates(context, cancellation);
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
        materialize_candidates(context, cancellation);
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
        materialize_candidates(context, cancellation);
    }
}

} // namespace ghidra::analyzer
