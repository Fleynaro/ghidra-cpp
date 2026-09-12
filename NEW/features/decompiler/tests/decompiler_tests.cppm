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
    description.calling_convention = "__cdecl";
    description.spaces = {
        SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    description.registers = {
        RegisterDescription{"RAX", Storage{"register", 0, 8}},
        RegisterDescription{"RCX", Storage{"register", 8, 8}},
        RegisterDescription{"RDX", Storage{"register", 0x10, 8}},
        RegisterDescription{"RBX", Storage{"register", 0x18, 8}},
        RegisterDescription{"RSP", Storage{"register", 0x20, 8}},
        RegisterDescription{"R8", Storage{"register", 0x80, 8}},
        RegisterDescription{"R9", Storage{"register", 0x88, 8}},
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
        EXPECT_EQ(result.c_source, "\nxunknown8 __cdecl copy_one(void)\n\n{\n  return 7;\n}\n");
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
    /// Supplies the externally known name for the function entry.
    class TestSymbolProvider final : public SymbolProvider {
    public:
        /// Returns the documented function symbol at the test image entry.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address != 0x1000) {
                return std::nullopt;
            }
            return SymbolDescription{address, "StringLengthWorkerW", ""};
        }
    };

    /// Supplies the primitive and pointer types.
    class TestTypeProvider final : public TypeProvider {
    public:
        /// Resolves the requested Example 1 type without embedding it in the engine.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "wchar_t") {
                type.size = 2;
                type.kind = TypeKind::unicode_character;
            } else if (name == "wchar_t *" || name == "__uint64 *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = name == "wchar_t *" ? "wchar_t" : "__uint64";
            } else if (name == "__uint64") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "long") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else {
                return std::nullopt;
            }
            return type;
        }
    };

    /// Supplies the typed ABI storage for the parameters and result.
    class TestPrototypeProvider final : public PrototypeProvider {
    public:
        /// Returns the documented x86-64 register prototype at the function entry.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            if (address != 0x1000) {
                return std::nullopt;
            }
            PrototypeDescription prototype;
            prototype.calling_convention = "__cdecl";
            prototype.return_type = "long";
            prototype.return_storage = Storage{"register", 0, 4};
            prototype.parameters = {
                PrototypeParameterDescription{"param_1", "wchar_t *", Storage{"register", 8, 8}},
                PrototypeParameterDescription{"param_2", "__uint64", Storage{"register", 0x10, 8}},
                PrototypeParameterDescription{"param_3", "__uint64 *", Storage{"register", 0x80, 8}},
            };
            return prototype;
        }
    };

    /// Supplies the documented stack-local names and types.
    class TestVariableProvider final : public VariableProvider {
    public:
        /// Returns typed locals at their stack-frame storage locations.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x1000) {
                return {};
            }
            return {
                VariableDescription{"local_res8", "wchar_t *", Storage{"stack", 8, 8}},
                VariableDescription{"local_res10", "__uint64", Storage{"stack", 0x10, 8}},
                VariableDescription{"local_10", "int", Storage{"stack", static_cast<std::uint64_t>(-0x10), 4}},
                VariableDescription{"local_res18", "__uint64", Storage{"stack", static_cast<std::uint64_t>(-0x18), 8}},
            };
        }
    };

    // The bytes below are the complete contiguous function from Example 1.
    // Each group is one x86-64 instruction from the listing: the first three
    // save RCX/RDX/R8, SUB reserves the stack frame, and the following groups
    // implement the bounded UTF-16 scan, error assignment, optional output
    // write, epilogue, and RET. Branch targets are preserved by keeping the
    // original instruction order and relative branch bytes unchanged.
    const std::vector<std::uint8_t> function_bytes{
        0x4c, 0x89, 0x44, 0x24, 0x18,                   // MOV [RSP+0x18], R8: save param_3.
        0x48, 0x89, 0x54, 0x24, 0x10,                   // MOV [RSP+0x10], RDX: save param_2.
        0x48, 0x89, 0x4c, 0x24, 0x08,                   // MOV [RSP+0x08], RCX: save param_1.
        0x48, 0x83, 0xec, 0x18,                         // SUB RSP, 0x18: allocate locals.
        0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // MOV local_10, 0.
        0x48, 0x8b, 0x44, 0x24, 0x28,                   // MOV RAX, local_res10.
        0x48, 0x89, 0x04, 0x24,                         // MOV local_18, RAX: save original limit.
        0x48, 0x83, 0x7c, 0x24, 0x28, 0x00,             // CMP local_res10, 0.
        0x74, 0x2a,                                     // JZ LAB_1417515e6.
        0x48, 0x8b, 0x44, 0x24, 0x20,                   // MOV RAX, local_res8.
        0x0f, 0xb7, 0x00,                               // MOVZX EAX, word ptr [RAX]: read wchar_t.
        0x85, 0xc0,                                     // TEST EAX, EAX: test for L'\0'.
        0x74, 0x1e,                                     // JZ LAB_1417515e6.
        0x48, 0x8b, 0x44, 0x24, 0x20,                   // MOV RAX, local_res8.
        0x48, 0x83, 0xc0, 0x02,                         // ADD RAX, 2: advance UTF-16 pointer.
        0x48, 0x89, 0x44, 0x24, 0x20,                   // MOV local_res8, RAX.
        0x48, 0x8b, 0x44, 0x24, 0x28,                   // MOV RAX, local_res10.
        0x48, 0x83, 0xe8, 0x01,                         // SUB RAX, 1: decrement remaining count.
        0x48, 0x89, 0x44, 0x24, 0x28,                   // MOV local_res10, RAX.
        0xeb, 0xce,                                     // JMP LAB_1417515b4: loop back.
        0x48, 0x83, 0x7c, 0x24, 0x28, 0x00,             // CMP local_res10, 0.
        0x75, 0x08,                                     // JNZ LAB_1417515f6.
        0xc7, 0x44, 0x24, 0x08, 0x57, 0x00, 0x07, 0x80, // MOV local_10, 0x80070057.
        0x48, 0x83, 0x7c, 0x24, 0x30, 0x00,             // CMP param_3, 0.
        0x74, 0x29,                                     // JZ LAB_141751627: skip optional write.
        0x83, 0x7c, 0x24, 0x08, 0x00,                   // CMP local_10, 0.
        0x7c, 0x16,                                     // JL LAB_14175161b: failure path.
        0x48, 0x8b, 0x44, 0x24, 0x28,                   // MOV RAX, local_res10.
        0x48, 0x8b, 0x0c, 0x24,                         // MOV RCX, local_18.
        0x48, 0x2b, 0xc8,                               // SUB RCX, RAX: consumed count.
        0x48, 0x8b, 0x44, 0x24, 0x30,                   // MOV RAX, param_3.
        0x48, 0x89, 0x08,                               // MOV [RAX], RCX: store consumed count.
        0xeb, 0x0c,                                     // JMP LAB_141751627.
        0x48, 0x8b, 0x44, 0x24, 0x30,                   // MOV RAX, param_3: failure output pointer.
        0x48, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00,       // MOV [RAX], 0: clear output on failure.
        0x8b, 0x44, 0x24, 0x08,                         // MOV EAX, local_10: return status.
        0x48, 0x83, 0xc4, 0x18,                         // ADD RSP, 0x18: release locals.
        0xc3,                                           // RET.
    };

    // This is the current exact output of the real decompiler pipeline for
    // the bytes above. It is intentionally kept as a complete source string
    // so later decompiler improvements produce a deliberate test diff.
    const std::string expected_c = R"(
