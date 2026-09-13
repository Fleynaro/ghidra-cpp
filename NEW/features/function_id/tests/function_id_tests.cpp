#include <gtest/gtest.h>

import std;
import function_id;
import sleigh_runtime;

namespace {

/// Opens an original packed database and turns a parse failure into a test failure.
[[nodiscard]] fid::Database open_database(std::string_view name) {
    auto result = fid::Database::open(std::filesystem::path(FUNCTION_ID_TEST_DIR) / name);
    if (!result) {
        ADD_FAILURE() << result.error().message;
        throw std::runtime_error(result.error().message);
    }
    return std::move(*result);
}

/// Converts whitespace-separated hexadecimal bytes into the Sleigh decoder input format.
[[nodiscard]] std::vector<fid::Byte> bytes(std::string_view text) {
    std::vector<fid::Byte> result;
    std::istringstream input{std::string(text)};
    unsigned value = 0;
    while (input >> std::hex >> value)
        result.push_back(static_cast<fid::Byte>(value));
    return result;
}

/// Decodes a contiguous function extent through Sleigh, preserving each constructor result.
[[nodiscard]] std::vector<sleigh_runtime::Instruction>
decode_function(sleigh_runtime::Decoder& decoder, std::span<const fid::Byte> function, std::uint64_t address = 0) {
    std::vector<sleigh_runtime::Instruction> result;
    const sleigh_runtime::ProcessorContext context{{
        sleigh_runtime::ContextValue{"longMode", 1},
        sleigh_runtime::ContextValue{"addrsize", 2},
        sleigh_runtime::ContextValue{"bit64", 1},
        sleigh_runtime::ContextValue{"opsize", 1},
        sleigh_runtime::ContextValue{"protectedMode", 1},
    }};
    std::size_t offset = 0;
    while (offset < function.size()) {
        auto decoded = decoder.decode(
            address + offset, function.subspan(offset, std::min<std::size_t>(16, function.size() - offset)), context);
        if (!decoded)
            throw std::runtime_error(decoded.error().message);
        if (decoded->length == 0 || decoded->length > function.size() - offset) {
            throw std::runtime_error("Sleigh returned an invalid instruction length");
        }
        offset += decoded->length;
        result.push_back(std::move(*decoded));
    }
    return result;
}

/// Verifies every checked-in original `.fidb` file is read directly by the native storage layer.
TEST(FunctionIdDatabase, OpensOriginalPackedDatabases) {
    for (const auto name : {"vs2012_x64.fidb", "vs2015_x64.fidb", "vs2017_x64.fidb", "vsOlder_x64.fidb"}) {
        auto database = open_database(name);
        EXPECT_GT(database.buffer_size(), 0U);
        EXPECT_FALSE(database.libraries().empty());
        EXPECT_FALSE(database.functions().empty());
    }
}

/// Verifies the hasher rejects a function whose effective extent is below the FID minimum.
TEST(FunctionIdHash, RejectsShortExtent) {
    std::vector<fid::Instruction> instructions(3);
    for (auto& instruction : instructions) {
        instruction.bytes = {0x90};
        instruction.instruction_mask = {0xff};
    }
    const auto result = fid::Hasher::hash(instructions);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, fid::ErrorCode::invalid_input);
}

/// Verifies FNV hashing of explicit masks and register/scalar objects without a processor dependency.
TEST(FunctionIdHash, HashesAbstractInstructionMetadata) {
    std::vector<fid::Instruction> instructions(4);
    for (auto& instruction : instructions) {
        instruction.bytes = {0x33, 0xc0};
        instruction.instruction_mask = {0xff, 0xc0};
        instruction.operand_masks = {{0xff, 0xff}, {0xff, 0xff}};
        instruction.operands = {
            {{fid::OperandObject{fid::OperandObjectKind::register_value, 0, true, false, false}}},
            {{fid::OperandObject{fid::OperandObjectKind::register_value, 0, true, false, false}}},
        };
    }
    const auto result = fid::Hasher::hash(instructions);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->code_unit_size, 4);
    EXPECT_EQ(result->specific_hash_additional_size, 0);
}

