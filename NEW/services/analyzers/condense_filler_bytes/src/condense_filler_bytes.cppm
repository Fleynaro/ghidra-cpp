export module analyzer_condense_filler_bytes;

import analyzer;
import std;

/// Owns the Condense Filler Bytes analyzer declaration and implementation.
export namespace recode::analyzer {
class CondenseFillerBytesAnalyzer final : public Analyzer {
public:
    /// Returns the Condense Filler Bytes analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Converts eligible undefined filler runs following functions to alignment data.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Reports whether a byte belongs to an existing code or data listing unit.
[[nodiscard]] bool is_defined(const AnalysisContext& context, Address address) {
    for (const auto& [start, record] : context.instructions()) {
        if (address >= start && address < start + record.instruction.length) {
            return true;
        }
    }
    for (const auto& [start, data] : context.data()) {
        if (address >= start && address < start + data.size) {
            return true;
        }
    }
    return false;
}

/// Returns the first byte immediately after a function body when it is undefined.
[[nodiscard]] std::optional<std::pair<Address, std::uint8_t>> first_filler_byte(const AnalysisContext& context,
                                                                                const Function& function) {
    if (function.body_ranges.empty()) {
        return std::nullopt;
    }
    const auto last_range = std::max_element(function.body_ranges.begin(), function.body_ranges.end(),
                                             [](const auto& left, const auto& right) { return left.end < right.end; });
    const Address end = last_range->end;
    if (end == std::numeric_limits<Address>::max()) {
        return std::nullopt;
    }
    const Address address = end + 1U;
    if (is_defined(context, address)) {
        return std::nullopt;
    }
    const auto byte = context.image().read_byte(address);
    if (!byte) {
        return std::nullopt;
    }
    return std::pair{address, static_cast<std::uint8_t>(*byte)};
}

/// Counts an undefined run of one byte value after a function body.
[[nodiscard]] std::uint32_t filler_run_length(const AnalysisContext& context, Address address, std::uint8_t filler) {
    std::uint64_t length = 0;
    while (length < std::numeric_limits<std::uint32_t>::max()) {
        const Address current = address + length;
        if (current < address || is_defined(context, current)) {
            break;
        }
        const auto byte = context.image().read_byte(current);
        if (!byte || *byte != filler) {
            break;
        }
        ++length;
    }
    return static_cast<std::uint32_t>(length);
}

} // namespace

/// Returns the late byte-analysis priority and function/data event contract.
AnalyzerDescriptor CondenseFillerBytesAnalyzer::descriptor() const {
    return {
        "Condense Filler Bytes", 905, {EventKind::memory_added, EventKind::function_added, EventKind::data_added}, {}};
}

/// Determines the dominant filler byte and records alignment data over matching runs.
void CondenseFillerBytesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                          CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/CondenseFillerBytesAnalyzer.java
    // Relevant methods: determineFillerValue(), added(), countUndefineds(), and replaceFillerBytes().
    if (!context.options().condense_filler_bytes) {
        return;
    }
    std::map<std::uint8_t, std::size_t> observations;
    for (const auto& [entry, function] : context.functions()) {
        static_cast<void>(entry);
        if (cancellation.is_cancelled()) {
            return;
        }
        if (const auto sample = first_filler_byte(context, function)) {
            ++observations[sample->second];
        }
    }
    if (observations.empty()) {
        return;
    }

    std::uint8_t filler = context.options().filler_byte;
    if (context.options().filler_auto_detect) {
        const auto winner =
            std::max_element(observations.begin(), observations.end(),
                             [](const auto& left, const auto& right) { return left.second < right.second; });
        filler = winner->first;
    }
    const std::uint32_t minimum = std::max<std::uint32_t>(1U, context.options().filler_minimum_length);
    for (const auto& [entry, function] : context.functions()) {
        static_cast<void>(entry);
        if (cancellation.is_cancelled()) {
            return;
        }
        const auto sample = first_filler_byte(context, function);
        if (!sample || sample->second != filler) {
            continue;
        }
        const std::uint32_t length = filler_run_length(context, sample->first, filler);
        if (length < minimum) {
            continue;
        }
        // AlignmentDataType is represented as an alignment DataObject in the native listing model.
        static_cast<void>(context.add_data(DataObject{sample->first, length, "alignment", "", false, true, false}));
    }
}

} // namespace recode::analyzer
