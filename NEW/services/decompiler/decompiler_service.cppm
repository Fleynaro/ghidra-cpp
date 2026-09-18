export module recode.service.decompiler;

import std;
import recode.core;
import recode.runtime.workers.pool;
import decompiler;
import recode.decompiler;

export namespace recode::services::decompiler {

namespace core = recode::core;
namespace runtime = recode::runtime;

/// Supplies the processor mode required by the x86-64 SLA for provider reads.
[[nodiscard]] core::ProcessorContext x86_64_processor_context() {
    return core::ProcessorContext{{{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}}};
}

/// Bridges canonical memory reads to the native decompiler LoadImage contract.
class LegacyMemoryProvider final : public recode::decompiler::MemoryProvider {
public:
    /// Retains a non-owning shared provider for the duration of a decompiler task.
    LegacyMemoryProvider(std::shared_ptr<const core::contracts::IMemoryProvider> memory,
                         core::AddressSpaceId address_space)
        : memory_(std::move(memory)), address_space_(std::move(address_space)) {}

    /// Reads a native ram range through the canonical provider.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, recode::decompiler::ProviderError>
    read(std::uint64_t address, std::size_t size) const override {
        auto bytes = memory_->read(core::Address{address_space_, address}, size);
        if (!bytes)
            return std::unexpected(recode::decompiler::ProviderError{bytes.error().message});
        return bytes->values();
    }

    /// Promotes canonical volatile ranges into the native memory-provider representation.
    [[nodiscard]] std::vector<recode::decompiler::MemoryRangeDescription> volatile_ranges() const override {
        std::vector<recode::decompiler::MemoryRangeDescription> result;
        for (const auto& range : memory_->volatile_ranges()) {
            if (range.start.space != address_space_)
                continue;
            result.push_back(recode::decompiler::MemoryRangeDescription{range.start.space.name(), range.start.offset,
                                                                        range.end.offset - range.start.offset + 1U});
        }
        return result;
    }

private:
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
    core::AddressSpaceId address_space_;
};

/// Adapts the core decoder contract to the native decompiler p-code provider.
class ContractPcodeProvider final : public recode::decompiler::PcodeProvider {
public:
    /// Retains canonical decoder and memory contracts for one native task.
    ContractPcodeProvider(std::shared_ptr<const core::contracts::IPCodeDecoder> decoder,
                          std::shared_ptr<const core::contracts::IMemoryProvider> memory,
                          core::AddressSpaceId address_space)
        : decoder_(std::move(decoder)), memory_(std::move(memory)), address_space_(std::move(address_space)) {}

    /// Reads a bounded instruction through the core decoder and converts its p-code values.
    [[nodiscard]] std::expected<recode::decompiler::Instruction, recode::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        auto bytes = memory_->read(core::Address{address_space_, address}, 16);
        if (!bytes)
            return std::unexpected(recode::decompiler::ProviderError{bytes.error().message});
        core::contracts::DecodeRequest request;
        request.address = core::Address{address_space_, address};
        request.bytes = std::move(*bytes);
        request.context = x86_64_processor_context();
        auto decoded = decoder_->decode(request);
        if (!decoded)
            return std::unexpected(recode::decompiler::ProviderError{decoded.error().message});
        // The Sleigh adapter records LOAD/STORE's legacy address-space selector
        // as a pointer into its own native runtime. That pointer is not valid in
        // the separately constructed decompiler architecture, so retain the
        // canonical space name and let the native adapter create its own selector.
        for (auto& operation : decoded->pcode)
            if (operation.memory_space && !operation.inputs.empty() && operation.inputs.front().space.name() == "const")
                operation.inputs.erase(operation.inputs.begin());
        return *decoded;
    }

private:
    std::shared_ptr<const core::contracts::IPCodeDecoder> decoder_;
    std::shared_ptr<const core::contracts::IMemoryProvider> memory_;
    core::AddressSpaceId address_space_;
};

/// Supplies only direct callees reachable from the requested root, avoiding a
/// global child-function table that can mix unrelated native body ranges.
class QueryFunctionProvider final : public recode::decompiler::FunctionProvider {
public:
    /// Captures the immutable query and root entry used for bounded call discovery.
    QueryFunctionProvider(std::shared_ptr<const core::contracts::IProjectQuery> query, std::uint64_t root_entry)
        : query_(std::move(query)), root_entry_(root_entry) {}

