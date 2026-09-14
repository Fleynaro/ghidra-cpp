export module analyzer_types;

import std;
export import pe_loader;
export import sleigh_runtime;

// This module is the native value-model boundary for the analyzer pipeline.
// The types preserve the observable contracts of Ghidra's Program, Listing,
// Function, ReferenceManager, and analysis option objects without depending on
// the original database implementation.
// Original references:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Function.java,
// ghidra/program/model/listing/Listing.java, ghidra/program/model/symbol/Reference.java,
// ghidra/app/plugin/core/analysis/AutoAnalysisManager.java, and
// ghidra/app/plugin/core/analysis/AnalysisOptions.java.

export namespace ghidra::analyzer {

using Address = std::uint64_t;

/// Identifies a contiguous address range in the loaded program image.
struct AddressRange {
    Address start{};
    Address end{};

    /// Supports exact normalized range comparisons in structured tests.
    friend bool operator==(const AddressRange&, const AddressRange&) = default;

    /// Reports whether an address belongs to this inclusive range.
    [[nodiscard]] bool contains(Address address) const noexcept {
        return address >= start && address <= end;
    }
};

/// Identifies the state mutation that can wake one or more analyzers.
enum class EventKind : std::uint8_t {
    memory_added,
    external_added,
    external_changed,
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

    /// Supports exact CFG-change detection before function-change events.
    friend bool operator==(const BasicBlock&, const BasicBlock&) = default;
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
    // BasicBlockModel blocks retain calls; SimpleBlockModel blocks split on
    // every flow instruction, including calls.
    std::vector<BasicBlock> simple_blocks;
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
    /// Controls whether disassembly is restricted to executable PE regions.
    bool respect_execute_flag{true};
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
    bool known_non_returning_functions{true};
    bool discovered_non_returning_functions{true};
    bool create_analysis_bookmarks{true};
    bool seed_provider_functions{true};
    // CreateFunctionCmd excludes existing function bodies unless explicitly
    // requested by a shared-return or thunk recovery mode.
    bool allow_shared_function_body{};
    bool create_stack_parameters{};
    std::uint32_t non_return_threshold{3};
    std::size_t maximum_disassembly_instructions{100000};
    std::size_t maximum_events{1000000};
    std::filesystem::path pattern_root;
    /// Optional Ghidra no-return name file; an empty path uses repository defaults.
    std::filesystem::path no_return_names_file;
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

/// Reports the result of a deterministic event-driven analysis run.
struct AnalysisResult {
    bool completed{};
    bool cancelled{};
    std::vector<std::string> errors;
    std::vector<std::string> executed_analyzers;
};

} // namespace ghidra::analyzer
