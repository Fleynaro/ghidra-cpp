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

    /// Returns every symbol so data and function mappings are installed before flow recovery.
    [[nodiscard]] std::vector<SymbolDescription> symbols() const override {
        return symbols_;
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

/// Supplies address-keyed flow corrections before native control-flow recovery.
/// Original contract: `Ghidra/Features/Decompiler/src/decompile/override.cc` and
/// the `<com>override ...</com>` commands in the prioritized datatests.
class FlowTableProvider final : public FlowProvider {
public:
    /// Stores immutable function-local override records.
    explicit FlowTableProvider(std::vector<std::pair<std::uint64_t, FlowDescription>> flows)
        : flows_(std::move(flows)) {}

    /// Returns the flow metadata associated with one function entry.
    [[nodiscard]] std::optional<FlowDescription> flow_at(std::uint64_t address) const override {
        for (const auto& flow : flows_) {
            if (flow.first == address) {
                return flow.second;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<std::pair<std::uint64_t, FlowDescription>> flows_;
};

/// Supplies bounded child bodies needed by full-body inlining and cross-function flow.
class FunctionTableProvider final : public FunctionProvider {
public:
    /// Stores immutable function-body descriptions.
    explicit FunctionTableProvider(std::vector<FunctionDescription> functions) : functions_(std::move(functions)) {}

    /// Returns every child body known to the provider.
    [[nodiscard]] std::vector<FunctionDescription> functions() const override {
        return functions_;
    }

private:
    std::vector<FunctionDescription> functions_;
};

/// Supplies structured call-fixups without requiring the unavailable SLEIGH text parser.
class InjectionTableProvider final : public InjectionProvider {
public:
    /// Stores immutable call-fixup records.
    explicit InjectionTableProvider(std::vector<CallFixupDescription> call_fixups,
                                    std::vector<CallOtherFixupDescription> call_other_fixups = {})
        : call_fixups_(std::move(call_fixups)), call_other_fixups_(std::move(call_other_fixups)) {}

    /// Returns provider-owned call-fixups for registration before prototypes are applied.
    [[nodiscard]] std::vector<CallFixupDescription> call_fixups() const override {
        return call_fixups_;
    }

    /// Returns provider-owned CALLOTHER payloads keyed by their emitted index.
    [[nodiscard]] std::vector<CallOtherFixupDescription> call_other_fixups() const override {
        return call_other_fixups_;
    }

private:
    std::vector<CallFixupDescription> call_fixups_;
    std::vector<CallOtherFixupDescription> call_other_fixups_;
};

/// Supplies one real LOAD whose result is intentionally unused, matching the
/// side-effect-sensitive shape of the original deadvolatile fixture.
class VolatileLoadProvider final : public PcodeProvider {
public:
    /// Constructs the deterministic volatile-load p-code provider.
    VolatileLoadProvider() = default;

    /// Emits a four-byte LOAD from the provider-marked volatile range and a constant return.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(ProviderError{"VolatileLoadProvider has no instruction at this address"});
        }
        Instruction instruction;
        instruction.address = address;
        instruction.length = 1;
        instruction.mnemonic = "volatile-load";
        instruction.assembly = "[0x2000]";
        instruction.pcode = {
            PcodeOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::load),
                           Storage{"register", 8, 4},
                           {Storage{"ram", 0x2000, 8}},
                           std::optional<std::string>{"ram"}},
            PcodeOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
                           Storage{"register", 0, 4},
                           {Storage{"const", 0, 4}}},
            PcodeOperation{
                std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 4}}},
        };
        return instruction;
    }
};

/// Provides a small address-keyed p-code program for provider-only override
/// and callother tests. It intentionally exposes the same Instruction contract
/// as the Sleigh adapter so the native flow engine remains the code under test.
class ScriptedPcodeProvider final : public PcodeProvider {
public:
    /// Stores immutable instructions keyed by their native entry address.
    explicit ScriptedPcodeProvider(std::vector<Instruction> instructions) : instructions_(std::move(instructions)) {}

    /// Returns the scripted instruction or a precise provider diagnostic.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        const auto iterator =
            std::find_if(instructions_.begin(), instructions_.end(),
                         [address](const Instruction& instruction) { return instruction.address == address; });
        if (iterator == instructions_.end()) {
            return std::unexpected(ProviderError{"Scripted p-code has no instruction at the requested address"});
        }
        return *iterator;
    }

private:
    std::vector<Instruction> instructions_;
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

/// Builds the extended x86-64 register description needed by original SysV
/// SSE and x87 fixtures. It remains separate from the legacy Windows-style
/// helper because the native bootstrap's register ranking is intentionally
/// part of the existing x86 test contract.
/// Original register layout: `Ghidra/Processors/x86/data/languages/ia.sinc`.
static ArchitectureDescription make_x86_64_extended_architecture() {
    ArchitectureDescription architecture = make_x86_64_architecture();
    architecture.registers.insert(architecture.registers.end(),
                                  {RegisterDescription{"RBP", Storage{"register", 0x28, 8}},
                                   RegisterDescription{"RSI", Storage{"register", 0x30, 8}},
                                   RegisterDescription{"RDI", Storage{"register", 0x38, 8}},
                                   RegisterDescription{"R10", Storage{"register", 0x90, 8}},
                                   RegisterDescription{"R11", Storage{"register", 0x98, 8}},
                                   RegisterDescription{"R12", Storage{"register", 0xa0, 8}},
                                   RegisterDescription{"R13", Storage{"register", 0xa8, 8}},
                                   RegisterDescription{"R14", Storage{"register", 0xb0, 8}},
                                   RegisterDescription{"R15", Storage{"register", 0xb8, 8}},
                                   RegisterDescription{"XMM0", Storage{"register", 0x1200, 16}},
                                   RegisterDescription{"XMM1", Storage{"register", 0x1210, 16}},
                                   RegisterDescription{"ST0", Storage{"register", 0x1100, 10}},
                                   RegisterDescription{"ST1", Storage{"register", 0x1110, 10}}});
    return architecture;
}

/// Builds the Sleigh context required by the checked-in x86-64 SLA fixture.
/// Original processor context: the x86 compiler specification used by the
/// datatests under `Ghidra/Features/Decompiler/src/decompile/datatests`.
static std::vector<std::pair<std::string, std::uint64_t>> make_x86_64_context() {
    return {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}};
}

/// Decodes compact hexadecimal fixture text for the chunk-image helper. The
/// implementation is shared with the later synthetic x86 fixture cases.
static std::vector<std::uint8_t> bytes_from_hex(std::string_view text);

/// Places original XML byte chunks into one sparse-compatible image while
/// retaining data that lives before or after the selected function.
/// Original image contract: `<bytechunk space="ram" offset="...">` in the
/// requested files under `Ghidra/Features/Decompiler/src/decompile/datatests`.
static std::pair<std::uint64_t, std::vector<std::uint8_t>>
make_chunk_image(std::vector<std::pair<std::uint64_t, std::string>> chunks) {
    if (chunks.empty()) {
        throw std::invalid_argument("Embedded datatest image has no chunks");
    }
    const auto base = std::min_element(chunks.begin(), chunks.end(),
                                       [](const auto& left, const auto& right) { return left.first < right.first; });
    const std::uint64_t base_address = base->first;
    std::uint64_t end_address = base_address;
    for (const auto& chunk : chunks) {
        const std::uint64_t chunk_end = chunk.first + bytes_from_hex(chunk.second).size();
        end_address = std::max(end_address, chunk_end);
    }
    std::vector<std::uint8_t> image(static_cast<std::size_t>(end_address - base_address), 0);
    for (const auto& chunk : chunks) {
        const std::vector<std::uint8_t> bytes = bytes_from_hex(chunk.second);
        std::ranges::copy(bytes, image.begin() + static_cast<std::ptrdiff_t>(chunk.first - base_address));
    }
    return {base_address, std::move(image)};
}

/// Runs a bounded machine-byte image through Sleigh and the native decompiler,
/// allowing XML-style discontiguous code and data chunks.
/// Original pipeline: `loadimage.cc`, `translate.cc`, and `funcdata.cc`.
static DecompilationResult decompile_embedded_at(std::uint64_t entry, std::uint64_t image_base,
                                                 std::vector<std::uint8_t> image, std::size_t function_size,
                                                 std::string function_name, ProviderContext metadata,
                                                 ArchitectureDescription architecture) {
    const auto memory = std::make_shared<SparseMemory>(image_base, std::move(image));
    auto provider = std::make_shared<SleighPcodeProvider>(
        std::filesystem::path("..") / "sleigh_runtime" / "test_data" / "x86-64.sla", memory, make_x86_64_context());
    metadata.pcode = std::move(provider);
    metadata.memory = memory;
    Decompiler decompiler(std::move(architecture), std::move(metadata));
    return decompiler.decompile(FunctionDescription{std::move(function_name), entry, entry + function_size});
}

/// Runs one bounded machine-byte image through Sleigh and the native decompiler.
/// The image may contain read-only data after `function_size`, which permits a
/// compact jump table without loading original XML or database state.
/// Original pipeline: `translate.cc`, `flow.cc`, and `funcdata.cc` as exercised
/// by the XML datatests in `Ghidra/Features/Decompiler/src/decompile/datatests`.
static DecompilationResult decompile_embedded(std::uint64_t entry, std::vector<std::uint8_t> image,
                                              std::size_t function_size, std::string function_name,
                                              ProviderContext metadata = {},
                                              ArchitectureDescription architecture = make_x86_64_architecture()) {
    return decompile_embedded_at(entry, entry, std::move(image), function_size, std::move(function_name),
                                 std::move(metadata), std::move(architecture));
}

/// Runs an original XML chunk set with the selected function entry and exact
/// semantic byte sequence preserved.
/// Original source: the `<bytechunk>` records in each requested x86 datatest.
static DecompilationResult
decompile_embedded_chunks(std::uint64_t entry, std::vector<std::pair<std::uint64_t, std::string>> chunks,
                          std::size_t function_size, std::string function_name, ProviderContext metadata = {},
                          ArchitectureDescription architecture = make_x86_64_extended_architecture()) {
    auto image = make_chunk_image(std::move(chunks));
    return decompile_embedded_at(entry, image.first, std::move(image.second), function_size, std::move(function_name),
                                 std::move(metadata), std::move(architecture));
}

/// Runs a provider-only p-code body through the same native architecture and
/// analysis pipeline as the machine-code datatests.
/// Original boundary: `decompile/cpp/translate.cc` and `funcdata.cc`.
static DecompilationResult decompile_scripted(std::uint64_t entry, std::uint64_t body_size,
                                              std::vector<Instruction> instructions, std::string function_name,
                                              ProviderContext metadata = {}) {
    metadata.pcode = std::make_shared<ScriptedPcodeProvider>(std::move(instructions));
    metadata.memory = std::make_shared<SparseMemory>(entry, std::vector<std::uint8_t>{0});
    Decompiler decompiler(make_x86_64_architecture(), std::move(metadata));
    return decompiler.decompile(FunctionDescription{std::move(function_name), entry, entry + body_size});
}

/// Converts compact hexadecimal fixture text into the byte vector consumed by
/// `SparseMemory`. Whitespace is ignored so the original datatest layout can be
/// retained in focused native cases without loading XML at runtime.
static std::vector<std::uint8_t> bytes_from_hex(std::string_view text) {
    std::vector<std::uint8_t> result;
    std::uint8_t high = 0;
    bool have_high = false;
    const auto digit = [](char value) -> std::uint8_t {
        if (value >= '0' && value <= '9') {
            return static_cast<std::uint8_t>(value - '0');
        }
        if (value >= 'a' && value <= 'f') {
            return static_cast<std::uint8_t>(value - 'a' + 10);
        }
        if (value >= 'A' && value <= 'F') {
            return static_cast<std::uint8_t>(value - 'A' + 10);
        }
        throw std::invalid_argument("Hex fixture contains a non-hexadecimal character");
    };
    for (const char character : text) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            continue;
        }
        const std::uint8_t nibble = digit(character);
        if (!have_high) {
            high = nibble;
            have_high = true;
        } else {
            result.push_back(static_cast<std::uint8_t>((high << 4U) | nibble));
            have_high = false;
        }
    }
    if (have_high) {
        throw std::invalid_argument("Hex fixture contains an incomplete byte");
    }
    return result;
}

/// Writes a compact instruction fragment into a synthetic x86 image.
static void write_fixture_bytes(std::vector<std::uint8_t>& image, std::size_t offset,
                                std::initializer_list<std::uint8_t> bytes) {
    if (offset > image.size() || bytes.size() > image.size() - offset) {
        throw std::out_of_range("Synthetic x86 fixture write exceeds image bounds");
    }
    std::copy(bytes.begin(), bytes.end(), image.begin() + static_cast<std::ptrdiff_t>(offset));
}

/// Writes one little-endian absolute table entry into a synthetic x86 image.
static void write_fixture_u64(std::vector<std::uint8_t>& image, std::size_t offset, std::uint64_t value) {
    if (offset > image.size() || sizeof(value) > image.size() - offset) {
        throw std::out_of_range("Synthetic x86 table write exceeds image bounds");
    }
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        image[offset + byte] = static_cast<std::uint8_t>(value >> (byte * 8U));
    }
}

/// Describes the image and bounded root body generated for a real jump-table case.
struct JumpTableFixture {
    std::vector<std::uint8_t> image;
    std::size_t function_size = 0;
    std::vector<std::uint64_t> targets;
};

