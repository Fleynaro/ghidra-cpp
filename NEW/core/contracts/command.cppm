export module recode.core.contracts.command;

import std;
import recode.core.data_object;
import recode.core.binary;
import recode.core.decompilation;
import recode.core.events.event;
import recode.core.function;
import recode.core.function_id;
import recode.core.flow;
import recode.core.identifiers;
import recode.core.instruction;
import recode.core.reference;
import recode.core.function_signature;
import recode.core.contracts.operation;
import recode.core.contracts.pe_loader;
import recode.core.contracts.pcode_decoder;

export namespace recode::core::contracts {

/// Requests creation of a project session from one primary artifact.
struct CreateProject {
    ProjectId project;
    std::filesystem::path directory;
    BinaryArtifact artifact;
};

/// Requests loading and materialization of the primary binary.
struct LoadPrimaryBinary {
    BinaryArtifact artifact;
    LoadOptions options;
};

/// Requests one transient decoder response.
struct DecodeInstruction {
    DecodeRequest request;
    bool materialize{};
};

/// Requests a bounded decoder batch.
struct DecodeInstructionBatch {
    DecodeBatchRequest request;
    bool materialize{};
};

/// Requests asynchronous function decompilation.
struct DecompileFunction {
    FunctionKey function;
};

/// Requests asynchronous Function ID matching.
struct IdentifyFunction {
    FunctionKey function;
    FunctionIdOptions options;
};

/// Starts the project analysis scheduler.
struct StartAnalysis {
    std::optional<AnalysisRunId> run;
};

/// Requests a source-priority-aware function rename.
struct RenameFunction {
    FunctionKey function;
    std::string name;
    std::string source{"user"};
};

/// Requests a complete signature replacement.
struct AssignFunctionSignature {
    FunctionKey function;
    FunctionSignature signature;
};

/// Requests a data object definition.
struct DefineData {
    DataObject data;
};

/// Requests creation of a function snapshot.
struct CreateFunction {
    FunctionSnapshot function;
};

/// Requests insertion of one reference.
struct AddReference {
    Reference reference;
};

/// Requests a flow override.
struct SetFlowOverride {
    FlowOverride override_value;
};

/// Requests orderly project closure.
struct CloseProject {};

/// Enumerates every command accepted by the runtime dispatcher.
using CommandPayload =
    std::variant<CreateProject, LoadPrimaryBinary, DecodeInstruction, DecodeInstructionBatch, DecompileFunction,
                 IdentifyFunction, StartAnalysis, RenameFunction, AssignFunctionSignature, DefineData, CreateFunction,
                 AddReference, SetFlowOverride, CloseProject>;

/// Adds routing and optimistic-concurrency metadata to a command payload.
struct CommandRequest {
    CommandId id;
    ProjectId project;
    CorrelationId correlation;
    std::optional<CausationId> causation;
    std::optional<Revision> expected_revision;
    ExecutionMode mode{ExecutionMode::inline_mode};
    WorkPriority priority{WorkPriority::interactive};
    CommandPayload payload;
};

/// Classifies the transient outcome of a command.
enum class CommandStatus : std::uint8_t { completed, queued, cancelled, rejected, failed };

/// Carries the non-event result of a command.
using CommandResult = std::variant<std::monostate, Revision, Instruction, DecodeBatchResult, PeLoadResult,
                                   FunctionSnapshot, Decompilation, FunctionIdResult, std::string>;

/// Returns a command result and the events committed while producing it.
struct CommandResponse {
    CommandId command;
    CommandStatus status{CommandStatus::failed};
    std::optional<CommandResult> result;
    std::vector<EventId> appended_events;
    std::optional<Revision> committed_revision;
    std::vector<Diagnostic> diagnostics;
};

/// Handles typed command execution in a runtime-owned project commit lane.
class ICommandHandler {
public:
    /// Releases a command handler through its contract.
    virtual ~ICommandHandler() = default;

    /// Validates and executes one command request.
    [[nodiscard]] virtual Result<CommandResponse> handle(const CommandRequest& request) = 0;
};

} // namespace recode::core::contracts
