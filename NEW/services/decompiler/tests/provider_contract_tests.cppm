module;

#include <gtest/gtest.h>

export module provider_contract_tests;

import decompiler;
import ghidra.decompiler;
import std;

namespace newghidra::decompiler::provider_contract_tests {

/// Returns a malformed branch with no destination so validation is exercised
/// before the native `PcodeEmitFd::dump` operand array is indexed.
class MalformedArityProvider final : public PcodeProvider {
public:
    /// Produces the one malformed instruction used by the arity contract test.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        Instruction instruction;
        instruction.address = address;
        instruction.length = 1;
        instruction.pcode.push_back(PcodeOperation{static_cast<std::uint32_t>(ghidra::CPUI_BRANCH), std::nullopt, {}});
        return instruction;
    }
};

/// Records the native address-space name and returns deterministic bytes for
/// the space-aware LoadImage contract test.
class SpaceAwareMemoryProvider final : public MemoryProvider {
public:
    /// Rejects the legacy offset-only call so a lost address space is visible.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, ProviderError> read(std::uint64_t,
                                                                               std::size_t) const override {
        return std::unexpected(ProviderError{"The offset-only memory API was used"});
    }

    /// Records `space` and returns zero-filled bytes for any bounded request.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, ProviderError> read(std::string_view space, std::uint64_t,
                                                                               std::size_t size) const override {
        last_space_ = std::string(space);
        return std::vector<std::uint8_t>(size, 0);
    }

    /// Returns the most recent space observed by the provider.
    [[nodiscard]] std::string last_space() const {
        return last_space_;
    }

private:
    mutable std::string last_space_;
};

/// Emits one LOAD in the non-default `io` address space so ProviderLoadImage
/// must forward the native Address space rather than only its offset.
class IoLoadProvider final : public PcodeProvider {
public:
    /// Produces the bounded LOAD/RETURN body used by the address-space test.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(ProviderError{"IoLoadProvider has no instruction at this address"});
        }
        Instruction instruction;
        instruction.address = address;
        instruction.length = 1;
        instruction.pcode.push_back(PcodeOperation{static_cast<std::uint32_t>(ghidra::CPUI_LOAD),
                                                   Storage{"register", 0, 4},
                                                   {Storage{"const", 0x20, 4}},
                                                   std::optional<std::string>{"io"}});
        instruction.pcode.push_back(
            PcodeOperation{static_cast<std::uint32_t>(ghidra::CPUI_RETURN), std::nullopt, {Storage{"register", 0, 4}}});
        return instruction;
    }
};

/// Builds an architecture whose data space is `io` and whose addressable unit
/// size differs from the code space, covering the production Address boundary.
static ArchitectureDescription contract_architecture() {
    ArchitectureDescription description;
    description.name = "provider-contract-architecture";
    description.code_space = "code";
    description.data_space = "io";
    description.stack_register = "SP";
    description.pointer_size = 4;
    description.spaces = {
        SpaceDescription{"code", 4, 1, false, 2, 0, true},
        SpaceDescription{"io", 4, 2, false, 3, 0, true},
        SpaceDescription{"register", 4, 1, false, 4, 0, true},
    };
    description.registers = {
        RegisterDescription{"R0", Storage{"register", 0, 4}},
        RegisterDescription{"SP", Storage{"register", 4, 4}},
    };
    return description;
}

/// Verifies that malformed fixed-arity p-code becomes a clear provider error
/// instead of reaching native operand indexing with an undersized array.
TEST(ProviderContract, RejectsMalformedPcodeArityBeforeNativeEmission) {
    auto provider = std::make_shared<MalformedArityProvider>();
    auto memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0});
    Decompiler decompiler(contract_architecture(), std::move(provider), std::move(memory));

    try {
        static_cast<void>(decompiler.decompile(FunctionDescription{"malformed", 0, 1}));
        FAIL() << "Malformed p-code unexpectedly reached native analysis";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("Provider p-code BRANCH has 0 inputs; expected 1"), std::string::npos);
    }
}

/// Verifies both backward-compatible delegation and end-to-end preservation of
/// a non-default native address space through ProviderLoadImage.
TEST(ProviderContract, PreservesMemorySpaceThroughLoadImage) {
    auto provider = std::make_shared<IoLoadProvider>();
    auto memory = std::make_shared<SpaceAwareMemoryProvider>();
    MemoryProvider& memory_contract = *memory;
    const auto direct_read = memory_contract.read("io", 0x20, 4);
    ASSERT_TRUE(direct_read);
    ASSERT_EQ(direct_read->size(), 4U);

    Decompiler decompiler(contract_architecture(), std::move(provider), memory);
    static_cast<void>(decompiler.decompile(FunctionDescription{"io_load", 0, 1}));

    EXPECT_EQ(memory->last_space(), "io");
}

} // namespace newghidra::decompiler::provider_contract_tests
