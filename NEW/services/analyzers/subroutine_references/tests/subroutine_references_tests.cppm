module;

#include <gtest/gtest.h>

export module subroutine_references_tests;

import analyzer;
import analyzer_disassemble_entry_points;
import analyzer_subroutine_references;
import analyzer_test_support;
import std;

namespace recode::analyzer::tests {
namespace {

/// Returns the complete post-analysis body oracle copied from the Ghidra
/// Delta for `tests/data/test_subroutine_references.exe`.
[[nodiscard]] std::vector<ExpectedFunctionBody> expected_fixture_bodies() {
    return {{0x140001000, {{0x140001000, 0x14000100A}}},
            {0x140001014, {{0x140001014, 0x14000101E}}},
            {0x140001028, {{0x140001028, 0x140001032}}},
            {0x14000103C, {{0x14000103C, 0x140001046}}},
            {0x140001050, {{0x140001050, 0x14000105F}}},
            {0x140001068, {{0x140001068, 0x140001077}}},
            {0x140001098, {{0x140001098, 0x1400010D9}}},
            {0x1400010E0, {{0x140001080, 0x14000108F}, {0x1400010E0, 0x140001114}}},
            {0x14000111C, {{0x14000111C, 0x14000113C}}},
            {0x140001144, {{0x140001144, 0x140001148}}},
            {0x140001150, {{0x140001150, 0x140001161}}},
            {0x140001168, {{0x140001168, 0x140001179}}},
            {0x140001180, {{0x140001180, 0x1400011A6}}},
            {0x1400011B0, {{0x1400011B0, 0x1400011D4}}},
            {0x1400011DC, {{0x1400011DC, 0x1400011FE}}},
            {0x140001208, {{0x140001208, 0x14000120C}}},
            {0x14000122C, {{0x140001214, 0x140001225}, {0x14000122C, 0x140001293}}},
            {0x14000129C, {{0x14000129C, 0x14000129C}}},
            {0x1400012C4, {{0x1400012C4, 0x1400012C4}}}};
}

/// Returns the eleven provider/importer function entries present before the
/// Subroutine References task, copied from the Ghidra before-analysis table.
[[nodiscard]] std::set<Address> expected_initial_entries() {
    return {0x140001098, 0x1400010E0, 0x14000111C, 0x140001150, 0x140001168, 0x140001180,
            0x1400011B0, 0x1400011DC, 0x14000122C, 0x14000129C, 0x1400012C4};
}

/// Returns the eight entries added by FunctionAnalyzer in the Ghidra Delta.
[[nodiscard]] std::set<Address> expected_created_entries() {
    return {0x140001000, 0x140001014, 0x140001028, 0x14000103C, 0x140001050, 0x140001068, 0x140001144, 0x140001208};
}

/// Describes one direct call row from the immutable Ghidra fixture oracle.
struct ExpectedCall {
    Address source{};
    Address target{};
    Address fallthrough{};
};

/// Returns every direct call reference observed before Ghidra analysis.
[[nodiscard]] std::array<ExpectedCall, 23> expected_calls() {
    return {{{0x14000109C, 0x140001000, 0x1400010A1}, {0x1400010AF, 0x140001014, 0x1400010B4},
             {0x1400010CC, 0x14000103C, 0x1400010D1}, {0x1400010E4, 0x140001000, 0x1400010E9},
             {0x1400010F8, 0x140001028, 0x1400010FD}, {0x140001120, 0x140001000, 0x140001125},
             {0x140001154, 0x140001144, 0x140001159}, {0x14000116C, 0x140001150, 0x140001171},
             {0x14000118E, 0x140001180, 0x140001193}, {0x1400011BA, 0x1400011DC, 0x1400011BF},
             {0x1400011E6, 0x1400011B0, 0x1400011EB}, {0x140001234, 0x140001028, 0x140001239},
             {0x140001248, 0x14000103C, 0x14000124D}, {0x140001267, 0x140001000, 0x14000126C},
             {0x140001277, 0x140001014, 0x14000127C}, {0x1400012C8, 0x140001098, 0x1400012CD},
             {0x1400012CD, 0x1400010E0, 0x1400012D2}, {0x1400012D2, 0x14000111C, 0x1400012D7},
             {0x1400012D7, 0x140001168, 0x1400012DC}, {0x1400012E1, 0x140001180, 0x1400012E6},
             {0x1400012EB, 0x1400011DC, 0x1400012F0}, {0x1400012F0, 0x140001208, 0x1400012F5},
             {0x1400012FA, 0x14000122C, 0x1400012FF}}};
}

/// Converts a function map into a stable body snapshot for idempotency checks.
[[nodiscard]] std::map<Address, std::vector<AddressRange>> body_snapshot(const AnalysisContext& context) {
    std::map<Address, std::vector<AddressRange>> result;
    for (const auto& [entry, function] : context.functions()) {
        result.emplace(entry, function.body_ranges);
    }
    return result;
}

/// Materializes every executable byte in the fixture, matching the explicit
/// `DisassembleCommand` preparation performed by the original Ghidra harness.
/// This exhaustive setup belongs to the oracle fixture, not to the production
/// Disassemble Entry Points analyzer.
void prepare_fixture_listing(AnalysisContext& context) {
    for (const auto& region : context.image().memory_regions()) {
        if (!region.executable || region.size == 0U) {
            continue;
        }
        const Address end = region.start + region.size - 1U;
        for (Address address = region.start;; ++address) {
            static_cast<void>(context.disassemble(address));
            if (address == end) {
                break;
            }
        }
    }
}

/// Returns whether a native reference is one of the call-reference categories
/// that FunctionAnalyzer's ReferenceType.isCall() accepts.
[[nodiscard]] bool is_call_reference(const Reference& reference) {
    return reference.kind == ReferenceKind::conditional_call || reference.kind == ReferenceKind::unconditional_call ||
           reference.kind == ReferenceKind::computed_call || reference.kind == ReferenceKind::external;
}

/// Verifies the complete direct-call inventory and ensures duplicate discovery
/// did not create duplicate native references.
void expect_fixture_calls(const AnalysisContext& context) {
    std::vector<ExpectedCall> actual;
    for (const auto& reference : context.references()) {
        if (!is_call_reference(reference) || !reference.fallthrough) {
            continue;
        }
        actual.push_back(ExpectedCall{reference.source, reference.target, *reference.fallthrough});
    }
    std::sort(actual.begin(), actual.end(), [](const ExpectedCall& left, const ExpectedCall& right) {
        return std::tie(left.source, left.target, left.fallthrough) <
               std::tie(right.source, right.target, right.fallthrough);
    });
    const auto expected = expected_calls();
    ASSERT_EQ(actual.size(), expected.size());
    for (const auto& call : expected) {
        EXPECT_NE(std::find_if(actual.begin(), actual.end(),
                               [&](const ExpectedCall& candidate) {
                                   return candidate.source == call.source && candidate.target == call.target &&
                                          candidate.fallthrough == call.fallthrough;
                               }),
                  actual.end())
            << "missing call reference from 0x" << std::hex << call.source << " to 0x" << call.target;
    }
}

/// Builds a minimal synthetic instruction with an explicit flow for tests that
/// isolate FunctionAnalyzer branches not expressible as x64 machine code.
[[nodiscard]] sleigh_runtime::Instruction synthetic_instruction(Address address, sleigh_runtime::FlowKind kind,
                                                                std::optional<Address> target, bool has_fallthrough,
                                                                bool terminal) {
    sleigh_runtime::Instruction instruction;
    instruction.address = address;
    instruction.length = 1;
    instruction.flow = {kind, target ? std::optional{sleigh_runtime::Varnode{"ram", *target, 8}} : std::nullopt,
                        has_fallthrough, terminal};
    return instruction;
}

/// Verifies the real PE fixture against hardcoded Ghidra bodies, existing-entry
/// filtering, call fan-in, and the absence of the repository-only body pass.
TEST(SubroutineReferencesIntegrationTest, MatchesGhidraDeltaAndCreatesBodiesSynchronously) {
    auto context = load_fixture("subroutine_references");
    auto& options = context.options();
    options.function_start_search = false;
    options.reference = false;
    options.data_reference = false;
    options.scalar_operand_references = false;
    options.stack = false;
    options.constant_propagation = false;
    options.non_returning_functions = false;
    options.known_non_returning_functions = false;
    options.discovered_non_returning_functions = false;
    prepare_fixture_listing(context);

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);

