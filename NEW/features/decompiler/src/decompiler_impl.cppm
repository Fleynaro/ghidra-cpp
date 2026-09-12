module decompiler;
import std;
import ghidra.decompiler;
import sleigh_runtime;

// Provider frontend provenance: this adapter invokes the ported Ghidra engine
// under NEW's explicit provider boundary; analysis algorithms remain in the
// source files mapped by NEW/features/decompiler/CMakeLists.txt.

namespace newghidra::decompiler {
namespace detail {

/// Checks one provider storage record before it is converted to native
/// VarnodeData. Non-empty spaces and non-zero, non-wrapping ranges are
/// required because the native engine cannot represent malformed varnodes.
static void validate_storage(const Storage& storage, std::string_view context) {
    if (storage.space.empty() || storage.size == 0) {
        throw ghidra::BadDataError(std::string(context) + " has an empty space or zero size");
    }
    if (storage.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
        throw ghidra::BadDataError(std::string(context) + " exceeds the native varnode size limit");
    }
    // Constant-space offsets are values, not byte addresses, and may use the
    // full uint64 range without making the represented varnode wrap.
    if (storage.space != "const" && storage.offset > std::numeric_limits<std::uint64_t>::max() - (storage.size - 1U)) {
        throw ghidra::BadDataError(std::string(context) + " address range overflows");
    }
}

/// Validates one complete provider instruction at the frontend boundary.
/// Opcode values are checked before the native enum cast, while every storage
/// range and the instruction end address are checked for representability.
/// This is the provider-side equivalent of the bounds assumptions in
/// `Ghidra/Features/Decompiler/src/decompile/cpp/translate.cc`.
static void validate_instruction(const Instruction& instruction, std::uint64_t requested_address) {
    if (instruction.address != requested_address) {
        throw ghidra::BadDataError("Provider returned an instruction at a different address");
    }
    if (instruction.length == 0 ||
        instruction.length > static_cast<std::size_t>(std::numeric_limits<ghidra::int4>::max()) ||
        requested_address > std::numeric_limits<std::uint64_t>::max() - instruction.length) {
        throw ghidra::BadDataError("Provider returned an invalid instruction length or overflowing address");
    }
    for (const PcodeOperation& operation : instruction.pcode) {
        if (operation.opcode == 0 || operation.opcode >= static_cast<std::uint32_t>(ghidra::CPUI_MAX)) {
            throw ghidra::BadDataError("Provider returned an invalid p-code opcode");
        }
        if (operation.output) {
            validate_storage(*operation.output, "P-code output");
        }
        if (operation.inputs.size() > static_cast<std::size_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw ghidra::BadDataError("P-code input count exceeds the native limit");
        }
        for (const Storage& input : operation.inputs) {
            validate_storage(input, "P-code input");
        }
    }
}

/// Returns whether a legacy LOAD/STORE constant selector names the supplied
/// memory space. `memory_space` is the canonical frontend field, but the old
/// selector is accepted without dropping a real constant address.
static bool has_matching_memory_selector(const PcodeOperation& operation, ghidra::AddrSpace* memory_space,
                                         const ghidra::Translate* translator) {
    if (operation.inputs.empty() || operation.inputs.front().space != "const") {
        return false;
    }
    const ghidra::Address encoded_space = translator->createConstFromSpace(memory_space);
    return operation.inputs.front().offset == encoded_space.getOffset();
}

/// Checks a provider storage range against the actual native address space
/// limit after its name has been resolved.
static void validate_space_range(const Storage& storage, ghidra::AddrSpace* space, std::string_view context) {
    if (space->getType() == ghidra::IPTR_CONSTANT) {
        return;
    }
    if (storage.offset > space->getHighest() || storage.size - 1U > space->getHighest() - storage.offset) {
        throw ghidra::BadDataError(std::string(context) + " exceeds its address space");
    }
}

/// Adapts the provider contract to the native Ghidra Translate interface.
class ProviderTranslate final : public ghidra::Translate {
public:
    /// Builds address spaces and register mappings from provider metadata.
    ProviderTranslate(const ArchitectureDescription& description, std::shared_ptr<PcodeProvider> provider)
        : provider_(std::move(provider)), pointer_size_(description.pointer_size) {
        if (!provider_) {
            throw std::invalid_argument("A p-code provider is required");
        }
        if (pointer_size_ == 0 ||
            pointer_size_ > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Architecture pointer size is outside the native range");
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
            validate_storage(register_description.location, "Register description");
            validate_space_range(register_description.location, space, "Register description");
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
        if (size <= 0 || offset > std::numeric_limits<ghidra::uintb>::max() - static_cast<ghidra::uintb>(size)) {
            return {};
        }
        ghidra::string result;
        for (const auto& entry : registers_) {
            const ghidra::VarnodeData& location = entry.second;
            if (location.space == space && location.offset <= offset &&
                location.offset <= std::numeric_limits<ghidra::uintb>::max() - location.size &&
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
        validate_instruction(*result, address.getOffset());
        return static_cast<ghidra::int4>(result->length);
    }

    /// Emits provider p-code as native VarnodeData values.
    ghidra::int4 oneInstruction(ghidra::PcodeEmit& emit, const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        validate_instruction(*result, address.getOffset());
        const bool has_call = std::any_of(result->pcode.begin(), result->pcode.end(), [](const PcodeOperation& op) {
            return op.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_CALL) ||
                   op.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_CALLIND);
        });
        for (const PcodeOperation& operation : result->pcode) {
            // x86 CALL semantics materialize the return PC as a STORE before
            // the CALL op. The native compiler specification models this
            // location as the function return-address effect; discard only
            // that exact synthetic store so it cannot become a user local.
            if (has_call && operation.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_STORE) &&
                !operation.inputs.empty() && operation.inputs.back().space == "const" &&
                address.getOffset() <= std::numeric_limits<std::uint64_t>::max() - result->length &&
                operation.inputs.back().offset == address.getOffset() + result->length) {
                continue;
            }
            std::vector<ghidra::VarnodeData> inputs;
            std::size_t input_index = 0;
            if (operation.memory_space.has_value() &&
                (operation.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_LOAD) ||
                 operation.opcode == static_cast<ghidra::uint4>(ghidra::CPUI_STORE))) {
                ghidra::AddrSpace* memory_space = getSpaceByName(*operation.memory_space);
                if (memory_space == nullptr) {
                    throw ghidra::BadDataError("P-code references an unknown memory space: " + *operation.memory_space);
                }
                const ghidra::Address encoded_space = createConstFromSpace(memory_space);
                inputs.push_back(ghidra::VarnodeData{getConstantSpace(), encoded_space.getOffset(),
                                                     static_cast<ghidra::uint4>(pointer_size_)});
                if (has_matching_memory_selector(operation, memory_space, this)) {
                    input_index = 1;
                }
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
        validate_instruction(*result, address.getOffset());
        emit.dump(address, result->mnemonic, result->assembly);
        return static_cast<ghidra::int4>(result->length);
    }

private:
    /// Converts one public storage record into the native storage triple.
    ghidra::VarnodeData materialize(const Storage& storage) const {
        validate_storage(storage, "P-code storage");
        ghidra::AddrSpace* space = getSpaceByName(storage.space);
        if (space == nullptr) {
            throw ghidra::BadDataError("P-code references an unknown address space: " + storage.space);
        }
        validate_space_range(storage, space, "P-code storage");
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
    std::uint32_t pointer_size_;
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
        ghidra::forcePrintCLanguageRegistration();
        if (description_.pointer_size == 0) {
            throw std::invalid_argument("Architecture pointer size must be greater than zero");
        }
        const auto supplied_space = [&](const std::string& name) {
            return std::any_of(description_.spaces.begin(), description_.spaces.end(),
                               [&](const SpaceDescription& space) { return space.name == name; });
        };
        if (!description_.spaces.empty()) {
            const auto first_space = std::find_if(description_.spaces.begin(), description_.spaces.end(),
                                                  [](const SpaceDescription& space) { return !space.name.empty(); });
            if (first_space == description_.spaces.end()) {
                throw std::invalid_argument("Architecture description has no named processor address space");
            }
            if (!supplied_space(description_.code_space)) {
                description_.code_space = first_space->name;
            }
            if (!supplied_space(description_.data_space)) {
                description_.data_space = description_.code_space;
            }
        }
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
        // The bootstrap model follows `architecture.cc`'s compiler-spec
        // boundary but is deliberately assembled only from provider metadata.
        // It must not mention x86 registers, `ram`, or `stack` for an
        // architecture that did not supply those names.
        const auto xml_escape = [](std::string_view value) {
            std::string escaped;
            for (const char character : value) {
                switch (character) {
                    case '&':
                        escaped += "&amp;";
                        break;
                    case '<':
                        escaped += "&lt;";
                        break;
                    case '>':
                        escaped += "&gt;";
                        break;
                    case '\"':
                        escaped += "&quot;";
                        break;
                    case '\'':
                        escaped += "&apos;";
                        break;
                    default:
                        escaped += character;
                        break;
                }
            }
            return escaped;
        };
        const std::string model_name =
            description_.calling_convention.empty() || description_.calling_convention == "__thiscall"
                ? "provider-default"
                : description_.calling_convention;
        std::string bootstrap_stack_register = description_.stack_register;
        const auto stack_register_exists = [&](std::string_view name) {
            return std::any_of(description_.registers.begin(), description_.registers.end(),
                               [&](const RegisterDescription& reg) { return reg.name == name; });
        };
        const bool has_declared_stack_register = stack_register_exists(description_.stack_register);
        if (!stack_register_exists(bootstrap_stack_register)) {
            const auto first_register = std::find_if(
                description_.registers.begin(), description_.registers.end(),
                [](const RegisterDescription& reg) { return !reg.name.empty() && reg.location.size != 0; });
            if (first_register == description_.registers.end()) {
                throw std::invalid_argument("Architecture bootstrap requires at least one named register");
            }
            // The native ProtoModel requires a stack space even for a
            // provider that has no declared ABI stack register. Use the first
            // provider register only as a generic bootstrap anchor; never
            // synthesize an x86 register or address-space name.
            bootstrap_stack_register = first_register->name;
        }
        std::string input_register_entries;
        std::string output_register_entry;
        std::vector<std::pair<std::string, std::string>> input_register_candidates;
        for (const RegisterDescription& register_description : description_.registers) {
            if (register_description.name.empty() || register_description.location.size == 0) {
                continue;
            }
            const std::string entry = "<pentry minsize=\"1\" maxsize=\"" +
                                      std::to_string(register_description.location.size) + "\"><register name=\"" +
                                      xml_escape(register_description.name) + "\"/></pentry>";
            if (output_register_entry.empty()) {
                output_register_entry = entry;
            } else if (register_description.name != bootstrap_stack_register) {
                input_register_candidates.emplace_back(register_description.name, entry);
            }
        }
        // Keep the bootstrap model small and deterministic. Provider register
        // order remains authoritative; when more than four inputs are offered,
        // retain the first and last two slots rather than introducing an ABI
        // name or architecture-specific calling convention.
        const auto append_input = [&](std::size_t index) {
            input_register_entries += input_register_candidates[index].second;
        };
        if (input_register_candidates.size() <= 4) {
            for (std::size_t index = 0; index < input_register_candidates.size(); ++index) {
                append_input(index);
            }
        } else {
            append_input(0);
            append_input(1);
            append_input(input_register_candidates.size() - 2);
            append_input(input_register_candidates.size() - 1);
        }
        const std::string data_space = xml_escape(description_.data_space);
        const auto has_register = [&](std::string_view name) {
            return std::any_of(description_.registers.begin(), description_.registers.end(),
                               [&](const RegisterDescription& reg) { return reg.name == name; });
        };
        const bool legacy_x86 = description_.data_space == "ram" && description_.stack_register == "RSP" &&
                                has_register("RCX") && has_register("RDX") && has_register("R8") &&
                                has_register("R9") && has_register("RAX");
        std::string specification_xml;
        if (legacy_x86) {
            // Preserve the original provider's x86-64 bootstrap contract and
            // generated C output while allowing generic descriptions below.
            specification_xml = "<compiler_spec><global><range space=\"ram\"/></global>";
            specification_xml += "<stackpointer register=\"RSP\" space=\"ram\"/>";
            specification_xml += "<returnaddress><varnode space=\"stack\" offset=\"0\" size=\"8\"/></returnaddress>";
            specification_xml += "<default_proto><prototype name=\"" + xml_escape(description_.calling_convention) +
                                 "\" extrapop=\"8\" stackshift=\"8\"><input>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"RCX\"/></pentry>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"RDX\"/></pentry>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"R8\"/></pentry>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"R9\"/></pentry>"
                                 "</input><output><pentry minsize=\"1\" maxsize=\"8\"><register name=\"RAX\"/>"
                                 "</pentry></output></prototype></default_proto></compiler_spec>";
        } else {
            specification_xml = "<compiler_spec><global><range space=\"" + data_space + "\"/></global>";
            if (getSpaceByName(description_.data_space) != nullptr) {
                specification_xml += "<stackpointer register=\"" + xml_escape(bootstrap_stack_register) +
                                     "\" space=\"" + data_space + "\"/>";
                if (has_declared_stack_register && getSpaceByName("stack") != nullptr) {
                    specification_xml += "<returnaddress><varnode space=\"stack\" offset=\"0\" size=\"" +
                                         std::to_string(description_.pointer_size) + "\"/></returnaddress>";
                }
            }
            specification_xml += "<default_proto><prototype name=\"" + xml_escape(model_name) +
                                 "\" extrapop=\"0\"><input>" + input_register_entries + "</input><output>" +
                                 output_register_entry + "</output></prototype></default_proto></compiler_spec>";
        }
        std::istringstream specification_text(specification_xml);
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
    std::size_t first_input = 0;
    if (value.memory_space.has_value() &&
        (value.opcode == sleigh_runtime::PcodeOpcode::load || value.opcode == sleigh_runtime::PcodeOpcode::store) &&
        !value.inputs.empty() && value.inputs.front().space == "const") {
        // Sleigh's native LOAD/STORE encoding carries the target space as a
        // leading constant selector. The provider contract carries that
        // information in memory_space, so remove only this known selector and
        // retain constant address operands that follow it.
        first_input = 1;
    }
    for (std::size_t index = first_input; index < value.inputs.size(); ++index) {
        const sleigh_runtime::Varnode& input = value.inputs[index];
        result.inputs.push_back(convert_storage(input));
    }
    result.memory_space = value.memory_space;
    return result;
}

/// Resolves a provider type into the native type factory while retaining the
/// provider's composite shape, signedness, explicit sizes, typedef identity,
/// and optional source declaration. Recursive types are inserted into the
/// cache before their fields are resolved so self-referential providers can be
/// represented by the native factory.
/// Composite construction follows `type.cc`'s TypeFactory/TypeStruct/TypeUnion
/// responsibilities rather than reducing provider declarations to integers.
static ghidra::Datatype* resolve_provider_type(const ProviderContext& context, ghidra::TypeFactory* types,
                                               const std::string& name, std::map<std::string, ghidra::Datatype*>& cache,
                                               std::uint32_t pointer_size, std::vector<std::string>* declarations) {
    const auto cached = cache.find(name);
    if (cached != cache.end()) {
        return cached->second;
    }
    if (name == "void") {
        return types->getTypeVoid();
    }

    TypeDescription description;
    description.name = name;
    description.size = pointer_size;
    description.kind = name == "wchar_t" ? TypeKind::unicode_character
                                         : (name.find("unsigned") != std::string::npos ? TypeKind::unsigned_integer
                                                                                       : TypeKind::signed_integer);
    if (context.types) {
        if (const std::optional<TypeDescription> supplied = context.types->type_named(name)) {
            description = *supplied;
        }
    }
    const std::string effective_name = description.name.empty() ? name : description.name;
    if (declarations != nullptr && !description.declaration.empty() &&
        std::find(declarations->begin(), declarations->end(), description.declaration) == declarations->end()) {
        declarations->push_back(description.declaration);
    }
    // Compiler specifications can predeclare provider names. Reusing those
    // native datatypes avoids redefining a composite when several functions
    // reference the same metadata name.
    if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
        cache.emplace(name, existing);
        return existing;
    }

    if (description.kind == TypeKind::void_type || name == "void") {
        cache.emplace(name, types->getTypeVoid());
        return types->getTypeVoid();
    }
    if (description.kind == TypeKind::pointer) {
        ghidra::Datatype* pointed_to =
            resolve_provider_type(context, types, description.element_type, cache, pointer_size, declarations);
        const std::uint32_t size = description.size == 0 ? pointer_size : description.size;
        if (size == 0 || size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider pointer type has an invalid size: " + effective_name);
        }
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getMetatype() != ghidra::TYPE_PTR || existing->getSize() != static_cast<ghidra::int4>(size)) {
                throw std::invalid_argument("Provider pointer conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return existing;
        }
        ghidra::TypePointer* pointer =
            types->getTypePointer(static_cast<ghidra::int4>(size), pointed_to, 1, effective_name);
        cache.emplace(name, pointer);
        return pointer;
    }
    if (description.kind == TypeKind::array) {
        if (description.element_count > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider array has too many elements: " + effective_name);
        }
        ghidra::Datatype* element =
            resolve_provider_type(context, types, description.element_type, cache, pointer_size, declarations);
        const std::uint64_t array_size = static_cast<std::uint64_t>(element->getSize()) * description.element_count;
        if (array_size > static_cast<std::uint64_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider array size overflows the native type limit: " + effective_name);
        }
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getMetatype() != ghidra::TYPE_ARRAY ||
                existing->getSize() != static_cast<ghidra::int4>(array_size)) {
                throw std::invalid_argument("Provider array conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return existing;
        }
        ghidra::TypeArray* array = types->getTypeArray(static_cast<ghidra::int4>(description.element_count), element);
        cache.emplace(name, array);
        return array;
    }
    if (description.kind == TypeKind::structure || description.kind == TypeKind::union_type) {
        const bool is_union = description.kind == TypeKind::union_type;
        const ghidra::type_metatype expected_metatype = is_union ? ghidra::TYPE_UNION : ghidra::TYPE_STRUCT;
        ghidra::Datatype* composite = types->findByName(effective_name);
        if (composite != nullptr) {
            if (composite->getMetatype() != expected_metatype) {
                throw std::invalid_argument("Provider type changes composite kind: " + effective_name);
            }
            // A complete native definition may have come from another
            // provider lookup or compiler specification. Reuse it instead of
            // passing it through assignRawFields a second time.
            if (!composite->isIncomplete()) {
                if (description.size != 0 && composite->getSize() != static_cast<ghidra::int4>(description.size)) {
                    throw std::invalid_argument("Provider composite conflicts with an existing type: " +
                                                effective_name);
                }
                cache.emplace(name, composite);
                return composite;
            }
        } else {
            composite = is_union ? static_cast<ghidra::Datatype*>(types->getTypeUnion(effective_name))
                                 : static_cast<ghidra::Datatype*>(types->getTypeStruct(effective_name));
        }
        cache.emplace(name, composite);
        std::vector<ghidra::TypeField> fields;
        ghidra::int4 field_id = 0;
        ghidra::int4 inferred_size = 0;
        for (const TypeFieldDescription& field : description.fields) {
            ghidra::Datatype* field_type =
                resolve_provider_type(context, types, field.type_name, cache, pointer_size, declarations);
            if (!is_union && (field.offset > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max()) ||
                              field.offset > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max()) -
                                                 static_cast<std::uint32_t>(field_type->getSize()))) {
                throw std::invalid_argument("Provider field offset overflows composite type: " + effective_name);
            }
            const ghidra::int4 offset = is_union ? 0 : static_cast<ghidra::int4>(field.offset);
            fields.emplace_back(field_id++, offset, field.name, field_type);
            inferred_size = std::max(inferred_size, offset + field_type->getSize());
        }
        if (description.size != 0) {
            if (description.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max()) ||
                description.size < static_cast<std::uint32_t>(inferred_size)) {
                throw std::invalid_argument("Provider composite type has an incompatible size: " + effective_name);
            }
            if (description.size > static_cast<std::uint32_t>(inferred_size)) {
                const ghidra::int4 padding_size = static_cast<ghidra::int4>(description.size - inferred_size);
                fields.emplace_back(field_id++, is_union ? 0 : inferred_size, "__provider_padding",
                                    types->getBase(padding_size, ghidra::TYPE_UNKNOWN));
            }
        }
        if (is_union) {
            auto* union_type = static_cast<ghidra::TypeUnion*>(composite);
            types->assignRawFields(union_type, fields);
        } else {
            auto* structure = static_cast<ghidra::TypeStruct*>(composite);
            std::vector<ghidra::TypeBitField> bitfields;
            types->assignRawFields(structure, fields, bitfields);
        }
        return composite;
    }

