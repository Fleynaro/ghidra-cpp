module sleigh_runtime;

import std;
import :internal;

namespace sleigh_runtime {
namespace {

/// Converts a legacy Ghidra varnode into the owning public representation.
[[nodiscard]] Varnode materialize_varnode(const ghidra::VarnodeData& varnode) {
    // Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc
    // SleighBuilder::generateLocation() and PcodeCacher::emit().
    if (varnode.space == nullptr) {
        throw std::runtime_error("Sleigh emitted a p-code varnode without an address space");
    }
    return Varnode{varnode.space->getName(), varnode.offset, varnode.size};
}

/// Captures the assembly callback emitted by the legacy Sleigh runtime.
// Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc
// Sleigh::printAssembly().
class AssemblyCapture final : public ghidra::AssemblyEmit {
public:
    /// Stores the mnemonic and operand body for one decoded instruction.
    void dump(const ghidra::Address&, const ghidra::string& mnemonic, const ghidra::string& body) override {
        mnemonic_ = mnemonic;
        body_ = body;
    }

    /// Returns the captured instruction mnemonic.
    [[nodiscard]] const std::string& mnemonic() const {
        return mnemonic_;
    }

    /// Returns the captured printable operand body.
    [[nodiscard]] const std::string& body() const {
        return body_;
    }

private:
    std::string mnemonic_;
    std::string body_;
};

/// Captures and owns every raw p-code operation emitted for one instruction.
// Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc
// Sleigh::oneInstruction() and PcodeEmit::dump().
class PcodeCapture final : public ghidra::PcodeEmit {
public:
    /// Copies the raw operation before the legacy cache can reuse its storage.
    void dump(const ghidra::Address&, ghidra::OpCode opcode, ghidra::VarnodeData* output, ghidra::VarnodeData* inputs,
              ghidra::int4 input_count) override {
        PcodeOp op{static_cast<PcodeOpcode>(opcode), std::nullopt, {}};
        if (output != nullptr) {
            op.output = materialize_varnode(*output);
        }
        op.inputs.reserve(static_cast<std::size_t>(input_count));
        for (ghidra::int4 index = 0; index < input_count; ++index) {
            op.inputs.push_back(materialize_varnode(inputs[index]));
        }
        if ((opcode == ghidra::CPUI_LOAD || opcode == ghidra::CPUI_STORE) && input_count > 0 &&
            inputs[0].space != nullptr && inputs[0].space->getType() == ghidra::IPTR_CONSTANT) {
            auto* memory_space = reinterpret_cast<ghidra::AddrSpace*>(inputs[0].offset);
            if (memory_space != nullptr) {
                op.memory_space = memory_space->getName();
            }
        }
        operations_.push_back(std::move(op));
    }

    /// Moves the captured p-code sequence into the public result.
    std::vector<PcodeOp> take_operations() && {
        return std::move(operations_);
    }

private:
    std::vector<PcodeOp> operations_;
};

/// Stores a bounded instruction byte window for the standalone SleighLoadImage interface.
// Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/loadimage.hh
// LoadImage::loadFill().
class ByteLoadImage final : public ghidra::SleighLoadImage {
public:
    /// Creates an image whose bytes are supplied by the public decoder call.
    ByteLoadImage() = default;

    /// Replaces the current 16-byte instruction window.
    void set_bytes(std::uint64_t address, std::span<const std::uint8_t> bytes) {
        address_ = address;
        std::fill(window_.begin(), window_.end(), 0);
        std::copy(bytes.begin(), bytes.end(), window_.begin());
    }

    /// Supplies bytes requested by Sleigh's parser and rejects unrelated addresses.
    void loadFill(ghidra::uint1* destination, ghidra::int4 size, const ghidra::Address& address) override {
        if (destination == nullptr || size < 0) {
            throw ghidra::DataUnavailError("Invalid load image request");
        }
        if (address.getOffset() != address_ || static_cast<std::size_t>(size) > window_.size()) {
            throw ghidra::DataUnavailError("Requested bytes are outside the instruction window");
        }
        std::copy_n(window_.begin(), size, destination);
    }

    /// Identifies this in-memory image to the legacy runtime.
    [[nodiscard]] ghidra::string getArchType() const override {
        return "sleigh-runtime";
    }

