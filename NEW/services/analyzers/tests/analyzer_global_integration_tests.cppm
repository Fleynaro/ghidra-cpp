module;

#include <gtest/gtest.h>

export module analyzer_global_integration_tests;

import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Loads the one integration PE from the aggregate analyzer fixture directory.
[[nodiscard]] AnalysisContext load_integration_fixture() {
    const auto path = std::filesystem::path(ANALYZER_FIXTURE_DIR) / "tests" / "data" / "test_analyzers_integration.exe";
    auto image = pe::PeLoader::load_file(path);
    if (!image)
        throw std::runtime_error(image.error().message);
    return AnalysisContext(std::move(*image), "x86-64.sla");
}

/// Enables every native analyzer option while retaining explicit no-input
/// behavior for optional archives, pattern files, and PDB providers.
void configure_complete_pipeline(AnalysisContext& context) {
    auto& options = context.options();
    options.aggressive_instruction_finder = true;
    options.apply_data_archives = true;
    options.ascii_strings = true;
    options.call_convention_id = true;
    options.call_fixup_installer = true;
    options.condense_filler_bytes = true;
    options.constant_propagation = true;
    options.create_address_tables = true;
    options.data_reference = true;
    options.decompiler_parameter_id = true;
    options.decompiler_switch_analysis = true;
    options.demangler_microsoft = true;
    options.disassemble_entry_points = true;
    options.embedded_media = true;
    options.external_entry_references = true;
    options.function_id = true;
    options.function_start_search = true;
    options.non_returning_functions = true;
    options.known_non_returning_functions = true;
    options.discovered_non_returning_functions = true;
    options.pdb_msdia = false;
    options.pdb_universal = false;
    options.reference = true;
    options.scalar_operand_references = true;
    options.shared_return_calls = true;
    options.stack = true;
    options.subroutine_references = true;
    options.variadic_function_signature_override = true;
    options.windows_pe_x86_propagate_external_parameters = true;
    options.windows_resource_reference = true;
    options.x86_constant_reference = true;
    options.address_table_alignment = 8U;
    options.address_table_minimum_entries = 4U;
    options.address_table_auto_label = true;
    options.create_stack_parameters = true;
    options.aggressive_minimum_functions = 20U;
    options.ascii_minimum_length = 5U;
    options.ascii_require_null_termination = true;
    options.ascii_end_alignment = 4U;
    options.filler_minimum_length = 1U;
    options.maximum_disassembly_instructions = 200000U;
    options.maximum_events = 300000U;

    const auto root = std::filesystem::path(ANALYZER_FIXTURE_DIR);
    options.no_return_names_file =
        root / ".." / ".." / ".." / "Ghidra" / "Features" / "Base" / "data" / "PEFunctionsThatDoNotReturn";
    // PDB providers remain registered and are covered by their focused tests,
    // but this aggregate image deliberately has no PDB input. Keeping debug
    // symbols out of the shared run lets ASCII/data/address-table analyzers
    // consume the same raw initialized storage instead of PDB-defined objects.
    options.pdb_path.clear();
    options.pdb_universal = false;
    options.pdb_msdia = false;
}

/// Returns the native spelling used in stable reference fingerprints.
[[nodiscard]] std::string reference_kind_name(ReferenceKind kind) {
    switch (kind) {
        case ReferenceKind::fallthrough:
            return "fallthrough";
        case ReferenceKind::conditional_jump:
            return "conditional_jump";
        case ReferenceKind::unconditional_jump:
            return "unconditional_jump";
        case ReferenceKind::conditional_call:
            return "conditional_call";
        case ReferenceKind::unconditional_call:
            return "unconditional_call";
        case ReferenceKind::computed_jump:
            return "computed_jump";
        case ReferenceKind::computed_call:
            return "computed_call";
        case ReferenceKind::data:
            return "data";
        case ReferenceKind::scalar:
            return "scalar";
        case ReferenceKind::stack:
            return "stack";
        case ReferenceKind::external:
            return "external";
    }
    return "unknown";
}

/// Captures all mutable artifact identities needed to prove repeat stability.
struct AnalysisFingerprint {
    std::vector<std::string> functions;
    std::vector<std::string> data;
    std::vector<std::string> references;
    std::vector<std::string> strings;
    std::vector<std::string> symbols;
    std::vector<std::string> bookmarks;
    std::vector<std::string> tables;
    std::vector<std::string> media;
    std::vector<std::string> constants;

