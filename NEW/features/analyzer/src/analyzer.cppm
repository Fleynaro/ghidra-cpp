export module analyzer;

import std;
export import pe_loader;
export import sleigh_runtime;

// This module is the native program/listing boundary for the first analysis
// layer. It intentionally replaces Ghidra's database-backed Program objects
// with observable value types while keeping the event and scheduling contracts.

export namespace ghidra::analyzer {

using Address = std::uint64_t;

/// Identifies a contiguous address range in the loaded program image.
struct AddressRange {
    Address start{};
    Address end{};

    /// Reports whether an address belongs to this inclusive range.
    [[nodiscard]] bool contains(Address address) const noexcept;
};

/// Identifies the state mutation that can wake one or more analyzers.
enum class EventKind : std::uint8_t {
    memory_added,
    external_added,
    code_added,
    data_added,
    reference_added,
    function_added,
    function_changed,
    flow_changed,
    constant_added,
};

/// Carries a coalesced program event and the affected addresses.
struct AnalysisEvent {
    EventKind kind{};
    std::vector<Address> addresses;
    std::uint64_t sequence{};
    bool removed{};
};

/// Describes the observable kind of a reference in the native listing model.
enum class ReferenceKind : std::uint8_t {
    fallthrough,
    conditional_jump,
    unconditional_jump,
    conditional_call,
    unconditional_call,
    computed_jump,
    computed_call,
    data,
    scalar,
    stack,
    external,
};

/// Describes an explicit flow override applied to an instruction reference.
enum class FlowOverride : std::uint8_t {
    none,
    call_return,
};

/// Describes one source-to-destination relation and its analyzer provenance.
struct Reference {
    Address source{};
    Address target{};
    ReferenceKind kind{ReferenceKind::data};
    std::optional<std::size_t> operand_index;
    std::optional<Address> fallthrough;
    FlowOverride flow_override{FlowOverride::none};
    bool analysis_source{};
    std::optional<std::int64_t> stack_offset;
};

/// Owns one decoded instruction and the references originating from it.
struct InstructionRecord {
    sleigh_runtime::Instruction instruction;
    std::vector<std::size_t> reference_indices;
};

/// Represents one non-overlapping CFG block view of a function body.
struct BasicBlock {
    Address start{};
    Address end{};
    std::vector<Address> instructions;
    std::vector<Address> successors;
    std::vector<Address> predecessors;
};

/// Represents one stack storage object inferred from p-code or operands.
struct StackVariable {
    std::int64_t offset{};
    std::uint32_t size{};
    std::string name;
    bool parameter{};
};

/// Represents one function, including its body, CFG, and no-return property.
struct Function {
    Address entry{};
    std::string name;
    std::set<Address> body;
    std::vector<BasicBlock> blocks;
    std::vector<StackVariable> stack_variables;
    bool external{};
    bool no_return{};
    std::optional<Address> provider_end;
    bool thunk{};
    std::uint32_t stack_frame_size{};
    std::int64_t stack_pointer_delta{};
    std::optional<std::string> frame_pointer;
    std::vector<AddressRange> body_ranges;
    std::set<Address> instruction_starts;
};

/// Represents one explicitly defined data object in program memory.
struct DataObject {
    Address address{};
    std::uint32_t size{};
    std::string type;
};

/// Represents one PE import without treating the external namespace as local code.
struct ExternalSymbol {
    std::string library;
    std::string name;
    Address iat_address{};
    std::optional<std::uint16_t> ordinal;
    bool delay_loaded{};
    bool no_return{};
};

/// Represents one analysis bookmark retained in the observable state.
struct Bookmark {
    Address address{};
    std::string category;
    std::string comment;
};

/// Represents a value known at one instruction and storage location.
struct ConstantFact {
    Address instruction{};
    sleigh_runtime::Varnode location;
    std::uint64_t value{};
    bool path_stable{};
    Address function_entry{};
};

/// Controls optional analyzers and their Ghidra-compatible thresholds.
struct AnalysisOptions {
    bool disassemble_entry_points{true};
    bool function_start_search{true};
    bool function_start_after_code{};
    bool function_start_after_data{};
    bool subroutine_references{true};
    bool function_body{true};
    bool reference{true};
    bool data_reference{true};
    bool scalar_operand_references{true};
    bool stack{true};
    bool constant_propagation{true};
    bool non_returning_functions{true};
    bool create_analysis_bookmarks{true};
    bool seed_provider_functions{true};
    bool allow_shared_function_body{true};
    bool create_stack_parameters{};
    std::uint32_t non_return_threshold{3};
    std::size_t maximum_disassembly_instructions{100000};
    std::size_t maximum_events{1000000};
    std::filesystem::path pattern_root;
};

/// Identifies the state immediately preceding a Function Start Search match.
enum class FunctionStartAfter : std::uint8_t {
    none,
    function,
    instruction,
    data,
    pointer,
    defined,
};

/// Describes the valid-code predicate attached to a function-start action.
struct FunctionStartValidCode {
    bool existing_function{};
    bool subroutine{};
    std::uint32_t minimum_instructions{};
    std::optional<std::uint32_t> maximum_instructions;
    bool contiguous{true};
};

/// Preserves the action attributes needed by delayed Function Start phases.
struct FunctionStartProperties {
    FunctionStartAfter after{FunctionStartAfter::none};
    FunctionStartValidCode valid_code;
    std::optional<std::string> section;
    std::optional<std::string> label;
    bool possible{};
    bool thunk{};
    bool no_return{};
    std::size_t pattern_mark_offset{};
    std::int64_t alignment_mark{};
    std::uint32_t alignment_bits{};
};

/// Describes the registration contract used by the priority scheduler.
struct AnalyzerDescriptor {
    std::string name;
    std::int32_t priority{};
    std::set<EventKind> triggers;
    std::vector<std::string> prerequisites;
};

/// Carries cooperative cancellation state into an analyzer invocation.
class CancellationToken final {
public:
    /// Constructs a token initially in the non-cancelled state.
    CancellationToken() = default;

