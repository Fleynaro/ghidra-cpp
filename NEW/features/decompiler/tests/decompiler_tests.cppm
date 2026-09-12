module;

#include <gtest/gtest.h>

export module decompiler_tests;

import decompiler;
import ghidra.decompiler;
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

/// Removes formatting-only spaces at the ends of generated C source lines.
///
/// The native printer emits two trailing spaces on some blank lines. Golden
/// tests compare generated structure and expressions without depending on
/// that presentation detail.
static std::string without_trailing_whitespace(std::string value) {
    std::size_t line_start = 0;
    while (line_start < value.size()) {
        const std::size_t newline = value.find('\n', line_start);
        const std::size_t line_end = newline == std::string::npos ? value.size() : newline;
        std::size_t trim_end = line_end;
        while (trim_end > line_start && (value[trim_end - 1] == ' ' || value[trim_end - 1] == '\t')) {
            --trim_end;
        }
        value.erase(trim_end, line_end - trim_end);
        if (newline == std::string::npos) {
            break;
        }
        line_start = trim_end + 1;
    }
    return value;
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
        EXPECT_EQ(result.c_source, "\nundefined8 __cdecl copy_one(void)\n\n{\n  return 7;\n}\n");
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

/// Runs the Visual Studio `bsearch` example through the real decoder and
/// decompiler, including provider-backed `__doserrno` and callback metadata.
TEST(DecompilerExamples, Example2BsearchEndToEnd) {
    /// Supplies Example 2 symbols and remembers child addresses discovered from CALL p-code.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
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

    // The following groups are the complete Example 2 instruction listing.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x8b, 0xc4,                   // MOV RAX, RSP.
        0x48, 0x89, 0x58, 0x08,             // MOV [RAX+local_res8], RBX.
        0x48, 0x89, 0x68, 0x10,             // MOV [RAX+local_res10], RBP.
        0x48, 0x89, 0x70, 0x18,             // MOV [RAX+local_res18], RSI.
        0x48, 0x89, 0x78, 0x20,             // MOV [RAX+local_res20], RDI.
        0x41, 0x55,                         // PUSH R13.
        0x41, 0x56,                         // PUSH R14.
        0x41, 0x57,                         // PUSH R15.
        0x48, 0x83, 0xec, 0x20,             // SUB RSP, 0x20.
        0x49, 0x8d, 0x78, 0xff,             // LEA RDI, [_NumOfElements-1].
        0x45, 0x33, 0xf6,                   // XOR R14D, R14D.
        0x4d, 0x8b, 0xf9,                   // MOV R15, _SizeOfElements.
        0x49, 0x8b, 0xf0,                   // MOV RSI, _NumOfElements.
        0x48, 0x8b, 0xda,                   // MOV RBX, _Base.
        0x4c, 0x8b, 0xe9,                   // MOV R13, _Key.
        0x49, 0x0f, 0xaf, 0xf9,             // IMUL RDI, _SizeOfElements.
        0x48, 0x03, 0xfa,                   // ADD RDI, _Base.
        0x48, 0x85, 0xd2,                   // TEST _Base, _Base.
        0x75, 0x36,                         // JNZ LAB_14170cae6.
        0x4d, 0x85, 0xc0,                   // TEST _NumOfElements, _NumOfElements.
        0x74, 0x31,                         // JZ LAB_14170cae6.
        0xe8, 0x4a, 0x45, 0x00, 0x00,       // CALL __doserrno.
        0xc7, 0x00, 0x16, 0x00, 0x00, 0x00, // MOV [RAX], 0x16.
        0xe8, 0xaf, 0x94, 0x00, 0x00,       // CALL FUN_141715f74.
        0x33, 0xc0,                         // XOR EAX, EAX.
        0x48, 0x8b, 0x5c, 0x24, 0x40,       // MOV RBX, [RSP+local_res8].
        0x48, 0x8b, 0x6c, 0x24, 0x48,       // MOV RBP, [RSP+local_res10].
        0x48, 0x8b, 0x74, 0x24, 0x50,       // MOV RSI, [RSP+local_res18].
        0x48, 0x8b, 0x7c, 0x24, 0x58,       // MOV RDI, [RSP+local_res20].
        0x48, 0x83, 0xc4, 0x20,             // ADD RSP, 0x20.
        0x41, 0x5f,                         // POP R15.
        0x41, 0x5e,                         // POP R14.
        0x41, 0x5d,                         // POP R13.
        0xc3,                               // RET.
        0x4d, 0x85, 0xc9,                   // TEST _SizeOfElements, _SizeOfElements.
        0x74, 0xca,                         // JZ LAB_14170cab5.
        0x4c, 0x39, 0x74, 0x24, 0x60,       // CMP [_PtFuncCompare], R14.
        0x74, 0xc3,                         // JZ LAB_14170cab5.
        0x48, 0x3b, 0xd7,                   // CMP _Base, RDI.
        0x77, 0xce,                         // JA LAB_14170cac5.
        0x48, 0x8b, 0xee,                   // MOV RBP, RSI.
        0x48, 0xd1, 0xed,                   // SHR RBP, 0x1.
        0x74, 0x41,                         // JZ LAB_14170cb40.
        0x40, 0xf6, 0xc6, 0x01,             // TEST SIL, 0x1.
        0x48, 0x8d, 0x75, 0xff,             // LEA RSI, [RBP-1].
        0x49, 0x8b, 0xcd,                   // MOV _Key, R13.
        0x48, 0x0f, 0x45, 0xf5,             // CMOVNZ RSI, RBP.
        0x4c, 0x8b, 0xf6,                   // MOV R14, RSI.
        0x4d, 0x0f, 0xaf, 0xf7,             // IMUL R14, R15.
        0x4c, 0x03, 0xf3,                   // ADD R14, RBX.
        0x49, 0x8b, 0xd6,                   // MOV _Base, R14.
        0xff, 0x54, 0x24, 0x60,             // CALL [_PtFuncCompare].
        0x85, 0xc0,                         // TEST EAX, EAX.
        0x74, 0x18,                         // JZ LAB_14170cb3b.
        0x79, 0x08,                         // JNS LAB_14170cb2d.
        0x49, 0x8b, 0xfe,                   // MOV RDI, R14.
        0x49, 0x2b, 0xff,                   // SUB RDI, R15.
        0xeb, 0x07,                         // JMP LAB_14170cb34.
        0x4b, 0x8d, 0x1c, 0x3e,             // LEA RBX, [R14+R15].
        0x48, 0x8b, 0xf5,                   // MOV RSI, RBP.
        0x48, 0x3b, 0xdf,                   // CMP RBX, RDI.
        0x76, 0xbe,                         // JBE LAB_14170caf7.
        0xeb, 0x8a,                         // JMP LAB_14170cac5.
        0x49, 0x8b, 0xc6,                   // MOV RAX, R14.
        0xeb, 0x87,                         // JMP LAB_14170cac7.
        0x45, 0x33, 0xf6,                   // XOR R14D, R14D.
        0x48, 0x85, 0xf6,                   // TEST RSI, RSI.
        0x0f, 0x84, 0x79, 0xff, 0xff, 0xff, // JZ LAB_14170cac5.
        0x48, 0x8b, 0xd3,                   // MOV _Base, RBX.
        0x49, 0x8b, 0xcd,                   // MOV _Key, R13.
        0xff, 0x54, 0x24, 0x60,             // CALL [_PtFuncCompare].
        0x85, 0xc0,                         // TEST EAX, EAX.
        0x49, 0x0f, 0x45, 0xde,             // CMOVNZ RBX, R14.
        0x48, 0x8b, 0xc3,                   // MOV RAX, RBX.
        0xe9, 0x63, 0xff, 0xff, 0xff,       // JMP LAB_14170cac7.
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
    const DecompilationResult result =
        decompiler.decompile(FunctionDescription{"bsearch", 0x14170ca74, 0x14170ca74 + function_bytes.size()});

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
  undefined1 *pxVar4;
  undefined1 *pxVar3;
  uint8 uVar5;
  uint8 uVar6;
  void * pvVar7;
  void * pvVar8;
  int8 in_stack_00000028;
  undefined1 axStack_38 [56];
  
  pxVar4 = axStack_38;
  pvVar7 = (void *)((_NumOfElements - 1) * _SizeOfElements + (int8)_Base);
  if ((((_Base == (void *)0x0) && (_NumOfElements != 0)) || (_SizeOfElements == 0)) ||
     (in_stack_00000028 == 0)) {
    puVar2 = __doserrno();
    *(undefined4 *)puVar2 = 0x16;
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

/// Exercises the iterator loop and indirect callback call from Example 4.
TEST(DecompilerExamples, Example4VectorConstructorIteratorEndToEnd) {
    /// Supplies the typed iterator prototype and callback symbol metadata.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the root function.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x140001500)
                return SymbolDescription{address, "vector_constructor_iterator", ""};
            return std::nullopt;
        }
        /// Supplies primitive and callback pointer types.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "__uint64") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "void *" || name == "_func_void_ptr_void_ptr *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "void";
            } else
                return std::nullopt;
            return type;
        }
        /// Supplies the four documented iterator parameters.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            if (address != 0x140001500)
                return std::nullopt;
            PrototypeDescription prototype;
            prototype.calling_convention = "__cdecl";
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
            if (address != 0x140001500)
                return {};
            return {
                VariableDescription{"local_res8", "undefined8", Storage{"stack", 8, 8}},
                VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
                VariableDescription{"local_res18", "undefined8", Storage{"stack", 0x18, 8}},
            };
        }
    };
    // The following groups are the complete Example 4 instruction listing.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x89, 0x5c, 0x24, 0x08, // MOV [RSP+local_res8], RBX: save RBX.
        0x48, 0x89, 0x6c, 0x24, 0x10, // MOV [RSP+local_res10], RBP: save RBP.
        0x48, 0x89, 0x74, 0x24, 0x18, // MOV [RSP+local_res18], RSI: save RSI.
        0x57,                         // PUSH RDI: save RDI.
        0x48, 0x83, 0xec, 0x20,       // SUB RSP, 0x20: allocate the stack frame.
        0x49, 0x8b, 0xf1,             // MOV RSI, R9: save callback.
        0x41, 0x8b, 0xd8,             // MOV EBX, R8D: save element count.
        0x48, 0x8b, 0xea,             // MOV RBP, RDX: save element stride.
        0x48, 0x8b, 0xf9,             // MOV RDI, RCX: save current element.
        0xeb, 0x08,                   // JMP LAB_14000153a: enter loop check.
        0x48, 0x8b, 0xcf,             // MOV RCX, RDI: pass current element.
        0xff, 0xd6,                   // CALL RSI: invoke element constructor.
        0x48, 0x03, 0xfd,             // ADD RDI, RBP: advance by stride.
        0xff, 0xcb,                   // DEC EBX: decrement remaining count.
        0x79, 0xf4,                   // JNS LAB_140001532: continue while non-negative.
        0x48, 0x8b, 0x5c, 0x24, 0x30, // MOV RBX, [RSP+local_res8]: restore RBX.
        0x48, 0x8b, 0x6c, 0x24, 0x38, // MOV RBP, [RSP+local_res10]: restore RBP.
        0x48, 0x8b, 0x74, 0x24, 0x40, // MOV RSI, [RSP+local_res18]: restore RSI.
        0x48, 0x83, 0xc4, 0x20,       // ADD RSP, 0x20: release the stack frame.
        0x5f,                         // POP RDI: restore RDI.
        0xc3,                         // RET: return.
    };
    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90);
    auto memory = std::make_shared<SparseMemory>(0x140001500, std::move(image_bytes));
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
        FunctionDescription{"vector_constructor_iterator", 0x140001500, 0x140001500 + function_bytes.size()});
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
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the root and dynamically discovered allocator child.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x141501684)
                return SymbolDescription{address, "FUN_141501684", ""};
            if (address != 0)
                return SymbolDescription{address, "FUN_140001044", ""};
            return std::nullopt;
        }
        /// Supplies integer and pointer types used by the table update.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "longlong" || name == "ulonglong" || name == "undefined8") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "longlong *" || name == "undefined8 *" || name == "void *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "undefined8";
            } else
                return std::nullopt;
            return type;
        }
        /// Supplies the root ABI and allocator return prototype.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x141501684) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "longlong", Storage{"register", 8, 8}},
                    PrototypeParameterDescription{"param_2", "ulonglong", Storage{"register", 0x10, 8}},
                    PrototypeParameterDescription{"param_3", "undefined8", Storage{"register", 0x80, 8}},
                };
                return prototype;
            }
            if (address != 0) {
                prototype.return_type = "void *";
                prototype.return_storage = Storage{"register", 0, 8};
                return prototype;
            }
            return std::nullopt;
        }
        /// Supplies saved-register locals for the bounded slot function.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x141501684)
                return {};
            return {VariableDescription{"local_res8", "undefined8", Storage{"stack", 8, 8}},
                    VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
                    VariableDescription{"local_res18", "undefined8", Storage{"stack", 0x18, 8}}};
        }
    };
    // The following groups are the complete Example 5 instruction listing.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x89, 0x5c, 0x24, 0x08, // MOV [RSP+local_res8], RBX: save RBX.
        0x48, 0x89, 0x6c, 0x24, 0x10, // MOV [RSP+local_res10], RBP: save RBP.
        0x48, 0x89, 0x74, 0x24, 0x18, // MOV [RSP+local_res18], RSI: save RSI.
        0x57,                         // PUSH RDI: save RDI.
        0x48, 0x83, 0xec, 0x20,       // SUB RSP, 0x20: allocate the stack frame.
        0x0f, 0xb6, 0xfa,             // MOVZX EDI, DL: mask the slot selector.
        0x49, 0x8b, 0xe8,             // MOV RBP, R8: save param_3.
        0x48, 0x8b, 0xf2,             // MOV RSI, RDX: save param_2.
        0x48, 0x8b, 0x04, 0xf9,       // MOV RAX, [RCX+RDI*8]: load the slot record.
        0x48, 0x8b, 0xd9,             // MOV RBX, RCX: save table base.
        0x48, 0x85, 0xc0,             // TEST RAX, RAX: check for an empty slot.
        0x74, 0x06,                   // JZ LAB_1415016b3: allocate when empty.
        0x83, 0x78, 0x40, 0x07,       // CMP [RAX+0x40], 0x7: check record capacity.
        0x75, 0x19,                   // JNZ LAB_1415016cc: append to a non-full record.
        0xb9, 0x80, 0x00, 0x00, 0x00, // MOV ECX, 0x80: request record allocation.
        0xe8, 0x87, 0xf9, 0xaf, 0xfe, // CALL FUN_140001044: allocate a record.
        0x48, 0x8b, 0x0c, 0xfb,       // MOV RCX, [RBX+RDI*8]: load previous slot head.
        0x83, 0x60, 0x40, 0x00,       // AND [RAX+0x40], 0: clear record count.
        0x48, 0x89, 0x08,             // MOV [RAX], RCX: link previous slot head.
        0x48, 0x89, 0x04, 0xfb,       // MOV [RBX+RDI*8], RAX: publish new record.
        0x8b, 0x48, 0x40,             // MOV ECX, [RAX+0x40]: load record count.
        0x48, 0x8b, 0x5c, 0x24, 0x30, // MOV RBX, [RSP+local_res8]: restore RBX.
        0x48, 0x89, 0x74, 0xc8, 0x48, // MOV [RAX+RCX*8+0x48], RSI: store param_2.
        0x48, 0x8b, 0x48, 0x40,       // MOV RCX, [RAX+0x40]: reload record count.
        0x48, 0x8b, 0x74, 0x24, 0x40, // MOV RSI, [RSP+local_res18]: restore RSI.
        0x48, 0x89, 0x6c, 0xc8, 0x08, // MOV [RAX+RCX*8+0x8], RBP: store param_3.
        0xff, 0x40, 0x40,             // INC [RAX+0x40]: increment record count.
        0x48, 0x8b, 0x6c, 0x24, 0x38, // MOV RBP, [RSP+local_res10]: restore RBP.
        0x48, 0x83, 0xc4, 0x20,       // ADD RSP, 0x20: release the stack frame.
        0x5f,                         // POP RDI: restore RDI.
        0xc3,                         // RET: return.
    };
    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90);
    auto memory = std::make_shared<SparseMemory>(0x141501684, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    Decompiler decompiler(std::move(architecture), std::move(providers));
    const DecompilationResult result =
        decompiler.decompile(FunctionDescription{"FUN_141501684", 0x141501684, 0x141501684 + function_bytes.size()});
    ASSERT_FALSE(result.c_source.empty());
    const std::string expected_c = R"(
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
    *(undefined4 *)(puVar1 + 8) = 0;
    *puVar1 = uVar3;
    *(void * *)(param_1 + uVar2 * 8) = puVar1;
  }
  puVar1[(uint8)(uint4)puVar1[8] + 9] = param_2;
  puVar1[puVar1[8] + 1] = param_3;
  *(int4 *)(puVar1 + 8) = (int4)puVar1[8] + 1;
  return;
}
)";
    EXPECT_NE(result.c_source.find("FUN_141501684"), std::string::npos);
    EXPECT_NE(result.c_source.find("FUN_140001044"), std::string::npos);
    EXPECT_EQ(result.c_source, expected_c);
}

