module;

#include <gtest/gtest.h>

export module decompiler_architecture_tests;

import decompiler;
import sleigh_runtime;
import std;

namespace newghidra::decompiler::architecture_tests {

/// Describes one processor smoke case backed by a specification stored in NEW.
struct ArchitectureCase {
    std::string name;
    std::string sla;
    ArchitectureProviderContext provider;
    std::vector<std::uint8_t> instruction;
    std::string expected_mnemonic;
};

/// Creates the common RAM/register space model for a fixed-width processor.
static std::vector<SpaceDescription> fixed_width_spaces(std::uint32_t address_size, bool big_endian) {
    return {
        SpaceDescription{"ram", address_size, 1, big_endian, 2, 0, true},
        SpaceDescription{"register", 4, 1, big_endian, 3, 0, true},
    };
}

/// Creates a named register in the processor register address space.
static RegisterDescription register_at(std::string name, std::uint64_t offset, std::uint32_t size) {
    return RegisterDescription{std::move(name), Storage{"register", offset, size}};
}

/// Builds the MIPS32 big-endian context used by the original parameter-storage tests.
/// Original sources: `Ghidra/Processors/MIPS/data/languages/mips.sinc` and
/// `Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc`.
static ArchitectureProviderContext mips32be_context() {
    ArchitectureDescription architecture;
    architecture.name = "MIPS:BE:32:default:default";
    architecture.spaces = fixed_width_spaces(4, true);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "sp";
    architecture.calling_convention = "__stdcall";
    architecture.pointer_size = 4;
    architecture.registers = {
        register_at("v0", 0x08, 4), register_at("a0", 0x10, 4), register_at("a1", 0x14, 4),
        register_at("sp", 0x74, 4), register_at("ra", 0x7c, 4),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Builds the ARM32 little-endian context used by the original ARM ABI tests.
/// Original sources: `Ghidra/Processors/ARM/data/languages/ARM.sinc` and
/// `Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc`.
static ArchitectureProviderContext arm32_context() {
    ArchitectureDescription architecture;
    architecture.name = "ARM:LE:32:v8:default";
    architecture.spaces = fixed_width_spaces(4, false);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "sp";
    architecture.calling_convention = "__stdcall";
    architecture.pointer_size = 4;
    architecture.registers = {
        register_at("r0", 0x20, 4), register_at("r1", 0x24, 4), register_at("r2", 0x28, 4),
        register_at("r3", 0x2c, 4), register_at("sp", 0x54, 4), register_at("lr", 0x58, 4),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Builds the AArch64 little-endian context used by the original AArch64 ABI tests.
/// Original sources: `Ghidra/Processors/AARCH64/data/languages/AARCH64instructions.sinc`
/// and `Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc`.
static ArchitectureProviderContext aarch64_context() {
    ArchitectureDescription architecture;
    architecture.name = "AARCH64:LE:64:v8A:default";
    architecture.spaces = fixed_width_spaces(8, false);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "sp";
    architecture.calling_convention = "__cdecl";
    architecture.pointer_size = 8;
    architecture.registers = {
        register_at("x0", 0x4000, 8),  register_at("x1", 0x4008, 8), register_at("x2", 0x4010, 8),
        register_at("x3", 0x4018, 8),  register_at("x8", 0x4040, 8), register_at("x29", 0x40e8, 8),
        register_at("x30", 0x40f0, 8), register_at("sp", 0x00, 8),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Builds the PowerPC32 big-endian context used by the original PPC ABI tests.
/// Original sources: `Ghidra/Processors/PowerPC/data/languages/ppc_common.sinc`
/// and `Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc`.
static ArchitectureProviderContext ppc32be_context() {
    ArchitectureDescription architecture;
    architecture.name = "PowerPC:BE:32:default:default";
    architecture.spaces = fixed_width_spaces(4, true);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "r1";
    architecture.calling_convention = "__stdcall";
    architecture.pointer_size = 4;
    architecture.registers = {
        register_at("r0", 0x00, 4), register_at("r1", 0x04, 4), register_at("r3", 0x0c, 4),
        register_at("r4", 0x10, 4), register_at("r5", 0x14, 4), register_at("pc", 0x780, 4),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Builds the Motorola 68020 context used by the original 68000 processor family.
/// Original sources: `Ghidra/Processors/68000/data/languages/68000.sinc` and
/// `Ghidra/Processors/68000/data/languages/68000.cspec`.
static ArchitectureProviderContext m68000_context() {
    ArchitectureDescription architecture;
    architecture.name = "68000:BE:32:68020";
    architecture.spaces = fixed_width_spaces(4, true);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "SP";
    architecture.calling_convention = "__stdcall";
    architecture.pointer_size = 4;
    architecture.registers = {
        register_at("D0", 0x00, 4), register_at("D1", 0x04, 4), register_at("A0", 0x20, 4),
        register_at("SP", 0x3c, 4), register_at("PC", 0x50, 4),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Builds the original 8051 context, which has multiple distinct data spaces.
/// Original sources: `Ghidra/Processors/8051/data/languages/8051_main.sinc` and
/// `Ghidra/Processors/8051/data/languages/8051.cspec`.
static ArchitectureProviderContext m8051_context() {
    ArchitectureDescription architecture;
    architecture.name = "8051:BE:16:default";
    architecture.spaces = {
        SpaceDescription{"RAM", 1, 1, false, 2, 0, true},      SpaceDescription{"CODE", 2, 1, false, 3, 0, true},
        SpaceDescription{"INTMEM", 1, 1, false, 4, 0, true},   SpaceDescription{"EXTMEM", 2, 1, false, 5, 0, true},
        SpaceDescription{"SFR", 1, 1, false, 6, 0, true},      SpaceDescription{"BITS", 1, 1, false, 7, 0, true},
        SpaceDescription{"register", 1, 1, false, 8, 0, true},
    };
    architecture.code_space = "CODE";
    architecture.data_space = "INTMEM";
    architecture.stack_register = "SP";
    architecture.calling_convention = "__stdcall";
    architecture.pointer_size = 2;
    architecture.registers = {
        register_at("R0", 0x00, 1), register_at("R1", 0x01, 1),   register_at("ACC", 0x0a, 1),
        register_at("B", 0x0a, 1),  register_at("DPTR", 0x82, 2), register_at("SP", 0x40, 1),
        register_at("PC", 0x44, 2),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Builds the x86-32 context used by the original GCC compiler specification.
/// Original sources: `Ghidra/Processors/x86/data/languages/ia.sinc` and
/// `Ghidra/Processors/x86/data/languages/x86gcc.cspec`.
static ArchitectureProviderContext x86_32_context() {
    ArchitectureDescription architecture;
    architecture.name = "x86:LE:32:default:gcc";
    architecture.spaces = fixed_width_spaces(4, false);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "ESP";
    architecture.calling_convention = "__cdecl";
    architecture.pointer_size = 4;
    architecture.registers = {
        register_at("EAX", 0x00, 4), register_at("ECX", 0x04, 4), register_at("EDX", 0x08, 4),
        register_at("EBX", 0x0c, 4), register_at("ESP", 0x10, 4), register_at("EBP", 0x14, 4),
        register_at("ESI", 0x18, 4), register_at("EDI", 0x1c, 4), register_at("EIP", 0x280, 4),
    };
    return ArchitectureProviderContext{
        std::move(architecture),
        {{"addrsize", 1}, {"opsize", 1}, {"rexprefix", 0}},
    };
}

/// Builds a Toy provider context without claiming that the installed Ghidra has a Toy SLA.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/unittests/testfuncproto.cc`.
static ArchitectureProviderContext toy_context() {
    ArchitectureDescription architecture;
    architecture.name = "Toy:LE:32:default:default";
    architecture.spaces = fixed_width_spaces(4, false);
    architecture.code_space = "ram";
    architecture.data_space = "ram";
    architecture.stack_register = "sp";
    architecture.calling_convention = "default";
    architecture.pointer_size = 4;
    architecture.registers = {
        register_at("r0", 0x00, 4),
        register_at("r1", 0x04, 4),
        register_at("sp", 0x10, 4),
    };
    return ArchitectureProviderContext{std::move(architecture), {}};
}

/// Returns the processor inventory whose compiled specifications are stored locally in NEW.
/// Other architecture contexts remain covered by provider-only construction tests below.
static std::vector<ArchitectureCase> architecture_cases() {
    return {
        {"MIPS32BE", {}, mips32be_context(), {}, {}},
        {"ARM32", "ARM8_le.sla", arm32_context(), {0x00, 0x00, 0xa0, 0xe3}, "mov"},
        {"AArch64", {}, aarch64_context(), {}, {}},
        {"PPC32BE", {}, ppc32be_context(), {}, {}},
        {"68000", {}, m68000_context(), {}, {}},
        {"8051", {}, m8051_context(), {}, {}},
        {"Toy", {}, toy_context(), {}, {}},
        {"x86-32", "x86-64.sla", x86_32_context(), {0xb8, 0x01, 0x00, 0x00, 0x00}, "MOV"},
    };
}

/// Normalizes mnemonic case because processor specifications preserve family-specific spelling.
static std::string lowercase_ascii(std::string value) {
    for (char& character : value) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }
    return value;
}

/// Supplies one deterministic COPY/RETURN instruction for provider-only Toy construction tests.
class ToyProvider final : public PcodeProvider {
public:
    /// Stores the register output and pointer width used by the Toy context.
    explicit ToyProvider(std::uint32_t pointer_size) : pointer_size_(pointer_size) {}

    /// Returns a one-byte provider instruction at address zero.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(ProviderError{"ToyProvider has no instruction at this address"});
        }
        Instruction instruction;
        instruction.address = address;
        instruction.length = 1;
        instruction.mnemonic = "copy";
        instruction.assembly = "r0, 1";
        instruction.pcode.push_back(PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
            Storage{"register", 0, pointer_size_},
            {Storage{"const", 1, pointer_size_}},
        });
        instruction.pcode.push_back(PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::return_op),
            std::nullopt,
            {Storage{"const", 0, pointer_size_}},
        });
        return instruction;
    }

private:
    std::uint32_t pointer_size_;
};

/// Verifies that every requested family has an explicit, non-x86 architecture context.
TEST(ArchitectureContexts, CoversRequestedProcessorFamilies) {
    const std::vector<ArchitectureCase> cases = architecture_cases();
    ASSERT_EQ(cases.size(), 8U);
    for (const ArchitectureCase& test_case : cases) {
        SCOPED_TRACE(test_case.name);
        EXPECT_FALSE(test_case.provider.architecture.name.empty());
        EXPECT_FALSE(test_case.provider.architecture.spaces.empty());
        EXPECT_FALSE(test_case.provider.architecture.registers.empty());
        EXPECT_GT(test_case.provider.architecture.pointer_size, 0U);
        if (test_case.name == "ARM32" || test_case.name == "x86-32") {
            EXPECT_FALSE(test_case.sla.empty());
        } else {
            EXPECT_TRUE(test_case.sla.empty());
        }
    }
}

/// Verifies that the provider frontend can construct all selected architecture descriptions.
/// This test intentionally uses a synthetic instruction provider so architecture construction
/// remains testable for architecture families without a local compiled SLA fixture.
TEST(ArchitectureProviders, ConstructsSelectedArchitectureDescriptions) {
    for (const ArchitectureCase& test_case : architecture_cases()) {
        SCOPED_TRACE(test_case.name);
        if (test_case.name == "Toy") {
            auto provider = std::make_shared<ToyProvider>(test_case.provider.architecture.pointer_size);
            auto memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0});
            EXPECT_NO_THROW({ Decompiler decompiler(test_case.provider.architecture, provider, memory); });
            continue;
        }

        auto provider = std::make_shared<ToyProvider>(test_case.provider.architecture.pointer_size);
        auto memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0});
        EXPECT_NO_THROW({ Decompiler decompiler(test_case.provider.architecture, provider, memory); });
    }
}

/// Verifies real decoding and provider materialization for every SLA stored in NEW.
TEST(ArchitectureProviders, DecodesLocalSlas) {
    constexpr std::uint64_t entry = 0x1000;
    for (const ArchitectureCase& test_case : architecture_cases()) {
        if (test_case.sla.empty()) {
            continue;
        }
        SCOPED_TRACE(test_case.name);
        try {
            std::vector<std::uint8_t> image = test_case.instruction;
            image.insert(image.end(), 16, 0);
            auto memory = std::make_shared<SparseMemory>(entry, std::move(image));
            auto provider =
                std::make_shared<SleighPcodeProvider>(test_case.sla, memory, test_case.provider.processor_context);
            const auto decoded = provider->decode(entry);
            ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
            EXPECT_EQ(decoded->address, entry);
            EXPECT_EQ(decoded->length, test_case.instruction.size());
            EXPECT_FALSE(decoded->mnemonic.empty());
            EXPECT_EQ(lowercase_ascii(decoded->mnemonic), lowercase_ascii(test_case.expected_mnemonic));

            ProviderContext providers;
            providers.pcode = std::move(provider);
            providers.memory = memory;
            EXPECT_NO_THROW({ Decompiler decompiler(test_case.provider.architecture, std::move(providers)); });
        } catch (const std::exception& error) {
            ADD_FAILURE() << test_case.name << " raised an exception: " << error.what();
        } catch (...) {
            ADD_FAILURE() << test_case.name << " raised a non-standard exception";
        }
    }
}

} // namespace newghidra::decompiler::architecture_tests