long __cdecl StringLengthWorkerW(wchar_t * param_1,__uint64 param_2,__uint64 * param_3)

{
  wchar_t * local_res8;
  __uint64 local_res10;
  int local_10;
  
  local_10 = 0;
  local_res10 = param_2;
  for (local_res8 = param_1; (local_res10 != 0 && (*local_res8 != L'\0'));
      local_res8 = local_res8 + 1) {
    local_res10 = local_res10 - 1;
  }
  if (local_res10 == 0) {
    local_10 = -0x7ff8ffa9;
  }
  if (param_3 != (__uint64 *)0x0) {
    if (local_10 < 0) {
      *param_3 = 0;
    }
    else {
      *param_3 = param_2 - local_res10;
    }
  }
  return local_10;
}
)";

    // Sleigh reads a maximum 16-byte instruction window. Padding is mapped
    // after RET for that read-ahead, but is deliberately excluded from the
    // FunctionDescription range so it cannot become part of the function.
    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Read-ahead NOP padding.
    auto memory = std::make_shared<SparseMemory>(0x1000, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    providers.symbols = std::make_shared<TestSymbolProvider>();
    providers.types = std::make_shared<TestTypeProvider>();
    providers.prototypes = std::make_shared<TestPrototypeProvider>();
    providers.variables = std::make_shared<TestVariableProvider>();
    Decompiler decompiler(test_architecture(), std::move(providers));
    const DecompilationResult result =
        decompiler.decompile(FunctionDescription{"StringLengthWorkerW", 0x1000, 0x1000 + function_bytes.size()});

    ASSERT_FALSE(result.raw_instructions.empty());
    EXPECT_EQ(result.raw_instructions.back().mnemonic, "RET");
    ASSERT_FALSE(result.c_source.empty());
    EXPECT_NE(result.c_source.find("long __cdecl StringLengthWorkerW"), std::string::npos);
    EXPECT_NE(result.c_source.find("wchar_t * param_1"), std::string::npos);
    EXPECT_NE(result.c_source.find("__uint64 param_2"), std::string::npos);
    EXPECT_NE(result.c_source.find("__uint64 * param_3"), std::string::npos);
    EXPECT_NE(result.c_source.find("int local_10"), std::string::npos);
    EXPECT_EQ(result.c_source.find("in_register_"), std::string::npos);
    EXPECT_EQ(result.c_source, expected_c);
}