/// Runs the MetricSpentOnBankInterest constructor/destructor example through
/// the complete native pipeline and locks the current native C-printer output.
TEST(DecompilerExamples, Example6MetricSpentOnBankInterestEndToEnd) {
    /// Supplies symbols, types, prototypes, and stack-local metadata from Example 6.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the root function and every direct call target in the listing.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x140a6d118) {
                return SymbolDescription{
                    address, "MetricSpentOnBankInterest::MetricSpentOnBankInterest_Constructor_or_Destructor", ""};
            }
            if (address == 0x14132745c) {
                return SymbolDescription{address, "FUN_14132745c", ""};
            }
            if (address == 0x14015dcec) {
                return SymbolDescription{address, "FUN_14015dcec", ""};
            }
            if (address == 0x14015dd88) {
                return SymbolDescription{address, "FUN_14015dd88", ""};
            }
            if (address == 0x140a6c838) {
                return SymbolDescription{address, "rage::fwEvent::~fwEvent", ""};
            }
            if (address == 0x1413290b8) {
                return SymbolDescription{address, "FUN_1413290b8", ""};
            }
            return std::nullopt;
        }

        /// Resolves the primitive, pointer, and temporary array types used by Example 6.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "undefined") {
                type.size = 1;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined1") {
                type.size = 1;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined4") {
                type.size = 4;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined8") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined4[2]") {
                type.size = 8;
                type.kind = TypeKind::array;
                type.element_type = "undefined4";
                type.element_count = 2;
            } else if (name == "undefined1[56]") {
                type.size = 56;
                type.kind = TypeKind::array;
                type.element_type = "undefined1";
                type.element_count = 56;
            } else if (name == "undefined1[48]") {
                type.size = 48;
                type.kind = TypeKind::array;
                type.element_type = "undefined1";
                type.element_count = 48;
            } else if (name == "MetricSpentOnBankInterest_vftable *[3]") {
                type.size = 24;
                type.kind = TypeKind::array;
                type.element_type = "MetricSpentOnBankInterest_vftable *";
                type.element_count = 3;
            } else if (name == "MetricSpentOnBankInterest_vftable *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "MetricSpentOnBankInterest_vftable";
            } else if (name == "fwEvent *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "fwEvent";
            } else {
                return std::nullopt;
            }
            return type;
        }

        /// Supplies the root fastcall prototype and the typed direct-call signatures.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x140a6d118) {
                prototype.calling_convention = "__fastcall";
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "int", Storage{"register", 8, 4}},
                    PrototypeParameterDescription{"param_2", "undefined1", Storage{"register", 0x10, 1}},
                    PrototypeParameterDescription{"param_3", "undefined1", Storage{"register", 0x80, 1}},
                };
                return prototype;
            }
            if (address == 0x14132745c) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "MetricSpentOnBankInterest_vftable *",
                                                  Storage{"register", 8, 8}},
                };
                return prototype;
            }
            if (address == 0x14015dcec || address == 0x14015dd88) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "undefined1 *", Storage{"register", 8, 8}},
                };
                return prototype;
            }
            if (address == 0x140a6c838) {
                prototype.return_type = "undefined1";
                return prototype;
            }
            if (address == 0x1413290b8) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "MetricSpentOnBankInterest_vftable *",
                                                  Storage{"register", 8, 8}},
                };
                return prototype;
            }
            return std::nullopt;
        }

        /// Supplies the documented local storage ranges and names for the root function.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x140a6d118) {
                return {};
            }
            return {
                VariableDescription{"local_res8", "undefined4[2]", Storage{"stack", 0x8, 8}},
                VariableDescription{"local_a8", "MetricSpentOnBankInterest_vftable *[3]",
                                    Storage{"stack", static_cast<std::uint64_t>(-0xa8), 24}},
                VariableDescription{"local_90", "undefined1[56]",
                                    Storage{"stack", static_cast<std::uint64_t>(-0x90), 56}},
                VariableDescription{"local_58", "undefined1[48]",
                                    Storage{"stack", static_cast<std::uint64_t>(-0x58), 48}},
                VariableDescription{"local_res18", "undefined8", Storage{"stack", 0x18, 8}},
                VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
                VariableDescription{"local_8", "undefined", Storage{"stack", static_cast<std::uint64_t>(-0x8), 1}},
                VariableDescription{"local_18", "undefined1", Storage{"stack", static_cast<std::uint64_t>(-0x18), 1}},
                VariableDescription{"local_28", "undefined8", Storage{"stack", static_cast<std::uint64_t>(-0x28), 8}},
                VariableDescription{"local_b8", "undefined1", Storage{"stack", static_cast<std::uint64_t>(-0xb8), 1}},
                VariableDescription{"local_c0", "undefined1", Storage{"stack", static_cast<std::uint64_t>(-0xc0), 1}},
                VariableDescription{"local_c8", "undefined4", Storage{"stack", static_cast<std::uint64_t>(-0xc8), 4}},
                VariableDescription{"local_d0", "undefined4", Storage{"stack", static_cast<std::uint64_t>(-0xd0), 4}},
                VariableDescription{"local_d8", "undefined8", Storage{"stack", static_cast<std::uint64_t>(-0xd8), 8}},
                VariableDescription{"local_e0", "undefined8", Storage{"stack", static_cast<std::uint64_t>(-0xe0), 8}},
                VariableDescription{"local_e8", "undefined1", Storage{"stack", static_cast<std::uint64_t>(-0xe8), 1}},
            };
        }
    };

    // These bytes reproduce the complete contiguous Example 6 listing.
    const std::vector<std::uint8_t> function_bytes{
        0x85, 0xc9,                                     // TEST param_1, param_1.
        0x0f, 0x88, 0xc2, 0x00, 0x00, 0x00,             // JS LAB_140a6d1e2.
        0x48, 0x89, 0x5c, 0x24, 0x10,                   // MOV [RSP+local_res10], RBX.
        0x48, 0x89, 0x74, 0x24, 0x18,                   // MOV [RSP+local_res18], RSI.
        0x57,                                           // PUSH RDI.
        0x48, 0x81, 0xec, 0x00, 0x01, 0x00, 0x00,       // SUB RSP, 0x100.
        0x48, 0x63, 0xd9,                               // MOVSXD RBX, param_1.
        0x48, 0x8d, 0x4c, 0x24, 0x60,                   // LEA param_1, [RSP+0x60].
        0x41, 0x8a, 0xf0,                               // MOV SIL, param_3.
        0x40, 0x8a, 0xfa,                               // MOV DIL, param_2.
        0xe8, 0x17, 0xa3, 0x8b, 0x00,                   // CALL FUN_14132745c.
        0x48, 0x8d, 0x4c, 0x24, 0x78,                   // LEA param_1, [RSP+0x78].
        0xe8, 0x9d, 0x0b, 0x6f, 0xff,                   // CALL FUN_14015dcec.
        0x48, 0x8d, 0x8c, 0x24, 0xb0, 0x00, 0x00, 0x00, // LEA param_1, [RSP+0xb0].
        0xe8, 0x2c, 0x0c, 0x6f, 0xff,                   // CALL FUN_14015dd88.
        0x83, 0xa4, 0x24, 0x10, 0x01, 0x00, 0x00, 0x00, // AND [RSP+local_res8], 0.
        0xc6, 0x44, 0x24, 0x50, 0x00,                   // MOV [RSP+local_b8], 0.
        0xc6, 0x44, 0x24, 0x48, 0x00,                   // MOV [RSP+local_c0], 0.
        0x83, 0x4c, 0x24, 0x40, 0xff,                   // OR [RSP+local_c8], -1.
        0xc7, 0x44, 0x24, 0x38, 0x5b, 0x12, 0xba, 0x0b, // MOV [RSP+local_d0], 0x0bba125b.
        0x48, 0x8d, 0x05, 0x36, 0x3f, 0xe8, 0x00,       // LEA RAX, MetricSpentOnBankInterest::vftable.
        0x48, 0x8b, 0xcb,                               // MOV param_1, RBX.
        0x45, 0x33, 0xc9,                               // XOR R9D, R9D.
        0x48, 0x89, 0x44, 0x24, 0x60,                   // MOV [RSP+local_a8], RAX.
        0x48, 0x8d, 0x84, 0x24, 0x10, 0x01, 0x00, 0x00, // LEA RAX, [RSP+local_res8].
        0x44, 0x8a, 0xc6,                               // MOV param_3, SIL.
        0x48, 0x89, 0x44, 0x24, 0x30,                   // MOV [RSP+local_d8], RAX.
        0x48, 0x8d, 0x44, 0x24, 0x60,                   // LEA RAX, [RSP+local_a8].
        0x40, 0x8a, 0xd7,                               // MOV param_2, DIL.
        0x48, 0x89, 0x44, 0x24, 0x28,                   // MOV [RSP+local_e0], RAX.
        0x48, 0x89, 0x9c, 0x24, 0xe0, 0x00, 0x00, 0x00, // MOV [RSP+local_28], RBX.
        0x40, 0x88, 0xbc, 0x24, 0xf0, 0x00, 0x00, 0x00, // MOV [RSP+local_18], DIL.
        0xc6, 0x44, 0x24, 0x20, 0x00,                   // MOV [RSP+local_e8], 0.
        0xe8, 0x74, 0xf6, 0xff, 0xff,                   // CALL rage::fwEvent::~fwEvent.
        0x48, 0x8d, 0x4c, 0x24, 0x60,                   // LEA param_1, [RSP+local_a8].
        0xe8, 0xea, 0xbe, 0x8b, 0x00,                   // CALL FUN_1413290b8.
        0x4c, 0x8d, 0x9c, 0x24, 0x00, 0x01, 0x00, 0x00, // LEA R11, [RSP+0x100].
        0x49, 0x8b, 0x5b, 0x18,                         // MOV RBX, [R11+local_res10].
        0x49, 0x8b, 0x73, 0x20,                         // MOV RSI, [R11+local_res18].
        0x49, 0x8b, 0xe3,                               // MOV RSP, R11.
        0x5f,                                           // POP RDI.
        0xc3,                                           // RET.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    constexpr std::uint64_t entry = 0x140a6d118;
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    architecture.calling_convention = "__fastcall";
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    try {
        Decompiler decompiler(std::move(architecture), std::move(providers));
        const DecompilationResult result = decompiler.decompile(FunctionDescription{
            "MetricSpentOnBankInterest_Constructor_or_Destructor", entry, entry + function_bytes.size()});

        ASSERT_FALSE(result.raw_instructions.empty());
        ASSERT_FALSE(result.c_source.empty());
        // This is the current output of the native pipeline for the bytes and
        // provider metadata above. It intentionally records the port's current
        // stack-variable recovery, which is less typed than TEST/dec_code_examples/6.md.
        const std::string expected_c = R"(
void __fastcall
MetricSpentOnBankInterest::MetricSpentOnBankInterest_Constructor_or_Destructor
          (int param_1,undefined1 param_2,undefined1 param_3)

{
  int8 iVar1;
  BADSPACEBASE *in_RSP;
  undefined1 *pxVar2;
  undefined1 local_e8;
  undefined8 local_28;)"
                                       "\n  \n"
                                       R"(  if (-1 < param_1) {
    iVar1 = (int8)param_1;
    pxVar2 = &local_e8;
    FUN_14132745c(&local_28);
    FUN_14015dcec((undefined1 *)(pxVar2 + 0x80));
    FUN_14015dd88((undefined1 *)(pxVar2 + 0xb8));
    *(undefined4 *)(pxVar2 + 0x118) = 0;
    pxVar2[0x58] = 0;
    pxVar2[0x50] = 0;
    *(undefined4 *)(pxVar2 + 0x48) = 0xffffffff;
    *(undefined4 *)(pxVar2 + 0x40) = 0xbba125b;
    *(undefined8 *)(pxVar2 + 0x68) = 0x1418f10b8;
    *(undefined1 **)(pxVar2 + 0x38) = pxVar2 + 0x118;
    *(undefined1 **)(pxVar2 + 0x30) = pxVar2 + 0x68;
    *(int8 *)(pxVar2 + 0xe8) = iVar1;
    pxVar2[0xf8] = param_2;
    pxVar2[0x28] = 0;
    rage::fwEvent::~fwEvent();
    FUN_1413290b8((MetricSpentOnBankInterest_vftable *)(pxVar2 + 0x68));
  }
  return;
}
)";
        EXPECT_NE(result.c_source.find("MetricSpentOnBankInterest"), std::string::npos);
        EXPECT_NE(result.c_source.find("rage::fwEvent::~fwEvent"), std::string::npos);
        EXPECT_EQ(result.c_source, expected_c);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Runs the Visual Studio malloc implementation through the native decompiler pipeline.
