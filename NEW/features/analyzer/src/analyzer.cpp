module analyzer;

import std;

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

/// Converts a p-code target to a mapped code address when it is concrete.
[[nodiscard]] std::optional<Address> concrete_target(const AnalysisContext& context,
                                                     const std::optional<sleigh_runtime::Varnode>& target) {
    if (!target || target->space == "register" || target->space == "const") {
        return std::nullopt;
    }
    if (context.image().is_executable(target->offset)) {
        return target->offset;
    }
    const auto translated = context.image().rva_to_va(static_cast<pe::Rva>(target->offset));
    if (translated && context.image().is_executable(*translated)) {
        return *translated;
    }
    return std::nullopt;
}

/// Returns whether a reference is a call reference.
[[nodiscard]] bool is_call_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_call || kind == ReferenceKind::conditional_call ||
           kind == ReferenceKind::computed_call || kind == ReferenceKind::external;
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
        if (function && function->provider_end && instruction != context.instructions().end() &&
            instruction->second.instruction.length != 0 &&
            checked_add(address, instruction->second.instruction.length - 1) > function->provider_end) {
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
            if (reference.kind == ReferenceKind::unconditional_call ||
                reference.kind == ReferenceKind::conditional_call || reference.kind == ReferenceKind::computed_call) {
                continue;
            }
            if (context.instructions().contains(reference.target)) {
                work.push_back(reference.target);
            }
        }
    }
    return body;
}