    // This is the exact pre-target boundary used by run_ghidra.py.  The first
    // pass intentionally has no FunctionAnalyzer, so it proves provider seeds
    // and disassembly are not being mistaken for call-driven creation.
    std::set<Address> initial_entries;
    for (const auto& [entry, function] : context.functions()) {
        static_cast<void>(function);
        initial_entries.insert(entry);
    }
    EXPECT_EQ(initial_entries, expected_initial_entries());

    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    const auto result = manager.re_analyze_all();
    ASSERT_TRUE(result.completed);

    const auto expected_bodies = expected_fixture_bodies();
    ASSERT_EQ(context.functions().size(), expected_bodies.size());
    expect_function_bodies(context, expected_bodies);
    expect_fixture_calls(context);

    const auto created = expected_created_entries();
    for (const Address entry : created) {
        EXPECT_TRUE(context.functions().contains(entry));
    }
    for (const Address entry : expected_initial_entries()) {
        EXPECT_TRUE(context.functions().contains(entry));
    }
    EXPECT_FALSE(context.function_at(0x140001000)->thunk);

    // FunctionBodyAnalyzer is not an upstream Ghidra component and is not in
    // this pipeline. Function creation itself already supplied the complete
    // bodies and CFGs before the manager drained this pass.
    EXPECT_EQ(std::find_if(manager.registry().analyzers().begin(), manager.registry().analyzers().end(),
                           [](const auto& analyzer) { return analyzer->descriptor().name == "Function Body"; }),
              manager.registry().analyzers().end());
    EXPECT_TRUE(std::all_of(created.begin(), created.end(),
                            [&](Address entry) { return !context.functions().at(entry).blocks.empty(); }));
}

/// Verifies repeated AutoAnalysisManager scheduling is idempotent and does not
/// duplicate either calls, functions, or body ranges.
TEST(SubroutineReferencesIntegrationTest, RepeatedAnalysisIsIdempotent) {
    auto context = load_fixture("subroutine_references");
    context.options().function_start_search = false;
    context.options().reference = false;
    context.options().data_reference = false;
    context.options().scalar_operand_references = false;
    context.options().stack = false;
    context.options().constant_propagation = false;
    context.options().non_returning_functions = false;
    context.options().known_non_returning_functions = false;
    context.options().discovered_non_returning_functions = false;
    prepare_fixture_listing(context);

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);
    const auto first_bodies = body_snapshot(context);
    const auto first_reference_count = context.references().size();
    const auto first_function_count = context.functions().size();

