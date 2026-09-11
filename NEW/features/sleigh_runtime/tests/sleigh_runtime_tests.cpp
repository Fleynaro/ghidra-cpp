#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
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
