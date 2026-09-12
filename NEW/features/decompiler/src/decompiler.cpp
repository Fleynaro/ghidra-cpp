// Provider frontend provenance: this adapter invokes the ported Ghidra engine
// under NEW's explicit provider boundary; analysis algorithms remain in the
// source files mapped by NEW/features/decompiler/CMakeLists.txt.

#include <mutex>

import decompiler;
import sleigh_runtime;

#include "architecture.hh"
#include "capability.hh"
#include "comment.hh"
#include "cpool.hh"
#include "database.hh"
#include "error.hh"
#include "funcdata.hh"
#include "globalcontext.hh"
#include "loadimage.hh"
#include "pcodeinject.hh"
#include "printc.hh"
#include "space.hh"
#include "translate.hh"
#include "type.hh"

namespace newghidra::decompiler {
namespace detail {

/// Adapts the provider contract to the native Ghidra Translate interface.
class ProviderTranslate final : public ghidra::Translate {
public:
    /// Builds address spaces and register mappings from provider metadata.
    ProviderTranslate(const ArchitectureDescription& description, std::shared_ptr<PcodeProvider> provider)
        : provider_(std::move(provider)) {
        if (!provider_) {
            throw std::invalid_argument("A p-code provider is required");
        }
        insertSpace(new ghidra::ConstantSpace(this, this));

        std::vector<SpaceDescription> descriptions = description.spaces;
        if (descriptions.empty()) {
            descriptions.push_back(SpaceDescription{"ram", 8, 1, false, 2, 0, true});
            descriptions.push_back(SpaceDescription{"register", 8, 1, false, 3, 0, true});
        }
        for (const SpaceDescription& space : descriptions) {
            if (space.name == "const" || space.name == "unique" || space.name.empty()) {
                continue;
            }
            const int index = space.index < 1 ? numSpaces() : space.index;
            insertSpace(new ghidra::AddrSpace(
                this, this, ghidra::IPTR_PROCESSOR, space.name, space.big_endian, space.address_size, space.word_size,
                index, space.physical ? ghidra::AddrSpace::hasphysical : 0, space.delay, space.delay));
        }
        insertSpace(new ghidra::UniqueSpace(this, this, numSpaces(), ghidra::AddrSpace::hasphysical));

        code_space_name_ = description.code_space;
        data_space_name_ = description.data_space;
        if (getSpaceByName(code_space_name_) == nullptr) {
            code_space_name_ = firstProcessorSpace();
        }
        if (getSpaceByName(data_space_name_) == nullptr) {
            data_space_name_ = code_space_name_;
        }
        setDefaultCodeSpace(getSpaceByName(code_space_name_)->getIndex());
        setDefaultDataSpace(getSpaceByName(data_space_name_)->getIndex());

        for (const RegisterDescription& register_description : description.registers) {
            ghidra::AddrSpace* space = getSpaceByName(register_description.location.space);
            if (space == nullptr) {
                throw std::invalid_argument("Register references an unknown address space: " +
                                            register_description.location.space);
            }
            registers_.emplace(register_description.name,
                               ghidra::VarnodeData{space, register_description.location.offset,
                                                   static_cast<ghidra::uint4>(register_description.location.size)});
        }
    }

    /// Provides the provider's no-op XML initialization hook.
    void initialize(ghidra::DocumentStorage&) override {}

    /// Resolves a named register location.
    const ghidra::VarnodeData& getRegister(const ghidra::string& name) const override {
        const auto iterator = registers_.find(name);
        if (iterator == registers_.end()) {
            throw ghidra::LowlevelError("Unknown provider register: " + name);
        }
        return iterator->second;
    }

    /// Returns the smallest named register containing a location.
    ghidra::string getRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset, ghidra::int4 size) const override {
        ghidra::string result;
        for (const auto& entry : registers_) {
            const ghidra::VarnodeData& location = entry.second;
            if (location.space == space && location.offset <= offset &&
                offset + static_cast<ghidra::uintb>(size) <= location.offset + location.size &&
                (result.empty() || location.size < getRegister(result).size)) {
                result = entry.first;
            }
        }
        return result;
    }

    /// Returns a named register with an exactly matching location.
    ghidra::string getExactRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset,
                                        ghidra::int4 size) const override {
        for (const auto& entry : registers_) {
            const ghidra::VarnodeData& location = entry.second;
            if (location.space == space && location.offset == offset && location.size == size) {
                return entry.first;
            }
        }
        return {};
    }

    /// Copies all named register mappings into the requested map.
    void getAllRegisters(std::map<ghidra::VarnodeData, ghidra::string>& result) const override {
        for (const auto& entry : registers_) {
            result.emplace(entry.second, entry.first);
        }
    }

