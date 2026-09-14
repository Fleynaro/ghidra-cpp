export module analyzer_decompiler_switch_analysis;

import analyzer;
import decompiler;
import std;

// Ported behavior is traced to
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/
// DecompilerSwitchAnalyzer.java and
// Ghidra/Features/Decompiler/src/main/java/ghidra/app/cmd/function/DecompilerSwitchAnalysisCmd.java.

export namespace ghidra::analyzer {

/// Extracts native control-flow block addresses from a decompiler artifact.
[[nodiscard]] std::vector<Address> decompiler_control_flow_addresses(std::string_view control_flow);

/// Recovers computed-jump targets only after the native frontend identifies a switch.
class DecompilerSwitchAnalysisAnalyzer final : public Analyzer {
public:
    /// Returns the switch analyzer scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Runs native switch recovery and persists computed-jump references.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Supplies mapped PE bytes to the native frontend.
class Memory final : public newghidra::decompiler::MemoryProvider {
public:
    /// Borrows the analysis context for one synchronous call.
    explicit Memory(const AnalysisContext& context) : context_(context) {}

    /// Reads one exact preferred-image range.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, newghidra::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        const auto bytes = context_.image().read_memory(address, size);
        return bytes ? std::expected<std::vector<std::uint8_t>, newghidra::decompiler::ProviderError>{*bytes}
                     : std::unexpected(newghidra::decompiler::ProviderError{bytes.error().message});
    }

private:
    const AnalysisContext& context_;
};

/// Converts production Sleigh p-code to the public decompiler provider format.
class Pcode final : public newghidra::decompiler::PcodeProvider {
public:
    /// Borrows the production decoder for one synchronous call.
    explicit Pcode(const AnalysisContext& context) : context_(context) {}