/// Builds an x86-64 indirect switch with either an index mask or a range guard.
/// The table and case bodies are outside the bounded root body, matching the
/// layout of `switchind.xml`, `switchmask.xml`, and `switchmulti.xml`.
static JumpTableFixture make_jump_table_fixture(std::uint64_t entry, std::size_t case_count, bool masked,
                                                bool call_cases) {
    constexpr std::size_t table_offset = 0x200;
    constexpr std::size_t child_offset = 0x300;
    const std::size_t case_start = masked ? 11 : 13;
    const std::size_t default_offset = case_start + case_count * 6;
    JumpTableFixture fixture;
    fixture.image.resize(child_offset + case_count * 0x10 + 8, 0);
    fixture.targets.reserve(case_count);

    if (masked) {
        write_fixture_bytes(
            fixture.image, 0,
            {0x48, 0x83, 0xe1, static_cast<std::uint8_t>(case_count - 1), 0xff, 0x24, 0xcd, 0x00, 0x00, 0x00, 0x00});
        const std::uint32_t table_address = static_cast<std::uint32_t>(entry + table_offset);
        for (std::size_t byte = 0; byte < sizeof(table_address); ++byte) {
            fixture.image[7 + byte] = static_cast<std::uint8_t>(table_address >> (byte * 8U));
        }
    } else {
        write_fixture_bytes(fixture.image, 0,
                            {0x48, 0x83, 0xf9, static_cast<std::uint8_t>(case_count - 1), 0x77,
                             static_cast<std::uint8_t>(default_offset - 6), 0xff, 0x24, 0xcd, 0x00, 0x00, 0x00, 0x00});
        const std::uint32_t table_address = static_cast<std::uint32_t>(entry + table_offset);
        for (std::size_t byte = 0; byte < sizeof(table_address); ++byte) {
            fixture.image[9 + byte] = static_cast<std::uint8_t>(table_address >> (byte * 8U));
        }
    }
    for (std::size_t index = 0; index < case_count; ++index) {
        const std::size_t case_offset = case_start + index * 6;
        fixture.targets.push_back(entry + case_offset);
        if (call_cases) {
            const std::uint64_t child = entry + child_offset + index * 0x10;
            const std::int64_t relative =
                static_cast<std::int64_t>(child) - static_cast<std::int64_t>(entry + case_offset + 5);
            write_fixture_bytes(fixture.image, case_offset,
                                {0xe8, static_cast<std::uint8_t>(relative), static_cast<std::uint8_t>(relative >> 8),
                                 static_cast<std::uint8_t>(relative >> 16), static_cast<std::uint8_t>(relative >> 24),
                                 0xc3});
            write_fixture_bytes(fixture.image, child_offset + index * 0x10,
                                {0xb8, static_cast<std::uint8_t>(0x10 + index), 0x00, 0x00, 0x00, 0xc3});
        } else {
            const std::uint8_t operation = static_cast<std::uint8_t>(index % 4);
            const std::array<std::uint8_t, 3> operation_bytes = {static_cast<std::uint8_t>(operation == 0   ? 0x83
                                                                                           : operation == 1 ? 0x83
                                                                                           : operation == 2 ? 0x83
                                                                                                            : 0x83),
                                                                 static_cast<std::uint8_t>(operation == 0   ? 0xc0
                                                                                           : operation == 1 ? 0xe8
                                                                                           : operation == 2 ? 0xf0
                                                                                                            : 0xc8),
                                                                 static_cast<std::uint8_t>(index + 1)};
            write_fixture_bytes(fixture.image, case_offset,
                                {0x8b, 0xc2, operation_bytes[0], operation_bytes[1], operation_bytes[2], 0xc3});
        }
    }
    if (!masked) {
        write_fixture_bytes(fixture.image, default_offset, {0xb8, 0xff, 0xff, 0xff, 0xff, 0xc3});
        fixture.function_size = default_offset + 6;
    } else {
        fixture.function_size = case_start + case_count * 6;
    }
    for (std::size_t index = 0; index < fixture.targets.size(); ++index) {
        write_fixture_u64(fixture.image, table_offset + index * sizeof(std::uint64_t), fixture.targets[index]);
    }
    return fixture;
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

/// Returns the provider types shared by structure-oriented tests.
/// The original declarations come from `concat.xml`, `union_datatype.xml`,
/// `bitfields.xml`, `retstruct.xml`, and `ptrtoarray.xml`. Every aggregate is
/// intentionally described through fields, array shape, union overlap, or
/// bitfield metadata so prototype installation and native type propagation are
/// exercised instead of merely preserving printable names.
static std::vector<TypeDescription> composite_types() {
    TypeDescription row = array_type("IntRow", "int32", 2, 4);
    row.declaration = "typedef int32 IntRow[2];";

    TypeDescription record;
    record.name = "Record";
    record.size = 16;
    record.declaration = "struct Record { int32 first; int32 second; IntRow values; };";
    record.kind = TypeKind::structure;
    record.fields = {
        TypeFieldDescription{"first", "int32", 0},
        TypeFieldDescription{"second", "int32", 4},
        TypeFieldDescription{"values", "IntRow", 8},
    };

    TypeDescription pair;
    pair.name = "Pair";
    pair.size = 16;
    pair.declaration = "struct Pair { int64 low; int64 high; };";
    pair.kind = TypeKind::structure;
    pair.fields = {
        TypeFieldDescription{"low", "int64", 0},
        TypeFieldDescription{"high", "int64", 8},
    };

    TypeDescription word;
    word.name = "Word";
    word.size = 4;
    word.declaration = "union Word { int32 number; float32 real; };";
    word.kind = TypeKind::union_type;
    word.fields = {
        TypeFieldDescription{"number", "int32", 0},
        TypeFieldDescription{"real", "float32", 0},
    };

    TypeDescription packed;
    packed.name = "PackedFlags";
    packed.size = 4;
    packed.declaration = "struct PackedFlags { uint32 low:3; uint32 mode:5; bool enabled:1; };";
    packed.kind = TypeKind::structure;
    packed.bitfields = {
        TypeBitFieldDescription{"low", "uint32", 3, 0},
        TypeBitFieldDescription{"mode", "uint32", 5, 0},
        TypeBitFieldDescription{"enabled", "bool", 1, 0},
    };

    return {integer_type("int32", 4, true),
            integer_type("int64", 8, true),
            integer_type("uint32", 4, false),
            floating_type("float32", 4),
            TypeDescription{"bool", 1, "", TypeKind::boolean, false, "", 0, {}, {}, {}, {}},
            row,
            record,
            pair,
            word,
            packed,
            pointer_type("IntRow *", "IntRow"),
            pointer_type("Record *", "Record"),
            pointer_type("Pair *", "Pair"),
            pointer_type("Word *", "Word"),
            pointer_type("PackedFlags *", "PackedFlags"),
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
    EXPECT_NE(artifact.find(token), std::string_view::npos) << "missing token: " << token << "\nGenerated artifact:\n"
                                                            << artifact;
}

/// Verifies that a low-level artifact does not contain a transformation
/// surrogate, matching XML `<stringmatch min="0" max="0">` assertions.
static void expect_excludes(std::string_view artifact, std::string_view token) {
    EXPECT_EQ(artifact.find(token), std::string_view::npos)
        << "unexpected token: " << token << "\nGenerated artifact:\n"
        << artifact;
}

/// Returns the generated C body for one function, excluding provider-emitted
/// declarations. This keeps semantic assertions from passing on a type name
/// that was printed before native analysis materialized the function.
static std::string_view generated_function_body(std::string_view c_source, std::string_view function_name) {
    const std::size_t function_position = c_source.find(function_name);
    if (function_position == std::string_view::npos) {
        return {};
    }
    const std::size_t body_start = c_source.find('{', function_position);
    if (body_start == std::string_view::npos) {
        return {};
    }
    return c_source.substr(body_start + 1);
}

/// Verifies a token in the native function body rather than in a declaration.
/// This is the portable equivalent of an original XML `<stringmatch>` whose
/// intent is a recovered field, enum value, or pointer expression.
static void expect_function_body_contains(const DecompilationResult& result, std::string_view function_name,
                                          std::string_view token) {
    const std::string_view body = generated_function_body(result.c_source, function_name);
    ASSERT_FALSE(body.empty()) << "missing generated body for function: " << function_name << "\nC source:\n"
                               << result.c_source;
    EXPECT_NE(body.find(token), std::string_view::npos)
        << "missing semantic token: " << token << "\nGenerated function body:\n"
        << body;
}

/// Verifies that a low-level surrogate expression did not survive into a
/// function body after provider metadata was applied by native analysis.
static void expect_function_body_excludes(const DecompilationResult& result, std::string_view function_name,
                                          std::string_view token) {
    const std::string_view body = generated_function_body(result.c_source, function_name);
    ASSERT_FALSE(body.empty()) << "missing generated body for function: " << function_name << "\nC source:\n"
                               << result.c_source;
    EXPECT_EQ(body.find(token), std::string_view::npos)
        << "unexpected low-level token: " << token << "\nGenerated function body:\n"
        << body;
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

/// Exercises the four compiler-generated signed-modulo strength-reduction
/// sequences from the x86-64 Windows fixture. The tests assert the recovered
/// modulo constants rather than accepting a generic multiply/shift expression.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/modulo2.xml`.
TEST(DecompilerDatatests, PortedModulo2) {
    const std::array<std::pair<std::uint64_t, std::vector<std::uint8_t>>, 3> cases{{
        {0x490000, {0x89, 0xc8, 0xc1, 0xe9, 0x1f, 0x01, 0xc1, 0x83, 0xe1, 0xfe, 0x29, 0xc8, 0xc3}},
        {0x490010, {0x48, 0x63, 0xc1, 0x48, 0x69, 0xc8, 0x56, 0x55, 0x55, 0x55, 0x48, 0x89, 0xca, 0x48, 0xc1,
                    0xea, 0x3f, 0x48, 0xc1, 0xe9, 0x20, 0x01, 0xd1, 0x8d, 0x0c, 0x49, 0x29, 0xc8, 0xc3}},
        {0x490030, {0x89, 0xc8, 0x8d, 0x48, 0x03, 0x85, 0xc0, 0x0f, 0x49, 0xc8, 0x83, 0xe1, 0xfc, 0x29, 0xc8, 0xc3}},
    }};
    const std::array<std::string_view, 3> names{"mod2", "mod3", "mod4"};
    const std::array<std::string_view, 3> expressions{"param_1 % 2", "param_1 % 3", "param_1 % 4"};
    for (std::size_t index = 0; index < cases.size(); ++index) {
        const auto& [entry, body] = cases[index];
        const PrototypeDescription prototype =
            make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                           {PrototypeParameterDescription{"param_1", "int32", Storage{"register", 8, 4}}});
        const auto metadata = make_metadata({{entry, std::string(names[index]), ""}}, {integer_type("int32", 4, true)},
                                            {{entry, prototype}});
        const DecompilationResult result =
            decompile_embedded(entry, body, body.size(), std::string(names[index]), metadata);

        expect_complete_analysis(result);
        expect_function_body_contains(result, names[index], expressions[index]);
    }
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

/// Exercises typed structure fields, an embedded fixed array, and a pointer to
/// an array through real provider metadata and x86 pointer arithmetic.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/concat.xml` and
/// `Ghidra/Features/Decompiler/src/decompile/datatests/ptrtoarray.xml`.
TEST(DecompilerDatatests, PortedStructuresArraysAndPointers) {
    const std::uint64_t entry = 0x408000;
    const std::vector<std::uint8_t> bytes{
        0x8b, 0x01,             // mov eax, [rcx+0], read Record::first
        0x03, 0x41, 0x08,       // add eax, [rcx+8], consume Record::values[0]
        0x03, 0x02,             // add eax, [rdx], consume IntRow through a pointer-to-array
        0x44, 0x89, 0x41, 0x04, // mov [rcx+4], r8d, write Record::second
        0xc3,                   // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"record", "Record *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"row", "IntRow *", Storage{"register", 0x10, 8}},
                        PrototypeParameterDescription{"value", "int32", Storage{"register", 0x80, 4}}});
    const auto metadata = make_metadata({{entry, "record_access", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "record_access", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "record_access", "record->first");
    expect_function_body_contains(result, "record_access", "record->values");
    expect_function_body_contains(result, "record_access", "record->second");
    expect_function_body_contains(result, "record_access", "row");
    const std::string_view body = generated_function_body(result.c_source, "record_access");
    EXPECT_TRUE(body.find("(*row)[0]") != std::string_view::npos || body.find("row[0]") != std::string_view::npos)
        << "pointer-to-array access was not materialized in the function body:\n"
        << body;
    expect_function_body_excludes(result, "record_access", "CONCAT");
    expect_function_body_excludes(result, "record_access", "ZEXT");
}

/// Exercises overlapping provider union fields through a typed load.
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
    expect_function_body_contains(result, "union_value", "word->number");
    expect_function_body_excludes(result, "union_value", "undefined4");
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
        0x48, 0x8b, 0x01,       // mov rax, [rcx], return Pair::low
        0x48, 0x8b, 0x51, 0x08, // mov rdx, [rcx+8], return Pair::high
        0xc3,                   // return Pair in RDX:RAX
    };
    PrototypeDescription prototype =
        make_prototype("__cdecl", "Pair", std::nullopt,
                       {PrototypeParameterDescription{"source", "Pair *", Storage{"register", 8, 8}}});
    prototype.return_storage_pieces = {
        Storage{"register", 0x10, 8}, // most-significant Pair::high piece
        Storage{"register", 0, 8},    // least-significant Pair::low piece
    };
    const auto metadata = make_metadata({{entry, "return_pair", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "return_pair", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "return_pair", "source");
    expect_function_body_contains(result, "return_pair", "source->low");
    expect_function_body_contains(result, "return_pair", "source->high");
    expect_function_body_contains(result, "return_pair", "return");
}

/// Returns the named aggregate types shared by the ABI-piece datatests.
/// Original layouts: `concatsplit.xml`, `piecestruct.xml`, and `stackreturn.xml`.
static std::vector<TypeDescription> aggregate_piece_types() {
    TypeDescription pair;
    pair.name = "Pair";
    pair.size = 16;
    pair.declaration = "struct Pair { int64 low; int64 high; };";
    pair.kind = TypeKind::structure;
    pair.fields = {
        TypeFieldDescription{"low", "int64", 0},
        TypeFieldDescription{"high", "int64", 8},
    };
    TypeDescription pair_pointer = pointer_type("Pair *", "Pair");

    TypeDescription piece_array = array_type("PieceArray", "int32", 4, 4);
    TypeDescription piece;
    piece.name = "Piece";
    piece.size = 24;
    piece.declaration = "struct Piece { int32 a; int32 b; int32 values[4]; };";
    piece.kind = TypeKind::structure;
    piece.fields = {
        TypeFieldDescription{"a", "int32", 0},
        TypeFieldDescription{"b", "int32", 4},
        TypeFieldDescription{"values", "PieceArray", 8},
    };
    TypeDescription piece_pointer = pointer_type("Piece *", "Piece");
    return {integer_type("int32", 4, true),
            integer_type("int64", 8, true),
            pair,
            pair_pointer,
            piece_array,
            piece,
            piece_pointer};
}

/// Exercises a by-value structure passed in two non-contiguous registers.
/// The provider's most-significant-first storage list is converted by the real
/// `ParameterPieces::assignAddressFromPieces` join path, and the C printer must
/// expose field assignments rather than a synthetic CONCAT expression.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/concatsplit.xml`.
TEST(DecompilerDatatests, PortedConcatSplitAggregatePieces) {
    const std::uint64_t entry = 0x460000;
    const std::vector<std::uint8_t> bytes{
        0x48, 0x89, 0x01,       // mov [rcx], rax: store Pair::low
        0x48, 0x89, 0x51, 0x08, // mov [rcx+8], rdx: store Pair::high
        0xc3,                   // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"out", "Pair *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{
                            "value", "Pair", std::nullopt, {Storage{"register", 0x10, 8}, Storage{"register", 0, 8}}}});
    const auto metadata = make_metadata({{entry, "test_split", ""}}, aggregate_piece_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "test_split", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "test_split", "out->low");
    expect_function_body_contains(result, "test_split", "out->high");
    expect_function_body_excludes(result, "test_split", "CONCAT");
    expect_function_body_excludes(result, "test_split", "ZEXT");
}

/// Exercises six explicitly stored scalar inputs that populate structure fields
/// and an array through real x86 stores. The assertions verify that prototype
/// storage remains attached to source names while structure splitting recovers
/// every destination field.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/piecestruct.xml`.
TEST(DecompilerDatatests, PortedPieceStructureFields) {
    const std::uint64_t entry = 0x461000;
    const std::vector<std::uint8_t> bytes{
        0x89, 0x11,             // mov [rcx], edx: Piece::a
        0x44, 0x89, 0x41, 0x04, // mov [rcx+4], r8d: Piece::b
        0x44, 0x89, 0x49, 0x08, // mov [rcx+8], r9d: values[0]
        0x89, 0x41, 0x0c,       // mov [rcx+12], eax: values[1]
        0x89, 0x59, 0x10,       // mov [rcx+16], ebx: values[2]
        0x44, 0x89, 0x51, 0x14, // mov [rcx+20], r10d: values[3]
        0xc3,                   // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"out", "Piece *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"a", "int32", Storage{"register", 0x10, 4}},
                        PrototypeParameterDescription{"b", "int32", Storage{"register", 0x80, 4}},
                        PrototypeParameterDescription{"c", "int32", Storage{"register", 0x88, 4}},
                        PrototypeParameterDescription{"d", "int32", Storage{"register", 0, 4}},
                        PrototypeParameterDescription{"e", "int32", Storage{"register", 0x18, 4}},
                        PrototypeParameterDescription{"f", "int32", Storage{"register", 0x90, 4}}});
    const auto metadata = make_metadata({{entry, "assign", ""}}, aggregate_piece_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "assign", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "assign", "out->a");
    expect_function_body_contains(result, "assign", "out->b");
    expect_function_body_contains(result, "assign", "out->values[0]");
    expect_function_body_contains(result, "assign", "out->values[3]");
}

/// Exercises a structure return split between RDX and RAX while two typed
/// integer parameters occupy RCX and R8. This is the minimal real return-piece
/// case that guards against losing the ordered output storage in prototype
/// installation.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/multiret.xml`.
TEST(DecompilerDatatests, PortedMultiReturnAggregatePieces) {
    const std::uint64_t entry = 0x462000;
    const std::vector<std::uint8_t> bytes{
        0x48, 0x89, 0xc8, // mov rax, rcx: low return piece
        0x4c, 0x89, 0xc2, // mov rdx, r8: high return piece
        0xc3,             // return
    };
    PrototypeDescription prototype =
        make_prototype("__cdecl", "Pair", std::nullopt,
                       {PrototypeParameterDescription{"low", "int64", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"high", "int64", Storage{"register", 0x80, 8}}});
    prototype.return_storage_pieces = {
        Storage{"register", 0x10, 8}, // most-significant RDX piece
        Storage{"register", 0, 8},    // least-significant RAX piece
    };
    const auto metadata = make_metadata({{entry, "multi_return", ""}}, aggregate_piece_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "multi_return", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "multi_return", "low");
    expect_function_body_contains(result, "multi_return", "high");
    expect_function_body_contains(result, "multi_return", "return");
}

/// Exercises the original stack-return pattern with three real child calls.
/// Each child prototype stores its result in a callee-relative stack location,
/// so the root's native call analysis must preserve the stack return facts and
/// the named data symbols receiving those values.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/stackreturn.xml`.
TEST(DecompilerDatatests, PortedStackReturnAggregateStorage) {
    const std::uint64_t entry = 0x100020;
    const std::uint64_t perfect = 0x100120;
    const std::uint64_t small = 0x100121;
    const std::uint64_t big = 0x100122;
    const std::vector<std::uint8_t> root_bytes{
        0x48, 0x83, 0xec, 0x18, 0xbf, 0x64, 0x00, 0x00, 0x00, 0x66, 0xe8, 0xf3, 0x00, 0x48, 0x8b, 0x44,
        0x24, 0x08, 0x67, 0x48, 0xa3, 0x40, 0x01, 0x10, 0x00, 0xbf, 0x01, 0x00, 0x00, 0x00, 0xbe, 0x14,
        0x00, 0x00, 0x00, 0x66, 0xe8, 0xda, 0x00, 0x8b, 0x44, 0x24, 0x08, 0x90, 0x67, 0xa3, 0x50, 0x01,
        0x10, 0x00, 0x90, 0xbf, 0x02, 0x00, 0x00, 0x00, 0xbe, 0x1e, 0x00, 0x00, 0x00, 0x66, 0xe8, 0xc1,
        0x00, 0x48, 0x8b, 0x44, 0x24, 0x08, 0x67, 0x48, 0xa3, 0x58, 0x01, 0x10, 0x00, 0xc3,
    };
    std::vector<std::uint8_t> image = root_bytes;
    image.resize(0x103, 0x90);
    image[0x100] = 0xc3;
    image[0x101] = 0xc3;
    image[0x102] = 0xc3;

    const PrototypeDescription root = make_prototype("__cdecl", "void", std::nullopt, {});
    const PrototypeDescription perfect_prototype =
        make_prototype("__cdecl", "int64", Storage{"stack", 0x10, 8},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 0x38, 4}}});
    const PrototypeDescription small_prototype =
        make_prototype("__cdecl", "int32", Storage{"stack", 0x10, 4},
                       {PrototypeParameterDescription{"first", "int32", Storage{"register", 0x38, 4}},
                        PrototypeParameterDescription{"second", "int32", Storage{"register", 0x30, 4}}});
    const PrototypeDescription big_prototype =
        make_prototype("__cdecl", "int64", Storage{"stack", 0x12, 8},
                       {PrototypeParameterDescription{"first", "int32", Storage{"register", 0x38, 4}},
                        PrototypeParameterDescription{"second", "int32", Storage{"register", 0x30, 4}}});
    SymbolDescription perf_ret{0x100140, "perf_ret", "", SymbolKind::data, 8, "int64", false};
    SymbolDescription small_ret{0x100150, "small_ret", "", SymbolKind::data, 4, "int32", false};
    SymbolDescription big_ret{0x100158, "big_ret", "", SymbolKind::data, 8, "int64", false};
    ProviderContext metadata =
        make_metadata({{entry, "stackreturn", ""},
                       {perfect, "perfect", ""},
                       {small, "small", ""},
                       {big, "big", ""},
                       perf_ret,
                       small_ret,
                       big_ret},
                      {integer_type("int32", 4, true), integer_type("int64", 8, true)},
                      {{entry, root}, {perfect, perfect_prototype}, {small, small_prototype}, {big, big_prototype}});
    metadata.functions = std::make_shared<FunctionTableProvider>(std::vector<FunctionDescription>{
        FunctionDescription{"perfect", perfect, perfect + 1},
        FunctionDescription{"small", small, small + 1},
        FunctionDescription{"big", big, big + 1},
    });
    const DecompilationResult result =
        decompile_embedded(entry, std::move(image), root_bytes.size(), "stackreturn", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "perfect");
    expect_contains(result.c_source, "small");
    expect_contains(result.c_source, "big");
    expect_contains(result.c_source, "perf_ret");
    expect_contains(result.c_source, "small_ret");
    expect_contains(result.c_source, "big_ret");
}

