module;

#include <gtest/gtest.h>

export module native_type_tests;

import decompiler;
import ghidra.decompiler;
import ghidra.decompiler.grammar;
import std;

namespace newghidra::decompiler::tests {

/// Provides the smallest provider-style architecture description needed by the
/// native type and cast tests, replacing testtypes.cc's x86 XML document.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc:44-65.
static ArchitectureDescription type_test_architecture() {
    ArchitectureDescription description;
    description.name = "native-type-test";
    description.code_space = "ram";
    description.data_space = "ram";
    description.stack_register = "RSP";
    description.pointer_size = 8;
    description.spaces = {
        SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    description.registers = {
        RegisterDescription{"RAX", Storage{"register", 0x00, 8}},
        RegisterDescription{"RCX", Storage{"register", 0x08, 8}},
        RegisterDescription{"RDX", Storage{"register", 0x10, 8}},
        RegisterDescription{"RSP", Storage{"register", 0x20, 8}},
    };
    return description;
}

/// Adapts provider architecture metadata to the native Translate interface
/// without decoding instructions or consulting XML architecture specifications.
/// Original source: Ghidra/Features/Decompiler/src/decompile/cpp/translate.cc and
/// NEW/features/decompiler/src/decompiler_impl.cppm's ProviderTranslate.
class TypeTestTranslate final : public ghidra::Translate {
public:
    /// Constructs address spaces and register mappings from provider metadata.
    /// The resulting translator is intentionally sufficient only for native type
    /// construction and p-code graph ownership; instruction methods fail clearly.
    explicit TypeTestTranslate(const ArchitectureDescription& description) {
        if (description.pointer_size == 0) {
            throw std::invalid_argument("Type-test architecture requires a non-zero pointer size");
        }

        insertSpace(new ghidra::ConstantSpace(this, this));
        for (const SpaceDescription& space : description.spaces) {
            if (space.name.empty() || space.name == "const" || space.name == "unique") {
                continue;
            }
            const int index = space.index < 1 ? numSpaces() : space.index;
            insertSpace(new ghidra::AddrSpace(
                this, this, ghidra::IPTR_PROCESSOR, space.name, space.big_endian, space.address_size, space.word_size,
                index, space.physical ? ghidra::AddrSpace::hasphysical : 0, space.delay, space.delay));
        }
        insertSpace(new ghidra::UniqueSpace(this, this, numSpaces(), ghidra::AddrSpace::hasphysical));

        const std::string code_space =
            description.code_space.empty() ? first_processor_space() : description.code_space;
        const std::string data_space = description.data_space.empty() ? code_space : description.data_space;
        if (getSpaceByName(code_space) == nullptr || getSpaceByName(data_space) == nullptr) {
            throw std::invalid_argument("Type-test architecture names an unknown default address space");
        }
        setDefaultCodeSpace(getSpaceByName(code_space)->getIndex());
        setDefaultDataSpace(getSpaceByName(data_space)->getIndex());

        for (const RegisterDescription& register_description : description.registers) {
            ghidra::AddrSpace* space = getSpaceByName(register_description.location.space);
            if (space == nullptr || register_description.location.size == 0) {
                throw std::invalid_argument("Type-test register references invalid storage");
            }
            registers_.emplace(register_description.name,
                               ghidra::VarnodeData{space, register_description.location.offset,
                                                   static_cast<ghidra::uint4>(register_description.location.size)});
        }
        setDefaultFloatFormats();
    }

    /// Accepts the native initialization hook because all provider metadata was
    /// materialized by the constructor rather than read from XML.
    void initialize(ghidra::DocumentStorage&) override {}

    /// Resolves a provider-declared register by name.
    const ghidra::VarnodeData& getRegister(const ghidra::string& name) const override {
        const auto iterator = registers_.find(name);
        if (iterator == registers_.end()) {
            throw ghidra::LowlevelError("Unknown type-test register: " + name);
        }
        return iterator->second;
    }

    /// Returns the smallest provider register containing a requested location.
    ghidra::string getRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset, ghidra::int4 size) const override {
        for (const auto& [name, location] : registers_) {
            if (location.space == space && location.offset <= offset && offset - location.offset <= location.size &&
                size <= location.size - (offset - location.offset)) {
                return name;
            }
        }
        return {};
    }

    /// Returns a provider register whose storage exactly matches a location.
    ghidra::string getExactRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset,
                                        ghidra::int4 size) const override {
        for (const auto& [name, location] : registers_) {
            if (location.space == space && location.offset == offset && location.size == size) {
                return name;
            }
        }
        return {};
    }

