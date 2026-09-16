module;

#include <gtest/gtest.h>

export module metadata_provider_tests;

import decompiler;
import ghidra.decompiler;
import sleigh_runtime;
import std;

namespace newghidra::decompiler::metadata_tests {

/// Provides one deterministic COPY/RETURN body so metadata tests execute the
/// real native prototype, type, symbol, action, and printer pipeline.
class MetadataInstructionProvider final : public PcodeProvider {
public:
    /// Selects whether the body returns a mapped data symbol or an immediate.
    explicit MetadataInstructionProvider(bool load_data, std::uint64_t load_address = 0x1000,
                                         std::uint32_t load_size = 8)
        : load_data_(load_data), load_address_(load_address), load_size_(load_size) {}

    /// Emits one bounded instruction with native p-code and rejects other addresses.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(ProviderError{"MetadataInstructionProvider has no instruction at this address"});
        }
        Instruction instruction;
        instruction.address = address;
        instruction.length = 1;
        instruction.mnemonic = load_data_ ? "load" : "copy";
        instruction.assembly = load_data_ ? "RAX, [data]" : "RAX, 1";
        if (load_data_) {
            instruction.pcode.push_back(PcodeOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::load),
                                                       Storage{"register", 0, load_size_},
                                                       {Storage{"const", load_address_, 8}},
                                                       std::optional<std::string>{"ram"}});
            instruction.pcode.push_back(PcodeOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::return_op),
                                                       std::nullopt,
                                                       {Storage{"register", 0, load_size_}}});
        } else {
            instruction.pcode.push_back(PcodeOperation{std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
                                                       Storage{"register", 0, 4},
                                                       {Storage{"const", 1, 4}}});
            instruction.pcode.push_back(PcodeOperation{
                std::to_underlying(sleigh_runtime::PcodeOpcode::return_op), std::nullopt, {Storage{"const", 0, 4}}});
        }
        return instruction;
    }

private:
    bool load_data_;
    std::uint64_t load_address_;
    std::uint32_t load_size_;
};

/// Supplies recursive provider types used to verify each requested metadata
/// shape is materialized by the native TypeFactory rather than a fallback.
class MetadataTypeProvider final : public TypeProvider {
public:
    /// Stores the immutable named type records used by this fixture.
    explicit MetadataTypeProvider(std::vector<TypeDescription> types) {
        for (TypeDescription& type : types) {
            std::string name = type.name;
            types_.emplace(std::move(name), std::move(type));
        }
    }

    /// Resolves one type by its provider name.
    [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
        const auto found = types_.find(std::string(name));
        if (found == types_.end()) {
            return std::nullopt;
        }
        return found->second;
    }

private:
    std::map<std::string, TypeDescription> types_;
};

/// Supplies a function name and an address-tied namespaced data object.
class MetadataSymbolProvider final : public SymbolProvider {
public:
    /// Constructs the provider with the one mapped data record under test.
    MetadataSymbolProvider() {
        data_.kind = SymbolKind::data;
        data_.address = 0x1000;
        data_.name = "counter";
        data_.namespace_name = "runtime::state";
        data_.size = 8;
        data_.type_name = "uint8";
        data_.read_only = true;
        data_.display_format = DisplayFormat::hexadecimal;
    }

    /// Returns the root function name and rejects unrelated lookup addresses.
    [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
        if (address == 0) {
            return SymbolDescription{0, "read_counter", ""};
        }
        return std::nullopt;
    }

    /// Enumerates the data record so it is installed before native flow analysis.
    [[nodiscard]] std::vector<SymbolDescription> symbols() const override {
        return {data_};
    }

private:
    SymbolDescription data_;
};

/// Supplies a simple custom prototype that forces all recursive metadata types
/// to resolve before the native action and printer execute.
class MetadataPrototypeProvider final : public PrototypeProvider {
public:
    /// Returns the aggregate-bearing prototype at the only function entry.
    [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
        if (address != 0) {
            return std::nullopt;
        }
        PrototypeDescription prototype;
        prototype.calling_convention = "__cdecl";
        prototype.return_type = "uint4";
        prototype.return_storage = Storage{"register", 0, 4};
        prototype.parameters = {
            PrototypeParameterDescription{"flags", "FlagsAlias", Storage{"register", 8, 4}},
            PrototypeParameterDescription{"state", "State", Storage{"register", 0x10, 4}},
            PrototypeParameterDescription{"payload", "Payload", Storage{"register", 0x18, 4}},
            PrototypeParameterDescription{"bytes", "ByteArray", Storage{"register", 0x20, 4}},
        };
        return prototype;
    }
};