/// Verifies relocations preserve the full digest while replacing the specific scalar value.
TEST(FunctionIdHash, RelocationOnlyAffectsSpecificDigest) {
    std::vector<fid::Instruction> plain(4);
    for (auto& instruction : plain) {
        instruction.bytes = {0x83, 0xc0, 0x01};
        instruction.instruction_mask = {0xff, 0xc0, 0x00};
        instruction.operand_masks = {{0xff, 0xff, 0xff}};
        instruction.operands = {{{fid::OperandObject{fid::OperandObjectKind::scalar, 1, true, false, false}}}};
    }
    auto relocated = plain;
    relocated.front().operands.front().front().relocated = true;
    const auto normal_hash = fid::Hasher::hash(plain);
    const auto relocated_hash = fid::Hasher::hash(relocated);
    ASSERT_TRUE(normal_hash.has_value());
    ASSERT_TRUE(relocated_hash.has_value());
    EXPECT_EQ(normal_hash->full_hash, relocated_hash->full_hash);
    EXPECT_NE(normal_hash->specific_hash, relocated_hash->specific_hash);
}

/// Verifies the original x86 skipper removes recognized NOP code units from the score and digest.
TEST(FunctionIdHash, SkipsX86NopInstruction) {
    std::vector<fid::Instruction> instructions(5);
    for (auto& instruction : instructions) {
        instruction.bytes = {0x33, 0xc0};
        instruction.instruction_mask = {0xff, 0xc0};
    }
    instructions[2].bytes = {0x90};
    instructions[2].instruction_mask = {0xff};
    instructions[2].skip = true;
    const auto result = fid::Hasher::hash(instructions);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->code_unit_size, 4);
}

/// Verifies that architecture-specific x86 skipping is not applied to generic instruction bytes.
TEST(FunctionIdHash, DoesNotSkipUnmarkedGenericBytes) {
    std::vector<fid::Instruction> instructions(4);
    for (auto& instruction : instructions) {
        instruction.bytes = {0x90};
        instruction.instruction_mask = {0xff};
    }
    const auto result = fid::Hasher::hash(instructions);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->code_unit_size, 4);
}

/// Verifies that a non-x86 Sleigh instruction retains byte-dependent hash semantics instead of a constant fallback.
TEST(FunctionIdHash, HashesNonX86SleighInstructionBytes) {
    sleigh_runtime::Decoder decoder("ARM8_le.sla");
    const std::array<std::uint8_t, 4> bx_lr{0x1e, 0xff, 0x2f, 0xe1};
    const std::array<std::uint8_t, 4> svc_zero{0x00, 0x00, 0x00, 0xef};
    const auto first = decoder.decode(0x400000ULL, bx_lr, {});
    const auto second = decoder.decode(0x400000ULL, svc_zero, {});
    ASSERT_TRUE(first.has_value()) << first.error().message;
    ASSERT_TRUE(second.has_value()) << second.error().message;
    const std::array<sleigh_runtime::Instruction, 4> first_extent{*first, *first, *first, *first};
    const std::array<sleigh_runtime::Instruction, 4> second_extent{*second, *second, *second, *second};
    const auto first_hash = fid::Hasher::hash_sleigh(first_extent);
    const auto second_hash = fid::Hasher::hash_sleigh(second_extent);
    ASSERT_TRUE(first_hash.has_value()) << first_hash.error().message;
    ASSERT_TRUE(second_hash.has_value()) << second_hash.error().message;
    EXPECT_NE(first_hash->full_hash, second_hash->full_hash);
}

/// Verifies that the public program context can distinguish unrestricted sources from no sources.
TEST(FunctionIdFilter, DistinguishesWildcardAndEmptySourceSets) {
    const fid::ProgramInfo unrestricted{std::nullopt, std::nullopt, std::nullopt, false};
    const fid::ProgramInfo no_sources{std::nullopt, std::nullopt, std::set<std::string>{}, false};
    EXPECT_FALSE(unrestricted.source_languages.has_value());
    ASSERT_TRUE(no_sources.source_languages.has_value());
    EXPECT_TRUE(no_sources.source_languages->empty());
}

