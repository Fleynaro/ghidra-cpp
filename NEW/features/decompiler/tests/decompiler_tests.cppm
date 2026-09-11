module;

#include "error.hh"

#include <gtest/gtest.h>

export module decompiler_tests;

import decompiler;
import sleigh_runtime;
import std;

namespace newghidra::decompiler::tests {

/// Provides one deterministic instruction so the frontend can be tested independently of decoding.
class SingleInstructionProvider final : public PcodeProvider {
public:
    /// Constructs a provider for a single one-byte COPY instruction.
    SingleInstructionProvider() = default;

    /// Returns the documented COPY operation at address zero and rejects other addresses.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(ProviderError{"SingleInstructionProvider has no instruction at this address"});
        }
        Instruction instruction;
        instruction.address = 0;
        instruction.length = 1;
        instruction.mnemonic = "mov";
        instruction.assembly = "rax, 7";
        instruction.pcode.push_back(PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
            Storage{"register", 0, 8},
            {Storage{"const", 7, 8}},
        });
        instruction.pcode.push_back(PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::return_op),
            std::nullopt,
            {Storage{"const", 0, 8}},
        });
        return instruction;
    }
};

/// Builds the minimal x86-like provider architecture used by frontend tests.
static ArchitectureDescription test_architecture() {
    ArchitectureDescription description;
    description.name = "test-x86-64";
    description.spaces = {
        SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    description.registers = {
        RegisterDescription{"RAX", Storage{"register", 0, 8}},
        RegisterDescription{"RSP", Storage{"register", 8, 8}},
    };
    return description;
}

/// Verifies the entire provider contract and the native engine's raw-flow path.
TEST(DecompilerFrontend, MaterializesProviderPcodeAndFlow) {
    try {
        auto provider = std::make_shared<SingleInstructionProvider>();
        auto memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0x90});
        Decompiler decompiler(test_architecture(), provider, memory);

        const DecompilationResult result = decompiler.decompile(FunctionDescription{"copy_one", 0, 1});

        ASSERT_EQ(result.raw_instructions.size(), 1U);
        ASSERT_EQ(result.raw_instructions.front().pcode.size(), 2U);
        EXPECT_EQ(result.raw_instructions.front().pcode.front().opcode,
                  std::to_underlying(sleigh_runtime::PcodeOpcode::copy));
        EXPECT_FALSE(result.raw_pcode.empty());
        EXPECT_FALSE(result.high_pcode.empty());
        EXPECT_EQ(result.c_source, "\nunkbyte8 copy_one(void)\n\n{\n  return 7;\n}\n");
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Verifies that malformed provider ranges fail before native analysis starts.
TEST(DecompilerFrontend, RejectsInstructionOutsideFunctionRange) {
    try {
        auto provider = std::make_shared<SingleInstructionProvider>();
        auto memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0x90});
        Decompiler decompiler(test_architecture(), provider, memory);

        EXPECT_THROW(decompiler.decompile(FunctionDescription{"invalid", 0, 0}), std::invalid_argument);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Runs Example 1 from machine bytes through Sleigh, native p-code flow, SSA,
/// type recovery, control-flow actions, and the real C printer.
TEST(DecompilerExamples, Example1StringLengthWorkerWEndToEnd) {
    // The bytes below are the complete contiguous function from Example 1.
    // Each group is one x86-64 instruction from the listing: the first three
    // save RCX/RDX/R8, SUB reserves the stack frame, and the following groups
    // implement the bounded UTF-16 scan, error assignment, optional output
    // write, epilogue, and RET. Branch targets are preserved by keeping the
    // original instruction order and relative branch bytes unchanged.
    const std::vector<std::uint8_t> function_bytes{
        0x4c, 0x89, 0x44, 0x24, 0x18,             // MOV [RSP+0x18], R8: save param_3.
        0x48, 0x89, 0x54, 0x24, 0x10,             // MOV [RSP+0x10], RDX: save param_2.
        0x48, 0x89, 0x4c, 0x24, 0x08,             // MOV [RSP+0x08], RCX: save param_1.
        0x48, 0x83, 0xec, 0x18,                   // SUB RSP, 0x18: allocate locals.
        0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // MOV local_10, 0.
        0x48, 0x8b, 0x44, 0x24, 0x28,             // MOV RAX, local_res10.
        0x48, 0x89, 0x04, 0x24,                   // MOV local_18, RAX: save original limit.
        0x48, 0x83, 0x7c, 0x24, 0x28, 0x00,      // CMP local_res10, 0.
        0x74, 0x2a,                               // JZ LAB_1417515e6.
        0x48, 0x8b, 0x44, 0x24, 0x20,             // MOV RAX, local_res8.
        0x0f, 0xb7, 0x00,                         // MOVZX EAX, word ptr [RAX]: read wchar_t.
        0x85, 0xc0,                               // TEST EAX, EAX: test for L'\0'.
        0x74, 0x1e,                               // JZ LAB_1417515e6.
        0x48, 0x8b, 0x44, 0x24, 0x20,             // MOV RAX, local_res8.
        0x48, 0x83, 0xc0, 0x02,                   // ADD RAX, 2: advance UTF-16 pointer.
        0x48, 0x89, 0x44, 0x24, 0x20,             // MOV local_res8, RAX.
        0x48, 0x8b, 0x44, 0x24, 0x28,             // MOV RAX, local_res10.
        0x48, 0x83, 0xe8, 0x01,                   // SUB RAX, 1: decrement remaining count.
        0x48, 0x89, 0x44, 0x24, 0x28,             // MOV local_res10, RAX.
        0xeb, 0xce,                               // JMP LAB_1417515b4: loop back.
        0x48, 0x83, 0x7c, 0x24, 0x28, 0x00,      // CMP local_res10, 0.
        0x75, 0x08,                               // JNZ LAB_1417515f6.
        0xc7, 0x44, 0x24, 0x08, 0x57, 0x00, 0x07, 0x80, // MOV local_10, 0x80070057.
        0x48, 0x83, 0x7c, 0x24, 0x30, 0x00,      // CMP param_3, 0.
        0x74, 0x29,                               // JZ LAB_141751627: skip optional write.
        0x83, 0x7c, 0x24, 0x08, 0x00,            // CMP local_10, 0.
        0x7c, 0x16,                               // JL LAB_14175161b: failure path.
        0x48, 0x8b, 0x44, 0x24, 0x28,             // MOV RAX, local_res10.
        0x48, 0x8b, 0x0c, 0x24,                   // MOV RCX, local_18.
        0x48, 0x2b, 0xc8,                         // SUB RCX, RAX: consumed count.
        0x48, 0x8b, 0x44, 0x24, 0x30,             // MOV RAX, param_3.
        0x48, 0x89, 0x08,                         // MOV [RAX], RCX: store consumed count.
        0xeb, 0x0c,                               // JMP LAB_141751627.
        0x48, 0x8b, 0x44, 0x24, 0x30,             // MOV RAX, param_3: failure output pointer.
        0x48, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, // MOV [RAX], 0: clear output on failure.
        0x8b, 0x44, 0x24, 0x08,                   // MOV EAX, local_10: return status.
        0x48, 0x83, 0xc4, 0x18,                   // ADD RSP, 0x18: release locals.
        0xc3,                                     // RET.
    };

    // This is the current exact output of the real decompiler pipeline for
    // the bytes above. It is intentionally kept as a complete source string
    // so later decompiler improvements produce a deliberate test diff.
    const std::string expected_c = R"(
unkbyte4 StringLengthWorkerW(void)

{
  BADSPACEBASE *in_RSP;
  unkbyte8 in_register_00000010;
  unkint8 in_register_00000020;
  unkbyte8 in_register_00000080;
  
  *(unkbyte8 *)(in_register_00000020 + 0x18) = in_register_00000080;
  *(unkbyte8 *)(in_register_00000020 + 0x10) = in_register_00000010;
  *(BADSPACEBASE **)(in_register_00000020 + 8) = in_RSP;
  *(unkbyte4 *)(in_register_00000020 + -0x10) = 0;
  *(unkint8 *)(in_register_00000020 + -0x18) = *(unkint8 *)(in_register_00000020 + 0x10);
  while ((*(unkint8 *)(in_register_00000020 + 0x10) != 0 &&
         (**(unkint2 **)(in_register_00000020 + 8) != 0))) {
    *(unkint8 *)(in_register_00000020 + 8) = *(unkint8 *)(in_register_00000020 + 8) + 2;
    *(unkint8 *)(in_register_00000020 + 0x10) = *(unkint8 *)(in_register_00000020 + 0x10) + -1;
  }
  if (*(unkint8 *)(in_register_00000020 + 0x10) == 0) {
    *(unkbyte4 *)(in_register_00000020 + -0x10) = 0x80070057;
  }
  if (*(unkint8 *)(in_register_00000020 + 0x18) != 0) {
    if (*(unkint4 *)(in_register_00000020 + -0x10) < 0) {
      **(unkbyte8 **)(in_register_00000020 + 0x18) = 0;
    }
    else {
      *(unkint8 *)*(unkbyte8 *)(in_register_00000020 + 0x18) =
           *(unkint8 *)(in_register_00000020 + -0x18) - *(unkint8 *)(in_register_00000020 + 0x10);
    }
  }
  return *(unkbyte4 *)(in_register_00000020 + -0x10);
}
)";

    // Sleigh reads a maximum 16-byte instruction window. Padding is mapped
    // after RET for that read-ahead, but is deliberately excluded from the
    // FunctionDescription range so it cannot become part of the function.
    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Read-ahead NOP padding.
    auto memory = std::make_shared<SparseMemory>(0x1000, std::move(image_bytes));
    SleighPcodeProvider provider(
        std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
        {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    Decompiler decompiler(test_architecture(), std::make_shared<SleighPcodeProvider>(std::move(provider)), memory);
    const DecompilationResult result =
        decompiler.decompile(FunctionDescription{"StringLengthWorkerW", 0x1000,
                                                 0x1000 + function_bytes.size()});

    ASSERT_FALSE(result.raw_instructions.empty());
    EXPECT_EQ(result.raw_instructions.back().mnemonic, "RET");
    ASSERT_FALSE(result.c_source.empty());
    EXPECT_EQ(result.c_source, expected_c);
}

/// Verifies that the production p-code provider consumes bytes through NEW's
/// compiled Sleigh runtime rather than a decoder embedded in this module.
TEST(SleighProvider, DecodesX86BytesIntoProviderPcode) {
    // This is the x86-64 instruction `sub rsp, 0x40` used by the runtime's
    // own instruction-family tests. The REX.W prefix selects 64-bit operands;
    // opcode 83 selects an immediate sign-extended arithmetic operation;
    // ModR/M EC selects RSP as both the destination and source; and 40 is the
    // stack-frame decrement. The remaining bytes are NOP padding so the
    // provider can honor the runtime's 16-byte decode window without reading
    // past the represented image.
    const std::vector<std::uint8_t> instruction_bytes{
        0x48, // REX.W: use 64-bit register arithmetic.
        0x83, // Group-1 arithmetic with an 8-bit immediate.
        0xec, // SUB the immediate from RSP.
        0x40, // Allocate 0x40 bytes of stack space.
        0x90, // Padding byte; not part of the decoded instruction.
        0x90, // Padding byte; keeps the decode window mapped.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; preserves deterministic test storage.
        0x90, // Padding byte; completes the runtime's maximum decode window.
    };
    auto memory = std::make_shared<SparseMemory>(0x100, instruction_bytes);
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});

    const auto decoded = provider.decode(0x100);

    if (!decoded) {
        FAIL() << decoded.error().message;
    }
    EXPECT_EQ(decoded->length, 4U);
    ASSERT_FALSE(decoded->pcode.empty());
    for (const PcodeOperation& operation : decoded->pcode) {
        EXPECT_FALSE(operation.inputs.empty());
        EXPECT_NE(operation.opcode, 0U);
    }
}

} // namespace newghidra::decompiler::tests
