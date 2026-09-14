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

    /// Supports exact normalized range comparisons in structured tests.
    friend bool operator==(const AddressRange&, const AddressRange&) = default;

    /// Reports whether an address belongs to this inclusive range.
    [[nodiscard]] bool contains(Address address) const noexcept;
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

    /// Reports whether the current options permit decoding at an address.
    [[nodiscard]] bool can_disassemble(Address address) const noexcept;

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

} // namespace ghidra::analyzer

namespace ghidra::analyzer {
namespace {

/// Returns whether an event kind is present in an analyzer trigger set.
[[nodiscard]] bool triggered_by(const AnalyzerDescriptor& descriptor, EventKind kind) {
    return descriptor.triggers.contains(kind);
}

/// Adds a displacement to an address without allowing wraparound.
[[nodiscard]] std::optional<Address> checked_add(Address address, std::uint64_t displacement) {
    if (displacement > std::numeric_limits<Address>::max() - address) {
        return std::nullopt;
    }
    return address + displacement;
}

/// Returns the end address of a decoded instruction without permitting an
/// overflowing length to create a wrapped body range. This mirrors the
/// address-set checks performed by Ghidra's Address implementation.
[[nodiscard]] std::optional<Address> instruction_end(Address address, std::size_t length) {
    if (length == 0) {
        return std::nullopt;
    }
    return checked_add(address, static_cast<std::uint64_t>(length - 1));
}

/// Formats addresses the same way Ghidra's default function names are printed.
[[nodiscard]] std::string address_name(Address address) {
    std::ostringstream stream;
    stream << "FUN_" << std::uppercase << std::hex << address;
    return stream.str();
}

/// Maps a decoded flow kind to the reference category used by the listing.
[[nodiscard]] std::optional<ReferenceKind> reference_kind(sleigh_runtime::FlowKind kind) {
    switch (kind) {
        case sleigh_runtime::FlowKind::branch:
            return ReferenceKind::unconditional_jump;
        case sleigh_runtime::FlowKind::conditional_branch:
            return ReferenceKind::conditional_jump;
        case sleigh_runtime::FlowKind::call:
            return ReferenceKind::unconditional_call;
        case sleigh_runtime::FlowKind::indirect_branch:
            return ReferenceKind::computed_jump;
        case sleigh_runtime::FlowKind::indirect_call:
            return ReferenceKind::computed_call;
        default:
            return std::nullopt;
    }
}

/// Returns the fall-through reference category used by the listing model.
[[nodiscard]] ReferenceKind fallthrough_reference_kind() noexcept {
    return ReferenceKind::fallthrough;
}

/// Converts a p-code target to a mapped code address when it is concrete.
[[nodiscard]] std::optional<Address> concrete_target(const AnalysisContext& context,
                                                     const std::optional<sleigh_runtime::Varnode>& target) {
    if (!target || target->space == "register" || target->space == "const") {
        return std::nullopt;
    }
    if (context.image().find_memory_region(target->offset)) {
        return target->offset;
    }
    const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(target->offset));
    if (translated && context.image().find_memory_region(*translated)) {
        return *translated;
    }
    return std::nullopt;
}

/// Returns whether a reference is a call reference.
[[nodiscard]] bool is_call_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_call || kind == ReferenceKind::conditional_call ||
           kind == ReferenceKind::computed_call || kind == ReferenceKind::external;
}

/// Returns whether a reference is a flow reference usable by CFG construction.
[[nodiscard]] bool is_flow_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_jump || kind == ReferenceKind::conditional_jump ||
           kind == ReferenceKind::computed_jump;
}

/// Computes one function body using direct flow and fall-through references.
[[nodiscard]] std::set<Address> follow_function_body(const AnalysisContext& context, Address entry) {
    std::set<Address> body;
    std::deque<Address> work;
    work.push_back(entry);
    while (!work.empty()) {
        const Address address = work.front();
        work.pop_front();
        const auto function = context.function_at(entry);
        if (function && function->provider_end && address > *function->provider_end) {
            continue;
        }
        const auto instruction = context.instructions().find(address);
        if (address != entry) {
            if (const auto containing = context.function_containing(address);
                containing && containing->entry != entry && !context.options().allow_shared_function_body) {
                continue;
            }
        }
        if (function && function->provider_end && instruction != context.instructions().end()) {
            const auto end = instruction_end(address, instruction->second.instruction.length);
            if (!end || *end > *function->provider_end) {
                continue;
            }
        }
        if (instruction == context.instructions().end()) {
            continue;
        }
        if (!body.insert(address).second) {
            continue;
        }
        if (instruction == context.instructions().end()) {
            continue;
        }
        const auto& decoded = instruction->second.instruction;
        const auto next = checked_add(address, decoded.length);
        const bool suppressed_call_return = std::any_of(instruction->second.reference_indices.begin(),
                                                        instruction->second.reference_indices.end(), [&](auto index) {
                                                            const auto& reference = context.references()[index];
                                                            return is_call_reference(reference.kind) &&
                                                                   reference.flow_override == FlowOverride::call_return;
                                                        });
        if (!suppressed_call_return && decoded.flow.has_fallthrough && next && context.instructions().contains(*next)) {
            work.push_back(*next);
        } else if (!suppressed_call_return && decoded.flow.kind == sleigh_runtime::FlowKind::none && next &&
                   context.instructions().contains(*next)) {
            work.push_back(*next);
        }
        for (const auto reference_index : instruction->second.reference_indices) {
            const auto& reference = context.references()[reference_index];
            // CreateFunctionCmd/FollowFlow follows control-flow references only. Data,
            // scalar, stack, and external operand references must not expand a function
            // body merely because their target happens to contain an instruction.
            if (!is_flow_reference(reference.kind)) {
                continue;
            }
            if (context.instructions().contains(reference.target)) {
                work.push_back(reference.target);
            }
        }
    }
    return body;
}