    /// Requests cancellation of the current analysis run.
    void cancel() noexcept;

    /// Clears cancellation so a new manager run can be started explicitly.
    void reset() noexcept;

    /// Reports whether cancellation was requested.
    [[nodiscard]] bool is_cancelled() const noexcept;

private:
    std::atomic_bool cancelled_{false};
};

/// Owns PE memory, Sleigh decoding, and all mutable analysis artifacts.
class AnalysisContext final {
public:
    /// Creates a context from an already validated PE image and compiled SLA.
    ///
    /// Throws `std::runtime_error` when the decoder cannot load the SLA.
    AnalysisContext(pe::LoadedPeImage image, std::filesystem::path sla_path);

    /// Prevents copying the owning provider-backed analysis state.
    AnalysisContext(const AnalysisContext&) = delete;

    /// Transfers ownership of the provider-backed analysis state.
    AnalysisContext(AnalysisContext&&) noexcept = default;

    /// Prevents copying assignment of the owning analysis state.
    AnalysisContext& operator=(const AnalysisContext&) = delete;

    /// Transfers provider and listing ownership by move assignment.
    AnalysisContext& operator=(AnalysisContext&&) noexcept = default;

    /// Returns immutable PE metadata and mapped memory.
    [[nodiscard]] const pe::LoadedPeImage& image() const noexcept;

    /// Returns the current analyzer options.
    [[nodiscard]] const AnalysisOptions& options() const noexcept;

    /// Returns mutable options used before a manager run.
    [[nodiscard]] AnalysisOptions& options() noexcept;

    /// Decodes one instruction through the owned Sleigh provider.
    [[nodiscard]] std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError> decode(Address address) const;

    /// Decodes and records one instruction if it is valid executable code.
    [[nodiscard]] bool disassemble(Address address);

    /// Follows direct flow and fall-through edges from a seed instruction.
    ///
    /// Calls are recorded as references but are not included in the traversed
    /// function-like code stream, matching CreateFunctionCmd's FollowFlow use.
    [[nodiscard]] std::size_t disassemble_flow(Address seed);

