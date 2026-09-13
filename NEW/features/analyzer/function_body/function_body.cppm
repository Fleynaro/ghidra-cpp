module analyzer;

import std;

namespace ghidra::analyzer {

/// Returns the Function Body priority and event contract.
AnalyzerDescriptor FunctionBodyAnalyzer::descriptor() const {
    return {"Function Body",
            400,
            {EventKind::function_added, EventKind::flow_changed, EventKind::code_added},
            {"Subroutine References"}};
}

/// Rebuilds affected bodies and CFGs after code or flow state changes.
void FunctionBodyAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                                   CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java
    // Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/FollowFlow.java
    // Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/SimpleBlockModel.java
    // Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java
    std::set<Address> affected;
    for (const auto& event : events) {
        affected.insert(event.addresses.begin(), event.addresses.end());
    }
    for (const auto& [entry, function] : context.functions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (affected.contains(entry) || std::any_of(affected.begin(), affected.end(),
                                                    [&](Address address) { return function.body.contains(address); })) {
            static_cast<void>(context.rebuild_function_body(entry));
        }
    }
}

} // namespace ghidra::analyzer