/// Converts instruction-start members into the complete byte AddressSet used by Ghidra bodies.
[[nodiscard]] std::vector<AddressRange> build_body_ranges(const AnalysisContext& context,
                                                          const std::set<Address>& body) {
    std::vector<AddressRange> ranges;
    for (const Address start : body) {
        const auto instruction = context.instructions().find(start);
        const auto length = instruction == context.instructions().end() || instruction->second.instruction.length == 0
                                ? 1U
                                : instruction->second.instruction.length;
        const auto end = instruction_end(start, length);
        if (!end) {
            continue;
        }
        const auto adjacent = ranges.empty() ? std::optional<Address>{} : checked_add(ranges.back().end, 1);
        if (!ranges.empty() && adjacent && start <= *adjacent) {
            ranges.back().end = std::max(ranges.back().end, *end);
        } else {
            ranges.push_back(AddressRange{start, *end});
        }
    }
    return ranges;
}

/// Adds an address to a vector only once while preserving deterministic order.
void append_unique(std::vector<Address>& values, Address value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

/// Returns whether an instruction must terminate a BasicBlockModel block.
[[nodiscard]] bool is_basic_block_terminator(const InstructionRecord& instruction,
                                             const std::vector<Reference>& references) {
    const auto kind = instruction.instruction.flow.kind;
    if (kind == sleigh_runtime::FlowKind::branch || kind == sleigh_runtime::FlowKind::conditional_branch ||
        kind == sleigh_runtime::FlowKind::indirect_branch || kind == sleigh_runtime::FlowKind::return_op ||
        instruction.instruction.flow.terminal) {
        return true;
    }
    return std::any_of(instruction.reference_indices.begin(), instruction.reference_indices.end(),
                       [&](const auto index) { return is_flow_reference(references[index].kind); });
}

/// Builds the SimpleBlockModel view from BasicBlockModel blocks by splitting
/// after calls while retaining the same decoded instruction ownership.
/// Ported from Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/SimpleBlockModel.java
/// and BasicBlockModel.java block-boundary rules.
[[nodiscard]] std::vector<BasicBlock> build_simple_blocks(const AnalysisContext& context, const Function& function,
                                                          const std::vector<Reference>& references) {
    std::vector<BasicBlock> blocks;
    for (const auto& basic : function.blocks) {
        BasicBlock current;
        for (const Address address : basic.instructions) {
            const auto instruction = context.instructions().find(address);
            if (instruction == context.instructions().end()) {
                continue;
            }
            if (current.instructions.empty()) {
                current.start = address;
            }
            current.instructions.push_back(address);
            current.end = instruction_end(address, instruction->second.instruction.length).value_or(address);
            const bool call = instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::call ||
                              instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::indirect_call;
            if (call) {
                blocks.push_back(std::move(current));
                current = BasicBlock{};
            }
        }
        if (!current.instructions.empty()) {
            blocks.push_back(std::move(current));
        }
    }
    std::map<Address, std::size_t> block_by_instruction;
    for (std::size_t index = 0; index < blocks.size(); ++index) {
        for (const Address address : blocks[index].instructions) {
            block_by_instruction.emplace(address, index);
        }
    }
    const auto is_simple_flow = [](ReferenceKind kind) { return is_flow_reference(kind) || is_call_reference(kind); };
    for (auto& block : blocks) {
        if (block.instructions.empty()) {
            continue;
        }
        const Address last = block.instructions.back();
        const auto instruction = context.instructions().find(last);
        if (instruction == context.instructions().end()) {
            continue;
        }
        for (const auto reference_index : instruction->second.reference_indices) {
            const auto& reference = references[reference_index];
            if (is_simple_flow(reference.kind) && function.instruction_starts.contains(reference.target)) {
                append_unique(block.successors, reference.target);
            }
        }
        const bool call_return = std::any_of(instruction->second.reference_indices.begin(),
                                             instruction->second.reference_indices.end(), [&](const auto index) {
                                                 return is_call_reference(references[index].kind) &&
                                                        references[index].flow_override == FlowOverride::call_return;
                                             });
        if (!call_return && (instruction->second.instruction.flow.has_fallthrough ||
                             instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::none)) {
            if (const auto next = checked_add(last, instruction->second.instruction.length);
                next && function.instruction_starts.contains(*next)) {
                append_unique(block.successors, *next);
            }
        }
        for (const Address successor : block.successors) {
            if (const auto target = block_by_instruction.find(successor); target != block_by_instruction.end()) {
                append_unique(blocks[target->second].predecessors, block.start);
            }
        }
    }
    return blocks;
}

} // namespace

/// Tests inclusive address-range membership.
bool AddressRange::contains(Address address) const noexcept {
    return address >= start && address <= end;
}

/// Marks the token cancelled for cooperative analyzer checks.
void CancellationToken::cancel() noexcept {
    cancelled_.store(true, std::memory_order_release);
}

/// Clears cancellation for an explicitly requested subsequent run.
void CancellationToken::reset() noexcept {
    cancelled_.store(false, std::memory_order_release);
}

/// Reads the cancellation flag without taking a lock.
bool CancellationToken::is_cancelled() const noexcept {
    return cancelled_.load(std::memory_order_acquire);
}