    /// Copies all provider register mappings into the native result map.
    void getAllRegisters(std::map<ghidra::VarnodeData, ghidra::string>& result) const override {
        result.clear();
        for (const auto& [name, location] : registers_) {
            result.emplace(location, name);
        }
    }

    /// Reports that this deliberately minimal architecture declares no user p-code operations.
    void getUserOpNames(std::vector<ghidra::string>& result) const override {
        result.clear();
    }

    /// Rejects instruction-length queries because this test architecture has no decoder.
    ghidra::int4 instructionLength(const ghidra::Address&) const override {
        throw ghidra::LowlevelError("Type-test architecture does not decode instructions");
    }

    /// Rejects p-code emission requests because the tests construct native p-code
    /// operations directly through Funcdata, not through a decoder substitute.
    ghidra::int4 oneInstruction(ghidra::PcodeEmit&, const ghidra::Address&) const override {
        throw ghidra::LowlevelError("Type-test architecture does not emit instructions");
    }

    /// Rejects assembly requests because presentation is outside the type-test contract.
    ghidra::int4 printAssembly(ghidra::AssemblyEmit&, const ghidra::Address&) const override {
        throw ghidra::LowlevelError("Type-test architecture does not print instructions");
    }

private:
    /// Finds the first processor address space when provider metadata omits a default name.
    std::string first_processor_space() const {
        for (int index = 0; index < numSpaces(); ++index) {
            ghidra::AddrSpace* space = getSpace(index);
            if (space != nullptr && space->getType() == ghidra::IPTR_PROCESSOR) {
                return space->getName();
            }
        }
        throw std::invalid_argument("Type-test architecture has no processor address space");
    }

    std::map<ghidra::string, ghidra::VarnodeData> registers_;
};

/// Owns only the native subsystems required by testtypes.cc: type storage, the
/// C cast strategy, the type-operation table, and a real Funcdata p-code graph.
/// XML loading, executable memory, and instruction decoding are intentionally absent.
/// Original source: Ghidra/Features/Decompiler/src/decompile/cpp/architecture.cc.
class TypeTestArchitecture final : public ghidra::Architecture {
public:
    /// Constructs the in-memory architecture and initializes native core types,
    /// p-code operations, and the global symbol scope used by Funcdata.
    explicit TypeTestArchitecture(ArchitectureDescription description) : description_(std::move(description)) {
        ghidra::AttributeId::initialize();
        ghidra::ElementId::initialize();
        ghidra::forcePrintCLanguageRegistration();
        archid = description_.name;

        translate = new TypeTestTranslate(description_);
        copySpaces(translate);
        insertSpace(new ghidra::OtherSpace(this, translate, ghidra::OtherSpace::INDEX));
        insertSpace(new ghidra::FspecSpace(this, translate, numSpaces()));
        insertSpace(new ghidra::IopSpace(this, translate, numSpaces()));
        insertSpace(new ghidra::JoinSpace(this, translate, numSpaces()));

        const auto* provider_translate = static_cast<const TypeTestTranslate*>(translate);
        const ghidra::VarnodeData& stack_register = provider_translate->getRegister(description_.stack_register);
        addSpacebase(getSpaceByName(description_.data_space), "stack", stack_register, stack_register.size, false, true,
                     true);

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
        userops.initialize(this);

        ghidra::DocumentStorage empty;
        buildInstructions(empty);
        print->initializeFromArchitecture();
        print->adjustTypeOperators();
    }

    /// Returns the provider description retained for diagnostics and test setup.
    const ArchitectureDescription& description() const {
        return description_;
    }

    /// Records native architecture warnings without requiring an XML console.
    void printWarning(const ghidra::string& message) const override {
        warnings_ << message << '\n';
    }

protected:
    /// Returns the already-materialized provider translator to the native factory protocol.
    ghidra::Translate* buildTranslator(ghidra::DocumentStorage&) override {
        return const_cast<ghidra::Translate*>(translate);
    }

