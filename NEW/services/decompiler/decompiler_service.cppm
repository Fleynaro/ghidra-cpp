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
    LegacyMemoryProvider(std::shared_ptr<const core::contracts::IMemoryProvider> memory,
                         core::AddressSpaceId address_space)
        : memory_(std::move(memory)), address_space_(std::move(address_space)) {}

    /// Reads a native ram range through the canonical provider.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, newghidra::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        auto bytes = memory_->read(core::Address{address_space_, address}, size);
        if (!bytes)
            return std::unexpected(newghidra::decompiler::ProviderError{bytes.error().message});
        return bytes->values();
    }

    /// Promotes canonical volatile ranges into the native memory-provider representation.
    [[nodiscard]] std::vector<newghidra::decompiler::MemoryRangeDescription> volatile_ranges() const override {
        std::vector<newghidra::decompiler::MemoryRangeDescription> result;
        for (const auto& range : memory_->volatile_ranges()) {
            if (range.start.space != address_space_)
                continue;
            result.push_back(newghidra::decompiler::MemoryRangeDescription{range.start.space.name(), range.start.offset,
                                                                           range.end.offset - range.start.offset + 1U});
        }
        return result;
    }

private:
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
    core::AddressSpaceId address_space_;
};

/// Adapts the core decoder contract to the native decompiler p-code provider.
class ContractPcodeProvider final : public newghidra::decompiler::PcodeProvider {
public:
    /// Retains canonical decoder and memory contracts for one native task.
    ContractPcodeProvider(std::shared_ptr<const core::contracts::IPCodeDecoder> decoder,
                          std::shared_ptr<const core::contracts::IMemoryProvider> memory,
                          core::AddressSpaceId address_space)
        : decoder_(std::move(decoder)), memory_(std::move(memory)), address_space_(std::move(address_space)) {}

    /// Reads a bounded instruction through the core decoder and converts its p-code values.
    [[nodiscard]] std::expected<newghidra::decompiler::Instruction, newghidra::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        auto bytes = memory_->read(core::Address{address_space_, address}, 16);
        if (!bytes)
            return std::unexpected(newghidra::decompiler::ProviderError{bytes.error().message});
        core::contracts::DecodeRequest request;
        request.address = core::Address{address_space_, address};
        request.bytes = std::move(*bytes);
        auto decoded = decoder_->decode(request);
        if (!decoded)
            return std::unexpected(newghidra::decompiler::ProviderError{decoded.error().message});
        return *decoded;
    }

private:
    std::shared_ptr<const core::contracts::IPCodeDecoder> decoder_;
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
    core::AddressSpaceId address_space_;
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
            if (!provider_memory)
                return std::unexpected(core::Error::make(core::DiagnosticCode::resource_unavailable,
                                                         "Decompiler requires an IMemoryProvider contract"));
            const auto address_space = request.function.key.entry.space;
            auto legacy_memory = std::make_shared<LegacyMemoryProvider>(provider_memory, address_space);
            if (!request.providers.pcode)
                return std::unexpected(core::Error::make(core::DiagnosticCode::resource_unavailable,
                                                         "Decompiler requires an IPCodeDecoder contract"));
            auto legacy_pcode =
                std::make_shared<ContractPcodeProvider>(request.providers.pcode, provider_memory, address_space);
            const auto architecture = request.providers.architecture
                                          ? make_native_architecture(*request.providers.architecture)
                                          : default_native_architecture();
            newghidra::decompiler::Decompiler native{architecture, legacy_pcode, legacy_memory};
            const auto ranges = request.function.body.ranges();
            if (ranges.empty())
                return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                         "Cannot decompile a function with an empty body"));
            const auto body_end =
                std::ranges::max_element(ranges, {}, [](const auto& range) { return range.end.offset; });
            if (body_end->end.offset > std::numeric_limits<std::uint64_t>::max() - 16U)
                return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                         "Function body is too close to the address-space limit"));
            const auto native_end = body_end->end.offset + 16U;
            const auto result = native.decompile(newghidra::decompiler::FunctionDescription{
                request.function.name, request.function.key.entry.offset, native_end});
            core::Decompilation decompilation;
            decompilation.function = request.function.key;
            decompilation.read_revision = request.read_revision;
            decompilation.status = core::DecompilationStatus::complete;
            if (request.include_text)
                decompilation.c_source = result.c_source;
            if (request.include_control_flow)
                decompilation.control_flow_text = result.control_flow;
            const auto provider_project = request.providers.project ? request.providers.project : query_;
            if (provider_project)
                for (const auto& instruction : provider_project->instructions())
                    if (instruction.key.address.space == request.function.key.entry.space &&
                        instruction.key.address.offset >= request.function.key.entry.offset &&
                        instruction.key.address.offset <= body_end->end.offset)
                        decompilation.raw_instructions.push_back(instruction);
            return decompilation;
        } catch (const std::exception& error) {
            return fallback(request, std::string("Native decompiler range recovery used: ") + error.what());
        } catch (...) {
            return fallback(request, "Native decompiler range recovery used after an unknown native exception");
        }
    }