    /// Returns no architecture-specific user operations.
    void getUserOpNames(std::vector<ghidra::string>& result) const override {
        result.clear();
    }

    /// Decodes only enough information to determine instruction length.
    ghidra::int4 instructionLength(const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        return static_cast<ghidra::int4>(result->length);
    }

    /// Emits provider p-code as native VarnodeData values.
    ghidra::int4 oneInstruction(ghidra::PcodeEmit& emit, const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        for (const PcodeOperation& operation : result->pcode) {
            std::vector<ghidra::VarnodeData> inputs;
            std::size_t input_index = 0;
            if (operation.memory_space.has_value() &&
                (operation.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_LOAD) ||
                 operation.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_STORE))) {
                ghidra::AddrSpace* memory_space = getSpaceByName(*operation.memory_space);
                if (memory_space == nullptr) {
                    throw ghidra::BadDataError("P-code references an unknown memory space: " +
                                               *operation.memory_space);
                }
                const ghidra::Address encoded_space = createConstFromSpace(memory_space);
                inputs.push_back(ghidra::VarnodeData{getConstantSpace(), encoded_space.getOffset(),
                                                     static_cast<ghidra::uint4>(sizeof(void*))});
                input_index = operation.inputs.empty() ? 0 : 1;
            }
            inputs.reserve(inputs.size() + operation.inputs.size());
            for (; input_index < operation.inputs.size(); ++input_index) {
                inputs.push_back(materialize(operation.inputs[input_index]));
            }
            ghidra::VarnodeData output{};
            ghidra::VarnodeData* output_pointer = nullptr;
            if (operation.output.has_value()) {
                output = materialize(*operation.output);
                output_pointer = &output;
            }
            emit.dump(address, static_cast<ghidra::OpCode>(operation.opcode), output_pointer,
                      inputs.empty() ? nullptr : inputs.data(), static_cast<ghidra::int4>(inputs.size()));
        }
        return static_cast<ghidra::int4>(result->length);
    }

    /// Emits provider assembly text through the native assembly callback.
    ghidra::int4 printAssembly(ghidra::AssemblyEmit& emit, const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        emit.dump(address, result->mnemonic, result->assembly);
        return static_cast<ghidra::int4>(result->length);
    }

private:
    /// Converts one public storage record into the native storage triple.
    ghidra::VarnodeData materialize(const Storage& storage) const {
        ghidra::AddrSpace* space = getSpaceByName(storage.space);
        if (space == nullptr) {
            throw ghidra::BadDataError("P-code references an unknown address space: " + storage.space);
        }
        return ghidra::VarnodeData{space, storage.offset, static_cast<ghidra::uint4>(storage.size)};
    }

    /// Finds the first processor space when a provider omits a default name.
    ghidra::string firstProcessorSpace() const {
        for (int index = 0; index < numSpaces(); ++index) {
            ghidra::AddrSpace* space = getSpace(index);
            if (space != nullptr && space->getType() == ghidra::IPTR_PROCESSOR) {
                return space->getName();
            }
        }
        throw std::invalid_argument("Architecture description has no processor address space");
    }

    std::shared_ptr<PcodeProvider> provider_;
    std::map<ghidra::string, ghidra::VarnodeData> registers_;
    ghidra::string code_space_name_;
    ghidra::string data_space_name_;
};

/// Supplies memory to the native LoadImage interface.
class ProviderLoadImage final : public ghidra::LoadImage {
public:
    /// Creates a load image backed by the caller's memory provider.
    explicit ProviderLoadImage(std::shared_ptr<MemoryProvider> memory)
        : ghidra::LoadImage("provider-memory"), memory_(std::move(memory)) {}

    /// Reads bytes from the provider and reports unavailable ranges as native errors.
    void loadFill(ghidra::uint1* destination, ghidra::int4 size, const ghidra::Address& address) override {
        if (destination == nullptr || size < 0) {
            throw ghidra::DataUnavailError("Invalid provider memory request");
        }
        if (!memory_) {
            throw ghidra::DataUnavailError("No memory provider is configured");
        }
        const auto result = memory_->read(address.getOffset(), static_cast<std::size_t>(size));
        if (!result) {
            throw ghidra::DataUnavailError(result.error().message);
        }
        std::copy(result->begin(), result->end(), destination);
    }

    /// Identifies the provider-backed image.
    ghidra::string getArchType() const override {
        return "provider";
    }

    /// Leaves provider addresses unchanged.
    void adjustVma(long) override {}

private:
    std::shared_ptr<MemoryProvider> memory_;
};