/// Constructs the provider-backed analysis context.
AnalysisContext::AnalysisContext(pe::LoadedPeImage image, std::filesystem::path sla_path)
    : image_(std::move(image)), decoder_(std::move(sla_path)), processor_context_({}) {
    // Ghidra's x86 language defaults are part of the processor context, not
    // instruction bytes. Keeping them here is required for correct relative
    // CALL/JMP targets and prevents state leakage between decoded instructions.
    if (image_.coff_header().machine == pe::Machine::amd64) {
        processor_context_.values = {{"addrsize", 2}, {"opsize", 1}, {"rexprefix", 0}, {"longMode", 1}};
    }
    for (const auto& descriptor : image_.imports()) {
        for (const auto& symbol : descriptor.symbols) {
            const std::string name =
                symbol.name.empty() && symbol.ordinal ? "Ordinal_" + std::to_string(*symbol.ordinal) : symbol.name;
            external_symbols_.push_back(
                ExternalSymbol{descriptor.dll_name, name, symbol.iat_slot_va, symbol.ordinal, false});
            emit(EventKind::external_added, symbol.iat_slot_va);
        }
    }
    for (const auto& descriptor : image_.delay_imports()) {
        for (const auto& symbol : descriptor.symbols) {
            const std::string name =
                symbol.name.empty() && symbol.ordinal ? "Ordinal_" + std::to_string(*symbol.ordinal) : symbol.name;
            external_symbols_.push_back(
                ExternalSymbol{descriptor.dll_name, name, symbol.iat_slot_va, symbol.ordinal, true});
            emit(EventKind::external_added, symbol.iat_slot_va);
        }
    }
}

/// Returns the immutable PE image owned by the context.
const pe::LoadedPeImage& AnalysisContext::image() const noexcept {
    return image_;
}

/// Returns the current analyzer options.
const AnalysisOptions& AnalysisContext::options() const noexcept {
    return options_;
}

/// Reports whether an address is mapped and allowed by the current execute
/// policy. The policy is the native equivalent of EntryPointAnalyzer's
/// `Respect Execute Flag` option.
bool AnalysisContext::can_disassemble(Address address) const noexcept {
    const auto region = image_.find_memory_region(address);
    return region && (!options_.respect_execute_flag || region->executable);
}

/// Returns mutable analyzer options for configuration before a run.
AnalysisOptions& AnalysisContext::options() noexcept {
    return options_;
}

/// Decodes one instruction through Sleigh and preserves provider errors.
std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError> AnalysisContext::decode(Address address) const {
    const auto region = image_.find_memory_region(address);
    if (!region || (options_.respect_execute_flag && !region->executable)) {
        return std::unexpected(sleigh_runtime::DecodeError{"Address is not in permitted PE memory"});
    }
    const auto offset = address - region->start;
    const auto available = region->size - offset;
    const auto bytes = image_.read_memory(address, std::min<std::uint64_t>(16, available));
    if (!bytes) {
        return std::unexpected(sleigh_runtime::DecodeError{bytes.error().message});
    }
    return decoder_.decode(address, *bytes, processor_context_);
}

/// Records one decoded instruction and materializes its direct flow reference.
bool AnalysisContext::define_instruction(sleigh_runtime::Instruction instruction) {
    if (instruction.length == 0 || instruction.address > std::numeric_limits<Address>::max() - instruction.length) {
        return false;
    }
    if (instructions_.contains(instruction.address)) {
        return false;
    }
    const Address address = instruction.address;
    const auto kind = reference_kind(instruction.flow.kind);
    instructions_.emplace(address, InstructionRecord{std::move(instruction), {}});
    emit(EventKind::code_added, address);
    if (kind) {
        const auto target = concrete_target(*this, instructions_.at(address).instruction.flow.target);
        if (target && can_disassemble(*target)) {
            Reference reference{address, *target, *kind, std::nullopt, std::nullopt, FlowOverride::none, false};
            const auto next = checked_add(address, instructions_.at(address).instruction.length);
            if (is_call_reference(*kind)) {
                reference.fallthrough = next;
            }
            static_cast<void>(add_reference(std::move(reference)));
        }
        const auto next = checked_add(address, instructions_.at(address).instruction.length);
        if (instructions_.at(address).instruction.flow.has_fallthrough && next && can_disassemble(*next)) {
            Reference fallthrough{
                address, *next, fallthrough_reference_kind(), std::nullopt, std::nullopt, FlowOverride::none, false};
            static_cast<void>(add_reference(std::move(fallthrough)));
        }
    }
    return true;
}

/// Decodes and records one instruction when its bytes are executable.
bool AnalysisContext::disassemble(Address address) {
    if (instructions_.contains(address)) {
        return false;
    }
    const auto decoded = decode(address);
    return decoded && define_instruction(*decoded);
}

/// Follows direct flow and fall-through edges from an entry seed.
std::size_t AnalysisContext::disassemble_flow(Address seed) {
    std::deque<Address> work{seed};
    std::set<Address> visited;
    std::size_t decoded_count = 0;
    while (!work.empty() && decoded_count < options_.maximum_disassembly_instructions) {
        const Address address = work.front();
        work.pop_front();
        if (!visited.insert(address).second || !can_disassemble(address)) {
            continue;
        }
        const bool was_present = instructions_.contains(address);
        if (total_disassembled_ >= options_.maximum_disassembly_instructions) {
            break;
        }
        if (!was_present && disassemble(address)) {
            ++decoded_count;
            ++total_disassembled_;
        }
        const auto instruction = instructions_.find(address);
        if (instruction == instructions_.end()) {
            continue;
        }
        const auto& flow = instruction->second.instruction.flow;
        if (const auto target = concrete_target(*this, flow.target); target && can_disassemble(*target)) {
            work.push_back(*target);
        }
        if (flow.has_fallthrough || flow.kind == sleigh_runtime::FlowKind::none) {
            if (const auto next = checked_add(address, instruction->second.instruction.length);
                next && can_disassemble(*next)) {
                work.push_back(*next);
            }
        }
    }
    return decoded_count;
}

