export module decompiler;

import std;
export import ghidra.core;

export namespace newghidra::decompiler {

/// Re-exports the canonical storage value used by p-code and ABI boundaries.
using Storage = ghidra::core::StorageLocation;

/// Re-exports the canonical p-code operation used by Sleigh and decompiler providers.
using PcodeOperation = ghidra::core::PcodeOp;

/// Re-exports the canonical opcode enumeration used by provider operations.
using PcodeOpcode = ghidra::core::PcodeOpcode;

/// Re-exports the canonical decoder snapshot shared by Sleigh and the native frontend.
using Instruction = ghidra::core::DecodedInstruction;

/// Re-exports the canonical decoder failure value.
using ProviderError = ghidra::core::DecodeError;

/// Selects the integer representation used by the native C printer.
///
/// The values mirror the original decompiler's `Datatype::encodeIntegerFormat`
/// contract: zero means no forced representation and the remaining values
/// select hexadecimal, decimal, octal, binary, or character output.
enum class DisplayFormat {
    none,
    hexadecimal,
    decimal,
    octal,
    binary,
    character,
};

/// Selects the native NaN simplification policy for one provider session.
/// Original option source: `Ghidra/Features/Decompiler/src/decompile/cpp/options.cc`,
/// `OptionNanIgnore`, and the `<nanignore>` option in compiler specifications.
enum class NanHandling {
    native_default,
    none,
    compare,
    all,
};

/// Describes analysis options that were previously supplied by XML command
/// streams. The provider applies these before native action construction so
/// they affect the same flow, heritage, and transformation passes as the
/// original `option` commands.
struct AnalysisOptions {
    /// Propagates mapped read-only memory values into integer or floating constants.
    bool readonly_propagate = false;
    /// Forces the default integer representation in the native C printer.
    DisplayFormat integer_format = DisplayFormat::none;
    /// Selects the native NaN simplification policy, or preserves its default.
    NanHandling nan_handling = NanHandling::native_default;
};

/// Describes one dynamic constant display conversion from the provider.
/// Original command source: `Ghidra/Features/Decompiler/src/decompile/cpp/ifacedecomp.cc`,
/// `IfcMapconvert::execute` and `IfcForceFormat::execute`.
struct ConstantFormatDescription {
    /// Address of the p-code operation whose dynamic hash identifies the use.
    std::uint64_t pcode_address = 0;
    /// Constant value selected by the original varnode command.
    std::uint64_t value = 0;
    /// Dynamic hash associated with the selected p-code use.
    std::uint64_t hash = 0;
    /// Integer representation forced for this use.
    DisplayFormat format = DisplayFormat::none;
    /// Optional encoded-constant width used to disambiguate same-value uses.
    std::uint32_t size = 0;
};

/// Provides decoded instructions to the decompiler's Translate boundary.
class PcodeProvider {
public:
    /// Releases the provider through its interface.
    virtual ~PcodeProvider() = default;

    /// Decodes the instruction beginning at `address`.
    [[nodiscard]] virtual std::expected<Instruction, ProviderError> decode(std::uint64_t address) const = 0;
};

/// Describes a contiguous memory range with provider-defined access semantics.
struct MemoryRangeDescription {
    std::string space = "ram";
    std::uint64_t first = 0;
    std::uint64_t size = 0;
};

/// Supplies bytes and immutable memory metadata to the native LoadImage boundary.
class MemoryProvider {
public:
    /// Releases the provider through its interface.
    virtual ~MemoryProvider() = default;

    /// Reads exactly `size` bytes beginning at `address`.
    [[nodiscard]] virtual std::expected<std::vector<std::uint8_t>, ProviderError> read(std::uint64_t address,
                                                                                       std::size_t size) const = 0;

    /// Reads exactly `size` bytes in `space` beginning at `address`.
    ///
    /// The default forwards to the original offset-only contract so existing
    /// providers remain source-compatible and retain their existing behavior
    /// while multi-space providers can distinguish otherwise-identical offsets.
    [[nodiscard]] virtual std::expected<std::vector<std::uint8_t>, ProviderError>
    read(std::string_view space, std::uint64_t address, std::size_t size) const {
        static_cast<void>(space);
        return read(address, size);
    }

