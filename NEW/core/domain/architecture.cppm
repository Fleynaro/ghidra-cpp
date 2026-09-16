export module ghidra.core.architecture;

import std;
import ghidra.core.address_space;
import ghidra.core.identifiers;
import ghidra.core.storage_location;

export namespace ghidra::core {

/// Names one language/compiler/architecture resource set.
struct ArchitectureDescription {
    std::string language_id;
    std::string compiler_spec_id;
    std::string architecture_id;
    bool big_endian{};
    std::uint32_t pointer_size{8};
    std::vector<AddressSpaceDescriptor> spaces;
    std::vector<RegisterDescriptor> registers;
    AddressSpaceId code_space{"ram"};
    AddressSpaceId data_space{"ram"};
    std::uint32_t instruction_alignment{1};
    std::vector<std::string> calling_conventions;
    std::vector<std::string> feature_flags;
};

/// Identifies a language resource independently of native SLA objects.
using LanguageId = StrongIdentifier<struct LanguageIdTag>;
/// Identifies a compiler specification resource.
using CompilerSpecId = StrongIdentifier<struct CompilerSpecIdTag>;
/// Identifies an architecture family.
using ArchitectureId = StrongIdentifier<struct ArchitectureIdTag>;

} // namespace ghidra::core
