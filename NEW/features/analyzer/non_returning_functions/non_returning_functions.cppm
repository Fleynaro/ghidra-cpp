module analyzer;

import std;

namespace ghidra::analyzer {
namespace {

/// Returns whether a reference carries a direct or computed call flow.
[[nodiscard]] bool is_call_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_call || kind == ReferenceKind::conditional_call ||
           kind == ReferenceKind::computed_call || kind == ReferenceKind::external;
}

/// Returns whether an instruction is a concrete post-call no-return indicator.
[[nodiscard]] bool is_non_return_indicator(const InstructionRecord& record) {
    std::string text = record.instruction.mnemonic + " " + record.instruction.assembly;
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text.find("int3") != std::string::npos || text.find("ud2") != std::string::npos ||
           text.find("debugbreak") != std::string::npos;
}

/// Normalizes decorated import/export names and checks the no-return set.
[[nodiscard]] bool known_no_return_name(std::string_view name) {
    std::string normalized(name);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (const auto separator = normalized.rfind('!'); separator != std::string::npos)
        normalized.erase(0, separator + 1);
    for (const auto prefix : {std::string_view("__imp_"), std::string_view("imp_")}) {
        if (normalized.starts_with(prefix))
            normalized.erase(0, prefix.size());
    }
    if (normalized.starts_with('_') && !normalized.starts_with("__"))
        normalized.erase(0, 1);
    if (const auto at = normalized.find('@');
        at != std::string::npos &&
        std::all_of(normalized.begin() + static_cast<std::ptrdiff_t>(at + 1), normalized.end(),
                    [](unsigned char value) { return std::isdigit(value) != 0; })) {
        normalized.erase(at);
    }
    static constexpr std::array<std::string_view, 18> names{"abort",
                                                            "exit",
                                                            "exitprocess",
                                                            "terminateprocess",
                                                            "quick_exit",
                                                            "quickexit",
                                                            "terminate",
                                                            "__fastfail",
                                                            "__report_gsfailure",
                                                            "__security_fail",
                                                            "security_check_cookie",
                                                            "fatal",
                                                            "panic",
                                                            "assert_failed",
                                                            "raisefailfast",
                                                            "unhandledexceptionfilter",
                                                            "stdterminate",
                                                            "_exit"};
    return std::find(names.begin(), names.end(), normalized) != names.end();
}

/// Repairs only call references that actually target one known no-return function.
void repair_callers(AnalysisContext& context, Address target) {
    for (const auto& reference : context.references()) {
        if (reference.target == target && is_call_reference(reference.kind)) {
            static_cast<void>(context.set_flow_override(reference.source, FlowOverride::call_return, target));
        }
    }
}

} // namespace

/// Returns the FindNoReturnFunctionsAnalyzer priority and code-event contract.
AnalyzerDescriptor NonReturningFunctionsAnalyzer::descriptor() const {
    return {"Non-Returning Functions",
            302,
            {EventKind::code_added, EventKind::external_added, EventKind::reference_added, EventKind::function_added,
             EventKind::flow_changed},
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
            static_cast<void>(context.create_function(symbol.address_va, *symbol.name));
        }
        if (context.set_function_no_return(symbol.address_va, true) && context.options().create_analysis_bookmarks) {
            static_cast<void>(context.add_bookmark(
                Bookmark{symbol.address_va, "Non-Returning Function", "Non-Returning Function Identified"}));
        }
        repair_callers(context, symbol.address_va);
    }
    for (const auto& symbol : context.external_symbols()) {
        if (cancellation.is_cancelled())
            return;
        if (!known_no_return_name(symbol.name))
            continue;
        if (context.set_external_no_return(symbol.iat_address, true) && context.options().create_analysis_bookmarks) {
            static_cast<void>(context.add_bookmark(
                Bookmark{symbol.iat_address, "Non-Returning Function", "External no-return function"}));
        }
        for (const auto& reference : context.references()) {
            if (reference.kind == ReferenceKind::external && reference.target == symbol.iat_address) {
                static_cast<void>(
                    context.set_flow_override(reference.source, FlowOverride::call_return, symbol.iat_address));
            }
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
            static_cast<void>(context.create_function(target));
        }
        if (context.set_function_no_return(target, true) && context.options().create_analysis_bookmarks) {
            static_cast<void>(
                context.add_bookmark(Bookmark{target, "Non-Returning Function", "Non-Returning Function Found"}));
        }
        for (const Address caller : callers) {
            static_cast<void>(context.set_flow_override(caller, FlowOverride::call_return, target));
        }
    }
}

} // namespace ghidra::analyzer