private:
    /// Converts canonical architecture facts into the native frontend description without inventing x86 metadata.
    [[nodiscard]] static newghidra::decompiler::ArchitectureDescription
    make_native_architecture(const core::ArchitectureDescription& source) {
        newghidra::decompiler::ArchitectureDescription result;
        result.name = source.architecture_id.empty() ? source.language_id : source.architecture_id;
        result.code_space = source.code_space.name();
        result.data_space = source.data_space.name();
        result.pointer_size = source.pointer_size;
        result.calling_convention = source.calling_conventions.empty() ? "default" : source.calling_conventions.front();
        for (std::size_t index = 0; index < source.spaces.size(); ++index) {
            const auto& space = source.spaces[index];
            result.spaces.push_back(newghidra::decompiler::SpaceDescription{
                space.id.name(), std::max<std::uint32_t>(1, space.address_bits / 8), space.addressable_unit_size,
                space.big_endian, static_cast<std::int32_t>(index), 0, space.physical});
        }
        for (const auto& register_description : source.registers)
            result.registers.push_back(
                newghidra::decompiler::RegisterDescription{register_description.name, register_description.storage});
        const auto stack = std::ranges::find_if(result.registers, [](const auto& register_description) {
            return register_description.name == "RSP" || register_description.name == "SP";
        });
        if (stack != result.registers.end())
            result.stack_register = stack->name;
        return result;
    }

    /// Provides the explicit compatibility architecture for callers that have no architecture contract yet.
    [[nodiscard]] static newghidra::decompiler::ArchitectureDescription default_native_architecture() {
        core::ArchitectureDescription source;
        source.language_id = "x86:LE:64:default";
        source.architecture_id = "x86:LE:64:default:gcc";
        source.pointer_size = 8;
        source.calling_conventions = {"__cdecl"};
        source.spaces = {
            core::AddressSpaceDescriptor{core::AddressSpaceId{"ram"}, core::AddressSpaceKind::ram, 64, 1, 8, false,
                                         false, true},
            core::AddressSpaceDescriptor{core::AddressSpaceId{"register"}, core::AddressSpaceKind::reg, 64, 1, 8, false,
                                         false, true},
        };
        source.registers = {
            core::RegisterDescriptor{"RAX", {"register", 0, 8}},
            core::RegisterDescriptor{"RCX", {"register", 8, 8}},
            core::RegisterDescriptor{"RDX", {"register", 0x10, 8}},
            core::RegisterDescriptor{"RBX", {"register", 0x18, 8}},
            core::RegisterDescriptor{"RSP", {"register", 0x20, 8}},
            core::RegisterDescriptor{"R8", {"register", 0x80, 8}},
            core::RegisterDescriptor{"R9", {"register", 0x88, 8}},
        };
        return make_native_architecture(source);
    }

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
        const auto provider_project = request.providers.project ? request.providers.project : query_;
        if (provider_project) {
            for (const auto& instruction : provider_project->instructions()) {
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