    ASSERT_TRUE(manager.re_analyze_all().completed);
    EXPECT_EQ(body_snapshot(context), first_bodies);
    EXPECT_EQ(context.references().size(), first_reference_count);
    EXPECT_EQ(context.functions().size(), first_function_count);
    expect_fixture_calls(context);
}

/// Verifies computed calls with concrete destinations, conditional calls, and
/// unresolved indirect calls independently of x64's lack of a conditional-call
/// opcode. The construction maps directly to FunctionAnalyzer's `isCall()`
/// branch and to FollowFlow's call exclusion list.
TEST(SubroutineReferencesIntegrationTest, HandlesComputedConditionalAndUnresolvedCalls) {
    auto context = load_fixture("disassemble_entry_points");
    context.options().seed_provider_functions = false;
    context.options().disassemble_entry_points = false;
    context.options().subroutine_references = true;

    const auto region = std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                                     [](const pe::MemoryRegion& item) { return item.executable; });
    ASSERT_NE(region, context.image().memory_regions().end());
    const Address base = region->start + 0x100U;
    const Address computed_target = base + 0x10U;
    const Address conditional_target = base + 0x20U;
    const Address unresolved_call = base + 0x30U;

    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(base, sleigh_runtime::FlowKind::indirect_call, computed_target, true, false)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(computed_target, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(base + 1U, sleigh_runtime::FlowKind::conditional_call, conditional_target, true, false)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(conditional_target, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(unresolved_call, sleigh_runtime::FlowKind::indirect_call, std::nullopt, true, false)));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);

    ASSERT_TRUE(context.function_at(computed_target));
    ASSERT_TRUE(context.function_at(conditional_target));
    EXPECT_FALSE(context.function_at(unresolved_call));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
        return reference.source == base && reference.target == computed_target &&
               reference.kind == ReferenceKind::computed_call;
    }));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
        return reference.source == base + 1U && reference.target == conditional_target &&
               reference.kind == ReferenceKind::conditional_call;
    }));
}

