export module analyzer_ascii_strings;

import analyzer;
import std;

// Original Ghidra sources:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/string/StringsAnalyzer.java
// Ghidra/Features/Base/src/main/java/ghidra/program/util/string/StringSearcher.java
// Ghidra/Features/Base/src/main/java/ghidra/util/ascii/AsciiCharSetRecognizer.java

/// Finds null-terminated ASCII strings in initialized, accessible PE memory.
export namespace recode::analyzer {
class AsciiStringsAnalyzer final : public Analyzer {
public:
    /// Returns the original analyzer name, late data-type priority, and event contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Scans initialized memory and creates accepted string data objects.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Returns whether a byte belongs to Ghidra's ASCII recognizer character set.
[[nodiscard]] bool is_ascii_character(std::uint8_t value) noexcept {
    return (value >= static_cast<std::uint8_t>(' ') && value <= static_cast<std::uint8_t>('~')) || value == 0x09U ||
           value == 0x0aU || value == 0x0dU;
}

/// Returns whether an address range intersects an already decoded instruction.
[[nodiscard]] bool overlaps_instruction(const AnalysisContext& context, Address start, Address end) {
    for (const auto& [instruction_start, record] : context.instructions()) {
        if (record.instruction.length == 0U || instruction_start > end ||
            instruction_start + record.instruction.length - 1U < start) {
            continue;
        }
        return true;
    }
    return false;
}

/// Returns whether an address range overlaps an already defined data object.
[[nodiscard]] bool overlaps_data(const AnalysisContext& context, Address start, Address end) {
    for (const auto& [address, object] : context.data()) {
        if (object.size != 0U && address <= end && start <= address + object.size - 1U) {
            return true;
        }
    }
    return false;
}

/// Returns whether the candidate contains at least one printable alpha-numeric character.
[[nodiscard]] bool has_text_character(std::span<const std::uint8_t> bytes) noexcept {
    return std::any_of(bytes.begin(), bytes.end(), [](std::uint8_t value) {
        return (value >= static_cast<std::uint8_t>('a') && value <= static_cast<std::uint8_t>('z')) ||
               (value >= static_cast<std::uint8_t>('A') && value <= static_cast<std::uint8_t>('Z')) ||
               (value >= static_cast<std::uint8_t>('0') && value <= static_cast<std::uint8_t>('9'));
    });
}

/// Rejects linker-section and symbol-table spellings that the Ghidra string
/// model scores as metadata rather than user-visible character strings.
[[nodiscard]] bool is_metadata_spelling(std::string_view value) noexcept {
    if (value.empty() || value.front() == '.' || value.front() == '@') {
        return true;
    }
    const bool has_space = std::any_of(value.begin(), value.end(), [](char character) {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n';
    });
    return !has_space && value.find('_') != std::string_view::npos;
}

/// Converts one accepted byte sequence into the native string record value.
[[nodiscard]] std::string string_value(std::span<const std::uint8_t> bytes) {
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

} // namespace

/// Returns the StringsAnalyzer priority: DATA_TYPE_PROPOGATION.after() five times is 905.
AnalyzerDescriptor AsciiStringsAnalyzer::descriptor() const {
    return {"ASCII Strings", 905, {EventKind::memory_added, EventKind::data_added}, {}};
}

/// Implements the StringSearcher null, alignment, minimum-length, and conflict gates.
void AsciiStringsAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                   CancellationToken& cancellation) {
    // The native repository has no public NGramUtils/StringModel API. The byte recognizer and
    // all listing gates are preserved; the absent statistical score is recorded in GHIDRA_PORT.md.
    if (!context.options().ascii_strings) {
        return;
    }
    const auto minimum = std::max<std::uint32_t>(4U, context.options().ascii_minimum_length);
    const auto alignment = context.options().ascii_alignment == 1U || context.options().ascii_alignment == 2U ||
                                   context.options().ascii_alignment == 4U
                               ? context.options().ascii_alignment
                               : 1U;
    for (const auto& region : context.image().memory_regions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!region.initialized || region.headers ||
            (context.options().ascii_search_accessible_memory &&
             (!region.readable && !region.writable && !region.executable))) {
            continue;
        }
        const auto bytes = context.image().read_memory(region.start, region.size);
        if (!bytes) {
            continue;
        }
        std::size_t offset = 0;
        while (offset < bytes->size()) {
            if (cancellation.is_cancelled()) {
                return;
            }
            if (bytes->at(offset) == 0U || !is_ascii_character(bytes->at(offset)) ||
                (region.start + offset) % alignment != 0) {
                ++offset;
                continue;
            }
            const auto candidate_start = offset;
            while (offset < bytes->size() && bytes->at(offset) != 0U && is_ascii_character(bytes->at(offset))) {
                ++offset;
            }
            const auto character_count = offset - candidate_start;
            const bool terminated = offset < bytes->size() && bytes->at(offset) == 0U;
            if (character_count < minimum || (context.options().ascii_require_null_termination && !terminated) ||
                !has_text_character(std::span<const std::uint8_t>(bytes->data() + candidate_start, character_count))) {
                continue;
            }
            const auto start = region.start + candidate_start;
            std::uint64_t size = static_cast<std::uint64_t>(character_count) + (terminated ? 1U : 0U);
            if (terminated && context.options().ascii_end_alignment > 1U) {
                const auto remainder = (start + size) % context.options().ascii_end_alignment;
                const auto padding = remainder == 0U ? 0U : context.options().ascii_end_alignment - remainder;
                std::uint64_t valid_padding = 0;
                while (valid_padding < padding && candidate_start + size + valid_padding < bytes->size() &&
                       bytes->at(candidate_start + size + valid_padding) == 0U) {
                    ++valid_padding;
                }
                size += valid_padding;
            }
            const auto end = start + size - 1U;
            if (overlaps_instruction(context, start, end)) {
                continue;
            }
            if (!context.options().ascii_allow_existing_substrings && overlaps_data(context, start, end)) {
                continue;
            }
            if (!context.options().ascii_allow_middle_references &&
                std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
                    return reference.target > start && reference.target <= end;
                })) {
                continue;
            }
            const auto value =
                string_value(std::span<const std::uint8_t>(bytes->data() + candidate_start, character_count));
            if (is_metadata_spelling(value)) {
                if (terminated) {
                    ++offset;
                }
                continue;
            }
            static_cast<void>(
                context.add_string(StringRecord{start, static_cast<std::uint32_t>(size), 1U, value, "string",
                                                terminated, alignment == 1U || start % alignment == 0U, false}));
            if (terminated) {
                ++offset;
            }
        }
    }
}

} // namespace recode::analyzer
