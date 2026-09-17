module;

#include <gtest/gtest.h>

export module demangler_microsoft_tests;

import analyzer_demangler_microsoft;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Verifies member and static MSVC signatures, including the implicit member receiver.
TEST(MicrosoftDemanglerTest, ParsesMemberAndStaticFunctions) {
    const MicrosoftDemangler demangler;
    const auto member = demangler.demangle("?add@Calculator@fixture@@QEAAHHH@Z");
    ASSERT_TRUE(member.valid);
    EXPECT_EQ(member.short_name, "add");
    EXPECT_EQ(member.namespace_name, "fixture::Calculator");
    EXPECT_EQ(member.calling_convention, "__thiscall");
    EXPECT_EQ(member.return_type, "int");
    ASSERT_EQ(member.parameter_types.size(), 3U);
    EXPECT_EQ(member.parameter_types[0], "Calculator *");
    EXPECT_EQ(member.parameter_types[1], "int");
    EXPECT_EQ(member.parameter_types[2], "int");

    const auto static_member = demangler.demangle("?scale@Calculator@fixture@@SAHH@Z");
    ASSERT_TRUE(static_member.valid);
    EXPECT_EQ(static_member.calling_convention, "__cdecl");
    EXPECT_EQ(static_member.parameter_types, std::vector<std::string>{"int"});
}

/// Verifies the analyzer applies the fixture's genuine decorated export names and signatures.
TEST(MicrosoftDemanglerAnalyzerTest, AppliesFixtureNamesAndSignatures) {
    auto context = load_fixture("demangler_microsoft");
    context.options().seed_provider_functions = false;
    for (const auto& exported : context.image().exported_symbols()) {
        if (!exported.forwarded && context.image().is_executable(exported.address_va)) {
            static_cast<void>(context.disassemble_flow(exported.address_va));
            static_cast<void>(context.create_function(exported.address_va));
        }
    }
    CancellationToken cancellation;
    DemanglerMicrosoftAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    const auto add = std::find_if(context.functions().begin(), context.functions().end(),
                                  [](const auto& item) { return item.second.name == "add"; });
    ASSERT_NE(add, context.functions().end());
    EXPECT_EQ(add->second.calling_convention, "__thiscall");
    ASSERT_EQ(add->second.parameters.size(), 3U);
    EXPECT_EQ(add->second.parameters[0].name, "this");
    EXPECT_EQ(add->second.parameters[0].type, "Calculator *");
    EXPECT_EQ(add->second.parameters[1].type, "int");
    EXPECT_TRUE(std::any_of(context.symbols().begin(), context.symbols().end(), [](const SymbolRecord& symbol) {
        return symbol.mangled_name == "?scale@Calculator@fixture@@SAHH@Z" && symbol.demangled_name == "scale";
    }));
}

} // namespace
} // namespace recode::analyzer::tests