    /// Supports exact comparison of deterministic aggregate analysis state.
    friend bool operator==(const AnalysisFingerprint&, const AnalysisFingerprint&) = default;
};

/// Renders one complete final-state fingerprint without depending on the oracle report.
[[nodiscard]] AnalysisFingerprint fingerprint(const AnalysisContext& context) {
    AnalysisFingerprint result;
    for (const auto& [entry, function] : context.functions()) {
        std::ostringstream row;
        row << std::hex << entry << ':' << function.name << ':' << function.body.size() << ':'
            << function.instruction_starts.size() << ':' << function.stack_variables.size() << ':' << function.no_return
            << ':' << function.parameter_id_complete << ':' << function.switch_recovered << ':'
            << function.calling_convention << ':' << function.return_type;
        for (const auto& range : function.body_ranges)
            row << ':' << range.start << '-' << range.end;
        result.functions.push_back(row.str());
    }
    for (const auto& [address, data] : context.data()) {
        std::ostringstream row;
        row << std::hex << address << ':' << data.size << ':' << data.type << ':' << data.value;
        result.data.push_back(row.str());
    }
    for (const auto& reference : context.references()) {
        std::ostringstream row;
        row << std::hex << reference.source << ':' << reference.target << ':' << reference_kind_name(reference.kind)
            << ':' << static_cast<unsigned>(reference.flow_override);
        result.references.push_back(row.str());
    }
    for (const auto& string : context.strings()) {
        std::ostringstream row;
        row << std::hex << string.address << ':' << string.size << ':' << string.value;
        result.strings.push_back(row.str());
    }
    for (const auto& symbol : context.symbols()) {
        std::ostringstream row;
        row << std::hex << symbol.address << ':' << symbol.mangled_name << ':' << symbol.demangled_name << ':'
            << symbol.kind;
        result.symbols.push_back(row.str());
    }
    for (const auto& bookmark : context.bookmarks()) {
        std::ostringstream row;
        row << std::hex << bookmark.address << ':' << bookmark.category << ':' << bookmark.comment;
        result.bookmarks.push_back(row.str());
    }
    for (const auto& table : context.address_tables()) {
        std::ostringstream row;
        row << std::hex << table.address << ':' << table.entry_size;
        for (const auto target : table.targets)
            row << ':' << target;
        result.tables.push_back(row.str());
    }
    for (const auto& media : context.embedded_media()) {
        std::ostringstream row;
        row << std::hex << media.address << ':' << media.size << ':' << media.type << ':' << media.validated;
        result.media.push_back(row.str());
    }
    for (const auto& fact : context.constant_facts()) {
        std::ostringstream row;
        row << std::hex << fact.instruction << ':' << fact.location.space.name() << ':' << fact.location.offset << ':'
            << fact.value << ':' << fact.path_stable << ':' << fact.function_entry;
        result.constants.push_back(row.str());
    }
    return result;
}

/// Finds a recovered function by its fixture-owned exported name.
[[nodiscard]] const Function* function_named(const AnalysisContext& context, std::string_view name) {
    const auto iterator = std::find_if(context.functions().begin(), context.functions().end(),
                                       [&](const auto& item) { return item.second.name == name; });
    return iterator == context.functions().end() ? nullptr : &iterator->second;
}

/// Finds the function at a fixture export without assuming compiler addresses
/// or assuming that an earlier flow pass preserved the export's display name.
[[nodiscard]] const Function* function_at_export(const AnalysisContext& context, std::string_view name) {
    const auto symbol =
        std::find_if(context.image().exported_symbols().begin(), context.image().exported_symbols().end(),
                     [&](const auto& item) { return item.name && *item.name == name; });
    return symbol == context.image().exported_symbols().end() ? nullptr : context.function_at(symbol->address_va);
}

/// Returns whether one fixture-owned string value was recovered by ASCII analysis.
[[nodiscard]] bool has_string(const AnalysisContext& context, std::string_view value) {
    return std::any_of(context.strings().begin(), context.strings().end(),
                       [&](const StringRecord& string) { return string.value == value; });
}