    /// Leaves executable loading disabled because type tests have no binary image.
    void buildLoader(ghidra::DocumentStorage&) override {}

    /// Leaves p-code injection disabled because no injection behavior is under test.
    ghidra::PcodeInjectLibrary* buildPcodeInjectLibrary() override {
        return nullptr;
    }

    /// Keeps the type factory assembled directly from native core-type APIs.
    void buildTypegrp(ghidra::DocumentStorage&) override {}

    /// Keeps the primitive types assembled directly from native TypeFactory APIs.
    void buildCoreTypes(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory comment database created by the constructor.
    void buildCommentDB(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory Unicode string manager created by the constructor.
    void buildStringManager(ghidra::DocumentStorage&) override {}

    /// Keeps the empty in-memory constant pool created by the constructor.
    void buildConstantPool(ghidra::DocumentStorage&) override {}

    /// Keeps the empty context database created by the constructor.
    void buildContext(ghidra::DocumentStorage&) override {}

    /// Does not add symbols because the fixture creates only a global scope and a dummy Funcdata.
    void buildSymbols(ghidra::DocumentStorage&) override {}

    /// Does not read processor or compiler XML specifications.
    void buildSpecFile(ghidra::DocumentStorage&) override {}

    /// Leaves the provider-described address spaces unchanged.
    void modifySpaces(ghidra::Translate*) override {}

    /// Uses the explicit provider description as the complete architecture resolution.
    void resolveArchitecture() override {
        archid = description_.name;
    }

private:
    ArchitectureDescription description_;
    mutable std::ostringstream warnings_;
};

/// Initializes printer capabilities before Architecture's base constructor builds
/// its default printer, then creates the native type-test architecture.
static std::unique_ptr<TypeTestArchitecture> make_type_test_architecture() {
    ghidra::forcePrintCLanguageRegistration();
    ghidra::CapabilityPoint::initializeAll();
    return std::make_unique<TypeTestArchitecture>(type_test_architecture());
}

/// Builds a fresh native architecture and Funcdata graph for each test so that
/// TypeFactory-owned types and p-code links cannot leak between test cases.
/// Original source: testtypes.cc's static TypeTestEnvironment and dummy function.
class TypeTestEnvironment {
public:
    /// Creates the native architecture, global scope, and high-level dummy function.
    TypeTestEnvironment() : architecture_(make_type_test_architecture()), dummy_(nullptr) {
        const ghidra::Address address(architecture_->getDefaultCodeSpace(), 0x1000);
        // An empty function name keeps Funcdata from creating a local scope; the
        // type tests require only its real p-code graph, not symbol ownership.
        dummy_ =
            std::make_unique<ghidra::Funcdata>("", "", architecture_->symboltab->getGlobalScope(), address, nullptr);
        dummy_->setHighLevel();
    }

    /// Returns the provider-backed native architecture used by the fixture.
    TypeTestArchitecture& architecture() {
        return *architecture_;
    }

    /// Returns the real TypeFactory populated during architecture bootstrap.
    ghidra::TypeFactory& types() {
        return *architecture_->types;
    }

    /// Returns the C cast strategy attached to the native printer.
    ghidra::CastStrategy& strategy() {
        return *architecture_->print->getCastStrategy();
    }

    /// Returns the real Funcdata graph used to attach operands to native PcodeOps.
    ghidra::Funcdata& function() {
        return *dummy_;
    }

private:
    std::unique_ptr<TypeTestArchitecture> architecture_;
    std::unique_ptr<ghidra::Funcdata> dummy_;
};

/// Creates a real native base type or cached core type by size and meta-type.
/// Original source: testtypes.cc's parse("intN"), parse("uintN"), and parse("xunknownN").
static ghidra::Datatype* base_type(ghidra::TypeFactory& types, ghidra::int4 size, ghidra::type_metatype meta) {
    if (meta == ghidra::TYPE_VOID) {
        return types.getTypeVoid();
    }
    const std::string prefix = meta == ghidra::TYPE_INT       ? "int"
                               : meta == ghidra::TYPE_UINT    ? "uint"
                               : meta == ghidra::TYPE_FLOAT   ? "float"
                               : meta == ghidra::TYPE_UNKNOWN ? "xunknown"
                               : meta == ghidra::TYPE_BOOL    ? "bool"
                                                              : "";
    if (!prefix.empty()) {
        const std::string name = prefix + std::to_string(size);
        if (ghidra::Datatype* core = types.findByName(name); core != nullptr) {
            return core;
        }
    }
    return types.getBase(size, meta);
}

/// Creates a pointer through TypeFactory, preserving the native pointer/cast implementation.
/// Original source: testtypes.cc's pointer type grammar and getTypePointer calls.
static ghidra::Datatype* pointer_type(ghidra::TypeFactory& types, ghidra::Datatype* pointed_to,
                                      ghidra::int4 pointer_size = 8) {
    return types.getTypePointer(pointer_size, pointed_to, 1);
}

/// Defines a two-field native structure using TypeFactory's layout algorithm.
/// Original source: testtypes.cc's structone and structtwo declarations.
static ghidra::TypeStruct* two_int_fields(ghidra::TypeFactory& types, const ghidra::string& name) {
    ghidra::Datatype* int4_type = base_type(types, 4, ghidra::TYPE_INT);
    auto* structure = types.getTypeStruct(name);
    std::vector<ghidra::TypeField> fields{
        ghidra::TypeField(0, 0, "a", int4_type),
        ghidra::TypeField(1, 4, "b", int4_type),
    };
    std::vector<ghidra::TypeBitField> bitfields;
    types.assignRawFields(structure, fields, bitfields);
    return structure;
}

/// Defines an enum and installs its named values through the native TypeFactory API.
/// Original source: testtypes.cc's enumone, enum2, enum3, and enum4 declarations.
static ghidra::TypeEnum* enum_type(ghidra::TypeFactory& types, const ghidra::string& name,
                                   std::initializer_list<std::pair<ghidra::uintb, ghidra::string>> values) {
    auto* enumeration = types.getTypeEnum(name);
    std::map<ghidra::uintb, ghidra::string> named_values;
    for (const auto& [value, label] : values) {
        named_values.emplace(value, label);
    }
    types.setEnumValues(named_values, enumeration);
    return enumeration;
}

/// Creates the same native PcodeOp/Varnode graph as testtypes.cc::castPrinted
/// and returns the real TypeOp/CastStrategy decision for input slot zero.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc:80-102.
static bool cast_printed(TypeTestEnvironment& environment, ghidra::OpCode opcode, ghidra::Datatype* required,
                         ghidra::Datatype* current) {
    ghidra::TypeOp* instruction = environment.architecture().inst[opcode];
    ghidra::PcodeOp* operation;
    const ghidra::Address address(environment.architecture().getDefaultCodeSpace(), 0x1000);
    if ((instruction->getFlags() & ghidra::PcodeOp::unary) != 0) {
        operation = environment.function().newOp(1, address);
        ghidra::Varnode* input = environment.function().newUnique(current->getSize(), current);
        ghidra::Varnode* output = environment.function().newUniqueOut(required->getSize(), operation);
        output->updateType(required, true, true);
        environment.function().opSetOpcode(operation, opcode);
        environment.function().opSetInput(operation, input, 0);
    } else {
        operation = environment.function().newOp(2, address);
        ghidra::Varnode* first = environment.function().newUnique(required->getSize(), required);
        ghidra::Varnode* second = environment.function().newUnique(current->getSize(), current);
        environment.function().opSetOpcode(operation, opcode);
        environment.function().opSetInput(operation, first, 0);
        environment.function().opSetInput(operation, second, 1);
        environment.function().newUniqueOut(1, operation);
    }
    return instruction->getInputCast(operation, 0, &environment.strategy()) != nullptr;
}

/// Creates the native integer-token operation and delegates long-size marking to
/// CastStrategy::markExplicitLongSize, preserving constant type and value handling.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc:104-119.
static bool long_printed(TypeTestEnvironment& environment, ghidra::OpCode opcode, ghidra::Datatype* type,
                         ghidra::uintb value) {
    const ghidra::Address address(environment.architecture().getDefaultCodeSpace(), 0x1000);
    ghidra::PcodeOp* operation = environment.function().newOp(2, address);
    ghidra::Datatype* int_type = base_type(environment.types(), 4, ghidra::TYPE_INT);
    ghidra::Varnode* constant = environment.function().newConstant(type->getSize(), value);
    constant->updateType(type, false, true);
    ghidra::Varnode* shift_amount = environment.function().newUnique(int_type->getSize(), int_type);
    environment.function().opSetOpcode(operation, opcode);
    environment.function().opSetInput(operation, constant, 0);
    environment.function().opSetInput(operation, shift_amount, 1);
    environment.function().newUniqueOut(constant->getSize(), operation);
    return environment.strategy().markExplicitLongSize(operation, 0);
}

/// Parses the original C-like type syntax through the native grammar entrypoint.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc:74-78.
static ghidra::Datatype* parse_original_type(TypeTestEnvironment& environment, const std::string& text) {
    std::istringstream stream(text);
    std::string unused_name;
    return ghidra::parse_type(stream, unused_name, &environment.architecture());
}

/// Verifies that the grammar path used by the original cast tests still creates
/// arrays, pointers, structures, and enumerations rather than relying only on
/// manually constructed TypeFactory objects.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc:123-162.
TEST(NativeType, ParsesOriginalGrammarDeclarations) {
    TypeTestEnvironment environment;
    ghidra::Datatype* array = parse_original_type(environment, "int1 var[4]");
    ASSERT_NE(array, nullptr);
    EXPECT_EQ(array->getSize(), 4);
    ghidra::Datatype* pointer = parse_original_type(environment, "int4 *");
    ASSERT_NE(pointer, nullptr);
    EXPECT_EQ(pointer->getMetatype(), ghidra::TYPE_PTR);
    ghidra::Datatype* structure = parse_original_type(environment, "struct grammar_record { int4 a; int4 b; }");
    ASSERT_NE(structure, nullptr);
    EXPECT_EQ(structure->getName(), "grammar_record");
    ghidra::Datatype* enumeration = parse_original_type(environment, "enum grammar_enum { ONE=1, TWO=2 }");
    ASSERT_NE(enumeration, nullptr);
    EXPECT_NE(dynamic_cast<ghidra::TypeEnum*>(enumeration), nullptr);
    EXPECT_EQ(enumeration->getName(), "grammar_enum");
}

/// Verifies the original basic cast matrix using native primitive, unknown,
/// character, array, named, and floating-point datatypes.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::cast_basic.
TEST(NativeType, CastBasic) {
    TypeTestEnvironment environment;
    ghidra::TypeFactory& types = environment.types();
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 4, ghidra::TYPE_INT),
                             base_type(types, 2, ghidra::TYPE_INT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 4, ghidra::TYPE_INT),
                              base_type(types, 4, ghidra::TYPE_UINT)));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, pointer_type(types, base_type(types, 4, ghidra::TYPE_INT)),
                             base_type(types, 8, ghidra::TYPE_UINT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 1, ghidra::TYPE_INT),
                              base_type(types, 1, ghidra::TYPE_BOOL)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 4, ghidra::TYPE_UNKNOWN),
                              base_type(types, 4, ghidra::TYPE_UINT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 4, ghidra::TYPE_INT),
                              base_type(types, 4, ghidra::TYPE_UNKNOWN)));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 4, ghidra::TYPE_INT),
                             base_type(types, 4, ghidra::TYPE_FLOAT)));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY,
                             types.getTypeArray(4, base_type(types, 1, ghidra::TYPE_INT)),
                             base_type(types, 4, ghidra::TYPE_UINT)));
    ghidra::Datatype* named_int = types.getBase(4, ghidra::TYPE_INT, "myint4");
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, named_int, base_type(types, 4, ghidra::TYPE_INT)));
    ASSERT_FALSE(
        cast_printed(environment, ghidra::CPUI_COPY, types.getTypeChar(1), base_type(types, 1, ghidra::TYPE_INT)));
    ASSERT_FALSE(
        cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 1, ghidra::TYPE_UINT), types.getTypeChar(1)));
}