/// Runs Example 3 through the complete native pipeline, including typed child
/// calls to the two helpers and the external `rand()` function.
TEST(DecompilerExamples, Example3TypedChildCallEndToEnd) {
    /// Supplies symbols for the root function and every direct child call.
    class TestSymbolProvider final : public SymbolProvider {
    public:
        /// Resolves the known Example 3 symbols by their machine-code address.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x140333210) {
                return SymbolDescription{address, "FUN_140333210", ""};
            }
            if (address == 0x14032b6f4) {
                return SymbolDescription{address, "FUN_14032b6f4", ""};
            }
            if (address == 0x14032b5e0) {
                return SymbolDescription{address, "FUN_14032b5e0", ""};
            }
            if (address == 0x141705a10) {
                return SymbolDescription{address, "rand", ""};
            }
            return std::nullopt;
        }
    };

    /// Supplies the primitive, floating-point, and saved-register types.
    class TestTypeProvider final : public TypeProvider {
    public:
        /// Resolves the type names used by Example 3's prototype and locals.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "undefined1") {
                type.size = 1;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined8") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined1[16]") {
                type.size = 16;
                type.kind = TypeKind::array;
                type.element_type = "undefined1";
                type.element_count = 16;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "uint") {
                type.size = 4;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "float") {
                type.size = 4;
                type.kind = TypeKind::floating_point;
            } else {
                return std::nullopt;
            }
            return type;
        }
    };

    /// Supplies root, helper, and rand prototypes so call return values are typed.
    class TestPrototypeProvider final : public PrototypeProvider {
    public:
        /// Returns the ABI storage and return type for a known Example 3 function.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x140333210) {
                prototype.calling_convention = "__fastcall";
                prototype.return_type = "int";
                prototype.return_storage = Storage{"register", 0, 4};
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "undefined8", Storage{"register", 8, 8}},
                    PrototypeParameterDescription{"param_2", "int", Storage{"register", 0x10, 4}},
                };
                return prototype;
            }
            if (address == 0x14032b6f4) {
                prototype.return_type = "void";
                return prototype;
            }
            if (address == 0x14032b5e0) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "undefined8", Storage{"register", 8, 8}},
                };
                return prototype;
            }
            if (address == 0x141705a10) {
                prototype.return_type = "int";
                prototype.return_storage = Storage{"register", 0, 4};
                return prototype;
            }
            return std::nullopt;
        }
    };

    /// Supplies the saved nonvolatile register locals from Example 3.
    class TestVariableProvider final : public VariableProvider {
    public:
        /// Returns stack locals with the documented storage and types.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x140333210) {
                return {};
            }
            return {
                VariableDescription{"local_res8", "undefined8", Storage{"stack", 8, 8}},
                VariableDescription{"local_18", "undefined1[16]",
                                    Storage{"stack", static_cast<std::uint64_t>(-0x18), 16}},
                VariableDescription{"local_28", "undefined1[16]",
                                    Storage{"stack", static_cast<std::uint64_t>(-0x28), 16}},
            };
        }
    };

    // These bytes are the complete contiguous function from Example 3.
    // The prologue saves RBX/RDI and reserves 0x40 bytes; MOVAPS saves XMM6
    // and XMM7; the first two CALL instructions target the supplied helper
    // symbols; the conditional body blends floating-point values; the second
    // branch calls rand(); and the epilogue restores all nonvolatile state.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x89, 0x5c, 0x24, 0x08,                   // MOV [RSP+8], RBX: save local_res8.
        0x57,                                           // PUSH RDI: save the nonvolatile integer register.
        0x48, 0x83, 0xec, 0x40,                         // SUB RSP, 0x40: allocate the stack frame.
        0x0f, 0x29, 0x74, 0x24, 0x30,                   // MOVAPS [RSP+0x30], XMM6: save local_18.
        0x8b, 0xfa,                                     // MOV EDI, EDX: copy param_2 into the selector register.
        0x48, 0x8b, 0xd9,                               // MOV RBX, RCX: copy param_1 into the saved context.
        0x0f, 0x29, 0x7c, 0x24, 0x20,                   // MOVAPS [RSP+0x20], XMM7: save local_28.
        0xe8, 0xc6, 0x84, 0xff, 0xff,                   // CALL FUN_14032b6f4: first helper call.
        0x48, 0x8b, 0xcb,                               // MOV RCX, RBX: pass param_1 to the second helper.
        0x0f, 0x28, 0xf8,                               // MOVAPS XMM7, XMM0: preserve the helper result.
        0xe8, 0xa7, 0x83, 0xff, 0xff,                   // CALL FUN_14032b5e0: second helper call.
        0x0f, 0x28, 0xf0,                               // MOVAPS XMM6, XMM0: preserve the second result.
        0x85, 0xff,                                     // TEST EDI, EDI: select the random/direct branch.
        0x74, 0x24,                                     // JZ LAB_140333264: use rand when param_2 is zero.
        0xf3, 0x0f, 0x5c, 0xf7,                         // SUBSS XMM6, XMM7: subtract helper results.
        0x40, 0x0f, 0xb6, 0xc7,                         // MOVZX EAX, DIL: reduce param_2 to one byte.
        0x66, 0x0f, 0x6e, 0xc8,                         // MOVD XMM1, EAX: move selector into an XMM register.
        0x0f, 0x5b, 0xc9,                               // CVTDQ2PS XMM1, XMM1: convert selector to float.
        0xf3, 0x0f, 0x59, 0x0d, 0x79, 0xfd, 0x50, 0x01, // MULSS XMM1, [DAT_141842fd0].
        0xf3, 0x0f, 0x59, 0xf1,                         // MULSS XMM6, XMM1: scale the difference.
        0xf3, 0x0f, 0x58, 0xf7,                         // ADDSS XMM6, XMM7: add the base value.
        0x0f, 0x28, 0xc6,                               // MOVAPS XMM0, XMM6: select the direct result.
        0xeb, 0x20,                                     // JMP LAB_140333284: join both branches.
        0xe8, 0xa7, 0x27, 0x3d, 0x01,                   // CALL rand: typed child call returning int in EAX.
        0xf3, 0x0f, 0x5c, 0xf7,                         // SUBSS XMM6, XMM7: subtract helper results.
        0x66, 0x0f, 0x6e, 0xc0,                         // MOVD XMM0, EAX: convert rand result input.
        0x0f, 0x5b, 0xc0,                               // CVTDQ2PS XMM0, XMM0: convert rand result to float.
        0xf3, 0x0f, 0x59, 0x05, 0xfc, 0xd1, 0x53, 0x01, // MULSS XMM0, [DAT_141870478].
        0xf3, 0x0f, 0x59, 0xc6,                         // MULSS XMM0, XMM6: scale the random result.
        0xf3, 0x0f, 0x58, 0xc7,                         // ADDSS XMM0, XMM7: add the base value.
        0x48, 0x8b, 0x5c, 0x24, 0x50,                   // MOV RBX, [RSP+0x50]: restore local_res8.
        0x0f, 0x28, 0x74, 0x24, 0x30,                   // MOVAPS XMM6, [RSP+0x30]: restore local_18.
        0x0f, 0x28, 0x7c, 0x24, 0x20,                   // MOVAPS XMM7, [RSP+0x20]: restore local_28.
        0x48, 0x83, 0xc4, 0x40,                         // ADD RSP, 0x40: release the stack frame.
        0x5f,                                           // POP RDI: restore the nonvolatile integer register.
        0xc3,                                           // RET: return the selected value.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    auto memory = std::make_shared<SparseMemory>(0x140333210, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    architecture.calling_convention = "__fastcall";
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    providers.symbols = std::make_shared<TestSymbolProvider>();
    providers.types = std::make_shared<TestTypeProvider>();
    providers.prototypes = std::make_shared<TestPrototypeProvider>();
    providers.variables = std::make_shared<TestVariableProvider>();
    Decompiler decompiler(std::move(architecture), std::move(providers));
    const DecompilationResult result =
        decompiler.decompile(FunctionDescription{"FUN_140333210", 0x140333210, 0x140333210 + function_bytes.size()});

    ASSERT_FALSE(result.raw_instructions.empty());
    ASSERT_FALSE(result.c_source.empty());
    const std::string expected_c = R"(
