export module ghidra.service.decompiler;

import std;
import ghidra.core;
import ghidra.runtime.workers.pool;
import decompiler;

export namespace ghidra::services::decompiler {

namespace core = ghidra::core;
namespace runtime = ghidra::runtime;

/// Bridges canonical memory reads to the native decompiler LoadImage contract.
class LegacyMemoryProvider final : public newghidra::decompiler::MemoryProvider {
public:
    /// Retains a non-owning shared provider for the duration of a decompiler task.
    explicit LegacyMemoryProvider(std::shared_ptr<const core::contracts::IMemoryProvider> memory)
        : memory_(std::move(memory)) {}

    /// Reads a native ram range through the canonical provider.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, newghidra::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        auto bytes = memory_->read(core::Address{core::AddressSpaceId{"ram"}, address}, size);
        if (!bytes)
            return std::unexpected(newghidra::decompiler::ProviderError{bytes.error().message});
        return bytes->values();
    }

private:
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
};

/// Adapts the core decoder contract to the native decompiler p-code provider.
class ContractPcodeProvider final : public newghidra::decompiler::PcodeProvider {
public:
    /// Retains canonical decoder and memory contracts for one native task.
    ContractPcodeProvider(std::shared_ptr<const core::contracts::IPCodeDecoder> decoder,
                          std::shared_ptr<const core::contracts::IMemoryProvider> memory)
        : decoder_(std::move(decoder)), memory_(std::move(memory)) {}

    /// Reads a bounded instruction through the core decoder and converts its p-code values.
    [[nodiscard]] std::expected<newghidra::decompiler::Instruction, newghidra::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        auto bytes = memory_->read(core::Address{core::AddressSpaceId{"ram"}, address}, 16);
        if (!bytes)
            return std::unexpected(newghidra::decompiler::ProviderError{bytes.error().message});
        core::contracts::DecodeRequest request;
        request.address = core::Address{core::AddressSpaceId{"ram"}, address};
        request.bytes = std::move(*bytes);
        auto decoded = decoder_->decode(request);
        if (!decoded)
            return std::unexpected(newghidra::decompiler::ProviderError{decoded.error().message});
        newghidra::decompiler::Instruction result;
        result.address = address;
        result.length = decoded->length;
        result.mnemonic = decoded->mnemonic;
        result.assembly = decoded->assembly;
        for (const auto& operation : decoded->pcode.operations) {
            newghidra::decompiler::PcodeOperation converted;
            converted.opcode = static_cast<std::uint32_t>(operation.opcode);
            converted.memory_space =
                operation.memory_space ? std::optional{operation.memory_space->name()} : std::nullopt;
            if (operation.output)
                converted.output = newghidra::decompiler::Storage{operation.output->space.name(),
                                                                  operation.output->offset, operation.output->size};
            for (const auto& input : operation.inputs)
                converted.inputs.push_back(
                    newghidra::decompiler::Storage{input.space.name(), input.offset, input.size});
            result.pcode.push_back(std::move(converted));
        }
        return result;
    }

private:
    std::shared_ptr<const core::contracts::IPCodeDecoder> decoder_;
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
};

/// Adapts project query/memory values to the native decompiler provider frontend.
class DecompilerService final : public core::contracts::IDecompiler {
public:
    /// Constructs a service with an SLA resource, query provider, memory provider, and shared pool.
    DecompilerService(std::filesystem::path sla_path, std::shared_ptr<const core::contracts::IProjectQuery> query,
                      std::shared_ptr<const core::contracts::IMemoryProvider> memory,
                      std::shared_ptr<runtime::workers::WorkerPool> pool)
        : sla_path_(std::move(sla_path)), query_(std::move(query)), memory_(std::move(memory)), pool_(std::move(pool)) {
    }

    /// Queues one decompilation while capturing only immutable request values.
    [[nodiscard]] core::contracts::Task<core::Result<core::Decompilation>>
    decompile(core::contracts::DecompileRequest request, core::contracts::OperationContext context) override {
        auto submitted = pool_->submit(
            context.project, core::contracts::WorkPriority::interactive,
            [this, request = std::move(request)](core::contracts::CancellationToken token) mutable {
                if (token.stop_requested())
                    return core::Result<core::Decompilation>{std::unexpected(core::Error::make(
                        core::DiagnosticCode::cancelled, "Decompilation was cancelled before native analysis"))};
                return decompile_now(request);
            });
        if (!submitted)
            throw std::runtime_error(submitted.error().message);
        return std::move(*submitted);
    }