/// Returns whether a bookmark category was produced by one analyzer phase.
[[nodiscard]] bool has_bookmark_category(const AnalysisContext& context, std::string_view category) {
    return std::any_of(context.bookmarks().begin(), context.bookmarks().end(),
                       [&](const Bookmark& bookmark) { return bookmark.category == category; });
}

/// Returns whether a reference of a given kind targets a data or code object.
[[nodiscard]] bool has_reference_kind(const AnalysisContext& context, ReferenceKind kind) {
    return std::any_of(context.references().begin(), context.references().end(),
                       [&](const Reference& reference) { return reference.kind == kind; });
}

/// Verifies that every recovered function and body range is mapped executable code.
[[nodiscard]] bool has_valid_function_bodies(const AnalysisContext& context) {
    for (const auto& [entry, function] : context.functions()) {
        if (!context.image().is_executable(entry) || function.body.empty() || function.instruction_starts.empty())
            return false;
        for (const auto& range : function.body_ranges) {
            if (range.start > range.end || !context.image().is_executable(range.start) ||
                !context.image().is_executable(range.end))
                return false;
        }
    }
    return true;
}

/// Verifies exact reference identities are unique after all event-driven passes.
[[nodiscard]] bool has_unique_references(const AnalysisContext& context) {
    std::set<std::string> identities;
    for (const auto& reference : context.references()) {
        std::ostringstream row;
        row << reference.source << ':' << reference.target << ':' << static_cast<unsigned>(reference.kind) << ':'
            << static_cast<unsigned>(reference.flow_override);
        if (!identities.insert(row.str()).second)
            return false;
    }
    return true;
}

/// Returns whether the function body ranges of two distinct functions overlap.
[[nodiscard]] bool has_overlapping_functions(const AnalysisContext& context) {
    for (auto left = context.functions().begin(); left != context.functions().end(); ++left) {
        auto right = left;
        ++right;
        for (; right != context.functions().end(); ++right) {
            for (const auto& left_range : left->second.body_ranges)
                for (const auto& right_range : right->second.body_ranges)
                    if (left_range.start <= right_range.end && right_range.start <= left_range.end)
                        return true;
        }
    }
    return false;
}