int __fastcall FUN_140333210(undefined8 param_1,int param_2)

{
  uint4 uVar1;
  
  FUN_14032b6f4();
  FUN_14032b5e0(param_1);
  if (param_2 == 0) {
    uVar1 = rand();
  }
  else {
    uVar1 = param_2 & 0xff;
  }
  return uVar1;
}
)";
    EXPECT_NE(result.c_source.find("FUN_14032b6f4"), std::string::npos);
    EXPECT_NE(result.c_source.find("FUN_14032b5e0(param_1)"), std::string::npos);
    EXPECT_NE(result.c_source.find("rand()"), std::string::npos);
    EXPECT_NE(result.c_source.find("param_2 == 0"), std::string::npos);
    EXPECT_NE(result.c_source.find("param_2 & 0xff"), std::string::npos);
    EXPECT_EQ(result.c_source, expected_c);
}

/// Runs the Visual Studio `bsearch` example through the real decoder and
/// decompiler, including provider-backed `__doserrno` and callback metadata.
TEST(DecompilerExamples, Example2BsearchEndToEnd) {
    /// Supplies Example 2 symbols and remembers child addresses discovered from CALL p-code.
    class TestProviders final : public SymbolProvider, public TypeProvider,
                                public PrototypeProvider, public VariableProvider {
    public:
        /// Resolves the root and discovered external function symbols.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x14170ca74) {
                return SymbolDescription{address, "bsearch", ""};
            }
            if (address != 0) {
                if (children_.empty()) {
                    children_[address] = "__doserrno";
                } else if (children_.find(address) == children_.end()) {
                    children_[address] = "FUN_141715f74";
                }
                return SymbolDescription{address, children_[address], ""};
            }
            return std::nullopt;
        }

        /// Resolves the primitive and callback pointer types used by bsearch.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "size_t" || name == "ulonglong" || name == "ulong") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "void *" || name == "ulong *" || name == "_PtFuncCompare *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = name == "void *" ? "void" : (name == "ulong *" ? "ulong" : "void");
            } else {
                return std::nullopt;
            }
            return type;
        }

        /// Supplies bsearch, __doserrno, and the library helper prototypes.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x14170ca74) {
                prototype.calling_convention = "__cdecl";
                prototype.return_type = "void *";
                prototype.return_storage = Storage{"register", 0, 8};
                prototype.parameters = {
                    PrototypeParameterDescription{"_Key", "void *", Storage{"register", 8, 8}},
                    PrototypeParameterDescription{"_Base", "void *", Storage{"register", 0x10, 8}},
                    PrototypeParameterDescription{"_NumOfElements", "size_t", Storage{"register", 0x80, 8}},
                    PrototypeParameterDescription{"_SizeOfElements", "size_t", Storage{"register", 0x88, 8}},
                    PrototypeParameterDescription{"_PtFuncCompare", "_PtFuncCompare *", Storage{"stack", 0x60, 8}},
                };
                return prototype;
            }
            const auto child = children_.find(address);
            if (child != children_.end() && child->second == "__doserrno") {
                prototype.return_type = "ulong *";
                prototype.return_storage = Storage{"register", 0, 8};
                return prototype;
            }
            prototype.return_type = "void";
            return prototype;
        }

        /// Supplies the four saved register locals from the x64 prologue.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x14170ca74) {
                return {};
            }
            return {
                VariableDescription{"local_res8", "undefined8", Storage{"stack", 8, 8}},
                VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
                VariableDescription{"local_res18", "undefined8", Storage{"stack", 0x18, 8}},
                VariableDescription{"local_res20", "undefined8", Storage{"stack", 0x20, 8}},
            };
        }

    private:
        mutable std::map<std::uint64_t, std::string> children_;
    };

    // Complete bsearch machine code. Each comment identifies the instruction
    // group represented by the following bytes; labels are represented by the
    // unchanged relative branch displacements in the byte stream.
    const std::vector<std::uint8_t> function_bytes{
        0x48,0x8b,0xc4, 0x48,0x89,0x58,0x08, 0x48,0x89,0x68,0x10, 0x48,0x89,0x70,0x18, 0x48,0x89,0x78,0x20, // prologue saves
        0x41,0x55, 0x41,0x56, 0x41,0x57, 0x48,0x83,0xec,0x20, // nonvolatile saves and frame
        0x49,0x8d,0x78,0xff, 0x45,0x33,0xf6, 0x4d,0x8b,0xf9, 0x49,0x8b,0xf0, 0x48,0x8b,0xda, 0x4c,0x8b,0xe9, // argument setup
        0x49,0x0f,0xaf,0xf9, 0x48,0x03,0xfa, 0x48,0x85,0xd2, 0x75,0x36, 0x4d,0x85,0xc0, 0x74,0x31, // upper bound checks
        0xe8,0x4a,0x45,0x00,0x00, 0xc7,0x00,0x16,0x00,0x00,0x00, 0xe8,0xaf,0x94,0x00,0x00, // __doserrno and invalid-parameter helper
        0x33,0xc0, 0x48,0x8b,0x5c,0x24,0x40, 0x48,0x8b,0x6c,0x24,0x48, 0x48,0x8b,0x74,0x24,0x50, 0x48,0x8b,0x7c,0x24,0x58, // error return epilogue
        0x48,0x83,0xc4,0x20, 0x41,0x5f, 0x41,0x5e, 0x41,0x5d, 0xc3, // return null path
        0x4d,0x85,0xc9, 0x74,0xca, 0x4c,0x39,0x74,0x24,0x60, 0x74,0xc3, 0x48,0x3b,0xd7, 0x77,0xce, // main loop guards
        0x48,0x8b,0xee, 0x48,0xd1,0xed, 0x74,0x41, 0x40,0xf6,0xc6,0x01, 0x48,0x8d,0x75,0xff, 0x49,0x8b,0xcd, 0x48,0x0f,0x45,0xf5, // midpoint
        0x4c,0x8b,0xf6, 0x4d,0x0f,0xaf,0xf7, 0x4c,0x03,0xf3, 0x49,0x8b,0xd6, 0xff,0x54,0x24,0x60, // callback comparison
        0x85,0xc0, 0x74,0x18, 0x79,0x08, 0x49,0x8b,0xfe, 0x49,0x2b,0xff, 0xeb,0x07, 0x4b,0x8d,0x1c,0x3e, 0x48,0x8b,0xf5, // probe update
        0x48,0x3b,0xdf, 0x76,0xbe, 0xeb,0x8a, 0x49,0x8b,0xc6, 0xeb,0x87, // loop and return probe
        0x45,0x33,0xf6, 0x48,0x85,0xf6, 0x0f,0x84,0x79,0xff,0xff,0xff, 0x48,0x8b,0xd3, 0x49,0x8b,0xcd, 0xff,0x54,0x24,0x60, 0x85,0xc0, 0x49,0x0f,0x45,0xde, 0x48,0x8b,0xc3, 0xe9,0x63,0xff,0xff,0xff, // one-element callback path
    };
    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    auto memory = std::make_shared<SparseMemory>(0x14170ca74, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    architecture.calling_convention = "__cdecl";
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    Decompiler decompiler(std::move(architecture), std::move(providers));
    const DecompilationResult result = decompiler.decompile(
        FunctionDescription{"bsearch", 0x14170ca74, 0x14170ca74 + function_bytes.size()});

    ASSERT_FALSE(result.raw_instructions.empty());
    ASSERT_FALSE(result.c_source.empty());
    const std::string expected_c = R"(
void * __cdecl
bsearch(void * _Key,void * _Base,size_t _NumOfElements,size_t _SizeOfElements,
       _PtFuncCompare * _PtFuncCompare)

{
  int4 iVar1;
  ulong * puVar2;
  BADSPACEBASE *in_RSP;
  xunknown1 *pxVar4;
  xunknown1 *pxVar3;
  uint8 uVar5;
  uint8 uVar6;
  void * pvVar7;
  void * pvVar8;
  int8 in_stack_00000028;
  xunknown1 axStack_38 [56];
  
  pxVar4 = axStack_38;
  pvVar7 = (void *)((_NumOfElements - 1) * _SizeOfElements + (int8)_Base);
  if ((((_Base == (void *)0x0) && (_NumOfElements != 0)) || (_SizeOfElements == 0)) ||
     (in_stack_00000028 == 0)) {
    puVar2 = __doserrno();
    *(xunknown4 *)puVar2 = 0x16;
    FUN_141715f74();
  }
  else if (_Base <= pvVar7) {
    do {
      uVar5 = _NumOfElements >> 1;
      if (uVar5 == 0) {
        pvVar7 = (void *)0x0;
        if (_NumOfElements == 0) {
          return 0;
        }
        iVar1 = (**(code **)(pxVar4 + 0x60))(_Key,_Base);
        if (iVar1 != 0) {
          _Base = pvVar7;
        }
        return _Base;
      }
      uVar6 = uVar5 - 1;
      if ((_NumOfElements & 1) != 0) {
        uVar6 = uVar5;
      }
      pvVar8 = (void *)(uVar6 * _SizeOfElements + (int8)_Base);
      pxVar3 = pxVar4 + -8;
      iVar1 = (**(code **)(pxVar4 + 0x60))(_Key,pvVar8);
      pxVar4 = pxVar3 + 8;
      if (iVar1 == 0) {
        return pvVar8;
      }
      if (iVar1 < 0) {
        pvVar7 = (void *)((int8)pvVar8 - _SizeOfElements);
        uVar5 = uVar6;
      }
      else {
        _Base = (void *)((int8)pvVar8 + _SizeOfElements);
      }
      _NumOfElements = uVar5;
    } while (_Base <= pvVar7);
  }
  return 0;
}
)";
    EXPECT_NE(result.c_source.find("bsearch"), std::string::npos);
    EXPECT_NE(result.c_source.find("__doserrno"), std::string::npos);
    EXPECT_NE(result.c_source.find("FUN_141715f74"), std::string::npos);
    EXPECT_NE(result.c_source.find("_PtFuncCompare"), std::string::npos);
    EXPECT_NE(result.c_source.find("while"), std::string::npos);
    EXPECT_EQ(result.c_source, expected_c);
}

