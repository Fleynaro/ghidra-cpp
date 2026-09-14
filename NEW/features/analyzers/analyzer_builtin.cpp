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
void AutoAnalysisManager::register_builtin_analyzers() {
    register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    register_analyzer(std::make_unique<FunctionStartPreAnalyzer>());
    register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
    register_analyzer(std::make_unique<KnownNoReturnFunctionsAnalyzer>());
    register_analyzer(std::make_unique<FunctionStartAnalyzer>());
    register_analyzer(std::make_unique<FunctionStartFunctionAnalyzer>());
    register_analyzer(std::make_unique<ConstantPropagationAnalyzer>());
    register_analyzer(std::make_unique<ScalarOperandReferencesAnalyzer>());
    register_analyzer(std::make_unique<ReferenceAnalyzer>());
    register_analyzer(std::make_unique<DataReferenceAnalyzer>());
    register_analyzer(std::make_unique<FunctionStartPostAnalyzer>());
    register_analyzer(std::make_unique<FunctionStartDataPostAnalyzer>());
    register_analyzer(std::make_unique<StackAnalyzer>());
}

} // namespace ghidra::analyzer
