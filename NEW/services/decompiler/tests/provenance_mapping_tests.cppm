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
                                            std::shared_ptr<Metadata> metadata) {
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
    return decompiler.decompile(FunctionDescription{std::move(name), entry, entry + function_size});
}

/// Parses native Clang XML token records without flattening them to C text.
struct MarkupToken {
    std::string element;
    std::string content;
    std::optional<std::uint64_t> operation;
    std::optional<std::uint32_t> varnode;
    std::optional<std::int64_t> field_offset;
};

/// Returns one quoted XML attribute from native markup.
static std::optional<std::string> markup_attribute(std::string_view tag, std::string_view name) {
    const std::string needle = std::string(name) + "=\"";
    const std::size_t start = tag.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }
    const std::size_t value_start = start + needle.size();
    const std::size_t value_end = tag.find('"', value_start);
    if (value_end == std::string_view::npos) {
        return std::nullopt;
    }
    return std::string(tag.substr(value_start, value_end - value_start));
}

/// Decodes the five XML entities that can occur in token content attributes.
static std::string decode_markup_entities(std::string value) {
    const std::array<std::pair<std::string_view, std::string_view>, 5> entities{
        std::pair{"&lt;", "<"}, std::pair{"&gt;", ">"}, std::pair{"&amp;", "&"}, std::pair{"&quot;", "\""},
        std::pair{"&apos;", "'"}};
    for (const auto& [encoded, decoded] : entities) {
        std::size_t offset = 0;
        while ((offset = value.find(encoded, offset)) != std::string::npos) {
            value.replace(offset, encoded.size(), decoded);
            offset += decoded.size();
        }
    }
    return value;
}

/// Parses an unsigned integer attribute without depending on generated C text.
static std::optional<std::uint64_t> unsigned_markup_attribute(std::string_view tag, std::string_view name) {
    const std::optional<std::string> value = markup_attribute(tag, name);
    if (!value) {
        return std::nullopt;
    }
    std::string_view digits = *value;
    int base = 10;
    if (digits.starts_with("0x")) {
        digits.remove_prefix(2);
        base = 16;
    }
    std::uint64_t result = 0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), result, base);
    return error == std::errc{} && end == digits.data() + digits.size() ? std::optional{result} : std::nullopt;
}

/// Parses a signed integer field offset from native markup.
static std::optional<std::int64_t> signed_markup_attribute(std::string_view tag, std::string_view name) {
    const std::optional<std::string> value = markup_attribute(tag, name);
    if (!value) {
        return std::nullopt;
    }
    std::int64_t result = 0;
    const auto [end, error] = std::from_chars(value->data(), value->data() + value->size(), result);
    return error == std::errc{} && end == value->data() + value->size() ? std::optional{result} : std::nullopt;
}

