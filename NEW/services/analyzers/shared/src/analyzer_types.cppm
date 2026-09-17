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

export namespace recode::analyzer {

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
    external_entry_added,
    code_added,
    data_added,
    data_archive_added,
    address_table_added,
    embedded_media_added,
    symbol_added,
    pdb_symbol_added,
    pdb_type_added,
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
    std::optional<ReferenceKind> flow_original_kind;
    std::optional<Address> flow_original_fallthrough;
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

/// Describes one recovered formal parameter in a function signature.
struct FunctionParameter {
    std::string name;
    std::string type;
    std::string storage;
    std::int64_t storage_offset{};
    std::uint32_t size{};
    bool indirect{};

    /// Supports exact signature-change detection in the analysis context.
    friend bool operator==(const FunctionParameter&, const FunctionParameter&) = default;
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
    /// Concrete destination used when CreateThunkFunctionCmd semantics identify
    /// this entry as a thunk.  The optional value keeps the native model's
    /// thunk relationship observable instead of reducing it to a boolean.
    std::optional<Address> thunk_target;
    std::string calling_convention;
    std::string return_type{"void"};
    std::vector<FunctionParameter> parameters;
    bool variadic{};
    bool signature_committed{};
    bool parameter_id_complete{};
    bool switch_recovered{};
};

/// Represents one explicitly defined data object in program memory.
struct DataObject {
    Address address{};
    std::uint32_t size{};
    std::string type;
    std::string value;
    bool read_only{};
    bool alignment{};
    bool string_data{};
    std::uint32_t character_width{1};
};

/// Represents one PE import without treating the external namespace as local code.
struct ExternalSymbol {
    std::string library;
    std::string name;
    Address iat_address{};
    std::optional<std::uint16_t> ordinal;
    bool delay_loaded{};
    bool no_return{};
    bool function{};
    bool external_entry{};
    std::string demangled_name;
};

/// Stores one string model result, including the terminator and encoding rules.
struct StringRecord {
    Address address{};
    std::uint32_t size{};
    std::uint32_t character_width{1};
    std::string value;
    std::string type{"string"};
    bool terminated{};
    bool aligned{};
    bool existing{};
};

/// Stores one local or external symbol after demangling and namespace recovery.
struct SymbolRecord {
    Address address{};
    std::string mangled_name;
    std::string demangled_name;
    std::string namespace_name;
    std::string kind;
    bool external{};
    bool primary{};
};

/// Describes one datatype archive selected by Apply Data Archives.
struct DataArchiveRecord {
    std::filesystem::path path;
    std::string name;
    std::string source_language;
    std::vector<std::string> types;
    bool built_in{};
    bool applied{};
    std::string error;
};

/// Stores a validated address table and the pointers it contributed.
struct AddressTableRecord {
    Address address{};
    std::uint32_t entry_size{};
    std::vector<Address> targets;
    bool relative{};
    bool relocation_backed{};
    bool split{};
};

/// Stores the semantic result of a discovered embedded media object.
struct EmbeddedMediaRecord {
    Address address{};
    std::uint32_t size{};
    std::string type;
    bool validated{};
};

/// Stores one PDB symbol applied to the native program model.
struct PdbSymbolRecord {
    Address address{};
    std::string name;
    std::string namespace_name;
    std::string type;
    std::uint32_t size{};
    bool function{};
    bool external{};
};

/// Stores one PDB type declaration retained by the applicator.
struct PdbTypeRecord {
    std::string name;
    std::string kind;
    std::uint32_t size{};
    std::vector<std::pair<std::string, std::string>> fields;
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
    // FunctionAnalyzer's createOnlyThunks mode filters missing call targets
    // through CreateThunkFunctionCmd before scheduling function creation.
    bool create_only_thunks{};
    bool create_stack_parameters{};
    std::uint32_t non_return_threshold{3};
    std::size_t maximum_disassembly_instructions{100000};
    std::size_t maximum_events{1000000};
    std::filesystem::path pattern_root;
    /// Optional Ghidra no-return name file; an empty path uses repository defaults.
    std::filesystem::path no_return_names_file;
    bool aggressive_instruction_finder{};
    bool apply_data_archives{};
    bool ascii_strings{true};
    bool call_convention_id{};
    bool call_fixup_installer{true};
    bool condense_filler_bytes{true};
    bool create_address_tables{};
    bool decompiler_parameter_id{};
    bool decompiler_switch_analysis{};
    bool demangler_microsoft{true};
    bool embedded_media{true};
    bool external_entry_references{true};
    bool function_id{true};
    bool pdb_msdia{};
    bool pdb_universal{};
    bool shared_return_calls{true};
    bool shared_return_assume_contiguous_functions_only{true};
    bool shared_return_allow_conditional_jumps{};
    bool variadic_function_signature_override{};
    bool windows_pe_x86_propagate_external_parameters{true};
    bool windows_resource_reference{true};
    bool x86_constant_reference{true};
    std::uint32_t aggressive_minimum_functions{20};
    std::uint32_t ascii_minimum_length{5};
    std::uint32_t ascii_alignment{1};
    std::uint32_t ascii_end_alignment{4};
    bool ascii_require_null_termination{true};
    bool ascii_allow_middle_references{true};
    bool ascii_allow_existing_substrings{true};
    bool ascii_search_accessible_memory{true};
    bool ascii_force_model_reload{};
    std::filesystem::path ascii_model_file{"StringModel.sng"};
    bool ascii_create_one_time{true};
    std::uint32_t filler_minimum_length{1};
    std::uint8_t filler_byte{0};
    bool filler_auto_detect{true};
    std::uint32_t address_table_minimum_entries{2};
    std::uint32_t address_table_alignment{4};
    std::uint32_t address_table_pointer_alignment{1};
    std::uint64_t address_table_minimum_pointer_address{0x1024};
    std::uint32_t address_table_maximum_distance{0xffffff};
    bool address_table_auto_label{};
    bool address_table_relocation_guide{true};
    bool address_table_allow_offcuts{};
    std::uint32_t function_id_minimum_instructions{10};
    std::uint32_t function_id_maximum_matches{3};
    std::chrono::milliseconds decompiler_timeout{std::chrono::seconds(30)};
    std::vector<std::filesystem::path> data_archive_paths;
    std::vector<std::filesystem::path> fid_database_paths;
    std::filesystem::path pdb_path;
    std::string source_language;
    bool apply_source_language_archives{true};
    bool demangler_apply_function_signatures{true};
    bool demangler_apply_namespaces{true};
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

} // namespace recode::analyzer
