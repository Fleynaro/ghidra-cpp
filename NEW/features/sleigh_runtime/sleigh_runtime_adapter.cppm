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
            auto *memory_space = reinterpret_cast<ghidra::AddrSpace *>(inputs[0].offset);
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

/// Stores a bounded instruction byte window for the Ghidra LoadImage interface.
// Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/loadimage.hh
// LoadImage::loadFill().
class ByteLoadImage final : public ghidra::LoadImage {
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
    for (const PcodeOp& operation : operations) {
        FlowKind kind = FlowKind::none;
        switch (operation.opcode) {
            case PcodeOpcode::branch:
                kind = FlowKind::branch;
                break;
            case PcodeOpcode::cbranch:
                kind = FlowKind::conditional_branch;
                break;
            case PcodeOpcode::branch_ind:
                kind = FlowKind::indirect_branch;
                break;
            case PcodeOpcode::call:
                kind = FlowKind::call;
                break;
            case PcodeOpcode::call_ind:
                kind = FlowKind::indirect_call;
                break;
            case PcodeOpcode::return_op:
                kind = FlowKind::return_op;
                break;
            default:
                break;
        }
        if (kind != FlowKind::none) {
            FlowInfo flow{kind, std::nullopt};
            if (!operation.inputs.empty()) {
                flow.target = operation.inputs.front();
            }
            return flow;
        }
    }
    return {};
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

/// Holds the legacy runtime objects and their ownership order.
class Decoder::Implementation {
public:
    /// Constructs an implementation and loads the compiled specification once.
    explicit Implementation(std::filesystem::path path)
        : sla_path_(std::move(path)), image_(), context_(std::make_unique<ghidra::ContextInternal>()),
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
            result.mnemonic = assembly.mnemonic();
            result.assembly = assembly.body();
            for (std::string operand : split_operands(assembly.body())) {
                result.operands.push_back(classify_operand(std::move(operand)));
            }
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
