export module analyzer_context;

import analyzer_types;
import std;

// MSVC does not emit inline definitions from independently compiled named
// module interfaces unless the owning class is explicitly exported. The
// static library still needs those symbols for importing analyzer modules.
#if defined(_MSC_VER)
#define GHIDRA_ANALYZER_MODULE_EXPORT __declspec(dllexport)
#else
#define GHIDRA_ANALYZER_MODULE_EXPORT
#endif

export namespace ghidra::analyzer {

class AutoAnalysisManager;

/// Owns PE memory, Sleigh decoding, and all mutable analysis artifacts.
///
/// The implementation follows the observable contracts of Ghidra's
/// `ProgramDB`, `Listing`, `ReferenceManager`, `CreateFunctionCmd`,
/// `BasicBlockModel`, and `SimpleBlockModel` while keeping this native context
/// independent from the Java database layer.
export class GHIDRA_ANALYZER_MODULE_EXPORT AnalysisContext final {
private:
    /// Adds a displacement to an address without allowing wraparound.
    [[nodiscard]] static std::optional<Address> checked_add(Address address, std::uint64_t displacement) {
        if (displacement > std::numeric_limits<Address>::max() - address) {
            return std::nullopt;
        }
        return address + displacement;
    }

    /// Returns the end address of a decoded instruction without permitting an overflowing length.
    [[nodiscard]] static std::optional<Address> instruction_end(Address address, std::size_t length) {
        if (length == 0) {
            return std::nullopt;
        }
        return checked_add(address, static_cast<std::uint64_t>(length - 1));
    }

    /// Finds an existing instruction code unit containing an address in
    /// logarithmic time, preserving Listing offcut rejection without scanning
    /// every decoded instruction.
    [[nodiscard]] static bool is_instruction_offcut(const AnalysisContext& context, Address address) {
        const auto iterator = context.instructions_.upper_bound(address);
        if (iterator == context.instructions_.begin()) {
            return false;
        }
        const auto previous = std::prev(iterator);
        if (address == previous->first) {
            return false;
        }
        const auto end = instruction_end(previous->first, previous->second.instruction.length);
        return end && address <= *end;
    }

    /// Formats addresses the same way Ghidra's default function names are printed.
    [[nodiscard]] static std::string address_name(Address address) {
        std::ostringstream stream;
        stream << "FUN_" << std::uppercase << std::hex << address;
        return stream.str();
    }

