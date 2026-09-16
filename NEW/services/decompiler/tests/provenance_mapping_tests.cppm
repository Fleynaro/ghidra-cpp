module;

#include <gtest/gtest.h>

export module provenance_mapping_tests;

import decompiler;
import std;

namespace newghidra::decompiler::provenance_tests {

/// Supplies deterministic symbols, types, prototypes, and locals for the real
/// Sleigh-backed examples used by the provenance tests.
class Metadata final : public SymbolProvider, public TypeProvider, public PrototypeProvider, public VariableProvider {
public:
    /// Stores the metadata records that native Architecture installation reads.
    Metadata(std::vector<SymbolDescription> symbols, std::vector<TypeDescription> types,
             std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes,
             std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> variables = {})
        : symbols_(std::move(symbols)), types_(std::move(types)), prototypes_(std::move(prototypes)),
          variables_(std::move(variables)) {}

    /// Resolves the symbol at one machine address.
    [[nodiscard]] std::optional<SymbolDescription> symbol_at(std::uint64_t address) const override {
        for (const SymbolDescription& symbol : symbols_) {
            if (symbol.address == address) {
                return symbol;
            }
        }
        return std::nullopt;
    }

    /// Returns all symbols that must be installed before native flow recovery.
    [[nodiscard]] std::vector<SymbolDescription> symbols() const override {
        return symbols_;
    }

    /// Resolves one provider-defined type name.
    [[nodiscard]] std::optional<TypeDescription> type_named(std::string_view name) const override {
        for (const TypeDescription& type : types_) {
            if (type.name == name) {
                return type;
            }
        }
        return std::nullopt;
    }

    /// Resolves the prototype attached to one function address.
    [[nodiscard]] std::optional<PrototypeDescription> prototype_at(std::uint64_t address) const override {
        for (const auto& [prototype_address, prototype] : prototypes_) {
            if (prototype_address == address) {
                return prototype;
            }
        }
        return std::nullopt;
    }

