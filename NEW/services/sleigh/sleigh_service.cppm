export module ghidra.service.sleigh;

import std;
import ghidra.core;
import ghidra.runtime.workers.pool;
import sleigh_runtime;

export namespace ghidra::services::sleigh {

namespace core = ghidra::core;
namespace runtime = ghidra::runtime;

/// Adapts the stateful Sleigh decoder into a mutex-protected canonical decoder service.
class SleighService final : public core::contracts::IPCodeDecoder {
public:
    /// Loads one compiled SLA and associates it with the shared runtime pool.
    static core::Result<std::shared_ptr<SleighService>> open(std::filesystem::path path,
                                                             std::shared_ptr<runtime::workers::WorkerPool> pool) {
        try {
            return std::shared_ptr<SleighService>(new SleighService(std::move(path), std::move(pool)));
        } catch (const std::exception& error) {
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::resource_unavailable, error.what(),
                                  "Verify that the compiled SLA resource exists and matches the language."));
        }
    }

    /// Decodes one bounded instruction while serializing access to native state.
    [[nodiscard]] core::Result<core::Instruction> decode(const core::contracts::DecodeRequest& request) const override {
        std::scoped_lock lock(mutex_);
        try {
            sleigh_runtime::ProcessorContext context;
            for (const auto& [name, value] : request.context.values)
                context.values.push_back({name, value});
            auto decoded = decoder_->decode(request.address.offset, request.bytes.view(), context);
            if (!decoded)
                return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure, decoded.error().message));
            return convert(*decoded, request.address);
        } catch (const std::exception& error) {
            return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure,
                                                     std::string("Sleigh decode threw: ") + error.what()));
        } catch (...) {
            return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure,
                                                     "Sleigh decode threw an unknown native exception"));
        }
    }

    /// Queues deterministic address-ordered decoding through the shared worker pool.
    [[nodiscard]] core::contracts::Task<core::Result<core::contracts::DecodeBatchResult>>
    decode_batch(const core::contracts::DecodeBatchRequest& request,
                 core::contracts::OperationContext context) const override {
        auto submitted = pool_->submit(
            context.project, core::contracts::WorkPriority::analysis,
            [this, request](core::contracts::CancellationToken token) {
                core::contracts::DecodeBatchResult result;
                result.instructions.reserve(request.requests.size());
                for (const auto& item : request.requests) {
                    if (token.stop_requested())
                        return core::Result<core::contracts::DecodeBatchResult>{std::unexpected(
                            core::Error::make(core::DiagnosticCode::cancelled, "Sleigh batch decode was cancelled"))};
                    auto decoded = decode(item);
                    if (!decoded)
                        return core::Result<core::contracts::DecodeBatchResult>{std::unexpected(decoded.error())};
                    result.instructions.push_back(std::move(*decoded));
                }
                std::ranges::sort(result.instructions, {},
                                  [](const auto& instruction) { return instruction.key.address; });
                return core::Result<core::contracts::DecodeBatchResult>{std::move(result)};
            });
        if (!submitted)
            throw std::runtime_error(submitted.error().message);
        return std::move(*submitted);
    }

    /// Returns the compiled language resource path used for diagnostics and identity.
    [[nodiscard]] const std::filesystem::path& specification_path() const noexcept {
        return specification_path_;
    }

private:
    /// Constructs a decoder after the factory has validated the resource.
    SleighService(std::filesystem::path path, std::shared_ptr<runtime::workers::WorkerPool> pool)
        : specification_path_(std::move(path)),
          decoder_(std::make_unique<sleigh_runtime::Decoder>(specification_path_)), pool_(std::move(pool)) {}

    /// Converts a legacy Sleigh instruction and every observable field to core values.
    [[nodiscard]] static core::Instruction convert(const sleigh_runtime::Instruction& source, core::Address address) {
        core::Instruction result;
        result.key =
            core::InstructionKey{core::EntityId{core::make_identifier("instruction", address.offset)}, address};
        result.length = source.length;
        result.bytes = core::Bytes{source.bytes};
        result.mnemonic = source.mnemonic;
        result.assembly = source.assembly;
        result.instruction_mask = source.instruction_mask;
        result.architecture = source.is_x86 ? "x86" : "unknown";
        result.provenance = "sleigh";
        result.pcode.instruction = address;
        for (std::size_t index = 0; index < source.operands.size(); ++index) {
            const auto& operand = source.operands[index];
            core::InstructionOperand converted;
            converted.text = operand.text;
            converted.kind = static_cast<core::OperandKind>(operand.kind);
            converted.value_mask = operand.value_mask;
            for (const auto& object : operand.objects)
                converted.objects.push_back(core::OperandObject{static_cast<core::OperandObject::Kind>(object.kind),
                                                                object.value, object.whole_scalar,
                                                                object.address_scalar, object.relocated});
            if (operand.value)
                converted.scalar = core::Scalar{*operand.value, 64, false,
                                                operand.kind == sleigh_runtime::OperandKind::address, false};
            result.operands.push_back(std::move(converted));
        }
        result.flow.kind = static_cast<core::FlowKind>(source.flow.kind);
        if (source.flow.target)
            result.flow.target = core::Address{source.flow.target->space, source.flow.target->offset};
        result.flow.has_fallthrough = source.flow.has_fallthrough;
        result.flow.terminal = source.flow.terminal;
        for (std::size_t index = 0; index < source.pcode.size(); ++index) {
            const auto& operation = source.pcode[index];
            core::PcodeOp converted;
            converted.opcode = static_cast<core::PcodeOpcode>(operation.opcode);
            converted.sequence_index = index;
            converted.memory_space = operation.memory_space;
            converted.source_operand = operation.source_operand;
            if (operation.output)
                converted.output = *operation.output;
            for (const auto& input : operation.inputs)
                converted.inputs.push_back(input);
            result.pcode.operations.push_back(std::move(converted));
        }
        return result;
    }

    std::filesystem::path specification_path_;
    std::unique_ptr<sleigh_runtime::Decoder> decoder_;
    std::shared_ptr<runtime::workers::WorkerPool> pool_;
    mutable std::mutex mutex_;
};

} // namespace ghidra::services::sleigh