    /// Returns memory ranges whose accesses must retain volatile side effects.
    /// The default is an empty set so existing immutable providers remain valid.
    [[nodiscard]] virtual std::vector<MemoryRangeDescription> volatile_ranges() const {
        return {};
    }
};

/// Classifies a provider symbol so data objects are not mistaken for functions.
enum class SymbolKind {
    function,
    data,
};

/// Describes one externally supplied symbol, function name, or data object.
struct SymbolDescription {
    std::uint64_t address = 0;
    std::string name;
    std::string namespace_name;
    SymbolKind kind = SymbolKind::function;
    std::uint32_t size = 0;
    std::string type_name;
    bool read_only = false;
    /// Forces the integer representation for this mapped data symbol.
    DisplayFormat display_format = DisplayFormat::none;
    /// Names the provider address space containing this symbol. An empty value
    /// selects the architecture data space for compatibility with older
    /// providers.
    std::string space;
    /// Stable database identity for this symbol. Equal non-zero identities
    /// share one native Symbol and may have multiple mapped addresses.
    std::uint64_t identity = 0;
    /// Optional identity of an earlier symbol to which this mapping is an
    /// alias. This permits aliases to use different provider names while
    /// retaining one native variable identity.
    std::uint64_t alias_identity = 0;
};

/// Supplies symbols independently of the loader or database implementation.
class SymbolProvider {
public:
    /// Releases the provider through its interface.
    virtual ~SymbolProvider() = default;

    /// Returns a symbol at `address`, if one is known.
    [[nodiscard]] virtual std::optional<SymbolDescription> symbol_at(std::uint64_t address) const = 0;

    /// Returns all symbols that must be installed before flow analysis starts.
    /// Providers that only support address lookup may retain the default empty result.
    [[nodiscard]] virtual std::vector<SymbolDescription> symbols() const {
        return {};
    }
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
    enumeration,
};

/// Describes one field in an externally supplied structure or union.
struct TypeFieldDescription {
    std::string name;
    std::string type_name;
    std::uint32_t offset = 0;
};

/// Describes one named value in an externally supplied enumeration.
struct TypeEnumValueDescription {
    std::string name;
    std::int64_t value = 0;
};

/// Describes one contiguous bitfield group member in an externally supplied structure.
/// Members with the same group are packed into one native storage unit in declaration order.
struct TypeBitFieldDescription {
    std::string name;
    std::string type_name;
    std::uint32_t bit_count = 0;
    std::uint32_t group = 0;
};

/// Describes an externally supplied type declaration.
struct TypeDescription {
    std::string name;
    std::uint32_t size = 0;
    /// Optional source declaration retained by the frontend for emitted type
    /// declarations. The native type name remains the provider identifier.
    std::string declaration;
    TypeKind kind = TypeKind::signed_integer;
    bool signed_value = true;
    std::string element_type;
    std::uint32_t element_count = 0;
    std::vector<TypeFieldDescription> fields;
    std::vector<TypeBitFieldDescription> bitfields;
    std::vector<TypeEnumValueDescription> enum_values;
    /// Forces the integer representation for values of this type.
    DisplayFormat display_format = DisplayFormat::none;
    /// Names the containing type for a database relative pointer. Empty means
    /// this is an ordinary pointer and `relative_offset` is ignored.
    std::string relative_parent_type;
    /// Byte offset of the pointed-to value within `relative_parent_type`.
    std::int32_t relative_offset = 0;
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
    /// Optional ordered storage pieces for an ABI-split parameter. Pieces are
    /// listed most-significant first, matching `JoinRecord` and
    /// `ParameterPieces::assignAddressFromPieces` in the native decompiler.
    /// An empty vector preserves the single-location `storage` contract.
    std::vector<Storage> storage_pieces;
};

/// Describes a function prototype independently of compiler-spec XML.
struct PrototypeDescription {
    std::string calling_convention = "default";
    std::string return_type = "void";
    std::optional<Storage> return_storage;
    /// Optional ordered storage pieces for an ABI-split return value. Pieces
    /// are listed most-significant first; an empty vector preserves the
    /// single-location `return_storage` contract. The provider must not set
    /// both fields for the same return value.
    std::vector<Storage> return_storage_pieces;
    std::vector<PrototypeParameterDescription> parameters;
    /// Treat calls to this function as non-returning.
    bool no_return = false;
    /// Inline the full function body when no call-fixup is supplied.
    bool inline_function = false;
    /// Associates a provider-registered call-fixup with this function.
    std::optional<std::string> call_fixup;
    /// Supplies the ABI register or memory location for a hidden structure return pointer.
    std::optional<Storage> hidden_return_storage;
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
    /// Stable database identity used when recovered pieces are revisited after
    /// partial merge/split analysis.
    std::uint64_t identity = 0;
    /// Marks an explicitly isolated local whose storage must not be
    /// speculatively merged with a different provider identity.
    bool isolated = false;
};

/// Supplies source-level local names and types for a function.
class VariableProvider {
public:
    /// Releases the provider through its interface.
    virtual ~VariableProvider() = default;

