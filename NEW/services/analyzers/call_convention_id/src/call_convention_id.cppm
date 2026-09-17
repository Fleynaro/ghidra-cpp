export module analyzer_call_convention_id;

import analyzer;
import decompiler;
import sleigh_runtime;
import std;

// Ported behavior is traced to
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/
// DecompilerCallConventionAnalyzer.java and DecompilerParallelConventionAnalysisCmd.java.

export namespace recode::analyzer {

/// Extracts an explicit convention emitted by the native decompiler C printer.
[[nodiscard]] std::optional<std::string> identify_calling_convention(std::string_view c_source);

/// Identifies unknown function conventions by running the decompiler frontend.
class CallConventionIdAnalyzer final : public Analyzer {
public:
    /// Returns the decompiler convention analyzer scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Decompiles eligible functions and persists only explicit convention results.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Supplies PE bytes to the decompiler's immutable memory boundary.
class ContextMemory final : public recode::decompiler::MemoryProvider {
public:
    /// Borrows the analysis context for the synchronous decompilation call.
    explicit ContextMemory(const AnalysisContext& context) : context_(context) {}

    /// Reads exactly the requested preferred-image range.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, recode::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        const auto bytes = context_.image().read_memory(address, size);
        if (!bytes)
            return std::unexpected(recode::decompiler::ProviderError{bytes.error().message});
        return *bytes;
    }

private:
    const AnalysisContext& context_;
};

/// Converts one Sleigh instruction into the decompiler provider contract.
class ContextPcode final : public recode::decompiler::PcodeProvider {
public:
    /// Borrows the decoder for the synchronous decompilation call.
    explicit ContextPcode(const AnalysisContext& context) : context_(context) {}

    /// Decodes and materializes all p-code operations for one instruction.
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

/// Supplies the prototype already retained by the analyzer model.
class ContextPrototype final : public recode::decompiler::PrototypeProvider {
public:
    /// Borrows the function table for one synchronous frontend invocation.
    explicit ContextPrototype(const AnalysisContext& context) : context_(context) {}

    /// Converts the current native function signature without adding ABI guesses.
    [[nodiscard]] std::optional<recode::decompiler::PrototypeDescription>
    prototype_at(std::uint64_t address) const override {
        const auto* function = context_.function_at(address);
        if (!function)
            return std::nullopt;
        recode::decompiler::PrototypeDescription prototype;
        prototype.calling_convention = function->calling_convention.empty() ? "default" : function->calling_convention;
        prototype.return_type = function->return_type.empty() ? "void" : function->return_type;
        prototype.parameters.reserve(function->parameters.size());
        for (const auto& parameter : function->parameters)
            prototype.parameters.push_back({parameter.name, parameter.type, storage_from(parameter.storage), {}});
        return prototype;
    }

private:
    /// Parses only storage strings already supplied by the model.
    [[nodiscard]] static std::optional<recode::decompiler::Storage> storage_from(std::string_view value) {
        if (value.empty())
            return std::nullopt;
        const auto first = value.find(':');
        const auto second = value.find(':', first == std::string_view::npos ? first : first + 1U);
        if (first == std::string_view::npos || second == std::string_view::npos)
            return std::nullopt;
        const auto parse = [](std::string_view text) -> std::optional<std::uint64_t> {
            unsigned base = 10U;
            if (text.starts_with("0x") || text.starts_with("0X")) {
                text.remove_prefix(2U);
                base = 16U;
            }
            std::uint64_t parsed{};
            const auto* begin = text.data();
            const auto* end = begin + text.size();
            const auto result = std::from_chars(begin, end, parsed, base);
            return result.ec == std::errc{} && result.ptr == end ? std::optional{parsed} : std::nullopt;
        };
        const auto offset = parse(value.substr(first + 1U, second - first - 1U));
        const auto size = parse(value.substr(second + 1U));
        if (!offset || !size)
            return std::nullopt;
        return recode::decompiler::Storage{std::string(value.substr(0, first)), *offset,
                                           static_cast<std::uint32_t>(*size)};
    }

