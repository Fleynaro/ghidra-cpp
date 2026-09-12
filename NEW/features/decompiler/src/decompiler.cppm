export module decompiler;

import std;

export namespace newghidra::decompiler {

/// Describes one concrete p-code storage location supplied by a provider.
struct Storage {
    std::string space;
    std::uint64_t offset = 0;
    std::uint32_t size = 0;
};

/// Describes one p-code operation before it is materialized in the native engine.
struct PcodeOperation {
    std::uint32_t opcode = 0;
    std::optional<Storage> output;
    std::vector<Storage> inputs;
    std::optional<std::string> memory_space;
};

/// Describes one decoded instruction and its raw p-code sequence.
struct Instruction {
    std::uint64_t address = 0;
    std::size_t length = 0;
    std::string mnemonic;
    std::string assembly;
    std::vector<PcodeOperation> pcode;
};

/// Carries a provider failure without exposing any decoder implementation type.
struct ProviderError {
    std::string message;
};

/// Provides decoded instructions to the decompiler's Translate boundary.
class PcodeProvider {
public:
    /// Releases the provider through its interface.
    virtual ~PcodeProvider() = default;

    /// Decodes the instruction beginning at `address`.
    [[nodiscard]] virtual std::expected<Instruction, ProviderError> decode(std::uint64_t address) const = 0;
};

/// Supplies bytes and immutable memory metadata to the native LoadImage boundary.
class MemoryProvider {
public:
    /// Releases the provider through its interface.
    virtual ~MemoryProvider() = default;

    /// Reads exactly `size` bytes beginning at `address`.
    [[nodiscard]] virtual std::expected<std::vector<std::uint8_t>, ProviderError> read(std::uint64_t address,
                                                                                       std::size_t size) const = 0;
};

/// Describes one externally supplied symbol or function name.
struct SymbolDescription {
    std::uint64_t address = 0;
    std::string name;
    std::string namespace_name;
};

/// Supplies symbols independently of the loader or database implementation.
class SymbolProvider {
public:
    /// Releases the provider through its interface.
    virtual ~SymbolProvider() = default;

    /// Returns a symbol at `address`, if one is known.
    [[nodiscard]] virtual std::optional<SymbolDescription> symbol_at(std::uint64_t address) const = 0;
};

/// Classifies an externally supplied type declaration.
enum class TypeKind {
    void_type,
    boolean,
    signed_integer,
    unsigned_integer,
    floating_point,
    unicode_character,
    pointer,
    array,
    structure,
    union_type,
    typedef_type,
};

/// Describes one field in an externally supplied structure or union.
struct TypeFieldDescription {
    std::string name;
    std::string type_name;
    std::uint32_t offset = 0;
};

/// Describes an externally supplied type declaration.
struct TypeDescription {
    std::string name;
    std::uint32_t size = 0;
    std::string declaration;
    TypeKind kind = TypeKind::signed_integer;
    bool signed_value = true;
    std::string element_type;
    std::uint32_t element_count = 0;
    std::vector<TypeFieldDescription> fields;
};

/// Supplies primitive, typedef, array, and structure declarations.
class TypeProvider {
public:
    /// Releases the provider through its interface.
    virtual ~TypeProvider() = default;

    /// Resolves a type by its provider-defined name.
    [[nodiscard]] virtual std::optional<TypeDescription> type_named(std::string_view name) const = 0;
};

/// Describes one externally supplied parameter and its native storage.
struct PrototypeParameterDescription {
    std::string name;
    std::string type_name;
    std::optional<Storage> storage;
};

/// Describes a function prototype independently of compiler-spec XML.
struct PrototypeDescription {
    std::string calling_convention = "default";
    std::string return_type = "void";
    std::optional<Storage> return_storage;
    std::vector<PrototypeParameterDescription> parameters;
};

/// Supplies calling conventions, parameters, and return-value declarations.
class PrototypeProvider {
public:
    /// Releases the provider through its interface.
    virtual ~PrototypeProvider() = default;

    /// Returns the prototype associated with a function address.
    [[nodiscard]] virtual std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const = 0;
};

/// Supplies comments and strings required by high-level output.
class CommentProvider {
public:
    /// Releases the provider through its interface.
    virtual ~CommentProvider() = default;

    /// Returns the comment associated with an instruction or data address.
    [[nodiscard]] virtual std::optional<std::string> comment_at(std::uint64_t address) const = 0;
};

/// Describes an externally supplied local variable and its storage location.
struct VariableDescription {
    std::string name;
    std::string type_name;
    Storage storage;
};

/// Supplies source-level local names and types for a function.
class VariableProvider {
public:
    /// Releases the provider through its interface.
    virtual ~VariableProvider() = default;

