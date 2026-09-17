module;

#include <gtest/gtest.h>

export module native_funcproto_tests;

import recode.decompiler;
import recode.decompiler.grammar;
import std;

// Coverage reference: Ghidra/Features/Decompiler/src/decompile/unittests/testfuncproto.cc.
// This module deliberately uses the native ProtoModel, ParamActive, XML model
// decoding, and C declaration grammar instead of reproducing the legacy test
// harness or going through the provider frontend.

namespace ghidra::native_funcproto_tests {

/// Identifies the storage class expected for one parameter or trial.
enum class StorageKind {
    invalid,
    reg,
    stack,
    join,
};

/// Describes one expected concrete constituent of a joined storage location.
struct StorageConstituentExpectation {
    StorageKind kind = StorageKind::invalid;
    std::string register_name;
    uintb offset = 0;
    int4 size = 0;
};

/// Describes one expected concrete storage location in a table-driven case.
struct StorageExpectation {
    StorageKind kind = StorageKind::invalid;
    std::string register_name;
    uintb offset = 0;
    int4 size = 0;
    std::vector<StorageConstituentExpectation> constituents;
};

/// Describes the type and storage contract for one assigned prototype piece.
struct ParameterExpectation {
    StorageExpectation storage;
    std::string type_name;
    std::optional<type_metatype> metatype;
    std::string pointed_to_name;
    std::optional<int4> same_type_as;
    uint4 required_flags = 0;
};

/// Describes one complete parameter-storage assignment scenario from testfuncproto.cc.
struct AssignmentCase {
    std::string name;
    std::string model_name;
    std::string declaration;
    std::vector<ParameterExpectation> expected;
};

/// Describes the state and, when useful, address of one recovered parameter trial.
struct TrialExpectation {
    std::optional<StorageExpectation> storage;
    std::optional<bool> used;
    std::optional<bool> unreferenced;
};

/// Describes one complete parameter-recovery scenario from testfuncproto.cc.
struct RecoveryCase {
    std::string name;
    std::string model_name;
    std::vector<StorageExpectation> active_storage;
    std::vector<TrialExpectation> expected;
};

/// Provides the minimal empty load image required by Architecture::init.
class EmptyLoadImage final : public LoadImage {
public:
    /// Constructs an image that contains no bytes or symbols.
    EmptyLoadImage() : LoadImage("native-funcproto-test-image") {}

    /// Rejects byte reads because these prototype tests do not decode instructions.
    void loadFill(uint1*, int4, const Address&) override {
        throw DataUnavailError("The function-prototype test image contains no bytes");
    }

    /// Identifies the synthetic image to native diagnostics.
    string getArchType() const override {
        return "native-funcproto-test";
    }

    /// Leaves synthetic image addresses unchanged.
    void adjustVma(long) override {}
};

/// Provides an empty injection library so Architecture can initialize normally.
class EmptyInjectLibrary final : public PcodeInjectLibrary {
public:
    /// Constructs the empty library with the architecture's unique temporary base.
    EmptyInjectLibrary(Architecture* architecture, uint4 temporary_base)
        : PcodeInjectLibrary(architecture, temporary_base) {}

    /// Rejects injection allocation because no test model supplies payloads.
    int4 allocateInject(const string&, const string&, int4) override {
        throw LowlevelError("Native function-prototype tests do not define injections");
    }

    /// Performs no registration for the empty payload set.
    void registerInject(int4) override {}

    /// Rejects manually supplied call-fixup text in this isolated fixture.
    int4 manualCallFixup(const string&, const string&) override {
        throw LowlevelError("Native function-prototype tests do not define call fixups");
    }

    /// Rejects manually supplied call-other-fixup text in this isolated fixture.
    int4 manualCallOtherFixup(const string&, const string&, const vector<string>&, const string&) override {
        throw LowlevelError("Native function-prototype tests do not define call-other fixups");
    }

    /// Returns the reusable context required by the injection interface.
    InjectContext& getCachedContext() override {
        return context_;
    }

    /// Returns the empty behavior table because no payload can be executed.
    const vector<OpBehavior*>& getBehaviors() override {
        return behaviors_;
    }

private:
    /// Encodes no state because the test library never materializes an injection.
    class EmptyContext final : public InjectContext {
    public:
        /// Emits no XML state for the empty context.
        void encode(Encoder&) const override {}
    };