/// Parses native Clang XML markup while retaining statement-level `opref` inheritance.
/// The parser intentionally consumes markup records, not rendered C source; it
/// recognizes the compact XML encoder contract emitted by `EmitMarkup`.
static std::vector<MarkupToken> parse_markup(const DecompilationResult& result) {
    const std::string_view markup = result.clang_markup;
    std::vector<MarkupToken> tokens;
    std::vector<std::optional<std::uint64_t>> operation_stack;
    std::vector<std::optional<std::size_t>> token_stack;
    std::size_t cursor = 0;
    while (cursor < markup.size()) {
        const std::size_t open = markup.find('<', cursor);
        if (open == std::string_view::npos) {
            break;
        }
        if (open > cursor && !token_stack.empty() && token_stack.back().has_value()) {
            tokens[*token_stack.back()].content +=
                decode_markup_entities(std::string(markup.substr(cursor, open - cursor)));
        }
        const std::size_t close = markup.find('>', open + 1);
        if (close == std::string_view::npos) {
            ADD_FAILURE() << "unterminated Clang markup tag";
            return {};
        }
        const std::string_view tag = markup.substr(open + 1, close - open - 1);
        cursor = close + 1;
        if (tag.empty() || tag.front() == '?' || tag.front() == '!') {
            continue;
        }
        if (tag.front() == '/') {
            if (!operation_stack.empty()) {
                operation_stack.pop_back();
            }
            if (!token_stack.empty()) {
                token_stack.pop_back();
            }
            continue;
        }

        std::size_t name_end = 0;
        while (name_end < tag.size() && tag[name_end] != ' ' && tag[name_end] != '\t' && tag[name_end] != '/') {
            ++name_end;
        }
        const std::string_view element = tag.substr(0, name_end);
        std::optional<std::uint64_t> operation = operation_stack.empty() ? std::nullopt : operation_stack.back();
        if (const std::optional<std::uint64_t> own_operation = unsigned_markup_attribute(tag, "opref"); own_operation) {
            operation = own_operation;
        }
        const bool token_element = element == "variable" || element == "field" || element == "bitfield" ||
                                   element == "op" || element == "funcname" || element == "value" ||
                                   element == "syntax" || element == "type";
        if (token_element) {
            MarkupToken token{std::string(element), {}, operation, std::nullopt, std::nullopt};
            if (const std::optional<std::string> content = markup_attribute(tag, "content"); content) {
                token.content = decode_markup_entities(*content);
            }
            if (const std::optional<std::uint64_t> varnode = unsigned_markup_attribute(tag, "varref"); varnode) {
                token.varnode = static_cast<std::uint32_t>(*varnode);
            }
            token.field_offset = signed_markup_attribute(tag, "off");
            tokens.push_back(std::move(token));
        }
        if (tag.empty() || tag.back() != '/') {
            operation_stack.push_back(operation);
            token_stack.push_back(token_element ? std::optional{tokens.size() - 1} : std::nullopt);
        }
    }
    return tokens;
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

/// Returns whether an analyzed PcodeOp is attached to a decoded ASM address.
static bool has_operation_at(const DecompilationResult& result, std::uint64_t address) {
    return std::ranges::any_of(result.pcode_provenance,
                               [address](const PcodeOpProvenance& operation) { return operation.address == address; });
}

/// Verifies one token's complete Clang-to-Pcode-to-ASM path.
static void expect_token_chain(const DecompilationResult& result, const std::vector<MarkupToken>& tokens,
                               const MarkupToken& token) {
    ASSERT_TRUE(token.operation.has_value()) << "token lacks an opref: " << token.content;
    const PcodeOpProvenance* operation = find_operation(result, *token.operation);
    ASSERT_NE(operation, nullptr) << "missing native PcodeOp for opref " << *token.operation;
    ASSERT_TRUE(has_operation_at(result, operation->address));
    if (token.varnode.has_value()) {
        const VarnodeProvenance* varnode = find_varnode(result, *token.varnode);
        ASSERT_NE(varnode, nullptr) << "missing native Varnode for varref " << *token.varnode;
        EXPECT_TRUE(!varnode->defining_op.has_value() || find_operation(result, *varnode->defining_op) != nullptr);
    }
    (void)tokens;
}

/// Selects tokens whose content contains the requested semantic source fragment.
static std::vector<const MarkupToken*> tokens_containing(const std::vector<MarkupToken>& tokens,
                                                         std::string_view content) {
    std::vector<const MarkupToken*> result;
    for (const MarkupToken& token : tokens) {
        if (token.content.find(content) != std::string::npos) {
            result.push_back(&token);
        }
    }
    return result;
}

/// Selects semantic markup tokens while excluding declaration text with the same spelling.
static std::vector<const MarkupToken*> tokens_with_element(const std::vector<MarkupToken>& tokens,
                                                           std::string_view content, std::string_view element) {
    std::vector<const MarkupToken*> result;
    for (const MarkupToken& token : tokens) {
        if (token.element == element && token.content.find(content) != std::string::npos) {
            result.push_back(&token);
        }
    }
    return result;
}

/// Extracts unique originating ASM addresses from selected mapped tokens.
static std::vector<std::uint64_t> addresses_for_tokens(const DecompilationResult& result,
                                                       std::span<const MarkupToken* const> tokens) {
    std::vector<std::uint64_t> addresses;
    for (const MarkupToken* token : tokens) {
        if (!token->operation.has_value()) {
            continue;
        }
        const PcodeOpProvenance* operation = find_operation(result, *token->operation);
        if (operation != nullptr && !std::ranges::contains(addresses, operation->address)) {
            addresses.push_back(operation->address);
        }
    }
    return addresses;
}

/// Verifies every decoded instruction in a selected construct has native p-code.
static void expect_asm_to_pcode(const DecompilationResult& result, std::span<const std::uint64_t> addresses) {
    for (const std::uint64_t address : addresses) {
        ASSERT_TRUE(
            std::ranges::any_of(result.raw_instructions,
                                [address](const Instruction& instruction) { return instruction.address == address; }))
            << "missing decoded instruction at " << std::hex << address;
        EXPECT_TRUE(has_operation_at(result, address)) << "missing PcodeOp at " << std::hex << address;
    }
}

/// Verifies reverse ASM-to-token lookup through native PcodeOp sequence IDs.
static void expect_asm_to_tokens(const DecompilationResult& result, const std::vector<MarkupToken>& tokens,
                                 std::span<const std::uint64_t> addresses) {
    for (const std::uint64_t address : addresses) {
        bool found = false;
        for (const MarkupToken& token : tokens) {
            if (!token.operation.has_value()) {
                continue;
            }
            const PcodeOpProvenance* operation = find_operation(result, *token.operation);
            if (operation != nullptr && operation->address == address) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "no Clang token mapped to ASM address " << std::hex << address;
    }
}

/// Creates the nested aggregate load/store fixture adapted from the existing
/// `ProviderAggregateFieldsAndArrayElement` real x86 test.
static DecompilationResult nested_member_result() {
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
    return decompile_sleigh(entry, bytes, bytes.size(), "nested_member", std::move(metadata));
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
    const DecompilationResult nested = nested_member_result();
    const DecompilationResult loop = loop_result();
    const DecompilationResult conditional = if_else_result();
    const DecompilationResult switched = switch_result();
    const DecompilationResult called = call_result();

    for (const DecompilationResult* result : {&nested, &loop, &conditional, &switched, &called}) {
        ASSERT_FALSE(result->clang_markup.empty());
        ASSERT_FALSE(result->pcode_provenance.empty());
        ASSERT_FALSE(result->varnode_provenance.empty());
        const std::vector<MarkupToken> tokens = parse_markup(*result);
        ASSERT_FALSE(tokens.empty());
        for (const MarkupToken& token : tokens) {
            if (token.operation.has_value()) {
                expect_token_chain(*result, tokens, token);
            }
        }
    }

    const std::vector<MarkupToken> nested_tokens = parse_markup(nested);
    const std::vector<const MarkupToken*> outer_fields = tokens_containing(nested_tokens, "field_0x10");
    const std::vector<const MarkupToken*> inner_fields = tokens_containing(nested_tokens, "field_0x4");
    ASSERT_FALSE(outer_fields.empty());
    ASSERT_FALSE(inner_fields.empty());
    for (const MarkupToken* token : outer_fields) {
        ASSERT_EQ(token->element, "field");
        ASSERT_TRUE(token->field_offset.has_value());
        EXPECT_EQ(*token->field_offset, 0x10);
        expect_token_chain(nested, nested_tokens, *token);
    }
    for (const MarkupToken* token : inner_fields) {
        ASSERT_EQ(token->element, "field");
        ASSERT_TRUE(token->field_offset.has_value());
        EXPECT_EQ(*token->field_offset, 4);
        expect_token_chain(nested, nested_tokens, *token);
    }
    const std::vector<const MarkupToken*> object_tokens = tokens_containing(nested_tokens, "obj");
    ASSERT_FALSE(object_tokens.empty());
    bool saw_object_varnode = false;
    for (const MarkupToken* token : object_tokens) {
        if (token->element != "variable") {
            continue;
        }
        if (!token->varnode.has_value()) {
            continue;
        }
        const VarnodeProvenance* varnode = find_varnode(nested, *token->varnode);
        ASSERT_NE(varnode, nullptr);
        saw_object_varnode = true;
        EXPECT_EQ(varnode->high_variable_name, "obj");
    }
    ASSERT_TRUE(saw_object_varnode);
    expect_asm_to_pcode(nested, std::array<std::uint64_t, 3>{0x500000, 0x500003, 0x500006});
    expect_asm_to_tokens(nested, nested_tokens, std::array<std::uint64_t, 3>{0x500000, 0x500003, 0x500006});

    const std::vector<MarkupToken> loop_tokens = parse_markup(loop);
    const std::vector<const MarkupToken*> loop_keywords = tokens_containing(loop_tokens, "for");
    ASSERT_FALSE(loop_keywords.empty());
    for (const MarkupToken* token : loop_keywords) {
        expect_token_chain(loop, loop_tokens, *token);
    }
    const std::vector<const MarkupToken*> local_tokens = tokens_containing(loop_tokens, "local_counter");
    ASSERT_FALSE(local_tokens.empty());
    const std::vector<std::uint64_t> loop_addresses = addresses_for_tokens(loop, loop_keywords);
    ASSERT_FALSE(loop_addresses.empty());
    expect_asm_to_pcode(loop, loop_addresses);
    expect_asm_to_tokens(loop, loop_tokens, loop_addresses);

    const std::vector<MarkupToken> conditional_tokens = parse_markup(conditional);
    const std::vector<const MarkupToken*> if_tokens = tokens_with_element(conditional_tokens, "if", "op");
    ASSERT_FALSE(if_tokens.empty());
    for (const MarkupToken* token : if_tokens) {
        expect_token_chain(conditional, conditional_tokens, *token);
    }
    const std::vector<const MarkupToken*> arithmetic_tokens = tokens_with_element(conditional_tokens, "+", "op");
    ASSERT_FALSE(arithmetic_tokens.empty());
    const std::vector<std::uint64_t> conditional_addresses = addresses_for_tokens(conditional, if_tokens);
    ASSERT_FALSE(conditional_addresses.empty());
    expect_asm_to_pcode(conditional, conditional_addresses);
    expect_asm_to_tokens(conditional, conditional_tokens, conditional_addresses);

    const std::vector<MarkupToken> switch_tokens = parse_markup(switched);
    const std::vector<const MarkupToken*> switch_keywords = tokens_with_element(switch_tokens, "switch", "op");
    ASSERT_FALSE(switch_keywords.empty());
    for (const MarkupToken* token : switch_keywords) {
        expect_token_chain(switched, switch_tokens, *token);
    }
    const std::vector<const MarkupToken*> case_tokens = tokens_with_element(switch_tokens, "", "value");
    ASSERT_GE(case_tokens.size(), 2U);
    for (const MarkupToken* token : case_tokens) {
        expect_token_chain(switched, switch_tokens, *token);
    }
    const std::vector<std::uint64_t> switch_addresses = addresses_for_tokens(switched, case_tokens);
    ASSERT_FALSE(switch_addresses.empty());
    expect_asm_to_pcode(switched, switch_addresses);
    expect_asm_to_tokens(switched, switch_tokens, switch_addresses);

    const std::vector<MarkupToken> call_tokens = parse_markup(called);
    const std::vector<const MarkupToken*> callee_tokens = tokens_containing(call_tokens, "callee");
    ASSERT_FALSE(callee_tokens.empty());
    for (const MarkupToken* token : callee_tokens) {
        ASSERT_EQ(token->element, "funcname");
        expect_token_chain(called, call_tokens, *token);
    }
    const std::vector<std::uint64_t> call_addresses = addresses_for_tokens(called, callee_tokens);
    ASSERT_FALSE(call_addresses.empty());
    expect_asm_to_pcode(called, call_addresses);
    expect_asm_to_tokens(called, call_tokens, call_addresses);
}

} // namespace newghidra::decompiler::provenance_tests