/// Verifies pointer compatibility, pointer depth, composite identity, enum
/// pointers, character pointers, and named pointer behavior.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::cast_pointer.
TEST(NativeType, CastPointer) {
    TypeTestEnvironment environment;
    ghidra::TypeFactory& types = environment.types();
    /// Reuses one native pointer construction path for every pointer-cast case.
    auto ptr = [&types](ghidra::Datatype* type) { return pointer_type(types, type); };
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 4, ghidra::TYPE_UINT)),
                             ptr(base_type(types, 4, ghidra::TYPE_INT))));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 1, ghidra::TYPE_VOID)),
                              ptr(base_type(types, 4, ghidra::TYPE_FLOAT))));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 2, ghidra::TYPE_INT)),
                              ptr(base_type(types, 1, ghidra::TYPE_VOID))));
    ghidra::Datatype* named_int = types.getBase(4, ghidra::TYPE_INT, "myint4");
    ghidra::Datatype* named_pointer = types.getTypePointer(8, named_int, 1);
    ASSERT_FALSE(
        cast_printed(environment, ghidra::CPUI_COPY, named_pointer, ptr(base_type(types, 4, ghidra::TYPE_INT))));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, ptr(ptr(base_type(types, 1, ghidra::TYPE_BOOL))),
                             ptr(ptr(base_type(types, 1, ghidra::TYPE_INT)))));
    ghidra::TypeStruct* first = two_int_fields(types, "structone");
    ghidra::TypeStruct* second = two_int_fields(types, "structtwo");
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, ptr(first), ptr(second)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 4, ghidra::TYPE_UNKNOWN)),
                              ptr(base_type(types, 4, ghidra::TYPE_INT))));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 4, ghidra::TYPE_UINT)),
                              ptr(base_type(types, 4, ghidra::TYPE_UNKNOWN))));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, ptr(types.getTypeChar(1)),
                              ptr(base_type(types, 1, ghidra::TYPE_INT))));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 1, ghidra::TYPE_UINT)),
                             ptr(types.getTypeChar(1))));
    ghidra::Datatype* named_pointer_with_name =
        types.getTypePointer(8, base_type(types, 4, ghidra::TYPE_INT), 1, "myptrint4");
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, ptr(base_type(types, 4, ghidra::TYPE_INT)),
                              named_pointer_with_name));
}