    EmptyContext context_;
    vector<OpBehavior*> behaviors_;
};

/// Implements the native Translate boundary for the Toy-like register file used by the original tests.
class ToyTestTranslate final : public Translate {
public:
    /// Constructs deterministic RAM, register, and unique spaces and all named Toy registers.
    ToyTestTranslate() {
        setBigEndian(false);
        setUniqueBase(0x1000);
        insertSpace(new ConstantSpace(this, this));
        // Architecture::init copies this index-1 space before addOtherSpace() adds its global range.
        insertSpace(new OtherSpace(this, this, OtherSpace::INDEX));
        insertSpace(new AddrSpace(this, this, IPTR_PROCESSOR, "ram", false, 4, 1, 2, AddrSpace::hasphysical, 0, 0));
        insertSpace(
            new AddrSpace(this, this, IPTR_PROCESSOR, "register", false, 4, 1, 3, AddrSpace::hasphysical, 0, 0));
        insertSpace(new UniqueSpace(this, this, numSpaces(), AddrSpace::hasphysical));
        setDefaultCodeSpace(getSpaceByName("ram")->getIndex());
        setDefaultDataSpace(getSpaceByName("ram")->getIndex());

        addRegister("r1", 0, 4);
        addRegister("r2", 4, 4);
        addRegister("r3", 8, 4);
        addRegister("r4", 12, 4);
        addRegister("r5", 16, 4);
        addRegister("r8", 28, 4);
        addRegister("r9", 32, 4);
        addRegister("r10", 36, 4);
        addRegister("r11", 40, 4);
        addRegister("r12", 44, 4);
        addRegister("r12l", 44, 2);
        addRegister("r10l", 36, 2);
        addRegister("sp", 48, 4);
    }

    /// Accepts the empty XML initialization used by the architecture fixture.
    void initialize(DocumentStorage&) override {}

    /// Returns the exact native location for a named Toy register.
    const VarnodeData& getRegister(const string& name) const override {
        const auto iterator = registers_.find(name);
        if (iterator == registers_.end()) {
            throw LowlevelError("Unknown Toy test register: " + name);
        }
        return iterator->second;
    }

    /// Returns the smallest named register containing the requested range.
    string getRegisterName(AddrSpace* space, uintb offset, int4 size) const override {
        if (size <= 0 || offset > std::numeric_limits<uintb>::max() - static_cast<uintb>(size)) {
            return {};
        }
        string result;
        for (const auto& [name, location] : registers_) {
            if (location.space != space || location.offset > offset ||
                location.offset + location.size < offset + static_cast<uintb>(size)) {
                continue;
            }
            if (result.empty() || location.size < getRegister(result).size) {
                result = name;
            }
        }
        return result;
    }

    /// Returns a register name only when the location and size match exactly.
    string getExactRegisterName(AddrSpace* space, uintb offset, int4 size) const override {
        for (const auto& [name, location] : registers_) {
            if (location.space == space && location.offset == offset && location.size == size) {
                return name;
            }
        }
        return {};
    }

    /// Copies every named Toy register into the native location-to-name map.
    void getAllRegisters(map<VarnodeData, string>& result) const override {
        for (const auto& [name, location] : registers_) {
            result.emplace(location, name);
        }
    }

    /// Supplies no processor-specific user operations.
    void getUserOpNames(vector<string>& result) const override {
        result.clear();
    }

    /// Rejects instruction-length queries because this fixture tests prototypes only.
    int4 instructionLength(const Address&) const override {
        throw UnimplError("The Toy prototype fixture does not decode instructions", 0);
    }

    /// Rejects p-code translation because this fixture tests prototypes only.
    int4 oneInstruction(PcodeEmit&, const Address&) const override {
        throw UnimplError("The Toy prototype fixture does not translate instructions", 0);
    }

    /// Rejects assembly printing because this fixture tests prototypes only.
    int4 printAssembly(AssemblyEmit&, const Address&) const override {
        throw UnimplError("The Toy prototype fixture does not print instructions", 0);
    }

private:
    /// Adds one named register to the deterministic register file.
    void addRegister(const string& name, uintb offset, int4 size) {
        registers_.emplace(name, VarnodeData{getSpaceByName("register"), offset, static_cast<uint4>(size)});
    }