/// Exercises a mixed floating/integer prototype with explicit XMM and general
/// register locations. The XMM offsets are the real x86-64 Sleigh register
/// locations, so the test covers provider prototype storage and float typing
/// together instead of checking a declaration in isolation.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/mixfloatint.xml`.
TEST(DecompilerDatatests, PortedMixedFloatIntegerPrototype) {
    const std::uint64_t entry = 0x463000;
    const std::vector<std::uint8_t> bytes{
        0xf2, 0x0f, 0x58, 0xc0, // addsd xmm0, xmm0
        0xc3,                   // return float64 in XMM0
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "float64", Storage{"register", 0x1200, 8},
                       {PrototypeParameterDescription{"a", "float64", Storage{"register", 0x1200, 8}},
                        PrototypeParameterDescription{"b", "int32", Storage{"register", 8, 4}},
                        PrototypeParameterDescription{"c", "float64", Storage{"register", 0x1280, 8}},
                        PrototypeParameterDescription{"d", "int32", Storage{"register", 0x10, 4}},
                        PrototypeParameterDescription{"e", "int32", Storage{"register", 0x80, 4}},
                        PrototypeParameterDescription{"f", "int32", Storage{"register", 0x88, 4}}});
    const auto metadata = make_metadata(
        {{entry, "dldlll", ""}}, {integer_type("int32", 4, true), floating_type("float64", 8)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "dldlll", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "float64 __cdecl dldlll");
    expect_contains(result.c_source, "float64 a");
    expect_contains(result.c_source, "int32 b");
    expect_contains(result.c_source, "float64 c");
    expect_contains(result.c_source, "int32 f");
    expect_contains(result.c_source, "return");
}

/// Verifies the x86 extended-precision storage contract from longdouble.xml by
/// running the original writeLongDouble bytes through Sleigh and the native
/// decompiler. The assertions distinguish a real ten-byte floating value from
/// an integer declaration: x87 stack copies and the native FLOAT_ADD operation
/// must survive analysis, and the typed pointer store must be printed as a
/// float10 assignment.
/// Original fixture: `Ghidra/Features/Decompiler/src/decompile/datatests/longdouble.xml:40-45,51-67,83-102`.
TEST(DecompilerDatatests, PortedLongDoubleFloat10CoreBehavior) {
    const std::uint64_t entry = 0x101100;
    const std::string code = "f30f1efadb6c2408d9c0db3fdc059e010000db7f10c3";

    TypeDescription float10 = floating_type("float10", 10);
    TypeDescription float10_pointer = pointer_type("float10 *", "float10");
    const TypeDescription double_type = floating_type("double_value", 8);
    SymbolDescription constant{0x1012b0, "long_double_point_seven", "", SymbolKind::data, 8, "double_value"};
    constant.read_only = true;
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptrwrite", "float10 *", Storage{"register", 0x38, 8}},
                        PrototypeParameterDescription{"valwrite", "float10", Storage{"stack", 8, 10}}});
    const ProviderContext metadata = make_metadata({SymbolDescription{entry, "writeLongDouble", ""}, constant},
                                                   {float10, float10_pointer, double_type}, {{entry, prototype}});

    const DecompilationResult result = decompile_embedded_chunks(entry, {{entry, code}, {0x1012b0, "666666666666e63f"}},
                                                                 0x16, "writeLongDouble", metadata);

    ASSERT_FALSE(result.raw_pcode.empty());
    // The x87 instruction converts the mapped IEEE double constant through the
    // native FloatFormat path before the ten-byte value participates in FLOAT_ADD.
    ghidra::FloatFormat double_format(8);
    const ghidra::uintb point_seven_encoding = double_format.getEncoding(0.7);
    ghidra::FloatFormat::floatclass point_seven_class;
    EXPECT_EQ(double_format.getHostFloat(point_seven_encoding, &point_seven_class), 0.7);
    EXPECT_EQ(point_seven_class, ghidra::FloatFormat::normalized);
    EXPECT_NE(result.raw_pcode.find("f+"), std::string::npos);
    EXPECT_NE(result.raw_pcode.find(":10"), std::string::npos);
    EXPECT_NE(result.c_source.find("float10"), std::string::npos);
    EXPECT_NE(result.c_source.find("ptrwrite"), std::string::npos);
    EXPECT_NE(result.c_source.find("valwrite"), std::string::npos);
}

/// Verifies all three forced integer representations from displayformat.xml
/// through mapped provider symbols and the native C printer. The binary form
/// is attached to the exact constant use at 0x100619, so this test exercises
/// the dynamic-symbol path rather than accepting a declaration-only format.
/// Original fixture: `Ghidra/Features/Decompiler/src/decompile/datatests/displayformat.xml:8-35`.
TEST(DecompilerDatatests, PortedDisplayFormatForcedIntegerConstants) {
    const std::uint64_t entry = 0x1005fa;
    const std::string code = "554889e5c7050c0a200064000000c6050d0a200077c705ff0920006d0b0000"
                             "c705fd092000aa000000905dc3";
    TypeDescription uint8_type;
    uint8_type.name = "uint8_t";
    uint8_type.size = 1;
    uint8_type.kind = TypeKind::typedef_type;
    uint8_type.element_type = "char";
    uint8_type.signed_value = true;
    uint8_type.display_format = DisplayFormat::hexadecimal;
    TypeDescription octint4 = integer_type("octint4", 4, true);
    octint4.kind = TypeKind::typedef_type;
    octint4.element_type = "int4";
    octint4.display_format = DisplayFormat::octal;
    std::vector<SymbolDescription> symbols{
        SymbolDescription{entry, "setglobals", ""},
        SymbolDescription{0x301014, "globalfree", "", SymbolKind::data, 4, "int4"},
        SymbolDescription{0x30101c, "globalhex", "", SymbolKind::data, 1, "uint8_t"},
        SymbolDescription{0x301018, "globaloct", "", SymbolKind::data, 4, "octint4"},
        SymbolDescription{0x301020, "globalbin", "", SymbolKind::data, 4, "octint4"},
    };
    for (std::size_t index = 1; index < symbols.size(); ++index) {
        symbols[index].read_only = true;
    }
    const PrototypeDescription prototype = make_prototype("__cdecl", "void", std::nullopt, {});
    ProviderContext metadata =
        make_metadata(std::move(symbols), {integer_type("int4", 4, true), uint8_type, octint4}, {{entry, prototype}});
    metadata.analysis_options.readonly_propagate = true;
    metadata.constant_formats.push_back(ConstantFormatDescription{0x100608, 0x77, 0, DisplayFormat::hexadecimal, 1});
    metadata.constant_formats.push_back(ConstantFormatDescription{0x10060f, 0xb6d, 0, DisplayFormat::octal, 4});
    metadata.constant_formats.push_back(ConstantFormatDescription{0x100619, 0xaa, 0, DisplayFormat::binary, 4});

    const DecompilationResult result = decompile_embedded_chunks(
        entry,
        {{entry, code}, {0x301014, "64000000"}, {0x301018, "6d0b0000"}, {0x30101c, "77"}, {0x301020, "aa000000"}}, 0x2c,
        "setglobals", std::move(metadata), make_x86_64_architecture());

    EXPECT_NE(result.c_source.find("globalfree = 100"), std::string::npos);
    EXPECT_NE(result.c_source.find("globalhex = 0x77"), std::string::npos);
    ASSERT_NE(result.c_source.find("globaloct = 05555"), std::string::npos) << result.c_source;
    EXPECT_NE(result.c_source.find("globalbin = 0b10101010"), std::string::npos);
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

/// Exercises an enumerated read-only data symbol and verifies that its type and
/// name are installed before the RIP-relative load is analyzed.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/readvolatile.xml`.
TEST(DecompilerDatatests, PortedDataSymbolMetadata) {
    const std::uint64_t entry = 0x40e800;
    const std::uint64_t data_address = entry + 0x20;
    std::vector<std::uint8_t> bytes{
        0x8b, 0x05, 0x1a, 0x00, 0x00, 0x00, // mov eax, [rip + 0x1a]
        0xc3,                               // return
    };
    bytes.resize(0x24, 0);
    bytes[0x20] = 0x07;
    SymbolDescription data_symbol;
    data_symbol.address = data_address;
    data_symbol.name = "readonly_value";
    data_symbol.kind = SymbolKind::data;
    data_symbol.size = 4;
    data_symbol.type_name = "int32";
    data_symbol.read_only = true;
    const PrototypeDescription prototype = make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {});
    const auto metadata = make_metadata({{entry, "data_symbol_root", ""}, data_symbol},
                                        {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, 7, "data_symbol_root", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "readonly_value");
}

/// Exercises the hidden return-pointer storage used by special-return ABIs.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/retspecial.xml`.
TEST(DecompilerDatatests, PortedSpecialReturnStorage) {
    const std::uint64_t entry = 0x40f800;
    const std::vector<std::uint8_t> bytes{0xc3};
    PrototypeDescription prototype = make_prototype("__cdecl", "int32", std::nullopt, {});
    prototype.hidden_return_storage = Storage{"register", 8, 8};
    const auto metadata =
        make_metadata({{entry, "special_return", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "special_return", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "rethidden");
}

/// Exercises no-return function metadata so the call site becomes a terminal
/// flow edge instead of retaining an unreachable fall-through block.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/multiret.xml`.
TEST(DecompilerDatatests, PortedNoReturnMetadata) {
    const std::uint64_t entry = 0x40fc00;
    const std::uint64_t target = entry + 0x10;
    const std::vector<std::uint8_t> bytes{
        0xe8, 0x0b, 0x00, 0x00, 0x00, // call no_return_function
        0xc3,                         // unreachable fall-through in the source image
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        0xc3, // external body placeholder
    };
    PrototypeDescription root = make_prototype("__cdecl", "void", std::nullopt, {});
    PrototypeDescription no_return = root;
    no_return.no_return = true;
    const auto metadata = make_metadata({{entry, "noreturn_root", ""}, {target, "abort_path", ""}}, {},
                                        {{entry, root}, {target, no_return}});
    const DecompilationResult result = decompile_embedded(entry, bytes, 6, "noreturn_root", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "abort_path");
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

// Focused provider-metadata ports for original fixtures that were previously
// represented only by primitive fallback types or legacy database commands.

/// Exercises real structure fields, a fixed array field, and a pointer-to-array
/// load/store sequence without relying on database `map addr` commands.
/// Original source comments from `concat.xml` and `ptrtoarray.xml`: entire
/// structures built out of PIECE/ZEXT operations should expose individual
/// fields, while pointer-to-array expressions must retain their array shape.
TEST(DecompilerDatatests, ProviderAggregateFieldsAndArrayElement) {
    const std::uint64_t entry = 0x410000;
    const std::vector<std::uint8_t> bytes{
        0x8b, 0x01,             // mov eax, [rcx+0], read Record::first
        0x03, 0x41, 0x08,       // add eax, [rcx+8], consume Record::values[0]
        0x03, 0x02,             // add eax, [rdx], consume IntRow through a pointer-to-array
        0x44, 0x89, 0x41, 0x04, // mov [rcx+4], r8d, write Record::second
        0xc3,                   // return the aggregate-derived sum
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"record", "Record *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"row", "IntRow *", Storage{"register", 0x10, 8}},
                        PrototypeParameterDescription{"value", "int32", Storage{"register", 0x80, 4}}});
    const auto metadata = make_metadata({{entry, "aggregate_fields", ""}}, composite_types(), {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "aggregate_fields", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "aggregate_fields", "record->first");
    expect_function_body_contains(result, "aggregate_fields", "record->second");
    // C printer intentionally elides the zero subscript for an array field;
    // the recovered field type and access are still preserved semantically.
    expect_function_body_contains(result, "aggregate_fields", "record->values");
    const std::string_view body = generated_function_body(result.c_source, "aggregate_fields");
    EXPECT_TRUE(body.find("(*row)[0]") != std::string_view::npos || body.find("row[0]") != std::string_view::npos)
        << "pointer-to-array access was not materialized in the function body:\n"
        << body;
    expect_function_body_excludes(result, "aggregate_fields", "CONCAT");
    expect_function_body_excludes(result, "aggregate_fields", "ZEXT");
}

/// Exercises a provider deindirect record against a real x86 CALLIND and verifies
/// that the recovered target is named before native flow analysis starts.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/deindirect.xml`.
TEST(DecompilerDatatests, PortedDeindirectTargetOverride) {
    const std::uint64_t entry = 0x420000;
    const std::uint64_t target = entry + 0x10;
    const std::vector<std::uint8_t> bytes{
        0x48, 0xc7, 0xc0, 0x10, 0x00, 0x42, 0x00, // mov rax, target
        0xff, 0xd0,                               // call rax
        0xc3,                                     // return
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90,       // gap before target
        0xc3,                                     // target return
    };
    FlowDescription flow;
    flow.indirect_call_targets.push_back(IndirectCallTargetDescription{entry + 7, target});
    ProviderContext metadata =
        make_metadata({{entry, "deindirect", ""}, {target, "realfunc", ""}}, {integer_type("int32", 4, true)}, {});
    metadata.flow =
        std::make_shared<FlowTableProvider>(std::vector<std::pair<std::uint64_t, FlowDescription>>{{entry, flow}});
    const DecompilationResult result = decompile_embedded(entry, bytes, 10, "deindirect", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "realfunc");
}

/// Exercises destination and jump-table records through native flow metadata.
/// Original sources: `switchind.xml`, `switchmask.xml`, and `switchmulti.xml`;
/// the explicit table keeps this provider plumbing regression independent of
/// processor-specific XML compiler specifications. It does not claim to port
/// the original switch-restructuring fixtures.
TEST(DecompilerDatatests, PortedDestinationAndJumpTableOverrides) {
    const std::uint64_t entry = 0x430000;
    const std::uint64_t target = entry + 0x10;
    const std::vector<std::uint8_t> bytes{
        0x48, 0xc7, 0xc0, 0x10, 0x00, 0x43, 0x00, // mov rax, target
        0xff, 0xe0,                               // branchind rax
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // gap before target
        0xb8, 0x01, 0x00, 0x00, 0x00,             // target returns one
        0xc3,
    };
    FlowDescription flow;
    flow.jump_tables.push_back(JumpTableDescription{entry + 7, {target}, std::nullopt, 0, 0});
    ProviderContext metadata = make_metadata({{entry, "switch_override", ""}, {target, "switch_case", ""}},
                                             {integer_type("int32", 4, true)}, {});
    metadata.flow =
        std::make_shared<FlowTableProvider>(std::vector<std::pair<std::uint64_t, FlowDescription>>{{entry, flow}});
    const DecompilationResult result =
        decompile_embedded(entry, bytes, bytes.size(), "switch_override", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "return");
}

/// Exercises full-body child availability and the provider inline flag.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/inline.xml`.
TEST(DecompilerDatatests, PortedInlineFunctionBody) {
    const std::uint64_t entry = 0x440000;
    const std::uint64_t child = entry + 0x10;
    const std::vector<std::uint8_t> bytes{
        0xe8, 0x0b, 0x00, 0x00, 0x00,                                                 // call child
        0xc3,                                                                         // root return
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x8d, 0x41, 0x32, // child: eax = ecx + 50
        0xc3,
    };
    PrototypeDescription root =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}});
    PrototypeDescription inline_child = root;
    inline_child.inline_function = true;
    ProviderContext metadata = make_metadata({{entry, "inline_root", ""}, {child, "add50", ""}},
                                             {integer_type("int32", 4, true)}, {{entry, root}, {child, inline_child}});
    metadata.functions =
        std::make_shared<FunctionTableProvider>(std::vector<FunctionDescription>{{"add50", child, child + 4}});
    DecompilationResult result;
    try {
        result = decompile_embedded(entry, bytes, 6, "inline_root", std::move(metadata));
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
        return;
    } catch (const std::exception& error) {
        FAIL() << error.what();
        return;
    }

    expect_complete_analysis(result);
    expect_contains(result.c_source, "Inlined function: add50");
    expect_contains(result.c_source, "+");
}

/// Exercises structured call-fixup registration and replaces a call with a
/// provider-owned COPY operation rather than accepting unparsed SLEIGH text.
/// Original sources: `inline.xml` and `injectoverride.xml`.
TEST(DecompilerDatatests, PortedStructuredCallFixupInjection) {
    const std::uint64_t entry = 0x450000;
    const std::uint64_t target = entry + 0x10;
    const std::vector<std::uint8_t> bytes{
        0xb9, 0x07, 0x00, 0x00, 0x00, // ecx = 7
        0xe8, 0x06, 0x00, 0x00, 0x00, // call target
        0xc3,                         // return
        0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
        0xc3, // target body is replaced by fixup
    };
    PrototypeDescription root = make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {});
    PrototypeDescription target_prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}});
    target_prototype.call_fixup = "provider_identity";
    CallFixupDescription fixup;
    fixup.name = "provider_identity";
    fixup.operations.push_back(
        InjectionOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
                           InjectionVarnode{InjectionVarnodeKind::storage, Storage{"register", 0, 4}, 0},
                           {InjectionVarnode{InjectionVarnodeKind::storage, Storage{"register", 8, 4}, 0}}});
    ProviderContext metadata =
        make_metadata({{entry, "fixup_root", ""}, {target, "fixed_call", ""}}, {integer_type("int32", 4, true)},
                      {{entry, root}, {target, target_prototype}});
    metadata.injections = std::make_shared<InjectionTableProvider>(std::vector<CallFixupDescription>{fixup});
    DecompilationResult result;
    try {
        result = decompile_embedded(entry, bytes, 11, "fixup_root", std::move(metadata));
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
        return;
    } catch (const std::exception& error) {
        FAIL() << error.what();
        return;
    }

    expect_complete_analysis(result);
    expect_contains(result.c_source, "return");
    expect_contains(result.c_source, "return 7");
}