/// Exercises the iterator loop and indirect callback call from Example 4.
TEST(DecompilerExamples, Example4VectorConstructorIteratorEndToEnd) {
    /// Supplies the typed iterator prototype and callback symbol metadata.
    class TestProviders final : public SymbolProvider, public TypeProvider,
                                public PrototypeProvider, public VariableProvider {
    public:
        /// Names the root function.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x140001500) return SymbolDescription{address, "vector_constructor_iterator", ""};
            return std::nullopt;
        }
        /// Supplies primitive and callback pointer types.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type; type.name = std::string(name);
            if (name == "void") { type.size = 1; type.kind = TypeKind::void_type; }
            else if (name == "int") { type.size = 4; type.kind = TypeKind::signed_integer; }
            else if (name == "__uint64") { type.size = 8; type.kind = TypeKind::unsigned_integer; }
            else if (name == "void *" || name == "_func_void_ptr_void_ptr *") {
                type.size = 8; type.kind = TypeKind::pointer; type.element_type = "void";
            } else return std::nullopt;
            return type;
        }
        /// Supplies the four documented iterator parameters.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            if (address != 0x140001500) return std::nullopt;
            PrototypeDescription prototype; prototype.calling_convention = "__cdecl";
            prototype.return_type = "void";
            prototype.parameters = {
                PrototypeParameterDescription{"param_1", "void *", Storage{"register", 8, 8}},
                PrototypeParameterDescription{"param_2", "__uint64", Storage{"register", 0x10, 8}},
                PrototypeParameterDescription{"param_3", "int", Storage{"register", 0x80, 4}},
                PrototypeParameterDescription{"param_4", "_func_void_ptr_void_ptr *", Storage{"register", 0x88, 8}},
            };
            return prototype;
        }
        /// Supplies the saved register locals.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x140001500) return {};
            return {
                VariableDescription{"local_res8", "undefined8", Storage{"stack", 8, 8}},
                VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
                VariableDescription{"local_res18", "undefined8", Storage{"stack", 0x18, 8}},
            };
        }
    };
    // Prologue, register setup, indirect callback loop, and epilogue.
    const std::vector<std::uint8_t> function_bytes{
        0x48,0x89,0x5c,0x24,0x08, 0x48,0x89,0x6c,0x24,0x10, 0x48,0x89,0x74,0x24,0x18,
        0x57, 0x48,0x83,0xec,0x20, 0x49,0x8b,0xf1, 0x41,0x8b,0xd8, 0x48,0x8b,0xea, 0x48,0x8b,0xf9,
        0xeb,0x08, // Jump into the decrement/check loop.
        0x48,0x8b,0xcf, 0xff,0xd6, 0x48,0x03,0xfd, // callback(param_1), then advance by stride.
        0xff,0xcb, 0x79,0xf4, // decrement count and loop while non-negative.
        0x48,0x8b,0x5c,0x24,0x30, 0x48,0x8b,0x6c,0x24,0x38, 0x48,0x8b,0x74,0x24,0x40,
        0x48,0x83,0xc4,0x20, 0x5f, 0xc3, // restore saved state and return.
    };
    std::vector<std::uint8_t> image_bytes = function_bytes; image_bytes.insert(image_bytes.end(), 16, 0x90);
    auto memory = std::make_shared<SparseMemory>(0x140001500, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize",2},{"opsize",1},{"rexprefix",0},{"longMode",1}});
    ArchitectureDescription architecture = test_architecture(); architecture.calling_convention = "__cdecl";
    ProviderContext providers; providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory; auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata; providers.types = metadata; providers.prototypes = metadata; providers.variables = metadata;
    Decompiler decompiler(std::move(architecture), std::move(providers));
    const DecompilationResult result = decompiler.decompile(FunctionDescription{"vector_constructor_iterator",0x140001500,0x140001500+function_bytes.size()});
    ASSERT_FALSE(result.c_source.empty());
    const std::string expected_c = R"(
void __cdecl
vector_constructor_iterator
          (void * param_1,__uint64 param_2,int param_3,_func_void_ptr_void_ptr * param_4)

{
  uint4 uVar1;
  uint8 uVar2;
  
  uVar2 = (uint8)(uint4)param_3;
  while (uVar1 = (int4)uVar2 - 1, uVar2 = (uint8)uVar1, -1 < (int4)uVar1) {
    (*param_4)(param_1);
    param_1 = (void *)((int8)param_1 + param_2);
  }
  return;
}
)";
    EXPECT_NE(result.c_source.find("vector_constructor_iterator"), std::string::npos);
    EXPECT_TRUE(result.c_source.find("while") != std::string::npos || result.c_source.find("for") != std::string::npos);
    EXPECT_EQ(result.c_source, expected_c);
}