/// Verifies enum-to-integer and enum-pointer compatibility decisions using a
/// TypeEnum populated by the native enumeration map implementation.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::cast_enum.
TEST(NativeType, CastEnum) {
    TypeTestEnvironment environment;
    ghidra::TypeFactory& types = environment.types();
    ghidra::TypeEnum* enumeration = enum_type(types, "enumone", {{1, "ONE"}, {2, "TWO"}});
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, base_type(types, 8, ghidra::TYPE_INT), enumeration));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY,
                              pointer_type(types, base_type(types, 8, ghidra::TYPE_UINT)),
                              pointer_type(types, enumeration)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_COPY, enumeration, base_type(types, 8, ghidra::TYPE_UINT)));
}

/// Verifies signed and unsigned comparison operators, pointer/integer
/// comparisons, and floating comparison casts through native TypeOp objects.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::cast_compare.
TEST(NativeType, CastCompare) {
    TypeTestEnvironment environment;
    ghidra::TypeFactory& types = environment.types();
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_INT_LESS, base_type(types, 4, ghidra::TYPE_INT),
                             base_type(types, 4, ghidra::TYPE_INT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_INT_LESS, base_type(types, 4, ghidra::TYPE_UINT),
                              base_type(types, 4, ghidra::TYPE_UINT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_INT_LESS,
                              pointer_type(types, base_type(types, 4, ghidra::TYPE_INT)),
                              pointer_type(types, base_type(types, 4, ghidra::TYPE_INT))));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_INT_SLESS, base_type(types, 4, ghidra::TYPE_UINT),
                             base_type(types, 4, ghidra::TYPE_UINT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_INT_SLESS, base_type(types, 4, ghidra::TYPE_INT),
                              base_type(types, 4, ghidra::TYPE_INT)));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_INT_EQUAL, base_type(types, 8, ghidra::TYPE_UINT),
                             pointer_type(types, base_type(types, 4, ghidra::TYPE_INT))));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_INT_EQUAL,
                              pointer_type(types, base_type(types, 4, ghidra::TYPE_INT)),
                              base_type(types, 8, ghidra::TYPE_UINT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_INT_NOTEQUAL, base_type(types, 4, ghidra::TYPE_INT),
                              base_type(types, 4, ghidra::TYPE_UINT)));
    ASSERT_FALSE(cast_printed(environment, ghidra::CPUI_INT_NOTEQUAL, base_type(types, 4, ghidra::TYPE_UINT),
                              base_type(types, 4, ghidra::TYPE_INT)));
    ASSERT_TRUE(cast_printed(environment, ghidra::CPUI_INT_EQUAL, base_type(types, 4, ghidra::TYPE_INT),
                             base_type(types, 4, ghidra::TYPE_FLOAT)));
}