/// Runs every registered analyzer together and verifies interactions among its artifacts.
TEST(AnalyzerGlobalIntegrationTest, RunsCompleteBuiltinPipelineAndRemainsStable) {
    auto context = load_integration_fixture();
    configure_complete_pipeline(context);
    AutoAnalysisManager manager(context);
    manager.register_builtin_analyzers();

    ASSERT_EQ(manager.registry().analyzers().size(), 34U);
    const auto first = manager.analyze();
    ASSERT_TRUE(first.completed) << std::accumulate(
        first.errors.begin(), first.errors.end(), std::string{},
        [](std::string left, const std::string& right) { return left.empty() ? right : left + "; " + right; });
    EXPECT_TRUE(first.errors.empty());
    EXPECT_FALSE(first.executed_analyzers.empty());
    for (const auto& analyzer : manager.registry().analyzers()) {
        const auto& name = analyzer->descriptor().name;
        EXPECT_TRUE(std::find(first.executed_analyzers.begin(), first.executed_analyzers.end(), name) !=
                    first.executed_analyzers.end())
            << "registered analyzer was never scheduled: " << name;
    }

    // Function discovery, body construction, stack analysis, and decompilation
    // consume one another's events in the completed scheduler run.
    EXPECT_GT(context.instructions().size(), 100U);
    EXPECT_GE(context.functions().size(), 20U);
    EXPECT_TRUE(has_valid_function_bodies(context));
    EXPECT_FALSE(has_overlapping_functions(context));
    EXPECT_NE(function_at_export(context, "fixture_entry"), nullptr);
    EXPECT_NE(function_at_export(context, "engine_tick"), nullptr);
    EXPECT_NE(function_at_export(context, "recursive_score"), nullptr);
    EXPECT_NE(function_at_export(context, "mutual_alpha"), nullptr);
    EXPECT_NE(function_at_export(context, "switch_mode"), nullptr);
    EXPECT_NE(function_at_export(context, "resource_lookup"), nullptr);
    EXPECT_EQ(function_named(context, "not_a_fixture_function"), nullptr);

    // The stack, parameter, calling-convention, and switch passes must leave
    // observable state on functions created by earlier flow analyzers.
    const auto* tick = function_at_export(context, "engine_tick");
    ASSERT_NE(tick, nullptr);
    EXPECT_FALSE(tick->stack_variables.empty());
    EXPECT_TRUE(tick->parameter_id_complete || !tick->parameters.empty());
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(),
                            [](const auto& item) { return !item.second.calling_convention.empty(); }));
    const auto* dense_switch = function_at_export(context, "switch_mode");
    ASSERT_NE(dense_switch, nullptr);
    EXPECT_TRUE(dense_switch->switch_recovered ||
                std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
                    return reference.kind == ReferenceKind::computed_jump &&
                           context.function_containing(reference.source) == dense_switch;
                }));

    // Code/data analyzers exchange facts without turning data pointers into
    // function-body edges or duplicating references.
    EXPECT_FALSE(context.data().empty());
    EXPECT_FALSE(context.strings().empty());
    EXPECT_FALSE(context.constant_facts().empty());
    EXPECT_TRUE(has_reference_kind(context, ReferenceKind::data));
    EXPECT_TRUE(has_reference_kind(context, ReferenceKind::scalar));
    EXPECT_TRUE(has_reference_kind(context, ReferenceKind::stack));
    EXPECT_TRUE(has_reference_kind(context, ReferenceKind::unconditional_call));
    EXPECT_TRUE(has_reference_kind(context, ReferenceKind::external));
    EXPECT_TRUE(has_unique_references(context));
    EXPECT_TRUE(has_string(context, "GTA-like native analysis integration fixture"));
    EXPECT_TRUE(has_string(context, "tick=%d entity=%s score=%u"));
    EXPECT_FALSE(has_string(context, "tiny"));

    // The media scanner, address-table scanner, PE resource scanner, and
    // demangler all consume initialized non-code data from the same image.
    EXPECT_GE(context.embedded_media().size(), 5U);
    EXPECT_TRUE(
        std::any_of(context.embedded_media().begin(), context.embedded_media().end(),
                    [](const EmbeddedMediaRecord& media) { return media.type.find("GIF") != std::string::npos; }));
    EXPECT_FALSE(context.address_tables().empty());
    EXPECT_TRUE(std::any_of(
        context.address_tables().begin(), context.address_tables().end(), [&](const AddressTableRecord& table) {
            return std::any_of(table.targets.begin(), table.targets.end(),
                               [&](Address target) { return context.function_at(target) != nullptr; });
        }));
    EXPECT_TRUE(context.image().resources().has_value());
    EXPECT_TRUE(has_bookmark_category(context, "Embedded Media"));
    EXPECT_TRUE(has_bookmark_category(context, "Address Table"));
    EXPECT_TRUE(has_bookmark_category(context, "WindowsResourceReference"));
    EXPECT_TRUE(std::any_of(context.symbols().begin(), context.symbols().end(), [](const SymbolRecord& symbol) {
        return symbol.mangled_name.find("?add@Calculator@demangle_fixture") != std::string::npos &&
               symbol.demangled_name == "add";
    }));

    // Imported APIs and no-return paths prove that external references are not
    // mistaken for local functions and that call-flow repair survives the pass.
    EXPECT_TRUE(std::any_of(context.external_symbols().begin(), context.external_symbols().end(),
                            [](const ExternalSymbol& symbol) { return symbol.name == "LoadStringW"; }));
    const auto* abort_function = function_at_export(context, "engine_abort");
    ASSERT_NE(abort_function, nullptr);
    EXPECT_TRUE(abort_function->no_return);
    EXPECT_TRUE(std::any_of(context.functions().begin(), context.functions().end(),
                            [](const auto& item) { return item.second.no_return; }));

    const auto before_repeat = fingerprint(context);
    const auto repeated = manager.re_analyze_all();
    ASSERT_TRUE(repeated.completed) << std::accumulate(
        repeated.errors.begin(), repeated.errors.end(), std::string{},
        [](std::string left, const std::string& right) { return left.empty() ? right : left + "; " + right; });
    EXPECT_TRUE(repeated.errors.empty());
    EXPECT_EQ(fingerprint(context), before_repeat);
    EXPECT_TRUE(has_unique_references(context));
    EXPECT_FALSE(has_overlapping_functions(context));
}

} // namespace
} // namespace recode::analyzer::tests
