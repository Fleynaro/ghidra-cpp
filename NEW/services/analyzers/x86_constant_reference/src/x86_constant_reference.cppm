export module analyzer_x86_constant_reference;

import analyzer;
import std;

// Original sources:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java
// Ghidra/Features/Base/ghidra_scripts/PropagateX86ConstantReferences.java

/// Creates x86 data references for LEA destinations recovered by constant propagation.
export namespace recode::analyzer {
class X86ConstantReferenceAnalyzer final : public Analyzer {
public:
    /// Returns the x86 constant-reference name and REFERENCE_ANALYSIS.before() priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Applies the LEA address, minimum-address, mapped-memory, and duplicate-reference rules.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Resolves an image VA or image-relative RVA through the existing PE loader.
[[nodiscard]] std::optional<Address> resolve_address(const AnalysisContext& context, std::uint64_t value) {
    if (context.image().find_memory_region(value)) {
        return value;
    }
    const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(value));
    if (translated && context.image().find_memory_region(*translated)) {
        return *translated;
    }
    return std::nullopt;
}

/// Parses an absolute hexadecimal address from a printed LEA operand.
[[nodiscard]] std::optional<std::uint64_t> printed_address(std::string_view text) {
    const auto marker = text.find("0x");
    if (marker == std::string_view::npos) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    const auto begin = text.data() + marker + 2;
    const auto end = begin + (text.size() - marker - 2);
    const auto parsed = std::from_chars(begin, end, value, 16);
    return parsed.ec == std::errc{} ? std::optional{value} : std::nullopt;
}

/// Returns the latest stable propagated value for a register output at an instruction.
[[nodiscard]] std::optional<std::uint64_t> propagated_value(const AnalysisContext& context, Address instruction,
                                                            const sleigh_runtime::Varnode& output) {
    for (auto iterator = context.constant_facts().rbegin(); iterator != context.constant_facts().rend(); ++iterator) {
        if (iterator->instruction <= instruction && iterator->location == output && iterator->path_stable) {
            return iterator->value;
        }
    }
    return std::nullopt;
}

/// Returns whether the supplied instruction is an x86 LEA.
[[nodiscard]] bool is_lea(const sleigh_runtime::Instruction& instruction) {
    std::string mnemonic = instruction.mnemonic;
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return mnemonic == "lea";
}

} // namespace

/// Returns the original x86 constant reference priority and event triggers.
AnalyzerDescriptor X86ConstantReferenceAnalyzer::descriptor() const {
    return {"x86 Constant Reference Analyzer",
            596,
            {EventKind::code_added, EventKind::constant_added, EventKind::function_added},
            {}};
}

/// Materializes DATA references from stable LEA constants without reimplementing the p-code propagator.
void X86ConstantReferenceAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                           CancellationToken& cancellation) {
    if (!context.options().x86_constant_reference || (context.image().coff_header().machine != pe::Machine::i386 &&
                                                      context.image().coff_header().machine != pe::Machine::amd64)) {
        return;
    }
    for (const auto& [address, record] : context.instructions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!is_lea(record.instruction) || record.instruction.operands.size() < 2U) {
            continue;
        }
        std::optional<std::uint64_t> value = record.instruction.operands[1].value;
        if (!value) {
            value = printed_address(record.instruction.operands[1].text);
        }
        for (const auto& operation : record.instruction.pcode) {
            if (operation.output && operation.output->space == "register") {
                if (const auto propagated = propagated_value(context, address, *operation.output)) {
                    value = propagated;
                }
            }
        }
        if (!value || *value <= 0x1000U) {
            continue;
        }
        const auto target = resolve_address(context, *value);
        if (!target) {
            continue;
        }
        static_cast<void>(context.add_reference(
            Reference{address, *target, ReferenceKind::data, 1U, std::nullopt, FlowOverride::none, true}));
    }
}

} // namespace recode::analyzer
