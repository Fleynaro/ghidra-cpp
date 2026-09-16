export module ghidra.core.decompilation;

import std;
import ghidra.core.analysis_fact;
import ghidra.core.diagnostics;
import ghidra.core.function;
import ghidra.core.function_signature;
import ghidra.core.instruction;
import ghidra.core.variable;

export namespace ghidra::core {

/// Classifies whether a decompilation completed, timed out, or failed.
enum class DecompilationStatus : std::uint8_t { complete, timeout, cancelled, failed };

/// Stores the structured and textual artifacts of one decompiler task.
struct Decompilation {
    FunctionKey function;
    Revision read_revision;
    DecompilationStatus status{DecompilationStatus::failed};
    std::string c_source;
    std::string control_flow_text;
    std::vector<Instruction> raw_instructions;
    std::optional<FunctionSignature> recovered_signature;
    std::vector<VariableDescription> recovered_variables;
    std::vector<SwitchFact> switches;
    std::vector<AnalysisEvidence> evidence;
    std::vector<Diagnostic> diagnostics;
    std::string cache_identity;
};

/// Names an optional persisted decompiler artifact.
struct DecompileArtifact {
    std::string kind;
    std::string content;
    Revision revision;
};

} // namespace ghidra::core
