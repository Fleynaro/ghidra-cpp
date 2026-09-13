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

/// Loads the architecture-specific no-return database used by Ghidra's
/// NonReturningFunctionNames helper. Repository-relative fallbacks keep the
/// native port autonomous while installed callers can provide a file path.
[[nodiscard]] std::set<std::string> load_no_return_names(const AnalysisContext& context) {
    std::vector<std::filesystem::path> candidates;
    if (!context.options().no_return_names_file.empty()) {
        candidates.push_back(context.options().no_return_names_file);
    }
    constexpr std::string_view relative = "Ghidra/Features/Base/data/PEFunctionsThatDoNotReturn";
    candidates.emplace_back(relative);
    candidates.emplace_back(std::filesystem::path{".."} / relative);
    candidates.emplace_back(std::filesystem::path{".."} / ".." / relative);
    candidates.emplace_back(std::filesystem::path{".."} / ".." / ".." / relative);
    for (const auto& candidate : candidates) {
        std::ifstream input(candidate);
        if (!input) {
            continue;
        }
        std::set<std::string> names;
        for (std::string line; std::getline(input, line);) {
            if (const auto comment = line.find('#'); comment != std::string::npos) {
                line.erase(comment);
            }
            const auto first = line.find_first_not_of(" \t");
            if (first == std::string::npos) {
                continue;
            }
            line.erase(0, first);
            while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back())) != 0) {
                line.pop_back();
            }
            names.insert(line);
        }
        if (!names.empty()) {
            return names;
        }
    }
    return {"abort",
            "CxxThrowException",
            "CxxThrowException@8",
            "CxxFrameHandler3",
            "crtExitProcess",
            "ExitProcess",
            "ExitThread",
            "exit",
            "ExRaiseAccessViolation",
            "ExRaiseDatatypeMisalignment",
            "ExRaiseStatus",
            "FreeLibraryAndExitThread",
            "invalid_parameter_noinfo_noreturn",
            "invoke_watson",
            "KeBugCheck",
            "KeBugCheckEx",
            "longjmp",
            "quick_exit",
            "RpcRaiseException",
            "terminate",
            "___raise_securityfailure",
            "___report_rangecheckfailure",
            "?_Xregex_error@std@@YAXW4error_type@regex_constant@1@@Z",
            "?_Xbad_alloc@std@@YAXXZ",
            "?_Xlength_error@std@@YAXPBD@Z",
            "?_Xout_of_range@std@@YAXPBD@Z",
            "?_Xbad_function_call@std@@YAXXZ",
            "?terminate@@YAXXZ"};
}

/// Normalizes decorated import/export names and checks the supplied exact or
/// wildcard no-return set. Ghidra strips all leading underscores first.
[[nodiscard]] bool known_no_return_name(std::string_view name, const std::set<std::string>& names) {
    std::string normalized(name);
    if (const auto separator = normalized.rfind('!'); separator != std::string::npos) {
        normalized.erase(0, separator + 1);
    }
    for (const auto prefix : {std::string_view("__imp_"), std::string_view("_imp_")}) {
        if (normalized.starts_with(prefix)) {
            normalized.erase(0, prefix.size());
        }
    }
    while (normalized.starts_with('_')) {
        normalized.erase(normalized.begin());
    }
    return std::any_of(names.begin(), names.end(), [&](const std::string& candidate) {
        std::string comparable = candidate;
        while (comparable.starts_with('_')) {
            comparable.erase(comparable.begin());
        }
        return comparable.ends_with('*') ? normalized.starts_with(comparable.substr(0, comparable.size() - 1))
                                         : normalized == comparable;
    });
}

/// Repairs only call references that actually target one known no-return function.
void repair_callers(AnalysisContext& context, Address target) {
    for (const auto& reference : context.references()) {
        if (reference.target == target && is_call_reference(reference.kind)) {
            static_cast<void>(context.set_flow_override(reference.source, FlowOverride::call_return, target));
        }
    }
}

/// Applies the early runtime-name no-return database before call-driven discovery.
void mark_known_functions(AnalysisContext& context, std::span<const AnalysisEvent> events,
                          CancellationToken& cancellation) {
    const auto names = load_no_return_names(context);
    // The native event model currently carries seed addresses rather than the
    // affected symbol ranges delivered by Ghidra's AddressSetView. Scan the
    // complete provider symbol set so a known symbol inside a section is not
    // skipped merely because it is not itself a region-start seed.
    static_cast<void>(events);
    for (const auto& symbol : context.image().exported_symbols()) {
        if (cancellation.is_cancelled() || symbol.forwarded || !symbol.name ||
            !known_no_return_name(*symbol.name, names))
            continue;
        if (!context.functions().contains(symbol.address_va)) {
            static_cast<void>(context.create_function(symbol.address_va, *symbol.name));
        }
        static_cast<void>(context.set_function_no_return(symbol.address_va, true));
        if (context.options().create_analysis_bookmarks)
            static_cast<void>(context.add_bookmark(
                Bookmark{symbol.address_va, "Non-Returning Function", "Known no-return function"}));
        repair_callers(context, symbol.address_va);
    }
    // COFF symbols are the PE equivalent of non-exported primary symbols in
    // Ghidra's symbol table and must participate in known-name analysis too.
    for (const auto& symbol : context.image().coff_symbols()) {
        const auto address = context.image().rva_to_va(static_cast<pe::Rva>(symbol.value));
        if (!address || symbol.section_number <= 0 || cancellation.is_cancelled() ||
            !known_no_return_name(symbol.name, names)) {
            continue;
        }
        if (!context.functions().contains(*address)) {
            static_cast<void>(context.create_function(*address, symbol.name));
        }
        static_cast<void>(context.set_function_no_return(*address, true));
        repair_callers(context, *address);
    }
    for (const auto& symbol : context.external_symbols()) {
        if (cancellation.is_cancelled())
            return;
        if (!known_no_return_name(symbol.name, names))
            continue;
        static_cast<void>(context.set_external_no_return(symbol.iat_address, true));
        if (context.options().create_analysis_bookmarks)
            static_cast<void>(context.add_bookmark(
                Bookmark{symbol.iat_address, "Non-Returning Function", "Known external no-return function"}));
        for (const auto& reference : context.references()) {
            if (reference.kind == ReferenceKind::external && reference.target == symbol.iat_address) {
                static_cast<void>(
                    context.set_flow_override(reference.source, FlowOverride::call_return, symbol.iat_address));
            }
        }
    }
}

} // namespace