    /// Raw windows are already addressed by the caller, so no VMA adjustment is needed.
    void adjustVma(long) override {}

private:
    std::array<std::uint8_t, 16> window_{};
    std::uint64_t address_ = 0;
};

/// Removes surrounding whitespace from a printable operand.
[[nodiscard]] std::string trim(std::string value) {
    const auto is_space = [](unsigned char character) { return std::isspace(character) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char character) {
                    return !is_space(static_cast<unsigned char>(character));
                }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [&](char character) { return !is_space(static_cast<unsigned char>(character)); })
                    .base(),
                value.end());
    return value;
}

/// Splits an x86 operand body without splitting commas nested in memory expressions.
[[nodiscard]] std::vector<std::string> split_operands(std::string_view body) {
    std::vector<std::string> operands;
    std::string current;
    int nesting = 0;
    for (const char character : body) {
        if (character == '[' || character == '(') {
            ++nesting;
        } else if (character == ']' || character == ')') {
            nesting = std::max(0, nesting - 1);
        }
        if (character == ',' && nesting == 0) {
            operands.push_back(trim(std::move(current)));
            current.clear();
        } else {
            current += character;
        }
    }
    current = trim(std::move(current));
    if (!current.empty()) {
        operands.push_back(std::move(current));
    }
    return operands;
}

/// Parses a decimal or hexadecimal literal when the printed operand is one literal.
[[nodiscard]] std::optional<std::uint64_t> parse_literal(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const int base = text.starts_with("0x") || text.starts_with("0X") ? 16 : 10;
    if (base == 16) {
        first += 2;
    }
    const auto result = std::from_chars(first, last, value, base);
    if (result.ec != std::errc{} || result.ptr != last) {
        return std::nullopt;
    }
    return value;
}

/// Classifies a printed operand while retaining its exact assembly spelling.
[[nodiscard]] Operand classify_operand(std::string text) {
    const std::string lower = [&text] {
        std::string value = text;
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return value;
    }();
    Operand operand{std::move(text), OperandKind::unknown, std::nullopt};
    if (lower.find('[') != std::string::npos || lower.find(']') != std::string::npos) {
        operand.kind = OperandKind::memory;
    } else if (const auto literal = parse_literal(lower); literal.has_value()) {
        operand.kind = OperandKind::immediate;
        operand.value = literal;
    } else if (lower.starts_with("lab_") || lower.starts_with("fun_") || lower.starts_with("dat_")) {
        operand.kind = OperandKind::address;
    } else if (!lower.empty() && std::isalpha(static_cast<unsigned char>(lower.front())) != 0) {
        operand.kind = OperandKind::register_value;
    }
    return operand;
}

/// Maps p-code operations to the first control-flow effect in emission order.
[[nodiscard]] FlowInfo find_flow(std::span<const PcodeOp> operations) {
    std::optional<Varnode> conditional_target;
    bool has_conditional_branch = false;
    for (const PcodeOp& operation : operations) {
        FlowKind kind = FlowKind::none;
        switch (operation.opcode) {
            case PcodeOpcode::branch:
                kind = has_conditional_branch ? FlowKind::conditional_branch : FlowKind::branch;
                break;
            case PcodeOpcode::cbranch:
                has_conditional_branch = true;
                if (!operation.inputs.empty()) {
                    conditional_target = operation.inputs.front();
                }
                break;
            case PcodeOpcode::branch_ind:
                kind = FlowKind::indirect_branch;
                break;
            case PcodeOpcode::call:
                kind = has_conditional_branch ? FlowKind::conditional_call : FlowKind::call;
                break;
            case PcodeOpcode::call_ind:
                kind = has_conditional_branch ? FlowKind::conditional_call : FlowKind::indirect_call;
                break;
            case PcodeOpcode::return_op:
                kind = FlowKind::return_op;
                break;
            default:
                break;
        }
        if (kind != FlowKind::none) {
            FlowInfo flow{kind, std::nullopt, true, false};
            if (!operation.inputs.empty()) {
                flow.target = operation.inputs.front();
            }
            if (kind == FlowKind::conditional_call && conditional_target) {
                // A conditional call's first p-code input is the call
                // destination; the cbranch condition is not a reference
                // destination.  This mirrors Sleigh's FlowType mapping rather
                // than manufacturing a second control-flow edge.
                flow.target = operation.inputs.empty() ? conditional_target : operation.inputs.front();
            } else if (kind == FlowKind::conditional_branch && conditional_target) {
                flow.target = conditional_target;
            }
            flow.has_fallthrough = kind == FlowKind::conditional_branch || kind == FlowKind::conditional_call ||
                                   kind == FlowKind::call || kind == FlowKind::indirect_call;
            flow.terminal =
                kind == FlowKind::branch || kind == FlowKind::indirect_branch || kind == FlowKind::return_op;
            return flow;
        }
    }
    if (has_conditional_branch) {
        // A cbranch without a following call or branch is itself the first
        // control-flow effect.  Delaying this return is what lets a Sleigh
        // conditional-call sequence retain its call destination while still
        // preserving ordinary conditional branches.
        return {FlowKind::conditional_branch, conditional_target, true, false};
    }
    return {};
}