    map<string, VarnodeData> registers_;
};

/// Owns a fully initialized native Architecture configured only from in-test XML.
class ToyTestArchitecture final : public Architecture {
public:
    /// Builds the native architecture and decodes all three original test models.
    ToyTestArchitecture() {
        forcePrintCLanguageRegistration();
        AttributeId::initialize();
        ElementId::initialize();
        DocumentStorage storage;
        istringstream processor_config("<processor_spec/>");
        Document* processor_document = storage.parseDocument(processor_config);
        storage.registerTag(processor_document->getRoot());
        istringstream specification(modelSpecification());
        Document* document = storage.parseDocument(specification);
        storage.registerTag(document->getRoot());
        init(storage);
    }

    /// Reports the synthetic architecture name in diagnostics.
    string getDescription() const override {
        return "Toy:LE:32:native-funcproto-tests";
    }

    /// Captures warnings without requiring a process-wide diagnostic stream.
    void printWarning(const string& message) const override {
        warnings_ << message << '\n';
    }

protected:
    /// Creates the deterministic native translator used by the compiler-spec decoder.
    Translate* buildTranslator(DocumentStorage&) override {
        return new ToyTestTranslate();
    }

    /// Supplies the empty image required by Architecture initialization.
    void buildLoader(DocumentStorage&) override {
        loader = new EmptyLoadImage();
    }

    /// Supplies the empty injection library required by Architecture initialization.
    PcodeInjectLibrary* buildPcodeInjectLibrary() override {
        return new EmptyInjectLibrary(this, translate->getUniqueStart(Translate::INJECT));
    }

    /// Creates the native type factory before compiler-spec parsing.
    void buildTypegrp(DocumentStorage&) override {
        types = new TypeFactory(this);
    }

    /// Registers the primitive types used by grammar parsing and pointer construction.
    void buildCoreTypes(DocumentStorage&) override {
        types->setCoreType("void", 1, TYPE_VOID, false);
        types->setCoreType("bool", 1, TYPE_BOOL, false);
        types->setCoreType("uint1", 1, TYPE_UINT, false);
        types->setCoreType("uint2", 2, TYPE_UINT, false);
        types->setCoreType("uint4", 4, TYPE_UINT, false);
        types->setCoreType("uint8", 8, TYPE_UINT, false);
        types->setCoreType("int1", 1, TYPE_INT, false);
        types->setCoreType("int2", 2, TYPE_INT, false);
        types->setCoreType("int4", 4, TYPE_INT, false);
        types->setCoreType("int8", 8, TYPE_INT, false);
        types->setCoreType("float4", 4, TYPE_FLOAT, false);
        types->setCoreType("float8", 8, TYPE_FLOAT, false);
        types->setCoreType("float10", 10, TYPE_FLOAT, false);
        types->setCoreType("float16", 16, TYPE_FLOAT, false);
        types->setCoreType("xunknown1", 1, TYPE_UNKNOWN, false);
        types->setCoreType("xunknown2", 2, TYPE_UNKNOWN, false);
        types->setCoreType("xunknown4", 4, TYPE_UNKNOWN, false);
        types->setCoreType("xunknown8", 8, TYPE_UNKNOWN, false);
        types->setCoreType("code", 1, TYPE_CODE, false);
        types->setCoreType("char", 1, TYPE_INT, true);
        types->setCoreType("wchar2", 2, TYPE_INT, true);
        types->setCoreType("wchar4", 4, TYPE_INT, true);
        types->cacheCoreTypes();
    }

    /// Creates the context database even though no instruction context is queried.
    void buildContext(DocumentStorage&) override {
        context = new ContextInternal();
    }

    /// Creates the comment database required by the Architecture owner.
    void buildCommentDB(DocumentStorage&) override {
        commentdb = new CommentDatabaseInternal();
    }

    /// Creates the in-memory string manager required by the Architecture owner.
    void buildStringManager(DocumentStorage&) override {
        stringManager = new StringManagerUnicode(this, 4096);
    }

    /// Creates the empty constant pool required by the Architecture owner.
    void buildConstantPool(DocumentStorage&) override {
        cpool = new ConstantPoolInternal();
    }

    /// Leaves symbols absent because all test inputs are compiler declarations.
    void buildSymbols(DocumentStorage&) override {}

