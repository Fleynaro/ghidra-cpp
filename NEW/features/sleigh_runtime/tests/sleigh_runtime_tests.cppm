#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

import sleigh_runtime;

namespace {

/// Creates the decoder from the module-local compiled x86-64 specification.
sleigh_runtime::Decoder make_decoder() {
    return sleigh_runtime::Decoder(std::filesystem::path(SLEIGH_RUNTIME_TEST_DATA_DIR) / "x86-64.sla");
}

/// Supplies the context values required by the x86-64 compiled specification.
sleigh_runtime::ProcessorContext x86_64_context() {
    return {{{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}}};
}

/// Verifies the public structural identity of a concrete p-code varnode.
void expect_varnode(const sleigh_runtime::Varnode& actual, std::string_view space, std::uint64_t offset,
                    std::uint32_t size) {
    EXPECT_EQ(actual.space, space);
    EXPECT_EQ(actual.offset, offset);
    EXPECT_EQ(actual.size, size);
}

/// Checks every emitted varnode so tests cannot pass with an empty or symbolic p-code result.
void expect_materialized_pcode(const sleigh_runtime::Instruction& instruction) {
    ASSERT_FALSE(instruction.pcode.empty());
    for (const auto& operation : instruction.pcode) {
        EXPECT_NE(sleigh_runtime::opcode_name(operation.opcode), "UNKNOWN");
        if (operation.output.has_value()) {
            EXPECT_FALSE(operation.output->space.empty());
            EXPECT_GT(operation.output->size, 0U);
        }
        for (const auto& input : operation.inputs) {
            EXPECT_FALSE(input.space.empty());
            EXPECT_GT(input.size, 0U);
        }
    }
}

/// Verifies that the public opcode table covers the legacy extract opcode and rejects unused slots.
TEST(SleighRuntime, NamesPcodeOpcodes) {
    EXPECT_EQ(sleigh_runtime::opcode_name(sleigh_runtime::PcodeOpcode::zpull), "EXTRACT");
    EXPECT_EQ(sleigh_runtime::opcode_name(static_cast<sleigh_runtime::PcodeOpcode>(45)), "UNKNOWN");
}

/// Verifies a register move all the way from SLA matching through p-code emission.
TEST(SleighRuntime, DecodesRegisterMove) {
    auto decoder = make_decoder();
    // 48 8b d9 - MOV RBX,RCX
    const std::array<std::uint8_t, 3> bytes{0x48, 0x8b, 0xd9};
    const auto result = decoder.decode(0x140000000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 3U);
    EXPECT_EQ(result->mnemonic, "MOV");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::copy);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0x18U, 8U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x8U, 8U);
}

/// Verifies an immediate arithmetic instruction and every emitted flag operation.
TEST(SleighRuntime, DecodesStackAdjustment) {
    auto decoder = make_decoder();
    // 48 83 ec 40 - SUB RSP,0x40
    const std::array<std::uint8_t, 4> bytes{0x48, 0x83, 0xec, 0x40};
    const auto result = decoder.decode(0x140000100ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "SUB");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::immediate);
    ASSERT_TRUE(result->operands[1].value.has_value());
    EXPECT_EQ(*result->operands[1].value, 0x40U);
    ASSERT_EQ(result->pcode.size(), 9U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_less);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::int_sborrow);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::int_sub);
    EXPECT_EQ(result->pcode[8].opcode, sleigh_runtime::PcodeOpcode::int_equal);
    ASSERT_TRUE(result->pcode[2].output.has_value());
    expect_varnode(*result->pcode[2].output, "register", 0x20U, 8U);
}

/// Verifies dynamic memory addressing, a register input, and the STORE operation.
TEST(SleighRuntime, DecodesMemoryStore) {
    auto decoder = make_decoder();
    // 48 89 5c 24 08 - MOV qword ptr [RSP+8],RBX
    const std::array<std::uint8_t, 5> bytes{0x48, 0x89, 0x5c, 0x24, 0x08};
    const auto result = decoder.decode(0x140000200ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 5U);
    EXPECT_EQ(result->mnemonic, "MOV");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::memory);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    ASSERT_EQ(result->pcode.size(), 3U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_add);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::copy);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::store);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "unique", 0x9d00U, 8U);
    ASSERT_EQ(result->pcode[2].inputs.size(), 3U);
    EXPECT_EQ(result->pcode[2].inputs[0].space, "const");
    EXPECT_NE(result->pcode[2].inputs[0].offset, 0U);
    EXPECT_EQ(result->pcode[2].inputs[0].size, sizeof(void*));
    expect_varnode(result->pcode[2].inputs[2], "unique", 0xd500U, 8U);
}

/// Verifies a direct relative jump, including its resolved absolute branch target.
TEST(SleighRuntime, DecodesDirectJump) {
    auto decoder = make_decoder();
    // eb 20 - JMP LAB_140333284
    const std::array<std::uint8_t, 2> bytes{0xeb, 0x20};
    const auto result = decoder.decode(0x140333262ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 2U);
    EXPECT_EQ(result->mnemonic, "JMP");
    ASSERT_EQ(result->operands.size(), 1U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::immediate);
    ASSERT_TRUE(result->operands[0].value.has_value());
    EXPECT_EQ(*result->operands[0].value, 0x140333284ULL);
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::branch);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "ram", 0x140333284ULL, 8U);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::branch);
    ASSERT_TRUE(result->flow.target.has_value());
    expect_varnode(*result->flow.target, "ram", 0x140333284ULL, 8U);
}