    /// Returns provider-owned locals attached to one root function address.
    [[nodiscard]] std::vector<VariableDescription> variables_at(std::uint64_t address) const override {
        for (const auto& [variable_address, variables] : variables_) {
            if (variable_address == address) {
                return variables;
            }
        }
        return {};
    }

private:
    std::vector<SymbolDescription> symbols_;
    std::vector<TypeDescription> types_;
    std::vector<std::pair<std::uint64_t, PrototypeDescription>> prototypes_;
    std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>> variables_;
};

/// Creates the primitive signed integer type used by all compact examples.
static TypeDescription int32_type() {
    return TypeDescription{"int32", 4, {}, TypeKind::signed_integer, true};
}

/// Creates a pointer type whose element is resolved by the native type loader.
static TypeDescription pointer_type(std::string name, std::string element) {
    TypeDescription result;
    result.name = std::move(name);
    result.size = 8;
    result.kind = TypeKind::pointer;
    result.element_type = std::move(element);
    return result;
}

/// Creates the nested structure metadata needed to print `obj->field_0x10.field_0x4`.
static std::vector<TypeDescription> nested_types() {
    TypeDescription inner;
    inner.name = "Inner";
    inner.size = 8;
    inner.declaration = "struct Inner { int32 field_0x0; int32 field_0x4; };";
    inner.kind = TypeKind::structure;
    inner.fields = {TypeFieldDescription{"field_0x0", "int32", 0}, TypeFieldDescription{"field_0x4", "int32", 4}};

    TypeDescription outer;
    outer.name = "Outer";
    outer.size = 0x20;
    outer.declaration = "struct Outer { char padding[0x10]; Inner field_0x10; };";
    outer.kind = TypeKind::structure;
    outer.fields = {TypeFieldDescription{"field_0x10", "Inner", 0x10}};

    return {int32_type(), inner, outer, pointer_type("Outer *", "Outer")};
}

/// Builds the architecture description used by the x86-64 SLA examples.
static ArchitectureDescription x86_architecture() {
    ArchitectureDescription result = make_x86_64_architecture();
    result.calling_convention = "__cdecl";
    return result;
}

/// Converts metadata and bytes into a real Sleigh-to-native decompiler result.
/// The provider only supplies bytes and declarative metadata; instruction
/// decoding, p-code construction, analysis, and markup all remain native.
static DecompilationResult decompile_sleigh(std::uint64_t entry, std::vector<std::uint8_t> image,
                                            std::size_t function_size, std::string name,
                                            std::shared_ptr<Metadata> metadata, bool capture_provenance = true) {
    image.insert(image.end(), 16, 0x90);
    auto memory = std::make_shared<SparseMemory>(entry, std::move(image));
    auto sleigh =
        std::make_shared<SleighPcodeProvider>("x86-64.sla", memory,
                                              std::vector<std::pair<std::string, std::uint64_t>>{
                                                  {"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}});
    ProviderContext providers;
    providers.pcode = std::move(sleigh);
    providers.memory = std::move(memory);
    providers.symbols = metadata;
    providers.types = metadata;
    providers.prototypes = metadata;
    providers.variables = metadata;
    Decompiler decompiler(x86_architecture(), std::move(providers));
    return decompiler.decompile(FunctionDescription{std::move(name), entry, entry + function_size, capture_provenance});
}

/// Finds a native PcodeOp snapshot by the sequence key emitted as `opref`.
static const PcodeOpProvenance* find_operation(const DecompilationResult& result, std::uint64_t sequence) {
    for (const PcodeOpProvenance& operation : result.pcode_provenance) {
        if (operation.sequence == sequence) {
            return &operation;
        }
    }
    return nullptr;
}

/// Finds a native Varnode snapshot by the creation key emitted as `varref`.
static const VarnodeProvenance* find_varnode(const DecompilationResult& result, std::uint32_t create_index) {
    for (const VarnodeProvenance& varnode : result.varnode_provenance) {
        if (varnode.create_index == create_index) {
            return &varnode;
        }
    }
    return nullptr;
}

/// Finds the direct reverse index entry for one originating ASM address.
static const InstructionProvenance* find_instruction(const DecompilationResult& result, std::uint64_t address) {
    for (const InstructionProvenance& instruction : result.instruction_provenance) {
        if (instruction.address == address) {
            return &instruction;
        }
    }
    return nullptr;
}

/// Verifies one structured markup node's complete Clang-to-Pcode-to-ASM path.
static void expect_node_chain(const DecompilationResult& result, const ClangMarkupNodeProvenance& node) {
    ASSERT_TRUE(node.selectable) << "node is not selectable: " << node.element;
    ASSERT_FALSE(node.operation_refs.empty()) << "node lacks operation refs: " << node.content;
    ASSERT_FALSE(node.originating_addresses.empty()) << "node lacks originating addresses: " << node.content;
    for (const std::uint64_t sequence : node.operation_refs) {
        const PcodeOpProvenance* operation = find_operation(result, sequence);
        ASSERT_NE(operation, nullptr) << "missing native PcodeOp for opref " << sequence;
        EXPECT_TRUE(std::ranges::contains(operation->clang_node_ids, node.id));
        EXPECT_NE(find_instruction(result, operation->address), nullptr);
    }
    if (node.varnode_ref.has_value()) {
        const VarnodeProvenance* varnode = find_varnode(result, *node.varnode_ref);
        ASSERT_NE(varnode, nullptr) << "missing native Varnode for varref " << *node.varnode_ref;
        EXPECT_TRUE(std::ranges::contains(varnode->clang_node_ids, node.id));
        EXPECT_TRUE(!varnode->defining_op.has_value() || find_operation(result, *varnode->defining_op) != nullptr);
    }
}

/// Selects structured nodes whose content contains the requested source fragment.
static std::vector<const ClangMarkupNodeProvenance*> nodes_containing(const DecompilationResult& result,
                                                                      std::string_view content) {
    std::vector<const ClangMarkupNodeProvenance*> selected;
    for (const ClangMarkupNodeProvenance& node : result.clang_nodes) {
        if (node.content.find(content) != std::string::npos) {
            selected.push_back(&node);
        }
    }
    return selected;
}

/// Selects structured nodes by element and semantic content.
static std::vector<const ClangMarkupNodeProvenance*>
nodes_with_element(const DecompilationResult& result, std::string_view content, std::string_view element) {
    std::vector<const ClangMarkupNodeProvenance*> selected;
    for (const ClangMarkupNodeProvenance& node : result.clang_nodes) {
        if (node.element == element && node.content.find(content) != std::string::npos) {
            selected.push_back(&node);
        }
    }
    return selected;
}

/// Verifies every independently selected ASM address has decoded PcodeOps.
static void expect_asm_to_pcode(const DecompilationResult& result, std::span<const std::uint64_t> addresses) {
    for (const std::uint64_t address : addresses) {
        ASSERT_TRUE(
            std::ranges::any_of(result.raw_instructions,
                                [address](const Instruction& instruction) { return instruction.address == address; }))
            << "missing decoded instruction at " << std::hex << address;
        const InstructionProvenance* instruction = find_instruction(result, address);
        ASSERT_NE(instruction, nullptr) << "missing reverse provenance at " << std::hex << address;
        EXPECT_FALSE(instruction->pcode_operations.empty()) << "missing PcodeOp at " << std::hex << address;
    }
}

/// Verifies reverse ASM-to-Pcode-to-Clang lookup from independently chosen addresses.
static void expect_asm_to_nodes(const DecompilationResult& result, std::span<const std::uint64_t> addresses,
                                std::string_view expected_content = {}) {
    for (const std::uint64_t address : addresses) {
        const InstructionProvenance* instruction = find_instruction(result, address);
        ASSERT_NE(instruction, nullptr) << "missing reverse provenance at " << std::hex << address;
        ASSERT_FALSE(instruction->pcode_operations.empty());
        bool found_node = false;
        for (const std::uint64_t sequence : instruction->pcode_operations) {
            const PcodeOpProvenance* operation = find_operation(result, sequence);
            ASSERT_NE(operation, nullptr);
            for (const std::uint64_t node_id : operation->clang_node_ids) {
                const auto node =
                    std::ranges::find_if(result.clang_nodes, [node_id](const ClangMarkupNodeProvenance& candidate) {
                        return candidate.id == node_id;
                    });
                ASSERT_NE(node, result.clang_nodes.end());
                if (expected_content.empty() || node->content.find(expected_content) != std::string::npos) {
                    found_node = true;
                }
            }
        }
        EXPECT_TRUE(found_node) << "no expected Clang node mapped to ASM address " << std::hex << address;
    }
}

/// Verifies all references are unique and every node has a stable reverse link.
static void expect_provenance_integrity(const DecompilationResult& result) {
    std::unordered_set<std::uint64_t> operation_ids;
    for (const PcodeOpProvenance& operation : result.pcode_provenance) {
        EXPECT_TRUE(operation_ids.insert(operation.sequence).second);
    }
    std::unordered_set<std::uint32_t> varnode_ids;
    for (const VarnodeProvenance& varnode : result.varnode_provenance) {
        EXPECT_TRUE(varnode_ids.insert(varnode.create_index).second);
    }
    for (std::size_t index = 0; index < result.clang_nodes.size(); ++index) {
        const ClangMarkupNodeProvenance& node = result.clang_nodes[index];
        EXPECT_EQ(node.id, index);
        if (node.primary_operation_ref.has_value()) {
            EXPECT_TRUE(std::ranges::contains(node.operation_refs, *node.primary_operation_ref));
        }
        for (const std::uint64_t sequence : node.operation_refs) {
            const PcodeOpProvenance* operation = find_operation(result, sequence);
            ASSERT_NE(operation, nullptr);
            EXPECT_TRUE(std::ranges::contains(operation->clang_node_ids, node.id));
        }
    }
    for (const InstructionProvenance& instruction : result.instruction_provenance) {
        EXPECT_FALSE(instruction.pcode_operations.empty());
        for (const std::uint64_t sequence : instruction.pcode_operations) {
            const PcodeOpProvenance* operation = find_operation(result, sequence);
            ASSERT_NE(operation, nullptr);
            EXPECT_EQ(operation->address, instruction.address);
        }
        const auto decoded =
            std::ranges::find_if(result.raw_instructions, [&instruction](const Instruction& candidate) {
                return candidate.address == instruction.address;
            });
        if (decoded != result.raw_instructions.end()) {
            EXPECT_EQ(instruction.length, decoded->length);
        } else {
            EXPECT_EQ(instruction.length, 0U);
        }
    }
}

/// Creates the nested aggregate load/store fixture adapted from the existing
/// `ProviderAggregateFieldsAndArrayElement` real x86 test.
static DecompilationResult nested_member_result(bool capture_provenance = true) {
    constexpr std::uint64_t entry = 0x500000;
    const std::vector<std::uint8_t> bytes{
        0x8b, 0x41, 0x14, // mov eax, [rcx+0x14]: obj->field_0x10.field_0x4
        0x83, 0xc0, 0x01, // add eax, 1
        0x89, 0x41, 0x14, // mov [rcx+0x14], eax: assignment through the nested field
        0xc3,             // return
    };
    PrototypeDescription prototype;
    prototype.calling_convention = "__cdecl";
    prototype.return_type = "int32";
    prototype.return_storage = Storage{"register", 0, 4};
    prototype.parameters = {PrototypeParameterDescription{"obj", "Outer *", Storage{"register", 8, 8}}};
    auto metadata =
        std::make_shared<Metadata>(std::vector<SymbolDescription>{{entry, "nested_member", ""}}, nested_types(),
                                   std::vector<std::pair<std::uint64_t, PrototypeDescription>>{{entry, prototype}});
    return decompile_sleigh(entry, bytes, bytes.size(), "nested_member", std::move(metadata), capture_provenance);
}

/// Creates a counted loop with a real stack local and a register parameter.
/// Original loop structuring source: `Ghidra/Features/Decompiler/src/decompile/cpp/block.cc`.
static DecompilationResult loop_result() {
    constexpr std::uint64_t entry = 0x510000;
    const std::vector<std::uint8_t> bytes{
        0x89, 0x4c, 0x24, 0xfc, // mov [rsp-4], ecx: initialize local_counter
        0x8b, 0x44, 0x24, 0xfc, // mov eax, [rsp-4]: loop condition
        0x85, 0xc0,             // test eax, eax
        0x7e, 0x0b,             // jle exit
        0x83, 0xe8, 0x01,       // sub eax, 1
        0x89, 0x44, 0x24, 0xfc, // mov [rsp-4], eax
        0xeb, 0xef,             // jmp loop condition
        0x8b, 0x44, 0x24, 0xfc, // mov eax, [rsp-4]: return local_counter
        0xc3,                   // ret
    };
    PrototypeDescription prototype;
    prototype.calling_convention = "__cdecl";
    prototype.return_type = "int32";
    prototype.return_storage = Storage{"register", 0, 4};
    prototype.parameters = {PrototypeParameterDescription{"count", "int32", Storage{"register", 8, 4}}};
    auto metadata = std::make_shared<Metadata>(
        std::vector<SymbolDescription>{{entry, "count_loop", ""}}, std::vector<TypeDescription>{int32_type()},
        std::vector<std::pair<std::uint64_t, PrototypeDescription>>{{entry, prototype}},
        std::vector<std::pair<std::uint64_t, std::vector<VariableDescription>>>{
            {entry,
             {VariableDescription{"local_counter", "int32", Storage{"stack", static_cast<std::uint64_t>(-4), 4}}}}});
    return decompile_sleigh(entry, bytes, bytes.size(), "count_loop", std::move(metadata));
}

/// Creates a real conditional branch with arithmetic on both sides.
static DecompilationResult if_else_result() {
    constexpr std::uint64_t entry = 0x520000;
    const std::vector<std::uint8_t> bytes{
        0x85, 0xc9,       // test ecx, ecx
        0x74, 0x05,       // jz else
        0x83, 0xc1, 0x01, // add ecx, 1
        0xeb, 0x03,       // jump join
        0x83, 0xe9, 0x01, // else: sub ecx, 1
        0x89, 0xc8,       // join: mov eax, ecx
        0xc3,             // ret
    };
    PrototypeDescription prototype;
    prototype.calling_convention = "__cdecl";
    prototype.return_type = "int32";
    prototype.return_storage = Storage{"register", 0, 4};
    prototype.parameters = {PrototypeParameterDescription{"value", "int32", Storage{"register", 8, 4}}};
    auto metadata = std::make_shared<Metadata>(
        std::vector<SymbolDescription>{{entry, "if_else_arithmetic", ""}}, std::vector<TypeDescription>{int32_type()},
        std::vector<std::pair<std::uint64_t, PrototypeDescription>>{{entry, prototype}});
    return decompile_sleigh(entry, bytes, bytes.size(), "if_else_arithmetic", std::move(metadata));
}

/// Creates the repository's compact native indirect jump-table switch pattern.
/// Original fixture: `Ghidra/Features/Decompiler/src/decompile/datatests/switchmulti.xml`.
static DecompilationResult switch_result() {
    constexpr std::uint64_t entry = 0x530000;
    constexpr std::size_t table_offset = 0x40;
    const std::vector<std::uint64_t> targets{entry + 0x0b, entry + 0x11, entry + 0x17, entry + 0x1d};
    std::vector<std::uint8_t> image{
        0x48, 0x83, 0xe1, 0x03,                // and rcx, 3
        0xff, 0x24, 0xcd, 0,    0,    0,    0, // jmp [rcx*8+table]
        0x8b, 0xc2, 0x83, 0xc0, 0x01, 0xc3,    // case 0: return value + 1
        0x8b, 0xc2, 0x83, 0xe8, 0x02, 0xc3,    // case 1: return value - 2
        0x8b, 0xc2, 0x83, 0xf0, 0x03, 0xc3,    // case 2: return value ^ 3
        0x8b, 0xc2, 0x83, 0xc8, 0x04, 0xc3,    // case 3: return value | 4
    };
    image.resize(table_offset, 0x90);
    const std::uint32_t table_address = static_cast<std::uint32_t>(entry + table_offset);
    for (unsigned byte = 0; byte < sizeof(table_address); ++byte) {
        image[7 + byte] = static_cast<std::uint8_t>(table_address >> (byte * 8U));
    }
    for (std::size_t index = 0; index < targets.size(); ++index) {
        for (unsigned byte = 0; byte < sizeof(std::uint64_t); ++byte) {
            image.push_back(static_cast<std::uint8_t>(targets[index] >> (byte * 8U)));
        }
    }
    PrototypeDescription prototype;
    prototype.calling_convention = "__cdecl";
    prototype.return_type = "int32";
    prototype.return_storage = Storage{"register", 0, 4};
    prototype.parameters = {PrototypeParameterDescription{"selector", "int32", Storage{"register", 8, 4}},
                            PrototypeParameterDescription{"value", "int32", Storage{"register", 0x10, 4}}};
    auto metadata = std::make_shared<Metadata>(
        std::vector<SymbolDescription>{{entry, "arithmetic_switch", ""}}, std::vector<TypeDescription>{int32_type()},
        std::vector<std::pair<std::uint64_t, PrototypeDescription>>{{entry, prototype}});
    return decompile_sleigh(entry, image, 0x23, "arithmetic_switch", std::move(metadata));
}

/// Creates a direct call to a separately named function using real x86 CALL.
static DecompilationResult call_result() {
    constexpr std::uint64_t entry = 0x540000;
    constexpr std::uint64_t callee = entry + 0x0a;
    const std::vector<std::uint8_t> bytes{
        0xe8, 0x05, 0x00, 0x00, 0x00, // call callee
        0xc3,                         // return call result
        0x90, 0x90, 0x90, 0x90,       // gap
        0xb8, 0x2a, 0x00, 0x00, 0x00, // callee: return 42
        0xc3,
    };
    PrototypeDescription root_prototype;
    root_prototype.calling_convention = "__cdecl";
    root_prototype.return_type = "int32";
    root_prototype.return_storage = Storage{"register", 0, 4};
    PrototypeDescription callee_prototype = root_prototype;
    auto metadata =
        std::make_shared<Metadata>(std::vector<SymbolDescription>{{entry, "caller", ""}, {callee, "callee", ""}},
                                   std::vector<TypeDescription>{int32_type()},
                                   std::vector<std::pair<std::uint64_t, PrototypeDescription>>{
                                       {entry, root_prototype}, {callee, callee_prototype}});
    return decompile_sleigh(entry, bytes, 6, "caller", std::move(metadata));
}

/// Verifies both directions for markup tokens, analyzed p-code, Varnodes, and
/// original ASM addresses across all requested source constructs.
/// Original markup contracts: `Ghidra/Features/Decompiler/src/decompile/cpp/prettyprint.cc`
/// (`EmitMarkup::beginStatement`, `tagVariable`, `tagField`, `tagOp`, `tagCaseLabel`).
TEST(DecompilerProvenance, BidirectionalSleighMarkupAndNativeGraph) {
    const DecompilationResult legacy = nested_member_result(false);
    EXPECT_TRUE(legacy.clang_markup.empty());
    EXPECT_TRUE(legacy.clang_nodes.empty());
    EXPECT_TRUE(legacy.instruction_provenance.empty());

    const DecompilationResult nested = nested_member_result();
    const DecompilationResult loop = loop_result();
    const DecompilationResult conditional = if_else_result();
    const DecompilationResult switched = switch_result();
    const DecompilationResult called = call_result();

    for (const DecompilationResult* result : {&nested, &loop, &conditional, &switched, &called}) {
        ASSERT_FALSE(result->clang_markup.empty());
        ASSERT_FALSE(result->clang_nodes.empty());
        ASSERT_FALSE(result->pcode_provenance.empty());
        ASSERT_FALSE(result->varnode_provenance.empty());
        ASSERT_FALSE(result->instruction_provenance.empty());
        expect_provenance_integrity(*result);
        for (const ClangMarkupNodeProvenance& node : result->clang_nodes) {
            if (node.selectable && !node.operation_refs.empty()) {
                expect_node_chain(*result, node);
            }
        }
    }

    const std::vector<const ClangMarkupNodeProvenance*> outer_fields =
        nodes_with_element(nested, "field_0x10", "field");
    const std::vector<const ClangMarkupNodeProvenance*> inner_fields = nodes_with_element(nested, "field_0x4", "field");
    ASSERT_FALSE(outer_fields.empty());
    ASSERT_FALSE(inner_fields.empty());
    for (const ClangMarkupNodeProvenance* node : outer_fields) {
        ASSERT_TRUE(node->field_offset.has_value());
        EXPECT_EQ(*node->field_offset, 0x10);
        expect_node_chain(nested, *node);
    }
    for (const ClangMarkupNodeProvenance* node : inner_fields) {
        ASSERT_TRUE(node->field_offset.has_value());
        EXPECT_EQ(*node->field_offset, 4);
        expect_node_chain(nested, *node);
    }
    const std::vector<const ClangMarkupNodeProvenance*> object_tokens = nodes_with_element(nested, "obj", "variable");
    ASSERT_FALSE(object_tokens.empty());
    bool saw_object_varnode = false;
    for (const ClangMarkupNodeProvenance* node : object_tokens) {
        if (!node->varnode_ref.has_value()) {
            continue;
        }
        const VarnodeProvenance* varnode = find_varnode(nested, *node->varnode_ref);
        ASSERT_NE(varnode, nullptr);
        saw_object_varnode = true;
        EXPECT_EQ(varnode->high_variable_name, "obj");
        expect_node_chain(nested, *node);
    }
    ASSERT_TRUE(saw_object_varnode);
    expect_asm_to_pcode(nested, std::array<std::uint64_t, 3>{0x500000, 0x500003, 0x500006});
    expect_asm_to_nodes(nested, std::array<std::uint64_t, 3>{0x500000, 0x500003, 0x500006});

    const std::vector<const ClangMarkupNodeProvenance*> loop_keywords = nodes_with_element(loop, "for", "op");
    ASSERT_FALSE(loop_keywords.empty());
    for (const ClangMarkupNodeProvenance* node : loop_keywords) {
        expect_node_chain(loop, *node);
    }
    const std::vector<const ClangMarkupNodeProvenance*> local_tokens =
        nodes_with_element(loop, "local_counter", "variable");
    ASSERT_FALSE(local_tokens.empty());
    for (const ClangMarkupNodeProvenance* node : local_tokens) {
        if (!node->operation_refs.empty()) {
            expect_node_chain(loop, *node);
        }
    }
    expect_asm_to_pcode(loop, std::array<std::uint64_t, 3>{0x510000, 0x51000a, 0x51000c});
    expect_asm_to_nodes(loop, std::array<std::uint64_t, 3>{0x510000, 0x51000a, 0x51000c});

    const std::vector<const ClangMarkupNodeProvenance*> if_tokens = nodes_with_element(conditional, "if", "op");
    ASSERT_FALSE(if_tokens.empty());
    for (const ClangMarkupNodeProvenance* node : if_tokens) {
        expect_node_chain(conditional, *node);
    }
    const std::vector<const ClangMarkupNodeProvenance*> arithmetic_tokens = nodes_with_element(conditional, "+", "op");
    ASSERT_FALSE(arithmetic_tokens.empty());
    for (const ClangMarkupNodeProvenance* node : arithmetic_tokens) {
        expect_node_chain(conditional, *node);
    }
    expect_asm_to_pcode(conditional, std::array<std::uint64_t, 3>{0x520000, 0x520004, 0x520009});
    expect_asm_to_nodes(conditional, std::array<std::uint64_t, 3>{0x520000, 0x520004, 0x520009});

    const std::vector<const ClangMarkupNodeProvenance*> switch_keywords = nodes_with_element(switched, "switch", "op");
    ASSERT_FALSE(switch_keywords.empty());
    for (const ClangMarkupNodeProvenance* node : switch_keywords) {
        expect_node_chain(switched, *node);
    }
    const std::vector<const ClangMarkupNodeProvenance*> case_tokens = nodes_with_element(switched, "", "value");
    ASSERT_GE(case_tokens.size(), 2U);
    for (const ClangMarkupNodeProvenance* node : case_tokens) {
        expect_node_chain(switched, *node);
    }
    const std::vector<const ClangMarkupNodeProvenance*> case_keywords = nodes_with_element(switched, "case", "syntax");
    ASSERT_FALSE(case_keywords.empty());
    for (const ClangMarkupNodeProvenance* node : case_keywords) {
        expect_node_chain(switched, *node);
    }
    expect_asm_to_pcode(switched, std::array<std::uint64_t, 3>{0x530000, 0x530004, 0x53000d});
    expect_asm_to_nodes(switched, std::array<std::uint64_t, 3>{0x530000, 0x530004, 0x53000d});

    const std::vector<const ClangMarkupNodeProvenance*> callee_tokens =
        nodes_with_element(called, "callee", "funcname");
    ASSERT_FALSE(callee_tokens.empty());
    for (const ClangMarkupNodeProvenance* node : callee_tokens) {
        expect_node_chain(called, *node);
    }
    expect_asm_to_pcode(called, std::array<std::uint64_t, 1>{0x540000});
    expect_asm_to_nodes(called, std::array<std::uint64_t, 1>{0x540000}, "callee");
}

} // namespace newghidra::decompiler::provenance_tests