    /// Leaves processor XML absent because Toy spaces are created by ToyTestTranslate.
    void buildSpecFile(DocumentStorage&) override {}

    /// Leaves translator spaces unchanged after construction.
    void modifySpaces(Translate*) override {}

    /// Selects the synthetic architecture identity.
    void resolveArchitecture() override {
        archid = "Toy:LE:32:native-funcproto-tests";
    }

private:
    /// Returns the compiler-spec XML containing the exact three model layouts from testfuncproto.cc.
    static string modelSpecification() {
        return R"xml(<compiler_spec>
  <stackpointer register="sp" space="ram"/>
  <default_proto>
    <prototype name="__model1" extrapop="unknown" stackshift="4">
      <input pointermax="4">
        <pentry minsize="1" maxsize="4"><register name="r12"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r11"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r10"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r9"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r8"/></pentry>
        <pentry minsize="1" maxsize="500" align="4"><addr offset="0" space="stack"/></pentry>
      </input>
      <output><pentry minsize="1" maxsize="4"><register name="r12"/></pentry></output>
    </prototype>
  </default_proto>
  <prototype name="__model2" extrapop="unknown" stackshift="4">
    <input>
      <pentry minsize="1" maxsize="4" metatype="ptr"><register name="r1"/></pentry>
      <pentry minsize="1" maxsize="4" metatype="ptr"><register name="r2"/></pentry>
      <pentry minsize="1" maxsize="4" metatype="float" extension="float"><register name="r3"/></pentry>
      <pentry minsize="1" maxsize="4" metatype="float" extension="float"><register name="r4"/></pentry>
      <pentry minsize="1" maxsize="4" metatype="float" extension="float"><register name="r5"/></pentry>
      <pentry minsize="1" maxsize="4"><register name="r10"/></pentry>
      <pentry minsize="1" maxsize="4"><register name="r9"/></pentry>
      <pentry minsize="1" maxsize="4"><register name="r8"/></pentry>
      <pentry minsize="5" maxsize="8"><addr space="join" piece1="r10" piece2="r9"/></pentry>
      <pentry minsize="1" maxsize="500" align="4"><addr offset="0" space="stack"/></pentry>
    </input>
    <output><pentry minsize="1" maxsize="4"><register name="r12"/></pentry></output>
  </prototype>
  <prototype name="__model3" extrapop="unknown" stackshift="4">
    <input>
      <group>
        <pentry minsize="1" maxsize="4" metatype="float" extension="float"><register name="r3"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r10"/></pentry>
      </group>
      <group>
        <pentry minsize="1" maxsize="4" metatype="float" extension="float"><register name="r4"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r9"/></pentry>
      </group>
      <group>
        <pentry minsize="1" maxsize="4" metatype="float" extension="float"><register name="r5"/></pentry>
        <pentry minsize="1" maxsize="4"><register name="r8"/></pentry>
      </group>
      <pentry minsize="1" maxsize="500" align="4"><addr offset="0" space="stack"/></pentry>
    </input>
    <output><pentry minsize="1" maxsize="4"><register name="r12"/></pentry></output>
  </prototype>
</compiler_spec>)xml";
    }

    mutable ostringstream warnings_;
};

/// Returns the process-local architecture shared by all table-driven cases.
static ToyTestArchitecture& testArchitecture() {
    /// Initializes capability singletons before the Architecture base constructor requests a printer.
    static const bool capabilities_initialized = [] {
        forcePrintCLanguageRegistration();
        CapabilityPoint::initializeAll();
        return true;
    }();
    (void)capabilities_initialized;
    static ToyTestArchitecture architecture;
    return architecture;
}

/// Constructs the shared architecture while converting native errors into a readable GTest failure.
static ToyTestArchitecture* testArchitectureOrNull() {
    try {
        return &testArchitecture();
    } catch (const LowlevelError& error) {
        ADD_FAILURE() << error.explain;
    } catch (const std::exception& error) {
        ADD_FAILURE() << error.what();
    } catch (...) {
        ADD_FAILURE() << "The native Toy architecture threw an unknown exception";
    }
    return nullptr;
}

/// Builds a register storage expectation while keeping table rows compact.
static StorageExpectation reg(const string& name, int4 size = 4) {
    return StorageExpectation{StorageKind::reg, name, 0, size, {}};
}