    /// Adds a decoded instruction supplied by a unit test or an adapter.
    [[nodiscard]] bool define_instruction(sleigh_runtime::Instruction instruction);

    /// Creates a function body using the current instruction flow graph.
    [[nodiscard]] bool create_function(Address entry, std::string name = {});

    /// Recomputes body blocks and CFG edges using SimpleBlockModel rules.
    [[nodiscard]] bool rebuild_function_body(Address entry);

    /// Adds one reference unless an identical relation already exists.
    [[nodiscard]] bool add_reference(Reference reference);

    /// Adds one data object unless its address and size already exist.
    [[nodiscard]] bool add_data(DataObject data);

    /// Adds one stack variable to a function unless its storage already exists.
    [[nodiscard]] bool add_stack_variable(Address function_entry, StackVariable variable);

    /// Adds or changes a flow override on a reference at an instruction.
    [[nodiscard]] bool set_flow_override(Address source, FlowOverride override_kind,
                                         std::optional<Address> target = std::nullopt);

    /// Sets the no-return property and emits a function-change event if needed.
    [[nodiscard]] bool set_function_no_return(Address entry, bool no_return);

    /// Sets the thunk property and emits a function-change event if needed.
    [[nodiscard]] bool set_function_thunk(Address entry, bool thunk);

    /// Stores recovered stack-frame metadata on a function.
    [[nodiscard]] bool set_function_stack_frame(Address entry, std::uint32_t frame_size,
                                                std::int64_t stack_pointer_delta,
                                                std::optional<std::string> frame_pointer);

    /// Sets the no-return property of an imported/external symbol.
    [[nodiscard]] bool set_external_no_return(Address iat_address, bool no_return);

    /// Removes a function entry while preserving a removal event for analyzers.
    [[nodiscard]] bool remove_function(Address entry);

    /// Adds one bookmark unless the same address/category/comment exists.
    [[nodiscard]] bool add_bookmark(Bookmark bookmark);

    /// Adds one constant fact unless the same fact is already known.
    [[nodiscard]] bool add_constant_fact(ConstantFact fact);

    /// Retains a candidate and its source pattern index for delayed analysis.
    [[nodiscard]] bool mark_potential_function_start(Address address, std::size_t pattern_index,
                                                     FunctionStartProperties properties = {});

    /// Returns decoded instructions in deterministic address order.
    [[nodiscard]] const std::map<Address, InstructionRecord>& instructions() const noexcept;

    /// Returns references in insertion order, which is also event order.
    [[nodiscard]] const std::vector<Reference>& references() const noexcept;

    /// Returns data objects in deterministic address order.
    [[nodiscard]] const std::map<Address, DataObject>& data() const noexcept;

    /// Returns functions in deterministic entry-address order.
    [[nodiscard]] const std::map<Address, Function>& functions() const noexcept;

    /// Returns retained bookmarks in deterministic insertion order.
    [[nodiscard]] const std::vector<Bookmark>& bookmarks() const noexcept;

    /// Returns retained constant facts in deterministic insertion order.
    [[nodiscard]] const std::vector<ConstantFact>& constant_facts() const noexcept;

    /// Returns parsed PE imports represented as external symbols.
    [[nodiscard]] const std::vector<ExternalSymbol>& external_symbols() const noexcept;

    /// Returns all delayed function candidates and their pattern indexes.
    [[nodiscard]] const std::map<Address, std::size_t>& potential_function_starts() const noexcept;

    /// Returns delayed action attributes by candidate address.
    [[nodiscard]] const std::map<Address, FunctionStartProperties>& potential_function_properties() const noexcept;

    /// Returns the function whose body contains an address, if any.
    [[nodiscard]] const Function* function_containing(Address address) const noexcept;

    /// Returns the function at an exact entry address, if any.
    [[nodiscard]] const Function* function_at(Address address) const noexcept;