/// Verifies the first real acceptance extent through Sleigh and the original VS2012 database.
TEST(FunctionIdAcceptance, SingleMatchSingleCompiler) {
    const auto function =
        bytes("40 53 48 83 ec 20 45 33 d2 4c 8b c9 48 85 c9 74 0e 48 85 d2 74 09 4d 85 c0 75 1d "
              "66 44 89 11 e8 c8 5c 00 00 bb 16 00 00 00 89 18 e8 2c ac 00 00 8b c3 48 83 c4 20 5b c3 "
              "66 44 39 11 74 09 48 83 c1 02 48 ff ca 75 f1 48 85 d2 75 06 66 45 89 11 eb cd 49 2b c8 "
              "41 0f b7 00 66 42 89 04 01 4d 8d 40 02 66 85 c0 74 05 48 ff ca 75 e9 48 85 d2 75 10 "
              "66 45 89 11 e8 72 5c 00 00 bb 22 00 00 00 eb a8 33 c0 eb ad");
    sleigh_runtime::Decoder decoder("x86-64.sla");
    const auto instructions = decode_function(decoder, function);
    const auto hash = fid::Hasher::hash_sleigh(instructions);
    ASSERT_TRUE(hash.has_value()) << hash.error().message;
    EXPECT_EQ(hash->code_unit_size, 41);
    auto database = open_database("vs2012_x64.fidb");
    const auto matches = database.find_full_hash(hash->full_hash);
    ASSERT_EQ(matches.size(), 1U);
    EXPECT_EQ(matches.front().name, "wcscat_s");
}