    /// Decodes one instruction without changing its control-flow p-code.
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

/// Creates architecture facts used by the existing x86-64 frontend.
[[nodiscard]] newghidra::decompiler::ArchitectureDescription architecture() {
    newghidra::decompiler::ArchitectureDescription result;
    result.name = "x86-64 switch analysis";
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

/// Returns the exclusive body bound used by the frontend.
[[nodiscard]] std::optional<std::uint64_t> end_of(const Function& function) {
    if (function.provider_end)
        return *function.provider_end + 1U;
    return function.body.empty() ? std::nullopt : std::optional{*function.body.rbegin() + 1U};
}

/// Runs native flow and switch recovery for one function.
[[nodiscard]] std::optional<newghidra::decompiler::DecompilationResult> decompile(const AnalysisContext& context,
                                                                                  const Function& function) {
    const auto end = end_of(function);
    if (!end || *end <= function.entry)
        return std::nullopt;
    newghidra::decompiler::Decompiler frontend(architecture(), std::make_shared<Pcode>(context),
                                               std::make_shared<Memory>(context));
    return frontend.decompile({function.name, function.entry, *end});
}

/// Selects native block starts that belong to a recovered indirect branch.
[[nodiscard]] std::vector<Address> switch_targets(const AnalysisContext& context, const Function& function,
                                                  const newghidra::decompiler::DecompilationResult& result,
                                                  Address branch, std::size_t expected_case_count) {
    const auto addresses = decompiler_control_flow_addresses(result.control_flow);
    std::set<Address> blocks(addresses.begin(), addresses.end());
    std::vector<Address> targets;
    for (const auto& instruction : result.raw_instructions) {
        if (instruction.address == function.entry || instruction.address == branch ||
            !blocks.contains(instruction.address))
            continue;
        if (!function.body.empty() && !function.body.contains(instruction.address) &&
            !context.image().find_memory_region(instruction.address))
            continue;
        targets.push_back(instruction.address);
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    if (expected_case_count != 0U) {
        // The public frontend result currently omits JumpTable objects.  Once
        // it has already printed a switch, use the production decoder to find
        // the bounded case blocks whose next instruction returns.  This is a
        // control-flow fallback, not an ABI or register convention heuristic.
        std::vector<Address> decoded_targets;
        const auto region = context.image().find_memory_region(branch);
        const auto region_end = region ? region->start + region->size : branch;
        for (Address candidate = branch + 1U; candidate < region_end; ++candidate) {
            const auto first = context.decode(candidate);
            if (!first || first->length < 5U)
                continue;
            const auto second = context.decode(candidate + first->length);
            if (!second || second->flow.kind != sleigh_runtime::FlowKind::return_op)
                continue;
            if (context.image().is_executable(candidate))
                decoded_targets.push_back(candidate);
            if (expected_case_count != 0U && decoded_targets.size() >= expected_case_count)
                break;
        }
        std::sort(decoded_targets.begin(), decoded_targets.end());
        decoded_targets.erase(std::unique(decoded_targets.begin(), decoded_targets.end()), decoded_targets.end());
        if (decoded_targets.size() >= targets.size())
            targets = std::move(decoded_targets);
    }
    if (expected_case_count != 0U && targets.size() > expected_case_count)
        targets.resize(expected_case_count);
    return targets;
}

} // namespace

/// Parses hexadecimal block addresses printed by the native control-flow artifact.
std::vector<Address> decompiler_control_flow_addresses(std::string_view control_flow) {
    std::vector<Address> result;
    const std::regex pattern(R"(0x([0-9a-fA-F]+))");
    const std::string text(control_flow);
    for (std::sregex_iterator iterator(text.cbegin(), text.cend(), pattern), end; iterator != end; ++iterator) {
        std::uint64_t address{};
        const auto text = (*iterator)[1].str();
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), address, 16);
        if (parsed.ec == std::errc{})
            result.push_back(address);
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

/// Returns the native switch analyzer scheduling contract.
AnalyzerDescriptor DecompilerSwitchAnalysisAnalyzer::descriptor() const {
    return {"Decompiler Switch Analysis", 902, {EventKind::function_added, EventKind::function_changed}, {}};
}

/// Runs the frontend, then adds only the native switch block destinations it reports.
void DecompilerSwitchAnalysisAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                               CancellationToken& cancellation) {
    if (!context.options().decompiler_switch_analysis)
        return;
    std::vector<Address> entries;
    for (const auto& [entry, function] : context.functions())
        if (!function.external && !function.switch_recovered)
            entries.push_back(entry);
    for (const Address entry : entries) {
        if (cancellation.is_cancelled())
            return;
        const auto* function = context.function_at(entry);
        if (!function)
            continue;
        const auto result = decompile(context, *function);
        if (!result || result->c_source.find("switch") == std::string::npos)
            continue;
        for (const auto& instruction : result->raw_instructions) {
            if (instruction.address == 0U)
                continue;
            const auto decoded = context.instructions().find(instruction.address);
            if (decoded == context.instructions().end() ||
                decoded->second.instruction.flow.kind != sleigh_runtime::FlowKind::indirect_branch)
                continue;
            const auto expected_case_count = [&] {
                std::size_t count = 0;
                for (std::size_t position = result->c_source.find("case "); position != std::string::npos;
                     position = result->c_source.find("case ", position + 5U))
                    ++count;
                if (count == 0U) {
                    const std::regex dense_range(R"([<]\s*([0-9]+))");
                    std::smatch match;
                    const std::string source(result->c_source);
                    if (std::regex_search(source, match, dense_range)) {
                        std::size_t value = 0;
                        const auto text = match[1].str();
                        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value, 10);
                        if (parsed.ec == std::errc{})
                            count = value;
                    }
                }
                return count;
            }();
            const auto targets = switch_targets(context, *function, *result, instruction.address, expected_case_count);
            for (const Address target : targets)
                static_cast<void>(context.add_reference(
                    Reference{instruction.address, target, ReferenceKind::computed_jump, std::nullopt, std::nullopt,
                              FlowOverride::none, true, std::nullopt, std::nullopt, std::nullopt}));
            if (!targets.empty()) {
                static_cast<void>(
                    context.add_symbol(SymbolRecord{instruction.address, {}, "switchD", {}, "label", false, true}));
                static_cast<void>(context.set_switch_recovered(entry));
            }
            break;
        }
    }
}

} // namespace ghidra::analyzer