    /// Returns variables associated with a function entry.
    [[nodiscard]] virtual std::vector<VariableDescription> variables_at(std::uint64_t address) const = 0;
};

class FunctionProvider;

/// Describes a recovered direct destination for an indirect call.
struct IndirectCallTargetDescription {
    std::uint64_t call_address = 0;
    std::uint64_t target_address = 0;
};

/// Describes a manually supplied jump-table model.
struct JumpTableDescription {
    std::uint64_t branch_address = 0;
    std::vector<std::uint64_t> target_addresses;
    std::optional<std::uint64_t> normalized_switch_address;
    std::uint64_t normalized_switch_hash = 0;
    std::uint64_t starting_value = 0;
};

/// Describes a raw p-code flow override at one instruction address.
struct FlowOverrideDescription {
    std::uint64_t address = 0;
    std::string type;
};

/// Describes a destination replacement for a call, branch, or callother override.
struct DestinationOverrideDescription {
    std::uint64_t address = 0;
    std::uint64_t target_address = 0;
    std::string type;
};

/// Describes a forced branch destination used after raw flow is emitted.
struct ForceGotoDescription {
    std::uint64_t address = 0;
    std::uint64_t target_address = 0;
};

/// Groups all flow corrections belonging to one function body.
struct FlowDescription {
    std::vector<IndirectCallTargetDescription> indirect_call_targets;
    std::vector<JumpTableDescription> jump_tables;
    std::vector<FlowOverrideDescription> flow_overrides;
    std::vector<DestinationOverrideDescription> destination_overrides;
    std::vector<ForceGotoDescription> force_gotos;
};

/// Supplies function-local flow corrections before native analysis begins.
class FlowProvider {
public:
    /// Releases the provider through its interface.
    virtual ~FlowProvider() = default;

    /// Returns flow corrections for a function entry, if any are known.
    [[nodiscard]] virtual std::optional<FlowDescription> flow_at(std::uint64_t address) const = 0;
};

/// Classifies a varnode used by a provider-owned injection payload.
enum class InjectionVarnodeKind {
    storage,
    input,
    output,
};

/// Describes one concrete or context-relative varnode in an injection.
/// Call-fixup contexts normally expose concrete register or memory storage, while
/// input/output kinds are reserved for injection contexts that publish lists.
struct InjectionVarnode {
    InjectionVarnodeKind kind = InjectionVarnodeKind::storage;
    Storage storage;
    std::size_t index = 0;
};

/// Describes one native p-code operation emitted by an injection payload.
struct InjectionOperation {
    std::uint32_t opcode = 0;
    std::optional<InjectionVarnode> output;
    std::vector<InjectionVarnode> inputs;
};

/// Describes a call-fixup that is already expressed as validated p-code records.
struct CallFixupDescription {
    std::string name;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<InjectionOperation> operations;
    std::int32_t parameter_shift = 0;
    bool incidental_copy = false;
};

/// Describes a callother-fixup using the same provider p-code representation.
struct CallOtherFixupDescription {
    std::string name;
    std::string output_name;
    std::vector<std::string> input_names;
    std::vector<InjectionOperation> operations;
    /// Identifies the CALLOTHER constant used by the provider p-code stream.
    /// The native user-op manager uses this index to attach the registered
    /// payload before flow generation starts.
    std::uint32_t userop_index = 0;
};

/// Supplies declarative call-fixup and callother-fixup payloads.
class InjectionProvider {
public:
    /// Releases the provider through its interface.
    virtual ~InjectionProvider() = default;

    /// Returns call-fixups to register before prototypes are applied.
    [[nodiscard]] virtual std::vector<CallFixupDescription> call_fixups() const = 0;