/// Verifies a direct relative call, its return-address store, and its resolved target.
TEST(SleighRuntime, DecodesDirectCall) {
    auto decoder = make_decoder();
    // e8 c6 84 ff ff - CALL FUN_14032b6f4
    const std::array<std::uint8_t, 5> bytes{0xe8, 0xc6, 0x84, 0xff, 0xff};
    const auto result = decoder.decode(0x140333229ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 5U);
    EXPECT_EQ(result->mnemonic, "CALL");
    ASSERT_EQ(result->operands.size(), 1U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::immediate);
    ASSERT_TRUE(result->operands[0].value.has_value());
    EXPECT_EQ(*result->operands[0].value, 0x14032b6f4ULL);
    ASSERT_EQ(result->pcode.size(), 3U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_sub);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::store);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::call);
    ASSERT_EQ(result->pcode[1].inputs.size(), 3U);
    expect_varnode(result->pcode[1].inputs[1], "register", 0x20U, 8U);
    expect_varnode(result->pcode[1].inputs[2], "const", 0x14033322eULL, 8U);
    ASSERT_EQ(result->pcode[2].inputs.size(), 1U);
    expect_varnode(result->pcode[2].inputs[0], "ram", 0x14032b6f4ULL, 8U);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::call);
    ASSERT_TRUE(result->flow.target.has_value());
    expect_varnode(*result->flow.target, "ram", 0x14032b6f4ULL, 8U);
}

/// Verifies scalar floating-point multiplication materializes register-sized varnodes.
TEST(SleighRuntime, DecodesScalarMultiply) {
    auto decoder = make_decoder();
    // f3 0f 59 f1 - MULSS XMM6,XMM1
    const std::array<std::uint8_t, 4> bytes{0xf3, 0x0f, 0x59, 0xf1};
    const auto result = decoder.decode(0x140001200ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "MULSS");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::float_mult);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0x1380U, 4U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x1380U, 4U);
    expect_varnode(result->pcode[0].inputs[1], "register", 0x1240U, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies SHUFPS loads a packed single-precision vector from memory and emits all four selected lanes.
TEST(SleighRuntime, DecodesShufflePackedSingles) {
    auto decoder = make_decoder();
    // 0f c6 00 0a - SHUFPS XMM0,xmmword ptr [RAX],0xa
    const std::array<std::uint8_t, 4> bytes{0x0f, 0xc6, 0x00, 0x0a};
    const auto result = decoder.decode(0x140001240ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "SHUFPS");
    ASSERT_EQ(result->operands.size(), 3U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::memory);
    EXPECT_NE(result->operands[1].text.find("["), std::string::npos);
    EXPECT_EQ(result->operands[2].kind, sleigh_runtime::OperandKind::immediate);
    ASSERT_TRUE(result->operands[2].value.has_value());
    EXPECT_EQ(*result->operands[2].value, 0x0aU);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);

    // The source vector is read as four 32-bit lanes before the shuffle selects outputs.
    ASSERT_EQ(result->pcode.size(), 75U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::load);
    EXPECT_EQ(result->pcode[3].opcode, sleigh_runtime::PcodeOpcode::load);
    EXPECT_EQ(result->pcode[6].opcode, sleigh_runtime::PcodeOpcode::load);
    EXPECT_EQ(result->pcode[9].opcode, sleigh_runtime::PcodeOpcode::load);
    for (const std::size_t operation : {1U, 4U, 7U, 10U}) {
        EXPECT_EQ(result->pcode[operation].opcode, sleigh_runtime::PcodeOpcode::copy);
    }

    // The immediate 0x0a selects two source lanes and two destination lanes.
    for (const std::size_t operation : {29U, 44U, 59U, 74U}) {
        EXPECT_EQ(result->pcode[operation].opcode, sleigh_runtime::PcodeOpcode::int_add);
        ASSERT_TRUE(result->pcode[operation].output.has_value());
        expect_varnode(*result->pcode[operation].output, "register", 0x1200U + ((operation - 29U) / 15U) * 4U, 4U);
    }
    expect_materialized_pcode(*result);
}

/// Verifies AVX vector addition expands into eight lane operations and a final zero-extension.
TEST(SleighRuntime, DecodesAvxVaddps) {
    auto decoder = make_decoder();
    // c5 f4 58 c2 - VADDPS YMM0,YMM1,YMM2
    const std::array<std::uint8_t, 4> bytes{0xc5, 0xf4, 0x58, 0xc2};
    const auto result = decoder.decode(0x140000000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "VADDPS");
    ASSERT_EQ(result->operands.size(), 3U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[2].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);

    ASSERT_EQ(result->pcode.size(), 10U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::copy);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "unique", 0x19f800U, 32U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x1280U, 32U);

    for (std::size_t lane = 0; lane < 8; ++lane) {
        const auto& operation = result->pcode[lane + 1];
        EXPECT_EQ(operation.opcode, sleigh_runtime::PcodeOpcode::float_add);
        ASSERT_TRUE(operation.output.has_value());
        expect_varnode(*operation.output, "register", 0x1200U + lane * 4U, 4U);
        ASSERT_EQ(operation.inputs.size(), 2U);
        expect_varnode(operation.inputs[0], "register", 0x1240U + lane * 4U, 4U);
        expect_varnode(operation.inputs[1], "unique", 0x19f800U + lane * 4U, 4U);
    }

    EXPECT_EQ(result->pcode[9].opcode, sleigh_runtime::PcodeOpcode::int_zext);
    ASSERT_TRUE(result->pcode[9].output.has_value());
    expect_varnode(*result->pcode[9].output, "register", 0x1200U, 64U);
    ASSERT_EQ(result->pcode[9].inputs.size(), 1U);
    expect_varnode(result->pcode[9].inputs[0], "register", 0x1200U, 32U);
    expect_materialized_pcode(*result);
}