/// Verifies Datatype ordering and dependency ordering for primitive,
/// character, enum, structure, floating-point, boolean, and pointer types.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::type_ordering.
TEST(NativeType, TypeOrdering) {
    TypeTestEnvironment environment;
    ghidra::TypeFactory& types = environment.types();
    ASSERT_LT(base_type(types, 4, ghidra::TYPE_UINT)->compare(*base_type(types, 4, ghidra::TYPE_INT), 10), 0);
    ghidra::Datatype* named_int = types.getBase(4, ghidra::TYPE_INT, "myint4");
    ghidra::Datatype* int_type = base_type(types, 4, ghidra::TYPE_INT);
    ASSERT_NE(int_type, named_int);
    ASSERT_EQ(int_type->compareDependency(*named_int), 0);
    ASSERT_LT(base_type(types, 1, ghidra::TYPE_INT)->compare(*types.getTypeChar(1), 10), 0);
    ASSERT_LT(types.getTypeChar(2)->compare(*base_type(types, 2, ghidra::TYPE_INT), 10), 0);
    ASSERT_LT(types.getTypeChar(4)->compare(*base_type(types, 4, ghidra::TYPE_INT), 10), 0);
    ASSERT_LT(base_type(types, 1, ghidra::TYPE_UINT)->compare(*types.getTypeChar(1), 10), 0);
    ghidra::TypeEnum* enumeration = enum_type(types, "enum2", {{1, "ONE"}, {2, "TWO"}});
    ASSERT_LT(enumeration->compare(*base_type(types, 8, ghidra::TYPE_INT), 10), 0);
    ghidra::TypeStruct* first = two_int_fields(types, "struct1");
    ghidra::TypeStruct* second = two_int_fields(types, "struct2");
    ASSERT_NE(first, second);
    ASSERT_EQ(first->compareDependency(*second), 0);
    ASSERT_LT(base_type(types, 4, ghidra::TYPE_UINT)->compare(*base_type(types, 2, ghidra::TYPE_UINT), 10), 0);
    ASSERT_LT(base_type(types, 8, ghidra::TYPE_FLOAT)->compare(*base_type(types, 4, ghidra::TYPE_FLOAT), 10), 0);
    ASSERT_LT(base_type(types, 1, ghidra::TYPE_BOOL)->compare(*base_type(types, 1, ghidra::TYPE_UINT), 10), 0);
    ASSERT_LT(pointer_type(types, base_type(types, 4, ghidra::TYPE_UINT))
                  ->compare(*pointer_type(types, base_type(types, 4, ghidra::TYPE_INT)), 10),
              0);
    ASSERT_LT(pointer_type(types, enum_type(types, "enum2-pointer", {{1, "ONE"}, {2, "TWO"}}))
                  ->compare(*pointer_type(types, base_type(types, 8, ghidra::TYPE_INT)), 10),
              0);
    ASSERT_LT(pointer_type(types, base_type(types, 4, ghidra::TYPE_INT))
                  ->compare(*pointer_type(types, base_type(types, 1, ghidra::TYPE_VOID)), 10),
              0);
    ASSERT_LT(pointer_type(types, base_type(types, 2, ghidra::TYPE_INT))
                  ->compare(*pointer_type(types, base_type(types, 2, ghidra::TYPE_UNKNOWN)), 10),
              0);
}

