export module analyzer_decompiler_parameter_id;

import analyzer;
import decompiler;
import std;

// Ported behavior is traced to
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/
// DecompilerFunctionAnalyzer.java and
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java.

export namespace recode::analyzer {

/// Runs native decompiler parameter recovery for eligible functions.
class DecompilerParameterIdAnalyzer final : public Analyzer {
public:
    /// Returns the parameter identification scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Runs the existing decompiler and records completion only after a result exists.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Supplies immutable PE bytes to the decompiler frontend.
class Memory final : public recode::decompiler::MemoryProvider {
public:
    /// Borrows the analysis context for one synchronous operation.
    explicit Memory(const AnalysisContext& context) : context_(context) {}

    /// Reads exactly the requested preferred-image range.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, recode::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        const auto bytes = context_.image().read_memory(address, size);
        return bytes ? std::expected<std::vector<std::uint8_t>, recode::decompiler::ProviderError>{*bytes}
                     : std::unexpected(recode::decompiler::ProviderError{bytes.error().message});
    }

private:
    const AnalysisContext& context_;
};

/// Converts the production Sleigh decode into frontend p-code records.
class Pcode final : public recode::decompiler::PcodeProvider {
public:
    /// Borrows the production decoder for one synchronous operation.
    explicit Pcode(const AnalysisContext& context) : context_(context) {}

    /// Decodes one instruction and preserves its complete p-code sequence.
    [[nodiscard]] std::expected<recode::decompiler::Instruction, recode::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        const auto decoded = context_.decode(address);
        if (!decoded)
            return std::unexpected(recode::decompiler::ProviderError{decoded.error().message});
        return *decoded;
    }

private:
    const AnalysisContext& context_;
};

/// Creates the processor spaces and register facts required by x86-64.sla.
[[nodiscard]] recode::decompiler::ArchitectureDescription architecture() {
    return recode::decompiler::make_x86_64_architecture();
}

/// Returns the bounded end required by the native flow engine.
[[nodiscard]] std::optional<std::uint64_t> end_of(const Function& function) {
    if (function.provider_end)
        return *function.provider_end + 1U;
    return function.body.empty() ? std::nullopt : std::optional{*function.body.rbegin() + 1U};
}

/// Performs one real native decompiler run and returns whether it produced a C artifact.
[[nodiscard]] bool has_decompilation(const AnalysisContext& context, const Function& function) {
    const auto end = end_of(function);
    if (!end || *end <= function.entry)
        return false;
    recode::decompiler::Decompiler frontend(architecture(), std::make_shared<Pcode>(context),
                                            std::make_shared<Memory>(context));
    const auto result = frontend.decompile({function.name, function.entry, *end});
    return !result.c_source.empty();
}

} // namespace

/// Returns the parameter identification scheduling contract.
AnalyzerDescriptor DecompilerParameterIdAnalyzer::descriptor() const {
    return {"Decompiler Parameter ID", 901, {EventKind::function_added, EventKind::function_changed}, {}};
}

/// Runs the native frontend and records completion without manufacturing parameters or storage.
void DecompilerParameterIdAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                            CancellationToken& cancellation) {
    if (!context.options().decompiler_parameter_id)
        return;
    std::vector<Address> entries;
    for (const auto& [entry, function] : context.functions())
        if (!function.external && !function.parameter_id_complete)
            entries.push_back(entry);
    for (const Address entry : entries) {
        if (cancellation.is_cancelled())
            return;
        const auto* function = context.function_at(entry);
        if (!function)
            continue;
        try {
            if (has_decompilation(context, *function))
                static_cast<void>(context.set_parameter_id_complete(entry));
        } catch (...) {
            // A single unsupported native body must not prevent parameter
            // identification for every other function in the aggregate run.
            continue;
        }
    }
}

} // namespace recode::analyzer