/// Inserts every byte in an address range without wrapping at the address
/// space boundary. Ghidra address sets reject such wrapped ranges.
template <typename Set> void insert_address_range(Set& addresses, const AddressRange& range) {
    for (Address address = range.start;;) {
        addresses.insert(address);
        if (address == range.end) {
            break;
        }
        ++address;
    }
}

/// Creates or updates one function from the current decoded flow graph.
bool AnalysisContext::create_function(Address entry, std::string name) {
    if (functions_.contains(entry) || !can_disassemble(entry)) {
        return false;
    }
    // CreateFunctionCmd refuses an entry in the middle of an existing
    // instruction; accepting it would create an invalid offcut function body.
    const bool offcut = std::any_of(instructions_.begin(), instructions_.end(), [&](const auto& item) {
        const auto end = instruction_end(item.first, item.second.instruction.length);
        return end && entry > item.first && entry <= *end;
    });
    if (offcut) {
        return false;
    }
    const auto containing = function_containing(entry);
    const std::optional<Address> carved_function = containing ? std::optional{containing->entry} : std::nullopt;
    if (!instructions_.contains(entry)) {
        static_cast<void>(disassemble_flow(entry));
    }
    if (!instructions_.contains(entry)) {
        return false;
    }
    Function function{entry, name.empty() ? address_name(entry) : std::move(name), {}, {}, {}, false, false};
    function.instruction_starts = follow_function_body(*this, entry);
    if (function.instruction_starts.empty()) {
        function.instruction_starts.insert(entry);
    }
    function.body_ranges = build_body_ranges(*this, function.instruction_starts);
    for (const auto& range : function.body_ranges) {
        insert_address_range(function.body, range);
    }
    functions_.emplace(entry, std::move(function));
    emit(EventKind::function_added, entry);
    if (carved_function && !options_.allow_shared_function_body) {
        auto existing = functions_.find(*carved_function);
        if (existing != functions_.end()) {
            for (const Address address : functions_.at(entry).body) {
                existing->second.body.erase(address);
                existing->second.instruction_starts.erase(address);
            }
            existing->second.body_ranges = build_body_ranges(*this, existing->second.instruction_starts);
            static_cast<void>(rebuild_function_body(*carved_function));
        }
    }
    static_cast<void>(rebuild_function_body(entry));
    return true;
}

/// Recomputes a function's body blocks and CFG edges from instruction flow.
bool AnalysisContext::rebuild_function_body(Address entry) {
    auto function = functions_.find(entry);
    if (function == functions_.end()) {
        return false;
    }
    const auto old_body = function->second.body;
    const auto old_ranges = function->second.body_ranges;
    const auto old_blocks = function->second.blocks;
    const auto old_simple_blocks = function->second.simple_blocks;
    const auto body = follow_function_body(*this, entry);
    if (!body.empty()) {
        function->second.instruction_starts = body;
    }
    if (function->second.instruction_starts.empty())
        return false;
    function->second.body_ranges = build_body_ranges(*this, function->second.instruction_starts);
    function->second.body.clear();
    for (const auto& range : function->second.body_ranges) {
        insert_address_range(function->second.body, range);
    }
    function->second.blocks.clear();
    function->second.simple_blocks.clear();
    if (function->second.instruction_starts.empty())
        return false;
    std::set<Address> starts{entry};
    for (const Address address : function->second.instruction_starts) {
        const auto instruction = instructions_.find(address);
        if (instruction == instructions_.end()) {
            continue;
        }
        const auto& decoded = instruction->second.instruction;
        for (const auto reference_index : instruction->second.reference_indices) {
            const auto& reference = references_[reference_index];
            if (is_flow_reference(reference.kind) && function->second.instruction_starts.contains(reference.target)) {
                starts.insert(reference.target);
            }
        }
        if (is_basic_block_terminator(instruction->second, references_)) {
            if (const auto next = checked_add(address, decoded.length);
                next && function->second.instruction_starts.contains(*next)) {
                starts.insert(*next);
            }
        }
    }
    BasicBlock current;
    Address previous_end = 0;
    for (const Address address : function->second.instruction_starts) {
        const auto instruction = instructions_.find(address);
        const auto length = instruction == instructions_.end() ? 1U : instruction->second.instruction.length;
        const bool new_block = current.instructions.empty() || starts.contains(address) || address != previous_end;
        if (new_block && !current.instructions.empty()) {
            function->second.blocks.push_back(std::move(current));
            current = BasicBlock{};
        }
        if (current.instructions.empty()) {
            current.start = address;
        }
        current.instructions.push_back(address);
        previous_end = checked_add(address, length).value_or(address + 1);
        current.end = previous_end - 1;
    }
    if (!current.instructions.empty()) {
        function->second.blocks.push_back(std::move(current));
    }
    std::map<Address, std::size_t> block_by_instruction;
    for (std::size_t index = 0; index < function->second.blocks.size(); ++index) {
        for (const Address address : function->second.blocks[index].instructions) {
            block_by_instruction.emplace(address, index);
        }
    }
    for (auto& block : function->second.blocks) {
        if (block.instructions.empty()) {
            continue;
        }
        const Address last = block.instructions.back();
        const auto instruction = instructions_.find(last);
        if (instruction == instructions_.end()) {
            continue;
        }
        const auto& decoded = instruction->second.instruction;
        for (const auto reference_index : instruction->second.reference_indices) {
            const auto& reference = references_[reference_index];
            if (is_flow_reference(reference.kind) && function->second.instruction_starts.contains(reference.target)) {
                append_unique(block.successors, reference.target);
            }
        }
        const bool suppressed_call_return = std::any_of(instruction->second.reference_indices.begin(),
                                                        instruction->second.reference_indices.end(), [&](auto index) {
                                                            const auto& reference = references_[index];
                                                            return is_call_reference(reference.kind) &&
                                                                   reference.flow_override == FlowOverride::call_return;
                                                        });
        if (!suppressed_call_return &&
            (decoded.flow.has_fallthrough || decoded.flow.kind == sleigh_runtime::FlowKind::none)) {
            if (const auto next = checked_add(last, decoded.length);
                next && function->second.instruction_starts.contains(*next)) {
                append_unique(block.successors, *next);
            }
        }
        for (const Address successor : block.successors) {
            const auto block_index = block_by_instruction.find(successor);
            if (block_index != block_by_instruction.end()) {
                append_unique(function->second.blocks[block_index->second].predecessors, block.start);
            }
        }
    }
    function->second.simple_blocks = build_simple_blocks(*this, function->second, references_);
    const bool changed = old_body != function->second.body || old_ranges != function->second.body_ranges ||
                         old_blocks != function->second.blocks || old_simple_blocks != function->second.simple_blocks;
    if (changed) {
        emit(EventKind::function_changed, entry);
    }
    return changed;
}