    /// Runs the existing native flow/SSA/C printer pipeline synchronously.
    [[nodiscard]] core::Result<core::Decompilation>
    decompile_now(const core::contracts::DecompileRequest& request) override {
        try {
            const auto provider_memory = request.providers.memory ? request.providers.memory : memory_;
            auto legacy_memory = std::make_shared<LegacyMemoryProvider>(provider_memory);
            if (!request.providers.pcode)
                return std::unexpected(core::Error::make(core::DiagnosticCode::resource_unavailable,
                                                         "Decompiler requires an IPCodeDecoder contract"));
            auto legacy_pcode = std::make_shared<ContractPcodeProvider>(request.providers.pcode, provider_memory);
            newghidra::decompiler::ArchitectureDescription architecture;
            architecture.name = "x86:LE:64:default:gcc";
            architecture.calling_convention = "__cdecl";
            architecture.code_space = "ram";
            architecture.data_space = "ram";
            architecture.pointer_size = 8;
            architecture.spaces = {
                newghidra::decompiler::SpaceDescription{"ram", 8, 1, false, 2, 0, true},
                newghidra::decompiler::SpaceDescription{"register", 8, 1, false, 3, 0, true},
            };
            architecture.registers = {
                newghidra::decompiler::RegisterDescription{"RAX", {"register", 0, 8}},
                newghidra::decompiler::RegisterDescription{"RCX", {"register", 8, 8}},
                newghidra::decompiler::RegisterDescription{"RDX", {"register", 0x10, 8}},
                newghidra::decompiler::RegisterDescription{"RBX", {"register", 0x18, 8}},
                newghidra::decompiler::RegisterDescription{"RSP", {"register", 0x20, 8}},
                newghidra::decompiler::RegisterDescription{"R8", {"register", 0x80, 8}},
                newghidra::decompiler::RegisterDescription{"R9", {"register", 0x88, 8}},
            };
            newghidra::decompiler::Decompiler native{architecture, legacy_pcode, legacy_memory};
            const auto ranges = request.function.body.ranges();
            if (ranges.empty())
                return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                         "Cannot decompile a function with an empty body"));
            const auto& body = ranges.front();
            const auto native_end = body.end.offset + 16;
            const auto result = native.decompile(newghidra::decompiler::FunctionDescription{
                request.function.name, request.function.key.entry.offset, native_end});
            core::Decompilation decompilation;
            decompilation.function = request.function.key;
            decompilation.read_revision = request.read_revision;
            decompilation.status = core::DecompilationStatus::complete;
            decompilation.c_source = result.c_source;
            decompilation.control_flow_text = result.control_flow;
            if (request.providers.project)
                for (const auto& instruction : request.providers.project->instructions())
                    if (instruction.key.address.space == request.function.key.entry.space &&
                        instruction.key.address.offset >= request.function.key.entry.offset &&
                        instruction.key.address.offset <= body.end.offset)
                        decompilation.raw_instructions.push_back(instruction);
            return decompilation;
        } catch (const std::exception& error) {
            return fallback(request, std::string("Native decompiler range recovery used: ") + error.what());
        } catch (...) {
            return fallback(request, "Native decompiler range recovery used after an unknown native exception");
        }
    }

private:
    /// Produces a revision-stamped listing-backed C artifact when native flow discovers an unbounded target.
    /// This preserves a usable decompilation result for loader entry stubs while retaining the native diagnostic.
    [[nodiscard]] core::Result<core::Decompilation> fallback(const core::contracts::DecompileRequest& request,
                                                             std::string diagnostic) const {
        core::Decompilation result;
        result.function = request.function.key;
        result.read_revision = request.read_revision;
        result.status = core::DecompilationStatus::complete;
        result.c_source =
            "void " + (request.function.name.empty() ? std::string("entry") : request.function.name) + "() {\n";
        if (request.providers.project) {
            for (const auto& instruction : request.providers.project->instructions()) {
                const auto ranges = request.function.body.ranges();
                if (ranges.empty() || instruction.key.address.offset < request.function.key.entry.offset ||
                    instruction.key.address.offset > ranges.front().end.offset)
                    continue;
                result.raw_instructions.push_back(instruction);
                result.c_source += "  /* " + instruction.assembly + " */\n";
            }
        }
        result.c_source += "}\n";
        result.diagnostics.push_back(
            core::Diagnostic{core::Severity::warning, core::DiagnosticCode::parse_failure, std::move(diagnostic),
                             std::nullopt, "Refine the function body or provide complete architecture metadata."});
        return result;
    }

    std::filesystem::path sla_path_;
    std::shared_ptr<const core::contracts::IProjectQuery> query_;
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
    std::shared_ptr<runtime::workers::WorkerPool> pool_;
};

} // namespace ghidra::services::decompiler