/// Exercises the bounded slot append and typed allocation call from Example 5.
TEST(DecompilerExamples, Example5SlotAppendEndToEnd) {
    /// Supplies the function, allocator child, primitive types, and prototype.
    class TestProviders final : public SymbolProvider, public TypeProvider,
                                public PrototypeProvider, public VariableProvider {
    public:
        /// Names the root and dynamically discovered allocator child.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x141501684) return SymbolDescription{address, "FUN_141501684", ""};
            if (address != 0) return SymbolDescription{address, "FUN_140001044", ""};
            return std::nullopt;
        }
        /// Supplies integer and pointer types used by the table update.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type; type.name = std::string(name);
            if (name == "void") { type.size=1; type.kind=TypeKind::void_type; }
            else if (name == "int") { type.size=4; type.kind=TypeKind::signed_integer; }
            else if (name == "longlong" || name == "ulonglong" || name == "undefined8") { type.size=8; type.kind=TypeKind::unsigned_integer; }
            else if (name == "longlong *" || name == "undefined8 *" || name == "void *") { type.size=8; type.kind=TypeKind::pointer; type.element_type="undefined8"; }
            else return std::nullopt;
            return type;
        }
        /// Supplies the root ABI and allocator return prototype.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x141501684) {
                prototype.return_type="void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "longlong", Storage{"register", 8, 8}},
                    PrototypeParameterDescription{"param_2", "ulonglong", Storage{"register", 0x10, 8}},
                    PrototypeParameterDescription{"param_3", "undefined8", Storage{"register", 0x80, 8}},
                };
                return prototype;
            }
            if (address != 0) { prototype.return_type="void *"; prototype.return_storage=Storage{"register",0,8}; return prototype; }
            return std::nullopt;
        }
        /// Supplies saved-register locals for the bounded slot function.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x141501684) return {};
            return {VariableDescription{"local_res8","undefined8",Storage{"stack",8,8}}, VariableDescription{"local_res10","undefined8",Storage{"stack",0x10,8}}, VariableDescription{"local_res18","undefined8",Storage{"stack",0x18,8}}};
        }
    };
    // Complete prologue, masked slot lookup, allocation branch, record writes, and return.
    const std::vector<std::uint8_t> function_bytes{
        0x48,0x89,0x5c,0x24,0x08, 0x48,0x89,0x6c,0x24,0x10, 0x48,0x89,0x74,0x24,0x18, 0x57, 0x48,0x83,0xec,0x20,
        0x0f,0xb6,0xfa, 0x49,0x8b,0xe8, 0x48,0x8b,0xf2, 0x48,0x8b,0x04,0xf9, 0x48,0x8b,0xd9, 0x48,0x85,0xc0,
        0x74,0x06, 0x83,0x78,0x40,0x07, 0x75,0x19, 0xb9,0x80,0x00,0x00,0x00, 0xe8,0x87,0xf9,0xaf,0xfe,
        0x48,0x8b,0x0c,0xfb, 0x83,0x60,0x40,0x00, 0x48,0x89,0x08, 0x48,0x89,0x04,0xfb,
        0x8b,0x48,0x40, 0x48,0x8b,0x5c,0x24,0x30, 0x48,0x89,0x74,0xc8,0x48, 0x8b,0x48,0x40,
        0x48,0x8b,0x74,0x24,0x40, 0x48,0x89,0x6c,0xc8,0x08, 0xff,0x40,0x40, 0x48,0x8b,0x6c,0x24,0x38,
        0x48,0x83,0xc4,0x20, 0x5f, 0xc3,
    };
    std::vector<std::uint8_t> image_bytes = function_bytes; image_bytes.insert(image_bytes.end(),16,0x90);
    auto memory = std::make_shared<SparseMemory>(0x141501684,std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..")/"sleigh_runtime"/"test_data"/"x86-64.sla",memory,{{"addrsize",2},{"opsize",1},{"rexprefix",0},{"longMode",1}});
    ArchitectureDescription architecture=test_architecture(); ProviderContext providers; providers.pcode=std::make_shared<SleighPcodeProvider>(std::move(provider)); providers.memory=memory;
    auto metadata=std::make_shared<TestProviders>(); providers.symbols=metadata; providers.types=metadata; providers.prototypes=metadata; providers.variables=metadata;
    Decompiler decompiler(std::move(architecture),std::move(providers)); const DecompilationResult result=decompiler.decompile(FunctionDescription{"FUN_141501684",0x141501684,0x141501684+function_bytes.size()});
    ASSERT_FALSE(result.c_source.empty()); const std::string expected_c = R"(
void __cdecl FUN_141501684(longlong param_1,ulonglong param_2,undefined8 param_3)

{
  void * puVar1;
  uint8 uVar2;
  undefined8 uVar3;
  
  uVar2 = param_2 & 0xff;
  puVar1 = *(void * *)(param_1 + uVar2 * 8);
  if ((puVar1 == (void *)0x0) || ((int4)puVar1[8] == 7)) {
    puVar1 = FUN_140001044();
    uVar3 = *(undefined8 *)(param_1 + uVar2 * 8);
    *(xunknown4 *)(puVar1 + 8) = 0;
    *puVar1 = uVar3;
    *(void * *)(param_1 + uVar2 * 8) = puVar1;
  }
  puVar1[(uint8)(uint4)puVar1[8] + 9] = param_2;
  puVar1[(uint8)(uint4)puVar1[8] + 1] = param_3;
  *(int4 *)(puVar1 + 8) = (int4)puVar1[8] + 1;
  return;
}
)"; EXPECT_NE(result.c_source.find("FUN_141501684"),std::string::npos); EXPECT_NE(result.c_source.find("FUN_140001044"),std::string::npos); EXPECT_EQ(result.c_source, expected_c);
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
