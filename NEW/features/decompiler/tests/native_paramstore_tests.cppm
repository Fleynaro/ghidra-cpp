module;

#include <gtest/gtest.h>

export module native_paramstore_tests;

import ghidra.decompiler;
import ghidra.decompiler.grammar;
import std;

// Port reference: Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc.
// Native implementation references: architecture.cc, fspec.cc, grammar.cc, and the
// compiler-spec model rules consumed by Architecture::restoreFromSpec.

namespace newghidra::decompiler::tests {

/// Identifies one architecture whose compiler-spec parameter rules are under test.
enum class ParameterArchitecture {
    x64,
    ppc64_be,
    mips32_be,
    aarch64,
};

/// Describes one processor address space supplied to the native translator.
struct ProviderSpace {
    std::string name;
    std::uint32_t address_size;
    std::uint32_t word_size;
    bool big_endian;
    std::int32_t index;
};

/// Describes one named register location supplied by the architecture provider.
struct ProviderRegister {
    std::string name;
    std::string space;
    std::uint64_t offset;
    std::uint32_t size;
};

/// Collects provider metadata and the authoritative compiler-spec source for one ABI.
struct ParameterArchitectureConfig {
    std::string name;
    std::uint32_t pointer_size;
    std::string code_space;
    std::string data_space;
    std::string stack_register;
    std::filesystem::path compiler_spec;
    std::vector<ProviderSpace> spaces;
    std::vector<ProviderRegister> registers;
};

/// Resolves a compiler specification from the module-local test data rather than the process directory.
static std::filesystem::path repository_file(std::string_view relative_path) {
    const std::filesystem::path configured_root(NATIVE_PARAMSTORE_DATA_ROOT);
    if (!configured_root.empty()) {
        return configured_root / relative_path;
    }
    return std::filesystem::path("data") / "compiler_specs" / relative_path;
}

/// Adds one named register binding to a provider architecture description.
static void add_register(ParameterArchitectureConfig& config, std::string name, std::string space, std::uint64_t offset,
                         std::uint32_t size) {
    config.registers.push_back(ProviderRegister{std::move(name), std::move(space), offset, size});
}

/// Adds the common integer-register aliases used by source signatures and compiler specs.
static void add_x64_integer_registers(ParameterArchitectureConfig& config) {
    const std::vector<std::pair<std::string, std::uint64_t>> registers{
        {"RAX", 0x00}, {"RCX", 0x08}, {"RDX", 0x10}, {"RBX", 0x18}, {"RSP", 0x20}, {"RBP", 0x28},
        {"RSI", 0x30}, {"RDI", 0x38}, {"R8", 0x80},  {"R9", 0x88},  {"R10", 0x90}, {"R11", 0x98},
        {"R12", 0xa0}, {"R13", 0xa8}, {"R14", 0xb0}, {"R15", 0xb8},
    };
    for (const auto& [name, offset] : registers) {
        add_register(config, name, "register", offset, 8);
    }

    const std::vector<std::pair<std::string, std::string>> four_byte_aliases{
        {"EAX", "RAX"}, {"ECX", "RCX"}, {"EDX", "RDX"}, {"EBX", "RBX"},
        {"ESP", "RSP"}, {"EBP", "RBP"}, {"ESI", "RSI"}, {"EDI", "RDI"},
    };
    const std::vector<std::pair<std::string, std::string>> two_byte_aliases{
        {"AX", "RAX"}, {"CX", "RCX"}, {"DX", "RDX"}, {"BX", "RBX"},
        {"SP", "RSP"}, {"BP", "RBP"}, {"SI", "RSI"}, {"DI", "RDI"},
    };
    for (const auto& [alias, parent] : four_byte_aliases) {
        const auto parent_it =
            std::find_if(registers.begin(), registers.end(), [&](const auto& entry) { return entry.first == parent; });
        add_register(config, alias, "register", parent_it->second, 4);
    }
    for (const auto& [alias, parent] : two_byte_aliases) {
        const auto parent_it =
            std::find_if(registers.begin(), registers.end(), [&](const auto& entry) { return entry.first == parent; });
        add_register(config, alias, "register", parent_it->second, 2);
    }
}

/// Builds x64 SIMD register aliases with the same low-lane locations used by x86-64-gcc.cspec.
static void add_x64_float_registers(ParameterArchitectureConfig& config) {
    for (std::uint32_t index = 0; index < 8; ++index) {
        const std::uint64_t offset = 0x100 + static_cast<std::uint64_t>(index) * 0x10;
        const std::string name = "XMM" + std::to_string(index);
        add_register(config, name, "register", offset, 16);
        add_register(config, name + "_Qa", "register", offset, 8);
    }
    add_register(config, "MXCSR", "register", 0x300, 4);
    add_register(config, "DF", "register", 0x304, 1);
}

/// Builds the x86-64 provider metadata used with the original System V compiler specification.
static ParameterArchitectureConfig x64_config() {
    ParameterArchitectureConfig config{
        "x86:LE:64:default:gcc",
        8,
        "ram",
        "ram",
        "RSP",
        repository_file("x86-64-gcc.cspec"),
        {{"ram", 8, 1, false, 2}, {"register", 8, 1, false, 3}},
        {},
    };
    add_x64_integer_registers(config);
    add_x64_float_registers(config);
    return config;
}

/// Builds the PowerPC64 big-endian register metadata used by ppc_64_be.cspec.
static ParameterArchitectureConfig ppc64_be_config() {
    ParameterArchitectureConfig config{
        "PowerPC:BE:64:default:default",
        8,
        "ram",
        "ram",
        "r1",
        repository_file("ppc_64_be.cspec"),
        {{"ram", 8, 1, true, 2}, {"register", 8, 1, true, 3}},
        {},
    };
    for (std::uint32_t index = 0; index < 32; ++index) {
        add_register(config, "r" + std::to_string(index), "register", 0x100 + index * 8ULL, 8);
        add_register(config, "f" + std::to_string(index), "register", 0x300 + index * 8ULL, 8);
    }
    add_register(config, "r2Save", "register", 0x110, 8);
    for (std::uint32_t index = 2; index <= 4; ++index) {
        add_register(config, "cr" + std::to_string(index), "register", 0x500 + index * 4ULL, 4);
    }
    return config;
}

/// Builds the MIPS32 big-endian aliases used by mips32be.cspec and its ABI rules.
static ParameterArchitectureConfig mips32_be_config() {
    ParameterArchitectureConfig config{
        "MIPS:BE:32:default:default",
        4,
        "ram",
        "ram",
        "sp",
        repository_file("mips32be.cspec"),
        {{"ram", 4, 1, true, 2}, {"register", 4, 1, true, 3}},
        {},
    };
    for (std::uint32_t index = 0; index < 32; ++index) {
        // The big-endian SLEIGH declaration lists each odd FPR before its even
        // partner, so a scalar f12/f14 is the upper half of its paired pentry.
        const std::uint64_t scalar_offset = 0x100 + ((index % 2 == 0) ? index + 1 : index - 1) * 4ULL;
        add_register(config, "f" + std::to_string(index), "register", scalar_offset, 4);
        add_register(config, "s" + std::to_string(index), "register", 0x200 + index * 4ULL, 4);
    }
    for (std::uint32_t index = 0; index < 4; ++index) {
        add_register(config, "a" + std::to_string(index), "register", index * 4ULL, 4);
    }
    add_register(config, "at", "register", 0x10, 4);
    add_register(config, "v0", "register", 0x20, 4);
    add_register(config, "v1", "register", 0x24, 4);
    add_register(config, "sp", "register", 0x2c, 4);
    add_register(config, "gp", "register", 0x30, 4);
    add_register(config, "ra", "register", 0x34, 4);
    add_register(config, "f12_13", "register", 0x100 + 12 * 4ULL, 8);
    add_register(config, "f14_15", "register", 0x100 + 14 * 4ULL, 8);
    add_register(config, "f0_1", "register", 0x100, 8);
    return config;
}

/// Builds AArch64 vector, scalar, and integer aliases used by AARCH64.cspec.
static ParameterArchitectureConfig aarch64_config() {
    ParameterArchitectureConfig config{
        "AARCH64:LE:64:v8A:default",
        8,
        "ram",
        "ram",
        "sp",
        repository_file("AARCH64.cspec"),
        {{"ram", 8, 1, false, 2}, {"register", 8, 1, false, 3}},
        {},
    };
    for (std::uint32_t index = 0; index < 31; ++index) {
        const std::uint64_t offset = index * 8ULL;
        add_register(config, "x" + std::to_string(index), "register", offset, 8);
        add_register(config, "w" + std::to_string(index), "register", offset, 4);
    }
    add_register(config, "sp", "register", 31 * 8ULL, 8);
    for (std::uint32_t index = 0; index < 32; ++index) {
        const std::uint64_t offset = 0x200 + index * 0x10ULL;
        add_register(config, "q" + std::to_string(index), "register", offset, 16);
        add_register(config, "d" + std::to_string(index), "register", offset, 8);
        add_register(config, "s" + std::to_string(index), "register", offset, 4);
    }
    return config;
}

/// Selects a complete provider configuration without consulting Java, XML processor capabilities, or test mocks.
static ParameterArchitectureConfig architecture_config(ParameterArchitecture architecture) {
    switch (architecture) {
        case ParameterArchitecture::x64:
            return x64_config();
        case ParameterArchitecture::ppc64_be:
            return ppc64_be_config();
        case ParameterArchitecture::mips32_be:
            return mips32_be_config();
        case ParameterArchitecture::aarch64:
            return aarch64_config();
    }
    throw std::invalid_argument("Unknown parameter-storage architecture");
}

/// Provides the address spaces and register aliases required by the native Architecture algorithms.
class ProviderStorageTranslate final : public ghidra::Translate {
public:
    /// Constructs a translator from provider metadata; instruction decoding is intentionally outside this test's scope.
    explicit ProviderStorageTranslate(const ParameterArchitectureConfig& config) : config_(config) {}