/// Applies the original SleighDebugLogger operand-mask recursion to one pattern expression.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighDebugLogger.java
void combine_pattern_mask(ghidra::ParserWalker& walker, ghidra::ConstructState* state,
                          ghidra::PatternExpression* expression, int offset, std::vector<std::uint8_t>& mask);

/// Combines the visible print-piece masks belonging to one resolved constructor state.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighDebugLogger.java
void combine_symbol_mask(ghidra::ParserWalker& walker, ghidra::ConstructState* state, std::vector<std::uint8_t>& mask);

/// Recursively combines one operand's constructor and token-field value bits.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighDebugLogger.java
void combine_operand_mask(ghidra::ParserWalker& walker, ghidra::ConstructState* state, ghidra::OperandSymbol* symbol,
                          std::vector<std::uint8_t>& mask) {
    ghidra::PatternExpression* expression = nullptr;
    if (state == nullptr || state->ct == nullptr || symbol == nullptr)
        return;
    const int handle = symbol->getIndex();
    if (handle < 0 || handle >= state->ct->getNumOperands())
        return;
    if (auto* triple = symbol->getDefiningSymbol()) {
        if (triple->getType() == ghidra::SleighSymbol::subtable_symbol) {
            auto* substate = state->resolve[handle];
            if (substate == nullptr)
                return;
            combine_symbol_mask(walker, substate, mask);
        } else {
            expression = triple->getPatternExpression();
        }
    } else {
        expression = symbol->getDefiningExpression();
    }
    if (symbol->getOffsetBase() < 0) {
        combine_pattern_mask(walker, state, expression,
                             static_cast<int>(state->offset) + static_cast<int>(symbol->getRelativeOffset()), mask);
    } else {
        auto* substate = state->resolve[handle];
        if (substate == nullptr)
            return;
        combine_pattern_mask(walker, substate, expression, static_cast<int>(substate->offset), mask);
    }
}

/// Recursively combines unary, binary, operand, and token-field pattern expressions.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighDebugLogger.java
void combine_pattern_mask(ghidra::ParserWalker& walker, ghidra::ConstructState* state,
                          ghidra::PatternExpression* expression, int offset, std::vector<std::uint8_t>& mask) {
    if (expression == nullptr)
        return;
    if (const auto* unary = dynamic_cast<const ghidra::UnaryExpression*>(expression)) {
        combine_pattern_mask(walker, state, unary->getUnary(), offset, mask);
        return;
    }
    if (const auto* binary = dynamic_cast<const ghidra::BinaryExpression*>(expression)) {
        combine_pattern_mask(walker, state, binary->getLeft(), offset, mask);
        combine_pattern_mask(walker, state, binary->getRight(), offset, mask);
        return;
    }
    if (const auto* operand = dynamic_cast<const ghidra::OperandValue*>(expression)) {
        auto* constructor = operand->getConstructor();
        if (constructor == nullptr || operand->getIndex() < 0 || operand->getIndex() >= constructor->getNumOperands())
            return;
        combine_operand_mask(walker, state, constructor->getOperand(operand->getIndex()), mask);
        return;
    }
    const auto* field = dynamic_cast<const ghidra::TokenField*>(expression);
    if (field == nullptr)
        return;
    const int start = field->getByteStart();
    const int end = field->getByteEnd();
    const int start_bit = field->getBitStart() % 8;
    const int end_bit = field->getBitEnd() % 8;
    for (int index = start; index <= end; ++index) {
        int first_bit = 0;
        int last_bit = 7;
        if (index == end) {
            if (field->isBigEndian())
                first_bit = start_bit;
            else
                last_bit = end_bit;
        }
        if (index == start) {
            if (field->isBigEndian())
                last_bit = end_bit;
            else
                first_bit = start_bit;
        }
        const auto width = 7 - last_bit + first_bit;
        const auto byte_mask = static_cast<std::uint8_t>((0xffU >> width) << first_bit);
        if (index + offset >= 0 && static_cast<std::size_t>(index + offset) < mask.size()) {
            mask[static_cast<std::size_t>(index + offset)] |= byte_mask;
        }
    }
}