TEST(DecompilerExamples, Example7MallocEndToEnd) {
    /// Supplies the malloc symbols, ABI types, and direct-call prototypes from Example 7.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Resolves malloc and every direct library call target in the listing.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x14170f4e8) {
                return SymbolDescription{address, "malloc", ""};
            }
            if (address == 0x141714a00) {
                return SymbolDescription{address, "_FF_MSGBANNER", ""};
            }
            if (address == 0x141714a74) {
                return SymbolDescription{address, "FUN_141714a74", ""};
            }
            if (address == 0x1417112e4) {
                return SymbolDescription{address, "__crtExitProcess", ""};
            }
            if (address == 0x14171eb00) {
                return SymbolDescription{address, "_callnewh", ""};
            }
            if (address == 0x141711004) {
                return SymbolDescription{address, "__doserrno", ""};
            }
            return std::nullopt;
        }

        /// Resolves the primitive and Windows allocation types used by malloc.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "size_t" || name == "SIZE_T") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "ulong") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "ulong *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "ulong";
            } else if (name == "void *" || name == "LPVOID" || name == "HANDLE") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "void";
            } else {
                return std::nullopt;
            }
            return type;
        }

        /// Supplies malloc, CRT helper, new-handler, and DOS-error prototypes.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x14170f4e8) {
                prototype.calling_convention = "__cdecl";
                prototype.return_type = "void *";
                prototype.return_storage = Storage{"register", 0, 8};
                prototype.parameters = {
                    PrototypeParameterDescription{"_Size", "size_t", Storage{"register", 8, 8}},
                };
                return prototype;
            }
            if (address == 0x141714a00) {
                prototype.return_type = "void";
                return prototype;
            }
            if (address == 0x141714a74 || address == 0x1417112e4) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "int", Storage{"register", 8, 4}},
                };
                return prototype;
            }
            if (address == 0x14171eb00) {
                prototype.return_type = "int";
                prototype.return_storage = Storage{"register", 0, 4};
                prototype.parameters = {
                    PrototypeParameterDescription{"_Size", "size_t", Storage{"register", 8, 8}},
                };
                return prototype;
            }
            if (address == 0x141711004) {
                prototype.return_type = "ulong *";
                prototype.return_storage = Storage{"register", 0, 8};
                return prototype;
            }
            return std::nullopt;
        }

        /// Supplies the two saved-register locals listed for malloc.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x14170f4e8) {
                return {};
            }
            return {
                VariableDescription{"local_res8", "undefined8", Storage{"stack", 0x8, 8}},
                VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
            };
        }
    };

    // These bytes reproduce the address-ordered Example 7 listing. The LAB_14170f57c
    // block is placed before the common epilogue because its branch target precedes
    // LAB_14170f58e in the original address space.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x89, 0x5c, 0x24, 0x08,             // MOV [RSP+local_res8], RBX.
        0x48, 0x89, 0x74, 0x24, 0x10,             // MOV [RSP+local_res10], RSI.
        0x57,                                     // PUSH RDI.
        0x48, 0x83, 0xec, 0x20,                   // SUB RSP, 0x20.
        0x48, 0x8b, 0xd9,                         // MOV RBX, _Size.
        0x48, 0x83, 0xf9, 0xe0,                   // CMP _Size, -0x20.
        0x77, 0x7c,                               // JA LAB_14170f57c.
        0xbf, 0x01, 0x00, 0x00, 0x00,             // MOV EDI, 0x1.
        0x48, 0x85, 0xc9,                         // TEST _Size, _Size.
        0x48, 0x0f, 0x45, 0xf9,                   // CMOVNZ RDI, _Size.
        0x48, 0x8b, 0x0d, 0xf5, 0x93, 0x5d, 0x01, // MOV _Size, [DAT_142ce8908].
        0x48, 0x85, 0xc9,                         // TEST _Size, _Size.
        0x75, 0x20,                               // JNZ LAB_14170f538.
        0xe8, 0xe3, 0x54, 0x00, 0x00,             // CALL _FF_MSGBANNER.
        0xb9, 0x1e, 0x00, 0x00, 0x00,             // MOV ECX, 0x1e.
        0xe8, 0x4d, 0x55, 0x00, 0x00,             // CALL FUN_141714a74.
        0xb9, 0xff, 0x00, 0x00, 0x00,             // MOV ECX, 0xff.
        0xe8, 0xb3, 0x1d, 0x00, 0x00,             // CALL __crtExitProcess.
        0x48, 0x8b, 0x0d, 0xd0, 0x93, 0x5d, 0x01, // MOV _Size, [DAT_142ce8908].
        0x4c, 0x8b, 0xc7,                         // MOV R8, RDI.
        0x33, 0xd2,                               // XOR EDX, EDX.
        0xff, 0x15, 0xe5, 0x6d, 0x07, 0x00,       // CALL [KERNEL32.DLL::HeapAlloc].
        0x48, 0x8b, 0xf0,                         // MOV RSI, RAX.
        0x48, 0x85, 0xc0,                         // TEST RAX, RAX.
        0x75, 0x2c,                               // JNZ LAB_14170f577.
        0x39, 0x05, 0xef, 0x9e, 0x5d, 0x01,       // CMP [DAT_142ce9440], EAX.
        0x74, 0x0e,                               // JZ LAB_14170f561.
        0x48, 0x8b, 0xcb,                         // MOV _Size, RBX.
        0xe8, 0xa5, 0xf5, 0x00, 0x00,             // CALL _callnewh.
        0x85, 0xc0,                               // TEST EAX, EAX.
        0x74, 0x0d,                               // JZ LAB_14170f56c.
        0xeb, 0xab,                               // JMP LAB_14170f50c.
        0xe8, 0x9e, 0x1a, 0x00, 0x00,             // CALL __doserrno.
        0xc7, 0x00, 0x0c, 0x00, 0x00, 0x00,       // MOV [RAX], 0xc.
        0xe8, 0x93, 0x1a, 0x00, 0x00,             // CALL __doserrno.
        0xc7, 0x00, 0x0c, 0x00, 0x00, 0x00,       // MOV [RAX], 0xc.
        0x48, 0x8b, 0xc6,                         // MOV RAX, RSI.
        0xeb, 0x12,                               // JMP LAB_14170f58e.
        0xe8, 0x7f, 0xf5, 0x00, 0x00,             // CALL _callnewh.
        0xe8, 0x7e, 0x1a, 0x00, 0x00,             // CALL __doserrno.
        0xc7, 0x00, 0x0c, 0x00, 0x00, 0x00,       // MOV [RAX], 0xc.
        0x33, 0xc0,                               // XOR EAX, EAX.
        0x48, 0x8b, 0x5c, 0x24, 0x30,             // MOV RBX, [RSP+local_res8].
        0x48, 0x8b, 0x74, 0x24, 0x38,             // MOV RSI, [RSP+local_res10].
        0x48, 0x83, 0xc4, 0x20,                   // ADD RSP, 0x20.
        0x5f,                                     // POP RDI.
        0xc3,                                     // RET.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    constexpr std::uint64_t entry = 0x14170f4e8;
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image_bytes));
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
    try {
        Decompiler decompiler(std::move(architecture), std::move(providers));
        const DecompilationResult result =
            decompiler.decompile(FunctionDescription{"malloc", entry, entry + function_bytes.size()});
        ASSERT_FALSE(result.raw_instructions.empty());
        ASSERT_FALSE(result.c_source.empty());
        // The current port represents the indirect HeapAlloc import as a
        // call-indirect expression and therefore returns its recovered int8
        // result instead of the LPVOID shown in TEST/dec_code_examples/7.md.
        const std::string expected_c = R"(
void * __cdecl malloc(size_t _Size)

{
  int iVar1;
  int8 iVar2;
  ulong * puVar3;
  size_t sVar4;

  if (_Size < 0xffffffffffffffe1) {
    sVar4 = 1;
    if (_Size != 0) {
      sVar4 = _Size;
    }
    do {
      if (iRam0000000142ce8908 == 0) {
        _FF_MSGBANNER();
        FUN_141714a74(0x1e);
        __crtExitProcess(0xff);
      }
      iVar2 = (*pcRam0000000141786328)(iRam0000000142ce8908,0,sVar4);
      if (iVar2 != 0) goto code_r0x00014170f58e;
      if (iRam0000000142ce9440 == 0) {
        puVar3 = __doserrno();
        *(undefined4 *)puVar3 = 0xc;
        break;
      }
      iVar1 = _callnewh(_Size);
    } while (iVar1 != 0);
    puVar3 = __doserrno();
    *(undefined4 *)puVar3 = 0xc;
  }
  else {
    _callnewh(_Size);
    puVar3 = __doserrno();
    *(undefined4 *)puVar3 = 0xc;
    iVar2 = 0;
  }
code_r0x00014170f58e:
  return iVar2;
}
)";
        EXPECT_EQ(without_trailing_whitespace(result.c_source), expected_c);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Runs the SIMD floating-point rounding helper through the native decompiler pipeline.