/// Verifies explicit long-size marking for the original shift token values,
/// including signed negative constants and 32-bit boundary transitions.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::cast_integertoken.
TEST(NativeType, CastIntegerToken) {
    TypeTestEnvironment environment;
    ghidra::TypeFactory& types = environment.types();
    ghidra::Datatype* int8_type = base_type(types, 8, ghidra::TYPE_INT);
    ghidra::Datatype* uint8_type = base_type(types, 8, ghidra::TYPE_UINT);
    ASSERT_TRUE(long_printed(environment, ghidra::CPUI_INT_LEFT, int8_type, 10));
    ASSERT_FALSE(long_printed(environment, ghidra::CPUI_INT_LEFT, int8_type, 0x100000000ULL));
    ASSERT_TRUE(long_printed(environment, ghidra::CPUI_INT_SRIGHT, int8_type, static_cast<ghidra::uintb>(-3)));
    ASSERT_FALSE(long_printed(environment, ghidra::CPUI_INT_SRIGHT, int8_type, 0xffffffff7fffffffULL));
    ASSERT_TRUE(long_printed(environment, ghidra::CPUI_INT_SRIGHT, int8_type, 0xffffffff80000000ULL));
    ASSERT_TRUE(long_printed(environment, ghidra::CPUI_INT_RIGHT, uint8_type, 0xffffffffULL));
    ASSERT_FALSE(long_printed(environment, ghidra::CPUI_INT_RIGHT, uint8_type, 0x100000000ULL));
}