/// Verifies AVX unaligned vector move materializes the expected zero-extension.
TEST(SleighRuntime, DecodesAvxVmovdqu) {
    auto decoder = make_decoder();
    // c5 fe 6f c1 - VMOVDQU YMM0,YMM1
    const std::array<std::uint8_t, 4> bytes{0xc5, 0xfe, 0x6f, 0xc1};
    const auto result = decoder.decode(0x140000004ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "VMOVDQU");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_zext);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0x1200U, 64U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x1240U, 32U);
    expect_materialized_pcode(*result);
}

/// Verifies AVX vector xor and the resulting zero-extension into the architectural YMM value.
TEST(SleighRuntime, DecodesAvxVpxor) {
    auto decoder = make_decoder();
    // c5 fd ef c0 - VPXOR YMM0,YMM0,YMM0
    const std::array<std::uint8_t, 4> bytes{0xc5, 0xfd, 0xef, 0xc0};
    const auto result = decoder.decode(0x140000008ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "VPXOR");
    ASSERT_EQ(result->operands.size(), 3U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[2].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);
    ASSERT_EQ(result->pcode.size(), 2U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_xor);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "unique", 0x20ee00U, 32U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x1200U, 32U);
    expect_varnode(result->pcode[0].inputs[1], "register", 0x1200U, 32U);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::int_zext);
    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "register", 0x1200U, 64U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 1U);
    expect_varnode(result->pcode[1].inputs[0], "unique", 0x20ee00U, 32U);
    expect_materialized_pcode(*result);
}

/// Verifies VZEROUPPER clears all 16 YMM register halves represented by 96 p-code operations.
TEST(SleighRuntime, DecodesVzeroupper) {
    auto decoder = make_decoder();
    // c5 f8 77 - VZEROUPPER
    const std::array<std::uint8_t, 3> bytes{0xc5, 0xf8, 0x77};
    const auto result = decoder.decode(0x14000000cULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 3U);
    EXPECT_EQ(result->mnemonic, "VZEROUPPER");
    EXPECT_TRUE(result->operands.empty());
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);
    ASSERT_EQ(result->pcode.size(), 96U);
    for (std::size_t operation = 0; operation < result->pcode.size(); ++operation) {
        const std::uint64_t expected_offset = 0x1210U + (operation / 6U) * 0x40U + (operation % 6U) * 8U;
        EXPECT_EQ(result->pcode[operation].opcode, sleigh_runtime::PcodeOpcode::copy);
        ASSERT_TRUE(result->pcode[operation].output.has_value());
        expect_varnode(*result->pcode[operation].output, "register", expected_offset, 8U);
        ASSERT_EQ(result->pcode[operation].inputs.size(), 1U);
        expect_varnode(result->pcode[operation].inputs[0], "const", 0U, 8U);
    }
    expect_materialized_pcode(*result);
}

/// Verifies the syscall system instruction and its architecture-specific CALLOTHER emission.
TEST(SleighRuntime, DecodesSyscall) {
    auto decoder = make_decoder();
    // 0f 05 - SYSCALL
    const std::array<std::uint8_t, 2> bytes{0x0f, 0x05};
    const auto result = decoder.decode(0x140100000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 2U);
    EXPECT_EQ(result->mnemonic, "SYSCALL");
    EXPECT_TRUE(result->operands.empty());
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);
    ASSERT_EQ(result->pcode.size(), 3U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::copy);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0x8U, 8U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "const", 0x140100002ULL, 8U);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::copy);
    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "register", 0x98U, 8U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 1U);
    expect_varnode(result->pcode[1].inputs[0], "register", 0x280U, 8U);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::call_other);
    ASSERT_EQ(result->pcode[2].inputs.size(), 1U);
    expect_varnode(result->pcode[2].inputs[0], "const", 5U, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies a privileged register-exchange instruction represented only by CALLOTHER.
TEST(SleighRuntime, DecodesSwapgs) {
    auto decoder = make_decoder();
    // 0f 01 f8 - SWAPGS
    const std::array<std::uint8_t, 3> bytes{0x0f, 0x01, 0xf8};
    const auto result = decoder.decode(0x140100000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 3U);
    EXPECT_EQ(result->mnemonic, "SWAPGS");
    EXPECT_TRUE(result->operands.empty());
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::call_other);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "const", 7U, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies RDRAND emits both the value-producing and status-producing CALLOTHER operations.
TEST(SleighRuntime, DecodesRdrand) {
    auto decoder = make_decoder();
    // 0f c7 f0 - RDRAND EAX
    const std::array<std::uint8_t, 3> bytes{0x0f, 0xc7, 0xf0};
    const auto result = decoder.decode(0x140100000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 3U);
    EXPECT_EQ(result->mnemonic, "RDRAND");
    ASSERT_EQ(result->operands.size(), 1U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    ASSERT_EQ(result->pcode.size(), 7U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::call_other);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0U, 4U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 1U);
    expect_varnode(result->pcode[0].inputs[0], "const", 0x680U, 4U);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::call_other);
    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "register", 0x200U, 1U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 1U);
    expect_varnode(result->pcode[1].inputs[0], "const", 0x681U, 4U);
    constexpr std::array<std::uint64_t, 5> cleared_flags{0x20bU, 0x207U, 0x206U, 0x204U, 0x202U};
    for (std::size_t operation = 0; operation < cleared_flags.size(); ++operation) {
        const auto& pcode = result->pcode[operation + 2];
        EXPECT_EQ(pcode.opcode, sleigh_runtime::PcodeOpcode::copy);
        ASSERT_TRUE(pcode.output.has_value());
        expect_varnode(*pcode.output, "register", cleared_flags[operation], 1U);
        ASSERT_EQ(pcode.inputs.size(), 1U);
        expect_varnode(pcode.inputs[0], "const", 0U, 1U);
    }
    expect_materialized_pcode(*result);
}