/// Adds an address to a vector only once while preserving deterministic order.
void append_unique(std::vector<Address>& values, Address value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

/// Returns whether a reference is a flow reference usable by CFG construction.
[[nodiscard]] bool is_flow_reference(ReferenceKind kind) {
    return kind == ReferenceKind::unconditional_jump || kind == ReferenceKind::conditional_jump ||
           kind == ReferenceKind::computed_jump;
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
            external_symbols_.push_back(
                ExternalSymbol{descriptor.dll_name, symbol.name, symbol.iat_slot_va, symbol.ordinal, false});
            emit(EventKind::external_added, symbol.iat_slot_va);
        }
    }
    for (const auto& descriptor : image_.delay_imports()) {
        for (const auto& symbol : descriptor.symbols) {
            external_symbols_.push_back(
                ExternalSymbol{descriptor.dll_name, symbol.name, symbol.iat_slot_va, symbol.ordinal, true});
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

/// Returns mutable analyzer options for configuration before a run.
AnalysisOptions& AnalysisContext::options() noexcept {
    return options_;
}

/// Decodes one instruction through Sleigh and preserves provider errors.
std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError> AnalysisContext::decode(Address address) const {
    const auto region = image_.find_memory_region(address);
    if (!region || !region->executable) {
        return std::unexpected(sleigh_runtime::DecodeError{"Address is not in executable PE memory"});
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
        if (target) {
            Reference reference{address, *target, *kind, std::nullopt, std::nullopt, FlowOverride::none, false};
            const auto next = checked_add(address, instructions_.at(address).instruction.length);
            if (is_call_reference(*kind)) {
                reference.fallthrough = next;
            }
            static_cast<void>(add_reference(std::move(reference)));
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
        if (!visited.insert(address).second || !image_.is_executable(address)) {
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
        if (const auto target = concrete_target(*this, flow.target); target) {
            work.push_back(*target);
        }
        if (flow.has_fallthrough || flow.kind == sleigh_runtime::FlowKind::none) {
            if (const auto next = checked_add(address, instruction->second.instruction.length);
                next && image_.is_executable(*next)) {
                work.push_back(*next);
            }
        }
    }
    return decoded_count;
}

/// Creates or updates one function from the current decoded flow graph.
bool AnalysisContext::create_function(Address entry, std::string name) {
    if (functions_.contains(entry) || !image_.is_executable(entry)) {
        return false;
    }
    if (!instructions_.contains(entry)) {
        static_cast<void>(disassemble_flow(entry));
    }
    if (!instructions_.contains(entry)) {
        return false;
    }
    Function function{entry, name.empty() ? address_name(entry) : std::move(name), {}, {}, {}, false, false};
    function.body = follow_function_body(*this, entry);
    if (function.body.empty()) {
        function.body.insert(entry);
    }
    functions_.emplace(entry, std::move(function));
    emit(EventKind::function_added, entry);
    static_cast<void>(rebuild_function_body(entry));
    return true;
}

/// Recomputes a function's body blocks and CFG edges from instruction flow.
bool AnalysisContext::rebuild_function_body(Address entry) {
    auto function = functions_.find(entry);
    if (function == functions_.end()) {
        return false;
    }
    const auto body = follow_function_body(*this, entry);
    if (!body.empty()) {
        function->second.body = body;
    }
    function->second.blocks.clear();
    if (function->second.body.empty()) {
        return false;
    }
    std::set<Address> starts{entry};
    for (const Address address : function->second.body) {
        const auto instruction = instructions_.find(address);
        if (instruction == instructions_.end()) {
            continue;
        }
        const auto& decoded = instruction->second.instruction;
        for (const auto reference_index : instruction->second.reference_indices) {
            const auto& reference = references_[reference_index];
            if (is_flow_reference(reference.kind) && function->second.body.contains(reference.target)) {
                starts.insert(reference.target);
            }
        }
        if (is_basic_block_terminator(instruction->second, references_)) {
            if (const auto next = checked_add(address, decoded.length); next && function->second.body.contains(*next)) {
                starts.insert(*next);
            }
        }
    }
    BasicBlock current;
    Address previous_end = 0;
    for (const Address address : function->second.body) {
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
            if (is_flow_reference(reference.kind) && function->second.body.contains(reference.target)) {
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
            if (const auto next = checked_add(last, decoded.length); next && function->second.body.contains(*next)) {
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
    emit(EventKind::function_changed, entry);
    return true;
}

/// Adds a reference while preserving the original first-reference identity.
bool AnalysisContext::add_reference(Reference reference) {
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
    emit(EventKind::external_added, iat_address);
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
               old.path_stable == fact.path_stable;
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
        if (function.body.contains(address)) {
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

namespace {

/// Converts one hexadecimal token to an address.
[[nodiscard]] std::optional<Address> parse_hex_address(std::string_view text) {
    if (text.size() < 3 || !text.starts_with("0x"))
        return std::nullopt;
    Address value = 0;
    const auto result = std::from_chars(text.data() + 2, text.data() + text.size(), value, 16);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        return std::nullopt;
    return value;
}

/// Extracts all hexadecimal addresses from one Markdown row.
[[nodiscard]] std::vector<Address> row_addresses(std::string_view row) {
    std::vector<Address> values;
    static const std::regex expression(R"(0x[0-9A-Fa-f]+)");
    for (std::cregex_iterator it(row.data(), row.data() + row.size(), expression), end; it != end; ++it) {
        if (const auto value = parse_hex_address(it->str()))
            values.push_back(*value);
    }
    return values;
}

/// Converts a function Delta Markdown row into an entry and body range row.
[[nodiscard]] std::optional<GoldenFunctionRow> parse_function_delta_row(std::string_view row, bool changed) {
    const auto values = row_addresses(row);
    if ((!changed && values.size() < 3) || (changed && values.size() < 6))
        return std::nullopt;
    GoldenFunctionRow result;
    result.entry = changed ? values[3] : values[0];
    const std::size_t start = changed ? 4 : 1;
    for (std::size_t index = start; index + 1 < values.size(); index += 2) {
        result.body_ranges.push_back(AddressRange{values[index], values[index + 1]});
        if (changed && index >= 5)
            break;
    }
    return result.body_ranges.empty() ? std::nullopt : std::optional{std::move(result)};
}

/// Maps a report reference label to the normalized native reference kind.
[[nodiscard]] std::optional<ReferenceKind> parse_reference_kind(std::string_view row) {
    static constexpr std::array<std::pair<std::string_view, ReferenceKind>, 11> names{
        std::pair{"UNCONDITIONAL_CALL", ReferenceKind::unconditional_call},
        std::pair{"CONDITIONAL_CALL", ReferenceKind::conditional_call},
        std::pair{"UNCONDITIONAL_JUMP", ReferenceKind::unconditional_jump},
        std::pair{"CONDITIONAL_JUMP", ReferenceKind::conditional_jump},
        std::pair{"COMPUTED_CALL", ReferenceKind::computed_call},
        std::pair{"COMPUTED_JUMP", ReferenceKind::computed_jump},
        std::pair{"EXTERNAL", ReferenceKind::external},
        std::pair{"SCALAR", ReferenceKind::scalar},
        std::pair{"STACK", ReferenceKind::stack},
        std::pair{"FALL_THROUGH", ReferenceKind::fallthrough},
        std::pair{"DATA", ReferenceKind::data},
    };
    for (const auto& [name, kind] : names) {
        if (row.find(name) != std::string_view::npos)
            return kind;
    }
    return std::nullopt;
}

} // namespace

/// Parses normalized function and direct-reference Delta rows from Markdown.
std::expected<GoldenDelta, std::string> parse_golden_delta(const std::filesystem::path& report) {
    std::ifstream input(report);
    if (!input)
        return std::unexpected("Unable to open golden Delta report: " + report.string());
    GoldenDelta delta;
    enum class Section { none, functions, added, removed, changed } section = Section::none;
    std::string line;
    while (std::getline(input, line)) {
        if (line == "### Added rows") {
            section = Section::added;
            continue;
        }
        if (line == "### Removed rows") {
            section = Section::removed;
            continue;
        }
        if (line == "### Changed rows") {
            section = Section::changed;
            continue;
        }
        if (line == "### Functions") {
            section = Section::functions;
            continue;
        }
        if (!line.empty() && line.front() == '#' && !line.starts_with("###"))
            section = Section::none;
        if (line.starts_with("- ") && section != Section::none && line.find("None") == std::string::npos) {
            const bool changed = section == Section::changed;
            if (const auto row = parse_function_delta_row(line, changed)) {
                if (section == Section::added)
                    delta.added_functions.push_back(*row);
                if (section == Section::removed)
                    delta.removed_functions.push_back(*row);
                if (section == Section::changed)
                    delta.changed_functions_after.push_back(*row);
            }
        }
        if (section == Section::functions && line.starts_with("| `")) {
            const auto values = row_addresses(line);
            if (values.empty())
                continue;
            if (line.find("`Added`") != std::string::npos)
                delta.added_functions.push_back(GoldenFunctionRow{values.back(), {}});
            if (line.find("`Removed`") != std::string::npos)
                delta.removed_functions.push_back(GoldenFunctionRow{values.back(), {}});
            if (line.find("`Changed`") != std::string::npos)
                delta.changed_functions_after.push_back(GoldenFunctionRow{values.back(), {}});
        }
        if (line.starts_with("|") && line.find("0x") != std::string::npos) {
            if (const auto kind = parse_reference_kind(line)) {
                const auto values = row_addresses(line);
                if (values.size() >= 3) {
                    Reference reference{values[0], values[1],          *kind, std::nullopt,
                                        values[2], FlowOverride::none, false};
                    delta.references.push_back(std::move(reference));
                }
            }
        }
    }
    return delta;
}

/// Checks exact body ranges and reference rows against the native context.
std::expected<void, std::string> compare_golden_delta(const AnalysisContext& context, const GoldenDelta& delta) {
    std::vector<std::string> errors;
    const auto body_matches = [&](const Function& function, const std::vector<AddressRange>& ranges) {
        if (ranges.empty())
            return true;
        std::vector<AddressRange> actual_ranges;
        for (const Address address : function.body) {
            const auto instruction = context.instructions().find(address);
            const auto length =
                instruction == context.instructions().end() || instruction->second.instruction.length == 0
                    ? 1U
                    : instruction->second.instruction.length;
            actual_ranges.push_back(AddressRange{address, address + length - 1});
        }
        std::sort(actual_ranges.begin(), actual_ranges.end(),
                  [](const AddressRange& left, const AddressRange& right) { return left.start < right.start; });
        std::vector<AddressRange> merged;
        for (const auto& range : actual_ranges) {
            if (!merged.empty() && range.start <= merged.back().end + 1) {
                merged.back().end = std::max(merged.back().end, range.end);
            } else {
                merged.push_back(range);
            }
        }
        for (const auto& range : ranges) {
            if (!std::any_of(merged.begin(), merged.end(), [&](const AddressRange& actual) {
                    return actual.start <= range.start && actual.end >= range.end;
                }))
                return false;
        }
        return std::all_of(merged.begin(), merged.end(), [&](const AddressRange& actual) {
            return std::any_of(ranges.begin(), ranges.end(), [&](const AddressRange& expected) {
                return expected.start <= actual.start && expected.end >= actual.end;
            });
        });
    };
    for (const auto& row : delta.added_functions) {
        const auto function = context.function_at(row.entry);
        if (!function || !body_matches(*function, row.body_ranges)) {
            errors.push_back("missing or mismatched added function 0x" + address_name(row.entry));
        }
    }
    for (const auto& row : delta.changed_functions_after) {
        const auto function = context.function_at(row.entry);
        if (!function || !body_matches(*function, row.body_ranges)) {
            errors.push_back("missing or mismatched changed function 0x" + address_name(row.entry));
        }
    }
    for (const auto& row : delta.removed_functions) {
        if (context.function_at(row.entry))
            errors.push_back("removed function still exists 0x" + address_name(row.entry));
    }
    for (const auto& expected : delta.references) {
        const bool found =
            std::any_of(context.references().begin(), context.references().end(), [&](const Reference& actual) {
                return actual.source == expected.source && actual.target == expected.target &&
                       actual.kind == expected.kind && actual.fallthrough == expected.fallthrough;
            });
        if (!found)
            errors.push_back("missing reference from 0x" + address_name(expected.source));
    }
    if (!errors.empty()) {
        std::ostringstream message;
        for (const auto& error : errors)
            message << error << '\n';
        return std::unexpected(message.str());
    }
    return {};
}

/// Attaches the event scheduler to a context.
AutoAnalysisManager::AutoAnalysisManager(AnalysisContext& context) : context_(context) {}

/// Registers a future analyzer implementation with this manager.
void AutoAnalysisManager::register_analyzer(std::unique_ptr<Analyzer> analyzer) {
    registry_.register_analyzer(std::move(analyzer));
}

/// Registers every requested analyzer in one extensible registry.
void AutoAnalysisManager::register_builtin_analyzers() {
    register_analyzer(std::make_unique<DisassembleEntryPointsAnalyzer>());
    register_analyzer(std::make_unique<FunctionStartPreAnalyzer>());
    register_analyzer(std::make_unique<NonReturningFunctionsAnalyzer>());
    register_analyzer(std::make_unique<SubroutineReferencesAnalyzer>());
    register_analyzer(std::make_unique<FunctionBodyAnalyzer>());
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
    auto dispatch = [&](const std::vector<AnalysisEvent>& events) {
        for (const auto& event : events) {
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
        return analyze();
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
    }
    return analyze(std::span<const Address>{});
}

} // namespace ghidra::analyzer