/// Implements the injection boundary when no architecture-specific injections are supplied.
class ProviderInjectLibrary final : public ghidra::PcodeInjectLibrary {
public:
    /// Constructs an empty injection library with the engine's temporary base.
    ProviderInjectLibrary(ghidra::Architecture* architecture, ghidra::uint4 temporary_base)
        : ghidra::PcodeInjectLibrary(architecture, temporary_base) {}

    /// Rejects dynamic injection allocation because the provider supplied none.
    ghidra::int4 allocateInject(const ghidra::string&, const ghidra::string&, ghidra::int4) override {
        throw ghidra::LowlevelError("Provider injection allocation is not configured");
    }

    /// Finalizes no payloads in the empty library.
    void registerInject(ghidra::int4) override {}

    /// Rejects manually supplied call-fixup text without a provider implementation.
    ghidra::int4 manualCallFixup(const ghidra::string&, const ghidra::string&) override {
        throw ghidra::LowlevelError("Provider call-fixup injection is not configured");
    }

    /// Rejects manually supplied call-other-fixup text without a provider implementation.
    ghidra::int4 manualCallOtherFixup(const ghidra::string&, const ghidra::string&, const std::vector<ghidra::string>&,
                                      const ghidra::string&) override {
        throw ghidra::LowlevelError("Provider call-other-fixup injection is not configured");
    }

    /// Returns the reusable empty injection context.
    ghidra::InjectContext& getCachedContext() override {
        return context_;
    }

    /// Returns the empty behavior table.
    const std::vector<ghidra::OpBehavior*>& getBehaviors() override {
        return behaviors_;
    }

private:
    /// Concrete empty context required by the native abstract injection API.
    class EmptyContext final : public ghidra::InjectContext {
    public:
        /// Encodes no state because this context never crosses a provider boundary.
        void encode(ghidra::Encoder&) const override {}
    };

    EmptyContext context_;
    std::vector<ghidra::OpBehavior*> behaviors_;
};

/// Owns the native Architecture subsystems configured from explicit providers.
class ProviderArchitecture final : public ghidra::Architecture {
public:
    /// Constructs the native engine and all in-memory provider-backed services.
    ProviderArchitecture(const ArchitectureDescription& description, std::shared_ptr<PcodeProvider> provider,
                         std::shared_ptr<MemoryProvider> memory)
        : description_(description), provider_(std::move(provider)), memory_(std::move(memory)) {
        initialize();
    }

    /// Returns the owned provider translator.
    ProviderTranslate* providerTranslate() const {
        return static_cast<ProviderTranslate*>(const_cast<ghidra::Translate*>(translate));
    }