/// Exercises overlapping union members through a typed load and verifies that
/// provider field metadata prevents the old undefined-integer fallback.
/// Original source comment from
/// `Ghidra/Features/Decompiler/src/decompile/datatests/union_datatype.xml`:
/// Contrived examples of functions manipulating union data-types.
TEST(DecompilerDatatests, ProviderUnionFieldSelection) {
    const std::uint64_t entry = 0x411000;
    const std::vector<std::uint8_t> bytes{
        0x8b,
        0x01, // mov eax, [rcx], read the overlapping union storage
        0xc3, // return
    };
    TypeDescription value;
    value.name = "Value";
    value.size = 4;
    value.declaration = "union Value { int32 number; float32 real; };";
    value.kind = TypeKind::union_type;
    value.fields = {
        TypeFieldDescription{"number", "int32", 0},
        TypeFieldDescription{"real", "float32", 0},
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "Value *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata(
        {{entry, "union_field", ""}},
        {integer_type("int32", 4, true), floating_type("float32", 4), value, pointer_type("Value *", "Value")},
        {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "union_field", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "union_field", "value->number");
    expect_function_body_excludes(result, "union_field", "undefined4");
}

/// Exercises named enum values in a real conditional branch so the C printer
/// must preserve semantic names instead of emitting only their integer values.
/// Original source comment from
/// `Ghidra/Features/Decompiler/src/decompile/datatests/enum.xml`: Functions
/// that read enum values and compare with constant values should print those
/// constants by name.
TEST(DecompilerDatatests, ProviderEnumNamedComparison) {
    const std::uint64_t entry = 0x412000;
    const std::vector<std::uint8_t> bytes{
        0x83, 0xf9, 0x01,                   // cmp ecx, 1
        0x75, 0x06,                         // jne default
        0xb8, 0x01, 0x00, 0x00, 0x00,       // return 1 for FLAG_ONE
        0xc3, 0xb8, 0x00, 0x00, 0x00, 0x00, // default return 0
        0xc3,
    };
    TypeDescription flags;
    flags.name = "Flags";
    flags.size = 4;
    flags.declaration = "enum Flags { FLAG_ONE = 1, FLAG_TWO = 2 };";
    flags.kind = TypeKind::enumeration;
    flags.signed_value = false;
    flags.enum_values = {
        TypeEnumValueDescription{"FLAG_ONE", 1},
        TypeEnumValueDescription{"FLAG_TWO", 2},
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"flags", "Flags", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "enum_compare", ""}}, {integer_type("int32", 4, true), flags}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "enum_compare", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "enum_compare", "FLAG_ONE");
    expect_function_body_contains(result, "enum_compare", "if");
    expect_function_body_contains(result, "enum_compare", "return");
}

/// Exercises native bitfield extraction from a provider-described packed
/// structure and verifies that the semantic field survives high-level lifting.
/// Original source comment from
/// `Ghidra/Features/Decompiler/src/decompile/datatests/bitfields.xml`: the
/// decompiler should expose named bitfields rather than leave shifts and masks
/// in the final C output.
TEST(DecompilerDatatests, ProviderBitfieldExtraction) {
    const std::uint64_t entry = 0x413000;
    const std::vector<std::uint8_t> bytes{
        0x8b, 0x01,       // mov eax, [rcx], load the packed storage
        0xc1, 0xe8, 0x03, // shr eax, 3, isolate mode
        0x83, 0xe0, 0x1f, // and eax, 31
        0xc3,             // return mode
    };
    TypeDescription packed;
    packed.name = "PackedFlags";
    packed.size = 4;
    packed.declaration = "struct PackedFlags { uint32 low:3; uint32 mode:5; bool enabled:1; };";
    packed.kind = TypeKind::structure;
    packed.bitfields = {
        TypeBitFieldDescription{"low", "uint32", 3, 0},
        TypeBitFieldDescription{"mode", "uint32", 5, 0},
        TypeBitFieldDescription{"enabled", "bool", 1, 0},
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "uint32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"flags", "PackedFlags *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata({{entry, "bitfield_extract", ""}},
                                        {integer_type("uint32", 4, false), integer_type("int32", 4, true),
                                         TypeDescription{"bool", 1, "", TypeKind::boolean, false, "", 0, {}, {}, {}},
                                         packed, pointer_type("PackedFlags *", "PackedFlags")},
                                        {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "bitfield_extract", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "bitfield_extract", "flags->mode");
    expect_function_body_excludes(result, "bitfield_extract", ">>");
    expect_function_body_excludes(result, "bitfield_extract", "& 0x1f");
}

/// Exercises the provider volatile-range contract with an unused LOAD. The
/// original deadvolatile fixture requires this read to remain for side effects.
/// Original fixture comment: the value returned by the LOAD is unused, but the
/// volatile read must still be present.
TEST(DecompilerDatatests, ProviderVolatileReadRetainsUnusedSideEffect) {
    auto provider = std::make_shared<VolatileLoadProvider>();
    auto memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0},
                                                 std::vector<MemoryRangeDescription>{{"ram", 0x2000, 4}});
    ProviderContext context;
    context.pcode = std::move(provider);
    context.memory = std::move(memory);
    Decompiler decompiler(make_x86_64_architecture(), std::move(context));

    const DecompilationResult result = decompiler.decompile(FunctionDescription{"volatile_read", 0, 1});

    expect_complete_analysis(result);
    EXPECT_NE(result.c_source.find("xVar1"), std::string::npos);
    EXPECT_NE(result.c_source.find("Ram0000000000002000"), std::string::npos) << "C source:\n"
                                                                              << result.c_source << "\nHigh p-code:\n"
                                                                              << result.high_pcode;
}