/// Builds a stack storage expectation for a byte offset and size.
static StorageExpectation stack(uintb offset, int4 size) {
    return StorageExpectation{StorageKind::stack, {}, offset, size, {}};
}

/// Builds an expectation for a logical join-space parameter.
static StorageExpectation join(std::initializer_list<StorageConstituentExpectation> constituents) {
    StorageExpectation result{StorageKind::join, {}, 0, 0, constituents};
    for (const StorageConstituentExpectation& constituent : result.constituents) {
        result.size += constituent.size;
    }
    return result;
}

/// Builds a register constituent expectation for a joined storage record.
static StorageConstituentExpectation joinReg(const string& name, int4 size = 4) {
    return StorageConstituentExpectation{StorageKind::reg, name, 0, size};
}

/// Builds an invalid-storage expectation used for a void return placeholder.
static StorageExpectation invalid() {
    return StorageExpectation{StorageKind::invalid, {}, 0, 0, {}};
}

/// Builds a scalar type expectation with an optional exact type identity constraint.
static ParameterExpectation scalar(StorageExpectation storage, string name,
                                   std::optional<int4> same_type_as = std::nullopt, uint4 required_flags = 0) {
    return ParameterExpectation{std::move(storage), std::move(name), std::nullopt, {}, same_type_as, required_flags};
}

/// Builds a metatype expectation for a parameter whose native type may have a different spelling.
static ParameterExpectation meta(StorageExpectation storage, type_metatype metatype, string name = {},
                                 string pointed_to_name = {}, uint4 required_flags = 0) {
    return ParameterExpectation{std::move(storage),         std::move(name), metatype,
                                std::move(pointed_to_name), std::nullopt,    required_flags};
}

/// Returns every storage-assignment scenario preserved from testfuncproto.cc lines 217-457.
static vector<AssignmentCase> assignmentCases() {
    return {{"register",
             "__model1",
             "void func(int4 a,int4 b);",
             {scalar(invalid(), "void"), scalar(reg("r12"), "int4"), scalar(reg("r11"), "int4")}},
            {"smallregister",
             "__model1",
             "int4 func(char a,int4 b,int2 c,int4 d);",
             {scalar(reg("r12"), "int4"), scalar(reg("r12", 1), "char"), scalar(reg("r11"), "int4"),
              scalar(reg("r10", 2), "int2"), scalar(reg("r9"), "int4")}},
            {"stackalign",
             "__model1",
             "int4 func(int4 a,int4 b,int4 c,int4 d,int4 e,int2 f,int1 *g);",
             {scalar(reg("r12"), "int4"), scalar(reg("r12"), "int4", 0), scalar(reg("r11"), "int4", 0),
              scalar(reg("r10"), "int4", 0), scalar(reg("r9"), "int4", 0), scalar(reg("r8"), "int4", 0),
              scalar(stack(0, 2), "int2"), meta(stack(4, 4), TYPE_PTR)}},
            {"pointeroverflow",
             "__model1",
             "int2 func(int4 a,int8 b,int4 c);",
             {scalar(reg("r12", 2), "int2"), scalar(reg("r12"), "int4"),
              meta(reg("r11"), TYPE_PTR, {}, "int8", ParameterPieces::indirectstorage), scalar(reg("r10"), "int4")}},
            {"stackoverflow",
             "__model2",
             "char func(int4 a,int8 b,int4 c);",
             {scalar(reg("r12", 1), "char"), scalar(reg("r10"), "int4"), scalar(stack(0, 8), "int8"),
              scalar(reg("r9"), "int4")}},
            {"floatreg",
             "__model2",
             "void func(int4 a,float4 b,float4 c,int4 d,float4 d);",
             {scalar(invalid(), "void"), scalar(reg("r10"), "int4"), scalar(reg("r3"), "float4"),
              scalar(reg("r4"), "float4"), scalar(reg("r9"), "int4"), scalar(reg("r5"), "float4")}},
            {"floattogeneric",
             "__model2",
             "float4 func(int4 a,float4 b,float4 c,float4 d,float4 e,float4 f);",
             {scalar(reg("r12"), "float4"), scalar(reg("r10"), "int4"), scalar(reg("r3"), "float4"),
              scalar(reg("r4"), "float4"), scalar(reg("r5"), "float4"), scalar(reg("r9"), "float4"),
              scalar(reg("r8"), "float4")}},
            {"grouped",
             "__model3",
             "float4 func(int4 a,float4 b,float4 c,int4 d,float4 e);",
             {scalar(reg("r12"), "float4"), scalar(reg("r10"), "int4"), scalar(reg("r4"), "float4"),
              scalar(reg("r5"), "float4"), scalar(stack(0, 4), "int4"), scalar(stack(4, 4), "float4")}},
            {"join",
             "__model2",
             "int2 func(int8 a,int4 b,int4 c);",
             {scalar(reg("r12", 2), "int2"), scalar(join({joinReg("r10"), joinReg("r9")}), "int8"),
              scalar(reg("r8"), "int4"), scalar(stack(0, 4), "int4")}},
            {"nojoin",
             "__model2",
             "int4 func(int4 a,int8 b,int4 c);",
             {scalar(reg("r12"), "int4"), scalar(reg("r10"), "int4"), scalar(stack(0, 8), "int8"),
              scalar(reg("r9"), "int4")}},
            {"hiddenreturn",
             "__model1",
             "int8 func(int4 a,int4 b);",
             {meta(reg("r12"), TYPE_PTR, {}, "int8", ParameterPieces::indirectstorage),
              meta(reg("r12"), TYPE_PTR, {}, "int8", ParameterPieces::hiddenretparm), scalar(reg("r11"), "int4"),
              scalar(reg("r10"), "int4")}},
            {"mixedmeta",
             "__model2",
             "int4 func(char *a,int4 b,float4 c,int4 *d);",
             {scalar(reg("r12"), "int4"), meta(reg("r1"), TYPE_PTR), scalar(reg("r10"), "int4"),
              scalar(reg("r3"), "float4"), meta(reg("r2"), TYPE_PTR)}}};
}

