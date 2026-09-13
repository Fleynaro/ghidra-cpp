module analyzer;

import std;

namespace ghidra::analyzer {
namespace {

/// Returns whether a reference carries a direct or computed call flow.
[[nodiscard]] bool is_call_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_call || kind == ReferenceKind::conditional_call ||
           kind == ReferenceKind::computed_call;
}

/// Returns whether an instruction is a concrete post-call no-return indicator.
[[nodiscard]] bool is_non_return_indicator(const InstructionRecord& record) {
    std::string text = record.instruction.mnemonic + " " + record.instruction.assembly;
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text.find("int3") != std::string::npos || text.find("ud2") != std::string::npos ||
           text.find("debugbreak") != std::string::npos;
}

/// Returns whether an exported name is in the PE no-return name set.
[[nodiscard]] bool known_no_return_name(std::string_view name) {
    static constexpr std::array<std::string_view, 11> names{
        "abort",           "exit",  "_exit", "quick_exit",   "terminate", "__fastfail", "__report_gsfailure",
        "__security_fail", "fatal", "panic", "assert_failed"};
    return std::find(names.begin(), names.end(), name) != names.end();
}

} // namespace

/// Returns the FindNoReturnFunctionsAnalyzer priority and code-event contract.
AnalyzerDescriptor NonReturningFunctionsAnalyzer::descriptor() const {
    return {"Non-Returning Functions",
            302,
            {EventKind::code_added, EventKind::function_added, EventKind::flow_changed},
            {}};
}

/// Applies known-name and repeated INT3/UD2 evidence to call targets.
void NonReturningFunctionsAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                            CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/FindNoReturnFunctionsAnalyzer.java
    // Relevant methods: added(), detectNoReturn(), targetOnlyCallsNoReturn(), setNoFallThru(), and
    // fixCallingFunctionBody(). The evidence threshold and call-flow override are retained.
    if (!context.options().non_returning_functions) {
        return;
    }
    for (const auto& symbol : context.image().exported_symbols()) {
        if (symbol.forwarded || !symbol.name || !known_no_return_name(*symbol.name)) {
            continue;
        }
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!context.functions().contains(symbol.address_va)) {
            context.create_function(symbol.address_va, *symbol.name);
        }
        if (context.set_function_no_return(symbol.address_va, true) && context.options().create_analysis_bookmarks) {
            context.add_bookmark(
                Bookmark{symbol.address_va, "Non-Returning Function", "Non-Returning Function Identified"});
        }
    }
    std::map<Address, std::set<Address>> evidence;
    for (const auto& reference : context.references()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!is_call_reference(reference.kind) || !reference.fallthrough) {
            continue;
        }
        const auto instruction = context.instructions().find(*reference.fallthrough);
        if (instruction != context.instructions().end() && is_non_return_indicator(instruction->second)) {
            evidence[reference.target].insert(reference.source);
        }
    }
    for (const auto& [target, callers] : evidence) {
        if (callers.size() < context.options().non_return_threshold) {
            continue;
        }
        if (!context.functions().contains(target)) {
            context.create_function(target);
        }
        if (context.set_function_no_return(target, true) && context.options().create_analysis_bookmarks) {
            context.add_bookmark(Bookmark{target, "Non-Returning Function", "Non-Returning Function Found"});
        }
        for (const Address caller : callers) {
            context.set_flow_override(caller, FlowOverride::call_return);
        }
    }
}

} // namespace ghidra::analyzer