/// Verifies native enum token decomposition, complement recognition, exact
/// single-name matches, and the no-representation case.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::enum_matching.
TEST(NativeType, EnumMatching) {
    TypeTestEnvironment environment;
    ghidra::TypeEnum* enumeration =
        enum_type(environment.types(), "enum3", {{0, "ZERO"}, {1, "ONE"}, {2, "TWO"}, {4, "FOUR"}, {8, "EIGHT"}});
    ghidra::TypeEnum::Representation representation;
    enumeration->getMatches(5, representation);
    ASSERT_EQ(representation.matchname.size(), 2U);
    ASSERT_EQ(representation.matchname[0], "FOUR");
    ASSERT_EQ(representation.matchname[1], "ONE");
    ASSERT_FALSE(representation.complement);
    representation.matchname.clear();
    enumeration->getMatches(0xfffffffffffffff7ULL, representation);
    ASSERT_EQ(representation.matchname.size(), 1U);
    ASSERT_EQ(representation.matchname[0], "EIGHT");
    ASSERT_TRUE(representation.complement);
    representation.matchname.clear();
    representation.complement = false;
    enumeration->getMatches(0, representation);
    ASSERT_EQ(representation.matchname.size(), 1U);
    ASSERT_EQ(representation.matchname[0], "ZERO");
    ASSERT_FALSE(representation.complement);
    representation.matchname.clear();
    enumeration->getMatches(0x10, representation);
    ASSERT_EQ(representation.matchname.size(), 0U);
    ASSERT_FALSE(representation.complement);
}

/// Verifies native enum token decomposition for overlapping bit combinations,
/// exact values, and a value that has no complete token representation.
/// Original source: Ghidra/Features/Decompiler/src/decompile/unittests/testtypes.cc::enum_matching2.
TEST(NativeType, EnumMatchingTwo) {
    TypeTestEnvironment environment;
    ghidra::TypeEnum* enumeration =
        enum_type(environment.types(), "enum4",
                  {{0, "ZERO"}, {1, "ONE"}, {2, "TWO"}, {4, "FOUR"}, {6, "SIX"}, {8, "EIGHT"}, {11, "ELEVEN"}});
    ghidra::TypeEnum::Representation representation;
    enumeration->getMatches(12, representation);
    ASSERT_EQ(representation.matchname.size(), 2U);
    ASSERT_EQ(representation.matchname[0], "EIGHT");
    ASSERT_EQ(representation.matchname[1], "FOUR");
    ASSERT_FALSE(representation.complement);
    representation.matchname.clear();
    enumeration->getMatches(7, representation);
    ASSERT_EQ(representation.matchname.size(), 2U);
    ASSERT_EQ(representation.matchname[0], "SIX");
    ASSERT_EQ(representation.matchname[1], "ONE");
    ASSERT_FALSE(representation.complement);
    representation.matchname.clear();
    enumeration->getMatches(11, representation);
    ASSERT_EQ(representation.matchname.size(), 1U);
    ASSERT_EQ(representation.matchname[0], "ELEVEN");
    ASSERT_FALSE(representation.complement);
}

} // namespace newghidra::decompiler::tests