    /// Materializes processor and unique spaces before Architecture copies them into its own manager.
    void initialize(ghidra::DocumentStorage&) override {
        setBigEndian(config_.spaces.front().big_endian);
        alignment = 1;
        insertSpace(new ghidra::ConstantSpace(this, this));
        for (const ProviderSpace& definition : config_.spaces) {
            insertSpace(new ghidra::AddrSpace(this, this, ghidra::IPTR_PROCESSOR, definition.name,
                                              definition.big_endian, definition.address_size, definition.word_size,
                                              definition.index, ghidra::AddrSpace::hasphysical, 0, 0));
        }
        insertSpace(new ghidra::UniqueSpace(this, this, numSpaces(), ghidra::AddrSpace::hasphysical));
        setDefaultCodeSpace(getSpaceByName(config_.code_space)->getIndex());
        setDefaultDataSpace(getSpaceByName(config_.data_space)->getIndex());
        for (const ProviderRegister& definition : config_.registers) {
            ghidra::AddrSpace* space = getSpaceByName(definition.space);
            if (space == nullptr) {
                throw std::invalid_argument("Register provider references an unknown space: " + definition.space);
            }
            registers_.emplace(definition.name, ghidra::VarnodeData{space, definition.offset,
                                                                    static_cast<ghidra::uint4>(definition.size)});
        }
    }

    /// Resolves a compiler-spec or grammar register name to its provider storage location.
    const ghidra::VarnodeData& getRegister(const ghidra::string& name) const override {
        const auto iterator = registers_.find(name);
        if (iterator == registers_.end()) {
            throw ghidra::LowlevelError("Unknown provider register: " + name);
        }
        return iterator->second;
    }

    /// Returns the smallest provider register containing a requested location.
    ghidra::string getRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset, ghidra::int4 size) const override {
        ghidra::string result;
        for (const auto& [name, location] : registers_) {
            if (location.space != space || location.offset > offset ||
                offset + static_cast<ghidra::uintb>(size) > location.offset + location.size) {
                continue;
            }
            if (result.empty() || location.size < getRegister(result).size) {
                result = name;
            }
        }
        return result;
    }

