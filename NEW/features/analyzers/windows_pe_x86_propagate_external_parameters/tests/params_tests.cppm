module;

#include <gtest/gtest.h>

export module windows_pe_x86_propagate_external_parameters_tests;

import analyzer_test_support;
import analyzer_reference;
import analyzer_windows_pe_x86_propagate_external_parameters;
import std;

namespace ghidra::analyzer::tests {
namespace {

/// Verifies known external parameters are attached to the last matching PUSH instructions.
TEST(WindowsPeX86PropagateExternalParametersAnalyzerTest, RecordsKnownParameterEvidence) {
    auto context = load_fixture("windows_pe_x86_propagate_external_parameters");
    context.options().seed_provider_functions = false;
    const auto entry = context.image().entry_point_va();
    ASSERT_TRUE(entry.has_value());
    ASSERT_GT(context.disassemble_flow(*entry), 0U);
    const auto external = std::find_if(context.external_symbols().begin(), context.external_symbols().end(),
                                       [](const ExternalSymbol& symbol) { return symbol.name == "MessageBoxA"; });
    ASSERT_NE(external, context.external_symbols().end());
    sleigh_runtime::Instruction external_stub;
    external_stub.address = external->iat_address;
    external_stub.length = 1;
    external_stub.mnemonic = "NOP";
    ASSERT_TRUE(context.define_instruction(std::move(external_stub)));
    ASSERT_TRUE(context.create_function(*entry));
    ASSERT_TRUE(context.create_function(external->iat_address));
    ASSERT_TRUE(context.create_function(0x401010ULL, "propagate_parameters"));
    ASSERT_TRUE(context.set_function_signature(external->iat_address, "__stdcall", "int32",
                                               {FunctionParameter{"type", "uint32", "stack", 0, 4, false},
                                                FunctionParameter{"caption", "char *", "stack", 4, 4, false},
                                                FunctionParameter{"text", "char *", "stack", 8, 4, false},
                                                FunctionParameter{"window", "void *", "stack", 12, 4, false}},
                                               false));
    CancellationToken cancellation;
    ReferenceAnalyzer reference_analyzer;
    reference_analyzer.analyze(context, {}, cancellation);
    // The native PE model exposes the IAT address while this fixture's import thunk is not
    // represented as an external symbol. Add the public direct-call relation explicitly.
    ASSERT_TRUE(context.add_reference(Reference{0x401021ULL, external->iat_address, ReferenceKind::unconditional_call,
                                                std::nullopt, std::nullopt, FlowOverride::none, true}));
    WindowsPeX86PropagateExternalParametersAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);

    EXPECT_EQ(std::count_if(context.bookmarks().begin(), context.bookmarks().end(),
                            [](const Bookmark& bookmark) { return bookmark.category == "External Parameter"; }),
              4U);
}

/// Verifies missing public external signatures result in no invented parameter comments or types.
TEST(WindowsPeX86PropagateExternalParametersAnalyzerTest, DoesNotInventMissingSignatures) {
    auto context = load_fixture("windows_pe_x86_propagate_external_parameters");
    CancellationToken cancellation;
    WindowsPeX86PropagateExternalParametersAnalyzer analyzer;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_TRUE(context.bookmarks().empty());
    EXPECT_TRUE(context.symbols().empty());
}

} // namespace
} // namespace ghidra::analyzer::tests
