module;

#include <gtest/gtest.h>

export module analyzer_decompiler_parameter_id_tests;

import analyzer_decompiler_parameter_id;
import analyzer_test_support;
import std;

/// Verifies that the analyzer invokes the native decompiler on a real bounded fixture function.
TEST(DecompilerParameterId, CompletesNativeParameterPass) {
    auto context = ghidra::analyzer::tests::load_fixture("decompiler_parameter_id");
    context.options().decompiler_parameter_id = true;
    ASSERT_TRUE(context.disassemble_flow(0x140001000ULL));
    ASSERT_TRUE(context.create_function(0x140001000ULL, "parameter_target"));
    ghidra::analyzer::DecompilerParameterIdAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    const auto* function = context.function_at(0x140001000ULL);
    ASSERT_NE(function, nullptr);
    EXPECT_TRUE(function->parameter_id_complete);
}

/// Verifies that an already completed signature is not rewritten by a later event.
TEST(DecompilerParameterId, PreservesCompletedState) {
    auto context = ghidra::analyzer::tests::load_fixture("decompiler_parameter_id");
    context.options().decompiler_parameter_id = true;
    ASSERT_TRUE(context.disassemble_flow(0x140001000ULL));
    ASSERT_TRUE(context.create_function(0x140001000ULL, "parameter_target"));
    ASSERT_TRUE(context.set_parameter_id_complete(0x140001000ULL));
    const auto before = context.function_at(0x140001000ULL)->parameters;
    ghidra::analyzer::DecompilerParameterIdAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    analyzer.analyze(context, {}, cancellation);
    EXPECT_EQ(context.function_at(0x140001000ULL)->parameters, before);
}

/// Verifies that an unsupported integration body is skipped without escaping
/// the parameter-identification analyzer's best-effort boundary.
TEST(DecompilerParameterId, IsolatesUnsupportedIntegrationBodies) {
    const auto path = std::filesystem::path(ANALYZER_FIXTURE_DIR) / "tests" / "data" / "test_analyzers_integration.exe";
    auto image = pe::PeLoader::load_file(path);
    ASSERT_TRUE(image.has_value()) << image.error().message;
    ghidra::analyzer::AnalysisContext context(std::move(*image), "x86-64.sla");
    context.options().decompiler_parameter_id = true;
    for (const auto& exported : context.image().exported_symbols()) {
        if (!exported.forwarded && context.image().is_executable(exported.address_va)) {
            static_cast<void>(context.disassemble_flow(exported.address_va));
            static_cast<void>(context.create_function(exported.address_va));
        }
    }
    ghidra::analyzer::DecompilerParameterIdAnalyzer analyzer;
    ghidra::analyzer::CancellationToken cancellation;
    EXPECT_NO_THROW(analyzer.analyze(context, {}, cancellation));
    EXPECT_FALSE(context.functions().empty());
}