    /// Returns a provider register whose space, offset, and size match exactly.
    ghidra::string getExactRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset,
                                        ghidra::int4 size) const override {
        for (const auto& [name, location] : registers_) {
            if (location.space == space && location.offset == offset && location.size == size) {
                return name;
            }
        }
        return {};
    }

    /// Copies all provider register aliases into the native translator register table.
    void getAllRegisters(std::map<ghidra::VarnodeData, ghidra::string>& result) const override {
        for (const auto& [name, location] : registers_) {
            result.emplace(location, name);
        }
    }

    /// Reports that this parameter-storage-only provider has no user-defined p-code operations.
    void getUserOpNames(std::vector<ghidra::string>& result) const override {
        result.clear();
    }

    /// Rejects instruction-length requests because grammar and parameter assignment do not decode instructions.
    ghidra::int4 instructionLength(const ghidra::Address&) const override {
        throw ghidra::UnimplError("Parameter-storage provider does not decode instructions", 0);
    }

    /// Rejects p-code translation requests because this test exercises the native ABI model directly.
    ghidra::int4 oneInstruction(ghidra::PcodeEmit&, const ghidra::Address&) const override {
        throw ghidra::UnimplError("Parameter-storage provider does not emit instructions", 0);
    }

    /// Rejects assembly requests because this test exercises the native ABI model directly.
    ghidra::int4 printAssembly(ghidra::AssemblyEmit&, const ghidra::Address&) const override {
        throw ghidra::UnimplError("Parameter-storage provider does not print instructions", 0);
    }

private:
    const ParameterArchitectureConfig& config_;
    std::map<ghidra::string, ghidra::VarnodeData> registers_;
};

/// Supplies the minimal load-image lifetime required by Architecture::init without providing executable bytes.
class EmptyLoadImage final : public ghidra::LoadImage {
public:
    /// Constructs a named empty image for the parameter-storage architecture.
    EmptyLoadImage() : ghidra::LoadImage("native-paramstore") {}

    /// Rejects byte reads because no instruction or data image is needed by the ABI assignment algorithms.
    void loadFill(ghidra::uint1*, ghidra::int4, const ghidra::Address&) override {
        throw ghidra::DataUnavailError("Parameter-storage tests do not provide an executable image");
    }

    /// Identifies this image as a provider-backed parameter-storage fixture.
    ghidra::string getArchType() const override {
        return "native-paramstore";
    }

    /// Leaves all addresses unchanged because the fixture has no file VMA.
    void adjustVma(long) override {}
};

/// Supplies an empty injection library so compiler-spec parsing remains native without unrelated fixup payloads.
class EmptyInjectLibrary final : public ghidra::PcodeInjectLibrary {
public:
    /// Constructs an empty library tied to the native architecture's temporary space.
    EmptyInjectLibrary(ghidra::Architecture* architecture, ghidra::uint4 temporary_base)
        : ghidra::PcodeInjectLibrary(architecture, temporary_base) {}

    /// Rejects an injection request that is outside the parameter-storage contract.
    ghidra::int4 allocateInject(const ghidra::string&, const ghidra::string&, ghidra::int4) override {
        throw ghidra::LowlevelError("Parameter-storage fixture has no injection payloads");
    }

    /// Leaves the empty injection table unchanged.
    void registerInject(ghidra::int4) override {}

    /// Rejects manually supplied call-fixup source not used by ABI assignment.
    ghidra::int4 manualCallFixup(const ghidra::string&, const ghidra::string&) override {
        throw ghidra::LowlevelError("Parameter-storage fixture has no call-fixup payloads");
    }

    /// Rejects manually supplied call-other-fixup source not used by ABI assignment.
    ghidra::int4 manualCallOtherFixup(const ghidra::string&, const ghidra::string&, const std::vector<ghidra::string>&,
                                      const ghidra::string&) override {
        throw ghidra::LowlevelError("Parameter-storage fixture has no call-other-fixup payloads");
    }

    /// Returns the reusable empty injection context required by the native interface.
    ghidra::InjectContext& getCachedContext() override {
        return context_;
    }

    /// Returns an empty behavior table because no injection payload is decoded.
    const std::vector<ghidra::OpBehavior*>& getBehaviors() override {
        return behaviors_;
    }

private:
    /// Encodes no state because the fixture never injects p-code.
    class EmptyContext final : public ghidra::InjectContext {
    public:
        /// Encodes the intentionally empty injection context.
        void encode(ghidra::Encoder&) const override {}
    };

    EmptyContext context_;
    std::vector<ghidra::OpBehavior*> behaviors_;
};

/// Owns the real native Architecture configured from provider spaces and an authoritative compiler specification.
class ParameterStorageArchitecture final : public ghidra::Architecture {
public:
    /// Stores architecture metadata; initialize() is called only after capability registration.
    explicit ParameterStorageArchitecture(ParameterArchitectureConfig config) : config_(std::move(config)) {}

    /// Runs the original Architecture initialization sequence over provider metadata and compiler-spec XML.
    void initialize() {
        ghidra::AttributeId::initialize();
        ghidra::ElementId::initialize();
        ghidra::DocumentStorage store;
        init(store);
    }

    /// Emits provider architecture warnings into an in-memory diagnostic string.
    void printWarning(const ghidra::string& message) const override {
        warnings_ += message + '\n';
    }

protected:
    /// Builds the provider translator that owns processor-space and register metadata.
    ghidra::Translate* buildTranslator(ghidra::DocumentStorage&) override {
        return new ProviderStorageTranslate(config_);
    }

    /// Installs the empty image needed by the inherited initialization lifecycle.
    void buildLoader(ghidra::DocumentStorage&) override {
        loader = new EmptyLoadImage();
    }

    /// Installs the empty injection library used while parsing the reduced compiler specification.
    ghidra::PcodeInjectLibrary* buildPcodeInjectLibrary() override {
        return new EmptyInjectLibrary(this, translate->getUniqueStart(ghidra::Translate::INJECT));
    }

    /// Creates the native type factory before compiler-spec data organization is parsed.
    void buildTypegrp(ghidra::DocumentStorage&) override {
        types = new ghidra::TypeFactory(this);
    }

    /// Registers the primitive types required by grammar declarations and ParamList rules.
    void buildCoreTypes(ghidra::DocumentStorage&) override {
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
    }

    /// Creates the in-memory comment database required by the inherited architecture.
    void buildCommentDB(ghidra::DocumentStorage&) override {
        commentdb = new ghidra::CommentDatabaseInternal();
    }