/// Combines only the operand print pieces, matching getOpRepresentationList().
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
void combine_symbol_mask(ghidra::ParserWalker& walker, ghidra::ConstructState* state, std::vector<std::uint8_t>& mask) {
    if (state == nullptr || state->ct == nullptr)
        return;
    for (const auto& piece : state->ct->getPrintPieces()) {
        if (piece.size() > 1 && piece[0] == '\n') {
            const auto index = static_cast<int>(piece[1] - 'A');
            if (index >= 0 && index < state->ct->getNumOperands())
                combine_operand_mask(walker, state, state->ct->getOperand(index), mask);
        }
    }
}

/// Collects the exact fixed handles printed by one operand representation.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
void collect_operand_handles(ghidra::ParserWalker& walker, ghidra::OperandSymbol* symbol,
                             std::vector<ghidra::FixedHandle>& handles);

/// Collects all visible operand handles from one matched subconstructor.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
void collect_constructor_handles(ghidra::ParserWalker& walker, std::vector<ghidra::FixedHandle>& handles) {
    if (walker.getConstructor() == nullptr)
        return;
    for (const auto& piece : walker.getConstructor()->getPrintPieces()) {
        if (piece.size() > 1 && piece[0] == '\n') {
            const auto index = static_cast<int>(piece[1] - 'A');
            if (index >= 0 && index < walker.getConstructor()->getNumOperands())
                collect_operand_handles(walker, walker.getConstructor()->getOperand(index), handles);
        }
    }
}

/// Collects one operand's nested print representation and resolved fixed handles.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
void collect_operand_handles(ghidra::ParserWalker& walker, ghidra::OperandSymbol* symbol,
                             std::vector<ghidra::FixedHandle>& handles) {
    if (symbol == nullptr || walker.getConstructor() == nullptr || symbol->getIndex() < 0 ||
        symbol->getIndex() >= walker.getConstructor()->getNumOperands())
        return;
    walker.pushOperand(symbol->getIndex());
    if (!walker.isState())
        return;
    if (auto* triple = symbol->getDefiningSymbol();
        triple && triple->getType() == ghidra::SleighSymbol::subtable_symbol) {
        collect_constructor_handles(walker, handles);
    } else {
        handles.push_back(walker.getParentHandle());
    }
    walker.popOperand();
}

/// Collects printed operand handles directly from resolved ConstructState children.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/symbol/OperandSymbol.java
void collect_state_handles(ghidra::ConstructState* state, ghidra::OperandSymbol* symbol,
                           std::vector<ghidra::FixedHandle>& handles) {
    if (state == nullptr || state->ct == nullptr || symbol == nullptr)
        return;
    const auto index = symbol->getIndex();
    if (index < 0 || index >= state->ct->getNumOperands())
        return;
    auto* child = state->resolve[index];
    if (child == nullptr || child->ct == nullptr)
        return;
    if (auto* triple = symbol->getDefiningSymbol();
        triple && triple->getType() == ghidra::SleighSymbol::subtable_symbol) {
        for (const auto& piece : child->ct->getPrintPieces()) {
            if (piece.size() > 1 && piece[0] == '\n') {
                const auto nested_index = static_cast<int>(piece[1] - 'A');
                if (nested_index >= 0 && nested_index < child->ct->getNumOperands())
                    collect_state_handles(child, child->ct->getOperand(nested_index), handles);
            }
        }
    } else {
        auto handle = child->hand;
        if (handle.offset_size == 0)
            handle.offset_size = child->length;
        handles.push_back(handle);
    }
}

/// Carries one flattened OperandSymbol representation and its exact value mask.
struct FlattenedOperand {
    std::vector<std::uint8_t> mask;
    std::vector<ghidra::FixedHandle> handles;
};

/// Flattens nested Sleigh print operands exactly as OperandSymbol.printList().
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/symbol/OperandSymbol.java
void collect_flattened_operands(ghidra::ParserWalker& walker, ghidra::ConstructState* state,
                                ghidra::OperandSymbol* symbol, std::size_t length,
                                std::vector<FlattenedOperand>& result) {
    walker.pushOperand(symbol->getIndex());
    if (auto* triple = symbol->getDefiningSymbol();
        triple && triple->getType() == ghidra::SleighSymbol::subtable_symbol) {
        auto* substate = state->resolve[symbol->getIndex()];
        for (const auto& piece : substate->ct->getPrintPieces()) {
            if (piece.size() > 1 && piece[0] == '\n') {
                collect_flattened_operands(walker, substate, substate->ct->getOperand(piece[1] - 'A'), length, result);
            }
        }
    } else {
        FlattenedOperand flattened;
        flattened.mask.assign(length, 0);
        combine_operand_mask(walker, state, symbol, flattened.mask);
        flattened.handles.push_back(walker.getParentHandle());
        result.push_back(std::move(flattened));
    }
    walker.popOperand();
}