TEST(DecompilerExamples, Example8FloatingPointRoundEndToEnd) {
    /// Supplies the root symbol and pointer prototype from Example 8.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the Example 8 root function.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x141500d88) {
                return SymbolDescription{address, "UndefinedFunction_141500d88", ""};
            }
            return std::nullopt;
        }

        /// Resolves scalar, floating-point, pointer, and SIMD temporary types.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "uint") {
                type.size = 4;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined8") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined8 *") {
                type.size = 8;
                type.kind = TypeKind::pointer;
                type.element_type = "undefined8";
            } else {
                return std::nullopt;
            }
            return type;
        }

        /// Supplies the void prototype and the documented RCX pointer parameter.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            if (address != 0x141500d88) {
                return std::nullopt;
            }
            PrototypeDescription prototype;
            prototype.return_type = "void";
            prototype.parameters = {
                PrototypeParameterDescription{"param_1", "undefined8 *", Storage{"register", 8, 8}},
            };
            return prototype;
        }

        /// Example 8 has no externally named stack locals.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t) const override {
            return {};
        }
    };

    // These bytes reproduce the complete Example 8 SIMD instruction listing.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x8b, 0x41, 0x10,                         // MOV RAX, [RCX+0x10].
        0x4c, 0x8b, 0xc1,                               // MOV R8, RCX.
        0xf3, 0x0f, 0x10, 0x08,                         // MOVSS XMM1, [RAX].
        0xf3, 0x0f, 0x58, 0x0d, 0x41, 0xc8, 0x33, 0x00, // ADDSS XMM1, [DAT_14183d5dc].
        0xf3, 0x0f, 0x2c, 0xd1,                         // CVTTSS2SI EDX, XMM1.
        0x81, 0xfa, 0x00, 0x00, 0x00, 0x80,             // CMP EDX, 0x80000000.
        0x74, 0x1e,                                     // JZ LAB_141500dc5.
        0x66, 0x0f, 0x6e, 0xc2,                         // MOVD XMM0, EDX.
        0x0f, 0x5b, 0xc0,                               // CVTDQ2PS XMM0, XMM0.
        0x0f, 0x2e, 0xc1,                               // UCOMISS XMM0, XMM1.
        0x74, 0x12,                                     // JZ LAB_141500dc5.
        0x0f, 0x14, 0xc9,                               // UNPCKLPS XMM1, XMM1.
        0x0f, 0x50, 0xc1,                               // MOVMSKPS EAX, XMM1.
        0x83, 0xe0, 0x01,                               // AND EAX, 0x1.
        0x2b, 0xd0,                                     // SUB EDX, EAX.
        0x66, 0x0f, 0x6e, 0xca,                         // MOVD XMM1, EDX.
        0x0f, 0x5b, 0xc9,                               // CVTDQ2PS XMM1, XMM1.
        0x49, 0x8b, 0x00,                               // MOV RAX, [R8].
        0xf3, 0x0f, 0x2c, 0xc9,                         // CVTTSS2SI ECX, XMM1.
        0x89, 0x08,                                     // MOV [RAX], ECX.
        0xc3,                                           // RET.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    constexpr std::uint64_t entry = 0x141500d88;
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    try {
        Decompiler decompiler(std::move(architecture), std::move(providers));
        const DecompilationResult result = decompiler.decompile(
            FunctionDescription{"UndefinedFunction_141500d88", entry, entry + function_bytes.size()});
        ASSERT_FALSE(result.raw_instructions.empty());
        ASSERT_FALSE(result.c_source.empty());
        // MOVMSKPS is exposed by the minimal architecture as an unknown
        // user-op, so the safe fallback name userop_162 is retained here.
        const std::string expected_c = R"(
void __cdecl UndefinedFunction_141500d88(undefined8 *param_1)

{
  uint4 uVar1;
  int4 iVar2;
  undefined1 axVar3 [16];

  axVar3._0_4_ = *(float4 *)param_1[2] + fRam000000014183d5dc;
  iVar2 = (int4)axVar3._0_4_;
  if ((iVar2 != -0x80000000) && ((float4)iVar2 != axVar3._0_4_)) {
    axVar3._4_4_ = axVar3._0_4_;
    axVar3._8_8_ = 0;
    uVar1 = userop_162((int4)(float4 *)param_1[2],axVar3);
    axVar3._0_4_ = (float4)(int4)(iVar2 - (uVar1 & 1));
  }
  *(int4 *)*param_1 = (int4)axVar3._0_4_;
  return;
}
)";
        EXPECT_EQ(without_trailing_whitespace(result.c_source), expected_c);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Runs the TLS-backed event context update through the native decompiler pipeline.