    if (description.kind == TypeKind::unicode_character) {
        const std::uint32_t size = description.size == 0 ? 2 : description.size;
        if (size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider unicode type has an invalid size: " + effective_name);
        }
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getMetatype() != ghidra::TYPE_INT || existing->getSize() != static_cast<ghidra::int4>(size)) {
                throw std::invalid_argument("Provider unicode type conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return existing;
        }
        ghidra::Datatype* result =
            types->getProviderUnicode(effective_name, static_cast<ghidra::int4>(size), ghidra::TYPE_INT);
        cache.emplace(name, result);
        return result;
    }
    if (description.kind == TypeKind::typedef_type) {
        if (description.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider typedef has an invalid size: " + effective_name);
        }
        ghidra::Datatype* underlying =
            description.element_type.empty()
                ? types->getBase(static_cast<ghidra::int4>(description.size == 0 ? 1 : description.size),
                                 description.signed_value ? ghidra::TYPE_INT : ghidra::TYPE_UINT)
                : resolve_provider_type(context, types, description.element_type, cache, pointer_size, declarations);
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getTypedef() != underlying || existing->getSize() != underlying->getSize()) {
                throw std::invalid_argument("Provider typedef conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return existing;
        }
        ghidra::Datatype* result = types->getTypedef(underlying, effective_name, 0, 0);
        cache.emplace(name, result);
        return result;
    }
    const ghidra::type_metatype metatype = description.kind == TypeKind::boolean            ? ghidra::TYPE_BOOL
                                           : description.kind == TypeKind::floating_point   ? ghidra::TYPE_FLOAT
                                           : description.kind == TypeKind::unsigned_integer ? ghidra::TYPE_UINT
                                           : description.kind == TypeKind::signed_integer && !description.signed_value
                                               ? ghidra::TYPE_UINT
                                               : ghidra::TYPE_INT;
    if (description.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
        throw std::invalid_argument("Provider type has an invalid size: " + effective_name);
    }
    const ghidra::int4 size = static_cast<ghidra::int4>(description.size == 0 ? 1 : description.size);
    if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
        if (existing->getMetatype() != metatype || existing->getSize() != size) {
            throw std::invalid_argument("Provider base type conflicts with an existing type: " + effective_name);
        }
        cache.emplace(name, existing);
        return existing;
    }
    ghidra::Datatype* result = types->getBase(size, metatype, effective_name);
    cache.emplace(name, result);
    return result;
}