/// Converts one fixed handle into the Scalar, Register, or Address object consumed by FunctionID.
// Ghidra reference:
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
void append_hash_handle(const ghidra::Sleigh& translator, const ghidra::Address& current,
                        const ghidra::FixedHandle& handle, bool whole_scalar, bool address_scalar,
                        std::vector<Operand::HashObject>& objects) {
    if (handle.space == nullptr)
        return;
    const auto type = handle.space->getType();
    if (type == ghidra::IPTR_CONSTANT) {
        auto value = static_cast<std::int64_t>(handle.offset_offset);
        const auto effective_size =
            handle.size != 0
                ? handle.size
                : (handle.offset_size != 0 ? handle.offset_size : translator.getDefaultCodeSpace()->getAddrSize());
        const auto width = static_cast<unsigned>(effective_size) * 8U;
        if (width != 0 && width < 64U &&
            (handle.offset_offset & (static_cast<ghidra::uintb>(1) << (width - 1U))) != 0) {
            value |= static_cast<std::int64_t>(~((static_cast<ghidra::uintb>(1) << width) - 1U));
        }
        objects.push_back({Operand::HashObject::Kind::scalar, value, whole_scalar, address_scalar, false});
        return;
    }
    if (!translator.getRegisterName(handle.space, handle.offset_offset, handle.size).empty()) {
        objects.push_back({Operand::HashObject::Kind::register_value, static_cast<std::int64_t>(handle.offset_offset),
                           true, false, false});
        return;
    }
    if (handle.offset_space == nullptr) {
        objects.push_back(
            {Operand::HashObject::Kind::address, static_cast<std::int64_t>(handle.offset_offset), false, true, false});
    } else if (!translator.getRegisterName(handle.offset_space, handle.offset_offset, handle.offset_size).empty()) {
        objects.push_back({Operand::HashObject::Kind::register_value, static_cast<std::int64_t>(handle.offset_offset),
                           true, false, false});
    }
    (void)current;
}

/// Appends a hash object while excluding non-address Sleigh spaces for generic architectures.
void append_generic_hash_handle(const ghidra::Sleigh& translator, const ghidra::FixedHandle& handle, bool whole_scalar,
                                bool address_scalar, std::vector<Operand::HashObject>& objects) {
    if (handle.space == nullptr)
        return;
    const auto type = handle.space->getType();
    if (type == ghidra::IPTR_CONSTANT) {
        auto value = static_cast<std::int64_t>(handle.offset_offset);
        const auto effective_size =
            handle.size != 0
                ? handle.size
                : (handle.offset_size != 0 ? handle.offset_size : translator.getDefaultCodeSpace()->getAddrSize());
        const auto width = static_cast<unsigned>(effective_size) * 8U;
        if (width != 0 && width < 64U && (handle.offset_offset & (static_cast<ghidra::uintb>(1) << (width - 1U))) != 0)
            value |= static_cast<std::int64_t>(~((static_cast<ghidra::uintb>(1) << width) - 1U));
        objects.push_back({Operand::HashObject::Kind::scalar, value, whole_scalar, address_scalar, false});
        return;
    }
    if (type != ghidra::IPTR_PROCESSOR)
        return;
    if (!translator.getRegisterName(handle.space, handle.offset_offset, handle.size).empty()) {
        objects.push_back({Operand::HashObject::Kind::register_value, static_cast<std::int64_t>(handle.offset_offset),
                           true, false, false});
    } else if (handle.offset_space == nullptr) {
        objects.push_back(
            {Operand::HashObject::Kind::address, static_cast<std::int64_t>(handle.offset_offset), false, true, false});
    } else if (handle.offset_space->getType() == ghidra::IPTR_PROCESSOR &&
               !translator.getRegisterName(handle.offset_space, handle.offset_offset, handle.offset_size).empty()) {
        objects.push_back({Operand::HashObject::Kind::register_value, static_cast<std::int64_t>(handle.offset_offset),
                           true, false, false});
    }
}