/// Returns a recovery trial expectation with optional address and state checks.
static TrialExpectation trial(std::optional<StorageExpectation> storage, std::optional<bool> used,
                              std::optional<bool> unreferenced) {
    return TrialExpectation{std::move(storage), used, unreferenced};
}

/// Returns a recovery trial expectation that checks only the used/unreferenced state.
static TrialExpectation stateOnly(std::optional<bool> used, std::optional<bool> unreferenced) {
    return trial(std::nullopt, used, unreferenced);
}

/// Returns a recovery trial expectation that checks a concrete used storage location.
static TrialExpectation used(StorageExpectation storage) {
    return trial(std::move(storage), true, std::nullopt);
}

/// Returns all parameter-recovery scenarios preserved from testfuncproto.cc lines 499-680.
static vector<RecoveryCase> recoveryCases() {
    return {{"recoverbasic",
             "__model1",
             {reg("r11"), reg("r10"), reg("r12")},
             {used(reg("r12")), used(reg("r11")), used(reg("r10"))}},
            {"recoversmallreg",
             "__model1",
             {reg("r11"), reg("r12l", 2), reg("r10l", 2)},
             {used(reg("r12l", 2)), used(reg("r11")), used(reg("r10l", 2))}},
            {"recoverstack",
             "__model2",
             {reg("r10"), stack(0, 2), reg("r8"), stack(4, 4), reg("r9")},
             {stateOnly(false, true), stateOnly(false, true), stateOnly(false, true), stateOnly(false, true),
              stateOnly(false, true), used(reg("r10")), used(reg("r9")), used(reg("r8")), used(stack(0, 2)),
              used(stack(4, 4))}},
            {"recoverunrefregister",
             "__model1",
             {reg("r12"), reg("r10")},
             {used(reg("r12")), trial(reg("r11"), true, true), used(reg("r10"))}},
            {"recoverunrefstack",
             "__model2",
             {stack(4, 4), stack(12, 4), reg("r8"), reg("r9"), reg("r10")},
             {stateOnly(false, true), stateOnly(false, true), stateOnly(false, true), stateOnly(false, true),
              stateOnly(false, true), used(reg("r10")), used(reg("r9")), used(reg("r8")),
              trial(stack(0, 4), true, true), used(stack(4, 4)), trial(stack(8, 4), true, true), used(stack(12, 4))}},
            {"recovergroups",
             "__model3",
             {reg("r3"), reg("r5"), reg("r9")},
             {used(reg("r3")), used(reg("r9")), used(reg("r5"))}},
            {"recoverholes",
             "__model1",
             {reg("r8"), reg("r12"), stack(0, 4)},
             {used(reg("r12")), stateOnly(false, true), stateOnly(false, true), stateOnly(false, true),
              stateOnly(false, std::nullopt), stateOnly(false, std::nullopt)}},
            {"recoverfloat",
             "__model2",
             {reg("r10"), reg("r5"), reg("r3")},
             {stateOnly(false, true), stateOnly(false, true), used(reg("r3")), used(reg("r4")), used(reg("r5")),
              used(reg("r10"))}},
            {"recovermixedmeta",
             "__model2",
             {reg("r10"), reg("r4"), reg("r1")},
             {used(reg("r1")), stateOnly(false, true), used(reg("r3")), used(reg("r4")), stateOnly(false, true),
              used(reg("r10"))}}};
}