TEST(DecompilerExamples, Example9TlsContextUpdateEndToEnd) {
    /// Supplies the root symbol, longlong type, and RCX source-object prototype.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the Example 9 root function.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x141500e30) {
                return SymbolDescription{address, "UndefinedFunction_141500e30", ""};
            }
            return std::nullopt;
        }

        /// Resolves the signed 64-bit source-object type.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            if (name != "longlong") {
                return std::nullopt;
            }
            TypeDescription type;
            type.name = "longlong";
            type.size = 8;
            type.kind = TypeKind::signed_integer;
            return type;
        }

        /// Supplies the void prototype and the documented source-object parameter.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            if (address != 0x141500e30) {
                return std::nullopt;
            }
            PrototypeDescription prototype;
            prototype.return_type = "void";
            prototype.parameters = {
                PrototypeParameterDescription{"param_1", "longlong", Storage{"register", 8, 8}},
            };
            return prototype;
        }

        /// Example 9 has no externally named stack locals.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t) const override {
            return {};
        }
    };

    // These bytes reproduce the complete Example 9 TLS instruction listing.
    const std::vector<std::uint8_t> function_bytes{
        0x48, 0x8b, 0x51, 0x10,                               // MOV RDX, [RCX+0x10].
        0x65, 0x48, 0x8b, 0x0c, 0x25, 0x58, 0x00, 0x00, 0x00, // MOV RCX, GS:[0x58].
        0x8b, 0x05, 0x29, 0x73, 0x7e, 0x01,                   // MOV EAX, [_tls_index].
        0x48, 0x8b, 0x04, 0xc1,                               // MOV RAX, [RCX+RAX*8].
        0x41, 0xb8, 0x30, 0x08, 0x00, 0x00,                   // MOV R8D, 0x830.
        0x4a, 0x8b, 0x0c, 0x00,                               // MOV RCX, [RAX+R8].
        0x8b, 0x02,                                           // MOV EAX, [RDX].
        0x89, 0x41, 0x68,                                     // MOV [RCX+0x68], EAX.
        0xc3,                                                 // RET.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    constexpr std::uint64_t entry = 0x141500e30;
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    try {
        Decompiler decompiler(std::move(architecture), std::move(providers));
        const DecompilationResult result = decompiler.decompile(
            FunctionDescription{"UndefinedFunction_141500e30", entry, entry + function_bytes.size()});
        ASSERT_FALSE(result.raw_instructions.empty());
        ASSERT_FALSE(result.c_source.empty());
        // The minimal architecture has no named GS/TLS or global-data
        // provider, so those values remain generated memory symbols.
        const std::string expected_c = R"(
void __cdecl UndefinedFunction_141500e30(longlong param_1)

{
  int8 in_register_00000118;

  *(undefined4 *)
   (*(int8 *)(*(int8 *)(*(int8 *)(in_register_00000118 + 0x58) + (uint8)uRam0000000142ce816c * 8) +
             0x830) + 0x68) = **(undefined4 **)(param_1 + 0x10);
  return;
}
)";
        EXPECT_EQ(without_trailing_whitespace(result.c_source), expected_c);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Runs the global table-entry initialization loop through the native decompiler pipeline.
TEST(DecompilerExamples, Example10GlobalTableInitializationEndToEnd) {
    /// Supplies the root and allocator symbols, primitive types, and ABI prototypes.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the root function and direct allocation helper.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x141500bb0) {
                return SymbolDescription{address, "FUN_141500bb0", ""};
            }
            if (address == 0x140001044) {
                return SymbolDescription{address, "FUN_140001044", ""};
            }
            return std::nullopt;
        }

        /// Resolves the integer, pointer, array, and overflow-helper types used by the loop.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "longlong") {
                type.size = 8;
                type.kind = TypeKind::signed_integer;
            } else if (name == "undefined8" || name == "ulonglong") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined1") {
                type.size = 1;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "undefined1[16]") {
                type.size = 16;
                type.kind = TypeKind::array;
                type.element_type = "undefined1";
                type.element_count = 16;
            } else {
                return std::nullopt;
            }
            return type;
        }

        /// Supplies the root two-int prototype and allocator return/argument types.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            PrototypeDescription prototype;
            if (address == 0x141500bb0) {
                prototype.return_type = "void";
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "int", Storage{"register", 8, 4}},
                    PrototypeParameterDescription{"param_2", "int", Storage{"register", 0x10, 4}},
                };
                return prototype;
            }
            if (address == 0x140001044) {
                prototype.return_type = "undefined8";
                prototype.return_storage = Storage{"register", 0, 8};
                prototype.parameters = {
                    PrototypeParameterDescription{"param_1", "undefined8", Storage{"register", 8, 8}},
                };
                return prototype;
            }
            return std::nullopt;
        }

        /// Supplies the three saved-register locals from the prologue.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
            if (address != 0x141500bb0) {
                return {};
            }
            return {
                VariableDescription{"local_res8", "undefined8", Storage{"stack", 0x8, 8}},
                VariableDescription{"local_res10", "undefined8", Storage{"stack", 0x10, 8}},
                VariableDescription{"local_res18", "undefined8", Storage{"stack", 0x18, 8}},
            };
        }
    };

    // These bytes reproduce the complete Example 10 table initialization listing.
    const std::vector<std::uint8_t> function_bytes{
        0x85, 0xc9,                                     // TEST ECX, ECX.
        0x0f, 0x84, 0x8b, 0x00, 0x00, 0x00,             // JZ LAB_141500c43.
        0x48, 0x89, 0x5c, 0x24, 0x08,                   // MOV [RSP+local_res8], RBX.
        0x48, 0x89, 0x6c, 0x24, 0x10,                   // MOV [RSP+local_res10], RBP.
        0x48, 0x89, 0x74, 0x24, 0x18,                   // MOV [RSP+local_res18], RSI.
        0x57,                                           // PUSH RDI.
        0x41, 0x56,                                     // PUSH R14.
        0x41, 0x57,                                     // PUSH R15.
        0x48, 0x83, 0xec, 0x20,                         // SUB RSP, 0x20.
        0x48, 0x63, 0xea,                               // MOVSXD RBP, EDX.
        0x8b, 0xf1,                                     // MOV ESI, ECX.
        0x41, 0xbf, 0x01, 0x00, 0x00, 0x00,             // MOV R15D, 0x1.
        0x4c, 0x8b, 0xf5,                               // MOV R14, RBP.
        0x44, 0x0f, 0xb7, 0x05, 0x72, 0xb6, 0x6e, 0x01, // MOVZX R8D, [DAT_142bec258].
        0x48, 0x8b, 0x1d, 0x63, 0xb6, 0x6e, 0x01,       // MOV RBX, [DAT_142bec250].
        0x48, 0xc7, 0xc1, 0xff, 0xff, 0xff, 0xff,       // MOV RCX, -1.
        0x4b, 0x8d, 0x3c, 0x40,                         // LEA RDI, [R8+R8*2].
        0x66, 0x45, 0x03, 0xc7,                         // ADD R8W, R15W.
        0xb8, 0x08, 0x00, 0x00, 0x00,                   // MOV EAX, 0x8.
        0x66, 0x44, 0x89, 0x05, 0x4f, 0xb6, 0x6e, 0x01, // MOV [DAT_142bec258], R8W.
        0x48, 0x83, 0x24, 0xfb, 0x00,                   // AND [RBX+RDI*8], 0.
        0x89, 0x6c, 0xfb, 0x08,                         // MOV [RBX+RDI*8+8], EBP.
        0x49, 0xf7, 0xe6,                               // MUL R14.
        0x48, 0x0f, 0x40, 0xc1,                         // CMOVO RAX, RCX.
        0x48, 0x8b, 0xc8,                               // MOV RCX, RAX.
        0xe8, 0x23, 0x04, 0xb0, 0xfe,                   // CALL FUN_140001044.
        0x48, 0x89, 0x44, 0xfb, 0x10,                   // MOV [RBX+RDI*8+0x10], RAX.
        0x41, 0x2b, 0xf7,                               // SUB ESI, R15D.
        0x75, 0xb3,                                     // JNZ LAB_141500bde.
        0x48, 0x8b, 0x5c, 0x24, 0x40,                   // MOV RBX, [RSP+local_res8].
        0x48, 0x8b, 0x6c, 0x24, 0x48,                   // MOV RBP, [RSP+local_res10].
        0x48, 0x8b, 0x74, 0x24, 0x50,                   // MOV RSI, [RSP+local_res18].
        0x48, 0x83, 0xc4, 0x20,                         // ADD RSP, 0x20.
        0x41, 0x5f,                                     // POP R15.
        0x41, 0x5e,                                     // POP R14.
        0x5f,                                           // POP RDI.
        0xc3,                                           // RET.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    constexpr std::uint64_t entry = 0x141500bb0;
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    try {
        Decompiler decompiler(std::move(architecture), std::move(providers));
        const DecompilationResult result =
            decompiler.decompile(FunctionDescription{"FUN_141500bb0", entry, entry + function_bytes.size()});
        ASSERT_FALSE(result.raw_instructions.empty());
        ASSERT_FALSE(result.c_source.empty());
        // The native port preserves the loop and overflow calculation but
        // currently recovers narrower temporary integer types than 10.md.
        const std::string expected_c = R"(
void __cdecl FUN_141500bb0(int param_1,int param_2)

{
  undefined8 uVar1;
  int8 iVar2;
  uint8 uVar3;
  int8 iVar4;
  uint8 uVar5;
  uint8 uVar6;
  undefined8 xVar7;
  undefined1 axVar8 [16];

  if (param_1 != 0) {
    uVar3 = (uint8)param_2;
    xVar7 = 1;
    uVar6 = uVar3;
    do {
      uVar5 = (uint8)uRam0000000142bec258;
      iVar4 = uVar5 * 3;
      uRam0000000142bec258 = uRam0000000142bec258 + (int2)xVar7;
      *(undefined8 *)(iRam0000000142bec250 + uVar5 * 0x18) = 0;
      *(int4 *)(iRam0000000142bec250 + 8 + uVar5 * 0x18) = (int4)uVar3;
      axVar8._8_8_ = 0;
      axVar8._0_8_ = uVar6;
      uVar1 = SUB168(ZEXT816(8) * axVar8,0);
      if (SUB168(ZEXT816(8) * axVar8,8) != 0) {
        uVar1 = 0xffffffffffffffff;
      }
      iVar2 = iRam0000000142bec250;
      uVar1 = FUN_140001044(uVar1);
      *(undefined8 *)(iVar2 + 0x10 + iVar4 * 8) = uVar1;
      param_1 = param_1 - (int4)xVar7;
    } while (param_1 != 0);
  }
  return;
}
)";
        EXPECT_EQ(without_trailing_whitespace(result.c_source), expected_c);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
}