/// Returns whether a Sleigh translator exposes the canonical x86 register names.
bool is_x86_translator(const ghidra::Sleigh& translator) {
    try {
        (void)translator.getRegister("RAX");
        return true;
    } catch (...) {
        try {
            (void)translator.getRegister("EAX");
            return true;
        } catch (...) {
            return false;
        }
    }
}

/// Builds the exact FunctionID instruction and operand masks from the matched Sleigh constructor tree.
// Ghidra references:
// Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/hash/MessageDigestFidHasher.java and
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
void materialize_hash_metadata(const ghidra::Sleigh& translator, ghidra::ParserContext* parser,
                               std::vector<Operand>& operands, std::vector<std::uint8_t>& instruction_mask) {
    if (parser == nullptr)
        return;
    auto* root = parser->getBaseState();
    if (root == nullptr || root->ct == nullptr)
        return;
    const auto length = static_cast<std::size_t>(parser->getLength());
    // The Sleigh pattern path supplies context-variable bits; all remaining
    // instruction bytes are fixed constructor bits before operand masks clear
    // relocatable/register/immediate fields, matching PatternGroup.getMask().
    instruction_mask.assign(length, 0xffU);
    std::vector<ghidra::uint1> fixed_mask;
    std::vector<ghidra::uint1> fixed_value;
    if (translator.getInstructionMask(parser->getAddr(), fixed_mask, fixed_value)) {
        const auto count = std::min(length, fixed_mask.size());
        for (std::size_t index = 0; index < count; ++index) {
            if (fixed_mask[index] != 0)
                instruction_mask[index] = fixed_mask[index];
        }
    }
    ghidra::ParserWalker walker(parser);
    walker.baseState();
    auto* mnemonic_state = root;
    while (mnemonic_state->ct->getFlowthruIndex() >= 0) {
        const auto flow_index = mnemonic_state->ct->getFlowthruIndex();
        if (flow_index >= mnemonic_state->ct->getNumOperands())
            return;
        walker.pushOperand(flow_index);
        mnemonic_state = mnemonic_state->resolve[flow_index];
        if (mnemonic_state == nullptr || mnemonic_state->ct == nullptr)
            return;
    }
    const auto textual_operands = operands;
    const bool x86 = is_x86_translator(translator);
    operands.clear();
    std::vector<int> visible_indices;
    for (const auto& piece : mnemonic_state->ct->getPrintPieces()) {
        if (piece.size() > 1 && piece[0] == '\n') {
            const auto index = static_cast<int>(piece[1] - 'A');
            if (index >= 0 && index < mnemonic_state->ct->getNumOperands())
                visible_indices.push_back(index);
        }
    }
    for (const int constructor_index : visible_indices) {
        auto* symbol = mnemonic_state->ct->getOperand(constructor_index);
        std::vector<ghidra::FixedHandle> handles;
        collect_state_handles(mnemonic_state, symbol, handles);
        if (handles.empty() && x86)
            collect_operand_handles(walker, symbol, handles);
        if (handles.empty()) {
            continue;
        }
        Operand operand;
        const auto visible_index = operands.size();
        if (visible_index < textual_operands.size())
            operand = textual_operands[visible_index];
        operand.value_mask.assign(length, 0);
        combine_operand_mask(walker, mnemonic_state, symbol, operand.value_mask);
        if (std::all_of(operand.value_mask.begin(), operand.value_mask.end(),
                        [](const auto value) { return value == 0; })) {
            if (auto* triple = symbol->getDefiningSymbol();
                triple && triple->getType() == ghidra::SleighSymbol::subtable_symbol) {
                if (auto* child = mnemonic_state->resolve[symbol->getIndex()];
                    child != nullptr && child->ct != nullptr && child->ct->getParent() != nullptr) {
                    std::vector<ghidra::uint1> fallback_mask;
                    std::vector<ghidra::uint1> fallback_value;
                    child->ct->getParent()->getConstructorMask(child->ct, fallback_mask, fallback_value, child->offset);
                    if (fallback_mask.size() == operand.value_mask.size()) {
                        operand.value_mask = std::move(fallback_mask);
                    }
                }
            }
        }
        for (std::size_t index = 0; index < length; ++index) {
            instruction_mask[index] &= static_cast<std::uint8_t>(~operand.value_mask[index]);
        }
        const auto* parent_state = mnemonic_state->resolve[symbol->getIndex()];
        if (parent_state == nullptr)
            continue;
        const auto parent_handle = parent_state->hand;
        const bool whole_scalar =
            parent_handle.space != nullptr && parent_handle.space->getType() == ghidra::IPTR_CONSTANT;
        const bool address_scalar =
            parent_handle.space != nullptr && parent_handle.offset_space == nullptr &&
            parent_handle.space->getType() != ghidra::IPTR_CONSTANT &&
            translator.getRegisterName(parent_handle.space, parent_handle.offset_offset, parent_handle.size).empty();
        for (const auto& handle : handles) {
            if (x86)
                append_hash_handle(translator, parser->getAddr(), handle, whole_scalar, address_scalar,
                                   operand.hash_objects);
            else
                append_generic_hash_handle(translator, handle, whole_scalar, address_scalar, operand.hash_objects);
        }
        operands.push_back(std::move(operand));
    }
}