/// Adds a reference while preserving the original first-reference identity.
bool AnalysisContext::add_reference(Reference reference) {
    // Ghidra's ReferenceManager keeps a stronger code or symbol reference
    // from being shadowed by a speculative operand result for the same source.
    if (reference.analysis_source) {
        const bool stronger_exists =
            std::any_of(references_.begin(), references_.end(), [&](const Reference& existing) {
                return !existing.analysis_source && existing.source == reference.source &&
                       existing.target == reference.target &&
                       (existing.operand_index == reference.operand_index ||
                        (!existing.operand_index && !reference.operand_index));
            });
        if (stronger_exists) {
            return false;
        }
    }
    const auto duplicate = std::find_if(references_.begin(), references_.end(), [&](const Reference& existing) {
        return existing.source == reference.source && existing.target == reference.target &&
               existing.kind == reference.kind && existing.operand_index == reference.operand_index &&
               existing.stack_offset == reference.stack_offset;
    });
    if (duplicate != references_.end()) {
        return false;
    }
    const Address source = reference.source;
    const auto index = references_.size();
    references_.push_back(std::move(reference));
    if (auto instruction = instructions_.find(source); instruction != instructions_.end()) {
        instruction->second.reference_indices.push_back(index);
    }
    emit(EventKind::reference_added, source);
    return true;
}

/// Adds one explicitly defined data object.
bool AnalysisContext::add_data(DataObject data) {
    if (data.size == 0) {
        return false;
    }
    if (!image_.find_memory_region(data.address, data.size))
        return false;
    const Address end = data.address + data.size - 1;
    for (const auto& [address, existing] : data_) {
        if (address == data.address)
            continue;
        const Address existing_end = address + existing.size - 1;
        if (!(end < address || data.address > existing_end))
            return false;
    }
    if (auto existing = data_.find(data.address); existing != data_.end()) {
        if (existing->second.size >= data.size) {
            return false;
        }
        existing->second = std::move(data);
        emit(EventKind::data_added, existing->first);
        return true;
    }
    const Address address = data.address;
    data_.emplace(address, std::move(data));
    emit(EventKind::data_added, address);
    return true;
}

/// Adds one unique stack storage object to an existing function.
bool AnalysisContext::add_stack_variable(Address function_entry, StackVariable variable) {
    auto function = functions_.find(function_entry);
    if (function == functions_.end()) {
        return false;
    }
    const auto duplicate = std::find_if(function->second.stack_variables.begin(),
                                        function->second.stack_variables.end(), [&](const StackVariable& existing) {
                                            return existing.offset == variable.offset && existing.size == variable.size;
                                        });
    if (duplicate != function->second.stack_variables.end()) {
        return false;
    }
    function->second.stack_variables.push_back(std::move(variable));
    emit(EventKind::function_changed, function_entry);
    return true;
}

/// Applies a flow override to every call reference at an instruction.
bool AnalysisContext::set_flow_override(Address source, FlowOverride override_kind, std::optional<Address> target) {
    bool changed = false;
    for (auto& reference : references_) {
        if (reference.source == source && (!target || reference.target == *target) &&
            is_call_reference(reference.kind) && reference.flow_override != override_kind) {
            reference.flow_override = override_kind;
            changed = true;
        }
    }
    if (changed) {
        emit(EventKind::flow_changed, source);
    }
    return changed;
}

/// Sets a function's no-return property and emits the corresponding change.
bool AnalysisContext::set_function_no_return(Address entry, bool no_return) {
    auto function = functions_.find(entry);
    if (function == functions_.end() || function->second.no_return == no_return) {
        return false;
    }
    function->second.no_return = no_return;
    emit(EventKind::function_changed, entry);
    return true;
}

/// Sets a function's thunk property and emits the corresponding change.
bool AnalysisContext::set_function_thunk(Address entry, bool thunk) {
    auto function = functions_.find(entry);
    if (function == functions_.end() || function->second.thunk == thunk) {
        return false;
    }
    function->second.thunk = thunk;
    emit(EventKind::function_changed, entry);
    return true;
}