    /// Prints warnings to the provider diagnostic stream.
    void printWarning(const ghidra::string& message) const override {
        warnings_ << message << '\n';
    }

private:
    /// Builds every native subsystem that does not require XML or Java state.
    void initialize() {
        ghidra::AttributeId::initialize();
        ghidra::ElementId::initialize();
        loader = new ProviderLoadImage(memory_);
        translate = new ProviderTranslate(description_, provider_);
        copySpaces(translate);
        insertSpace(new ghidra::OtherSpace(this, translate, ghidra::OtherSpace::INDEX));
        insertSpace(new ghidra::FspecSpace(this, translate, numSpaces()));
        insertSpace(new ghidra::IopSpace(this, translate, numSpaces()));
        insertSpace(new ghidra::JoinSpace(this, translate, numSpaces()));
        context = new ghidra::ContextInternal();
        types = new ghidra::TypeFactory(this);
        types->setupSizes();
        types->setCoreType("void", 1, ghidra::TYPE_VOID, false);
        types->setCoreType("bool", 1, ghidra::TYPE_BOOL, false);
        types->setCoreType("uint1", 1, ghidra::TYPE_UINT, false);
        types->setCoreType("uint2", 2, ghidra::TYPE_UINT, false);
        types->setCoreType("uint4", 4, ghidra::TYPE_UINT, false);
        types->setCoreType("uint8", 8, ghidra::TYPE_UINT, false);
        types->setCoreType("int1", 1, ghidra::TYPE_INT, false);
        types->setCoreType("int2", 2, ghidra::TYPE_INT, false);
        types->setCoreType("int4", 4, ghidra::TYPE_INT, false);
        types->setCoreType("int8", 8, ghidra::TYPE_INT, false);
        types->setCoreType("float4", 4, ghidra::TYPE_FLOAT, false);
        types->setCoreType("float8", 8, ghidra::TYPE_FLOAT, false);
        types->setCoreType("float10", 10, ghidra::TYPE_FLOAT, false);
        types->setCoreType("float16", 16, ghidra::TYPE_FLOAT, false);
        types->setCoreType("xunknown1", 1, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("xunknown2", 2, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("xunknown4", 4, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("xunknown8", 8, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("code", 1, ghidra::TYPE_CODE, false);
        types->setCoreType("char", 1, ghidra::TYPE_INT, true);
        types->setCoreType("wchar2", 2, ghidra::TYPE_INT, true);
        types->setCoreType("wchar4", 4, ghidra::TYPE_INT, true);
        types->cacheCoreTypes();
        commentdb = new ghidra::CommentDatabaseInternal();
        stringManager = new ghidra::StringManagerUnicode(this, 4096);
        cpool = new ghidra::ConstantPoolInternal();
        symboltab = new ghidra::Database(this, true);
        symboltab->attachScope(new ghidra::ScopeInternal(0, "", this), nullptr);
        symboltab->addRange(symboltab->getGlobalScope(), getDefaultDataSpace(), 0, getDefaultDataSpace()->getHighest());
        pcodeinjectlib = new ProviderInjectLibrary(this, translate->getUniqueStart(ghidra::Translate::INJECT));
        userops.initialize(this);
        ghidra::DocumentStorage specification;
        std::istringstream specification_text(
            "<compiler_spec>"
            "<global><range space=\"ram\"/></global>"
            "<stackpointer register=\"" +
            description_.stack_register +
            "\" space=\"ram\"/>"
            "<default_proto><prototype name=\"" +
            description_.calling_convention +
            "\" extrapop=\"0\">"
            "<input>"
            "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"RCX\"/></pentry>"
            "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"RDX\"/></pentry>"
            "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"R8\"/></pentry>"
            "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"R9\"/></pentry>"
            "</input>"
            "<output><pentry minsize=\"1\" maxsize=\"8\"><register name=\"RAX\"/></pentry></output>"
            "</prototype></default_proto>"
            "</compiler_spec>");
        ghidra::Document* document = specification.parseDocument(specification_text);
        specification.registerTag(document->getRoot());
        parseCompilerConfig(specification);
        if (defaultfp != nullptr) {
            defaultfp->setPrintInDecl(true);
        }
        const_cast<ghidra::Translate*>(translate)->setDefaultFloatFormats();
        ghidra::DocumentStorage empty;
        buildInstructions(empty);
        buildAction(empty);
        print->initializeFromArchitecture();
    }

    /// Provides the factory hook for the native Architecture base.
    ghidra::Translate* buildTranslator(ghidra::DocumentStorage&) override {
        return const_cast<ghidra::Translate*>(translate);
    }

    /// Provides the factory hook for the native Architecture base.
    void buildLoader(ghidra::DocumentStorage&) override {}

    /// Returns the already-created empty injection library.
    ghidra::PcodeInjectLibrary* buildPcodeInjectLibrary() override {
        return pcodeinjectlib;
    }

    /// Keeps the provider-created type factory.
    void buildTypegrp(ghidra::DocumentStorage&) override {}

    /// Keeps primitive type defaults supplied by TypeFactory.
    void buildCoreTypes(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory comment database.
    void buildCommentDB(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory string manager.
    void buildStringManager(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory constant pool.
    void buildConstantPool(ghidra::DocumentStorage&) override {}

    /// Keeps the empty context database.
    void buildContext(ghidra::DocumentStorage&) override {}

    /// Does not import external symbols; callers provide them through the contract.
    void buildSymbols(ghidra::DocumentStorage&) override {}

    /// Does not load processor XML; provider metadata is already materialized.
    void buildSpecFile(ghidra::DocumentStorage&) override {}

    /// Does not mutate provider address spaces after construction.
    void modifySpaces(ghidra::Translate*) override {}

    /// Resolves no architecture fields beyond the explicit description.
    void resolveArchitecture() override {
        archid = description_.name;
    }

    ArchitectureDescription description_;
    std::shared_ptr<PcodeProvider> provider_;
    std::shared_ptr<MemoryProvider> memory_;
    mutable std::ostringstream warnings_;
};

/// Converts one Sleigh runtime varnode into the provider value model.
static Storage convert_storage(const sleigh_runtime::Varnode& value) {
    return Storage{value.space, value.offset, value.size};
}

/// Converts one materialized Sleigh operation into the provider value model.
static PcodeOperation convert_operation(const sleigh_runtime::PcodeOp& value) {
    PcodeOperation result;
    result.opcode = std::to_underlying(value.opcode);
    if (value.output.has_value()) {
        result.output = convert_storage(*value.output);
    }
    for (const sleigh_runtime::Varnode& input : value.inputs) {
        result.inputs.push_back(convert_storage(input));
    }
    result.memory_space = value.memory_space;
    return result;
}

/// Resolves a provider type into the native type factory while retaining a
/// stable integer fallback for declarations not yet imported as structures.
static ghidra::Datatype* resolve_provider_type(const ProviderContext& context, ghidra::TypeFactory* types,
                                               const std::string& name,
                                               std::map<std::string, ghidra::Datatype*>& cache) {
    const auto cached = cache.find(name);
    if (cached != cache.end()) {
        return cached->second;
    }
    if (name == "void") {
        return types->getTypeVoid();
    }

    TypeDescription description;
    description.name = name;
    description.size = 8;
    description.kind = name == "wchar_t"
                           ? TypeKind::unicode_character
                           : (name.find("unsigned") != std::string::npos ? TypeKind::unsigned_integer
                                                                            : TypeKind::signed_integer);
    if (context.types) {
        if (const std::optional<TypeDescription> supplied = context.types->type_named(name)) {
            description = *supplied;
        }
    }

    if (description.kind == TypeKind::pointer) {
        ghidra::Datatype* pointed_to =
            resolve_provider_type(context, types, description.element_type, cache);
        ghidra::TypePointer* pointer = types->getTypePointer(
            static_cast<ghidra::int4>(description.size == 0 ? 8 : description.size), pointed_to, 1, name);
        cache.emplace(name, pointer);
        return pointer;
    }
    if (description.kind == TypeKind::array) {
        ghidra::Datatype* element =
            resolve_provider_type(context, types, description.element_type, cache);
        ghidra::TypeArray* array =
            types->getTypeArray(static_cast<ghidra::int4>(description.element_count), element);
        cache.emplace(name, array);
        return array;
    }
    if (description.kind == TypeKind::structure) {
        ghidra::TypeStruct* structure = types->getTypeStruct(name);
        cache.emplace(name, structure);
        std::vector<ghidra::TypeField> fields;
        ghidra::int4 field_id = 0;
        for (const TypeFieldDescription& field : description.fields) {
            fields.emplace_back(field_id++, static_cast<ghidra::int4>(field.offset), field.name,
                                resolve_provider_type(context, types, field.type_name, cache));
        }
        std::vector<ghidra::TypeBitField> bitfields;
        types->assignRawFields(structure, fields, bitfields);
        return structure;
    }

    if (description.kind == TypeKind::unicode_character) {
        ghidra::Datatype* result = types->getProviderUnicode(
            description.name, static_cast<ghidra::int4>(description.size == 0 ? 2 : description.size), ghidra::TYPE_INT);
        cache.emplace(name, result);
        return result;
    }
    const ghidra::type_metatype metatype =
        description.kind == TypeKind::unsigned_integer ? ghidra::TYPE_UINT
        : description.kind == TypeKind::boolean           ? ghidra::TYPE_BOOL
        : description.kind == TypeKind::floating_point    ? ghidra::TYPE_FLOAT
                                                          : ghidra::TYPE_INT;
    const ghidra::int4 size = static_cast<ghidra::int4>(description.size == 0 ? 1 : description.size);
    ghidra::Datatype* result = types->getBase(size, metatype, description.name);
    cache.emplace(name, result);
    return result;
}

} // namespace detail

/// Owns the provider and native architecture for a decompilation session.
class Decompiler::State {
public:
    /// Constructs one standalone native architecture.
    State(ArchitectureDescription description, ProviderContext context)
        : description(std::move(description)), context(std::move(context)), provider(this->context.pcode),
          memory(this->context.memory), architecture(std::make_unique<detail::ProviderArchitecture>(
                                            this->description, this->provider, this->memory)) {}

    ArchitectureDescription description;
    ProviderContext context;
    std::shared_ptr<PcodeProvider> provider;
    std::shared_ptr<MemoryProvider> memory;
    std::unique_ptr<detail::ProviderArchitecture> architecture;
};

/// Owns the stateful Sleigh decoder and its conversion context.
class SleighPcodeProvider::State {
public:
    /// Loads a compiled SLA and stores decoder inputs.
    State(std::filesystem::path path, std::shared_ptr<MemoryProvider> memory,
          std::vector<std::pair<std::string, std::uint64_t>> context)
        : decoder(std::move(path)), memory(std::move(memory)) {
        for (const auto& value : context) {
            processor_context.values.push_back({value.first, value.second});
        }
    }

    sleigh_runtime::Decoder decoder;
    std::shared_ptr<MemoryProvider> memory;
    sleigh_runtime::ProcessorContext processor_context;
};

/// Loads a compiled SLA and stores its provider-backed decoder state.
SleighPcodeProvider::SleighPcodeProvider(std::filesystem::path path, std::shared_ptr<MemoryProvider> memory,
                                         std::vector<std::pair<std::string, std::uint64_t>> context)
    : state_(std::make_unique<State>(std::move(path), std::move(memory), std::move(context))) {}

/// Releases the opaque decoder state.
SleighPcodeProvider::~SleighPcodeProvider() = default;

/// Transfers ownership of decoder state.
SleighPcodeProvider::SleighPcodeProvider(SleighPcodeProvider&&) noexcept = default;

/// Transfers ownership of decoder state.
SleighPcodeProvider& SleighPcodeProvider::operator=(SleighPcodeProvider&&) noexcept = default;

/// Decodes one instruction through the existing public Sleigh runtime.
std::expected<Instruction, ProviderError> SleighPcodeProvider::decode(std::uint64_t address) const {
    if (!state_->memory) {
        return std::unexpected(ProviderError{"SleighPcodeProvider requires a memory provider"});
    }
    const auto bytes = state_->memory->read(address, 16);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    const auto decoded = state_->decoder.decode(address, *bytes, state_->processor_context);
    if (!decoded) {
        return std::unexpected(ProviderError{decoded.error().message});
    }
    Instruction result;
    result.address = decoded->address;
    result.length = decoded->length;
    result.mnemonic = decoded->mnemonic;
    result.assembly = decoded->assembly;
    for (const sleigh_runtime::PcodeOp& operation : decoded->pcode) {
        result.pcode.push_back(detail::convert_operation(operation));
    }
    return result;
}

/// Constructs the provider-backed native decompiler.
Decompiler::Decompiler(ArchitectureDescription description, std::shared_ptr<PcodeProvider> provider,
                       std::shared_ptr<MemoryProvider> memory) {
    ghidra::forcePrintCLanguageRegistration();
    static std::once_flag capability_initialization;
    std::call_once(capability_initialization, [] { ghidra::CapabilityPoint::initializeAll(); });
    state_ = std::make_unique<State>(std::move(description),
                                     ProviderContext{std::move(provider), std::move(memory), {}, {}, {}, {}, {}});
}

/// Constructs the provider-backed native decompiler with all external services.
Decompiler::Decompiler(ArchitectureDescription description, ProviderContext context) {
    ghidra::forcePrintCLanguageRegistration();
    static std::once_flag capability_initialization;
    std::call_once(capability_initialization, [] { ghidra::CapabilityPoint::initializeAll(); });
    if (!context.pcode) {
        throw std::invalid_argument("A p-code provider is required");
    }
    state_ = std::make_unique<State>(std::move(description), std::move(context));
}

/// Releases the opaque native architecture state.
Decompiler::~Decompiler() = default;

/// Transfers native ownership state.
Decompiler::Decompiler(Decompiler&&) noexcept = default;

/// Transfers native ownership state.
Decompiler& Decompiler::operator=(Decompiler&&) noexcept = default;

/// Runs the original Funcdata pipeline and captures its diagnostic representations.
DecompilationResult Decompiler::decompile(const FunctionDescription& function) const {
    if (function.end <= function.entry) {
        throw std::invalid_argument("FunctionDescription requires an end address greater than entry");
    }
    DecompilationResult result;
    for (std::uint64_t address = function.entry; address < function.end;) {
        const auto decoded = state_->provider->decode(address);
        if (!decoded) {
            throw std::runtime_error(decoded.error().message);
        }
        if (decoded->length == 0 || address + decoded->length > function.end) {
            throw std::runtime_error("Provider returned an instruction outside the function range");
        }
        result.raw_instructions.push_back(*decoded);
        address += decoded->length;
    }

    ghidra::AddrSpace* code_space = state_->architecture->getDefaultCodeSpace();
    const ghidra::Address entry(code_space, function.entry);
    std::string function_name = function.name;
    if (state_->context.symbols) {
        const std::optional<SymbolDescription> symbol = state_->context.symbols->symbol_at(function.entry);
        if (symbol && !symbol->name.empty()) {
            function_name = symbol->name;
        }
    }
    auto install_external_function = [&](std::uint64_t address, const std::string& fallback_name) {
        ghidra::AddrSpace* external_space = state_->architecture->getDefaultCodeSpace();
        const ghidra::Address external_address(external_space, address);
        std::string external_name = fallback_name;
        if (state_->context.symbols) {
            const std::optional<SymbolDescription> symbol = state_->context.symbols->symbol_at(address);
            if (symbol && !symbol->name.empty()) {
                external_name = symbol->name;
            }
        }
        ghidra::FunctionSymbol* external_symbol =
            state_->architecture->symboltab->getGlobalScope()->addFunction(external_address, external_name);
        ghidra::Funcdata* external_data = external_symbol->getFunction();
        if (!state_->context.prototypes) {
            return;
        }
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(address);
        if (!prototype) {
            return;
        }
        std::map<std::string, ghidra::Datatype*> type_cache;
        ghidra::PrototypePieces pieces{};
        pieces.model = state_->architecture->defaultfp;
        pieces.name = external_name;
        pieces.outtype = detail::resolve_provider_type(state_->context, state_->architecture->types,
                                                       prototype->return_type, type_cache);
        pieces.firstVarArgSlot = -1;
        for (const PrototypeParameterDescription& parameter : prototype->parameters) {
            pieces.innames.push_back(parameter.name);
            pieces.intypes.push_back(detail::resolve_provider_type(state_->context,
                                                                   state_->architecture->types,
                                                                   parameter.type_name, type_cache));
        }
        external_data->getFuncProto().setCustomStorage(true);
        external_data->getFuncProto().setPieces(pieces);
        for (std::size_t index = 0; index < prototype->parameters.size(); ++index) {
            if (!prototype->parameters[index].storage) {
                continue;
            }
            ghidra::AddrSpace* parameter_space =
                state_->architecture->getSpaceByName(prototype->parameters[index].storage->space);
            if (parameter_space == nullptr) {
                throw std::runtime_error("Child prototype references an unknown storage space: " +
                                         prototype->parameters[index].storage->space);
            }
            ghidra::ParameterPieces parameter_pieces{};
            parameter_pieces.addr = ghidra::Address(parameter_space,
                                                    prototype->parameters[index].storage->offset);
            parameter_pieces.type = pieces.intypes[index];
            parameter_pieces.flags = ghidra::ParameterPieces::typelock |
                                     ghidra::ParameterPieces::namelock |
                                     ghidra::ParameterPieces::sizelock;
            external_data->getFuncProto().setParam(static_cast<ghidra::int4>(index),
                                                   prototype->parameters[index].name, parameter_pieces);
        }
        if (prototype->return_storage) {
            ghidra::AddrSpace* output_space =
                state_->architecture->getSpaceByName(prototype->return_storage->space);
            if (output_space == nullptr) {
                throw std::runtime_error("Child prototype references an unknown return space: " +
                                         prototype->return_storage->space);
            }
            ghidra::ParameterPieces output_pieces{};
            output_pieces.addr = ghidra::Address(output_space, prototype->return_storage->offset);
            output_pieces.type = pieces.outtype;
            output_pieces.flags = ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::sizelock;
            external_data->getFuncProto().setOutput(output_pieces);
        }
        external_data->getFuncProto().clearProviderErrors();
    };
    for (const Instruction& instruction : result.raw_instructions) {
        for (const PcodeOperation& operation : instruction.pcode) {
            if (operation.opcode != static_cast<std::uint32_t>(ghidra::CPUI_CALL) || operation.inputs.empty()) {
                continue;
            }
            const std::uint64_t target = operation.inputs.front().offset;
            if (target == function.entry) {
                continue;
            }
            install_external_function(target, "FUN_" + std::to_string(target));
        }
    }
    ghidra::FunctionSymbol* symbol =
        state_->architecture->symboltab->getGlobalScope()->addFunction(entry, function_name);
    ghidra::Funcdata* data = symbol->getFunction();

    if (state_->context.prototypes) {
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(function.entry);
        if (prototype) {
            std::map<std::string, ghidra::Datatype*> type_cache;
            ghidra::PrototypePieces pieces{};
            pieces.model = state_->architecture->defaultfp;
            pieces.name = function_name;
            pieces.outtype = detail::resolve_provider_type(state_->context, state_->architecture->types,
                                                           prototype->return_type, type_cache);
            pieces.firstVarArgSlot = -1;
            for (const auto& parameter : prototype->parameters) {
                pieces.innames.push_back(parameter.name);
                pieces.intypes.push_back(detail::resolve_provider_type(state_->context,
                                                                       state_->architecture->types,
                                                                       parameter.type_name, type_cache));
            }
            data->getFuncProto().setCustomStorage(true);
            data->getFuncProto().setPieces(pieces);
            for (std::size_t index = 0; index < prototype->parameters.size(); ++index) {
                const PrototypeParameterDescription& parameter = prototype->parameters[index];
                if (!parameter.storage) {
                    continue;
                }
                ghidra::AddrSpace* parameter_space = state_->architecture->getSpaceByName(parameter.storage->space);
                if (parameter_space == nullptr) {
                    throw std::runtime_error("Prototype parameter references an unknown storage space: " +
                                             parameter.storage->space);
                }
                ghidra::ParameterPieces parameter_pieces{};
                parameter_pieces.addr = ghidra::Address(parameter_space, parameter.storage->offset);
                parameter_pieces.type = pieces.intypes[index];
                parameter_pieces.flags = ghidra::ParameterPieces::typelock |
                                         ghidra::ParameterPieces::namelock |
                                         ghidra::ParameterPieces::sizelock;
                data->getFuncProto().setParam(static_cast<ghidra::int4>(index), parameter.name,
                                              parameter_pieces);
            }
            if (prototype->return_storage) {
                ghidra::AddrSpace* output_space =
                    state_->architecture->getSpaceByName(prototype->return_storage->space);
                if (output_space == nullptr) {
                    throw std::runtime_error("Prototype return references an unknown storage space: " +
                                             prototype->return_storage->space);
                }
                ghidra::ParameterPieces output_pieces{};
                output_pieces.addr = ghidra::Address(output_space, prototype->return_storage->offset);
                output_pieces.type = pieces.outtype;
                output_pieces.flags = ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::sizelock;
                data->getFuncProto().setOutput(output_pieces);
            }
            data->getFuncProto().clearProviderErrors();
        }
    }
    if (state_->context.comments) {
        for (const Instruction& instruction : result.raw_instructions) {
            const std::optional<std::string> comment = state_->context.comments->comment_at(instruction.address);
            if (comment) {
                state_->architecture->commentdb->addComment(ghidra::Comment::user1, entry,
                                                            ghidra::Address(code_space, instruction.address), *comment);
            }
        }
    }
    data->followFlow(entry, ghidra::Address(code_space, function.end));

    if (state_->context.variables) {
        std::map<std::string, ghidra::Datatype*> type_cache;
        for (const VariableDescription& variable : state_->context.variables->variables_at(function.entry)) {
            ghidra::AddrSpace* variable_space = state_->architecture->getSpaceByName(variable.storage.space);
            if (variable_space == nullptr) {
                throw std::runtime_error("Variable references an unknown storage space: " + variable.storage.space);
            }
            ghidra::Address variable_address(variable_space, variable.storage.offset);
            ghidra::Datatype* variable_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, variable.type_name, type_cache);
            data->getScopeLocal()->addTypeRecommendation(variable_address, variable_type);
            data->getScopeLocal()->addNameRecommendation(variable_address, entry,
                                                          variable_type->getSize(), variable.name);
        }
        data->getScopeLocal()->applyTypeRecommendations();
    }

    std::ostringstream raw;
    data->printRaw(raw);
    result.raw_pcode = raw.str();

    ghidra::Action* action = state_->architecture->allacts.getCurrent();
    if (action != nullptr) {
        action->reset(*data);
        action->perform(*data);
    }
    if (state_->context.variables) {
        data->getScopeLocal()->applyTypeRecommendations();
        data->getScopeLocal()->recoverNameRecommendationsForSymbols();
        std::map<std::string, ghidra::Datatype*> type_cache;
        for (const VariableDescription& variable : state_->context.variables->variables_at(function.entry)) {
            ghidra::AddrSpace* variable_space = state_->architecture->getSpaceByName(variable.storage.space);
            if (variable_space == nullptr) {
                continue;
            }
            ghidra::Datatype* variable_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, variable.type_name, type_cache);
            if (ghidra::MapEntry* map = data->getScopeLocal()->findOverlap(
                    ghidra::Address(variable_space, variable.storage.offset), variable_type->getSize())) {
                map->getSymbol()->setProviderInfo(variable.name, variable_type);
            }
        }
    }
    if (state_->context.prototypes) {
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(function.entry);
        if (prototype) {
            std::map<std::string, ghidra::Datatype*> type_cache;
            ghidra::Datatype* return_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, prototype->return_type, type_cache);
            if (prototype->return_storage && data->getFuncProto().getOutput() != nullptr) {
                data->getFuncProto().getOutput()->overrideSizeLockType(return_type);
            }
        }
    }

    std::ostringstream high;
    data->printVarnodeTree(high);
    result.high_pcode = high.str();
    result.data_flow = result.high_pcode;
    std::ostringstream flow;
    data->printBlockTree(flow);
    result.control_flow = flow.str();
    std::ostringstream ast;
    data->printLocalRange(ast);
    result.ast = ast.str();
    std::ostringstream c_output;
    state_->architecture->print->setOutputStream(&c_output);
    state_->architecture->print->setFlat(false);
    state_->architecture->print->docFunction(data);
    result.c_source = c_output.str();
    return result;
}

} // namespace newghidra::decompiler
