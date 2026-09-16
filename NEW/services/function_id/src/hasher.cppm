export module function_id:hasher;

import std;
import sleigh_runtime;
import :parse_exception;
import :storage_helpers;
import :types;

// Ghidra references:
// Features/FunctionID/src/main/java/ghidra/feature/fid/hash/MessageDigestFidHasher.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/hash/X86InstructionSkipper.java, and
// Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java.

export namespace fid {

/// Hashes caller-provided instruction metadata using Ghidra's FunctionID algorithm.
class Hasher final {
public:
    /// Hashes instructions, returning an error when fewer than the requested effective units exist.
    [[nodiscard]] static std::expected<HashQuad, Error> hash(std::span<const Instruction> instructions,
                                                             std::int8_t short_limit = 4) {
        try {
            detail::require(short_limit > 0, ErrorCode::invalid_input, "short hash limit must be positive");
            detail::require(instructions.size() >= static_cast<std::size_t>(short_limit), ErrorCode::invalid_input,
                            "function has too few code units for a FID hash");
            std::uint64_t full = 14695981039346656037ULL;
            std::uint64_t specific = full;
            int effective_count = 0;
            int call_count = 0;
            int specific_count = 0;
            for (const auto& instruction : instructions) {
                if (instruction.skip)
                    continue;
                detail::require(instruction.bytes.size() <= 110000, ErrorCode::invalid_input,
                                "instruction exceeds FunctionID hasher buffer");
                const bool missing_mask = instruction.instruction_mask.empty();
                const auto mask =
                    missing_mask ? std::vector<Byte>(instruction.bytes.size(), 0) : instruction.instruction_mask;
                detail::require(mask.size() == instruction.bytes.size(), ErrorCode::invalid_input,
                                "instruction mask length differs from instruction bytes");
                detail::require(instruction.operand_masks.size() == instruction.operands.size(),
                                ErrorCode::invalid_input, "operand mask and object counts differ");
                ++effective_count;
                if (instruction.is_call)
                    ++call_count;
                for (std::size_t operand_index = 0; operand_index < instruction.operands.size(); ++operand_index) {
                    const auto& operand_mask = instruction.operand_masks[operand_index];
                    if (operand_mask.empty())
                        continue;
                    std::uint32_t specific_update = static_cast<std::uint32_t>((operand_index + 1) * 7777U);
                    std::uint32_t full_update = specific_update;
                    for (const auto& object : instruction.operands[operand_index]) {
                        if (object.kind == OperandObjectKind::scalar) {
                            auto value = static_cast<std::int32_t>(object.value);
                            if (object.relocated || object.address_scalar) {
                                value = static_cast<std::int32_t>(detail::scalar_placeholder);
                            } else if (object.whole_scalar) {
                                ++specific_count;
                            } else if (value >= 256 || value <= -256) {
                                value = static_cast<std::int32_t>(detail::scalar_placeholder);
                            } else {
                                ++specific_count;
                            }
                            specific_update += detail::scalar_mix(value);
                            full_update += detail::scalar_placeholder;
                        } else if (object.kind == OperandObjectKind::register_value) {
                            const auto mixed = static_cast<std::uint32_t>(
                                (static_cast<std::uint64_t>(static_cast<std::uint32_t>(object.value)) + 7654321U) *
                                98777U);
                            specific_update += mixed;
                            full_update += mixed;
                        } else {
                            specific_update +=
                                detail::scalar_mix(static_cast<std::int32_t>(detail::scalar_placeholder));
                            full_update += detail::scalar_placeholder;
                        }
                    }
                    detail::update_int(full, full_update);
                    detail::update_int(specific, specific_update);
                }
                std::vector<Byte> masked(instruction.bytes.size());
                if (missing_mask) {
                    std::fill(masked.begin(), masked.end(), 0xa5U);
                } else {
                    for (std::size_t index = 0; index < masked.size(); ++index)
                        masked[index] = instruction.bytes[index] & mask[index];
                }
                detail::update_bytes(full, masked);
                detail::update_bytes(specific, masked);
                if (effective_count >= std::numeric_limits<std::int16_t>::max() - 1)
                    break;
            }
            detail::require(effective_count >= short_limit, ErrorCode::invalid_input,
                            "function has too few effective code units for a FID hash");
            HashQuad result;
            result.code_unit_size = static_cast<std::int16_t>(effective_count - call_count);
            result.full_hash = full;
            result.specific_hash_additional_size = static_cast<std::int8_t>(std::min(specific_count, 127));
            result.specific_hash = specific;
            return result;
        } catch (const detail::ParseException& exception) {
            return std::unexpected(exception.error);
        }
    }