/// Returns the early known no-return analyzer contract.
AnalyzerDescriptor KnownNoReturnFunctionsAnalyzer::descriptor() const {
    return {"Non-Returning Functions - Known", 97, {EventKind::memory_added, EventKind::external_added}, {}};
}

/// Marks known names before disassembly/function discovery scheduling.
void KnownNoReturnFunctionsAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                             CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/NoReturnFunctionAnalyzer.java
    // Relevant methods: canAnalyze(), added(), loadFunctionNamesIfNeeded(), and known function application.
    if (context.options().non_returning_functions && context.options().known_non_returning_functions) {
        mark_known_functions(context, events, cancellation);
    }
}

/// Returns the FindNoReturnFunctionsAnalyzer priority and code-event contract.
AnalyzerDescriptor NonReturningFunctionsAnalyzer::descriptor() const {
    return {"Non-Returning Functions - Discovered",
            302,
            {EventKind::code_added, EventKind::external_added, EventKind::external_changed, EventKind::reference_added,
             EventKind::function_added, EventKind::flow_changed},
            {}};
}

/// Applies known-name and repeated INT3/UD2 evidence to call targets.
void NonReturningFunctionsAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                            CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/FindNoReturnFunctionsAnalyzer.java
    // Relevant methods: added(), detectNoReturn(), targetOnlyCallsNoReturn(), setNoFallThru(), and
    // fixCallingFunctionBody(). The evidence threshold and call-flow override are retained.
    if (!context.options().non_returning_functions || !context.options().discovered_non_returning_functions) {
        return;
    }
    const auto names = load_no_return_names(context);
    for (const auto& symbol : context.image().exported_symbols()) {
        if (symbol.forwarded || !symbol.name || !known_no_return_name(*symbol.name, names)) {
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
        if (!known_no_return_name(symbol.name, names))
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
        const auto caller = context.function_containing(reference.source);
        bool indicator = false;
        bool boundary = false;
        Address cursor = *reference.fallthrough;
        for (std::size_t step = 0; step < 256; ++step) {
            const auto following = context.function_containing(cursor);
            if (following && (!caller || following->entry != caller->entry)) {
                boundary = true;
                break;
            }
            if (context.data().contains(cursor)) {
                boundary = true;
                break;
            }
            const auto instruction = context.instructions().find(cursor);
            if (instruction == context.instructions().end()) {
                boundary = true;
                break;
            }
            if (is_non_return_indicator(instruction->second)) {
                indicator = true;
                break;
            }
            const auto next = instruction->second.instruction.flow.has_fallthrough
                                  ? cursor + instruction->second.instruction.length
                                  : std::numeric_limits<Address>::max();
            if (next == std::numeric_limits<Address>::max()) {
                break;
            }
            cursor = next;
        }
        if (indicator || boundary) {
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
    // A function with no return and only calls to already-known no-return
    // targets is itself non-returning (FindNoReturnFunctionsAnalyzer's
    // targetOnlyCallsNoReturn() rule).
    for (std::size_t pass = 0; pass < context.functions().size(); ++pass) {
        for (const auto& [entry, function] : context.functions()) {
            bool has_call = false;
            bool has_return = false;
            bool all_calls_no_return = true;
            for (const Address address : function.instruction_starts) {
                const auto instruction = context.instructions().find(address);
                if (instruction == context.instructions().end())
                    continue;
                if (instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::return_op)
                    has_return = true;
                for (const auto reference_index : instruction->second.reference_indices) {
                    const auto& reference = context.references()[reference_index];
                    // The original targetOnlyCallsNoReturn rule reasons about calls.
                    // A jump to a no-return function is control flow, but it is not a
                    // call and must not make the containing function appear non-returning.
                    if (!is_call_reference(reference.kind))
                        continue;
                    has_call = true;
                    const auto target_function = context.function_at(reference.target);
                    const auto external = std::find_if(
                        context.external_symbols().begin(), context.external_symbols().end(),
                        [&](const ExternalSymbol& symbol) { return symbol.iat_address == reference.target; });
                    if ((!target_function || !target_function->no_return) &&
                        (external == context.external_symbols().end() || !external->no_return)) {
                        all_calls_no_return = false;
                    }
                }
            }
            if (has_call && !has_return && all_calls_no_return) {
                static_cast<void>(context.set_function_no_return(entry, true));
                repair_callers(context, entry);
            }
        }
    }
}

} // namespace ghidra::analyzer