/// Verifies LOCK CMPXCHG combines CALLOTHER, memory access, flags, and conditional flow.
TEST(SleighRuntime, DecodesLockedCmpxchg) {
    auto decoder = make_decoder();
    // f0 48 0f b1 0b - CMPXCHG.LOCK qword ptr [RBX],RCX
    const std::array<std::uint8_t, 5> bytes{0xf0, 0x48, 0x0f, 0xb1, 0x0b};
    const auto result = decoder.decode(0x140100000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 5U);
    EXPECT_EQ(result->mnemonic, "CMPXCHG.LOCK");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::memory);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::conditional_branch);
    ASSERT_EQ(result->pcode.size(), 18U);
    constexpr std::array<sleigh_runtime::PcodeOpcode, 18> opcodes{
        sleigh_runtime::PcodeOpcode::call_other,  sleigh_runtime::PcodeOpcode::load,
        sleigh_runtime::PcodeOpcode::copy,        sleigh_runtime::PcodeOpcode::int_less,
        sleigh_runtime::PcodeOpcode::int_sborrow, sleigh_runtime::PcodeOpcode::int_sub,
        sleigh_runtime::PcodeOpcode::int_sless,   sleigh_runtime::PcodeOpcode::int_equal,
        sleigh_runtime::PcodeOpcode::int_and,     sleigh_runtime::PcodeOpcode::popcount,
        sleigh_runtime::PcodeOpcode::int_and,     sleigh_runtime::PcodeOpcode::int_equal,
        sleigh_runtime::PcodeOpcode::cbranch,     sleigh_runtime::PcodeOpcode::copy,
        sleigh_runtime::PcodeOpcode::branch,      sleigh_runtime::PcodeOpcode::copy,
        sleigh_runtime::PcodeOpcode::store,       sleigh_runtime::PcodeOpcode::call_other};
    for (std::size_t operation = 0; operation < opcodes.size(); ++operation) {
        EXPECT_EQ(result->pcode[operation].opcode, opcodes[operation]);
    }
    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "unique", 0xd500U, 8U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 2U);
    EXPECT_EQ(result->pcode[1].inputs[0].space, "const");
    EXPECT_NE(result->pcode[1].inputs[0].offset, 0U);
    expect_varnode(result->pcode[1].inputs[1], "register", 0x18U, 8U);
    ASSERT_TRUE(result->pcode[5].output.has_value());
    expect_varnode(*result->pcode[5].output, "unique", 0x198700U, 8U);
    expect_materialized_pcode(*result);
}

/// Verifies FS segment addressing remains materialized as a segment-base plus displacement.
TEST(SleighRuntime, DecodesFsSegmentLoad) {
    auto decoder = make_decoder();
    // 64 48 8b 04 25 60 00 00 00 - MOV RAX,qword ptr FS:[0x60]
    const std::array<std::uint8_t, 9> bytes{0x64, 0x48, 0x8b, 0x04, 0x25, 0x60, 0x00, 0x00, 0x00};
    const auto result = decoder.decode(0x140100000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 9U);
    EXPECT_EQ(result->mnemonic, "MOV");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::memory);
    ASSERT_EQ(result->pcode.size(), 3U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_add);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "unique", 0xc900U, 8U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x110U, 8U);
    expect_varnode(result->pcode[0].inputs[1], "const", 0x60U, 8U);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::load);
    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "unique", 0x23e00U, 8U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 2U);
    EXPECT_EQ(result->pcode[1].inputs[0].space, "const");
    EXPECT_NE(result->pcode[1].inputs[0].offset, 0U);
    expect_varnode(result->pcode[1].inputs[1], "unique", 0xc900U, 8U);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::copy);
    ASSERT_TRUE(result->pcode[2].output.has_value());
    expect_varnode(*result->pcode[2].output, "register", 0U, 8U);
    ASSERT_EQ(result->pcode[2].inputs.size(), 1U);
    expect_varnode(result->pcode[2].inputs[0], "unique", 0x23e00U, 8U);
    expect_materialized_pcode(*result);
}

