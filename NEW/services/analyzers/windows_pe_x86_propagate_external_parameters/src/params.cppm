export module analyzer_windows_pe_x86_propagate_external_parameters;

import analyzer;
import std;

// Original source:
// Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/PropagateExternalParametersAnalyzer.java

/// Propagates already-known external x86 parameter metadata to pushed call arguments.
export namespace ghidra::analyzer {
class WindowsPeX86PropagateExternalParametersAnalyzer final : public Analyzer {
public:
    /// Returns the exact Java analyzer name and DATA_TYPE_PROPOGATION.after()^5 priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Creates parameter evidence for calls whose external function signature is public native state.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Returns whether an instruction is the x86 stack-argument PUSH operation.
[[nodiscard]] bool is_push(const sleigh_runtime::Instruction& instruction) {
    std::string mnemonic = instruction.mnemonic;
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return mnemonic == "push";
}

/// Accepts direct and computed call references that can target an imported function.
[[nodiscard]] bool is_call_reference_kind(ReferenceKind kind) noexcept {
    return kind == ReferenceKind::unconditional_call || kind == ReferenceKind::conditional_call ||
           kind == ReferenceKind::computed_call;
}

/// Adds native evidence for one parameter because the public context lacks EOL comments.
void record_parameter_evidence(AnalysisContext& context, Address address, const ExternalSymbol& external,
                               const FunctionParameter& parameter, std::size_t index) {
    const std::string comment = parameter.type + " " + parameter.name + " for " + external.name;
    static_cast<void>(
        context.add_symbol(SymbolRecord{address, {}, parameter.name, comment, "external_parameter", false, false}));
    if (context.options().create_analysis_bookmarks) {
        static_cast<void>(context.add_bookmark(Bookmark{address, "External Parameter", comment}));
    }
    (void)index;
}

/// Processes one call reference using the formal parameters already attached to its external function.
void process_reference(AnalysisContext& context, const Reference& reference, const ExternalSymbol& external,
                       const Function& external_function) {
    if (!is_call_reference_kind(reference.kind) && reference.kind != ReferenceKind::external) {
        return;
    }
    if (external_function.parameters.empty()) {
        return;
    }
    std::vector<Address> pushes;
    if (const auto caller = context.function_containing(reference.source)) {
        for (const auto address : caller->instruction_starts) {
            if (address >= reference.source) {
                break;
            }
            const auto instruction = context.instructions().find(address);
            if (instruction != context.instructions().end() && is_push(instruction->second.instruction)) {
                pushes.push_back(address);
            }
        }
    } else {
        // A native caller body may not have been materialized yet. Use the closest decoded PUSH
        // instructions as the same bounded fallback used by the resource-reference port.
        for (const auto& [address, instruction] : context.instructions()) {
            if (address < reference.source && is_push(instruction.instruction)) {
                pushes.push_back(address);
            }
        }
        if (pushes.size() > 64U) {
            pushes.erase(pushes.begin(), pushes.end() - 64);
        }
    }
    if (pushes.size() < external_function.parameters.size()) {
        return;
    }
    const auto start = pushes.size() - external_function.parameters.size();
    for (std::size_t index = 0; index < external_function.parameters.size(); ++index) {
        record_parameter_evidence(context, pushes[start + index], external, external_function.parameters[index], index);
    }
}

} // namespace

/// Returns the exact Java name, priority, and BYTE_ANALYZER event contract.
AnalyzerDescriptor WindowsPeX86PropagateExternalParametersAnalyzer::descriptor() const {
    return {"WindowsPE x86 Propagate External Parameters",
            905,
            {EventKind::code_added, EventKind::external_added, EventKind::function_changed},
            {}};
}

/// Applies only signatures already represented in the public native function model.
void WindowsPeX86PropagateExternalParametersAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                                              CancellationToken& cancellation) {
    if (!context.options().windows_pe_x86_propagate_external_parameters ||
        (context.image().coff_header().machine != pe::Machine::i386 &&
         context.image().coff_header().machine != pe::Machine::amd64)) {
        return;
    }
    const auto references = context.references();
    for (const auto& external : context.external_symbols()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        const auto function = context.function_at(external.iat_address);
        if (!function || function->parameters.empty()) {
            continue;
        }
        for (const auto& reference : references) {
            if (reference.target == external.iat_address) {
                process_reference(context, reference, external, *function);
            }
        }
    }
}

} // namespace ghidra::analyzer