/// Stores stack-frame metadata and emits a function change when it differs.
bool AnalysisContext::set_function_stack_frame(Address entry, std::uint32_t frame_size,
                                               std::int64_t stack_pointer_delta,
                                               std::optional<std::string> frame_pointer) {
    auto function = functions_.find(entry);
    if (function == functions_.end())
        return false;
    if (function->second.stack_frame_size == frame_size &&
        function->second.stack_pointer_delta == stack_pointer_delta && function->second.frame_pointer == frame_pointer)
        return false;
    function->second.stack_frame_size = frame_size;
    function->second.stack_pointer_delta = stack_pointer_delta;
    function->second.frame_pointer = std::move(frame_pointer);
    emit(EventKind::function_changed, entry);
    return true;
}

/// Sets an imported symbol's no-return property and emits an external change.
bool AnalysisContext::set_external_no_return(Address iat_address, bool no_return) {
    auto symbol = std::find_if(external_symbols_.begin(), external_symbols_.end(),
                               [&](const ExternalSymbol& item) { return item.iat_address == iat_address; });
    if (symbol == external_symbols_.end() || symbol->no_return == no_return)
        return false;
    symbol->no_return = no_return;
    emit(EventKind::external_changed, iat_address);
    return true;
}

/// Removes a function and preserves the original function-removal notification.
bool AnalysisContext::remove_function(Address entry) {
    const auto function = functions_.find(entry);
    if (function == functions_.end()) {
        return false;
    }
    functions_.erase(function);
    emit(EventKind::function_changed, entry, true);
    return true;
}

/// Adds a unique bookmark to the observable listing.
bool AnalysisContext::add_bookmark(Bookmark bookmark) {
    const auto duplicate = std::find_if(bookmarks_.begin(), bookmarks_.end(), [&](const Bookmark& existing) {
        return existing.address == bookmark.address && existing.category == bookmark.category &&
               existing.comment == bookmark.comment;
    });
    if (duplicate != bookmarks_.end()) {
        return false;
    }
    bookmarks_.push_back(std::move(bookmark));
    return true;
}

/// Adds a unique constant fact to the state.
bool AnalysisContext::add_constant_fact(ConstantFact fact) {
    const auto duplicate = std::find_if(constant_facts_.begin(), constant_facts_.end(), [&](const ConstantFact& old) {
        return old.instruction == fact.instruction && old.location == fact.location && old.value == fact.value &&
               old.path_stable == fact.path_stable && old.function_entry == fact.function_entry;
    });
    if (duplicate != constant_facts_.end()) {
        return false;
    }
    const Address address = fact.instruction;
    constant_facts_.push_back(std::move(fact));
    emit(EventKind::constant_added, address);
    return true;
}

/// Stores a candidate for one of the delayed Function Start Search passes.
bool AnalysisContext::mark_potential_function_start(Address address, std::size_t pattern_index,
                                                    FunctionStartProperties properties) {
    if (potential_function_starts_.contains(address)) {
        return false;
    }
    potential_function_starts_.emplace(address, pattern_index);
    potential_function_properties_.emplace(address, std::move(properties));
    return true;
}

/// Returns decoded instructions by address.
const std::map<Address, InstructionRecord>& AnalysisContext::instructions() const noexcept {
    return instructions_;
}

/// Returns all references in creation order.
const std::vector<Reference>& AnalysisContext::references() const noexcept {
    return references_;
}

/// Returns all data objects by address.
const std::map<Address, DataObject>& AnalysisContext::data() const noexcept {
    return data_;
}

/// Returns all functions by entry address.
const std::map<Address, Function>& AnalysisContext::functions() const noexcept {
    return functions_;
}

/// Returns all bookmarks in creation order.
const std::vector<Bookmark>& AnalysisContext::bookmarks() const noexcept {
    return bookmarks_;
}

/// Returns all known constant facts in creation order.
const std::vector<ConstantFact>& AnalysisContext::constant_facts() const noexcept {
    return constant_facts_;
}

/// Returns parsed PE imports in deterministic loader order.
const std::vector<ExternalSymbol>& AnalysisContext::external_symbols() const noexcept {
    return external_symbols_;
}

/// Returns delayed function candidates by address.
const std::map<Address, std::size_t>& AnalysisContext::potential_function_starts() const noexcept {
    return potential_function_starts_;
}

/// Returns delayed Function Start action properties by address.
const std::map<Address, FunctionStartProperties>& AnalysisContext::potential_function_properties() const noexcept {
    return potential_function_properties_;
}

/// Finds a containing function body by deterministic entry order.
const Function* AnalysisContext::function_containing(Address address) const noexcept {
    for (const auto& [entry, function] : functions_) {
        if (std::any_of(function.body_ranges.begin(), function.body_ranges.end(),
                        [&](const AddressRange& range) { return range.contains(address); })) {
            return &function;
        }
    }
    return nullptr;
}

/// Finds an exact function entry.
const Function* AnalysisContext::function_at(Address address) const noexcept {
    const auto function = functions_.find(address);
    return function == functions_.end() ? nullptr : &function->second;
}

/// Finds an executable PE region containing an address.
std::optional<pe::MemoryRegion> AnalysisContext::executable_region(Address address) const noexcept {
    const auto region = image_.find_memory_region(address);
    if (!region || !region->executable) {
        return std::nullopt;
    }
    return region;
}