/// Verifies a legal CET landing-pad instruction whose compiled semantics intentionally emit no p-code.
TEST(SleighRuntime, DecodesEndbr64WithoutPcode) {
    auto decoder = make_decoder();
    // f3 0f 1e fa - ENDBR64
    const std::array<std::uint8_t, 4> bytes{0xf3, 0x0f, 0x1e, 0xfa};
    const auto result = decoder.decode(0x140100000ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "ENDBR64");
    EXPECT_TRUE(result->operands.empty());
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::none);
    EXPECT_TRUE(result->pcode.empty());
}

/// Verifies ARM indirect return flow through BX LR and its condition normalization.
TEST(SleighRuntime, DecodesArmBxLr) {
    sleigh_runtime::Decoder decoder(std::filesystem::path(SLEIGH_RUNTIME_TEST_DATA_DIR) / "ARM8_le.sla");
    // 1e ff 2f e1 - BX LR
    const std::array<std::uint8_t, 4> bytes{0x1e, 0xff, 0x2f, 0xe1};
    const auto result = decoder.decode(0x400000ULL, bytes, {});

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "bx");
    ASSERT_EQ(result->pcode.size(), 5U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_and);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "unique", 0U, 4U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    expect_varnode(result->pcode[0].inputs[0], "register", 0x58U, 4U);
    expect_varnode(result->pcode[0].inputs[1], "const", 1U, 4U);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::int_not_equal);
    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "register", 0x69U, 1U);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::call_other);
    EXPECT_EQ(result->pcode[3].opcode, sleigh_runtime::PcodeOpcode::int_and);
    EXPECT_EQ(result->pcode[4].opcode, sleigh_runtime::PcodeOpcode::return_op);
    ASSERT_EQ(result->pcode[4].inputs.size(), 1U);
    expect_varnode(result->pcode[4].inputs[0], "register", 0x5cU, 4U);
    EXPECT_EQ(result->flow.kind, sleigh_runtime::FlowKind::return_op);
    ASSERT_TRUE(result->flow.target.has_value());
    expect_varnode(*result->flow.target, "register", 0x5cU, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies ARM supervisor-call dispatch through a system CALLOTHER operation.
TEST(SleighRuntime, DecodesArmSvc) {
    sleigh_runtime::Decoder decoder(std::filesystem::path(SLEIGH_RUNTIME_TEST_DATA_DIR) / "ARM8_le.sla");
    // 00 00 00 ef - SVC 0
    const std::array<std::uint8_t, 4> bytes{0x00, 0x00, 0x00, 0xef};
    const auto result = decoder.decode(0x400000ULL, bytes, {});

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "swi");
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::call_other);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    expect_varnode(result->pcode[0].inputs[0], "const", 0xfU, 4U);
    expect_varnode(result->pcode[0].inputs[1], "const", 0U, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies ARM PC-relative literal loading and the concrete instruction address calculation.
TEST(SleighRuntime, DecodesArmPcRelativeLoad) {
    sleigh_runtime::Decoder decoder(std::filesystem::path(SLEIGH_RUNTIME_TEST_DATA_DIR) / "ARM8_le.sla");
    // 00 00 9f e5 - LDR R0,[PC,#0]
    const std::array<std::uint8_t, 4> bytes{0x00, 0x00, 0x9f, 0xe5};
    const auto result = decoder.decode(0x400000ULL, bytes, {});

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "ldr");
    ASSERT_EQ(result->pcode.size(), 1U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::load);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0x20U, 4U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    EXPECT_EQ(result->pcode[0].inputs[0].space, "const");
    EXPECT_NE(result->pcode[0].inputs[0].offset, 0U);
    EXPECT_EQ(result->pcode[0].inputs[0].size, sizeof(void*));
    expect_varnode(result->pcode[0].inputs[1], "const", 0x400008U, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies ARM multi-register stack save and writeback as repeated concrete stores.
TEST(SleighRuntime, DecodesArmPushMultiple) {
    sleigh_runtime::Decoder decoder(std::filesystem::path(SLEIGH_RUNTIME_TEST_DATA_DIR) / "ARM8_le.sla");
    // 10 40 2d e9 - PUSH {R4,LR}
    const std::array<std::uint8_t, 4> bytes{0x10, 0x40, 0x2d, 0xe9};
    const auto result = decoder.decode(0x400000ULL, bytes, {});

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 4U);
    EXPECT_EQ(result->mnemonic, "stmdb");
    ASSERT_EQ(result->pcode.size(), 6U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_sub);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::store);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::int_sub);
    EXPECT_EQ(result->pcode[3].opcode, sleigh_runtime::PcodeOpcode::store);
    EXPECT_EQ(result->pcode[4].opcode, sleigh_runtime::PcodeOpcode::int_sub);
    EXPECT_EQ(result->pcode[5].opcode, sleigh_runtime::PcodeOpcode::int_add);
    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "register", 0x80U, 4U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 3U);
    EXPECT_EQ(result->pcode[1].inputs[0].space, "const");
    EXPECT_NE(result->pcode[1].inputs[0].offset, 0U);
    expect_varnode(result->pcode[1].inputs[1], "register", 0x80U, 4U);
    expect_varnode(result->pcode[1].inputs[2], "register", 0x58U, 4U);
    ASSERT_TRUE(result->pcode[5].output.has_value());
    expect_varnode(*result->pcode[5].output, "register", 0x54U, 4U);
    expect_materialized_pcode(*result);
}