/// Converts legacy Ghidra decoder errors into the public error value.
[[nodiscard]] DecodeError make_decode_error(const ghidra::LowlevelError& error) {
    return DecodeError{error.explain};
}

/// Maps the stable numeric p-code encoding to its canonical diagnostic name.
constexpr std::array<std::string_view, 75> opcode_names{"",
                                                        "COPY",
                                                        "LOAD",
                                                        "STORE",
                                                        "BRANCH",
                                                        "CBRANCH",
                                                        "BRANCHIND",
                                                        "CALL",
                                                        "CALLIND",
                                                        "CALLOTHER",
                                                        "RETURN",
                                                        "INT_EQUAL",
                                                        "INT_NOTEQUAL",
                                                        "INT_SLESS",
                                                        "INT_SLESSEQUAL",
                                                        "INT_LESS",
                                                        "INT_LESSEQUAL",
                                                        "INT_ZEXT",
                                                        "INT_SEXT",
                                                        "INT_ADD",
                                                        "INT_SUB",
                                                        "INT_CARRY",
                                                        "INT_SCARRY",
                                                        "INT_SBORROW",
                                                        "INT_2COMP",
                                                        "INT_NEGATE",
                                                        "INT_XOR",
                                                        "INT_AND",
                                                        "INT_OR",
                                                        "INT_LEFT",
                                                        "INT_RIGHT",
                                                        "INT_SRIGHT",
                                                        "INT_MULT",
                                                        "INT_DIV",
                                                        "INT_SDIV",
                                                        "INT_REM",
                                                        "INT_SREM",
                                                        "BOOL_NEGATE",
                                                        "BOOL_XOR",
                                                        "BOOL_AND",
                                                        "BOOL_OR",
                                                        "FLOAT_EQUAL",
                                                        "FLOAT_NOTEQUAL",
                                                        "FLOAT_LESS",
                                                        "FLOAT_LESSEQUAL",
                                                        "",
                                                        "FLOAT_NAN",
                                                        "FLOAT_ADD",
                                                        "FLOAT_DIV",
                                                        "FLOAT_MULT",
                                                        "FLOAT_SUB",
                                                        "FLOAT_NEG",
                                                        "FLOAT_ABS",
                                                        "FLOAT_SQRT",
                                                        "INT2FLOAT",
                                                        "FLOAT2FLOAT",
                                                        "FLOAT_TRUNC",
                                                        "FLOAT_CEIL",
                                                        "FLOAT_FLOOR",
                                                        "FLOAT_ROUND",
                                                        "MULTIEQUAL",
                                                        "INDIRECT",
                                                        "PIECE",
                                                        "SUBPIECE",
                                                        "CAST",
                                                        "PTRADD",
                                                        "PTRSUB",
                                                        "SEGMENTOP",
                                                        "CPOOLREF",
                                                        "NEW",
                                                        "INSERT",
                                                        "EXTRACT",
                                                        "POPCOUNT",
                                                        "LZCOUNT",
                                                        "SPULL"};

} // namespace

/// Returns the module-owned directory containing checked-in compiled SLA specifications.
std::filesystem::path default_specification_directory() {
    return std::filesystem::path(SLEIGH_RUNTIME_SPECIFICATION_DIR);
}

/// Resolves bare SLA filenames against the module specification directory.
std::filesystem::path resolve_sla_path(std::filesystem::path sla_path) {
    if (sla_path.empty() || !sla_path.parent_path().empty()) {
        return sla_path;
    }
    return default_specification_directory() / std::move(sla_path);
}

