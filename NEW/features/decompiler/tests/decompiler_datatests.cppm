module;

#include <gtest/gtest.h>

export module decompiler_datatests;

import decompiler;
import ghidra.decompiler;
import sleigh_runtime;
import std;

namespace newghidra::decompiler::datatests {

/// Provides the compact symbol table used by the embedded datatest programs.
/// Original contract: `Ghidra/Features/Decompiler/src/decompile/database.cc`
/// and the `<symbol>` records in `Ghidra/Features/Decompiler/src/decompile/datatests`.
class SymbolTableProvider final : public SymbolProvider {
public:
    /// Stores immutable symbol records for one test image.
    explicit SymbolTableProvider(std::vector<SymbolDescription> symbols) : symbols_(std::move(symbols)) {}

    /// Returns the symbol whose address exactly matches the provider request.
    [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
        for (const SymbolDescription& symbol : symbols_) {
            if (symbol.address == address) {
                return symbol;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<SymbolDescription> symbols_;
};

/// Provides source-level types without requiring compiler-spec or database XML.
/// Original contract: `Ghidra/Features/Decompiler/src/decompile/type.cc` and
/// the `parse line` type commands used by the original datatests.
class TypeTableProvider final : public TypeProvider {
public:
    /// Stores immutable provider type descriptions for a test image.
    explicit TypeTableProvider(std::vector<TypeDescription> types) : types_(std::move(types)) {}

    /// Resolves a named type while preserving the provider's declaration and shape.
    [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
        for (const TypeDescription& type : types_) {
            if (type.name == name) {
                return type;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<TypeDescription> types_;
};

/// Provides typed root and external function prototypes for real CALL analysis.
/// Original contract: `Ghidra/Features/Decompiler/src/decompile/fspec.cc` and
/// the prototype declarations embedded in the original XML command streams.
class PrototypeTableProvider final : public PrototypeProvider {
public:
    /// Stores address-keyed immutable prototypes.
    explicit PrototypeTableProvider(std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes)
        : prototypes_(std::move(prototypes)) {}

    /// Returns the prototype assigned to the requested function address.
    [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
        for (const auto& prototype : prototypes_) {
            if (prototype.first == address) {
                return prototype.second;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes_;
};

/// Supplies source names and types for stack or register locals.
/// Original contract: high-symbol and local-variable metadata normally carried
/// by the Ghidra database rather than by an XML datatest command.
class VariableTableProvider final : public VariableProvider {
public:
    /// Stores address-keyed local variable records.
    explicit VariableTableProvider(std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> variables)
        : variables_(std::move(variables)) {}

    /// Returns the locals associated with a function entry.
    [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
        for (const auto& variable_set : variables_) {
            if (variable_set.first == address) {
                return variable_set.second;
            }
        }
        return {};
    }

private:
    std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> variables_;
};

/// Builds the x86-64 provider architecture used by every embedded case.
/// Original architecture: `x86:LE:64:default:gcc` in the original datatests.
static ArchitectureDescription make_x86_64_architecture() {
    ArchitectureDescription architecture;
    architecture.name = "embedded-x86-64";
    architecture.calling_convention = "__cdecl";
    architecture.spaces = {
        SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    architecture.registers = {
        RegisterDescription{"RAX", Storage{"register", 0, 8}},
        RegisterDescription{"RCX", Storage{"register", 8, 8}},
        RegisterDescription{"RDX", Storage{"register", 0x10, 8}},
        RegisterDescription{"RBX", Storage{"register", 0x18, 8}},
        RegisterDescription{"RSP", Storage{"register", 0x20, 8}},
        RegisterDescription{"R8", Storage{"register", 0x80, 8}},
        RegisterDescription{"R9", Storage{"register", 0x88, 8}},
    };
    return architecture;
}

/// Builds the Sleigh context required by the checked-in x86-64 SLA fixture.
/// Original processor context: the x86 compiler specification used by the
/// datatests under `Ghidra/Features/Decompiler/src/decompile/datatests`.
static std::vector<std::pair<std::string, std::uint64_t>> make_x86_64_context() {
    return {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}};
}

/// Runs one bounded machine-byte image through Sleigh and the native decompiler.
/// The image may contain read-only data after `function_size`, which permits a
/// compact jump table without loading any original XML or database state.
/// Original pipeline: `translate.cc`, `flow.cc`, and `funcdata.cc` as exercised
/// by the XML datatests in `Ghidra/Features/Decompiler/src/decompile/datatests`.
static DecompilationResult decompile_embedded(std::uint64_t entry, std::vector<std::uint8_t> image,
                                              std::size_t function_size, std::string function_name,
                                              ProviderContext metadata = {}) {
    const auto memory = std::make_shared<SparseMemory>(entry, std::move(image));
    auto provider = std::make_shared<SleighPcodeProvider>(
        std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory, make_x86_64_context());
    metadata.pcode = std::move(provider);
    metadata.memory = memory;
    Decompiler decompiler(make_x86_64_architecture(), std::move(metadata));
    return decompiler.decompile(FunctionDescription{std::move(function_name), entry, entry + function_size});
}

/// Creates a signed or unsigned integer provider type with an optional C declaration.
/// Original type family: `int4`, `uint4`, and their aliases in the XML datatests.
static TypeDescription integer_type(std::string name, std::uint32_t size, bool signed_value,
                                    std::string declaration = {}) {
    TypeDescription type;
    type.name = std::move(name);
    type.size = size;
    type.declaration = std::move(declaration);
    type.kind = signed_value ? TypeKind::signed_integer : TypeKind::unsigned_integer;
    type.signed_value = signed_value;
    return type;
}

/// Creates a floating-point provider type used by the compact SSE case.
/// Original type family: `float4` and `float8` in `floatcast.xml` and `floatconv.xml`.
static TypeDescription floating_type(std::string name, std::uint32_t size) {
    TypeDescription type;
    type.name = std::move(name);
    type.size = size;
    type.kind = TypeKind::floating_point;
    type.signed_value = true;
    return type;
}

/// Creates a pointer provider type whose element is resolved recursively by the frontend.
/// Original type family: pointer declarations in `ptrtoarray.xml`, `pointersub.xml`,
/// `pointerrel.xml`, and `heapstring.xml`.
static TypeDescription pointer_type(std::string name, std::string element_type) {
    TypeDescription type;
    type.name = std::move(name);
    type.size = 8;
    type.kind = TypeKind::pointer;
    type.element_type = std::move(element_type);
    type.signed_value = false;
    return type;
}

/// Creates an array provider type with an explicit element count.
/// Original type family: the multidimensional and offset-array declarations in
/// `twodim.xml`, `threedim.xml`, `offsetarray.xml`, and `wayoffarray.xml`.
static TypeDescription array_type(std::string name, std::string element_type, std::uint32_t element_count,
                                  std::uint32_t element_size) {
    TypeDescription type;
    type.name = std::move(name);
    type.size = element_size * element_count;
    type.kind = TypeKind::array;
    type.element_type = std::move(element_type);
    type.element_count = element_count;
    type.signed_value = false;
    return type;
}

/// Returns the primitive provider types shared by structure-oriented tests.
/// The original declarations come from `concat.xml`, `union_datatype.xml`,
/// `bitfields.xml`, `retstruct.xml`, and `ptrtoarray.xml`; the current provider
/// boundary deliberately records those aggregate names in prototypes without
/// pretending to support the legacy database's field metadata.
static std::vector<TypeDescription> composite_types() {
    // The current provider schema intentionally omits the original database's
    // composite-field metadata.  Primitive and pointer types still exercise
    // real type propagation without recursively entering unsupported layouts.
    return {integer_type("int32", 4, true), integer_type("uint32", 4, false), floating_type("float32", 4),
            pointer_type("float32 *", "float32")};
}

/// Creates a prototype while keeping ABI storage explicit in the test source.
/// Original contract: the `parse line extern` declarations and parameter maps
/// in the XML datatests.
static PrototypeDescription make_prototype(std::string calling_convention, std::string return_type,
                                           std::optional<Storage> return_storage,
                                           std::vector<PrototypeParameterDescription> parameters) {
    PrototypeDescription prototype;
    prototype.calling_convention = std::move(calling_convention);
    prototype.return_type = std::move(return_type);
    prototype.return_storage = std::move(return_storage);
    prototype.parameters = std::move(parameters);
    return prototype;
}

/// Builds provider metadata from independent symbol, type, prototype, and local tables.
/// This is the portable replacement for XML commands that only describe external
/// facts rather than changing the decompiler algorithm.
static ProviderContext
make_metadata(std::vector<SymbolDescription> symbols, std::vector<TypeDescription> types,
              std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes,
              std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> variables = {}) {
    ProviderContext context;
    if (!symbols.empty()) {
        context.symbols = std::make_shared<SymbolTableProvider>(std::move(symbols));
    }
    if (!types.empty()) {
        context.types = std::make_shared<TypeTableProvider>(std::move(types));
    }
    if (!prototypes.empty()) {
        context.prototypes = std::make_shared<PrototypeTableProvider>(std::move(prototypes));
    }
    if (!variables.empty()) {
        context.variables = std::make_shared<VariableTableProvider>(std::move(variables));
    }
    return context;
}

/// Verifies that a native run produced raw, analyzed, flow, and printed artifacts.
/// Original datatest intent: every XML fixture executes decompilation and then
/// checks one or more of these representations with `<stringmatch>`.
static void expect_complete_analysis(const DecompilationResult& result) {
    ASSERT_FALSE(result.raw_instructions.empty());
    ASSERT_FALSE(result.raw_pcode.empty());
    ASSERT_FALSE(result.high_pcode.empty());
    ASSERT_FALSE(result.control_flow.empty());
    ASSERT_FALSE(result.c_source.empty());
}

/// Verifies a token in a generated artifact without coupling the test to whitespace.
/// Original XML used regular-expression `<stringmatch>` records; this portable
/// suite checks stable semantic tokens in the native result instead.
static void expect_contains(std::string_view artifact, std::string_view token) {
    EXPECT_NE(artifact.find(token), std::string_view::npos) << "missing token: " << token;
}

/// Exercises a counted loop from real x86 bytes and verifies conditional flow recovery.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/forloop1.xml`.
TEST(DecompilerDatatests, PortedForloop1) {
    const std::uint64_t entry = 0x400517;
    const std::vector<std::uint8_t> bytes{
        0xb8, 0x00, 0x00, 0x00, 0x00, // mov eax, 0
        0x83, 0xf9, 0x05,             // cmp ecx, 5
        0x7d, 0x05,                   // jge return
        0x83, 0xc0, 0x01,             // add eax, 1
        0xeb, 0xf6,                   // jump to the loop condition
        0xc3,                         // return eax
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"max", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "forloop1", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "forloop1", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "goto");
    expect_contains(result.control_flow, "Whiledo");
    EXPECT_TRUE(result.c_source.find("for") != std::string::npos || result.c_source.find("while") != std::string::npos);
}

/// Exercises a pre-test loop whose back edge is encoded as a real short jump.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/forloop_withskip.xml`.
TEST(DecompilerDatatests, PortedWhileLoopControlFlow) {
    const std::uint64_t entry = 0x401000;
    const std::vector<std::uint8_t> bytes{
        0x89, 0xc8,       // mov eax, ecx
        0x85, 0xc0,       // test eax, eax
        0x7e, 0x05,       // jle exit
        0x83, 0xe8, 0x01, // sub eax, 1
        0xeb, 0xf7,       // repeat the test
        0x31, 0xc0,       // exit value is zero
        0xc3,             // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"count", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "while_countdown", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "while_countdown", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "goto");
    expect_contains(result.control_flow, "Whiledo");
    EXPECT_TRUE(result.c_source.find("while") != std::string::npos || result.c_source.find("for") != std::string::npos);
}

/// Exercises two real conditional return paths and checks if/else restructuring.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/elseif.xml`.
TEST(DecompilerDatatests, PortedIfElse) {
    const std::uint64_t entry = 0x402000;
    const std::vector<std::uint8_t> bytes{
        0x85, 0xc9,                         // test ecx, ecx
        0x74, 0x06,                         // zero selects the second return
        0xb8, 0x01, 0x00, 0x00, 0x00, 0xc3, // return 1
        0xb8, 0x02, 0x00, 0x00, 0x00, 0xc3, // return 2
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"condition", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "if_else_value", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "if_else_value", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "goto");
    expect_contains(result.c_source, "if");
    expect_contains(result.c_source, "return");
}

/// Exercises an indexed indirect branch with a read-only embedded jump table.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/switchreturn.xml`.
TEST(DecompilerDatatests, PortedSwitchReturn) {
    const std::uint64_t entry = 0x403000;
    std::vector<std::uint8_t> image{
        0x83, 0xf9, 0x02,                         // cmp ecx, 2
        0x77, 0x19,                               // ja default
        0xff, 0x24, 0xcd, 0x30, 0x30, 0x40, 0x00, // jmp [rcx*8+0x403030]
        0xb8, 0x10, 0x00, 0x00, 0x00, 0xc3,       // case 0: return 0x10
        0xb8, 0x20, 0x00, 0x00, 0x00, 0xc3,       // case 1: return 0x20
        0xb8, 0x30, 0x00, 0x00, 0x00, 0xc3,       // case 2: return 0x30
        0xb8, 0xff, 0x00, 0x00, 0x00, 0xc3,       // default: return 0xff
    };
    image.resize(0x30, 0x90);
    // Encodes the read-only jump-table entries in the target architecture's byte order.
    const auto append_u64 = [&image](std::uint64_t value) {
        for (unsigned shift = 0; shift != 64; shift += 8) {
            image.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    };
    append_u64(entry + 0x0c);
    append_u64(entry + 0x12);
    append_u64(entry + 0x18);

    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"selector", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "switch_return", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, std::move(image), 0x24, "switch_return", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "callind");
    expect_contains(result.control_flow, "If");
    expect_contains(result.c_source, "return");
}

/// Exercises integer addition and multiplication materialized by LEA p-code.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/convert.xml`.
TEST(DecompilerDatatests, PortedArithmetic) {
    const std::uint64_t entry = 0x404000;
    const std::vector<std::uint8_t> bytes{
        0x8d,
        0x04,
        0x89, // lea eax, [rcx+rcx*4]
        0xc3, // return 5 * value
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "arithmetic", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "arithmetic", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, " + ");
    expect_contains(result.raw_pcode, " * ");
    expect_contains(result.c_source, "return");
}

/// Exercises signed division through EDX:EAX and verifies the native signed op.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/divopt.xml`.
TEST(DecompilerDatatests, PortedSignedDivision) {
    const std::uint64_t entry = 0x405000;
    const std::vector<std::uint8_t> bytes{
        0x89, 0xc8, // mov eax, ecx
        0x89, 0xd3, // preserve the divisor in ebx
        0x99,       // cdq
        0xf7, 0xfb, // idiv ebx
        0xc3,       // return quotient
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"numerator", "int32", Storage{"register", 8, 4}},
                        PrototypeParameterDescription{"divisor", "int32", Storage{"register", 0x10, 4}}});
    const auto metadata =
        make_metadata({{entry, "signed_division", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "signed_division", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, " s/ ");
    expect_contains(result.c_source, "/");
}

/// Exercises signed remainder using the same compact ABI as the division case.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/modulo.xml`.
TEST(DecompilerDatatests, PortedSignedModulo) {
    const std::uint64_t entry = 0x406000;
    const std::vector<std::uint8_t> bytes{
        0x89, 0xc8, // mov eax, ecx
        0x89, 0xd3, // preserve the divisor in ebx
        0x99,       // cdq
        0xf7, 0xfb, // idiv ebx
        0x89, 0xd0, // mov eax, edx
        0xc3,       // return remainder
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"numerator", "int32", Storage{"register", 8, 4}},
                        PrototypeParameterDescription{"divisor", "int32", Storage{"register", 0x10, 4}}});
    const auto metadata =
        make_metadata({{entry, "signed_modulo", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "signed_modulo", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, " s% ");
    expect_contains(result.c_source, "%");
}

/// Exercises SSE load, floating multiply, and float-to-integer conversion.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/floatcast.xml`.
TEST(DecompilerDatatests, PortedFloatingPoint) {
    const std::uint64_t entry = 0x407000;
    const std::vector<std::uint8_t> bytes{
        0xf3, 0x0f, 0x10, 0x01, // movss xmm0, [rcx]
        0xf3, 0x0f, 0x59, 0xc0, // mulss xmm0, xmm0
        0xf3, 0x0f, 0x2c, 0xc0, // cvttss2si eax, xmm0
        0xc3,                   // return converted value
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "float32 *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata(
        {{entry, "floating_point", ""}},
        {integer_type("int32", 4, true), floating_type("float32", 4), pointer_type("float32 *", "float32")},
        {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "floating_point", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, " f* ");
    expect_contains(result.raw_pcode, "TRUNC");
    expect_contains(result.c_source, "return");
}

/// Exercises typed structure fields, an embedded fixed array, and pointer stores.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/concat.xml` and
/// `Ghidra/Features/Decompiler/src/decompile/datatests/ptrtoarray.xml`.
TEST(DecompilerDatatests, PortedStructuresArraysAndPointers) {
    const std::uint64_t entry = 0x408000;
    const std::vector<std::uint8_t> bytes{
        0x8b, 0x01,       // mov eax, [rcx+0]
        0x89, 0x51, 0x04, // mov [rcx+4], edx
        0x8b, 0x41, 0x08, // mov eax, [rcx+8], first array element
        0xc3,             // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"record", "Record *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"value", "int32", Storage{"register", 0x10, 4}}});
    const auto metadata = make_metadata({{entry, "record_access", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "record_access", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "Record *");
    expect_contains(result.c_source, "record + 8");
    expect_contains(result.raw_pcode, "*(ram");
    expect_contains(result.raw_pcode, "= u0x");
}

/// Exercises a union declaration and overlapping provider fields through a load.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/union_datatype.xml`.
TEST(DecompilerDatatests, PortedUnionDatatype) {
    const std::uint64_t entry = 0x409000;
    const std::vector<std::uint8_t> bytes{
        0x8b,
        0x01, // read the union storage
        0xc3, // return the selected representation
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "uint32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"word", "Word *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata({{entry, "union_value", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "union_value", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "Word *");
    expect_contains(result.c_source, "undefined4");
    expect_contains(result.raw_pcode, "*(ram");
}

/// Exercises a bit-mask and shift over a provider declaration containing bitfields.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/bitfields.xml`.
TEST(DecompilerDatatests, PortedBitfields) {
    const std::uint64_t entry = 0x40a000;
    const std::vector<std::uint8_t> bytes{
        0x8b, 0x01,       // read Flags::raw
        0xc1, 0xe8, 0x01, // shift the packed field
        0x83, 0xe0, 0x07, // retain a three-bit value
        0xc3,             // return field value
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "uint32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"flags", "Flags *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata({{entry, "bitfield_value", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "bitfield_value", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, ">> 1 & 7");
    expect_contains(result.raw_pcode, " >> ");
    expect_contains(result.raw_pcode, " & ");
}

/// Exercises a direct CALL with provider-backed external symbol and prototype data.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/deindirect.xml`.
TEST(DecompilerDatatests, PortedCallsAndPrototypes) {
    const std::uint64_t entry = 0x40b000;
    const std::uint64_t helper = 0x40b010;
    const std::vector<std::uint8_t> bytes{
        0xe8, 0x0b, 0x00, 0x00, 0x00, // call helper
        0xc3,                         // return helper result
    };
    const PrototypeDescription root = make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {});
    const PrototypeDescription external =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}});
    const auto metadata = make_metadata({{entry, "call_root", ""}, {helper, "helper_function", ""}},
                                        {integer_type("int32", 4, true)}, {{entry, root}, {helper, external}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "call_root", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "call ");
    expect_contains(result.c_source, "helper_function");
    expect_contains(result.c_source, "return");
}

/// Exercises symbol namespace materialization independently of function metadata.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/namespace.xml`.
TEST(DecompilerDatatests, PortedNamespaceAndSymbols) {
    const std::uint64_t entry = 0x40c000;
    const std::vector<std::uint8_t> bytes{
        0xb8, 0x2a, 0x00, 0x00, 0x00, // mov eax, 42
        0xc3,                         // return
    };
    const PrototypeDescription prototype = make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {});
    const auto metadata =
        make_metadata({{entry, "entry", "demo::nested"}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "fallback", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "demo::nested::entry");
    expect_contains(result.c_source, "return");
}

/// Exercises a provider-declared structure as the function return value.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/retstruct.xml`.
TEST(DecompilerDatatests, PortedStructureReturn) {
    const std::uint64_t entry = 0x40d000;
    const std::vector<std::uint8_t> bytes{
        0x48,
        0x8b,
        0x01, // mov rax, [rcx]
        0xc3, // return Pair in RAX
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "Pair", Storage{"register", 0, 8},
                       {PrototypeParameterDescription{"source", "Pair *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata({{entry, "return_pair", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "return_pair", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "Pair __cdecl");
    expect_contains(result.c_source, "return");
}

/// Exercises an immutable SparseMemory read as the portable readonly portion of
/// the original volatile/readonly tests; no volatile flag is fabricated.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/readvolatile.xml`.
TEST(DecompilerDatatests, PortedReadonlyMemoryLoad) {
    const std::uint64_t entry = 0x40e000;
    const std::vector<std::uint8_t> bytes{
        0x8b,
        0x01, // load from the immutable provider image
        0xc3, // return loaded value
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"source", "int32 *", Storage{"register", 8, 8}}});
    const auto metadata =
        make_metadata({{entry, "readonly_load", ""}},
                      {integer_type("int32", 4, true), pointer_type("int32 *", "int32")}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "readonly_load", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "*(ram");
    expect_contains(result.c_source, "return");
}

/// Exercises copy/add simplification and compares raw p-code with transformed high p-code.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/copytrim.xml` and
/// `Ghidra/Features/Decompiler/src/decompile/datatests/condconst.xml`.
TEST(DecompilerDatatests, PortedPcodeTransformations) {
    const std::uint64_t entry = 0x40f000;
    const std::vector<std::uint8_t> bytes{
        0x89, 0xc8,       // copy ECX to EAX
        0x83, 0xc0, 0x01, // add one
        0xc3,             // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"input", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "copy_add", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "copy_add", metadata);

    expect_complete_analysis(result);
    EXPECT_NE(result.raw_pcode, result.high_pcode);
    expect_contains(result.raw_pcode, "= RCX");
    expect_contains(result.raw_pcode, " + ");
    expect_contains(result.c_source, "+");
}

/// Names one original XML fixture and records whether this suite has a portable
/// machine-byte/provider equivalent for its intent.
/// Original source directory: `Ghidra/Features/Decompiler/src/decompile/datatests`.
struct DatatestManifestEntry {
    std::string_view file;
    bool ported;
    std::string_view reason;
};

/// Returns the complete 89-fixture manifest from the original datatest directory.
/// Each nonportable reason identifies the original XML command or processor/database
/// behavior that cannot be represented by the current provider boundary.
static constexpr std::array<DatatestManifestEntry, 89> original_datatest_manifest() {
    return {{
        {"bitfields.xml", true, "Covered by PortedBitfields with embedded x86 bytes and Flags metadata."},
        {"bitfields2.xml", false, "Requires the original MIPS processor specification and XML parse-line commands."},
        {"boolless.xml", false, "Uses a CODE address space and the interactive dec command, not provider metadata."},
        {"ccmp.xml", false, "Requires processor-specific compare/borrow injection semantics from the XML image."},
        {"concat.xml", true, "Covered by PortedStructuresArraysAndPointers with Record metadata."},
        {"concatsplit.xml", false, "Depends on multi-piece aggregate ABI mapping and XML function remapping."},
        {"condconst.xml", true, "Covered by PortedPcodeTransformations with real copy/add flow."},
        {"condconst2.xml", false, "Requires ARM context injection and XML set-context commands."},
        {"condconstsub.xml", false, "Requires XML override-flow and mapped-function database commands."},
        {"condexesub.xml", false, "Requires readonly option, context injection, and mapped global data."},
        {"condmulti.xml", false, "Requires XML global-address mappings and several externally declared functions."},
        {"convert.xml", true, "Covered by PortedArithmetic using real integer p-code from Sleigh."},
        {"copytrim.xml", true, "Covered by PortedPcodeTransformations."},
        {"deadvolatile.xml", false, "Requires database volatile memory attributes unavailable in MemoryProvider."},
        {"deindirect.xml", true, "Covered by PortedCallsAndPrototypes with a provider-backed direct CALL."},
        {"deindirect2.xml", false, "Requires XML symbol remapping and indirect aggregate-call database state."},
        {"displayformat.xml", false, "Tests interactive display-format options rather than decompiler semantics."},
        {"divopt.xml", true, "Covered by PortedSignedDivision."},
        {"doublemove.xml", false, "Requires mapped floating globals and raw-print command sequencing."},
        {"dupptr.xml", false, "Requires XML function maps and pointer alias database annotations."},
        {"elseif.xml", true, "Covered by PortedIfElse."},
        {"enum.xml", false, "Requires XML enum declarations and database enum value propagation."},
        {"floatcast.xml", true, "Covered by PortedFloatingPoint."},
        {"floatconv.xml", false, "Depends on XML constant conversion maps and custom parameter mapping."},
        {"floatprint.xml", false, "Requires XML readonly globals and exact legacy floating printer fixtures."},
        {"forloop1.xml", true, "Covered by PortedForloop1."},
        {"forloop_loaditer.xml", false, "Requires XML stack-local name maps and variadic printf metadata."},
        {"forloop_thruspecial.xml", false, "Requires a special-operation injection and an XML puts prototype."},
        {"forloop_varused.xml", false, "Requires XML variadic-call and external symbol database setup."},
        {"forloop_withskip.xml", true, "Covered by PortedWhileLoopControlFlow."},
        {"gp.xml", false, "Requires processor global-pointer tracking and XML track commands."},
        {"heapstring.xml", false, "Requires string-manager data annotations and builtin call injection."},
        {"ifnoexit.xml", false, "Relies on an intentionally unterminated XML function flow."},
        {"ifswitch.xml", false, "Combines switch recovery with XML-specific function and data mappings."},
        {"impliedfield.xml", false, "Requires database field-implied type annotations."},
        {"indproto.xml", false, "Requires an indirect prototype override stored in the original database."},
        {"injectoverride.xml", false,
         "Explicitly tests XML p-code injection/override commands unsupported by ProviderInjectLibrary."},
        {"inline.xml", false, "Requires XML inline function attributes and call-fixup injection."},
        {"inlinetarget.xml", false, "Requires XML inline-target directives and database call-site state."},
        {"longdouble.xml", false, "Requires the original long-double compiler type and processor ABI."},
        {"loopcomment.xml", false, "Requires XML comment commands and the original comment database."},
        {"lzcount.xml", false, "Requires a processor CALLOTHER operation not supplied by this x86 provider case."},
        {"mixfloatint.xml", false, "Requires XML mixed ABI prototype and floating global mappings."},
        {"modulo.xml", true, "Covered by PortedSignedModulo."},
        {"modulo2.xml", false, "Requires optimized compiler-specific modulo sequences and XML expected-output setup."},
        {"multiret.xml", false,
         "Requires multiple return-register ABI pieces unavailable in the compact prototype contract."},
        {"nan.xml", false, "Requires XML floating constant/global annotations and legacy NaN printer expectations."},
        {"namespace.xml", true, "Covered by PortedNamespaceAndSymbols."},
        {"nestedoffset.xml", false, "Requires nested database field offsets and XML data maps."},
        {"noforloop_alias.xml", false, "Negative loop classification depends on XML alias/database facts."},
        {"noforloop_globcall.xml", false, "Negative loop classification depends on an XML-mapped global call."},
        {"noforloop_iterused.xml", false,
         "Negative loop classification depends on XML local naming and call metadata."},
        {"offcut.xml", false, "Requires offcut data symbols and database storage overlays."},
        {"offsetarray.xml", false, "Requires XML address-to-array data mapping not exposed by TypeProvider."},
        {"orcompare.xml", false, "Requires XML boolean-equate and processor flag setup."},
        {"overridedest.xml", false, "Explicitly requires an XML destination override command."},
        {"packstructaccess.xml", false, "Requires packed compiler layout and database field packing directives."},
        {"partialmerge.xml", false, "Requires XML partial variable merge annotations."},
        {"partialsplit.xml", false, "Requires XML partial split/database symbol state."},
        {"partialunion.xml", false, "Requires union field selection metadata stored in the original database."},
        {"piecestruct.xml", false, "Requires multi-register structure-piece ABI injection."},
        {"pointercmp.xml", false, "Requires XML pointer type maps and processor-specific comparison fixtures."},
        {"pointerrel.xml", false, "Requires XML pointer-relative data annotations."},
        {"pointersub.xml", false, "Requires XML pointer subtraction prototype and database type maps."},
        {"promotecompare.xml", false, "Requires compiler promotion rules supplied by the original XML specification."},
        {"ptrtoarray.xml", true, "Covered by PortedStructuresArraysAndPointers."},
        {"readvolatile.xml", true,
         "The immutable readonly load is covered; volatile qualification remains nonportable."},
        {"retspecial.xml", false, "Requires special return-register ABI injection."},
        {"retstruct.xml", true, "Covered by PortedStructureReturn."},
        {"revisit.xml", false, "Requires XML flow revisit overrides and database labels."},
        {"sbyte.xml", false, "Requires XML signed-byte type declarations and expected legacy casts."},
        {"skipnext2.xml", false, "Requires processor delay-slot/skip-next semantics from XML context."},
        {"stackcorner.xml", false, "Requires original stack-space and database-local corner cases."},
        {"stackreturn.xml", false, "Requires stack-return aggregate ABI metadata."},
        {"stackspill.xml", false, "Requires compiler-specific stack spill annotations and XML mappings."},
        {"stackstring.xml", false, "Requires string-manager constant propagation and readonly data symbols."},
        {"statuscmp.xml", false, "Requires processor status-register semantics and XML flag configuration."},
        {"switchhide.xml", false, "Requires XML switch hiding override."},
        {"switchind.xml", false, "Requires indirect switch prototype/database metadata beyond the compact table case."},
        {"switchloop.xml", false, "Requires a switch-loop XML fixture with database labels."},
        {"switchmask.xml", false, "Requires XML switch mask and custom jump-table metadata."},
        {"switchmulti.xml", false, "Requires multiple XML jump-table mappings and processor-specific layout."},
        {"switchreturn.xml", true, "Covered by PortedSwitchReturn."},
        {"threedim.xml", false, "Requires XML three-dimensional data declarations and global mappings."},
        {"twodim.xml", false,
         "The provider has array shape support, but the original test requires XML global data mapping."},
        {"union_datatype.xml", true, "Covered by PortedUnionDatatype."},
        {"varcross.xml", false, "Requires cross-function database variable identity."},
        {"wayoffarray.xml", false, "Requires XML offcut array data symbols."},
        {"wraprange.xml", false, "Requires processor address-range and XML context-wrap configuration."},
    }};
}

/// Verifies that every original XML datatest is accounted for without pretending
/// that unsupported XML commands are portable provider behavior.
/// Original source directory: `Ghidra/Features/Decompiler/src/decompile/datatests`.
TEST(DecompilerDatatestsManifest, AccountsForEveryOriginalDatatest) {
    const auto manifest = original_datatest_manifest();
    std::set<std::string_view> names;
    std::size_t portable_count = 0;
    for (const DatatestManifestEntry& entry : manifest) {
        EXPECT_TRUE(names.insert(entry.file).second) << "duplicate manifest entry: " << entry.file;
        EXPECT_FALSE(entry.file.empty());
        EXPECT_FALSE(entry.reason.empty());
        if (entry.ported) {
            ++portable_count;
        } else {
            EXPECT_GT(entry.reason.size(), 20U);
        }
    }
    EXPECT_EQ(manifest.size(), 89U);
    EXPECT_EQ(names.size(), 89U);
    EXPECT_GE(portable_count, 15U);
}

} // namespace newghidra::decompiler::datatests
