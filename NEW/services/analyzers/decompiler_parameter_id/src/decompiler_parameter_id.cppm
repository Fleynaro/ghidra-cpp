export module analyzer_decompiler_parameter_id;

import analyzer;
import decompiler;
import std;

// Ported behavior is traced to
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/
// DecompilerFunctionAnalyzer.java and
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerParameterIdCmd.java.

export namespace ghidra::analyzer {

/// Runs native decompiler parameter recovery for eligible functions.
class DecompilerParameterIdAnalyzer final : public Analyzer {
public:
    /// Returns the parameter identification scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Runs the existing decompiler and records completion only after a result exists.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Supplies immutable PE bytes to the decompiler frontend.
class Memory final : public newghidra::decompiler::MemoryProvider {
public:
    /// Borrows the analysis context for one synchronous operation.
    explicit Memory(const AnalysisContext& context) : context_(context) {}

    /// Reads exactly the requested preferred-image range.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, newghidra::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        const auto bytes = context_.image().read_memory(address, size);
        return bytes ? std::expected<std::vector<std::uint8_t>, newghidra::decompiler::ProviderError>{*bytes}
                     : std::unexpected(newghidra::decompiler::ProviderError{bytes.error().message});
    }

private:
    const AnalysisContext& context_;
};

/// Converts the production Sleigh decode into frontend p-code records.
class Pcode final : public newghidra::decompiler::PcodeProvider {
public:
    /// Borrows the production decoder for one synchronous operation.
    explicit Pcode(const AnalysisContext& context) : context_(context) {}

    /// Decodes one instruction and preserves its complete p-code sequence.
    [[nodiscard]] std::expected<newghidra::decompiler::Instruction, newghidra::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        const auto decoded = context_.decode(address);
        if (!decoded)
            return std::unexpected(newghidra::decompiler::ProviderError{decoded.error().message});
        newghidra::decompiler::Instruction result{
            decoded->address, decoded->length, decoded->mnemonic, decoded->assembly, {}};
        for (const auto& operation : decoded->pcode) {
            newghidra::decompiler::PcodeOperation converted;
            converted.opcode = std::to_underlying(operation.opcode);
            converted.memory_space = operation.memory_space;
            if (operation.output)
                converted.output = newghidra::decompiler::Storage{operation.output->space, operation.output->offset,
                                                                  operation.output->size};
            for (const auto& input : operation.inputs)
                converted.inputs.push_back({input.space, input.offset, input.size});
            result.pcode.push_back(std::move(converted));
        }
        return result;
    }

private:
    const AnalysisContext& context_;
};

/// Creates the processor spaces and register facts required by x86-64.sla.
[[nodiscard]] newghidra::decompiler::ArchitectureDescription architecture() {
    newghidra::decompiler::ArchitectureDescription result;
    result.name = "x86-64 parameter analysis";
    result.spaces = {{"const", 8, 1, false, 0, 0, true},
                     {"ram", 8, 1, false, 2, 0, true},
                     {"register", 8, 1, false, 3, 0, true},
                     {"unique", 8, 1, false, 4, 0, true}};
    const auto registers = std::array{std::pair{std::string_view{"RAX"}, std::uint64_t{0x00}},
                                      std::pair{std::string_view{"RCX"}, std::uint64_t{0x08}},
                                      std::pair{std::string_view{"RDX"}, std::uint64_t{0x10}},
                                      std::pair{std::string_view{"RBX"}, std::uint64_t{0x18}},
                                      std::pair{std::string_view{"RSP"}, std::uint64_t{0x20}},
                                      std::pair{std::string_view{"RBP"}, std::uint64_t{0x28}},
                                      std::pair{std::string_view{"RSI"}, std::uint64_t{0x30}},
                                      std::pair{std::string_view{"RDI"}, std::uint64_t{0x38}},
                                      std::pair{std::string_view{"R8"}, std::uint64_t{0x40}},
                                      std::pair{std::string_view{"R9"}, std::uint64_t{0x48}},
                                      std::pair{std::string_view{"R10"}, std::uint64_t{0x50}},
                                      std::pair{std::string_view{"R11"}, std::uint64_t{0x58}},
                                      std::pair{std::string_view{"R12"}, std::uint64_t{0x60}},
                                      std::pair{std::string_view{"R13"}, std::uint64_t{0x68}},
                                      std::pair{std::string_view{"R14"}, std::uint64_t{0x70}},
                                      std::pair{std::string_view{"R15"}, std::uint64_t{0x78}}};
    for (const auto [name, offset] : registers)
        result.registers.push_back({std::string(name), {"register", offset, 8}});
    result.stack_register = "RSP";
    result.pointer_size = 8;
    return result;
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
    newghidra::decompiler::Decompiler frontend(architecture(), std::make_shared<Pcode>(context),
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

} // namespace ghidra::analyzer