    /// Creates the native Unicode string manager over the empty image.
    void buildStringManager(ghidra::DocumentStorage&) override {
        stringManager = new ghidra::StringManagerUnicode(this, 4096);
    }

    /// Creates the empty constant-pool implementation required by Architecture ownership.
    void buildConstantPool(ghidra::DocumentStorage&) override {
        cpool = new ghidra::ConstantPoolInternal();
    }

    /// Creates the in-memory context database required by compiler-spec initialization.
    void buildContext(ghidra::DocumentStorage&) override {
        context = new ghidra::ContextInternal();
    }

    /// Leaves symbols empty because parameter storage is driven by parsed declarations.
    void buildSymbols(ghidra::DocumentStorage&) override {}

    /// Loads only the processor-independent compiler-spec elements needed by this contract test.
    void buildSpecFile(ghidra::DocumentStorage& store) override;

    /// Adds the mandatory OTHER space before Architecture copies provider spaces and compiler globals.
    void modifySpaces(ghidra::Translate* processor) override {
        insertSpace(new ghidra::OtherSpace(this, processor, ghidra::OtherSpace::INDEX));
    }

    /// Gives the architecture a stable provider identifier.
    void resolveArchitecture() override {
        archid = config_.name;
    }

private:
    ParameterArchitectureConfig config_;
    mutable std::string warnings_;
};