/// Resolves a provider calling-convention name to a native model. The
/// architecture default is used for the explicit `default` value; unknown
/// names are cloned as native unknown models so the requested convention is
/// still retained in generated declarations.
static ghidra::ProtoModel* resolve_provider_model(ghidra::Architecture* architecture, std::string_view convention) {
    if (convention.empty() || convention == "default") {
        return architecture->defaultfp;
    }
    if (ghidra::ProtoModel* model = architecture->getModel(std::string(convention)); model != nullptr) {
        return model;
    }
    return architecture->createUnknownModel(std::string(convention));
}

/// Applies provider types, the requested calling convention, and exact
/// parameter/return storage without asking the bootstrap ABI to reassign any
/// explicitly supplied locations. The model fills omitted locations, while
/// provider locations override those entries exactly.
/// Storage installation follows `fspec.cc`'s FuncProto::setPieces/setParam
/// contract so void returns without explicit storage remain valid native
/// prototypes.
static void apply_provider_prototype(ghidra::Architecture* architecture, ghidra::Funcdata* data,
                                     const std::string& function_name, const PrototypeDescription& prototype,
                                     const ProviderContext& context, std::uint32_t pointer_size,
                                     std::map<std::string, ghidra::Datatype*>& type_cache,
                                     std::vector<std::string>* declarations) {
    ghidra::PrototypePieces pieces{};
    pieces.model = resolve_provider_model(architecture, prototype.calling_convention);
    pieces.name = function_name;
    pieces.outtype = resolve_provider_type(context, architecture->types, prototype.return_type, type_cache,
                                           pointer_size, declarations);
    pieces.firstVarArgSlot = -1;
    for (const PrototypeParameterDescription& parameter : prototype.parameters) {
        pieces.innames.push_back(parameter.name);
        pieces.intypes.push_back(resolve_provider_type(context, architecture->types, parameter.type_name, type_cache,
                                                       pointer_size, declarations));
    }

    ghidra::FuncProto& function_prototype = data->getFuncProto();
    function_prototype.setCustomStorage(true);
    function_prototype.setPieces(pieces);
    if (prototype.return_storage) {
        validate_storage(*prototype.return_storage, "Prototype return storage");
        ghidra::AddrSpace* output_space = architecture->getSpaceByName(prototype.return_storage->space);
        if (output_space == nullptr) {
            throw std::runtime_error("Prototype references an unknown return storage space: " +
                                     prototype.return_storage->space);
        }
        validate_space_range(*prototype.return_storage, output_space, "Prototype return storage");
        ghidra::ParameterPieces output{};
        output.type = pieces.outtype;
        output.flags = ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::sizelock;
        output.addr = ghidra::Address(output_space, prototype.return_storage->offset);
        function_prototype.setOutput(output);
    }
    for (std::size_t index = 0; index < prototype.parameters.size(); ++index) {
        const PrototypeParameterDescription& parameter = prototype.parameters[index];
        if (!parameter.storage) {
            continue;
        }
        ghidra::ParameterPieces parameter_storage{};
        parameter_storage.type = pieces.intypes[index];
        parameter_storage.flags =
            ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::namelock | ghidra::ParameterPieces::sizelock;
        if (parameter.storage) {
            validate_storage(*parameter.storage, "Prototype parameter storage");
            ghidra::AddrSpace* parameter_space = architecture->getSpaceByName(parameter.storage->space);
            if (parameter_space == nullptr) {
                throw std::runtime_error("Prototype references an unknown parameter storage space: " +
                                         parameter.storage->space);
            }
            validate_space_range(*parameter.storage, parameter_space, "Prototype parameter storage");
            parameter_storage.addr = ghidra::Address(parameter_space, parameter.storage->offset);
        }
        function_prototype.setParam(static_cast<ghidra::int4>(index), parameter.name, parameter_storage);
    }
    function_prototype.clearProviderErrors();
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
    std::map<std::string, ghidra::Datatype*> type_cache;
    std::vector<std::string> type_declarations;
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
    std::expected<std::vector<std::uint8_t>, ProviderError> bytes =
        std::unexpected(ProviderError{"No mapped bytes are available"});
    ProviderError last_error{"No mapped bytes are available"};
    for (std::size_t window = 16; window != 0; --window) {
        const auto attempt = state_->memory->read(address, window);
        if (attempt) {
            bytes = *attempt;
            break;
        }
        last_error = attempt.error();
    }
    if (!bytes) {
        return std::unexpected(last_error);
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
    if (result.length == 0 || result.length > bytes->size()) {
        return std::unexpected(ProviderError{"Sleigh decoder returned an instruction longer than its mapped window"});
    }
    try {
        detail::validate_instruction(result, address);
    } catch (const ghidra::LowlevelError& error) {
        return std::unexpected(ProviderError{error.explain});
    } catch (const std::exception& error) {
        return std::unexpected(ProviderError{error.what()});
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
        try {
            detail::validate_instruction(*decoded, address);
        } catch (const ghidra::LowlevelError& error) {
            throw std::runtime_error(error.explain);
        }
        if (decoded->length > function.end - address) {
            throw std::runtime_error("Provider returned an instruction outside the function range");
        }
        result.raw_instructions.push_back(*decoded);
        address += decoded->length;
    }

    ghidra::AddrSpace* code_space = state_->architecture->getDefaultCodeSpace();
    const ghidra::Address entry(code_space, function.entry);
    std::string function_name = function.name;
    std::string function_namespace;
    if (state_->context.symbols) {
        const std::optional<SymbolDescription> symbol = state_->context.symbols->symbol_at(function.entry);
        if (symbol && !symbol->name.empty()) {
            function_name = symbol->name;
            function_namespace = symbol->namespace_name;
        }
    }
    auto add_function_symbol = [&](std::uint64_t address, const std::string& name, const std::string& namespace_name) {
        ghidra::Scope* scope = state_->architecture->symboltab->getGlobalScope();
        std::string basename = name;
        if (!namespace_name.empty()) {
            scope = state_->architecture->symboltab->findCreateScopeFromSymbolName(namespace_name + "::" + name,
                                                                                   basename, scope);
        }
        return scope->addFunction(ghidra::Address(code_space, address), basename);
    };
    auto install_external_function = [&](std::uint64_t address, const std::string& fallback_name) {
        std::string external_name = fallback_name;
        std::string external_namespace;
        if (state_->context.symbols) {
            const std::optional<SymbolDescription> symbol = state_->context.symbols->symbol_at(address);
            if (symbol && !symbol->name.empty()) {
                external_name = symbol->name;
                external_namespace = symbol->namespace_name;
            }
        }
        ghidra::FunctionSymbol* external_symbol = add_function_symbol(address, external_name, external_namespace);
        ghidra::Funcdata* external_data = external_symbol->getFunction();
        if (!state_->context.prototypes) {
            return;
        }
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(address);
        if (!prototype) {
            return;
        }
        detail::apply_provider_prototype(state_->architecture.get(), external_data, external_name, *prototype,
                                         state_->context, state_->description.pointer_size, state_->type_cache,
                                         &state_->type_declarations);
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
    ghidra::FunctionSymbol* symbol = add_function_symbol(function.entry, function_name, function_namespace);
    ghidra::Funcdata* data = symbol->getFunction();

    if (state_->context.prototypes) {
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(function.entry);
        if (prototype) {
            detail::apply_provider_prototype(state_->architecture.get(), data, function_name, *prototype,
                                             state_->context, state_->description.pointer_size, state_->type_cache,
                                             &state_->type_declarations);
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
        for (const VariableDescription& variable : state_->context.variables->variables_at(function.entry)) {
            ghidra::AddrSpace* variable_space = state_->architecture->getSpaceByName(variable.storage.space);
            if (variable_space == nullptr) {
                throw std::runtime_error("Variable references an unknown storage space: " + variable.storage.space);
            }
            ghidra::Address variable_address(variable_space, variable.storage.offset);
            ghidra::Datatype* variable_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, variable.type_name, state_->type_cache,
                state_->description.pointer_size, &state_->type_declarations);
            data->getScopeLocal()->addTypeRecommendation(variable_address, variable_type);
            data->getScopeLocal()->addNameRecommendation(variable_address, entry, variable_type->getSize(),
                                                         variable.name);
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
        for (const VariableDescription& variable : state_->context.variables->variables_at(function.entry)) {
            ghidra::AddrSpace* variable_space = state_->architecture->getSpaceByName(variable.storage.space);
            if (variable_space == nullptr) {
                continue;
            }
            ghidra::Datatype* variable_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, variable.type_name, state_->type_cache,
                state_->description.pointer_size, &state_->type_declarations);
            if (ghidra::MapEntry* map = data->getScopeLocal()->findOverlap(
                    ghidra::Address(variable_space, variable.storage.offset), variable_type->getSize())) {
                map->getSymbol()->setProviderInfo(variable.name, variable_type);
            }
        }
    }
    if (state_->context.prototypes) {
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(function.entry);
        if (prototype) {
            ghidra::Datatype* return_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, prototype->return_type, state_->type_cache,
                state_->description.pointer_size, &state_->type_declarations);
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
    // Provider declarations are source-level metadata rather than native
    // engine objects. Emit each supplied declaration once before the function
    // so typedefs and composite declarations are not silently discarded.
    // Recursive type resolution records aliases before their element types;
    // reverse insertion order restores dependency order for emitted C.
    for (auto declaration = state_->type_declarations.rbegin(); declaration != state_->type_declarations.rend();
         ++declaration) {
        c_output << *declaration;
        if (declaration->empty() || declaration->back() != '\n') {
            c_output << '\n';
        }
    }
    state_->architecture->print->setOutputStream(&c_output);
    state_->architecture->print->setFlat(false);
    state_->architecture->print->docFunction(data);
    result.c_source = c_output.str();
    return result;
}

} // namespace newghidra::decompiler