    /// Maps a decoded flow kind to the reference category used by the listing.
    [[nodiscard]] static std::optional<ReferenceKind> reference_kind(sleigh_runtime::FlowKind kind) {
        switch (kind) {
            case sleigh_runtime::FlowKind::branch:
                return ReferenceKind::unconditional_jump;
            case sleigh_runtime::FlowKind::conditional_branch:
                return ReferenceKind::conditional_jump;
            case sleigh_runtime::FlowKind::conditional_call:
                return ReferenceKind::conditional_call;
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

    /// Returns whether a reference is a call reference.
    [[nodiscard]] static bool is_call_reference(ReferenceKind kind) {
        return kind == ReferenceKind::unconditional_call || kind == ReferenceKind::conditional_call ||
               kind == ReferenceKind::computed_call || kind == ReferenceKind::external;
    }

    /// Returns whether an instruction has a side effect that
    /// CreateThunkFunctionCmd's p-code analysis rejects for automatic thunk
    /// recognition.
    [[nodiscard]] static bool has_thunk_side_effects(const sleigh_runtime::Instruction& instruction) {
        for (const auto& operation : instruction.pcode) {
            if (operation.opcode == sleigh_runtime::PcodeOpcode::store) {
                return true;
            }
            if (operation.output && operation.output->space == "register") {
                return true;
            }
            if (operation.opcode == sleigh_runtime::PcodeOpcode::call ||
                operation.opcode == sleigh_runtime::PcodeOpcode::call_ind ||
                operation.opcode == sleigh_runtime::PcodeOpcode::call_other ||
                operation.opcode == sleigh_runtime::PcodeOpcode::cbranch) {
                return true;
            }
        }
        return false;
    }

    /// Returns whether a reference is a flow reference usable by CFG construction.
    [[nodiscard]] static bool is_flow_reference(ReferenceKind kind) {
        return kind == ReferenceKind::unconditional_jump || kind == ReferenceKind::conditional_jump ||
               kind == ReferenceKind::computed_jump;
    }

    /// Converts a concrete p-code target into a mapped code address.
    [[nodiscard]] static std::optional<Address> concrete_target(const AnalysisContext& context,
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

    /// Follows direct flow and fall-through references to construct a function body.
    [[nodiscard]] static std::set<Address> follow_function_body(const AnalysisContext& context, Address entry) {
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
                // FollowFlow(..., followIntoFunctions=false) stops at another
                // function *entry* and at a fall-through symbol, not at every
                // byte already owned by another function.  Interior shared
                // code is handled later by CreateFunctionCmd's overlap
                // reconciliation.
                if (const auto containing = context.function_at(address);
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
            const auto& decoded = instruction->second.instruction;
            const auto next = checked_add(address, decoded.length);
            const bool suppressed_call_return = std::any_of(
                instruction->second.reference_indices.begin(), instruction->second.reference_indices.end(),
                [&](auto index) {
                    const auto& reference = context.references()[index];
                    return is_call_reference(reference.kind) && reference.flow_override == FlowOverride::call_return;
                });
            if (!suppressed_call_return && decoded.flow.has_fallthrough && next &&
                context.instructions().contains(*next)) {
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

    /// Converts instruction starts into the complete byte AddressSet used by Ghidra bodies.
    [[nodiscard]] static std::vector<AddressRange> build_body_ranges(const AnalysisContext& context,
                                                                     const std::set<Address>& body) {
        std::vector<AddressRange> ranges;
        for (const Address start : body) {
            const auto instruction = context.instructions().find(start);
            const auto length =
                instruction == context.instructions().end() || instruction->second.instruction.length == 0
                    ? 1U
                    : instruction->second.instruction.length;
            const auto end = instruction_end(start, length);
            if (!end) {
                continue;
            }
            const auto adjacent = ranges.empty() ? std::optional<Address>{} : checked_add(ranges.back().end, 1);
            const bool same_listing_range = !ranges.empty() && adjacent && start <= *adjacent;
            if (same_listing_range) {
                ranges.back().end = std::max(ranges.back().end, *end);
            } else {
                ranges.push_back(AddressRange{start, *end});
            }
        }
        return ranges;
    }

    /// Adds an address to a vector only once while preserving deterministic order.
    static void append_unique(std::vector<Address>& values, Address value) {
        if (std::find(values.begin(), values.end(), value) == values.end()) {
            values.push_back(value);
        }
    }

    /// Returns whether an instruction terminates a BasicBlockModel block.
    [[nodiscard]] static bool is_basic_block_terminator(const InstructionRecord& instruction,
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

    /// Builds the SimpleBlockModel view by splitting blocks after calls.
    /// Ported from Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/SimpleBlockModel.java
    /// and BasicBlockModel.java block-boundary rules.
    [[nodiscard]] static std::vector<BasicBlock> build_simple_blocks(const AnalysisContext& context,
                                                                     const Function& function,
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
                const bool call =
                    instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::conditional_call ||
                    instruction->second.instruction.flow.kind == sleigh_runtime::FlowKind::call ||
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
        const auto is_simple_flow = [](ReferenceKind kind) {
            return is_flow_reference(kind) || is_call_reference(kind);
        };
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
            const bool call_return =
                std::any_of(instruction->second.reference_indices.begin(), instruction->second.reference_indices.end(),
                            [&](const auto index) {
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

    /// Inserts every byte in an address range without wrapping at the address-space boundary.
    template <typename Set> static void insert_address_range(Set& addresses, const AddressRange& range) {
        for (Address address = range.start;;) {
            addresses.insert(address);
            if (address == range.end) {
                break;
            }
            ++address;
        }
    }

    /// Rebuilds the byte body and normalized ranges from instruction starts.
    /// CreateFunctionCmd stores byte addresses while FollowFlow discovers
    /// instruction entry points, so both views must remain synchronized.
    static void assign_body_from_starts(const AnalysisContext& context, Function& function, std::set<Address> starts) {
        function.instruction_starts = std::move(starts);
        function.body_ranges = build_body_ranges(context, function.instruction_starts);
        function.body.clear();
        for (const auto& range : function.body_ranges) {
            insert_address_range(function.body, range);
        }
    }

    /// Returns whether an instruction is fully retained by a byte body after
    /// CreateFunctionCmd overlap carving.
    [[nodiscard]] static bool contains_instruction(const AnalysisContext& context, const std::set<Address>& body,
                                                   Address start) {
        const auto instruction = context.instructions_.find(start);
        if (instruction == context.instructions_.end()) {
            return false;
        }
        const auto end = instruction_end(start, instruction->second.instruction.length);
        if (!end) {
            return false;
        }
        for (Address address = start;;) {
            if (!body.contains(address)) {
                return false;
            }
            if (address == *end) {
                return true;
            }
            ++address;
        }
    }

    /// Recognizes the bounded simple thunk flow used by
    /// CreateThunkFunctionCmd.getThunkedAddr().  The native Sleigh provider
    /// does not expose Java p-code register side-effect analysis here, so only
    /// fall-through instructions followed by one resolved unconditional jump
    /// are accepted; unresolved or conditional flows remain ordinary code.
    [[nodiscard]] std::optional<Address> simple_thunk_target(Address entry) const {
        Address cursor = entry;
        std::set<Address> visited;
        for (std::size_t count = 0; count < 8U && visited.insert(cursor).second; ++count) {
            const auto instruction = instructions_.find(cursor);
            if (instruction == instructions_.end()) {
                return std::nullopt;
            }
            if (has_thunk_side_effects(instruction->second.instruction)) {
                return std::nullopt;
            }
            const auto& flow = instruction->second.instruction.flow;
            const auto target = concrete_target(*this, flow.target);
            if ((flow.kind == sleigh_runtime::FlowKind::branch ||
                 flow.kind == sleigh_runtime::FlowKind::indirect_branch ||
                 ((flow.kind == sleigh_runtime::FlowKind::call ||
                   flow.kind == sleigh_runtime::FlowKind::indirect_call) &&
                  flow.terminal)) &&
                target) {
                return *target;
            }
            if (flow.kind != sleigh_runtime::FlowKind::none || !flow.has_fallthrough) {
                return std::nullopt;
            }
            const auto next = checked_add(cursor, instruction->second.instruction.length);
            if (!next) {
                return std::nullopt;
            }
            cursor = *next;
        }
        return std::nullopt;
    }

    /// Appends a state event for the manager to consume after the current task.
    void emit(EventKind kind, Address address, bool removed = false) {
        pending_events_.push_back(AnalysisEvent{kind, {address}, next_event_sequence_++, removed});
    }

    /// Seeds provisional functions from PE exception/unwind metadata.
    void seed_provider_functions() {
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

    pe::LoadedPeImage image_;
    mutable sleigh_runtime::Decoder decoder_;
    sleigh_runtime::ProcessorContext processor_context_;
    AnalysisOptions options_;
    std::map<Address, InstructionRecord> instructions_;
    std::vector<Reference> references_;
    std::map<Address, DataObject> data_;
    std::map<Address, Function> functions_;
    std::set<Address> function_creation_stack_;
    std::vector<Bookmark> bookmarks_;
    std::vector<ConstantFact> constant_facts_;
    std::vector<ExternalSymbol> external_symbols_;
    std::map<Address, std::size_t> potential_function_starts_;
    std::map<Address, FunctionStartProperties> potential_function_properties_;
    std::vector<AnalysisEvent> pending_events_;
    std::uint64_t next_event_sequence_{1};
    std::size_t total_disassembled_{0};

    friend class AutoAnalysisManager;

public:
    /// Creates a context from an already validated PE image and compiled SLA.
    /// Throws `std::runtime_error` when the decoder cannot load the SLA.
    AnalysisContext(pe::LoadedPeImage image, std::filesystem::path sla_path)
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

    /// Prevents copying the owning provider-backed analysis state.
    AnalysisContext(const AnalysisContext&) = delete;

    /// Transfers ownership of the provider-backed analysis state.
    AnalysisContext(AnalysisContext&&) noexcept = default;

    /// Prevents copying assignment of the owning analysis state.
    AnalysisContext& operator=(const AnalysisContext&) = delete;

    /// Transfers provider and listing ownership by move assignment.
    AnalysisContext& operator=(AnalysisContext&&) noexcept = default;

    /// Returns immutable PE metadata and mapped memory.
    [[nodiscard]] const pe::LoadedPeImage& image() const noexcept {
        return image_;
    }

    /// Returns the current analyzer options.
    [[nodiscard]] const AnalysisOptions& options() const noexcept {
        return options_;
    }

    /// Returns mutable options used before a manager run.
    [[nodiscard]] AnalysisOptions& options() noexcept {
        return options_;
    }

    /// Decodes one instruction through the owned Sleigh provider.
    [[nodiscard]] std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError>
    decode(Address address) const {
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

    /// Reports whether the current options permit decoding at an address.
    [[nodiscard]] bool can_disassemble(Address address) const noexcept {
        const auto region = image_.find_memory_region(address);
        return region && (!options_.respect_execute_flag || region->executable);
    }

    /// Decodes and records one instruction if it is valid executable code.
    [[nodiscard]] bool disassemble(Address address) {
        if (instructions_.contains(address)) {
            return false;
        }
        // A Ghidra Listing cannot hold a second instruction start inside an
        // existing code unit.  This guard is required when EntryPointAnalyzer
        // receives a whole newly-added memory range rather than one reachable
        // flow seed.
        if (is_instruction_offcut(*this, address)) {
            return false;
        }
        const auto decoded = decode(address);
        return decoded && define_instruction(*decoded);
    }

    /// Follows direct flow and fall-through edges from a seed instruction.
    /// Calls are recorded as references but are not included in the traversed
    /// function-like code stream, matching CreateFunctionCmd's FollowFlow use.
    [[nodiscard]] std::size_t disassemble_flow(Address seed) {
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

    /// Adds a decoded instruction supplied by a unit test or an adapter.
    [[nodiscard]] bool define_instruction(sleigh_runtime::Instruction instruction) {
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
                    address, *next, ReferenceKind::fallthrough, std::nullopt, std::nullopt, FlowOverride::none, false};
                static_cast<void>(add_reference(std::move(fallthrough)));
            }
        }
        return true;
    }

    /// Creates a function using CreateFunctionCmd's flow, overlap, and thunk
    /// responsibilities.  The body is constructed synchronously here; there
    /// is no upstream FunctionBodyAnalyzer that performs a second pass.
    [[nodiscard]] bool create_function(Address entry, std::string name = {}) {
        // Ghidra retries thunk/function creation around overlapping-function
        // exceptions.  This guard supplies the equivalent circular-thunk
        // protection for the native recursive target-creation path.
        if (!function_creation_stack_.insert(entry).second) {
            return false;
        }
        const auto cleanup_function = [this, entry](int*) { function_creation_stack_.erase(entry); };
        const std::unique_ptr<int, decltype(cleanup_function)> cleanup(nullptr, cleanup_function);

        if (functions_.contains(entry)) {
            return false;
        }
        // CreateFunctionCmd refuses an entry in the middle of an existing
        // instruction; accepting it would create an invalid offcut function body.
        if (is_instruction_offcut(*this, entry)) {
            return false;
        }
        // CreateFunctionCmd requires a code unit at the requested entry.  The
        // native disassembler may materialize it when this command is used by
        // an entry-point analyzer, but an unresolved call target must not be
        // guessed into a function by this command.
        if (!instructions_.contains(entry) && !can_disassemble(entry)) {
            return false;
        }
        if (!instructions_.contains(entry)) {
            static_cast<void>(disassemble_flow(entry));
        }
        if (!instructions_.contains(entry)) {
            return false;
        }

        const auto thunk_target = simple_thunk_target(entry);
        if (thunk_target && *thunk_target != entry) {
            if (!instructions_.contains(*thunk_target)) {
                static_cast<void>(disassemble_flow(*thunk_target));
            }
            if (!instructions_.contains(*thunk_target)) {
                return false;
            }
            if (!functions_.contains(*thunk_target) && !create_function(*thunk_target)) {
                return false;
            }
        }

        std::set<Address> discovered =
            thunk_target && *thunk_target != entry ? std::set<Address>{entry} : follow_function_body(*this, entry);
        if (discovered.empty()) {
            discovered.insert(entry);
        }
        std::set<Address> new_body;
        for (const Address start : discovered) {
            const auto instruction = instructions_.find(start);
            if (instruction == instructions_.end()) {
                continue;
            }
            const auto end = instruction_end(start, instruction->second.instruction.length);
            if (!end) {
                continue;
            }
            for (Address address = start;;) {
                new_body.insert(address);
                if (address == *end) {
                    break;
                }
                ++address;
            }
        }
        if (new_body.empty() || !new_body.contains(entry)) {
            return false;
        }

        // Mirror subtractBodyFromExisting(): preserve the original bodies so
        // a later failure can restore every changed owner, then carve only the
        // address interval that belongs to the new entry.
        std::vector<std::pair<Address, Function>> original_functions;
        const Address new_end = *new_body.rbegin();
        for (auto& [existing_entry, existing] : functions_) {
            if (options_.allow_shared_function_body || existing_entry == entry) {
                continue;
            }
            const bool overlaps = std::any_of(new_body.begin(), new_body.end(),
                                              [&](Address address) { return existing.body.contains(address); });
            if (!overlaps) {
                continue;
            }
            original_functions.emplace_back(existing_entry, existing);
            if (existing.body.size() == 1U) {
                const auto placeholder_starts = follow_function_body(*this, existing_entry);
                if (placeholder_starts.contains(entry)) {
                    // This is the special placeholder case in
                    // CreateFunctionCmd.subtractBodyFromExisting().
                    continue;
                }
                assign_body_from_starts(*this, existing, placeholder_starts);
            }
            const Address carve_start = entry;
            const auto carve_end =
                existing_entry < entry
                    ? std::optional<Address>{new_end}
                    : (existing_entry == 0U ? std::nullopt : std::optional<Address>{existing_entry - 1U});
            if (carve_end && carve_start <= *carve_end) {
                for (auto iterator = existing.body.begin(); iterator != existing.body.end();) {
                    if (*iterator >= carve_start && *iterator <= *carve_end) {
                        iterator = existing.body.erase(iterator);
                    } else {
                        ++iterator;
                    }
                }
                for (auto iterator = new_body.begin(); iterator != new_body.end();) {
                    if (existing.body.contains(*iterator)) {
                        iterator = new_body.erase(iterator);
                    } else {
                        ++iterator;
                    }
                }
                std::set<Address> retained_starts;
                for (const Address start : original_functions.back().second.instruction_starts) {
                    if (contains_instruction(*this, existing.body, start)) {
                        retained_starts.insert(start);
                    }
                }
                assign_body_from_starts(*this, existing, std::move(retained_starts));
            }
        }
        if (!new_body.contains(entry)) {
            for (auto& [original_entry, original] : original_functions) {
                functions_.at(original_entry) = std::move(original);
            }
            return false;
        }

        std::set<Address> new_starts;
        for (const Address start : discovered) {
            if (contains_instruction(*this, new_body, start)) {
                new_starts.insert(start);
            }
        }
        if (new_starts.empty()) {
            for (auto& [original_entry, original] : original_functions) {
                functions_.at(original_entry) = std::move(original);
            }
            return false;
        }

        Function function{entry, name.empty() ? address_name(entry) : std::move(name), {}, {}, {}, false, false};
        assign_body_from_starts(*this, function, std::move(new_starts));
        if (thunk_target && *thunk_target != entry) {
            function.thunk = true;
            function.thunk_target = thunk_target;
        }
        functions_.emplace(entry, std::move(function));
        emit(EventKind::function_added, entry);
        for (const auto& [existing_entry, original] : original_functions) {
            const auto& current = functions_.at(existing_entry);
            if (current.body != original.body || current.body_ranges != original.body_ranges ||
                current.instruction_starts != original.instruction_starts) {
                // CreateFunctionCmd records the already-carved body change;
                // rebuilding from flow here could reclaim shared interior code
                // that now belongs to the newly created function.
                emit(EventKind::function_changed, existing_entry);
            }
        }
        static_cast<void>(rebuild_function_body(entry));
        return true;
    }

    /// Recomputes body blocks and CFG edges using SimpleBlockModel rules.
    [[nodiscard]] bool rebuild_function_body(Address entry) {
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
        if (function->second.instruction_starts.empty()) {
            return false;
        }
        function->second.body_ranges = build_body_ranges(*this, function->second.instruction_starts);
        function->second.body.clear();
        for (const auto& range : function->second.body_ranges) {
            insert_address_range(function->second.body, range);
        }
        function->second.blocks.clear();
        function->second.simple_blocks.clear();
        if (function->second.instruction_starts.empty()) {
            return false;
        }
        std::set<Address> starts{entry};
        for (const Address address : function->second.instruction_starts) {
            const auto instruction = instructions_.find(address);
            if (instruction == instructions_.end()) {
                continue;
            }
            const auto& decoded = instruction->second.instruction;
            for (const auto reference_index : instruction->second.reference_indices) {
                const auto& reference = references_[reference_index];
                if (is_flow_reference(reference.kind) &&
                    function->second.instruction_starts.contains(reference.target)) {
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
                if (is_flow_reference(reference.kind) &&
                    function->second.instruction_starts.contains(reference.target)) {
                    append_unique(block.successors, reference.target);
                }
            }
            const bool suppressed_call_return = std::any_of(
                instruction->second.reference_indices.begin(), instruction->second.reference_indices.end(),
                [&](auto index) {
                    const auto& reference = references_[index];
                    return is_call_reference(reference.kind) && reference.flow_override == FlowOverride::call_return;
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
                             old_blocks != function->second.blocks ||
                             old_simple_blocks != function->second.simple_blocks;
        if (changed) {
            emit(EventKind::function_changed, entry);
        }
        return changed;
    }

    /// Adds one reference unless an identical relation already exists.
    [[nodiscard]] bool add_reference(Reference reference) {
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

    /// Adds one data object unless its address and size already exist.
    [[nodiscard]] bool add_data(DataObject data) {
        if (data.size == 0) {
            return false;
        }
        if (!image_.find_memory_region(data.address, data.size)) {
            return false;
        }
        const Address end = data.address + data.size - 1;
        for (const auto& [address, existing] : data_) {
            if (address == data.address) {
                continue;
            }
            const Address existing_end = address + existing.size - 1;
            if (!(end < address || data.address > existing_end)) {
                return false;
            }
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

    /// Adds one stack variable to a function unless its storage already exists.
    [[nodiscard]] bool add_stack_variable(Address function_entry, StackVariable variable) {
        auto function = functions_.find(function_entry);
        if (function == functions_.end()) {
            return false;
        }
        const auto duplicate =
            std::find_if(function->second.stack_variables.begin(), function->second.stack_variables.end(),
                         [&](const StackVariable& existing) {
                             return existing.offset == variable.offset && existing.size == variable.size;
                         });
        if (duplicate != function->second.stack_variables.end()) {
            return false;
        }
        function->second.stack_variables.push_back(std::move(variable));
        emit(EventKind::function_changed, function_entry);
        return true;
    }

    /// Adds or changes a flow override on a reference at an instruction.
    [[nodiscard]] bool set_flow_override(Address source, FlowOverride override_kind,
                                         std::optional<Address> target = std::nullopt) {
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

    /// Sets the no-return property and emits a function-change event if needed.
    [[nodiscard]] bool set_function_no_return(Address entry, bool no_return) {
        auto function = functions_.find(entry);
        if (function == functions_.end() || function->second.no_return == no_return) {
            return false;
        }
        function->second.no_return = no_return;
        emit(EventKind::function_changed, entry);
        return true;
    }

    /// Sets the thunk property and emits the corresponding change.
    [[nodiscard]] bool set_function_thunk(Address entry, bool thunk) {
        auto function = functions_.find(entry);
        if (function == functions_.end() || function->second.thunk == thunk) {
            return false;
        }
        function->second.thunk = thunk;
        emit(EventKind::function_changed, entry);
        return true;
    }

    /// Stores recovered stack-frame metadata on a function.
    [[nodiscard]] bool set_function_stack_frame(Address entry, std::uint32_t frame_size,
                                                std::int64_t stack_pointer_delta,
                                                std::optional<std::string> frame_pointer) {
        auto function = functions_.find(entry);
        if (function == functions_.end()) {
            return false;
        }
        if (function->second.stack_frame_size == frame_size &&
            function->second.stack_pointer_delta == stack_pointer_delta &&
            function->second.frame_pointer == frame_pointer) {
            return false;
        }
        function->second.stack_frame_size = frame_size;
        function->second.stack_pointer_delta = stack_pointer_delta;
        function->second.frame_pointer = std::move(frame_pointer);
        emit(EventKind::function_changed, entry);
        return true;
    }

    /// Sets the no-return property of an imported/external symbol.
    [[nodiscard]] bool set_external_no_return(Address iat_address, bool no_return) {
        auto symbol = std::find_if(external_symbols_.begin(), external_symbols_.end(),
                                   [&](const ExternalSymbol& item) { return item.iat_address == iat_address; });
        if (symbol == external_symbols_.end() || symbol->no_return == no_return) {
            return false;
        }
        symbol->no_return = no_return;
        emit(EventKind::external_changed, iat_address);
        return true;
    }

    /// Removes a function entry while preserving a removal event for analyzers.
    [[nodiscard]] bool remove_function(Address entry) {
        const auto function = functions_.find(entry);
        if (function == functions_.end()) {
            return false;
        }
        functions_.erase(function);
        emit(EventKind::function_changed, entry, true);
        return true;
    }

    /// Adds one bookmark unless the same address/category/comment exists.
    [[nodiscard]] bool add_bookmark(Bookmark bookmark) {
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

    /// Adds one constant fact unless the same fact is already known.
    [[nodiscard]] bool add_constant_fact(ConstantFact fact) {
        const auto duplicate =
            std::find_if(constant_facts_.begin(), constant_facts_.end(), [&](const ConstantFact& old) {
                return old.instruction == fact.instruction && old.location == fact.location &&
                       old.value == fact.value && old.path_stable == fact.path_stable &&
                       old.function_entry == fact.function_entry;
            });
        if (duplicate != constant_facts_.end()) {
            return false;
        }
        const Address address = fact.instruction;
        constant_facts_.push_back(std::move(fact));
        emit(EventKind::constant_added, address);
        return true;
    }

    /// Retains a candidate and its source pattern index for delayed analysis.
    [[nodiscard]] bool mark_potential_function_start(Address address, std::size_t pattern_index,
                                                     FunctionStartProperties properties = {}) {
        if (potential_function_starts_.contains(address)) {
            return false;
        }
        potential_function_starts_.emplace(address, pattern_index);
        potential_function_properties_.emplace(address, std::move(properties));
        return true;
    }

    /// Returns decoded instructions in deterministic address order.
    [[nodiscard]] const std::map<Address, InstructionRecord>& instructions() const noexcept {
        return instructions_;
    }

    /// Returns references in insertion order, which is also event order.
    [[nodiscard]] const std::vector<Reference>& references() const noexcept {
        return references_;
    }

    /// Returns data objects in deterministic address order.
    [[nodiscard]] const std::map<Address, DataObject>& data() const noexcept {
        return data_;
    }

    /// Returns functions in deterministic entry-address order.
    [[nodiscard]] const std::map<Address, Function>& functions() const noexcept {
        return functions_;
    }

    /// Returns retained bookmarks in deterministic insertion order.
    [[nodiscard]] const std::vector<Bookmark>& bookmarks() const noexcept {
        return bookmarks_;
    }

    /// Returns retained constant facts in deterministic insertion order.
    [[nodiscard]] const std::vector<ConstantFact>& constant_facts() const noexcept {
        return constant_facts_;
    }

    /// Returns parsed PE imports represented as external symbols.
    [[nodiscard]] const std::vector<ExternalSymbol>& external_symbols() const noexcept {
        return external_symbols_;
    }

    /// Returns all delayed function candidates and their pattern indexes.
    [[nodiscard]] const std::map<Address, std::size_t>& potential_function_starts() const noexcept {
        return potential_function_starts_;
    }

    /// Returns delayed action attributes by candidate address.
    [[nodiscard]] const std::map<Address, FunctionStartProperties>& potential_function_properties() const noexcept {
        return potential_function_properties_;
    }

    /// Returns the function whose body contains an address, if any.
    [[nodiscard]] const Function* function_containing(Address address) const noexcept {
        for (const auto& [entry, function] : functions_) {
            if (std::any_of(function.body_ranges.begin(), function.body_ranges.end(),
                            [&](const AddressRange& range) { return range.contains(address); })) {
                return &function;
            }
        }
        return nullptr;
    }

    /// Returns the function at an exact entry address, if any.
    [[nodiscard]] const Function* function_at(Address address) const noexcept {
        const auto function = functions_.find(address);
        return function == functions_.end() ? nullptr : &function->second;
    }

    /// Returns a mapped executable region containing an address.
    [[nodiscard]] std::optional<pe::MemoryRegion> executable_region(Address address) const noexcept {
        const auto region = image_.find_memory_region(address);
        if (!region || !region->executable) {
            return std::nullopt;
        }
        return region;
    }

    /// Returns the resolved simple thunk destination used by
    /// FunctionAnalyzer's optional createOnlyThunks mode.
    [[nodiscard]] std::optional<Address> thunk_target(Address entry) const {
        return simple_thunk_target(entry);
    }
};

} // namespace ghidra::analyzer