    /// Returns a mapped executable region containing an address.
    [[nodiscard]] std::optional<pe::MemoryRegion> executable_region(Address address) const noexcept;

private:
    friend class AutoAnalysisManager;

    /// Appends a state event for the manager to consume after the current task.
    void emit(EventKind kind, Address address, bool removed = false);

    /// Seeds provisional functions from PE exception/unwind metadata.
    void seed_provider_functions();

    pe::LoadedPeImage image_;
    mutable sleigh_runtime::Decoder decoder_;
    sleigh_runtime::ProcessorContext processor_context_;
    AnalysisOptions options_;
    std::map<Address, InstructionRecord> instructions_;
    std::vector<Reference> references_;
    std::map<Address, DataObject> data_;
    std::map<Address, Function> functions_;
    std::vector<Bookmark> bookmarks_;
    std::vector<ConstantFact> constant_facts_;
    std::vector<ExternalSymbol> external_symbols_;
    std::map<Address, std::size_t> potential_function_starts_;
    std::map<Address, FunctionStartProperties> potential_function_properties_;
    std::vector<AnalysisEvent> pending_events_;
    std::uint64_t next_event_sequence_{1};
    std::size_t total_disassembled_{0};
};

/// Defines the analyzer callback implemented by every analysis feature.
class Analyzer {
public:
    /// Releases the polymorphic analyzer instance.
    virtual ~Analyzer() = default;

    /// Returns the stable name, priority, and event contract.
    [[nodiscard]] virtual AnalyzerDescriptor descriptor() const = 0;

    /// Processes one coalesced event task.
    virtual void analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                         CancellationToken& cancellation) = 0;

    /// Processes removed state addresses; the default preserves add-only analyzers.
    virtual void removed(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) {}

    /// Receives the end-of-run lifecycle callback after all eligible tasks drain.
    virtual void analysis_ended(AnalysisContext&, bool) {}
};

/// Stores extensible analyzer registrations without manager redesign.
class AnalyzerRegistry final {
public:
    /// Registers an analyzer and rejects duplicate names.
    void register_analyzer(std::unique_ptr<Analyzer> analyzer);

    /// Returns analyzers in registration order.
    [[nodiscard]] const std::vector<std::unique_ptr<Analyzer>>& analyzers() const noexcept;

private:
    std::vector<std::unique_ptr<Analyzer>> analyzers_;
};

/// Reports the result of a deterministic event-driven analysis run.
struct AnalysisResult {
    bool completed{};
    bool cancelled{};
    std::vector<std::string> errors;
    std::vector<std::string> executed_analyzers;
};

/// Represents one normalized function-body row from a Ghidra Delta report.
struct GoldenFunctionRow {
    Address entry{};
    std::vector<AddressRange> body_ranges;
};

/// Represents the structured additions, removals, changes, and references in a Delta report.
struct GoldenDelta {
    std::vector<GoldenFunctionRow> added_functions;
    std::vector<GoldenFunctionRow> removed_functions;
    std::vector<GoldenFunctionRow> changed_functions_after;
    std::vector<Reference> references;
    bool strict_references{};
};

/// Parses normalized function/reference rows from a checked-in Ghidra report.
[[nodiscard]] std::expected<GoldenDelta, std::string> parse_golden_delta(const std::filesystem::path& report);

/// Compares Delta rows against actual context state and returns all mismatches.
[[nodiscard]] std::expected<void, std::string> compare_golden_delta(const AnalysisContext& context,
                                                                    const GoldenDelta& delta);

/// Coalesces program events and executes eligible analyzers by priority.
class AutoAnalysisManager final {
public:
    /// Creates a manager attached to one mutable analysis context.
    explicit AutoAnalysisManager(AnalysisContext& context);

    /// Registers one future analyzer implementation.
    void register_analyzer(std::unique_ptr<Analyzer> analyzer);

    /// Registers all ten requested analyzers and their prerequisite phases.
    void register_builtin_analyzers();

    /// Requests cancellation of the current or next run.
    void cancel() noexcept;