/// Verifies the full XMM6 load from the local stack slot and all four lane copies.
TEST(SleighRuntime, DecodesMovapsXmm6LocalLoad) {
    auto decoder = make_decoder();
    // 0f 28 74 24 30 - MOVAPS XMM6,xmmword ptr [RSP + local_18[0]]
    const std::array<std::uint8_t, 5> bytes{0x0f, 0x28, 0x74, 0x24, 0x30};
    const auto result = decoder.decode(0x1403332a0ULL, bytes, x86_64_context());

    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->length, 5U);
    EXPECT_EQ(result->mnemonic, "MOVAPS");
    ASSERT_EQ(result->operands.size(), 2U);
    EXPECT_EQ(result->operands[0].kind, sleigh_runtime::OperandKind::register_value);
    EXPECT_EQ(result->operands[1].kind, sleigh_runtime::OperandKind::memory);
    EXPECT_NE(result->operands[1].text.find('['), std::string::npos);
    ASSERT_EQ(result->pcode.size(), 7U);
    EXPECT_EQ(result->pcode[0].opcode, sleigh_runtime::PcodeOpcode::int_add);
    EXPECT_EQ(result->pcode[1].opcode, sleigh_runtime::PcodeOpcode::load);
    EXPECT_EQ(result->pcode[2].opcode, sleigh_runtime::PcodeOpcode::copy);
    EXPECT_EQ(result->pcode[3].opcode, sleigh_runtime::PcodeOpcode::copy);
    EXPECT_EQ(result->pcode[4].opcode, sleigh_runtime::PcodeOpcode::copy);
    EXPECT_EQ(result->pcode[5].opcode, sleigh_runtime::PcodeOpcode::copy);
    EXPECT_EQ(result->pcode[6].opcode, sleigh_runtime::PcodeOpcode::copy);

    ASSERT_TRUE(result->pcode[0].output.has_value());
    expect_varnode(*result->pcode[0].output, "unique", 0x9d00U, 8U);
    ASSERT_EQ(result->pcode[0].inputs.size(), 2U);
    expect_varnode(result->pcode[0].inputs[0], "const", 48U, 8U);
    expect_varnode(result->pcode[0].inputs[1], "register", 0x20U, 8U);

    ASSERT_TRUE(result->pcode[1].output.has_value());
    expect_varnode(*result->pcode[1].output, "unique", 0xd700U, 16U);
    ASSERT_EQ(result->pcode[1].inputs.size(), 2U);
    EXPECT_EQ(result->pcode[1].inputs[0].space, "const");
    EXPECT_NE(result->pcode[1].inputs[0].offset, 0U);
    EXPECT_EQ(result->pcode[1].inputs[0].size, sizeof(void*));
    expect_varnode(result->pcode[1].inputs[1], "unique", 0x9d00U, 8U);

    ASSERT_TRUE(result->pcode[2].output.has_value());
    expect_varnode(*result->pcode[2].output, "unique", 0x10e300U, 16U);
    ASSERT_EQ(result->pcode[2].inputs.size(), 1U);
    expect_varnode(result->pcode[2].inputs[0], "unique", 0xd700U, 16U);
    for (std::size_t operation = 3; operation < result->pcode.size(); ++operation) {
        ASSERT_TRUE(result->pcode[operation].output.has_value());
        expect_varnode(*result->pcode[operation].output, "register", result->pcode[operation].output->offset, 4U);
        ASSERT_EQ(result->pcode[operation].inputs.size(), 1U);
        expect_varnode(result->pcode[operation].inputs[0], "unique", 0x10e300U + (operation - 3U) * 4U, 4U);
    }
    expect_materialized_pcode(*result);
}

/// Verifies that the runtime reports malformed or unsupported instruction bytes.
TEST(SleighRuntime, RejectsInvalidInstruction) {
    auto decoder = make_decoder();
    // Empty bytes - malformed instruction input, which cannot match a constructor.
    const std::array<std::uint8_t, 0> bytes{};
    const auto result = decoder.decode(0x140000300ULL, bytes, x86_64_context());

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message.empty());
}

/// Verifies the decoder rejects a real instruction prefix when the supplied bytes are truncated.
TEST(SleighRuntime, RejectsTruncatedInstruction) {
    auto decoder = make_decoder();
    // 48 8b - incomplete MOV r64,r/m64 encoding, missing ModRM.
    const std::array<std::uint8_t, 2> bytes{0x48, 0x8b};
    const auto result = decoder.decode(0x140000350ULL, bytes, x86_64_context());

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message.empty());
}

/// Verifies SLA loading fails before decoding when the processor file is absent.
TEST(SleighRuntime, RejectsMissingSlaFile) {
    EXPECT_THROW(sleigh_runtime::Decoder("missing-runtime-spec.sla"), std::runtime_error);
}

/// Verifies the public byte-window guard prevents silently truncating oversized input.
TEST(SleighRuntime, RejectsOversizedInstructionInput) {
    auto decoder = make_decoder();
    const std::array<std::uint8_t, 17> bytes{};
    const auto result = decoder.decode(0x140000400ULL, bytes, x86_64_context());

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().message.find("16 bytes"), std::string::npos);
}

/// Verifies that a moved-from owning decoder reports an expected error instead of dereferencing a null implementation.
TEST(SleighRuntime, RejectsDecodeAfterMove) {
    auto moved_from = make_decoder();
    auto decoder = std::move(moved_from);
    const std::array<std::uint8_t, 3> bytes{0x48, 0x8b, 0xd9};

    const auto result = moved_from.decode(0x140000450ULL, bytes, x86_64_context());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "Cannot decode with a moved-from Decoder");
}