/// Builds the x86-like architecture used by the provider frontend fixtures.
static ArchitectureDescription metadata_architecture() {
    ArchitectureDescription architecture;
    architecture.name = "metadata-provider-x86-64";
    architecture.calling_convention = "__cdecl";
    architecture.spaces = {
        SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    architecture.registers = {
        RegisterDescription{"RAX", Storage{"register", 0, 8}},
        RegisterDescription{"RCX", Storage{"register", 8, 8}},
        RegisterDescription{"RDX", Storage{"register", 0x10, 8}},
        RegisterDescription{"R8", Storage{"register", 0x18, 8}},
        RegisterDescription{"R9", Storage{"register", 0x20, 8}},
        RegisterDescription{"RSP", Storage{"register", 0x28, 8}},
    };
    return architecture;
}

/// Creates all recursive shapes from the provider API, including native
/// declaration text used to verify that source-level metadata is preserved.
static std::vector<TypeDescription> metadata_types() {
    TypeDescription uint1;
    uint1.name = "uint1";
    uint1.size = 1;
    uint1.kind = TypeKind::unsigned_integer;
    uint1.signed_value = false;

    TypeDescription uint4;
    uint4.name = "uint4";
    uint4.size = 4;
    uint4.kind = TypeKind::unsigned_integer;
    uint4.signed_value = false;
    uint4.display_format = DisplayFormat::hexadecimal;

    TypeDescription uint8;
    uint8.name = "uint8";
    uint8.size = 8;
    uint8.kind = TypeKind::unsigned_integer;
    uint8.signed_value = false;

    TypeDescription float4;
    float4.name = "float4";
    float4.size = 4;
    float4.kind = TypeKind::floating_point;

    TypeDescription flags;
    flags.name = "Flags";
    flags.size = 4;
    flags.kind = TypeKind::structure;
    flags.signed_value = false;
    flags.declaration = "struct Flags { uint4 enabled:1; uint4 mode:3; uint4 reserved:28; };";
    flags.bitfields = {
        TypeBitFieldDescription{"enabled", "uint4", 1, 0},
        TypeBitFieldDescription{"mode", "uint4", 3, 0},
        TypeBitFieldDescription{"reserved", "uint4", 28, 0},
    };

    TypeDescription state;
    state.name = "State";
    state.size = 4;
    state.kind = TypeKind::enumeration;
    state.signed_value = false;
    state.declaration = "enum State { StateIdle = 0, StateReady = 1 };";
    state.enum_values = {
        TypeEnumValueDescription{"StateIdle", 0},
        TypeEnumValueDescription{"StateReady", 1},
    };

    TypeDescription payload;
    payload.name = "Payload";
    payload.size = 4;
    payload.kind = TypeKind::union_type;
    payload.signed_value = false;
    payload.declaration = "union Payload { uint4 word; float4 real; };";
    payload.fields = {
        TypeFieldDescription{"word", "uint4", 0},
        TypeFieldDescription{"real", "float4", 0},
    };

    TypeDescription byte_array;
    byte_array.name = "ByteArray";
    byte_array.size = 4;
    byte_array.kind = TypeKind::array;
    byte_array.signed_value = false;
    byte_array.element_type = "uint1";
    byte_array.element_count = 4;

    TypeDescription flags_alias;
    flags_alias.name = "FlagsAlias";
    flags_alias.size = 4;
    flags_alias.kind = TypeKind::typedef_type;
    flags_alias.signed_value = false;
    flags_alias.element_type = "Flags";
    flags_alias.declaration = "typedef struct Flags FlagsAlias;";

    return {std::move(uint1), std::move(uint4),   std::move(uint8),      std::move(float4),     std::move(flags),
            std::move(state), std::move(payload), std::move(byte_array), std::move(flags_alias)};
}

/// Creates a provider context for the recursive type materialization test.
static ProviderContext recursive_metadata_context() {
    ProviderContext context;
    context.pcode = std::make_shared<MetadataInstructionProvider>(false);
    context.memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0});
    context.symbols = std::make_shared<MetadataSymbolProvider>();
    context.types = std::make_shared<MetadataTypeProvider>(metadata_types());
    context.prototypes = std::make_shared<MetadataPrototypeProvider>();
    return context;
}