/// Reads a compiler specification from the repository without making tests depend on installed Java state.
static std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream) {
        throw std::runtime_error("Unable to open authoritative compiler specification: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/// Extracts one XML element from a compiler specification while omitting unrelated fixup and processor data.
static std::string extract_xml_element(const std::string& source, std::string_view name) {
    const std::string opening = "<" + std::string(name);
    const std::size_t begin = source.find(opening);
    if (begin == std::string::npos) {
        throw std::runtime_error("Compiler specification is missing <" + std::string(name) + ">");
    }
    const std::size_t opening_end = source.find('>', begin);
    if (opening_end == std::string::npos) {
        throw std::runtime_error("Malformed compiler specification opening tag <" + std::string(name) + ">");
    }
    if (opening_end > begin && source[opening_end - 1] == '/') {
        return source.substr(begin, opening_end - begin + 1);
    }
    const std::string closing = "</" + std::string(name) + ">";
    const std::size_t end = source.find(closing, opening_end);
    if (end == std::string::npos) {
        throw std::runtime_error("Compiler specification is missing </" + std::string(name) + ">");
    }
    return source.substr(begin, end + closing.size() - begin);
}

/// Removes compiler-spec injection payloads that are unrelated to parameter classification.
static void remove_xml_elements(std::string& source, std::string_view name) {
    const std::string opening = "<" + std::string(name);
    const std::string closing = "</" + std::string(name) + ">";
    for (;;) {
        const std::size_t begin = source.find(opening);
        if (begin == std::string::npos) {
            return;
        }
        const std::size_t opening_end = source.find('>', begin);
        if (opening_end == std::string::npos) {
            throw std::runtime_error("Malformed compiler specification injection element");
        }
        std::size_t end = opening_end + 1;
        if (source[opening_end - 1] != '/') {
            const std::size_t closing_begin = source.find(closing, opening_end);
            if (closing_begin == std::string::npos) {
                throw std::runtime_error("Compiler specification injection element is not closed");
            }
            end = closing_begin + closing.size();
        }
        source.erase(begin, end - begin);
    }
}

/// Builds the XML document set consumed by Architecture::init from the original compiler-spec contract.
static std::string reduced_compiler_spec(const std::filesystem::path& path) {
    const std::string source = read_text_file(path);
    const std::vector<std::string_view> elements{"data_organization", "global", "stackpointer", "default_proto"};
    std::string result = "<compiler_spec>";
    for (const std::string_view element : elements) {
        if (element == "default_proto") {
            std::string prototype = extract_xml_element(source, element);
            remove_xml_elements(prototype, "pcode");
            result += prototype;
        } else {
            result += extract_xml_element(source, element);
        }
    }
    // PPC64 and AArch64 compiler specifications derive the return convention from
    // their ABI rather than declaring a return-address element. Preserve it only
    // for architectures whose authoritative source provides one.
    if (source.find("<returnaddress") != std::string::npos) {
        result += extract_xml_element(source, "returnaddress");
    }
    result += "</compiler_spec>";
    return result;
}

/// Parses the authoritative compiler specification and an empty processor-spec root into DocumentStorage.
void ParameterStorageArchitecture::buildSpecFile(ghidra::DocumentStorage& store) {
    std::istringstream processor_text("<processor_spec/>");
    ghidra::Document* processor = store.parseDocument(processor_text);
    store.registerTag(processor->getRoot());

    std::istringstream compiler_text(reduced_compiler_spec(config_.compiler_spec));
    ghidra::Document* compiler = store.parseDocument(compiler_text);
    store.registerTag(compiler->getRoot());
}

/// Owns one provider-backed Architecture and exposes the original parameter-storage workflow to tests.
class ParamStoreEnvironment {
public:
    /// Releases all native architectures after their compiler-spec models and type factories are destroyed.
    ~ParamStoreEnvironment() = default;

    /// Returns a named prototype model, constructing its architecture on first use.
    ghidra::ProtoModel* getModel(ParameterArchitecture architecture, const std::string& model_name) {
        ParameterStorageArchitecture& native = getArchitecture(architecture);
        ghidra::ProtoModel* model = native.getModel(model_name);
        if (model == nullptr) {
            throw std::runtime_error("Compiler specification did not define prototype model: " + model_name);
        }
        return model;
    }

    /// Parses a C declaration into the real native TypeFactory owned by a prototype model.
    void parseType(ghidra::ProtoModel* model, const std::string& definition) {
        std::istringstream stream(definition);
        ghidra::parse_C(model->getArch(), stream);
    }

    /// Parses a C prototype, invokes ProtoModel::assignParameterStorage, and compares every assigned piece.
    bool test(ghidra::ProtoModel* model, const std::string& signature, const std::string& stores) {
        std::istringstream stream(signature);
        ghidra::PrototypePieces pieces{};
        ghidra::parse_protopieces(pieces, stream, model->getArch());
        std::vector<ghidra::ParameterPieces> assigned;
        model->assignParameterStorage(pieces, assigned, false);
        std::vector<ghidra::VarnodeData> expected;
        parse_stores(model, expected, stores);
        if (assigned.size() != expected.size()) {
            return false;
        }
        for (std::size_t index = 0; index < assigned.size(); ++index) {
            if (!compare_piece(model, expected[index], assigned[index])) {
                return false;
            }
        }
        return true;
    }

    /// Assigns one declaration and returns the complete native parameter pieces for metadata assertions.
    std::vector<ghidra::ParameterPieces> assignment(ghidra::ProtoModel* model, const std::string& signature) {
        std::istringstream stream(signature);
        ghidra::PrototypePieces pieces{};
        ghidra::parse_protopieces(pieces, stream, model->getArch());
        std::vector<ghidra::ParameterPieces> assigned;
        model->assignParameterStorage(pieces, assigned, false);
        return assigned;
    }

private:
    /// Constructs and initializes the provider Architecture after registering native print capabilities.
    ParameterStorageArchitecture& getArchitecture(ParameterArchitecture architecture) {
        const auto iterator = architectures_.find(architecture);
        if (iterator != architectures_.end()) {
            return *iterator->second;
        }
        ghidra::forcePrintCLanguageRegistration();
        ghidra::CapabilityPoint::initializeAll();
        auto native = std::make_unique<ParameterStorageArchitecture>(architecture_config(architecture));
        native->initialize();
        ParameterStorageArchitecture* result = native.get();
        architectures_.emplace(architecture, std::move(native));
        return *result;
    }

    /// Parses one join expression and returns its unified JoinSpace address.
    static void parse_join(ghidra::ProtoModel* model, const std::string& join, ghidra::VarnodeData& result) {
        std::vector<ghidra::VarnodeData> pieces;
        std::size_t position = join.find(' ') + 1;
        for (;;) {
            const std::size_t next = join.find(' ', position);
            const std::string element =
                next == std::string::npos ? join.substr(position) : join.substr(position, next - position);
            pieces.emplace_back();
            parse_store(model, element, pieces.back());
            if (next == std::string::npos) {
                break;
            }
            position = next + 1;
        }
        const ghidra::int4 logical_size = pieces.size() == 1 ? 4 : 0;
        ghidra::JoinRecord* record = model->getArch()->findAddJoin(pieces, logical_size);
        result = record->getUnified();
    }

    /// Parses one expected register, stack, join, or void storage expression.
    static void parse_store(ghidra::ProtoModel* model, const std::string& name, ghidra::VarnodeData& result) {
        if (name == "void") {
            result = ghidra::VarnodeData{};
            return;
        }
        if (name.compare(0, 5, "stack", 5) == 0) {
            const std::size_t separator = name.find(':');
            std::istringstream offset_stream(name.substr(5, separator));
            result.space = model->getArch()->getStackSpace();
            offset_stream >> std::hex >> result.offset;
            result.size = 1;
            if (separator != std::string::npos) {
                std::istringstream size_stream(name.substr(separator + 1));
                size_stream >> std::dec >> result.size;
            }
            return;
        }
        if (name.compare(0, 4, "join", 4) == 0) {
            parse_join(model, name, result);
            return;
        }
        const std::size_t separator = name.find(':');
        std::string register_name = name;
        ghidra::int4 requested_size = 0;
        if (separator != std::string::npos) {
            std::istringstream size_stream(name.substr(separator + 1));
            size_stream >> std::dec >> requested_size;
            register_name = name.substr(0, separator);
        }
        result = model->getArch()->translate->getRegister(register_name);
        if (requested_size != 0) {
            if (result.space->isBigEndian()) {
                result.offset += result.size - requested_size;
            }
            result.size = requested_size;
        }
    }

    /// Parses the comma-separated expected storage list used by the original contract.
    static void parse_stores(ghidra::ProtoModel* model, std::vector<ghidra::VarnodeData>& result,
                             const std::string& names) {
        std::size_t position = 0;
        for (;;) {
            const std::size_t next = names.find(',', position);
            const std::string element =
                next == std::string::npos ? names.substr(position) : names.substr(position, next - position);
            result.emplace_back();
            parse_store(model, element, result.back());
            if (next == std::string::npos) {
                break;
            }
            position = next + 1;
        }
    }

    /// Compares one expected VarnodeData against one native ParameterPieces result.
    static bool compare_piece(ghidra::ProtoModel* model, const ghidra::VarnodeData& expected,
                              const ghidra::ParameterPieces& actual) {
        if (expected.space == nullptr) {
            return actual.type != nullptr && actual.type->getMetatype() == ghidra::TYPE_VOID;
        }
        if (expected.space != actual.addr.getSpace() || expected.offset != actual.addr.getOffset() ||
            actual.type == nullptr || expected.size != actual.type->getSize()) {
            return false;
        }
        if (expected.space->getType() != ghidra::IPTR_JOIN) {
            return true;
        }
        ghidra::JoinRecord* expected_record = model->getArch()->findJoin(expected.offset);
        ghidra::JoinRecord* actual_record = model->getArch()->findJoin(actual.addr.getOffset());
        if (expected_record == nullptr || actual_record == nullptr ||
            expected_record->numPieces() != actual_record->numPieces()) {
            return false;
        }
        for (ghidra::int4 index = 0; index < expected_record->numPieces(); ++index) {
            const ghidra::VarnodeData& expected_piece = expected_record->getPiece(index);
            const ghidra::VarnodeData& actual_piece = actual_record->getPiece(index);
            if (expected_piece.space != actual_piece.space || expected_piece.offset != actual_piece.offset ||
                expected_piece.size != actual_piece.size) {
                return false;
            }
        }
        return true;
    }

    std::map<ParameterArchitecture, std::unique_ptr<ParameterStorageArchitecture>> architectures_;
};

/// Returns the process-wide environment so each ABI's named types are parsed into one real native model.
static ParamStoreEnvironment& parameter_environment() {
    static ParamStoreEnvironment environment;
    return environment;
}

/// Converts a parameter-storage assertion into a diagnostic that identifies the exact native contract case.
static void expect_storage(ParamStoreEnvironment& environment, ghidra::ProtoModel* model, std::string signature,
                           std::string stores) {
    ASSERT_TRUE(environment.test(model, signature, stores))
        << "Native parameter storage mismatch for " << signature << "; expected " << stores;
}

/// Parses and registers one named structure before the following ABI assertion uses it.
static void expect_type(ParamStoreEnvironment& environment, ghidra::ProtoModel* model, std::string definition) {
    environment.parseType(model, definition);
}

/// Verifies the complete x86-64 System V input/output, aggregate, floating-point, and stack assignment contract.
TEST(NativeParamStore, X64) {
    // Coverage source: Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc, paramstore_x64.
    // The model and grammar are native; only processor register metadata is supplied through the provider fixture.
    ParamStoreEnvironment& environment = parameter_environment();
    ghidra::ProtoModel* model = environment.getModel(ParameterArchitecture::x64, "__stdcall");

    expect_storage(environment, model, "void func(int4,int4);", "void,EDI,ESI");
    expect_storage(environment, model, "void func(float4,float4);", "void,XMM0:4,XMM1:4");
    expect_storage(environment, model, "void func(int2 a,int4 b,int1 c);", "void,DI,ESI,DX:1");
    expect_storage(environment, model, "void func(int8,int8);", "void,RDI,RSI");
    expect_storage(environment, model, "void func(float8,float8);", "void,XMM0:8,XMM1:8");
    expect_storage(environment, model, "void func(int4,float4,int4,float4);", "void,EDI,XMM0:4,ESI,XMM1:4");
    expect_storage(environment, model, "void func(float4,int4,float4,int4);", "void,XMM0:4,EDI,XMM1:4,ESI");
    expect_storage(environment, model, "void func(int4,float8,float8,int4);", "void,EDI,XMM0:8,XMM1:8,ESI");
    expect_storage(environment, model, "void func(float8,int8,int8,float8);", "void,XMM0:8,RDI,RSI,XMM1:8");
    expect_storage(environment, model, "void func(float10);", "void,stack8:10");
    expect_storage(environment, model, "void func(float4,float10,float4);", "void,XMM0:4,stack8:10,XMM1:4");
    expect_type(environment, model, "struct intfloatpair { int4 a; float4 b;};");
    expect_storage(environment, model, "void func(intfloatpair);", "void,RDI");
    expect_type(environment, model, "struct longfloatpair { int8 a; float4 b;};");
    expect_storage(environment, model, "void func(int4,longfloatpair);", "void,EDI,join XMM0:8 RSI");
    expect_type(environment, model, "struct longdoublepair { int8 a; float8 b;};");
    expect_storage(environment, model, "void func(int4,longdoublepair);", "void,EDI,join XMM0:8 RSI");
    expect_type(environment, model, "struct intdoublepair { int4 a; float8 b;};");
    expect_storage(environment, model, "void func(int4,intdoublepair);", "void,EDI,join XMM0:8 RSI");
    expect_type(environment, model, "struct floatintpair { float4 a; int4 b;};");
    expect_storage(environment, model, "void func(int4,floatintpair);", "void,EDI,RSI");
    expect_type(environment, model, "struct doubleintpair { float8 a; int4 b;};");
    expect_storage(environment, model, "void func(int4,doubleintpair);", "void,EDI,join RSI XMM0:8");
    expect_type(environment, model, "struct intintfloat { int4 a; int4 b; float4 c; };");
    expect_storage(environment, model, "void func(int4,intintfloat);", "void,EDI,join XMM0:4 RSI");
    expect_type(environment, model, "struct intintfloatfloat { int4 a; int4 b; float4 c; float4 d;};");
    expect_storage(environment, model, "void func(int4,intintfloatfloat);", "void,EDI,join XMM0:8 RSI");
    expect_type(environment, model, "struct intfloatfloatint { int4 a; float4 b; float4 c; int4 d;};");
    expect_storage(environment, model, "void func(int4,intfloatfloatint);", "void,EDI,join RDX RSI");
    expect_type(environment, model, "struct intfloatfloat { int4 a; float4 b; float4 c; };");
    expect_storage(environment, model, "void func(int4,intfloatfloat);", "void,EDI,join XMM0:4 RSI");
    expect_type(environment, model, "struct floatfloatpair { float4 a; float4 b; }; ");
    expect_storage(environment, model, "void func(int4,floatfloatpair);", "void,EDI,XMM0:8");
    expect_type(environment, model, "struct doublefloatpair { float8 a; float4 b; }; ");
    expect_storage(environment, model, "void func(int4,doublefloatpair);", "void,EDI,join XMM1:8 XMM0:8");
    expect_type(environment, model, "struct floatfloatfloat { float4 a; float4 b; float4 c; }; ");
    expect_storage(environment, model, "void func(floatfloatfloat,int8);", "void,join XMM1:4 XMM0:8,RDI");
    expect_type(environment, model, "struct intintintint { int4 a; int4 b; int4 c; int4 d; }; ");
    expect_storage(environment, model, "void func(intintintint);", "void,join RSI RDI");
    expect_storage(environment, model, "void func(int4,intintintint);", "void,EDI,join RDX RSI");
    expect_type(environment, model, "struct intintintintint { int4 a; int4 b; int4 c; int4 d; int4 e;};");
    expect_storage(environment, model, "void func(intintintintint);", "void,stack8:20");
    expect_storage(environment, model,
                   "void func(float4,float4,float4,float4,float4,float4,float4,float4,longfloatpair);",
                   "void,XMM0:4,XMM1:4,XMM2:4,XMM3:4,XMM4:4,XMM5:4,XMM6:4,XMM7:4,stack8:16");
    expect_storage(environment, model, "void func(xunknown4,xunknown8);", "void,EDI,RSI");
    expect_storage(environment, model, "intintintint func(void);", "join RDX RAX");
    expect_storage(environment, model, "floatintpair func(void);", "RAX");
    expect_storage(environment, model, "longfloatpair func(void);", "join XMM0:8 RAX");
    expect_storage(environment, model, "longdoublepair func(void);", "join XMM0:8 RAX");
    expect_storage(environment, model, "doubleintpair func(void);", "join RAX XMM0:8");
    expect_storage(environment, model, "floatfloatfloat func(void);", "join XMM1:4 XMM0:8");
    expect_type(environment, model, "struct doubledoublepair { float8 a; float8 b; }; ");
    expect_storage(environment, model, "doubledoublepair func(void);", "join XMM1:8 XMM0:8");
    expect_storage(environment, model, "floatfloatpair func(void);", "XMM0:8");
    expect_storage(environment, model, "intintintintint func(void);", "RAX,RDI");
    expect_type(environment, model, "struct doubleintintint { float8 a; int4 b; int4 c; int4 d; }; ");
    expect_storage(environment, model, "doubleintintint func(void);", "RAX,RDI");
}

/// Verifies big-endian PowerPC64 floating/general register allocation and stack spill boundaries.
TEST(NativeParamStore, Ppc64BigEndian) {
    // Coverage source: Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc,
    // paramstore_ppc64be_stdcall. The compiler spec is authoritative; endian-aware subregister comparison remains an
    // assertion, not a skip.
    ParamStoreEnvironment& environment = parameter_environment();
    ghidra::ProtoModel* model = environment.getModel(ParameterArchitecture::ppc64_be, "__stdcall");

    expect_storage(environment, model, "void func(int4 a,float4 b,float8 c);", "void,r3:4,join f1,f2");
    expect_storage(environment, model, "void func(float8 a,int8 b,float8 c);", "void,f1,r4,f2");
    expect_type(environment, model, "struct sparm { int4 a; float8 dd; }; ");
    expect_storage(environment, model,
                   "void func(int4 c,float8 ff,int4 d,float16 ld,sparm s,float8 gg,sparm t,int4 e,float8 hh);",
                   "void,r3:4,f1,r5:4,join f2 f3,join r8 r9,f4,stack70:16,stack84:4,f5");
}

/// Verifies big-endian MIPS32 floating-pair joins, odd-size justification, and aggregate return storage.
TEST(NativeParamStore, Mips32BigEndian) {
    // Coverage source: Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc,
    // paramstore_mips32be_stdcall. MIPS is supported through provider aliases for paired FPRs and the real
    // mips32be.cspec model.
    ParamStoreEnvironment& environment = parameter_environment();
    ghidra::ProtoModel* model = environment.getModel(ParameterArchitecture::mips32_be, "__stdcall");

    expect_storage(environment, model, "void func(int2 a,int4 b,char c);", "void,a0:2,a1,a2:1");
    expect_storage(environment, model, "void func(float8 a,float8 b);", "void,f12_13,f14_15");
    expect_storage(environment, model, "void func(float4 a,float4 b);", "void,f12,f14");
    expect_storage(environment, model, "void func(float4 a,float8 b);", "void,f12,f14_15");
    expect_storage(environment, model, "void func(float8 a,float4 b);", "void,f12_13,f14");
    expect_storage(environment, model, "void func(int4 a,int4 b,int4 c,int4 d);", "void,a0,a1,a2,a3");
    expect_storage(environment, model, "void func(float8 a,int4 b,float8 c);", "void,f12_13,a2,stack10:8");
    expect_storage(environment, model, "void func(float8 a,int4 b,int4 c);", "void,f12_13,a2,a3");
    expect_storage(environment, model, "void func(float4 a,int4 b,int4 c);", "void,f12,a1,a2");
    expect_storage(environment, model, "void func(int4 a,int4 b,int4 c,float8 d);", "void,a0,a1,a2,stack10:8");
    expect_storage(environment, model, "void func(int4 a,int4 b,int4 c,float4 d);", "void,a0,a1,a2,a3");
    expect_storage(environment, model, "void func(int4 a,int4 b,float8 c);", "void,a0,a1,join a2 a3");
    expect_storage(environment, model, "void func(int4 a,float8 b);", "void,a0,join a2 a3");
    expect_storage(environment, model, "void func(float4 a,float4 b,float4 c,float4 d);", "void,f12,f14,a2,a3");
    expect_storage(environment, model, "void func(float4 a,int4 b,float4 c,int4 d);", "void,f12,a1,a2,a3");
    expect_storage(environment, model, "void func(float8 a,float4 b,float4 c);", "void,f12_13,f14,a3");
    expect_storage(environment, model, "void func(float4 a,float4 b,float8 c);", "void,f12,f14,join a2 a3");
    expect_storage(environment, model, "void func(int4 a,float4 b,int4 c,float4 d);", "void,a0,a1,a2,a3");
    expect_storage(environment, model, "void func(int4 a,float4 b,int4 c,int4 d);", "void,a0,a1,a2,a3");
    expect_storage(environment, model, "void func(int4 a,int4 b,float4 c,int4 d);", "void,a0,a1,a2,a3");
    expect_storage(environment, model, "int4 func(void);", "v0");
    expect_storage(environment, model, "float4 func(void);", "f0");
    expect_storage(environment, model, "float8 func(void);", "f0_1");
    expect_type(environment, model, "struct onefieldstruct { int4 a; }; ");
    expect_type(environment, model, "struct twofieldstruct { int4 a; int4 b; }; ");
    expect_storage(environment, model, "onefieldstruct func(int4 a);", "v0,a0,a1");
    expect_storage(environment, model, "twofieldstruct func(int4 a);", "v0,a0,a1");
    expect_storage(environment, model, "void func(twofieldstruct a);", "void,join a0 a1");
    expect_type(environment, model, "struct intdouble { int4 a; float8 b; }; ");
    expect_storage(environment, model, "void func(intdouble a);", "void,join a0 a1 a2 a3");
}

/// Verifies AArch64 scalar/vector classification, stack fallback, hidden returns, and homogeneous aggregates.
TEST(NativeParamStore, Aarch64Cdecl) {
    // Coverage source: Ghidra/Features/Decompiler/src/decompile/unittests/testparamstore.cc, paramstore_aarch64_cdecl.
    // The little-endian AArch64 cases are retained in full because the exported grammar and model APIs support them.
    ParamStoreEnvironment& environment = parameter_environment();
    ghidra::ProtoModel* model = environment.getModel(ParameterArchitecture::aarch64, "__cdecl");

    expect_storage(environment, model, "void func(int2 a,int4 b,int1 c);", "void,w0:2,w1,w2:1");
    expect_storage(environment, model, "void func(int4, int4);", "void,w0,w1");
    expect_storage(environment, model, "void func(int8,int8);", "void,x0,x1");
    expect_storage(environment, model, "void func(float4,float4);", "void,s0,s1");
    expect_storage(environment, model, "void func(float8,float8);", "void,d0,d1");
    expect_storage(environment, model, "void func(int4,float4,int4,float4);", "void,w0,s0,w1,s1");
    expect_storage(environment, model, "void func(float4,int4,float4,int4);", "void,s0,w0,s1,w1");
    expect_storage(environment, model, "void func(int4,float8,float8,int4);", "void,w0,d0,d1,w1");
    expect_storage(environment, model, "void func(float8,int8,int8,float8);", "void,d0,x0,x1,d1");
    expect_storage(environment, model, "void func(float16);", "void,q0");
    expect_storage(environment, model, "void func(float4,float16);", "void,s0,q1");
    expect_storage(environment, model, "void func(int4,int4,int4,int4,int4,int4,int4,int4,int4,int4);",
                   "void,w0,w1,w2,w3,w4,w5,w6,w7,stack0:4,stack8:4");
    expect_storage(environment, model,
                   "void func(float4,float4,float4,float4,float4,float4,float4,float4,float4,float4);",
                   "void,s0,s1,s2,s3,s4,s5,s6,s7,stack0:4,stack8:4");
    expect_storage(environment, model, "void func(float4,float4,float4,float4,float4,float4,float4,float4,float16);",
                   "void,s0,s1,s2,s3,s4,s5,s6,s7,stack0:16");
    expect_storage(environment, model,
                   "void func(float4,float4,float4,float4,float4,float4,float4,float4,float4,float16);",
                   "void,s0,s1,s2,s3,s4,s5,s6,s7,stack0:4,stack10:16");
    expect_storage(environment, model, "void func(int4,int4,int4,int4,int4,int4,int4,int4,int4,float4);",
                   "void,w0,w1,w2,w3,w4,w5,w6,w7,stack0:4,s0");
    expect_storage(environment, model,
                   "void func(float4,float4,float4,float4,float4,float4,float4,float4,float4,int4);",
                   "void,s0,s1,s2,s3,s4,s5,s6,s7,stack0:4,w0");
    expect_type(environment, model, "struct intpair { int4 a; int4 b;};");
    expect_storage(environment, model, "void func(intpair);", "void,x0");
    expect_type(environment, model, "struct longpair { int8 a; int8 b; }; ");
    expect_storage(environment, model, "void func(longpair);", "void,join x1 x0");
    expect_type(environment, model, "struct longquad { int8 a; int8 b; int8 c; int8 d; }; ");
    expect_storage(environment, model, "void func(longquad);", "void,x0");
    expect_type(environment, model, "struct floatdouble { float4 a; float8 b; }; ");
    expect_storage(environment, model, "void func(floatdouble);", "void,join x1 x0");
    expect_type(environment, model, "struct intfloat { int4 a; float4 b; }; ");
    expect_storage(environment, model, "void func(intfloat);", "void,x0");
    expect_type(environment, model, "struct longdoublestruct { int8 a; float8 b; }; ");
    expect_storage(environment, model, "void func(longdoublestruct);", "void,join x1 x0");
    expect_storage(environment, model, "int4 func(void);", "w0");
    expect_storage(environment, model, "float4 func(void);", "s0");
    expect_storage(environment, model, "float8 func(void);", "d0");
    expect_storage(environment, model, "intpair func(void);", "x0");
    expect_storage(environment, model, "longpair func(void);", "join x1 x0");
    expect_storage(environment, model, "longquad func(void);", "void,x8");
    expect_type(environment, model, "struct floatpair { float4 a; float4 b; }; ");
    expect_storage(environment, model, "void func(floatpair);", "void,join s1 s0");
    expect_type(environment, model, "struct floatpairpair { floatpair a; floatpair b; }; ");
    expect_storage(environment, model, "void func(floatpairpair);", "void,join s3 s2 s1 s0");
    expect_type(environment, model, "struct doublequad { float8 a; float8 b; float8 c; float8 d; }; ");
    expect_storage(environment, model, "void func(doublequad);", "void,join d3 d2 d1 d0");
}

/// Verifies compiler-spec metadata that is not visible in the storage-only rows: stack roots and extrapop.
TEST(NativeParamStore, CompilerSpecMetadata) {
    // These values come directly from the authoritative compiler specifications loaded by the fixture.
    ParamStoreEnvironment& environment = parameter_environment();
    struct MetadataCase {
        ParameterArchitecture architecture;
        const char* model_name;
        const char* stack_register;
        std::uint64_t stack_offset;
        ghidra::int4 stack_size;
        ghidra::int4 extrapop;
    };
    const std::array cases{
        MetadataCase{ParameterArchitecture::x64, "__stdcall", "RSP", 0x20, 8, 8},
        MetadataCase{ParameterArchitecture::ppc64_be, "__stdcall", "r1", 0x108, 8, 0},
        MetadataCase{ParameterArchitecture::mips32_be, "__stdcall", "sp", 0x2c, 4, 0},
        MetadataCase{ParameterArchitecture::aarch64, "__cdecl", "sp", 0xf8, 8, 0},
    };
    for (const MetadataCase& test_case : cases) {
        SCOPED_TRACE(test_case.model_name);
        ghidra::ProtoModel* model = environment.getModel(test_case.architecture, test_case.model_name);
        ASSERT_NE(model, nullptr);
        EXPECT_EQ(model->getExtraPop(), test_case.extrapop);
        EXPECT_EQ(model->getArch()->getStackSpace()->getName(), "stack");
        const ghidra::VarnodeData& stack = model->getArch()->translate->getRegister(test_case.stack_register);
        EXPECT_EQ(stack.offset, test_case.stack_offset);
        EXPECT_EQ(stack.size, static_cast<ghidra::uint4>(test_case.stack_size));
    }
}

/// Verifies the zero-flag contract on a direct return exposed by the native parameter fixture.
TEST(NativeParamStore, ParameterMetadata) {
    ParamStoreEnvironment& environment = parameter_environment();
    ghidra::ProtoModel* model = environment.getModel(ParameterArchitecture::x64, "__stdcall");
    const std::vector<ghidra::ParameterPieces> normal = environment.assignment(model, "int4 func(void);");
    ASSERT_EQ(normal.size(), 1U);
    EXPECT_EQ(normal[0].flags, 0U);
}

} // namespace newghidra::decompiler::tests