    /// Returns variables associated with a function entry.
    [[nodiscard]] virtual std::vector<VariableDescription> variables_at(std::uint64_t address) const = 0;
};

/// Groups all external services used by one decompilation session.
struct ProviderContext {
    std::shared_ptr<PcodeProvider> pcode;
    std::shared_ptr<MemoryProvider> memory;
    std::shared_ptr<SymbolProvider> symbols;
    std::shared_ptr<TypeProvider> types;
    std::shared_ptr<PrototypeProvider> prototypes;
    std::shared_ptr<CommentProvider> comments;
    std::shared_ptr<VariableProvider> variables;
};

/// Stores one address-space description used to construct the engine model.
struct SpaceDescription {
    std::string name;
    std::uint32_t address_size = 8;
    std::uint32_t word_size = 1;
    bool big_endian = false;
    std::int32_t index = -1;
    std::int32_t delay = 0;
    bool physical = true;
};

/// Stores one named register location used by prototype and type providers.
struct RegisterDescription {
    std::string name;
    Storage location;
};

/// Supplies architecture facts independently of any machine-code decoder.
struct ArchitectureDescription {
    std::string name = "provider-architecture";
    std::vector<SpaceDescription> spaces;
    std::vector<RegisterDescription> registers;
    std::string code_space = "ram";
    std::string data_space = "ram";
    std::string stack_register = "RSP";
    std::string calling_convention = "default";
    std::uint32_t pointer_size = 8;
};

/// Supplies the function identity and bounded code range for one decompilation.
struct FunctionDescription {
    std::string name = "function";
    std::uint64_t entry = 0;
    std::uint64_t end = 0;
};

/// Reports the concrete artifacts produced by the native analysis pipeline.
struct DecompilationResult {
    std::vector<Instruction> raw_instructions;
    std::string raw_pcode;
    std::string high_pcode;
    std::string data_flow;
    std::string control_flow;
    std::string ast;
    std::string c_source;
};

/// Provides sparse immutable bytes for tests, loaders, and decoder adapters.
class SparseMemory final : public MemoryProvider {
public:
    /// Constructs an empty sparse image.
    SparseMemory() = default;

    /// Constructs an image mapped at `base`.
    SparseMemory(std::uint64_t base, std::vector<std::uint8_t> bytes) : base_(base), bytes_(std::move(bytes)) {}

    /// Reads a range and rejects overflow or unmapped bytes.
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, ProviderError> read(std::uint64_t address,
                                                                               std::size_t size) const override {
        if (address < base_ || address - base_ > bytes_.size() ||
            size > bytes_.size() - static_cast<std::size_t>(address - base_)) {
            return std::unexpected(ProviderError{"Requested memory range is outside SparseMemory"});
        }
        const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(address - base_);
        return std::vector<std::uint8_t>(begin, begin + static_cast<std::ptrdiff_t>(size));
    }

private:
    std::uint64_t base_ = 0;
    std::vector<std::uint8_t> bytes_;
};

/// Adapts the existing Sleigh runtime's public decoder to the provider contract.
class SleighPcodeProvider final : public PcodeProvider {
public:
    /// Loads one compiled SLA and retains the memory/context needed for decoding.
    SleighPcodeProvider(std::filesystem::path sla_path, std::shared_ptr<MemoryProvider> memory,
                        std::vector<std::pair<std::string, std::uint64_t>> context = {});

    /// Releases the opaque decoder state.
    ~SleighPcodeProvider();

    /// Prevents accidental copying of a stateful decoder.
    SleighPcodeProvider(const SleighPcodeProvider&) = delete;

    /// Prevents accidental copying of a stateful decoder.
    SleighPcodeProvider& operator=(const SleighPcodeProvider&) = delete;

    /// Transfers ownership of decoder state.
    SleighPcodeProvider(SleighPcodeProvider&&) noexcept;

    /// Transfers ownership of decoder state.
    SleighPcodeProvider& operator=(SleighPcodeProvider&&) noexcept;

    /// Decodes one instruction through NEW's Sleigh runtime.
    [[nodiscard]] std::expected<Instruction, ProviderError> decode(std::uint64_t address) const override;

private:
    class State;
    std::unique_ptr<State> state_;
};

/// Runs the original native flow, SSA, action, and C-printing pipeline.
class Decompiler final {
public:
    /// Constructs a decompiler over explicit provider contracts.
    Decompiler(ArchitectureDescription description, std::shared_ptr<PcodeProvider> provider,
               std::shared_ptr<MemoryProvider> memory);

    /// Constructs a decompiler over the complete external provider context.
    Decompiler(ArchitectureDescription description, ProviderContext context);

    /// Releases the opaque native architecture state.
    ~Decompiler();

    /// Prevents copying native ownership state.
    Decompiler(const Decompiler&) = delete;

    /// Prevents copying native ownership state.
    Decompiler& operator=(const Decompiler&) = delete;

    /// Transfers native ownership state.
    Decompiler(Decompiler&&) noexcept;

    /// Transfers native ownership state.
    Decompiler& operator=(Decompiler&&) noexcept;

    /// Runs bounded raw flow, native action analysis, and C output generation.
    [[nodiscard]] DecompilationResult decompile(const FunctionDescription& function) const;

private:
    class State;
    std::unique_ptr<State> state_;
};

} // namespace newghidra::decompiler