/// Verifies that enum values, bitfields, typedefs, arrays, structures, and
/// unions all survive recursive provider resolution and source emission.
TEST(MetadataProvider, MaterializesRecursiveHighValueTypes) {
    Decompiler decompiler(metadata_architecture(), recursive_metadata_context());
    const DecompilationResult result = decompiler.decompile(FunctionDescription{"metadata_types", 0, 1});

    ASSERT_FALSE(result.c_source.empty());
    EXPECT_NE(result.c_source.find("struct Flags"), std::string::npos);
    EXPECT_NE(result.c_source.find("enabled:1"), std::string::npos);
    EXPECT_NE(result.c_source.find("enum State"), std::string::npos);
    EXPECT_NE(result.c_source.find("union Payload"), std::string::npos);
    EXPECT_NE(result.c_source.find("bytes"), std::string::npos) << result.c_source;
    EXPECT_NE(result.c_source.find("FlagsAlias"), std::string::npos);
}

/// Verifies that a namespaced data symbol is mapped at its address before
/// analysis and retains its provider-forced hexadecimal display format.
TEST(MetadataProvider, MapsNamespacedDataSymbolWithDisplayFormat) {
    ProviderContext context;
    context.pcode = std::make_shared<MetadataInstructionProvider>(true);
    context.memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0});
    context.symbols = std::make_shared<MetadataSymbolProvider>();
    context.types = std::make_shared<MetadataTypeProvider>(metadata_types());
    PrototypeDescription prototype;
    prototype.calling_convention = "__cdecl";
    prototype.return_type = "uint8";
    prototype.return_storage = Storage{"register", 0, 8};
    /// Supplies the mapped-data prototype without adding another shared fixture.
    class SinglePrototypeProvider final : public PrototypeProvider {
    public:
        /// Stores the prototype used by the mapped data-symbol test.
        explicit SinglePrototypeProvider(PrototypeDescription prototype) : prototype_(std::move(prototype)) {}

        /// Returns the stored prototype at the root function.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            return address == 0 ? std::optional<PrototypeDescription>{prototype_} : std::nullopt;
        }

    private:
        PrototypeDescription prototype_;
    };
    context.prototypes = std::make_shared<SinglePrototypeProvider>(std::move(prototype));

    Decompiler decompiler(metadata_architecture(), std::move(context));
    const DecompilationResult result = decompiler.decompile(FunctionDescription{"read_counter", 0, 1});

    ASSERT_FALSE(result.c_source.empty());
    EXPECT_NE(result.c_source.find("runtime::state::counter"), std::string::npos) << result.c_source;
}

/// Verifies that a mapped object remains discoverable when a load starts at an
/// interior byte, which is the provider-side contract needed by offcut data.
TEST(MetadataProvider, ResolvesOffcutDataSymbol) {
    ProviderContext context;
    context.pcode = std::make_shared<MetadataInstructionProvider>(true, 0x1002, 4);
    context.memory = std::make_shared<SparseMemory>(0, std::vector<std::uint8_t>{0});
    context.symbols = std::make_shared<MetadataSymbolProvider>();
    context.types = std::make_shared<MetadataTypeProvider>(metadata_types());
    PrototypeDescription prototype;
    prototype.calling_convention = "__cdecl";
    prototype.return_type = "uint4";
    prototype.return_storage = Storage{"register", 0, 4};
    /// Supplies the prototype used by the offcut mapping test.
    class SinglePrototypeProvider final : public PrototypeProvider {
    public:
        /// Stores the prototype used by the offcut mapping test.
        explicit SinglePrototypeProvider(PrototypeDescription prototype) : prototype_(std::move(prototype)) {}

        /// Returns the stored prototype at the root function.
        [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
            return address == 0 ? std::optional<PrototypeDescription>{prototype_} : std::nullopt;
        }

    private:
        PrototypeDescription prototype_;
    };
    context.prototypes = std::make_shared<SinglePrototypeProvider>(std::move(prototype));

    Decompiler decompiler(metadata_architecture(), std::move(context));
    const DecompilationResult result = decompiler.decompile(FunctionDescription{"read_counter_offcut", 0, 1});

    ASSERT_FALSE(result.c_source.empty());
    EXPECT_NE(result.c_source.find("counter"), std::string::npos) << result.c_source;
}

} // namespace newghidra::decompiler::metadata_tests