/// Seeds provisional functions from PE exception/unwind metadata.
void AnalysisContext::seed_provider_functions() {
    // PE exception entries are pre-existing function evidence in the native
    // loader pipeline. Their end bounds constrain the later body pass.
    for (const auto& runtime : image_.exception_functions()) {
        if (!image_.is_executable(runtime.begin_va) || functions_.contains(runtime.begin_va)) {
            continue;
        }
        Function function{runtime.begin_va,
                          address_name(runtime.begin_va),
                          {runtime.begin_va},
                          {},
                          {},
                          false,
                          false,
                          runtime.end_va == 0 ? std::nullopt : std::optional<Address>{runtime.end_va - 1}};
        function.body_ranges = {AddressRange{runtime.begin_va, runtime.begin_va}};
        function.instruction_starts = {runtime.begin_va};
        functions_.emplace(runtime.begin_va, std::move(function));
        emit(EventKind::function_added, runtime.begin_va);
    }
}

/// Queues one event for dispatch after the current analyzer callback.
void AnalysisContext::emit(EventKind kind, Address address, bool removed) {
    pending_events_.push_back(AnalysisEvent{kind, {address}, next_event_sequence_++, removed});
}

/// Rejects duplicate analyzer names and takes ownership of the registration.
void AnalyzerRegistry::register_analyzer(std::unique_ptr<Analyzer> analyzer) {
    if (!analyzer) {
        throw std::invalid_argument("Cannot register a null analyzer");
    }
    const auto name = analyzer->descriptor().name;
    if (std::any_of(analyzers_.begin(), analyzers_.end(),
                    [&](const auto& existing) { return existing->descriptor().name == name; })) {
        throw std::invalid_argument("Analyzer is already registered: " + name);
    }
    analyzers_.push_back(std::move(analyzer));
}

/// Returns analyzer registrations in insertion order.
const std::vector<std::unique_ptr<Analyzer>>& AnalyzerRegistry::analyzers() const noexcept {
    return analyzers_;
}

/// Attaches the event scheduler to a context.
AutoAnalysisManager::AutoAnalysisManager(AnalysisContext& context) : context_(context) {}

/// Registers a future analyzer implementation with this manager.
void AutoAnalysisManager::register_analyzer(std::unique_ptr<Analyzer> analyzer) {
    registry_.register_analyzer(std::move(analyzer));
}

/// Requests cooperative cancellation on the manager token.
void AutoAnalysisManager::cancel() noexcept {
    cancellation_.cancel();
}

/// Returns the analyzer registry for inspection.
const AnalyzerRegistry& AutoAnalysisManager::registry() const noexcept {
    return registry_;
}

/// Runs the complete pipeline from all executable PE regions and metadata.
AnalysisResult AutoAnalysisManager::analyze() {
    std::vector<Address> seeds;
    for (const auto& region : context_.image().memory_regions()) {
        if (region.executable) {
            seeds.push_back(region.start);
        }
    }
    if (const auto entry = context_.image().entry_point_va(); entry && context_.image().is_executable(*entry)) {
        seeds.push_back(*entry);
    }
    for (const auto& symbol : context_.image().exported_symbols()) {
        if (!symbol.forwarded && context_.image().is_executable(symbol.address_va)) {
            seeds.push_back(symbol.address_va);
        }
    }
    if (const auto& tls = context_.image().tls(); tls) {
        for (const Address callback : tls->callback_addresses) {
            if (context_.image().is_executable(callback)) {
                seeds.push_back(callback);
            }
        }
    }
    for (const auto& runtime : context_.image().exception_functions()) {
        if (context_.image().is_executable(runtime.begin_va)) {
            seeds.push_back(runtime.begin_va);
        }
    }
    return analyze(seeds);
}

