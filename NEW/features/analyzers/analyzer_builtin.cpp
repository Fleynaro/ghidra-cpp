import analyzer;
import analyzer_constant_propagation;
import analyzer_data_reference;
import analyzer_disassemble_entry_points;
import analyzer_function_body;
import analyzer_function_start_search;
import analyzer_non_returning_functions;
import analyzer_reference;
import analyzer_scalar_operand_references;
import analyzer_stack;
import analyzer_subroutine_references;

namespace ghidra::analyzer {

/// Registers the complete built-in pipeline for the aggregate application target.
/// Individual analyzer libraries intentionally do not depend on this aggregate
/// registration unit, allowing their focused builds and tests to remain isolated.
void register_builtin_analyzers_impl(AutoAnalysisManager& manager) {
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartPreAnalyzer>());
    manager.register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    manager.register_analyzer(std::make_unique<KnownNoReturnFunctionsAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartFunctionAnalyzer>());
    manager.register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    manager.register_analyzer(std::make_unique<ScalarOperandReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<ReferenceAnalyzer>());
    manager.register_analyzer(std::make_unique<DataReferenceAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartPostAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartDataPostAnalyzer>());
    manager.register_analyzer(std::make_unique<StackAnalyzer>());
}

} // namespace ghidra::analyzer