/// Holds the legacy runtime objects and their ownership order.
class Decoder::Implementation {
public:
    /// Constructs an implementation and loads the compiled specification once.
    explicit Implementation(std::filesystem::path path)
        : sla_path_(resolve_sla_path(std::move(path))), image_(), context_(std::make_unique<ghidra::ContextInternal>()),
          translator_(std::make_unique<ghidra::Sleigh>(&image_, context_.get())) {
        if (!std::filesystem::is_regular_file(sla_path_)) {
            throw std::runtime_error("SLA file does not exist: " + sla_path_.string());
        }
        try {
            initialize_translator();
        } catch (const ghidra::LowlevelError& error) {
            throw std::runtime_error(error.explain);
        } catch (const ghidra::DecoderError& error) {
            throw std::runtime_error(error.explain);
        }
    }

    /// Decodes one bounded byte window using the compiled runtime.
    std::expected<Instruction, DecodeError> decode(std::uint64_t address, std::span<const std::uint8_t> bytes,
                                                   const ProcessorContext& processor_context) {
        if (bytes.empty()) {
            return std::unexpected(DecodeError{"Cannot decode an empty byte sequence"});
        }
        if (bytes.size() > 16) {
            return std::unexpected(DecodeError{"Sleigh instruction input cannot exceed 16 bytes"});
        }

        try {
            image_.set_bytes(address, bytes);
            context_ = std::make_unique<ghidra::ContextInternal>();
            translator_->reset(&image_, context_.get());
            initialize_translator();
            for (const ContextValue& value : processor_context.values) {
                context_->setVariableDefault(value.name, value.value);
            }

            const ghidra::Address instruction_address(translator_->getDefaultCodeSpace(), address);
            AssemblyCapture assembly;
            const int length = translator_->printAssembly(assembly, instruction_address);
            if (length <= 0 || static_cast<std::size_t>(length) > bytes.size()) {
                return std::unexpected(DecodeError{"Instruction bytes end before the decoded instruction"});
            }

            PcodeCapture pcode;
            translator_->oneInstruction(pcode, instruction_address);

            Instruction result;
            result.address = address;
            result.length = static_cast<std::size_t>(length);
            result.bytes.assign(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(length));
            result.is_x86 = is_x86_translator(*translator_);
            result.mnemonic = assembly.mnemonic();
            result.assembly = assembly.body();
            for (std::string operand : split_operands(assembly.body())) {
                result.operands.push_back(classify_operand(std::move(operand)));
            }
            auto* parser = translator_->getParserContextForInstruction(instruction_address);
            materialize_hash_metadata(*translator_, parser, result.operands, result.instruction_mask);
            result.pcode = std::move(pcode).take_operations();
            result.flow = find_flow(result.pcode);
            return result;
        } catch (const ghidra::LowlevelError& error) {
            return std::unexpected(make_decode_error(error));
        } catch (const ghidra::DecoderError& error) {
            return std::unexpected(DecodeError{error.explain});
        } catch (const std::exception& error) {
            return std::unexpected(DecodeError{error.what()});
        }
    }

private:
    /// Initializes or refreshes the legacy parser cache from the binary SLA file.
    void initialize_translator() {
        // Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc
        // Sleigh::initialize() owns FormatDecode and calls SleighBase::decode().
        translator_->initialize(sla_path_.string());
    }

    std::filesystem::path sla_path_;
    ByteLoadImage image_;
    std::unique_ptr<ghidra::ContextInternal> context_;
    std::unique_ptr<ghidra::Sleigh> translator_;
};

/// Loads a compiled binary SLA specification.
Decoder::Decoder(std::filesystem::path sla_path)
    : implementation_(std::make_unique<Implementation>(std::move(sla_path))) {}

/// Releases the owning runtime implementation.
Decoder::~Decoder() = default;

/// Transfers ownership of a decoder implementation.
Decoder::Decoder(Decoder&&) noexcept = default;

/// Transfers ownership of a decoder implementation by assignment.
Decoder& Decoder::operator=(Decoder&&) noexcept = default;

/// Decodes bytes into assembly, operands, flow, and concrete p-code.
std::expected<Instruction, DecodeError> Decoder::decode(std::uint64_t address, std::span<const std::uint8_t> bytes,
                                                        const ProcessorContext& context) {
    if (!implementation_) {
        return std::unexpected(DecodeError{"Cannot decode with a moved-from Decoder"});
    }
    return implementation_->decode(address, bytes, context);
}

/// Converts a public opcode into the same names used by Ghidra's p-code text.
std::string_view opcode_name(PcodeOpcode opcode) {
    const auto index = std::to_underlying(opcode);
    if (index >= opcode_names.size() || opcode_names[index].empty()) {
        return "UNKNOWN";
    }
    return opcode_names[index];
}

} // namespace sleigh_runtime
