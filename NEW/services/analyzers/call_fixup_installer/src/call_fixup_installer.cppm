export module analyzer_call_fixup_installer;

import analyzer;
import std;

// Original source:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupAnalyzer.java

/// Describes a compiler-spec call-fixup target mapping supplied by a native compiler-spec adapter.
export struct CallFixupRule {
    std::string target_name;
    std::string fixup_name;
    bool no_return{};
    bool fallthrough{true};
};

/// Installs supplied compiler-spec fixup mappings and repairs known non-fallthrough callers.
export namespace ghidra::analyzer {
class CallFixupInstallerAnalyzer final : public Analyzer {
public:
    /// Constructs an installer with immutable compiler-spec mappings.
    explicit CallFixupInstallerAnalyzer(std::vector<CallFixupRule> rules = {});

    /// Returns the original analyzer identity and DISASSEMBLY.after() priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Applies target matching, fixup evidence, no-return state, and caller flow repairs.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;

private:
    std::vector<CallFixupRule> rules_;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Returns the compiler-spec target name variants used by the Java analyzer.
[[nodiscard]] std::array<std::string, 3> target_variants(std::string_view name) {
    std::string normalized(name);
    constexpr std::string_view conflict = "libID_conflict_";
    if (normalized.starts_with(conflict)) {
        normalized.erase(0, conflict.size());
    }
    return {normalized, "_" + normalized, "__" + normalized};
}

/// Finds a matching supplied compiler-spec rule for one native function name.
[[nodiscard]] const CallFixupRule* find_rule(std::span<const CallFixupRule> rules, std::string_view name) {
    const auto variants = target_variants(name);
    const auto rule = std::find_if(rules.begin(), rules.end(), [&](const CallFixupRule& candidate) {
        return std::find(variants.begin(), variants.end(), candidate.target_name) != variants.end();
    });
    return rule == rules.end() ? nullptr : &*rule;
}

/// Repairs calls to a non-fallthrough function using the existing flow override and body rebuild APIs.
void repair_callers(AnalysisContext& context, Address target) {
    const auto references = context.references();
    for (const auto& reference : references) {
        if (reference.target != target ||
            (reference.kind != ReferenceKind::unconditional_call && reference.kind != ReferenceKind::conditional_call &&
             reference.kind != ReferenceKind::computed_call && reference.kind != ReferenceKind::external)) {
            continue;
        }
        if (context.set_flow_override(reference.source, FlowOverride::call_return, target)) {
            if (const auto caller = context.function_containing(reference.source)) {
                static_cast<void>(context.rebuild_function_body(caller->entry));
            }
        }
    }
}

} // namespace

/// Stores the compiler-spec target mappings without copying or parsing compiler-spec XML.
CallFixupInstallerAnalyzer::CallFixupInstallerAnalyzer(std::vector<CallFixupRule> rules) : rules_(std::move(rules)) {}

/// Returns the Call-Fixup Installer priority and function lifecycle contract.
AnalyzerDescriptor CallFixupInstallerAnalyzer::descriptor() const {
    return {"Call-Fixup Installer", 301, {EventKind::function_added, EventKind::function_changed}, {}};
}

/// Applies only rules supplied by the existing compiler-spec integration boundary.
void CallFixupInstallerAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                         CancellationToken& cancellation) {
    if (!context.options().call_fixup_installer || rules_.empty()) {
        return;
    }
    for (const auto& [entry, function] : context.functions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        const auto* rule = find_rule(rules_, function.name);
        if (!rule) {
            continue;
        }
        static_cast<void>(
            context.add_symbol(SymbolRecord{entry, function.name, rule->fixup_name, {}, "call_fixup", false, true}));
        if (rule->no_return) {
            static_cast<void>(context.set_function_no_return(entry, true));
        }
        if (rule->no_return || !rule->fallthrough) {
            repair_callers(context, entry);
        }
    }
}

} // namespace ghidra::analyzer
