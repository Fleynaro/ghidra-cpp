import analyzer;
import analyzer_aggressive_instruction_finder;
import analyzer_apply_data_archives;
import analyzer_ascii_strings;
import analyzer_call_convention_id;
import analyzer_call_fixup_installer;
import analyzer_constant_propagation;
import analyzer_condense_filler_bytes;
import analyzer_create_address_tables;
import analyzer_data_reference;
import analyzer_decompiler_parameter_id;
import analyzer_decompiler_switch_analysis;
import analyzer_demangler_microsoft;
import analyzer_embedded_media;
import analyzer_disassemble_entry_points;
import analyzer_external_entry_references;
import analyzer_function_id;
import analyzer_function_start_search;
import analyzer_non_returning_functions;
import analyzer_pdb_msdia;
import analyzer_pdb_universal;
import analyzer_reference;
import analyzer_scalar_operand_references;
import analyzer_shared_return_calls;
import analyzer_stack;
import analyzer_subroutine_references;
import analyzer_variadic_function_signature_override;
import analyzer_windows_pe_x86_propagate_external_parameters;
import analyzer_windows_resource_reference;
import analyzer_x86_constant_reference;

namespace recode::analyzer {

/// Registers the complete built-in pipeline for the aggregate application target.
/// Individual analyzer libraries intentionally do not depend on this aggregate
/// registration unit, allowing their focused builds and tests to remain isolated.
void register_builtin_analyzers_impl(AutoAnalysisManager& manager) {
    manager.register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    manager.register_analyzer(std::make_unique<EmbeddedMediaAnalyzer>());
    manager.register_analyzer(std::make_unique<ApplyDataArchivesAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionStartPreAnalyzer>());
    manager.register_analyzer(std::make_unique<ExternalEntryReferencesAnalyzer>());
    manager.register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    manager.register_analyzer(std::make_unique<SharedReturnCallsAnalyzer>());
    manager.register_analyzer(std::make_unique<CallFixupInstallerAnalyzer>());
    manager.register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
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
    manager.register_analyzer(std::make_unique<CreateAddressTablesAnalyzer>());
    manager.register_analyzer(std::make_unique<AsciiStringsAnalyzer>());
    manager.register_analyzer(std::make_unique<CondenseFillerBytesAnalyzer>());
    manager.register_analyzer(std::make_unique<DemanglerMicrosoftAnalyzer>());
    manager.register_analyzer(std::make_unique<WindowsResourceReferenceAnalyzer>());
    manager.register_analyzer(std::make_unique<WindowsPeX86PropagateExternalParametersAnalyzer>());
    manager.register_analyzer(std::make_unique<X86ConstantReferenceAnalyzer>());
    manager.register_analyzer(std::make_unique<AggressiveInstructionFinderAnalyzer>());
    manager.register_analyzer(std::make_unique<FunctionIdAnalyzer>());
    manager.register_analyzer(std::make_unique<DecompilerSwitchAnalysisAnalyzer>());
    manager.register_analyzer(std::make_unique<DecompilerParameterIdAnalyzer>());
    manager.register_analyzer(std::make_unique<CallConventionIdAnalyzer>());
    manager.register_analyzer(std::make_unique<VariadicFunctionSignatureOverrideAnalyzer>());
    manager.register_analyzer(std::make_unique<PdbUniversalAnalyzer>());
    manager.register_analyzer(std::make_unique<PdbMsdiaAnalyzer>());
}

} // namespace recode::analyzer