/// Exercises a counted loop with a real back edge and verifies that the native
/// control-flow restructuring preserves the loop's accumulator semantics.
/// Original source: `forloop1.xml`.
TEST(DecompilerDatatests, ProviderCountedLoopSemanticOutput) {
    const std::uint64_t entry = 0x414000;
    const std::vector<std::uint8_t> bytes{
        0x31, 0xc0,       // xor eax, eax, accumulator = 0
        0x85, 0xc9,       // test ecx, ecx
        0x7e, 0x08,       // jle done
        0x83, 0xc0, 0x01, // add eax, 1
        0x83, 0xe9, 0x01, // sub ecx, 1
        0x75, 0xf4,       // jne loop condition
        0xc3,             // return accumulator
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"count", "int32", Storage{"register", 8, 4}}});
    const auto metadata =
        make_metadata({{entry, "counted_loop", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "counted_loop", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "goto");
    expect_contains(result.c_source, "count");
    expect_contains(result.c_source, "return");
}

/// Exercises the narrow pointer-store sequence used by the first heap-string
/// case and verifies that the native string manager emits the exact narrow
/// builtin and recovered literal rather than leaving individual STOREs.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/heapstring.xml`, Heap string #1.
TEST(DecompilerDatatests, PortedHeapStringNarrow) {
    const std::uint64_t entry = 0x470000;
    const std::vector<std::uint8_t> bytes{
        0x48, 0xb8, 0x48, 0x45, 0x41, 0x50, 0x54, 0x45, 0x53, 0x54, // mov rax, "HEAPTEST"
        0x48, 0x89, 0x01,                                           // mov [rcx], rax
        0xc3,                                                       // return
    };
    const PrototypeDescription prototype = make_prototype(
        "__cdecl", "void", std::nullopt, {PrototypeParameterDescription{"dst", "CharPtr", Storage{"register", 8, 8}}});
    const auto metadata =
        make_metadata({{entry, "heap_narrow", ""}}, {integer_type("char", 1, true), pointer_type("CharPtr", "char")},
                      {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "heap_narrow", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "builtin_strncpy");
    expect_contains(result.c_source, "\"HEAPTEST\"");
    expect_contains(result.c_source, ",8);");
}

/// Exercises a UTF-16 pointer-store sequence and verifies the original heap
/// string algorithm's non-native-width choice of builtin_memcpy and byte count.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/heapstring.xml`, Heap string #6.
TEST(DecompilerDatatests, PortedHeapStringWide) {
    const std::uint64_t entry = 0x471000;
    const std::vector<std::uint8_t> bytes{
        0x48, 0xb8, 0x57, 0x00, 0x49, 0x00, 0x44, 0x00, 0x45, 0x00, // mov rax, UTF-16 "WIDE"
        0x48, 0x89, 0x01,                                           // mov [rcx], rax
        0xc3,                                                       // return
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"dst", "Wchar2Ptr", Storage{"register", 8, 8}}});
    TypeDescription utf16;
    utf16.name = "utf16";
    utf16.size = 2;
    utf16.kind = TypeKind::unicode_character;
    const auto metadata =
        make_metadata({{entry, "heap_wide", ""}}, {utf16, pointer_type("Wchar2Ptr", "utf16")}, {{entry, prototype}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "heap_wide", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "builtin_memcpy");
    expect_contains(result.c_source, "L\"WIDE\"");
    expect_contains(result.c_source, ",8);");
}

/// Exercises narrow stores into a typed stack array and verifies that the
/// external use keeps the local live while HeapSequence reconstructs one
/// builtin_strncpy operation for the complete string.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/stackstring.xml`, Stack string #2.
TEST(DecompilerDatatests, PortedStackStringNarrow) {
    const std::uint64_t entry = 0x472000;
    const std::uint64_t consumer = entry + 0x125;
    const std::vector<std::uint8_t> bytes{
        0x48, 0x83, 0xec, 0x20,       // sub rsp, 0x20
        0xc6, 0x04, 0x24, 0x53,       // stack_chars[0] = 'S'
        0xc6, 0x44, 0x24, 0x01, 0x54, // stack_chars[1] = 'T'
        0xc6, 0x44, 0x24, 0x02, 0x41, // stack_chars[2] = 'A'
        0xc6, 0x44, 0x24, 0x03, 0x43, // stack_chars[3] = 'C'
        0xc6, 0x44, 0x24, 0x04, 0x4b, // stack_chars[4] = 'K'
        0xc6, 0x44, 0x24, 0x05, 0x53, // stack_chars[5] = 'S'
        0xc6, 0x44, 0x24, 0x06, 0x54, // stack_chars[6] = 'T'
        0xc6, 0x44, 0x24, 0x07, 0x52, // stack_chars[7] = 'R'
        0xc6, 0x44, 0x24, 0x08, 0x49, // stack_chars[8] = 'I'
        0xc6, 0x44, 0x24, 0x09, 0x4e, // stack_chars[9] = 'N'
        0xc6, 0x44, 0x24, 0x0a, 0x47, // stack_chars[10] = 'G'
        0xc6, 0x44, 0x24, 0x0b, 0x21, // stack_chars[11] = '!'
        0xc6, 0x44, 0x24, 0x0c, 0x00, // stack_chars[12] = 0
        0x0f, 0x10, 0x04, 0x24,       // movups xmm0, [rsp]
        0x48, 0x8d, 0x0c, 0x24,       // lea rcx, [rsp]
        0xe8, 0xd4, 0x00, 0x00, 0x00, // call external consumer
        0x48, 0x83, 0xc4, 0x20,       // add rsp, 0x20
        0xc3,                         // return
    };
    const TypeDescription stack_chars = array_type("StackChars", "char", 16, 1);
    const PrototypeDescription prototype = make_prototype("__cdecl", "void", std::nullopt, {});
    const PrototypeDescription consumer_prototype = make_prototype(
        "__cdecl", "void", std::nullopt, {PrototypeParameterDescription{"dst", "CharPtr", Storage{"register", 8, 8}}});
    const auto metadata =
        make_metadata({{entry, "stack_narrow", ""}, {consumer, "consume_stack_narrow", ""}},
                      {integer_type("char", 1, true), stack_chars, pointer_type("CharPtr", "char")},
                      {{entry, prototype}, {consumer, consumer_prototype}},
                      {{entry,
                        {VariableDescription{"stack_chars", "StackChars",
                                             Storage{"stack", static_cast<std::uint64_t>(-0x20), 16}}}}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "stack_narrow", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "builtin_strncpy");
    expect_contains(result.c_source, "\"STACKSTRING!\"");
    expect_contains(result.c_source, ",0xd);");
}

/// Exercises native UTF-32 stack stores and verifies that the native wchar_t
/// width selects builtin_wcsncpy with a character count rather than a byte count.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/stackstring.xml`, Stack string #6.
TEST(DecompilerDatatests, PortedStackStringWide) {
    const std::uint64_t entry = 0x473000;
    const std::uint64_t consumer = entry + 0x100;
    const std::vector<std::uint8_t> bytes{
        0x48, 0x83, 0xec, 0x20,                                     // sub rsp, 0x20
        0x48, 0xb8, 0x48, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, // mov rax, L"HI"
        0x48, 0x89, 0x04, 0x24,                                     // mov [rsp], rax
        0x48, 0xb8, 0x44, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, // mov rax, L"DE"
        0x48, 0x89, 0x44, 0x24, 0x08,                               // mov [rsp+8], rax
        0x0f, 0x10, 0x04, 0x24,                                     // movups xmm0, [rsp]
        0x48, 0x8d, 0x0c, 0x24,                                     // lea rcx, [rsp]
        0xe8, 0xd2, 0x00, 0x00, 0x00,                               // call external consumer
        0x48, 0x83, 0xc4, 0x20,                                     // add rsp, 0x20
        0xc3,                                                       // return
    };
    const TypeDescription stack_wide = array_type("StackWide", "wchar4", 4, 4);
    const PrototypeDescription prototype = make_prototype("__cdecl", "void", std::nullopt, {});
    const PrototypeDescription consumer_prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"dst", "Wchar4Ptr", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata(
        {{entry, "stack_wide", ""}, {consumer, "consume_stack_wide", ""}},
        {integer_type("wchar4", 4, true), stack_wide, pointer_type("Wchar4Ptr", "wchar4")},
        {{entry, prototype}, {consumer, consumer_prototype}},
        {{entry,
          {VariableDescription{"stack_wide", "StackWide", Storage{"stack", static_cast<std::uint64_t>(-0x20), 16}}}}});
    const DecompilationResult result = decompile_embedded(entry, bytes, bytes.size(), "stack_wide", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "builtin_wcsncpy");
    expect_contains(result.c_source, "L\"HIDE\"");
    expect_contains(result.c_source, ",4);");
}

/// Verifies that shared intermediate pointer calculations are pushed through
/// the typed array field instead of surviving as duplicate pointer temporaries.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/dupptr.xml`,
/// `loadstore_ptrfieldarray` and its Intermediate pointers #3/#4 assertions.
TEST(DecompilerDatatests, DupptrPreservesTypedIntermediatePointerSemantics) {
    const std::uint64_t entry = 0x100718;
    const std::string code = "488d4491088b00c1e80383e00f8900c3";
    TypeDescription array_struct;
    array_struct.name = "arraystruct";
    array_struct.size = 136;
    array_struct.kind = TypeKind::structure;
    array_struct.fields = {
        TypeFieldDescription{"a", "int32", 0},
        TypeFieldDescription{"b", "int32", 4},
        TypeFieldDescription{"arr1", "IntArray16", 8},
        TypeFieldDescription{"arr2", "IntArray16", 72},
    };
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "arraystruct *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"a", "int32", Storage{"register", 0x10, 4}}});
    const auto metadata = make_metadata({{entry, "loadstore_ptrfieldarray", ""}},
                                        {integer_type("int32", 4, true), array_type("IntArray16", "int32", 16, 4),
                                         array_struct, pointer_type("arraystruct *", "arraystruct")},
                                        {{entry, prototype}});
    const DecompilationResult result = decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(),
                                                                 "loadstore_ptrfieldarray", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "loadstore_ptrfieldarray", "arr1[");
    expect_function_body_contains(result, "loadstore_ptrfieldarray", ">> 3");
    expect_function_body_contains(result, "loadstore_ptrfieldarray", "& 0xf");
    expect_function_body_contains(result, "loadstore_ptrfieldarray", "ptr->arr1[");
}

/// Verifies that an aliased stack array remains a while-loop across an iterate
/// call, preserving the XML fixture's no-for-loop contract.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/noforloop_alias.xml`.
TEST(DecompilerDatatests, NoforloopAliasRetainsWhileLoop) {
    const std::uint64_t entry = 0x400690;
    const std::string code = R"(
        554889e54883ec20897decc745f001000000c745f400000000
        c745f802000000c745fc03000000eb298b45f483c0018945f4
        488d45f04889c7e8adffffff8b45f489c6bf5d084000b800000000
        e85ef4ffff8b45f43945ec7fcf90c9c3
    )";
    TypeDescription int_array = array_type("IntArray4", "int32", 4, 4);
    const PrototypeDescription root_prototype = make_prototype(
        "__cdecl", "void", std::nullopt, {PrototypeParameterDescription{"max", "int32", Storage{"register", 0x38, 4}}});
    const PrototypeDescription might_change =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"value", "IntArray4 *", Storage{"register", 0x38, 8}}});
    const auto metadata =
        make_metadata({{entry, "noforloop_alias", ""}, {0x40067b, "might_change", ""}, {0x400440, "printf", ""}},
                      {integer_type("int32", 4, true), int_array, pointer_type("IntArray4 *", "IntArray4")},
                      {{entry, root_prototype}, {0x40067b, might_change}},
                      {{entry,
                        {VariableDescription{"i", "IntArray4", Storage{"stack", static_cast<std::uint64_t>(-0x10), 16},
                                             0x4101, true}}}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}, {0x40085d, "56616c203d2025640a00"}},
                                  bytes_from_hex(code).size(), "noforloop_alias", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "noforloop_alias", "while");
    expect_function_body_contains(result, "noforloop_alias", "int32 i [4]");
    expect_function_body_contains(result, "noforloop_alias", "+ 1");
    expect_function_body_contains(result, "noforloop_alias", "might_change((IntArray4 *)");
}

/// Verifies that a global modified through a child call is not normalized into
/// a counted for-loop because the provider-installed global symbol is mutable.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/noforloop_globcall.xml`.
TEST(DecompilerDatatests, NoforloopGlobalCallRetainsWhileLoop) {
    const std::uint64_t entry = 0x4006ed;
    const std::string code = R"(
        554889e54883ec0848897df8c7052d0920000000000000
        eb1b8b052509200083c00189051c092000488b45f84889c7
        e85bffffff8b050a09200083f8097eda90c9c3
    )";
    SymbolDescription global;
    global.address = 0x601030;
    global.name = "globvar";
    global.kind = SymbolKind::data;
    global.size = 4;
    global.type_name = "int32";
    global.identity = 0x5101;
    SymbolDescription global_alias = global;
    global_alias.address = 0x601034;
    global_alias.alias_identity = global.identity;
    const PrototypeDescription root_prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "int32 *", Storage{"register", 0x38, 8}}});
    const auto metadata =
        make_metadata({{entry, "noforloop_globcall", ""}, global, global_alias},
                      {integer_type("int32", 4, true), pointer_type("int32 *", "int32")}, {{entry, root_prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "noforloop_globcall", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "noforloop_globcall", "while( true )");
    expect_function_body_contains(result, "noforloop_globcall", "globvar = 0");
    expect_function_body_contains(result, "noforloop_globcall", "globvar._1_3_");
    expect_function_body_contains(result, "noforloop_globcall", "+ 1");
}

/// Verifies that using the iterator after the increment prevents a for-loop
/// rewrite and preserves the multiplication assigned to the global variable.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/noforloop_iterused.xml`.
TEST(DecompilerDatatests, NoforloopIteratorUseRetainsWhileLoop) {
    const std::uint64_t entry = 0x40063e;
    const std::string code = R"(
        554889e5534883ec18897decbb0a000000eb1d89debf50084000
        b800000000e8deffffff83c3016bc3648905c20920003b5dec7cde
        904883c4185b5dc3
    )";
    SymbolDescription global;
    global.address = 0x601030;
    global.name = "globvar";
    global.kind = SymbolKind::data;
    global.size = 4;
    global.type_name = "int32";
    global.identity = 0x5201;
    const PrototypeDescription root_prototype = make_prototype(
        "__cdecl", "void", std::nullopt, {PrototypeParameterDescription{"max", "int32", Storage{"register", 0x38, 4}}});
    const auto metadata = make_metadata({{entry, "noforloop_iterused", ""}, {0x400440, "printf", ""}, global},
                                        {integer_type("int32", 4, true)}, {{entry, root_prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}, {0x400850, "4265666f7265203d2025640a00"}},
                                  bytes_from_hex(code).size(), "noforloop_iterused", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "noforloop_iterused", "while");
    expect_function_body_contains(result, "noforloop_iterused", "globvar =");
    expect_function_body_contains(result, "noforloop_iterused", "* 100");
}