/// Compares one assigned parameter with its expected storage, type, and flags.
static void expectParameter(const ParameterPieces& actual, const ParameterExpectation& expected,
                            const ToyTestArchitecture& architecture) {
    const StorageExpectation& storage = expected.storage;
    switch (storage.kind) {
        case StorageKind::invalid:
            EXPECT_TRUE(actual.addr.isInvalid());
            break;
        case StorageKind::reg: {
            const VarnodeData& register_location = architecture.translate->getRegister(storage.register_name);
            EXPECT_EQ(actual.addr.getSpace(), register_location.space);
            EXPECT_EQ(actual.addr.getOffset(), register_location.offset);
            break;
        }
        case StorageKind::stack:
            EXPECT_EQ(actual.addr.getSpace(), architecture.getStackSpace());
            EXPECT_EQ(actual.addr.getOffset(), storage.offset);
            break;
        case StorageKind::join:
            EXPECT_EQ(actual.addr.getSpace(), architecture.getJoinSpace());
            ASSERT_EQ(actual.type->getSize(), storage.size);
            {
                JoinRecord* record = architecture.findJoin(actual.addr.getOffset());
                ASSERT_NE(record, nullptr);
                ASSERT_EQ(record->numPieces(), static_cast<int4>(storage.constituents.size()));
                for (std::size_t index = 0; index < storage.constituents.size(); ++index) {
                    const StorageConstituentExpectation& constituent = storage.constituents[index];
                    const VarnodeData& piece = record->getPiece(static_cast<int4>(index));
                    const VarnodeData& expected_location =
                        architecture.translate->getRegister(constituent.register_name);
                    EXPECT_EQ(piece.space, expected_location.space);
                    EXPECT_EQ(piece.offset, expected_location.offset + constituent.offset);
                    EXPECT_EQ(piece.size, static_cast<uint4>(constituent.size));
                }
            }
            break;
    }
    ASSERT_NE(actual.type, nullptr);
    EXPECT_EQ(actual.type->getName(), expected.type_name);
    if (storage.kind != StorageKind::invalid) {
        EXPECT_EQ(actual.type->getSize(), storage.size);
    }
    if (expected.metatype.has_value()) {
        EXPECT_EQ(actual.type->getMetatype(), *expected.metatype);
    }
    if (!expected.pointed_to_name.empty()) {
        const auto* pointer_type = dynamic_cast<const TypePointer*>(actual.type);
        ASSERT_NE(pointer_type, nullptr);
        ASSERT_NE(pointer_type->getPtrTo(), nullptr);
        EXPECT_EQ(pointer_type->getPtrTo()->getName(), expected.pointed_to_name);
    }
    // The original funcproto tests compare the complete flag word; checking zero
    // as well prevents accidental ABI flags on ordinary parameters being hidden.
    EXPECT_EQ(actual.flags, expected.required_flags);
}

