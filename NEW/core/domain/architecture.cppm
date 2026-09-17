export module recode.core.architecture;

import std;
import recode.core.address_space;
import recode.core.identifiers;
import recode.core.storage_location;

export namespace recode::core {

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

    /// Reports whether this canonical architecture declares a named address space.
    [[nodiscard]] bool has_space(std::string_view name) const noexcept {
        return std::ranges::any_of(spaces, [&](const auto& space) { return space.id.name() == name; });
    }
};

/// Identifies a language resource independently of native SLA objects.
using LanguageId = StrongIdentifier<struct LanguageIdTag>;
/// Identifies a compiler specification resource.
using CompilerSpecId = StrongIdentifier<struct CompilerSpecIdTag>;
/// Identifies an architecture family.
using ArchitectureId = StrongIdentifier<struct ArchitectureIdTag>;

} // namespace recode::core