    /// Runs the event-driven pipeline from PE memory and metadata seeds.
    [[nodiscard]] AnalysisResult analyze();

    /// Runs the same scheduler from explicit seed addresses.
    [[nodiscard]] AnalysisResult analyze(std::span<const Address> seeds);

    /// Requeues existing program state for a bounded repeat-analysis pass.
    [[nodiscard]] AnalysisResult re_analyze_all(std::span<const Address> restrict_set = {});

    /// Returns the registry for inspection and extension.
    [[nodiscard]] const AnalyzerRegistry& registry() const noexcept;

private:
    AnalysisContext& context_;
    AnalyzerRegistry registry_;
    CancellationToken cancellation_;
};

/// Disassembles PE entry points, exports, TLS callbacks, and runtime starts.
class DisassembleEntryPointsAnalyzer final : public Analyzer {
public:
    /// Returns the Ghidra analyzer name and block-analysis priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Processes new memory and external-entry events.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Performs the pre-function pattern pass and stores delayed candidates.
class FunctionStartPreAnalyzer final : public Analyzer {
public:
    /// Returns the pre-function pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Searches executable boundaries before ordinary function creation.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Performs the ordinary Function Start Search pattern pass.
class FunctionStartAnalyzer final : public Analyzer {
public:
    /// Returns the byte-analysis pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Validates candidates, disassembles them, and creates functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Rechecks candidates whose original constraints require an existing function.
class FunctionStartFunctionAnalyzer final : public Analyzer {
public:
    /// Returns the function-constrained pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Re-evaluates retained candidates after function events.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Performs the post-code Function Start Search phase.
class FunctionStartPostAnalyzer final : public Analyzer {
public:
    /// Returns the post-code pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Rechecks unresolved candidates after code analysis.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Performs the post-data Function Start Search phase.
class FunctionStartDataPostAnalyzer final : public Analyzer {
public:
    /// Returns the post-data pattern analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Rechecks unresolved candidates after data analysis.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Creates functions from existing direct CALL flow references only.
class SubroutineReferencesAnalyzer final : public Analyzer {
public:
    /// Returns the FunctionAnalyzer-compatible priority contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Creates missing direct-call targets without scanning raw bytes.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Builds Function Body, SimpleBlockModel, and BasicBlockModel state.
class FunctionBodyAnalyzer final : public Analyzer {
public:
    /// Returns the function-body analyzer contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Rebuilds affected function bodies and their CFGs.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Materializes references described by decoded memory p-code.
class ReferenceAnalyzer final : public Analyzer {
public:
    /// Returns the OperandReferenceAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Creates direct data references from LOAD and STORE semantics.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Follows pointer-valued data objects and creates data-origin references.
class DataReferenceAnalyzer final : public Analyzer {
public:
    /// Returns the DataOperandReferenceAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Resolves mapped pointer values without creating functions from them.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Converts eligible scalar operands into address/data references.
class ScalarOperandReferencesAnalyzer final : public Analyzer {
public:
    /// Returns the ScalarOperandAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Applies Ghidra's size, mapping, and offcut rejection rules.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Infers stack variables and stack references from function instructions.
class StackAnalyzer final : public Analyzer {
public:
    /// Returns the StackVariableAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Performs stack-frame discovery for newly created functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Evaluates decoded p-code to propagate constants through function CFGs.
class ConstantPropagationAnalyzer final : public Analyzer {
public:
    /// Returns the ConstantPropagationAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Runs bounded fixed-point symbolic propagation over affected functions.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Applies the early processor/runtime no-return name database.
class KnownNoReturnFunctionsAnalyzer final : public Analyzer {
public:
    /// Returns the early no-return priority and metadata event contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Marks known local and imported no-return symbols before flow discovery.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

/// Identifies known and evidence-backed non-returning functions.
class NonReturningFunctionsAnalyzer final : public Analyzer {
public:
    /// Returns the FindNoReturnFunctionsAnalyzer-compatible contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Applies known-name and repeated post-call evidence rules.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace ghidra::analyzer