/// Compares one recovered trial with its optional concrete address and state expectations.
static void expectTrial(const ParamTrial& actual, const TrialExpectation& expected,
                        const ToyTestArchitecture& architecture) {
    if (expected.storage.has_value()) {
        const StorageExpectation& storage = *expected.storage;
        switch (storage.kind) {
            case StorageKind::reg: {
                const VarnodeData& register_location = architecture.translate->getRegister(storage.register_name);
                EXPECT_EQ(actual.getAddress().getSpace(), register_location.space);
                EXPECT_EQ(actual.getAddress().getOffset(), register_location.offset);
                EXPECT_EQ(actual.getSize(), storage.size);
                break;
            }
            case StorageKind::stack:
                EXPECT_EQ(actual.getAddress().getSpace(), architecture.getStackSpace());
                EXPECT_EQ(actual.getAddress().getOffset(), storage.offset);
                EXPECT_EQ(actual.getSize(), storage.size);
                break;
            case StorageKind::invalid:
            case StorageKind::join:
                FAIL() << "Recovery expectations cannot use invalid or join storage";
                break;
        }
    }
    if (expected.used.has_value()) {
        EXPECT_EQ(actual.isUsed(), *expected.used);
    }
    if (expected.unreferenced.has_value()) {
        EXPECT_EQ(actual.isUnref(), *expected.unreferenced);
    }
}

/// Adds and marks one register trial using the native ParamActive contract.
static void registerActive(ParamActive& active, const ToyTestArchitecture& architecture, const string& name,
                           int4 size) {
    active.registerTrial(architecture.translate->getRegister(name).getAddr(), size);
    active.getTrial(active.getNumTrials() - 1).markActive();
}

/// Adds and marks one stack trial using the native ParamActive contract.
static void stackActive(ParamActive& active, const ToyTestArchitecture& architecture, uintb offset, int4 size) {
    active.registerTrial(Address(architecture.getStackSpace(), offset), size);
    active.getTrial(active.getNumTrials() - 1).markActive();
}

/// Registers a table row's storage locations in the same order as the original recovery tests.
static void registerActiveStorage(ParamActive& active, const ToyTestArchitecture& architecture,
                                  const vector<StorageExpectation>& storage) {
    for (const StorageExpectation& expectation : storage) {
        if (expectation.kind == StorageKind::reg) {
            registerActive(active, architecture, expectation.register_name, expectation.size);
        } else {
            stackActive(active, architecture, expectation.offset, expectation.size);
        }
    }
}

/// Runs every original storage-assignment declaration through grammar and ProtoModel algorithms.
TEST(NativeFuncProto, AssignParameterStorageTable) {
    ToyTestArchitecture* architecture_pointer = testArchitectureOrNull();
    ASSERT_NE(architecture_pointer, nullptr);
    ToyTestArchitecture& architecture = *architecture_pointer;
    for (const AssignmentCase& test_case : assignmentCases()) {
        SCOPED_TRACE(test_case.name);
        ProtoModel* model = architecture.getModel(test_case.model_name);
        ASSERT_NE(model, nullptr);
        PrototypePieces pieces{};
        istringstream declaration(test_case.declaration);
        ASSERT_NO_THROW(parse_protopieces(pieces, declaration, &architecture));
        vector<ParameterPieces> actual;
        ASSERT_NO_THROW(model->assignParameterStorage(pieces, actual, false));
        ASSERT_EQ(actual.size(), test_case.expected.size());
        for (std::size_t index = 0; index < actual.size(); ++index) {
            expectParameter(actual[index], test_case.expected[index], architecture);
            if (test_case.expected[index].same_type_as.has_value()) {
                EXPECT_EQ(actual[index].type, actual[*test_case.expected[index].same_type_as].type);
            }
        }
    }
}

/// Runs every original unordered trial set through ParamActive and ProtoModel input recovery.
TEST(NativeFuncProto, DeriveInputMapTable) {
    ToyTestArchitecture* architecture_pointer = testArchitectureOrNull();
    ASSERT_NE(architecture_pointer, nullptr);
    ToyTestArchitecture& architecture = *architecture_pointer;
    for (const RecoveryCase& test_case : recoveryCases()) {
        SCOPED_TRACE(test_case.name);
        ProtoModel* model = architecture.getModel(test_case.model_name);
        ASSERT_NE(model, nullptr);
        ParamActive active(false);
        registerActiveStorage(active, architecture, test_case.active_storage);
        ASSERT_NO_THROW(model->deriveInputMap(&active));
        ASSERT_EQ(active.getNumTrials(), static_cast<int4>(test_case.expected.size()));
        for (std::size_t index = 0; index < test_case.expected.size(); ++index) {
            expectTrial(active.getTrial(static_cast<int4>(index)), test_case.expected[index], architecture);
        }
    }
}

} // namespace ghidra::native_funcproto_tests