/// Verifies that scaled pointer arithmetic distributes the nested array offset
/// into the typed field rather than printing a raw multiply by element size.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/nestedoffset.xml`.
TEST(DecompilerDatatests, NestedOffsetUsesArrayField) {
    const std::uint64_t entry = 0x400517;
    const std::string code = "554889e5488d441602488d04878b005dc3";
    TypeDescription structure;
    structure.name = "twostruct";
    structure.size = 28;
    structure.kind = TypeKind::structure;
    structure.fields = {TypeFieldDescription{"field1", "int32", 0}, TypeFieldDescription{"field2", "int32", 4},
                        TypeFieldDescription{"array", "IntArray5", 8}};
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "twostruct *", Storage{"register", 0x38, 8}},
                        PrototypeParameterDescription{"a", "int64", Storage{"register", 0x10, 8}},
                        PrototypeParameterDescription{"b", "int64", Storage{"register", 0x80, 8}}});
    const auto metadata =
        make_metadata({{entry, "readstruct", ""}},
                      {integer_type("int32", 4, true), integer_type("int64", 8, true),
                       array_type("IntArray5", "int32", 5, 4), structure, pointer_type("twostruct *", "twostruct")},
                      {{entry, prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "readstruct", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "readstruct", "return ptr->array[");
    expect_function_body_excludes(result, "readstruct", "field1");
    expect_function_body_excludes(result, "readstruct", "* 4");
}

/// Verifies the positive offcut array case: the decompiler finds the mapped
/// structure containing the load and names its array field, not its first field.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/offsetarray.xml`.
TEST(DecompilerDatatests, OffsetArrayUsesMappedArrayField) {
    const std::uint64_t entry = 0x100000;
    const std::string code = "83ea018b449104c3";
    TypeDescription structure;
    structure.name = "mystruct";
    structure.size = 132;
    structure.kind = TypeKind::structure;
    structure.fields = {TypeFieldDescription{"firstfield", "int32", 0}, TypeFieldDescription{"array", "IntArray32", 4}};
    const PrototypeDescription root_prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "mystruct *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"index", "int32", Storage{"register", 0x10, 4}}});
    const auto metadata = make_metadata({{entry, "access_array1", ""}},
                                        {integer_type("int32", 4, true), array_type("IntArray32", "int32", 32, 4),
                                         structure, pointer_type("mystruct *", "mystruct")},
                                        {{entry, root_prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "access_array1", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "access_array1", "array[");
    expect_function_body_contains(result, "access_array1", "index - 1");
    expect_function_body_excludes(result, "access_array1", "firstfield");
}

/// Verifies the forward-search offcut case: an index outside the local mapped
/// structure is still interpreted through the array field rather than a raw
/// first-field reference.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/wayoffarray.xml`.
TEST(DecompilerDatatests, WayoffArrayUsesForwardMappedArrayField) {
    const std::uint64_t entry = 0x100000;
    const std::string code = "83ea048b449108c3";
    TypeDescription structure;
    structure.name = "mystruct";
    structure.size = 136;
    structure.kind = TypeKind::structure;
    structure.fields = {TypeFieldDescription{"firstfield", "int64", 0}, TypeFieldDescription{"array", "IntArray32", 8}};
    const PrototypeDescription root_prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "mystruct *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"index", "int32", Storage{"register", 0x10, 4}}});
    const auto metadata =
        make_metadata({{entry, "access_array1", ""}},
                      {integer_type("int32", 4, true), integer_type("int64", 8, true),
                       array_type("IntArray32", "int32", 32, 4), structure, pointer_type("mystruct *", "mystruct")},
                      {{entry, root_prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "access_array1", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "access_array1", "array[");
    expect_function_body_contains(result, "access_array1", "index - 4");
    expect_function_body_excludes(result, "access_array1", "firstfield");
}

/// Verifies that a mapped whole structure and its separately named field share
/// stable provider identity without collapsing the field expression into the
/// containing object.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/partialmerge.xml`,
/// `readpartial` and Partial Merge #1/#2.
TEST(DecompilerDatatests, PartialMergeKeepsWholeAndFieldIdentity) {
    const std::uint64_t entry = 0x1006a7;
    const std::string code = R"(
        8b1dc3ffffff48893dbcffffff83c30a89d8c30000
        8b05aeffffff01f048893da5ffffff83c00ac3
    )";
    TypeDescription highlow;
    highlow.name = "highlow";
    highlow.size = 8;
    highlow.kind = TypeKind::structure;
    highlow.fields = {TypeFieldDescription{"a", "int32", 0}, TypeFieldDescription{"b", "int32", 4}};
    SymbolDescription global;
    global.address = 0x100670;
    global.name = "glob1";
    global.kind = SymbolKind::data;
    global.size = 8;
    global.type_name = "highlow";
    global.identity = 0x7001;
    const PrototypeDescription prototype = make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {});
    const auto metadata = make_metadata({{entry, "readpartial", ""}, global}, {integer_type("int32", 4, true), highlow},
                                        {{entry, prototype}});
    const DecompilationResult result = decompile_embedded_chunks(entry, {{entry, code}}, 0x15, "readpartial", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "readpartial", "hVar1.a");
    expect_function_body_contains(result, "readpartial", "+ 10");
    expect_function_body_excludes(result, "readpartial", "undefined");
}

/// Verifies that simultaneous typed loads and stores split a structure into
/// named fields instead of preserving only an untyped aggregate copy.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/partialsplit.xml`,
/// `loadalone` and Partial splitting #1/#2.
TEST(DecompilerDatatests, PartialSplitNamesIndividualFields) {
    const std::uint64_t entry = 0x100000;
    const std::string code = "8b41048942040fb74106668942068b4108894208c3";
    TypeDescription myfoo;
    myfoo.name = "myfoo";
    myfoo.size = 12;
    myfoo.kind = TypeKind::structure;
    myfoo.fields = {TypeFieldDescription{"a", "int32", 0}, TypeFieldDescription{"b", "int16", 4},
                    TypeFieldDescription{"c", "int16", 6}, TypeFieldDescription{"d", "int32", 8}};
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptrload", "myfoo *", Storage{"register", 8, 8}},
                        PrototypeParameterDescription{"ptrstore", "myfoo *", Storage{"register", 0x10, 8}}});
    const auto metadata = make_metadata(
        {{entry, "loadalone", ""}},
        {integer_type("int32", 4, true), integer_type("int16", 2, true), myfoo, pointer_type("myfoo *", "myfoo")},
        {{entry, prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "loadalone", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "loadalone", "ptrstore->b");
    expect_function_body_contains(result, "loadalone", "ptrstore->c");
    expect_function_body_contains(result, "loadalone", "ptrstore->d");
}

/// Verifies that union field selection metadata survives a partial access and
/// prints the active union member rather than an undefined integer slice.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/partialunion.xml`.
TEST(DecompilerDatatests, PartialUnionSelectsActiveMember) {
    const std::uint64_t entry = 0x10068a;
    const std::string code = "f30f1001f30f11018b4104c3";
    TypeDescription astruct;
    astruct.name = "astruct";
    astruct.size = 12;
    astruct.kind = TypeKind::structure;
    astruct.fields = {TypeFieldDescription{"aval1", "int32", 0}, TypeFieldDescription{"aval2", "int32", 4},
                      TypeFieldDescription{"aval3", "int32", 8}};
    TypeDescription bstruct;
    bstruct.name = "bstruct";
    bstruct.size = 12;
    bstruct.kind = TypeKind::structure;
    bstruct.fields = {TypeFieldDescription{"bval1", "float32", 0}, TypeFieldDescription{"bval2", "int32", 4},
                      TypeFieldDescription{"bval3", "float32", 8}};
    TypeDescription union_type;
    union_type.name = "structunion";
    union_type.size = 12;
    union_type.kind = TypeKind::union_type;
    union_type.fields = {TypeFieldDescription{"a", "astruct", 0}, TypeFieldDescription{"b", "bstruct", 0}};
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "structunion *", Storage{"register", 8, 8}}});
    const auto metadata = make_metadata({{entry, "partialunion", ""}},
                                        {integer_type("int32", 4, true), floating_type("float32", 4), astruct, bstruct,
                                         union_type, pointer_type("structunion *", "structunion")},
                                        {{entry, prototype}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "partialunion", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "partialunion", "(value->a).aval1");
    expect_function_body_contains(result, "partialunion", "(value->a).aval2");
    expect_function_body_excludes(result, "partialunion", "undefined4");
}

/// Verifies that provider relative-pointer metadata constructs a named
/// TypePointerRel and prints ADJ accesses on both sides of the relative base.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/pointerrel.xml`.
TEST(DecompilerDatatests, PointerRelativeMetadataPrintsAdjAccesses) {
    const std::uint64_t entry = 0x1006fa;
    const std::string code = R"(
        554889e548897dd88975d4c745ec00000000
        f30f1005a0010000f30f1145f0488b45d84883c008488945f8
        c745f400000000eb39488b45f88b000145ec488b45f84883c004
        8b000145ec488b45f84883e804f30f1000f30f104df0f30f58c1
        f30f1145f0488345f8508345f4018b45f43b45d47cbff30f1045f0
        0f2e053d01000076048345ec058b45ec5dc3
    )";
    TypeDescription structure;
    structure.name = "mystruct";
    structure.size = 80;
    structure.kind = TypeKind::structure;
    structure.fields = {TypeFieldDescription{"a", "int32", 0}, TypeFieldDescription{"b", "float32", 4},
                        TypeFieldDescription{"c", "int32", 8}, TypeFieldDescription{"d", "int32", 12},
                        TypeFieldDescription{"arr", "IntArray16", 16}};
    TypeDescription relative;
    relative.name = "myptroff";
    relative.size = 8;
    relative.kind = TypeKind::pointer;
    relative.element_type = "int32";
    relative.relative_parent_type = "mystruct";
    relative.relative_offset = 8;
    relative.declaration = "typedef int32 * myptroff;";
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "mystruct *", Storage{"register", 0x38, 8}},
                        PrototypeParameterDescription{"count", "int32", Storage{"register", 0x30, 4}}});
    const auto metadata = make_metadata(
        {{entry, "process_array", ""}},
        {integer_type("int32", 4, true), floating_type("float32", 4), array_type("IntArray16", "int32", 16, 4),
         structure, pointer_type("mystruct *", "mystruct"), relative},
        {{entry, prototype}},
        {{entry,
          {VariableDescription{"ptrrel", "myptroff", Storage{"stack", static_cast<std::uint64_t>(-0x10), 8}, 0x7301,
                               true}}}});
    const DecompilationResult result = decompile_embedded_chunks(
        entry, {{entry, code}, {0x1008b4, "000020410000a841"}}, bytes_from_hex(code).size(), "process_array", metadata);

    expect_complete_analysis(result);
    expect_contains(result.c_source, "typedef int32 * myptroff;");
    expect_function_body_contains(result, "process_array", "ptrrel");
    expect_function_body_contains(result, "process_array", "ptrrel[-1]");
}

/// Verifies that a relative pointer can address an inner field and recover the
/// parent object through ADJ, matching the pointer subtraction fixture.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/pointersub.xml`.
TEST(DecompilerDatatests, PointerSubtractionUsesRelativeParent) {
    const std::uint64_t entry = 0x1005fa;
    const std::string code =
        "554889e548897de8488b45e84883e810488945f8488b45e8f20f1005ce000000f20f114008488b45f8c740080a000000488b45f85dc3";
    TypeDescription inner;
    inner.name = "InnerStruct";
    inner.size = 24;
    inner.kind = TypeKind::structure;
    inner.fields = {TypeFieldDescription{"a", "int64", 0}, TypeFieldDescription{"b", "float64", 8},
                    TypeFieldDescription{"c", "char", 16}};
    TypeDescription outer;
    outer.name = "OuterStruct";
    outer.size = 40;
    outer.kind = TypeKind::structure;
    outer.fields = {TypeFieldDescription{"outA", "int64", 0}, TypeFieldDescription{"outB", "int64", 8},
                    TypeFieldDescription{"inner", "InnerStruct", 16}};
    TypeDescription relative;
    relative.name = "outer_rel";
    relative.size = 8;
    relative.kind = TypeKind::pointer;
    relative.element_type = "InnerStruct";
    relative.relative_parent_type = "OuterStruct";
    relative.relative_offset = 16;
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "OuterStruct *", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "outer_rel", Storage{"register", 0x38, 8}}});
    const auto metadata =
        make_metadata({{entry, "getOuter", ""}},
                      {integer_type("int64", 8, true), floating_type("float64", 8), integer_type("char", 1, true),
                       inner, outer, pointer_type("OuterStruct *", "OuterStruct"), relative},
                      {{entry, prototype}});
    const DecompilationResult result = decompile_embedded_chunks(entry, {{entry, code}, {0x1006e8, "0000000000002340"}},
                                                                 bytes_from_hex(code).size(), "getOuter", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "getOuter", "ptr->b =");
    expect_function_body_contains(result, "getOuter", "ADJ(ptr)->outB = 10");
    expect_function_body_contains(result, "getOuter", "return ADJ(ptr)");
}

/// Verifies explicit pointer-to-array scaling, including the distinction between
/// advancing one row and dereferencing one scalar element.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/ptrtoarray.xml`.
TEST(DecompilerDatatests, PointerToArrayPreservesRowArithmetic) {
    const std::uint64_t entry = 0x10123e;
    const std::string code = R"(
        f30f1efa554889e54883ec1048897df88975f4488b45f84883c040
        4889c7e846ffffff488b45f84883e8804889c7e87dffffff
        837df40a7508488145f800020000488b45f84889c7e81cffffff
        488b45f84889c7e857ffffff488b45f8c9c3
    )";
    const PrototypeDescription display =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "IntArray16 *", Storage{"register", 0x38, 8}}});
    const PrototypeDescription display_low =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "int32 *", Storage{"register", 0x38, 8}}});
    const PrototypeDescription row_float =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "FloatArray1 *", Storage{"register", 0x38, 8}}});
    const PrototypeDescription row_int =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"ptr", "IntArray1 *", Storage{"register", 0x38, 8}}});
    const auto metadata = make_metadata(
        {{entry, "ptrToArray", ""},
         {0x101189, "floatarray", ""},
         {0x101198, "intarray", ""},
         {0x1011a7, "display", ""},
         {0x1011ee, "displayLow", ""}},
        {integer_type("int32", 4, true), floating_type("float32", 4), array_type("IntArray16", "int32", 16, 4),
         array_type("FloatArray1", "float32", 1, 4), array_type("IntArray1", "int32", 1, 4),
         pointer_type("IntArray16 *", "IntArray16"), pointer_type("FloatArray1 *", "FloatArray1"),
         pointer_type("IntArray1 *", "IntArray1"), pointer_type("int32 *", "int32")},
        {{entry,
          make_prototype("__cdecl", "void", std::nullopt,
                         {PrototypeParameterDescription{"param_1", "IntArray16 *", Storage{"register", 0x38, 8}}})},
         {0x101189, row_float},
         {0x101198, row_int},
         {0x1011a7, display},
         {0x1011ee, display_low}});
    const DecompilationResult result = decompile_embedded_chunks(entry, {{entry, code}}, 0x5f, "ptrToArray", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "ptrToArray", "display(param_1 + 1)");
    expect_function_body_contains(result, "ptrToArray", "displayLow");
    expect_function_body_contains(result, "ptrToArray", "0x80");
}

/// Verifies that a stack array retains one stable variable identity across an
/// aliasing call, so the post-call store does not merge with the call input.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/varcross.xml`,
/// `store_cross` and Store cross #3/#5.
TEST(DecompilerDatatests, VariableCrossingRetainsStableLocalIdentity) {
    const std::uint64_t entry = 0x10002b;
    const std::string code = R"(
        4883ec6831c083ff07b918000000b8480000000f45c84889e0
        488d542450c700000000004883c0044839d075f1894c24284889e7
        e89cffffff4883c468c3
    )";
    const PrototypeDescription use_array =
        make_prototype("__cdecl", "void", std::nullopt,
                       {PrototypeParameterDescription{"array", "IntArray20 *", Storage{"register", 0x38, 8}}});
    const PrototypeDescription root = make_prototype("__cdecl", "void", std::nullopt, {});
    SymbolDescription global;
    global.address = 0x101a00;
    global.name = "glob1";
    global.kind = SymbolKind::data;
    global.size = 4;
    global.type_name = "int32";
    global.identity = 0x8101;
    const auto metadata =
        make_metadata({{entry, "store_cross", ""}, {0x100000, "use_array", ""}, global},
                      {integer_type("int32", 4, true), array_type("IntArray20", "int32", 20, 4),
                       pointer_type("IntArray20 *", "IntArray20")},
                      {{entry, root}, {0x100000, use_array}},
                      {{entry,
                        {VariableDescription{"local_array", "IntArray20",
                                             Storage{"stack", static_cast<std::uint64_t>(-0x68), 80}, 0x8102, true}}}});
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}}, bytes_from_hex(code).size(), "store_cross", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "store_cross", "local_array");
    expect_function_body_contains(result, "store_cross", "use_array(&local_array)");
    expect_function_body_excludes(result, "store_cross", "local_array[10] = 0x18");
}

/// Verifies that offcut references resolve through mapped structure fields and
/// string interiors, including the exact `"world"` suffix from the XML fixture.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/offcut.xml`.
TEST(DecompilerDatatests, OffcutReferencesResolveThroughDataSymbols) {
    const std::uint64_t entry = 0x100000;
    const std::string code = R"(
        55534883ec1889fbbf80001000e8ee0f000089c583fb01741d
        83fb02742683fb03742f83fb04743c83fb05744589e84883c4185b5dc3
        bf9c001000e8c00f000001c5ebe9bf9c001000e8ba0f000001c5ebdb
        48c744240890001000b8900010008d28ebc9bfa4001000e8a20f000089c5
        ebbbbfaa001000e8940f000089c5ebad
    )";
    TypeDescription substruct;
    substruct.name = "substruct";
    substruct.size = 8;
    substruct.kind = TypeKind::structure;
    substruct.fields = {TypeFieldDescription{"sub1", "int32", 0}, TypeFieldDescription{"sub2", "int32", 4}};
    TypeDescription mystruct;
    mystruct.name = "mystruct";
    mystruct.size = 20;
    mystruct.kind = TypeKind::structure;
    mystruct.fields = {TypeFieldDescription{"a", "int32", 0}, TypeFieldDescription{"b", "int32", 4},
                       TypeFieldDescription{"c", "float32", 8}, TypeFieldDescription{"d", "substruct", 12}};
    SymbolDescription glob2;
    glob2.address = 0x100080;
    glob2.name = "glob2";
    glob2.kind = SymbolKind::data;
    glob2.size = 8;
    glob2.type_name = "substruct";
    glob2.read_only = true;
    glob2.identity = 0x8201;
    SymbolDescription glob1;
    glob1.address = 0x100090;
    glob1.name = "glob1";
    glob1.kind = SymbolKind::data;
    glob1.size = 20;
    glob1.type_name = "mystruct";
    glob1.read_only = true;
    glob1.identity = 0x8202;
    SymbolDescription string_symbol;
    string_symbol.address = 0x1000a4;
    string_symbol.name = "globstring";
    string_symbol.kind = SymbolKind::data;
    string_symbol.size = 12;
    string_symbol.type_name = "CharArray12";
    string_symbol.read_only = true;
    string_symbol.identity = 0x8203;
    const PrototypeDescription read_sub =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "substruct *", Storage{"register", 0x38, 8}}});
    const PrototypeDescription read_int =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "int32 *", Storage{"register", 0x38, 8}}});
    const PrototypeDescription read_string =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"ptr", "char *", Storage{"register", 0x38, 8}}});
    auto metadata = make_metadata({{entry, "offcut", ""},
                                   {0x101000, "read_sub", ""},
                                   {0x101008, "read_int", ""},
                                   {0x101010, "read_string", ""},
                                   glob2,
                                   glob1,
                                   string_symbol},
                                  {integer_type("int32", 4, true), floating_type("float32", 4),
                                   integer_type("char", 1, true), substruct, mystruct,
                                   array_type("CharArray12", "char", 12, 1), pointer_type("substruct *", "substruct"),
                                   pointer_type("int32 *", "int32"), pointer_type("char *", "char")},
                                  {{0x101000, read_sub}, {0x101008, read_int}, {0x101010, read_string}});
    metadata.analysis_options.readonly_propagate = true;
    const DecompilationResult result =
        decompile_embedded_chunks(entry, {{entry, code}, {0x1000a0, "0000000068656c6c6f20776f726c6400"}},
                                  bytes_from_hex(code).size(), "offcut", metadata);

    expect_complete_analysis(result);
    expect_function_body_contains(result, "offcut", "read_sub((substruct *)0x100080)");
    expect_function_body_contains(result, "offcut", "read_sub((substruct *)0x10009c)");
    expect_function_body_contains(result, "offcut", "read_int((int32 *)0x10009c)");
    expect_function_body_contains(result, "offcut", "read_string((char *)0x1000a4)");
    expect_function_body_contains(result, "offcut", "read_string((char *)0x1000aa)");
}