    /// Returns callother-fixups to register before user operations are analyzed.
    [[nodiscard]] virtual std::vector<CallOtherFixupDescription> call_other_fixups() const {
        return {};
    }
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
    std::shared_ptr<FlowProvider> flow;
    std::shared_ptr<FunctionProvider> functions;
    std::shared_ptr<InjectionProvider> injections;
    /// Replaces XML `option` commands for this decompilation session.
    AnalysisOptions analysis_options;
    /// Replaces XML dynamic constant-format commands for this session.
    std::vector<ConstantFormatDescription> constant_formats;
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
    /// If non-empty, construct this processor space as an overlay of the
    /// named provider space. The base must be declared in the same context.
    std::string overlay_base;
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

/// Provides the shared x86-64 provider description used by analyzer integrations targeting x86-64.sla.
[[nodiscard]] inline ArchitectureDescription make_x86_64_architecture() {
    ArchitectureDescription result;
    result.name = "x86-64 analysis";
    result.spaces = {{"const", 8, 1, false, 0, 0, true},
                     {"ram", 8, 1, false, 2, 0, true},
                     {"register", 8, 1, false, 3, 0, true},
                     {"unique", 8, 1, false, 4, 0, true}};
    const auto registers = std::array{std::pair{std::string_view{"RAX"}, std::uint64_t{0x00}},
                                      std::pair{std::string_view{"RCX"}, std::uint64_t{0x08}},
                                      std::pair{std::string_view{"RDX"}, std::uint64_t{0x10}},
                                      std::pair{std::string_view{"RBX"}, std::uint64_t{0x18}},
                                      std::pair{std::string_view{"RSP"}, std::uint64_t{0x20}},
                                      std::pair{std::string_view{"RBP"}, std::uint64_t{0x28}},
                                      std::pair{std::string_view{"RSI"}, std::uint64_t{0x30}},
                                      std::pair{std::string_view{"RDI"}, std::uint64_t{0x38}},
                                      std::pair{std::string_view{"R8"}, std::uint64_t{0x40}},
                                      std::pair{std::string_view{"R9"}, std::uint64_t{0x48}},
                                      std::pair{std::string_view{"R10"}, std::uint64_t{0x50}},
                                      std::pair{std::string_view{"R11"}, std::uint64_t{0x58}},
                                      std::pair{std::string_view{"R12"}, std::uint64_t{0x60}},
                                      std::pair{std::string_view{"R13"}, std::uint64_t{0x68}},
                                      std::pair{std::string_view{"R14"}, std::uint64_t{0x70}},
                                      std::pair{std::string_view{"R15"}, std::uint64_t{0x78}}};
    for (const auto [name, offset] : registers)
        result.registers.push_back({std::string(name), {"register", offset, 8}});
    result.code_space = "ram";
    result.data_space = "ram";
    result.stack_register = "RSP";
    result.pointer_size = 8;
    return result;
}

/// Bundles architecture facts with the processor-context values required by a
/// compiled Sleigh specification. The context owns no decoder, memory image,
/// or installed-file path, so the same value can be used with copied fixtures,
/// environment-resolved artifacts, or a different provider implementation.
struct ArchitectureProviderContext {
    ArchitectureDescription architecture;
    std::vector<std::pair<std::string, std::uint64_t>> processor_context;
};

/// Supplies the function identity and bounded code range for one decompilation.
struct FunctionDescription {
    std::string name = "function";
    std::uint64_t entry = 0;
    std::uint64_t end = 0;
};

/// Captures one native Varnode identity used by high-level Clang markup.
///
/// The `create_index` is the same identity emitted as the original Ghidra
/// `ATTRIB_VARREF` by `EmitMarkup::tagVariable`, `tagField`, and related
/// methods.  The snapshot is value-owned so callers can inspect provenance
/// after the native Funcdata graph remains private to the Decompiler.
struct VarnodeProvenance {
    std::uint32_t create_index = 0;
    std::string space;
    std::uint64_t offset = 0;
    std::uint32_t size = 0;
    std::optional<std::uint64_t> defining_op;
    std::string high_variable_name;
};

/// Captures one analyzed native PcodeOp and its Varnode edges.
///
/// `sequence` is the native `SeqNum::uniq` value emitted as `ATTRIB_OPREF` by
/// the original Ghidra markup emitter.  `address` is the originating machine
/// instruction address stored in the same native PcodeOp sequence number.
struct PcodeOpProvenance {
    std::uint64_t sequence = 0;
    std::string opcode;
    std::uint32_t opcode_value = 0;
    std::string address_space;
    std::uint64_t address = 0;
    std::optional<std::uint32_t> output_varnode;
    std::vector<std::uint32_t> input_varnodes;
};

/// Supplies additional function bodies that may be called or inlined by a root function.
class FunctionProvider {
public:
    /// Releases the provider through its interface.
    virtual ~FunctionProvider() = default;

    /// Returns bounded bodies that must be available to the native flow engine.
    [[nodiscard]] virtual std::vector<FunctionDescription> functions() const = 0;
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
    /// XML Clang markup emitted by the native `EmitMarkup` path.
    std::string clang_markup;
    /// Post-analysis native PcodeOps keyed by the markup `opref` sequence.
    std::vector<PcodeOpProvenance> pcode_provenance;
    /// Native Varnodes keyed by the markup `varref` creation index.
    std::vector<VarnodeProvenance> varnode_provenance;
};

/// Provides sparse immutable bytes for tests, loaders, and decoder adapters.
class SparseMemory final : public MemoryProvider {
public:
    /// Constructs an empty sparse image.
    SparseMemory() = default;

    /// Constructs an image mapped at `base`.
    SparseMemory(std::uint64_t base, std::vector<std::uint8_t> bytes,
                 std::vector<MemoryRangeDescription> volatile_ranges = {})
        : base_(base), bytes_(std::move(bytes)), volatile_ranges_(std::move(volatile_ranges)) {}

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

    /// Returns the volatile ranges supplied when this image was constructed.
    [[nodiscard]] std::vector<MemoryRangeDescription> volatile_ranges() const override {
        return volatile_ranges_;
    }

private:
    std::uint64_t base_ = 0;
    std::vector<std::uint8_t> bytes_;
    std::vector<MemoryRangeDescription> volatile_ranges_;
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
