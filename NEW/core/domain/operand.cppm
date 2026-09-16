export module ghidra.core.operand;

import std;
import ghidra.core.address;
import ghidra.core.scalar;
import ghidra.core.storage_location;

export namespace ghidra::core {

/// Classifies one decoded operand.
enum class OperandKind : std::uint8_t { unknown, register_value, immediate, memory, address };

/// Retains the scalar/register/address objects used by FID and listing clients.
struct OperandObject {
    enum class Kind : std::uint8_t { scalar, register_value, address };
    Kind kind{Kind::scalar};
    std::int64_t value{};
    bool whole_scalar{};
    bool address_scalar{};
    bool relocated{};
};

/// Describes one instruction operand and its exact byte mask.
struct InstructionOperand {
    std::string text;
    OperandKind kind{OperandKind::unknown};
    std::optional<Scalar> scalar;
    std::optional<Address> address;
    std::optional<RegisterDescriptor> register_value;
    std::vector<std::uint8_t> value_mask;
    std::vector<OperandObject> objects;
};

} // namespace ghidra::core