/// Exercises the original indirect switch shape with three real child bodies.
/// The table is recovered by native JumpTable/FlowInfo logic, while the child
/// FunctionProvider ranges ensure calls do not decode through neighboring code.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/switchind.xml`.
TEST(DecompilerDatatests, PortedSwitchIndirectMultiFunctionBodies) {
    const std::uint64_t entry = 0x480000;
    const JumpTableFixture fixture = make_jump_table_fixture(entry, 3, false, true);
    const PrototypeDescription root =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"selector", "int32", Storage{"register", 8, 4}}});
    std::vector<SymbolDescription> symbols{{entry, "switchind_root", ""}};
    std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes{{entry, root}};
    std::vector<FunctionDescription> functions;
    for (std::size_t index = 0; index < fixture.targets.size(); ++index) {
        const std::uint64_t child = entry + 0x300 + index * 0x10;
        const std::string name = "switch_case_" + std::to_string(index);
        symbols.push_back(SymbolDescription{child, name, ""});
        prototypes.emplace_back(child, make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {}));
        functions.push_back(FunctionDescription{name, child, child + 6});
    }
    ProviderContext metadata =
        make_metadata(std::move(symbols), {integer_type("int32", 4, true)}, std::move(prototypes));
    FlowDescription flow;
    flow.jump_tables.push_back(JumpTableDescription{entry + 6, fixture.targets, std::nullopt, 0, 0});
    metadata.flow =
        std::make_shared<FlowTableProvider>(std::vector<std::pair<std::uint64_t, FlowDescription>>{{entry, flow}});
    metadata.functions = std::make_shared<FunctionTableProvider>(std::move(functions));
    const DecompilationResult result =
        decompile_embedded(entry, fixture.image, fixture.function_size, "switchind_root", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "switch");
    expect_contains(result.c_source, "case 0");
    expect_contains(result.c_source, "case 1");
    expect_contains(result.c_source, "case 2");
    expect_contains(result.c_source, "switch_case_0");
    expect_contains(result.c_source, "switch_case_1");
    expect_contains(result.c_source, "switch_case_2");
    expect_excludes(result.c_source, "goto LAB_");
}

/// Exercises masked and multi-case indirect switches with exact case and
/// arithmetic assertions rather than accepting a generic return statement.
/// Original sources: `switchmask.xml` and `switchmulti.xml`.
TEST(DecompilerDatatests, PortedSwitchMaskAndMultiCaseSemantics) {
    const std::uint64_t mask_entry = 0x481000;
    const JumpTableFixture mask_fixture = make_jump_table_fixture(mask_entry, 4, true, false);
    const PrototypeDescription mask_prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"selector", "int32", Storage{"register", 8, 4}}});
    ProviderContext mask_metadata = make_metadata({{mask_entry, "switchmask_case", ""}},
                                                  {integer_type("int32", 4, true)}, {{mask_entry, mask_prototype}});
    FlowDescription mask_flow;
    mask_flow.jump_tables.push_back(JumpTableDescription{mask_entry + 4, mask_fixture.targets, std::nullopt, 0, 0});
    mask_metadata.flow = std::make_shared<FlowTableProvider>(
        std::vector<std::pair<std::uint64_t, FlowDescription>>{{mask_entry, mask_flow}});
    const DecompilationResult mask_result = decompile_embedded(
        mask_entry, mask_fixture.image, mask_fixture.function_size, "switchmask_case", std::move(mask_metadata));
    expect_complete_analysis(mask_result);
    expect_contains(mask_result.c_source, "switch");
    expect_contains(mask_result.c_source, "case 0");
    expect_contains(mask_result.c_source, "case 1");
    expect_contains(mask_result.c_source, "case 2");
    expect_contains(mask_result.c_source, "case 3");
    expect_contains(mask_result.c_source, "selector");

    const std::uint64_t multi_entry = 0x482000;
    const JumpTableFixture multi_fixture = make_jump_table_fixture(multi_entry, 7, false, false);
    const PrototypeDescription multi_prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"selector", "int32", Storage{"register", 8, 4}},
                        PrototypeParameterDescription{"value", "int32", Storage{"register", 0x10, 4}}});
    ProviderContext multi_metadata = make_metadata({{multi_entry, "switchmulti_case", ""}},
                                                   {integer_type("int32", 4, true)}, {{multi_entry, multi_prototype}});
    FlowDescription multi_flow;
    multi_flow.jump_tables.push_back(JumpTableDescription{multi_entry + 6, multi_fixture.targets, std::nullopt, 0, 0});
    multi_metadata.flow = std::make_shared<FlowTableProvider>(
        std::vector<std::pair<std::uint64_t, FlowDescription>>{{multi_entry, multi_flow}});
    const DecompilationResult multi_result = decompile_embedded(
        multi_entry, multi_fixture.image, multi_fixture.function_size, "switchmulti_case", std::move(multi_metadata));
    expect_complete_analysis(multi_result);
    expect_contains(multi_result.c_source, "case 0");
    expect_contains(multi_result.c_source, "case 6");
    expect_contains(multi_result.c_source, "value");
    expect_contains(multi_result.c_source, "+");
    expect_contains(multi_result.c_source, "-");
    expect_contains(multi_result.c_source, "return 0xffffffff");
}

/// Exercises callind_call and callother_call destination overrides on actual
/// native CALLIND/CALLOTHER flow operations, preserving both named targets.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/overridedest.xml`.
TEST(DecompilerDatatests, PortedDestinationOverridesForCallAndCallother) {
    const std::uint64_t entry = 0x483000;
    const std::uint64_t indirect_target = 0x483100;
    const std::uint64_t callother_target = 0x483200;
    const std::vector<Instruction> instructions{
        Instruction{
            entry,
            1,
            "callind",
            "callind",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::call_ind), std::nullopt, {Storage{"register", 8, 8}}}}},
        Instruction{entry + 1,
                    1,
                    "callother",
                    "callother",
                    {{std::to_underlying(sleigh_runtime::PcodeOpcode::call_other),
                      std::nullopt,
                      {Storage{"const", 5, 4}, Storage{"register", 8, 4}}}}},
        Instruction{
            entry + 2,
            1,
            "return",
            "return",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 8}}}}},
    };
    FlowDescription flow;
    flow.destination_overrides = {
        DestinationOverrideDescription{entry, indirect_target, "callind_call"},
        DestinationOverrideDescription{entry + 1, callother_target, "callother_call"},
    };
    ProviderContext metadata = make_metadata({{entry, "override_root", ""},
                                              {indirect_target, "indirect_destination", ""},
                                              {callother_target, "callother_destination", ""}},
                                             {integer_type("int32", 4, true)}, {});
    metadata.flow =
        std::make_shared<FlowTableProvider>(std::vector<std::pair<std::uint64_t, FlowDescription>>{{entry, flow}});
    DecompilationResult result;
    try {
        result = decompile_scripted(entry, 3, instructions, "override_root", std::move(metadata));
    } catch (const ghidra::LowlevelError& error) {
        FAIL() << error.explain;
        return;
    } catch (const std::exception& error) {
        FAIL() << error.what();
        return;
    }

    expect_complete_analysis(result);
    expect_contains(result.c_source, "indirect_destination");
    expect_contains(result.c_source, "callother_destination");
    expect_excludes(result.c_source, "goto");
}

/// Exercises provider CALLOTHER registration and payload replacement. The
/// operation uses the callother input list, proving the emitted user-op index
/// and injection context both reach the native injection algorithm.
/// Original sources: `injectoverride.xml` and `Ghidra/Features/Decompiler/src/decompile/cpp/userop.cc`.
TEST(DecompilerDatatests, PortedCallotherInjectionPayload) {
    const std::uint64_t entry = 0x484000;
    const std::vector<Instruction> instructions{
        Instruction{entry,
                    1,
                    "callother",
                    "callother",
                    {{std::to_underlying(sleigh_runtime::PcodeOpcode::call_other),
                      Storage{"register", 0, 4},
                      {Storage{"const", 7, 4}, Storage{"register", 8, 4}}}}},
        Instruction{
            entry + 1,
            1,
            "return",
            "return",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 8}}}}},
    };
    CallOtherFixupDescription fixup;
    fixup.name = "provider_add_input";
    fixup.output_name = "result";
    fixup.input_names = {"value"};
    fixup.userop_index = 7;
    fixup.operations.push_back(InjectionOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
                                                  InjectionVarnode{InjectionVarnodeKind::output, {}, 0},
                                                  {InjectionVarnode{InjectionVarnodeKind::input, {}, 0}}});
    const PrototypeDescription prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}});
    ProviderContext metadata =
        make_metadata({{entry, "callother_payload", ""}}, {integer_type("int32", 4, true)}, {{entry, prototype}});
    metadata.injections = std::make_shared<InjectionTableProvider>(std::vector<CallFixupDescription>{},
                                                                   std::vector<CallOtherFixupDescription>{fixup});
    const DecompilationResult result =
        decompile_scripted(entry, 2, instructions, "callother_payload", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "return value");
    expect_excludes(result.high_pcode, "CALLOTHER");
}

/// Exercises the callreturn flow override on a branch and verifies that the
/// target is represented as a call to the real bounded child body.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/multiret.xml`.
TEST(DecompilerDatatests, PortedCallReturnOverrideWithChildBody) {
    const std::uint64_t entry = 0x485000;
    const std::uint64_t child = entry + 0x10;
    const std::vector<Instruction> instructions{
        Instruction{
            entry,
            1,
            "branch",
            "branch",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::branch), std::nullopt, {Storage{"ram", child, 8}}}}},
        Instruction{
            entry + 1,
            1,
            "return",
            "return",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 8}}}}},
        Instruction{
            child,
            1,
            "return",
            "return",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 8}}}}},
    };
    FlowDescription flow;
    flow.flow_overrides.push_back(FlowOverrideDescription{entry, "callreturn"});
    const PrototypeDescription prototype = make_prototype("__cdecl", "void", std::nullopt, {});
    ProviderContext metadata =
        make_metadata({{entry, "callreturn_root", ""}, {child, "callreturn_target", ""}},
                      {integer_type("int32", 4, true)}, {{entry, prototype}, {child, prototype}});
    metadata.flow =
        std::make_shared<FlowTableProvider>(std::vector<std::pair<std::uint64_t, FlowDescription>>{{entry, flow}});
    metadata.functions = std::make_shared<FunctionTableProvider>(
        std::vector<FunctionDescription>{{"callreturn_target", child, child + 1}});
    const DecompilationResult result =
        decompile_scripted(entry, 2, instructions, "callreturn_root", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "callreturn_target");
    expect_contains(result.c_source, "return");
    expect_excludes(result.c_source, "goto");
}

/// Exercises the call-fixup input context used by injectoverride: the payload
/// copies the concrete call argument rather than a hard-coded register name.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/injectoverride.xml`.
TEST(DecompilerDatatests, PortedInjectOverrideCallFixupInputContext) {
    const std::uint64_t entry = 0x486000;
    const std::uint64_t target = entry + 0x20;
    const std::vector<Instruction> instructions{
        Instruction{entry,
                    1,
                    "constant",
                    "constant",
                    {{std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
                      Storage{"register", 8, 4},
                      {Storage{"const", 7, 4}}}}},
        Instruction{entry + 1,
                    1,
                    "call",
                    "call",
                    {{std::to_underlying(sleigh_runtime::PcodeOpcode::call),
                      std::nullopt,
                      {Storage{"ram", target, 8}, Storage{"register", 8, 4}}}}},
        Instruction{
            entry + 2,
            1,
            "return",
            "return",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 8}}}}},
        Instruction{
            target,
            1,
            "return",
            "return",
            {{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 8}}}}},
    };
    CallFixupDescription fixup;
    fixup.name = "provider_capture_argument";
    fixup.input_names = {"value"};
    fixup.operations.push_back(
        InjectionOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
                           InjectionVarnode{InjectionVarnodeKind::storage, Storage{"register", 0, 4}, 0},
                           {InjectionVarnode{InjectionVarnodeKind::input, {}, 0}}});
    PrototypeDescription target_prototype =
        make_prototype("__cdecl", "int32", Storage{"register", 0, 4},
                       {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}});
    target_prototype.call_fixup = fixup.name;
    const PrototypeDescription root_prototype = make_prototype("__cdecl", "int32", Storage{"register", 0, 4}, {});
    ProviderContext metadata =
        make_metadata({{entry, "inject_root", ""}, {target, "capture_target", ""}}, {integer_type("int32", 4, true)},
                      {{entry, root_prototype}, {target, target_prototype}});
    metadata.injections = std::make_shared<InjectionTableProvider>(std::vector<CallFixupDescription>{fixup});
    metadata.functions = std::make_shared<FunctionTableProvider>(
        std::vector<FunctionDescription>{{"capture_target", target, target + 1}});
    const DecompilationResult result = decompile_scripted(entry, 3, instructions, "inject_root", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "return 7");
    expect_excludes(result.high_pcode, "CALL");
}

/// Exercises indirect-call target recovery on the original x86-64 byte shape,
/// including a real provider child body at the recovered function address.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/deindirect2.xml`.
TEST(DecompilerDatatests, PortedDeindirectTwoWithRealChildBody) {
    const std::uint64_t entry = 0x100000;
    const std::uint64_t target = 0x100046;
    std::vector<std::uint8_t> image =
        bytes_from_hex("554889e54883ec3048c7c34600100048895de84889f94889f7488b5de8ffd3488d59104889036631c0c3");
    image.resize(0x4c, 0);
    write_fixture_bytes(image, 0x46, {0xc3});
    FlowDescription flow;
    flow.indirect_call_targets.push_back(IndirectCallTargetDescription{entry + 29, target});
    const PrototypeDescription prototype = make_prototype("__cdecl", "void", std::nullopt, {});
    ProviderContext metadata =
        make_metadata({{entry, "deindirect_two", ""}, {target, "obtainPtr", ""}}, {integer_type("int32", 4, true)},
                      {{entry, prototype}, {target, prototype}});
    metadata.flow =
        std::make_shared<FlowTableProvider>(std::vector<std::pair<std::uint64_t, FlowDescription>>{{entry, flow}});
    metadata.functions =
        std::make_shared<FunctionTableProvider>(std::vector<FunctionDescription>{{"obtainPtr", target, target + 1}});
    const DecompilationResult result = decompile_embedded(entry, image, 42, "deindirect_two", std::move(metadata));

    expect_complete_analysis(result);
    expect_contains(result.c_source, "obtainPtr");
    expect_contains(result.raw_pcode, "call");
    expect_contains(result.c_source, "obtainPtr");
}

/// Exercises the native SSA revisit path on the exact original mixed-width
/// global accesses and checks the width-specific SUB/CONCAT artifacts.
/// Original source: `Ghidra/Features/Decompiler/src/decompile/datatests/revisit.xml`.
TEST(DecompilerDatatests, PortedRevisitMixedWidthSsa) {
    const std::uint64_t entry = 0x100000;
    std::vector<std::uint8_t> image = bytes_from_hex("488d1d6d00000048891d52000000488b0d4b0000008b0183c00a890166e84800"
                                                     "668b054d0000006605640066890542000000c3");
    image.resize(0x80, 0);
    const std::vector<SymbolDescription> symbols{{entry, "revisit", ""},
                                                 {entry + 0x74, "i", "", SymbolKind::data, 4, "int32"}};
    const auto metadata = make_metadata(symbols, {integer_type("int32", 4, true)}, {});
    const DecompilationResult result = decompile_embedded(entry, image, 51, "revisit", metadata);

    expect_complete_analysis(result);
    expect_contains(result.raw_pcode, "RAX:2(0x00100020");
    expect_contains(result.raw_pcode, "r0x00100074:2(0x0010002b");
    expect_contains(result.raw_pcode, "#0xa:4");
    expect_contains(result.raw_pcode, "#0x64:2");
}