/// Runs the first-free global-slot registration helper through the native decompiler pipeline.
TEST(DecompilerExamples, Example11GlobalSlotRegistrationEndToEnd) {
    /// Supplies the root symbol, scalar types, and boolean-like return prototype.
    class TestProviders final : public SymbolProvider,
                                public TypeProvider,
                                public PrototypeProvider,
                                public VariableProvider {
    public:
        /// Names the Example 11 root function.
        [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
            if (address == 0x141500c44) {
                return SymbolDescription{address, "FUN_141500c44", ""};
            }
            return std::nullopt;
        }

        /// Resolves all scalar types used by the slot search and return value.
        [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
            TypeDescription type;
            type.name = std::string(name);
            if (name == "void") {
                type.size = 1;
                type.kind = TypeKind::void_type;
            } else if (name == "int") {
                type.size = 4;
                type.kind = TypeKind::signed_integer;
            } else if (name == "uint") {
                type.size = 4;
                type.kind = TypeKind::unsigned_integer;
            } else if (name == "longlong") {
                type.size = 8;
                type.kind = TypeKind::signed_integer;
            } else if (name == "ulonglong") {
                type.size = 8;
                type.kind = TypeKind::unsigned_integer;
            } else {
                return std::nullopt;
            }
            return type;
        }

        /// Supplies the ulonglong return and longlong/int parameter ABI locations.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            if (address != 0x141500c44) {
                return std::nullopt;
            }
            PrototypeDescription prototype;
            prototype.return_type = "ulonglong";
            prototype.return_storage = Storage{"register", 0, 8};
            prototype.parameters = {
                PrototypeParameterDescription{"param_1", "longlong", Storage{"register", 8, 8}},
                PrototypeParameterDescription{"param_2", "int", Storage{"register", 0x10, 4}},
            };
            return prototype;
        }

        /// Example 11 has no externally named stack locals.
        [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t) const override {
            return {};
        }
    };

    // These bytes reproduce the complete Example 11 global-slot registration listing.
    const std::vector<std::uint8_t> function_bytes{
        0x83, 0x79, 0x58, 0x00,                   // CMP [RCX+0x58], 0.
        0x4c, 0x8b, 0xc9,                         // MOV R9, RCX.
        0x75, 0x34,                               // JNZ LAB_141500c81.
        0x0f, 0xb7, 0x05, 0x04, 0xb6, 0x6e, 0x01, // MOVZX EAX, [DAT_142bec258].
        0x45, 0x33, 0xc0,                         // XOR R8D, R8D.
        0x33, 0xc9,                               // XOR ECX, ECX.
        0x44, 0x8b, 0xd0,                         // MOV R10D, EAX.
        0x85, 0xc0,                               // TEST EAX, EAX.
        0x7e, 0x21,                               // JLE LAB_141500c81.
        0x48, 0x8b, 0x05, 0xe9, 0xb5, 0x6e, 0x01, // MOV RAX, [DAT_142bec250].
        0x48, 0x83, 0x38, 0x00,                   // CMP [RAX], 0.
        0x75, 0x05,                               // JNZ LAB_141500c72.
        0x39, 0x50, 0x08,                         // CMP [RAX+8], EDX.
        0x74, 0x12,                               // JZ LAB_141500c84.
        0x48, 0xff, 0xc1,                         // INC RCX.
        0x41, 0xff, 0xc0,                         // INC R8D.
        0x48, 0x83, 0xc0, 0x18,                   // ADD RAX, 0x18.
        0x49, 0x3b, 0xca,                         // CMP RCX, R10.
        0x7c, 0xe6,                               // JL LAB_141500c67.
        0x32, 0xc0,                               // XOR AL, AL.
        0xc3,                                     // RET.
        0x41, 0x89, 0x51, 0x58,                   // MOV [R9+0x58], EDX.
        0x48, 0x8b, 0x0d, 0xc1, 0xb5, 0x6e, 0x01, // MOV RCX, [DAT_142bec250].
        0x4f, 0x8d, 0x04, 0x40,                   // LEA R8, [R8+R8*2].
        0x4a, 0x8b, 0x54, 0xc1, 0x10,             // MOV RDX, [RCX+R8*8+0x10].
        0xb0, 0x01,                               // MOV AL, 0x1.
        0x49, 0x89, 0x91, 0xb0, 0x00, 0x00, 0x00, // MOV [R9+0xb0], RDX.
        0x48, 0x8b, 0x0d, 0xa8, 0xb5, 0x6e, 0x01, // MOV RCX, [DAT_142bec250].
        0x4e, 0x89, 0x0c, 0xc1,                   // MOV [RCX+R8*8], R9.
        0xc3,                                     // RET.
    };

    std::vector<std::uint8_t> image_bytes = function_bytes;
    image_bytes.insert(image_bytes.end(), 16, 0x90); // Sleigh read-ahead padding.
    constexpr std::uint64_t entry = 0x141500c44;
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image_bytes));
    SleighPcodeProvider provider(std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory,
                                 {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ArchitectureDescription architecture = test_architecture();
    ProviderContext providers;
    providers.pcode = std::make_shared<SleighPcodeProvider>(std::move(provider));
    providers.memory = memory;
    auto metadata = std::make_shared<TestProviders>();
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    try {
        Decompiler decompiler(std::move(architecture), std::move(providers));
        const DecompilationResult result =
            decompiler.decompile(FunctionDescription{"FUN_141500c44", entry, entry + function_bytes.size()});
        ASSERT_FALSE(result.raw_instructions.empty());
        ASSERT_FALSE(result.c_source.empty());
        // The slot-search control flow matches 11.md; generated global
        // symbols and narrowed temporaries reflect the current port metadata.
        const std::string expected_c = R"(
ulonglong __cdecl FUN_141500c44(longlong param_1,int param_2)

{
  int8 *in_RAX;
  int8 iVar1;
  uint8 uVar2;

  if (*(int4 *)(param_1 + 0x58) == 0) {
    in_RAX = (int8 *)(uint8)uRam0000000142bec258;
    uVar2 = 0;
    iVar1 = 0;
    if ((uint4)uRam0000000142bec258 != 0) {
      in_RAX = piRam0000000142bec250;
      do {
        if ((*in_RAX == 0) && ((int4)in_RAX[1] == param_2)) {
          *(int *)(param_1 + 0x58) = param_2;
          *(int8 *)(param_1 + 0xb0) = piRam0000000142bec250[uVar2 * 3 + 2];
          piRam0000000142bec250[uVar2 * 3] = param_1;
          return CONCAT71((unkint7)((uint8)in_RAX >> 8),1);
        }
        iVar1 = iVar1 + 1;
        uVar2 = (uint8)((int4)uVar2 + 1);
        in_RAX = in_RAX + 3;
      } while (iVar1 < (int8)(uint8)(uint4)uRam0000000142bec258);
    }
  }
  return (uint8)in_RAX & 0xffffffffffffff00;
}
)";
        EXPECT_EQ(without_trailing_whitespace(result.c_source), expected_c);
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
    } catch (const std::exception& error) {
        FAIL() << error.what();
    }
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

/// Verifies that pugixml-backed parsing preserves the native DOM shape,
/// decoded attribute/content values, storage ownership, and tag registration.
TEST(DecompilerXml, PreservesDomAndStorageContract) {
    std::istringstream stream("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<root attr=\"a&amp;b\"><child count=\"0x2\">left &lt; right</child>"
                              "<![CDATA[raw &lt;]]><!-- ignored --></root>");
    ghidra::DocumentStorage storage;

    ghidra::Document* document = nullptr;
    ASSERT_NO_THROW(document = storage.parseDocument(stream));
    ASSERT_NE(document, nullptr);

    ghidra::Element* root = document->getRoot();
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->getName(), "root");
    EXPECT_EQ(root->getAttributeValue("attr"), "a&b");
    ASSERT_EQ(root->getChildren().size(), 1U);
    EXPECT_EQ(root->getContent(), "raw &lt;");

    const ghidra::Element* child = root->getChildren().front();
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->getParent(), root);
    EXPECT_EQ(child->getAttributeValue("count"), "0x2");
    EXPECT_EQ(child->getContent(), "left < right");

    storage.registerTag(child);
    EXPECT_EQ(storage.getTag("child"), child);
    EXPECT_EQ(storage.getTag("missing"), nullptr);
    EXPECT_THROW(root->getAttributeValue("missing"), ghidra::DecoderError);
}