    /// Adapts Sleigh's exact decoded prototype metadata to the FunctionID abstract inputs.
    [[nodiscard]] static std::expected<HashQuad, Error>
    hash_sleigh(std::span<const sleigh_runtime::Instruction> instructions, std::span<const Relocation> relocations = {},
                std::int8_t short_limit = 4) {
        try {
            std::vector<Instruction> converted;
            converted.reserve(instructions.size());
            for (const auto& decoded : instructions) {
                Instruction instruction;
                instruction.bytes = decoded.bytes;
                instruction.instruction_mask = decoded.instruction_mask;
                instruction.skip = decoded.is_x86 && detail::is_x86_skipped(decoded.bytes);
                detail::require(
                    instruction.bytes.size() == decoded.length &&
                        (instruction.instruction_mask.empty() || instruction.instruction_mask.size() == decoded.length),
                    ErrorCode::invalid_input, "Sleigh instruction bytes and mask lengths differ");
                instruction.is_call = decoded.flow.kind == sleigh_runtime::FlowKind::conditional_call ||
                                      decoded.flow.kind == sleigh_runtime::FlowKind::call ||
                                      decoded.flow.kind == sleigh_runtime::FlowKind::indirect_call;
                for (const auto& decoded_operand : decoded.operands) {
                    instruction.operand_masks.push_back(decoded_operand.value_mask);
                    std::vector<OperandObject> objects;
                    std::size_t mask_begin = decoded_operand.value_mask.size();
                    std::size_t mask_end = 0;
                    for (std::size_t index = 0; index < decoded_operand.value_mask.size(); ++index) {
                        if (decoded_operand.value_mask[index] != 0) {
                            mask_begin = std::min(mask_begin, index);
                            mask_end = index + 1;
                        }
                    }
                    for (const auto& decoded_object : decoded_operand.hash_objects) {
                        const auto relocated = [&]() {
                            if (mask_begin == decoded_operand.value_mask.size())
                                return decoded_object.relocated;
                            for (const auto& relocation : relocations) {
                                const auto operand_begin = decoded.address + mask_begin;
                                const auto operand_end = decoded.address + mask_end;
                                const auto relocation_end = relocation.address + relocation.size;
                                if (relocation.size != 0 && relocation.address < operand_end &&
                                    operand_begin < relocation_end)
                                    return true;
                            }
                            return decoded_object.relocated;
                        }();
                        objects.push_back(OperandObject{
                            decoded_object.kind == sleigh_runtime::Operand::HashObject::Kind::scalar
                                ? OperandObjectKind::scalar
                            : decoded_object.kind == sleigh_runtime::Operand::HashObject::Kind::register_value
                                ? OperandObjectKind::register_value
                                : OperandObjectKind::address,
                            decoded_object.value, decoded_object.whole_scalar, decoded_object.address_scalar,
                            relocated});
                    }
                    instruction.operands.push_back(std::move(objects));
                }
                converted.push_back(std::move(instruction));
            }
            return hash(converted, short_limit);
        } catch (const detail::ParseException& exception) {
            return std::unexpected(exception.error);
        }
    }
};

} // namespace fid