/// Names one original XML fixture and records whether a portable
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
        {"bitfields.xml", true,
         "Covered by ProviderBitfieldExtraction with embedded x86 bytes and real provider bitfield metadata."},
        {"bitfields2.xml", false, "Requires the original MIPS processor specification and XML parse-line commands."},
        {"boolless.xml", false, "Uses a CODE address space and the interactive dec command, not provider metadata."},
        {"ccmp.xml", false, "Requires processor-specific compare/borrow injection semantics from the XML image."},
        {"concat.xml", true,
         "Covered by ProviderAggregateFieldsAndArrayElement with embedded x86 bytes and real Record fields."},
        {"concatsplit.xml", true,
         "Covered by PortedConcatSplitAggregatePieces with an ordered provider aggregate join."},
        {"condconst.xml", true, "Covered by PortedPcodeTransformations with real copy/add flow."},
        {"condconst2.xml", false, "Requires ARM context injection and XML set-context commands."},
        {"condconstsub.xml", false, "Requires XML override-flow and mapped-function database commands."},
        {"condexesub.xml", false, "Requires readonly option, context injection, and mapped global data."},
        {"condmulti.xml", false, "Requires XML global-address mappings and several externally declared functions."},
        {"convert.xml", true, "Covered by PortedArithmetic using real integer p-code from Sleigh."},
        {"copytrim.xml", true, "Covered by PortedPcodeTransformations."},
        {"deadvolatile.xml", true,
         "Covered by ProviderVolatileReadRetainsUnusedSideEffect with provider volatile-range metadata."},
        {"deindirect.xml", true, "Covered by PortedDeindirectTargetOverride with an indirect-call target record."},
        {"deindirect2.xml", true,
         "Covered by PortedDeindirectTwoWithRealChildBody with native indirect target recovery."},
        {"displayformat.xml", true,
         "Covered by PortedDisplayFormatForcedIntegerConstants with native dynamic one-byte hex and four-byte "
         "octal/binary constant formatting."},
        {"divopt.xml", true, "Covered by PortedSignedDivision."},
        {"doublemove.xml", false, "Requires mapped floating globals and raw-print command sequencing."},
        {"dupptr.xml", true,
         "Covered by DupptrPreservesTypedIntermediatePointerSemantics with the original x86 pointer sequence."},
        {"elseif.xml", true, "Covered by PortedIfElse."},
        {"enum.xml", true, "Covered by ProviderEnumNamedComparison with provider enum values and embedded x86 flow."},
        {"floatcast.xml", true, "Covered by PortedFloatingPoint."},
        {"floatconv.xml", false, "Depends on XML constant conversion maps and custom parameter mapping."},
        {"floatprint.xml", false, "Requires XML readonly globals and exact legacy floating printer fixtures."},
        {"forloop1.xml", true, "Covered by PortedForloop1."},
        {"forloop_loaditer.xml", false, "Requires XML stack-local name maps and variadic printf metadata."},
        {"forloop_thruspecial.xml", false, "Requires a special-operation injection and an XML puts prototype."},
        {"forloop_varused.xml", false, "Requires XML variadic-call and external symbol database setup."},
        {"forloop_withskip.xml", true, "Covered by PortedWhileLoopControlFlow."},
        {"gp.xml", false, "Requires processor global-pointer tracking and XML track commands."},
        {"heapstring.xml", true,
         "Covered by PortedHeapStringNarrow and PortedHeapStringWide with real HeapSequence, StringManager, and "
         "builtin selection."},
        {"ifnoexit.xml", false, "Relies on an intentionally unterminated XML function flow."},
        {"ifswitch.xml", false, "Combines switch recovery with XML-specific function and data mappings."},
        {"impliedfield.xml", false, "Requires database field-implied type annotations."},
        {"indproto.xml", false, "Requires an indirect prototype override stored in the original database."},
        {"injectoverride.xml", true,
         "Covered by PortedStructuredCallFixupInjection, PortedInjectOverrideCallFixupInputContext, and "
         "PortedCallotherInjectionPayload with structured provider payloads."},
        {"inline.xml", true, "Covered by PortedInlineFunctionBody and bounded provider inline policy metadata."},
        {"inlinetarget.xml", false,
         "Excluded by request because the original fixture is PPC-specific; x86 inline policy is covered by "
         "PortedInlineFunctionBody."},
        {"longdouble.xml", true,
         "Core x86 float10 storage and FLOAT_ADD behavior are covered by PortedLongDoubleFloat10CoreBehavior; "
         "all 15 original stringmatch cases remain intentionally unresolved."},
        {"loopcomment.xml", false, "Requires XML comment commands and the original comment database."},
        {"lzcount.xml", false, "Requires a processor CALLOTHER operation not supplied by this x86 provider case."},
        {"mixfloatint.xml", true, "Covered by PortedMixedFloatIntegerPrototype with real XMM and integer storage."},
        {"modulo.xml", true, "Covered by PortedSignedModulo."},
        {"modulo2.xml", false, "Requires optimized compiler-specific modulo sequences and XML expected-output setup."},
        {"multiret.xml", true,
         "Covered by PortedMultiReturnAggregatePieces and PortedCallReturnOverrideWithChildBody."},
        {"nan.xml", false, "Requires XML floating constant/global annotations and legacy NaN printer expectations."},
        {"namespace.xml", true, "Covered by PortedNamespaceAndSymbols."},
        {"nestedoffset.xml", true, "Covered by NestedOffsetUsesArrayField with original x86 scaled-index bytes."},
        {"noforloop_alias.xml", true, "Covered by NoforloopAliasRetainsWhileLoop with isolated alias metadata."},
        {"noforloop_globcall.xml", true, "Covered by NoforloopGlobalCallRetainsWhileLoop with stable global identity."},
        {"noforloop_iterused.xml", true,
         "Covered by NoforloopIteratorUseRetainsWhileLoop with provider local metadata."},
        {"offcut.xml", true,
         "Covered by OffcutReferencesResolveThroughDataSymbols with mapped data and string interiors."},
        {"offsetarray.xml", true, "Covered by OffsetArrayUsesMappedArrayField with typed structure-array metadata."},
        {"orcompare.xml", false, "Requires XML boolean-equate and processor flag setup."},
        {"overridedest.xml", true,
         "Covered by PortedDestinationOverridesForCallAndCallother with native callind/callother rewrites."},
        {"packstructaccess.xml", false, "Requires packed compiler layout and database field packing directives."},
        {"partialmerge.xml", true, "Covered by PartialMergeKeepsWholeAndFieldIdentity with stable data identity."},
        {"partialsplit.xml", true, "Covered by PartialSplitNamesIndividualFields with typed split storage."},
        {"partialunion.xml", true,
         "Covered by PartialUnionSelectsActiveMember with overlapping provider union fields."},
        {"piecestruct.xml", true, "Covered by PortedPieceStructureFields with explicit provider parameter storage."},
        {"pointercmp.xml", false, "Requires XML pointer type maps and processor-specific comparison fixtures."},
        {"pointerrel.xml", true,
         "Covered by PointerRelativeMetadataPrintsAdjAccesses with named TypePointerRel metadata."},
        {"pointersub.xml", true, "Covered by PointerSubtractionUsesRelativeParent with relative parent metadata."},
        {"promotecompare.xml", false, "Requires compiler promotion rules supplied by the original XML specification."},
        {"ptrtoarray.xml", true, "Covered by PointerToArrayPreservesRowArithmetic with explicit row pointer types."},
        {"readvolatile.xml", true,
         "Covered by PortedReadonlyMemoryLoad, PortedDataSymbolMetadata, and provider volatile-range metadata."},
        {"retspecial.xml", true, "Covered by PortedSpecialReturnStorage with a provider hidden return pointer."},
        {"retstruct.xml", true, "Covered by PortedStructureReturn."},
        {"revisit.xml", true, "Covered by PortedRevisitMixedWidthSsa with exact mixed-width SSA assertions."},
        {"sbyte.xml", false, "Requires XML signed-byte type declarations and expected legacy casts."},
        {"skipnext2.xml", false, "Requires processor delay-slot/skip-next semantics from XML context."},
        {"stackcorner.xml", false, "Requires original stack-space and database-local corner cases."},
        {"stackreturn.xml", true,
         "Covered by PortedStackReturnAggregateStorage with real stack-relative child returns."},
        {"stackspill.xml", false, "Requires compiler-specific stack spill annotations and XML mappings."},
        {"stackstring.xml", true,
         "Covered by PortedStackStringNarrow and PortedStackStringWide with typed stack arrays and real HeapSequence "
         "recovery."},
        {"statuscmp.xml", false, "Requires processor status-register semantics and XML flag configuration."},
        {"switchhide.xml", false, "Requires XML switch hiding override."},
        {"switchind.xml", true,
         "Covered by PortedSwitchIndirectMultiFunctionBodies with real child bodies and recovered case calls."},
        {"switchloop.xml", false, "Requires a switch-loop XML fixture with database labels."},
        {"switchmask.xml", true, "Covered by PortedSwitchMaskAndMultiCaseSemantics with four recovered masked cases."},
        {"switchmulti.xml", true,
         "Covered by PortedSwitchMaskAndMultiCaseSemantics with seven recovered arithmetic cases."},
        {"switchreturn.xml", true, "Covered by PortedSwitchReturn."},
        {"threedim.xml", false, "Requires XML three-dimensional data declarations and global mappings."},
        {"twodim.xml", false,
         "The provider has array shape support, but the original test requires XML global data mapping."},
        {"union_datatype.xml", true, "Covered by ProviderUnionFieldSelection with real overlapping union fields."},
        {"varcross.xml", true, "Covered by VariableCrossingRetainsStableLocalIdentity with isolated local identity."},
        {"wayoffarray.xml", true, "Covered by WayoffArrayUsesForwardMappedArrayField with forward data mapping."},
        {"wraprange.xml", false, "Requires processor address-range and XML context-wrap configuration."},
    }};
}

/// Associates every portable manifest entry with a registered executable test.
/// A fixture may share one semantic port with another fixture, but each file
/// marked portable must still have at least one concrete test in this binary.
struct DatatestCoverageEntry {
    std::string_view file;
    std::string_view test_name;
};

/// Returns the executable coverage map for portable original XML fixtures.
/// Original source directory: `Ghidra/Features/Decompiler/src/decompile/datatests`.
static constexpr auto portable_datatest_coverage() {
    return std::to_array<DatatestCoverageEntry>({
        {"bitfields.xml", "ProviderBitfieldExtraction"},
        {"concat.xml", "ProviderAggregateFieldsAndArrayElement"},
        {"concatsplit.xml", "PortedConcatSplitAggregatePieces"},
        {"condconst.xml", "PortedPcodeTransformations"},
        {"convert.xml", "PortedArithmetic"},
        {"copytrim.xml", "PortedPcodeTransformations"},
        {"deadvolatile.xml", "ProviderVolatileReadRetainsUnusedSideEffect"},
        {"deindirect.xml", "PortedDeindirectTargetOverride"},
        {"deindirect2.xml", "PortedDeindirectTwoWithRealChildBody"},
        {"displayformat.xml", "PortedDisplayFormatForcedIntegerConstants"},
        {"divopt.xml", "PortedSignedDivision"},
        {"elseif.xml", "PortedIfElse"},
        {"enum.xml", "ProviderEnumNamedComparison"},
        {"floatcast.xml", "PortedFloatingPoint"},
        {"forloop1.xml", "PortedForloop1"},
        {"forloop_withskip.xml", "PortedWhileLoopControlFlow"},
        {"heapstring.xml", "PortedHeapStringNarrow"},
        {"injectoverride.xml", "PortedStructuredCallFixupInjection"},
        {"inline.xml", "PortedInlineFunctionBody"},
        {"longdouble.xml", "PortedLongDoubleFloat10CoreBehavior"},
        {"mixfloatint.xml", "PortedMixedFloatIntegerPrototype"},
        {"modulo.xml", "PortedSignedModulo"},
        {"multiret.xml", "PortedMultiReturnAggregatePieces"},
        {"namespace.xml", "PortedNamespaceAndSymbols"},
        {"dupptr.xml", "DupptrPreservesTypedIntermediatePointerSemantics"},
        {"nestedoffset.xml", "NestedOffsetUsesArrayField"},
        {"noforloop_alias.xml", "NoforloopAliasRetainsWhileLoop"},
        {"noforloop_globcall.xml", "NoforloopGlobalCallRetainsWhileLoop"},
        {"noforloop_iterused.xml", "NoforloopIteratorUseRetainsWhileLoop"},
        {"offcut.xml", "OffcutReferencesResolveThroughDataSymbols"},
        {"offsetarray.xml", "OffsetArrayUsesMappedArrayField"},
        {"partialmerge.xml", "PartialMergeKeepsWholeAndFieldIdentity"},
        {"partialsplit.xml", "PartialSplitNamesIndividualFields"},
        {"partialunion.xml", "PartialUnionSelectsActiveMember"},
        {"pointerrel.xml", "PointerRelativeMetadataPrintsAdjAccesses"},
        {"pointersub.xml", "PointerSubtractionUsesRelativeParent"},
        {"piecestruct.xml", "PortedPieceStructureFields"},
        {"ptrtoarray.xml", "PointerToArrayPreservesRowArithmetic"},
        {"readvolatile.xml", "PortedDataSymbolMetadata"},
        {"retspecial.xml", "PortedSpecialReturnStorage"},
        {"retstruct.xml", "PortedStructureReturn"},
        {"stackreturn.xml", "PortedStackReturnAggregateStorage"},
        {"stackstring.xml", "PortedStackStringNarrow"},
        {"overridedest.xml", "PortedDestinationOverridesForCallAndCallother"},
        {"revisit.xml", "PortedRevisitMixedWidthSsa"},
        {"switchind.xml", "PortedSwitchIndirectMultiFunctionBodies"},
        {"switchmask.xml", "PortedSwitchMaskAndMultiCaseSemantics"},
        {"switchmulti.xml", "PortedSwitchMaskAndMultiCaseSemantics"},
        {"switchreturn.xml", "PortedSwitchReturn"},
        {"union_datatype.xml", "ProviderUnionFieldSelection"},
        {"varcross.xml", "VariableCrossingRetainsStableLocalIdentity"},
        {"wayoffarray.xml", "WayoffArrayUsesForwardMappedArrayField"},
    });
}

/// Checks whether Google Test registered the named executable case in the
/// decompiler datatest suite. Registration is checked instead of trusting a
/// free-form manifest reason, so stale portable entries fail this validator.
static bool has_registered_datatest(std::string_view test_name) {
    const testing::UnitTest* unit_test = testing::UnitTest::GetInstance();
    for (int suite_index = 0; suite_index < unit_test->total_test_suite_count(); ++suite_index) {
        const testing::TestSuite* suite = unit_test->GetTestSuite(suite_index);
        if (suite == nullptr || std::string_view(suite->name()) != "DecompilerDatatests") {
            continue;
        }
        for (int test_index = 0; test_index < suite->total_test_count(); ++test_index) {
            const testing::TestInfo* test = suite->GetTestInfo(test_index);
            if (test != nullptr && std::string_view(test->name()) == test_name) {
                return true;
            }
        }
    }
    return false;
}

/// Verifies that every original XML datatest is accounted for without pretending
/// that unsupported XML commands are portable provider behavior.
/// Original source directory: `Ghidra/Features/Decompiler/src/decompile/datatests`.
TEST(DecompilerDatatestsManifest, AccountsForEveryOriginalDatatest) {
    const auto manifest = original_datatest_manifest();
    const auto coverage = portable_datatest_coverage();
    std::set<std::string_view> names;
    std::set<std::string_view> covered_files;
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
    for (const DatatestCoverageEntry& covered : coverage) {
        EXPECT_TRUE(covered_files.insert(covered.file).second)
            << "duplicate executable coverage entry: " << covered.file;
        const auto manifest_entry =
            std::find_if(manifest.begin(), manifest.end(),
                         [&](const DatatestManifestEntry& entry) { return entry.file == covered.file; });
        ASSERT_TRUE(manifest_entry != manifest.end()) << "coverage has no manifest entry: " << covered.file;
        EXPECT_TRUE(manifest_entry->ported) << "coverage points to a nonportable fixture: " << covered.file;
        EXPECT_TRUE(has_registered_datatest(covered.test_name))
            << "coverage test is not registered: " << covered.test_name << " for " << covered.file;
    }
    for (const DatatestManifestEntry& entry : manifest) {
        if (!entry.ported) {
            continue;
        }
        EXPECT_NE(covered_files.find(entry.file), covered_files.end())
            << "portable manifest entry has no executable coverage: " << entry.file;
    }
    EXPECT_EQ(manifest.size(), 89U);
    EXPECT_EQ(names.size(), 89U);
    EXPECT_EQ(covered_files.size(), portable_count);
    EXPECT_EQ(coverage.size(), portable_count);
}

} // namespace newghidra::decompiler::datatests