/// Verifies context is rebuilt per decode, so 64-bit and legacy x86 modes cannot leak state.
TEST(SleighRuntime, AppliesProcessorContextPerDecode) {
    auto decoder = make_decoder();
    // 48 8b d9 - MOV RBX,RCX in 64-bit mode.
    const std::array<std::uint8_t, 3> bytes{0x48, 0x8b, 0xd9};
    const auto long_mode = decoder.decode(0x140000500ULL, bytes, x86_64_context());
    const sleigh_runtime::ProcessorContext legacy_mode{
        {{"addrsize", 1}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 0}}};
    const auto legacy = decoder.decode(0x140000500ULL, bytes, legacy_mode);

    ASSERT_TRUE(long_mode.has_value()) << long_mode.error().message;
    ASSERT_TRUE(legacy.has_value()) << legacy.error().message;
    EXPECT_EQ(long_mode->mnemonic, "MOV");
    EXPECT_EQ(long_mode->length, 3U);
    EXPECT_EQ(legacy->mnemonic, "DEC");
    EXPECT_EQ(legacy->length, 1U);
}

/// Checks representative instruction families and exact opcode order from the reference vectors.
TEST(SleighRuntime, DecodesReferenceInstructionFamilies) {
    struct Vector {
        std::vector<std::uint8_t> bytes;
        const char* mnemonic;
        std::size_t length;
        std::size_t operand_count;
        std::vector<sleigh_runtime::PcodeOpcode> opcodes;
        sleigh_runtime::FlowKind flow;
    };

    const std::vector<Vector> vectors{
        // 57 - PUSH RDI
        {{0x57},
         "PUSH",
         1,
         1,
         {sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::int_sub, sleigh_runtime::PcodeOpcode::store},
         sleigh_runtime::FlowKind::none},
        // 0f 29 74 24 30 - MOVAPS [RSP+0x30],XMM6
        {{0x0f, 0x29, 0x74, 0x24, 0x30},
         "MOVAPS",
         5,
         2,
         {sleigh_runtime::PcodeOpcode::int_add, sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::store},
         sleigh_runtime::FlowKind::none},
        // 8b fa - MOV EDI,EDX
        {{0x8b, 0xfa},
         "MOV",
         2,
         2,
         {sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::int_zext},
         sleigh_runtime::FlowKind::none},
        // 0f 28 f8 - MOVAPS XMM7,XMM0
        {{0x0f, 0x28, 0xf8},
         "MOVAPS",
         3,
         2,
         {sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::copy,
          sleigh_runtime::PcodeOpcode::copy},
         sleigh_runtime::FlowKind::none},
        // e8 c6 84 ff ff - CALL FUN_14032b6f4
        {{0xe8, 0xc6, 0x84, 0xff, 0xff},
         "CALL",
         5,
         1,
         {sleigh_runtime::PcodeOpcode::int_sub, sleigh_runtime::PcodeOpcode::store, sleigh_runtime::PcodeOpcode::call},
         sleigh_runtime::FlowKind::call},
        // 85 ff - TEST EDI,EDI
        {{0x85, 0xff},
         "TEST",
         2,
         2,
         {sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::int_sless, sleigh_runtime::PcodeOpcode::int_equal,
          sleigh_runtime::PcodeOpcode::int_and, sleigh_runtime::PcodeOpcode::popcount,
          sleigh_runtime::PcodeOpcode::int_and, sleigh_runtime::PcodeOpcode::int_equal},
         sleigh_runtime::FlowKind::none},
        // 74 24 - JZ LAB_140333264
        {{0x74, 0x24},
         "JZ",
         2,
         1,
         {sleigh_runtime::PcodeOpcode::cbranch},
         sleigh_runtime::FlowKind::conditional_branch},
        // f3 0f 5c f7 - SUBSS XMM6,XMM7
        {{0xf3, 0x0f, 0x5c, 0xf7},
         "SUBSS",
         4,
         2,
         {sleigh_runtime::PcodeOpcode::float_sub},
         sleigh_runtime::FlowKind::none},
        // 40 0f b6 c7 - MOVZX EAX,DIL
        {{0x40, 0x0f, 0xb6, 0xc7},
         "MOVZX",
         4,
         2,
         {sleigh_runtime::PcodeOpcode::int_zext, sleigh_runtime::PcodeOpcode::int_zext},
         sleigh_runtime::FlowKind::none},
        // 66 0f 6e c8 - MOVD XMM1,EAX
        {{0x66, 0x0f, 0x6e, 0xc8},
         "MOVD",
         4,
         2,
         {sleigh_runtime::PcodeOpcode::int_zext},
         sleigh_runtime::FlowKind::none},
        // 0f 5b c9 - CVTDQ2PS XMM1,XMM1
        {{0x0f, 0x5b, 0xc9},
         "CVTDQ2PS",
         3,
         2,
         {sleigh_runtime::PcodeOpcode::float_int_to_float, sleigh_runtime::PcodeOpcode::float_int_to_float,
          sleigh_runtime::PcodeOpcode::float_int_to_float, sleigh_runtime::PcodeOpcode::float_int_to_float},
         sleigh_runtime::FlowKind::none},
        // f3 0f 59 f1 - MULSS XMM6,XMM1
        {{0xf3, 0x0f, 0x59, 0xf1},
         "MULSS",
         4,
         2,
         {sleigh_runtime::PcodeOpcode::float_mult},
         sleigh_runtime::FlowKind::none},
        // f3 0f 58 f7 - ADDSS XMM6,XMM7
        {{0xf3, 0x0f, 0x58, 0xf7},
         "ADDSS",
         4,
         2,
         {sleigh_runtime::PcodeOpcode::float_add},
         sleigh_runtime::FlowKind::none},
        // eb 20 - JMP LAB_140333284
        {{0xeb, 0x20}, "JMP", 2, 1, {sleigh_runtime::PcodeOpcode::branch}, sleigh_runtime::FlowKind::branch},
        // ff d6 - CALL RSI
        {{0xff, 0xd6},
         "CALL",
         2,
         1,
         {sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::int_sub, sleigh_runtime::PcodeOpcode::store,
          sleigh_runtime::PcodeOpcode::call_ind},
         sleigh_runtime::FlowKind::indirect_call},
        // 48 03 fd - ADD RDI,RBP
        {{0x48, 0x03, 0xfd},
         "ADD",
         3,
         2,
         {sleigh_runtime::PcodeOpcode::int_carry, sleigh_runtime::PcodeOpcode::int_scarry,
          sleigh_runtime::PcodeOpcode::int_add, sleigh_runtime::PcodeOpcode::int_sless,
          sleigh_runtime::PcodeOpcode::int_equal, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::popcount, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::int_equal},
         sleigh_runtime::FlowKind::none},
        // ff cb - DEC EBX
        {{0xff, 0xcb},
         "DEC",
         2,
         1,
         {sleigh_runtime::PcodeOpcode::int_sborrow, sleigh_runtime::PcodeOpcode::int_sub,
          sleigh_runtime::PcodeOpcode::int_zext, sleigh_runtime::PcodeOpcode::int_sless,
          sleigh_runtime::PcodeOpcode::int_equal, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::popcount, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::int_equal},
         sleigh_runtime::FlowKind::none},
        // 79 f4 - JNS LAB_140001532
        {{0x79, 0xf4},
         "JNS",
         2,
         1,
         {sleigh_runtime::PcodeOpcode::bool_negate, sleigh_runtime::PcodeOpcode::cbranch},
         sleigh_runtime::FlowKind::conditional_branch},
        // 0f 28 74 24 30 - MOVAPS XMM6,[RSP+0x30]
        {{0x0f, 0x28, 0x74, 0x24, 0x30},
         "MOVAPS",
         5,
         2,
         {sleigh_runtime::PcodeOpcode::int_add, sleigh_runtime::PcodeOpcode::load, sleigh_runtime::PcodeOpcode::copy,
          sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::copy,
          sleigh_runtime::PcodeOpcode::copy},
         sleigh_runtime::FlowKind::none},
        // 48 83 c4 40 - ADD RSP,0x40
        {{0x48, 0x83, 0xc4, 0x40},
         "ADD",
         4,
         2,
         {sleigh_runtime::PcodeOpcode::int_carry, sleigh_runtime::PcodeOpcode::int_scarry,
          sleigh_runtime::PcodeOpcode::int_add, sleigh_runtime::PcodeOpcode::int_sless,
          sleigh_runtime::PcodeOpcode::int_equal, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::popcount, sleigh_runtime::PcodeOpcode::int_and,
          sleigh_runtime::PcodeOpcode::int_equal},
         sleigh_runtime::FlowKind::none},
        // 5f - POP RDI
        {{0x5f},
         "POP",
         1,
         1,
         {sleigh_runtime::PcodeOpcode::copy, sleigh_runtime::PcodeOpcode::load, sleigh_runtime::PcodeOpcode::int_add,
          sleigh_runtime::PcodeOpcode::copy},
         sleigh_runtime::FlowKind::none},
        // c3 - RET
        {{0xc3},
         "RET",
         1,
         0,
         {sleigh_runtime::PcodeOpcode::load, sleigh_runtime::PcodeOpcode::int_add,
          sleigh_runtime::PcodeOpcode::return_op},
         sleigh_runtime::FlowKind::return_op},
    };

    auto decoder = make_decoder();
    const auto context = x86_64_context();
    for (std::size_t index = 0; index < vectors.size(); ++index) {
        SCOPED_TRACE(index);
        const auto& vector = vectors[index];
        const auto result = decoder.decode(0x140010000ULL + index * 0x100U, vector.bytes, context);
        ASSERT_TRUE(result.has_value()) << result.error().message;
        EXPECT_EQ(result->mnemonic, vector.mnemonic);
        EXPECT_EQ(result->length, vector.length);
        EXPECT_EQ(result->operands.size(), vector.operand_count);
        ASSERT_EQ(result->pcode.size(), vector.opcodes.size());
        for (std::size_t operation = 0; operation < vector.opcodes.size(); ++operation) {
            EXPECT_EQ(result->pcode[operation].opcode, vector.opcodes[operation]);
        }
        EXPECT_EQ(result->flow.kind, vector.flow);
        if (vector.flow != sleigh_runtime::FlowKind::none) {
            EXPECT_TRUE(result->flow.target.has_value());
        }
        expect_materialized_pcode(*result);
    }
}

} // namespace