/// Verifies FunctionAnalyzer.fallthroughCall() rejects a call destination that
/// equals the instruction's actual fall-through, including the no-zero-target
/// edge that the previous native value_or(0) implementation mishandled.
TEST(SubroutineReferencesIntegrationTest, IgnoresCallToActualFallthroughOnly) {
    auto context = load_fixture("disassemble_entry_points");
    context.options().seed_provider_functions = false;
    context.options().disassemble_entry_points = false;

    const auto region = std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                                     [](const pe::MemoryRegion& item) { return item.executable; });
    ASSERT_NE(region, context.image().memory_regions().end());
    const Address call = region->start + 0x180U;
    const Address fallthrough = call + 1U;
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(call, sleigh_runtime::FlowKind::call, fallthrough, true, false)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(fallthrough, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);
    EXPECT_FALSE(context.function_at(fallthrough));
    EXPECT_TRUE(std::any_of(context.references().begin(), context.references().end(), [&](const Reference& reference) {
        return reference.source == call && reference.target == fallthrough &&
               reference.kind == ReferenceKind::unconditional_call;
    }));
}

/// Verifies CreateFunctionCmd's thunk handoff: a newly discovered entry whose
/// first flow is a resolved unconditional jump creates its referenced function,
/// marks the entry as a thunk, and keeps the thunk body limited to its own
/// instruction instead of absorbing the destination body.
TEST(SubroutineReferencesIntegrationTest, CreatesSimpleThunkAndReferencedFunction) {
    auto context = load_fixture("disassemble_entry_points");
    context.options().seed_provider_functions = false;
    context.options().disassemble_entry_points = false;
    context.options().create_only_thunks = true;

    const auto region = std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                                     [](const pe::MemoryRegion& item) { return item.executable; });
    ASSERT_NE(region, context.image().memory_regions().end());
    const Address caller = region->start + 0x100U;
    const Address thunk = caller + 0x10U;
    const Address target = caller + 0x20U;
    ASSERT_TRUE(
        context.define_instruction(synthetic_instruction(caller, sleigh_runtime::FlowKind::call, thunk, true, false)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(thunk, sleigh_runtime::FlowKind::branch, target, false, true)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(target, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));

    AutoAnalysisManager manager(context);
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    ASSERT_TRUE(manager.analyze().completed);

    const auto thunk_function = context.function_at(thunk);
    ASSERT_NE(thunk_function, nullptr);
    ASSERT_NE(thunk_function->thunk_target, std::nullopt);
    EXPECT_EQ(*thunk_function->thunk_target, target);
    EXPECT_TRUE(thunk_function->thunk);
    EXPECT_EQ(thunk_function->body_ranges, (std::vector<AddressRange>{{thunk, thunk}}));
    EXPECT_TRUE(context.function_at(target));
    EXPECT_FALSE(context.function_at(target)->thunk);
}

/// Verifies FollowFlow does not turn undefined bytes between two reachable
/// instructions into a body range. This distinguishes real decoded Listing
/// code units from the materialized-code-unit expansion used for the PE oracle.
TEST(SubroutineReferencesIntegrationTest, PreservesUndefinedGapsBetweenFlowClusters) {
    auto context = load_fixture("disassemble_entry_points");
    context.options().seed_provider_functions = false;
    context.options().disassemble_entry_points = false;

    const auto region = std::find_if(context.image().memory_regions().begin(), context.image().memory_regions().end(),
                                     [](const pe::MemoryRegion& item) { return item.executable; });
    ASSERT_NE(region, context.image().memory_regions().end());
    const Address entry = region->start + 0x100U;
    const Address branch = entry + 1U;
    const Address target = entry + 0x20U;
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(entry, sleigh_runtime::FlowKind::none, std::nullopt, true, false)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(branch, sleigh_runtime::FlowKind::conditional_branch, target, false, false)));
    ASSERT_TRUE(context.define_instruction(
        synthetic_instruction(target, sleigh_runtime::FlowKind::return_op, std::nullopt, false, true)));
    // The raw fixture byte at this synthetic target is intentionally not used
    // as a decoder oracle.  Supplying the already-materialized jump reference
    // models Listing/ReferenceManager state exactly as FollowFlow consumes it.
    static_cast<void>(context.add_reference(Reference{branch, target, ReferenceKind::conditional_jump, std::nullopt,
                                                      std::nullopt, FlowOverride::none, false}));

    ASSERT_TRUE(context.create_function(entry));
    ASSERT_TRUE(context.function_at(entry));
    EXPECT_EQ(context.function_at(entry)->body_ranges, (std::vector<AddressRange>{{entry, branch}, {target, target}}));
}

} // namespace
} // namespace recode::analyzer::tests