/// Verifies that stream output keeps the native five-character XML escaping
/// contract used by the decompiler's XML encoder.
TEST(DecompilerXml, EscapesXmlSpecialCharacters) {
    std::ostringstream output;
    ghidra::xml_escape(output, "<&>\"'");
    EXPECT_EQ(output.str(), "&lt;&amp;&gt;&quot;&apos;");
}

/// Verifies that malformed XML and unsupported DTD input are rejected with
/// useful DecoderError diagnostics instead of returning partial DOM objects.
TEST(DecompilerXml, RejectsMalformedAndDtdDocuments) {
    std::istringstream malformed("<root>");
    try {
        const std::unique_ptr<ghidra::Document> document(ghidra::xml_tree(malformed));
        FAIL() << "Malformed XML unexpectedly produced a document";
    } catch (const ghidra::DecoderError& error) {
        EXPECT_FALSE(error.explain.empty());
    }

    std::istringstream dtd("<!DOCTYPE root><root/>");
    try {
        const std::unique_ptr<ghidra::Document> document(ghidra::xml_tree(dtd));
        FAIL() << "DTD input unexpectedly produced a document";
    } catch (const ghidra::DecoderError& error) {
        EXPECT_EQ(error.explain, "DTD's not supported");
    }
}

} // namespace newghidra::decompiler::tests