    /// Returns direct-call child bodies whose ranges are known by the projection.
    [[nodiscard]] std::vector<recode::decompiler::FunctionDescription> functions() const override {
        std::set<std::uint64_t> callees;
        if (!query_)
            return {};
        for (const auto& instruction : query_->instructions()) {
            const auto mnemonic_end = instruction.assembly.find(' ');
            const auto mnemonic = instruction.assembly.substr(0, mnemonic_end);
            if (mnemonic != "CALL" && (mnemonic.empty() || mnemonic.front() != 'J'))
                continue;
            const auto marker_start = instruction.assembly.find("0x");
            if (marker_start == std::string::npos)
                continue;
            const auto marker = marker_start + 2U;
            const auto end = instruction.assembly.find_first_not_of("0123456789abcdefABCDEF", marker);
            const auto text =
                instruction.assembly.substr(marker, end == std::string::npos ? std::string::npos : end - marker);
            std::uint64_t target{};
            const auto [parsed_end, parse_error] = std::from_chars(text.data(), text.data() + text.size(), target, 16);
            if (parse_error == std::errc{} && parsed_end == text.data() + text.size())
                callees.insert(target);
        }
        std::vector<recode::decompiler::FunctionDescription> result;
        for (const auto& function : query_->functions()) {
            if (function.key.entry.offset == root_entry_ || !callees.contains(function.key.entry.offset) ||
                function.body.ranges().empty())
                continue;
            const auto maximum = std::ranges::max_element(function.body.ranges(), {},
                                                          [](const auto& range) { return range.end.offset; });
            if (maximum->end.offset == std::numeric_limits<std::uint64_t>::max())
                continue;
            result.push_back(recode::decompiler::FunctionDescription{function.name, function.key.entry.offset,
                                                                     maximum->end.offset + 1U});
        }
        return result;
    }

private:
    std::shared_ptr<const core::contracts::IProjectQuery> query_;
    std::uint64_t root_entry_{};
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
            // The migrated native engine keeps process-global XML/attribute tables. The
            // public task contract remains asynchronous, but native sessions must be
            // serialized until those tables become explicitly thread-safe.
            static std::mutex native_pipeline_mutex;
            std::scoped_lock native_pipeline_lock(native_pipeline_mutex);
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
            const auto architecture =
                request.providers.architecture && !request.providers.architecture->registers.empty()
                    ? make_native_architecture(*request.providers.architecture)
                    : default_native_architecture();
            recode::decompiler::ProviderContext native_context;
            native_context.pcode = legacy_pcode;
            native_context.memory = legacy_memory;
            native_context.functions = std::make_shared<QueryFunctionProvider>(
                request.providers.project ? request.providers.project : query_, request.function.key.entry.offset);
            recode::decompiler::Decompiler native{architecture, std::move(native_context)};
            const auto ranges = request.function.body.ranges();
            if (ranges.empty())
                return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                         "Cannot decompile a function with an empty body"));
            const auto body_end =
                std::ranges::max_element(ranges, {}, [](const auto& range) { return range.end.offset; });
            if (body_end->end.offset == std::numeric_limits<std::uint64_t>::max())
                return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                         "Function body is too close to the address-space limit"));
            // The native FunctionDescription range is exclusive and is bounded
            // to the decoded function body so neighboring exports are not read.
            const auto native_end = body_end->end.offset + 1U;
            const auto result = native.decompile(recode::decompiler::FunctionDescription{
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
        } catch (const ghidra::DecoderError& error) {
            return fallback(request, std::string("Native decompiler decoder failure: ") + error.explain);
        } catch (const ghidra::LowlevelError& error) {
            return fallback(request, std::string("Native decompiler failure: ") + error.explain);
        } catch (const std::exception& error) {
            return fallback(request, std::string("Native decompiler range recovery used: ") + error.what());
        } catch (...) {
            return fallback(request, "Native decompiler range recovery used after an unknown native exception");
        }
    }

private:
    /// Converts canonical architecture facts into the native frontend description without inventing x86 metadata.
    [[nodiscard]] static recode::decompiler::ArchitectureDescription
    make_native_architecture(const core::ArchitectureDescription& source) {
        recode::decompiler::ArchitectureDescription result;
        result.name = source.architecture_id.empty() ? source.language_id : source.architecture_id;
        result.code_space = source.code_space.name();
        result.data_space = source.data_space.name();
        result.pointer_size = source.pointer_size;
        result.calling_convention = source.calling_conventions.empty() ? "default" : source.calling_conventions.front();
        for (std::size_t index = 0; index < source.spaces.size(); ++index) {
            const auto& space = source.spaces[index];
            result.spaces.push_back(recode::decompiler::SpaceDescription{
                space.id.name(), std::max<std::uint32_t>(1, space.address_bits / 8), space.addressable_unit_size,
                space.big_endian, static_cast<std::int32_t>(index), 0, space.physical});
        }
        for (const auto& register_description : source.registers)
            result.registers.push_back(
                recode::decompiler::RegisterDescription{register_description.name, register_description.storage});
        const auto stack = std::ranges::find_if(result.registers, [](const auto& register_description) {
            return register_description.name == "RSP" || register_description.name == "SP";
        });
        if (stack != result.registers.end())
            result.stack_register = stack->name;
        return result;
    }

    /// Provides the explicit compatibility architecture for callers that have no architecture contract yet.
    [[nodiscard]] static recode::decompiler::ArchitectureDescription default_native_architecture() {
        // Keep the provider's tested native space identifiers and register indexes.
        // A hand-written two-space approximation assigns the RAM id zero and
        // collides with the native processor-space conventions.
        return recode::decompiler::make_x86_64_architecture();
    }

    /// Produces a revision-stamped listing-backed C artifact when native flow discovers an unbounded target.
    /// This preserves a usable decompilation result for loader entry stubs while retaining the native diagnostic.
    [[nodiscard]] core::Result<core::Decompilation> fallback(const core::contracts::DecompileRequest& request,
                                                             std::string diagnostic) const {
        core::Decompilation result;
        result.function = request.function.key;
        result.read_revision = request.read_revision;
        result.status = core::DecompilationStatus::failed;
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

} // namespace recode::services::decompiler