/// Decodes one fixture and asserts that the original database lookup returns exactly the named functions.
void expect_fixture_matches(std::string_view hex, std::initializer_list<std::string_view> database_names,
                            std::initializer_list<std::string_view> expected_names) {
    const auto function = bytes(hex);
    sleigh_runtime::Decoder decoder("x86-64.sla");
    const auto instructions = decode_function(decoder, function);
    const auto hash = fid::Hasher::hash_sleigh(instructions);
    ASSERT_TRUE(hash.has_value()) << hash.error().message;
    std::vector<std::string> names;
    const fid::FunctionContext context{*hash, {}, {}};
    const fid::ProgramInfo program{std::string{"x86:LE:64:default"}, std::nullopt, std::nullopt, false};
    for (const auto database_name : database_names) {
        auto database = open_database(database_name);
        const auto result = database.identify(context, program, 0.0F);
        ASSERT_TRUE(result.has_value()) << result.error().message;
        names.insert(names.end(), result->names.begin(), result->names.end());
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    std::vector<std::string> expected;
    for (const auto name : expected_names)
        expected.emplace_back(name);
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(names, expected);
}

/// Verifies the second fixture matches ungetc across the two compiler database variants.
TEST(FunctionIdAcceptance, SingleMatchMultipleCompilers) {
    expect_fixture_matches("48 89 5c 24 08 48 89 54 24 10 57 48 83 ec 20 48 8b da 8b f9 33 c0 48 85 d2 0f 95 c0 "
                           "85 c0 75 15 e8 17 5b fe ff c7 00 16 00 00 00 e8 7c aa fe ff 83 c8 ff eb 1f 48 8b ca "
                           "e8 4f 98 fd ff 90 48 8b d3 8b cf e8 b0 fe ff ff 8b f8 48 8b cb e8 d6 98 fd ff 8b c7 "
                           "48 8b 5c 24 30 48 83 c4 20 5f c3",
                           {"vsOlder_x64.fidb", "vs2015_x64.fidb"}, {"ungetc"});
}

/// Verifies the third fixture matches malloc while accepting both listed compiler variants.
TEST(FunctionIdAcceptance, LibraryFunctionWithMultipleCompilers) {
    expect_fixture_matches("48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 20 48 8b d9 48 83 f9 e0 77 7c bf 01 00 00 00 "
                           "48 85 c9 48 0f 45 f9 48 8b 0d f5 93 5d 01 48 85 c9 75 20 e8 e3 54 00 00 b9 1e 00 00 00 "
                           "e8 4d 55 00 00 b9 ff 00 00 00 e8 b3 1d 00 00 48 8b 0d d0 93 5d 01 4c 8b c7 33 d2 ff 15 "
                           "e5 6d 07 00 48 8b f0 48 85 c0 75 2c 39 05 ef 9e 5d 01 74 0e 48 8b cb e8 a5 f5 00 00 "
                           "85 c0 74 0d eb ab e8 9e 1a 00 00 c7 00 0c 00 00 00 e8 93 1a 00 00 c7 00 0c 00 00 00 "
                           "48 8b c6 eb 12 e8 7f f5 00 00 e8 7e 1a 00 00 c7 00 0c 00 00 00 33 c0 48 8b 5c 24 30 "
                           "48 8b 74 24 38 48 83 c4 20 5f c3",
                           {"vsOlder_x64.fidb", "vs2012_x64.fidb"}, {"malloc"});
}

/// Verifies the fourth fixture matches memcmp and exercises short branches, byte operands, and skips.
TEST(FunctionIdAcceptance, ByteAndBranchOperands) {
    expect_fixture_matches("48 2b d1 49 83 f8 08 72 22 f6 c1 07 74 14 66 90 8a 01 3a 04 0a 75 2c 48 ff c1 49 ff c8 "
                           "f6 c1 07 75 ee 4d 8b c8 49 c1 e9 03 75 1f 4d 85 c0 74 0f 8a 01 3a 04 0a 75 0c 48 ff c1 "
                           "49 ff c8 75 f1 48 33 c0 c3 1b c0 83 d8 ff c3 90 49 c1 e9 02 74 37 48 8b 01 48 3b 04 0a "
                           "75 5b 48 8b 41 08 48 3b 44 0a 08 75 4c 48 8b 41 10 48 3b 44 0a 10 75 3d 48 8b 41 18 "
                           "48 3b 44 0a 18 75 2e 48 83 c1 20 49 ff c9 75 cd 49 83 e0 1f 4d 8b c8 49 c1 e9 03 74 9b "
                           "48 8b 01 48 3b 04 0a 75 1b 48 83 c1 08 49 ff c9 75 ee 49 83 e0 07 eb 83 48 83 c1 08 "
                           "48 83 c1 08 48 83 c1 08 48 8b 0c 11 48 0f c8 48 0f c9 48 3b c1 1b c0 83 d8 ff c3",
                           {"vs2017_x64.fidb"}, {"memcmp"});
}

/// Verifies the fifth fixture preserves Ghidra's conflicting equal-score names.
TEST(FunctionIdAcceptance, ConflictingMatches) {
    expect_fixture_matches(
        "48 8b c4 48 89 48 08 48 89 50 10 4c 89 40 18 4c 89 48 20 53 57 48 83 ec 28 33 c0 48 85 c9 "
        "0f 95 c0 85 c0 75 15 e8 12 4b 00 00 c7 00 16 00 00 00 e8 77 9a 00 00 83 c8 ff eb 6a "
        "48 8d 7c 24 48 e8 40 88 ff ff 48 8d 50 30 b9 01 00 00 00 e8 a2 88 ff ff 90 e8 2c 88 ff ff "
        "48 8d 48 30 e8 17 23 01 00 8b d8 e8 1c 88 ff ff 48 8d 48 30 4c 8b cf 45 33 c0 48 8b 54 24 40 "
        "e8 98 d5 00 00 8b f8 e8 01 88 ff ff 48 8d 50 30 8b cb e8 b2 22 01 00 90 e8 f0 87 ff ff "
        "48 8d 50 30 b9 01 00 00 00 e8 d6 88 ff ff 8b c7 48 83 c4 28 5f 5b c3",
        {"vs2012_x64.fidb", "vs2015_x64.fidb", "vs2017_x64.fidb", "vsOlder_x64.fidb"}, {"printf", "wprintf"});
}

} // namespace