/// Runs the scheduler from explicit memory seeds with deterministic priorities.
// Ported from Ghidra:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java
// Relevant methods: startAnalysis(), scheduleAnalysis(), and the event-driven
// AnalysisTaskList/AnalysisScheduler dispatch loop.
AnalysisResult AutoAnalysisManager::analyze(std::span<const Address> seeds) {
    struct PendingTask {
        std::size_t index{};
        std::int32_t priority{};
        std::string name;
        std::uint64_t sequence{};
    };
    struct TaskOrder {
        /// Places the numerically lowest analysis priority at the queue front.
        bool operator()(const PendingTask& left, const PendingTask& right) const {
            if (left.priority != right.priority) {
                return left.priority > right.priority;
            }
            if (left.name != right.name) {
                return left.name > right.name;
            }
            return left.sequence > right.sequence;
        }
    };

    AnalysisResult result;
    if (cancellation_.is_cancelled()) {
        result.cancelled = true;
        context_.pending_events_.clear();
        for (const auto& analyzer : registry_.analyzers()) {
            analyzer->analysis_ended(context_, true);
        }
        cancellation_.reset();
        return result;
    }
    for (const auto& analyzer : registry_.analyzers()) {
        const auto descriptor = analyzer->descriptor();
        for (const auto& prerequisite : descriptor.prerequisites) {
            const auto dependency =
                std::find_if(registry_.analyzers().begin(), registry_.analyzers().end(),
                             [&](const auto& candidate) { return candidate->descriptor().name == prerequisite; });
            if (dependency == registry_.analyzers().end()) {
                result.errors.push_back(descriptor.name + ": missing prerequisite analyzer " + prerequisite);
            } else if (dependency->get()->descriptor().priority >= descriptor.priority) {
                result.errors.push_back(descriptor.name +
                                        ": prerequisite must have a higher priority: " + prerequisite);
            }
        }
    }
    if (!result.errors.empty()) {
        // A rejected analysis setup is still a completed lifecycle attempt. Ghidra
        // notifies analyzers when a run ends, including runs that cannot start because
        // their analysis graph is invalid.
        for (const auto& analyzer : registry_.analyzers()) {
            try {
                analyzer->analysis_ended(context_, false);
            } catch (const std::exception& error) {
                result.errors.push_back(analyzer->descriptor().name + ": analysis-ended: " + error.what());
            }
        }
        cancellation_.reset();
        return result;
    }
    if (context_.options().seed_provider_functions) {
        context_.seed_provider_functions();
    }
    std::priority_queue<PendingTask, std::vector<PendingTask>, TaskOrder> queue;
    std::map<std::size_t, std::vector<AnalysisEvent>> pending;
    std::set<std::size_t> scheduled;
    std::uint64_t schedule_sequence = 1;
    std::size_t processed_events = 0;
    std::size_t dispatched_events = 0;
    auto dispatch = [&](const std::vector<AnalysisEvent>& events) {
        for (const auto& event : events) {
            if (++dispatched_events > context_.options().maximum_events) {
                result.errors.push_back("AutoAnalysisManager: maximum event count exceeded");
                return;
            }
            for (std::size_t index = 0; index < registry_.analyzers().size(); ++index) {
                const auto descriptor = registry_.analyzers()[index]->descriptor();
                if (!triggered_by(descriptor, event.kind)) {
                    continue;
                }
                auto& target = pending[index];
                auto existing = std::find_if(target.begin(), target.end(), [&](const AnalysisEvent& queued) {
                    return queued.kind == event.kind && queued.removed == event.removed;
                });
                if (existing == target.end()) {
                    target.push_back(event);
                } else {
                    for (const Address address : event.addresses) {
                        append_unique(existing->addresses, address);
                    }
                    existing->sequence = std::min(existing->sequence, event.sequence);
                }
                if (scheduled.insert(index).second) {
                    queue.push(PendingTask{index, descriptor.priority, descriptor.name, schedule_sequence++});
                }
            }
        }
    };
    for (const Address seed : seeds) {
        context_.emit(EventKind::memory_added, seed);
    }
    dispatch(std::exchange(context_.pending_events_, std::vector<AnalysisEvent>{}));
    while (!queue.empty()) {
        if (++processed_events > context_.options().maximum_events) {
            result.errors.push_back("AutoAnalysisManager: maximum event count exceeded");
            break;
        }
        if (cancellation_.is_cancelled()) {
            result.cancelled = true;
            break;
        }
        const PendingTask task = queue.top();
        queue.pop();
        scheduled.erase(task.index);
        auto events = std::move(pending[task.index]);
        pending.erase(task.index);
        try {
            std::vector<AnalysisEvent> added;
            std::vector<AnalysisEvent> removed;
            for (const auto& event : events) {
                (event.removed ? removed : added).push_back(event);
            }
            if (!added.empty()) {
                registry_.analyzers()[task.index]->analyze(context_, added, cancellation_);
            }
            if (!removed.empty()) {
                registry_.analyzers()[task.index]->removed(context_, removed, cancellation_);
            }
            result.executed_analyzers.push_back(task.name);
        } catch (const std::exception& error) {
            result.errors.push_back(task.name + ": " + error.what());
        }
        dispatch(std::exchange(context_.pending_events_, std::vector<AnalysisEvent>{}));
    }
    if (result.cancelled) {
        // Mutations performed before cancellation are retained by the context, but
        // their follow-up work must not leak into a later independent run. The caller
        // can explicitly request re-analysis when it is ready to resume.
        pending.clear();
        scheduled.clear();
        while (!queue.empty()) {
            queue.pop();
        }
        context_.pending_events_.clear();
    }
    result.completed = !result.cancelled && result.errors.empty();
    for (const auto& analyzer : registry_.analyzers()) {
        try {
            analyzer->analysis_ended(context_, result.cancelled);
        } catch (const std::exception& error) {
            result.errors.push_back(analyzer->descriptor().name + ": analysis-ended: " + error.what());
            result.completed = false;
        }
    }
    cancellation_.reset();
    return result;
}

/// Requeues the existing listing state for a repeat analysis pass.
AnalysisResult AutoAnalysisManager::re_analyze_all(std::span<const Address> restrict_set) {
    if (restrict_set.empty()) {
        for (const auto& region : context_.image().memory_regions()) {
            context_.emit(EventKind::memory_added, region.start);
        }
        for (const auto& [address, instruction] : context_.instructions()) {
            static_cast<void>(instruction);
            context_.emit(EventKind::code_added, address);
        }
        for (const auto& [address, data] : context_.data()) {
            static_cast<void>(data);
            context_.emit(EventKind::data_added, address);
        }
        for (const auto& [entry, function] : context_.functions()) {
            static_cast<void>(function);
            context_.emit(EventKind::function_added, entry);
        }
        for (const auto& reference : context_.references()) {
            context_.emit(EventKind::reference_added, reference.source);
        }
        for (const auto& fact : context_.constant_facts()) {
            context_.emit(EventKind::constant_added, fact.instruction);
        }
        for (const auto& symbol : context_.external_symbols()) {
            context_.emit(EventKind::external_added, symbol.iat_address);
        }
        return analyze(std::span<const Address>{});
    }
    for (const Address address : restrict_set) {
        context_.emit(EventKind::memory_added, address);
        if (context_.instructions().contains(address)) {
            context_.emit(EventKind::code_added, address);
        }
        if (context_.data().contains(address)) {
            context_.emit(EventKind::data_added, address);
        }
        if (context_.function_containing(address)) {
            context_.emit(EventKind::function_added, context_.function_containing(address)->entry);
        }
        for (const auto& reference : context_.references()) {
            if (reference.source == address || reference.target == address) {
                context_.emit(EventKind::reference_added, reference.source);
            }
        }
        for (const auto& symbol : context_.external_symbols()) {
            if (symbol.iat_address == address) {
                context_.emit(EventKind::external_added, address);
            }
        }
    }
    return analyze(std::span<const Address>{});
}

} // namespace ghidra::analyzer