    const AnalysisContext& context_;
};

/// Supplies the current function name to the decompiler printer.
class ContextSymbols final : public recode::decompiler::SymbolProvider {
public:
    /// Borrows function names for one synchronous frontend invocation.
    explicit ContextSymbols(const AnalysisContext& context) : context_(context) {}

    /// Returns the function symbol at an exact address.
    [[nodiscard]] std::optional<recode::decompiler::SymbolDescription> symbol_at(std::uint64_t address) const override {
        const auto* function = context_.function_at(address);
        if (!function)
            return std::nullopt;
        return recode::decompiler::SymbolDescription{
            address, function->name, {}, recode::decompiler::SymbolKind::function, 0, {}, false};
    }

private:
    const AnalysisContext& context_;
};

/// Creates the x86-64 architecture facts required by the existing decompiler frontend.
[[nodiscard]] recode::decompiler::ArchitectureDescription architecture() {
    return recode::decompiler::make_x86_64_architecture();
}

/// Returns the exclusive native function end used by the frontend.
[[nodiscard]] std::optional<std::uint64_t> function_end(const Function& function) {
    if (function.provider_end)
        return *function.provider_end + 1U;
    if (function.body.empty())
        return std::nullopt;
    return *function.body.rbegin() + 1U;
}

/// Runs the same decompiler frontend used by the CLI against one native function.
[[nodiscard]] std::optional<recode::decompiler::DecompilationResult> decompile(const AnalysisContext& context,
                                                                               const Function& function) {
    const auto end = function_end(function);
    if (!end || *end <= function.entry)
        return std::nullopt;
    recode::decompiler::ProviderContext providers;
    providers.pcode = std::make_shared<ContextPcode>(context);
    providers.memory = std::make_shared<ContextMemory>(context);
    providers.prototypes = std::make_shared<ContextPrototype>(context);
    providers.symbols = std::make_shared<ContextSymbols>(context);
    recode::decompiler::Decompiler frontend(architecture(), std::move(providers));
    return frontend.decompile({function.name, function.entry, *end});
}

} // namespace

/// Finds an explicit convention token in generated C, never inferring from registers or names.
std::optional<std::string> identify_calling_convention(std::string_view c_source) {
    for (const std::string_view convention : {"__fastcall", "__stdcall", "__thiscall", "__vectorcall", "__cdecl"})
        if (c_source.find(convention) != std::string_view::npos)
            return std::string(convention);
    return std::nullopt;
}

/// Returns the decompiler convention analyzer scheduling contract.
AnalyzerDescriptor CallConventionIdAnalyzer::descriptor() const {
    return {"Call Convention ID", 900, {EventKind::function_added, EventKind::function_changed}, {}};
}

/// Decompiles uncommitted functions and stores only conventions emitted by the frontend.
void CallConventionIdAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                       CancellationToken& cancellation) {
    if (!context.options().call_convention_id)
        return;
    std::vector<Address> entries;
    for (const auto& [entry, function] : context.functions())
        if (!function.external && !function.signature_committed &&
            (function.calling_convention.empty() || function.calling_convention == "default"))
            entries.push_back(entry);
    for (const Address entry : entries) {
        if (cancellation.is_cancelled())
            return;
        const auto* function = context.function_at(entry);
        if (!function)
            continue;
        try {
            const auto result = decompile(context, *function);
            if (!result)
                continue;
            const auto convention = identify_calling_convention(result->c_source);
            if (convention && *convention != "default")
                static_cast<void>(context.set_function_signature(entry, *convention, function->return_type,
                                                                 function->parameters, function->variadic, true));
        } catch (...) {
            // One malformed or unsupported native function must not abort the
            // aggregate run. The Java analyzer skips a failed decompiler task;
            // preserve that isolation while allowing later functions to run.
            continue;
        }
    }
}

} // namespace recode::analyzer
