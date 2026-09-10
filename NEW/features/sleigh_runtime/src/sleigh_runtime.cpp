module;

#include "globalcontext.hh"
#include "loadimage.hh"
#include "sleigh.hh"
#include "xml.hh"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

module sleigh_runtime;

namespace sleigh_runtime {
namespace {

/// Converts a legacy Ghidra varnode into the owning public representation.
Varnode materialize_varnode(const ghidra::VarnodeData& varnode) {
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
    const std::string& mnemonic() const {
        return mnemonic_;
    }

    /// Returns the captured printable operand body.
    const std::string& body() const {
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
        operations_.push_back(std::move(op));
    }

    /// Returns the owned p-code sequence for transfer into the public result.
    std::vector<PcodeOp>& operations() {
        return operations_;
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
    ByteLoadImage() : ghidra::LoadImage("sleigh-runtime-memory") {}

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
    ghidra::string getArchType() const override {
        return "sleigh-runtime";
    }

    /// Raw windows are already addressed by the caller, so no VMA adjustment is needed.
    void adjustVma(long) override {}

private:
    std::array<std::uint8_t, 16> window_{};
    std::uint64_t address_ = 0;
};

/// Escapes a filesystem path before placing it in the tiny XML document expected by Sleigh.
std::string escape_xml(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value) {
        switch (character) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&apos;";
                break;
            default:
                escaped += character;
                break;
        }
    }
    return escaped;
}

/// Removes surrounding whitespace from a printable operand.
std::string trim(std::string value) {
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
std::vector<std::string> split_operands(std::string_view body) {
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
std::optional<std::uint64_t> parse_literal(std::string_view text) {
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
Operand classify_operand(std::string text) {
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
FlowInfo find_flow(const std::vector<PcodeOp>& operations) {
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
DecodeError make_decode_error(const ghidra::LowlevelError& error) {
    return DecodeError{error.explain};
}

} // namespace

/// Holds the legacy runtime objects and their ownership order.
class Decoder::Implementation {
public:
    /// Constructs an implementation and loads the compiled specification once.
    explicit Implementation(std::filesystem::path path)
        : sla_path_(std::move(path)), image_(), context_(std::make_unique<ghidra::ContextInternal>()), translator_() {
        if (!std::filesystem::is_regular_file(sla_path_)) {
            throw std::runtime_error("SLA file does not exist: " + sla_path_.string());
        }
        translator_ = std::make_unique<ghidra::Sleigh>(&image_, context_.get());
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

        image_.set_bytes(address, bytes);
        context_ = std::make_unique<ghidra::ContextInternal>();
        translator_->reset(&image_, context_.get());
        try {
            initialize_translator();
            for (const auto& [name, value] : processor_context.values) {
                context_->setVariableDefault(name, value);
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
            result.pcode = std::move(pcode.operations());
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
        ghidra::DocumentStorage storage;
        make_document_storage(storage);
        // Ghidra reference: Ghidra/Features/Decompiler/src/decompile/cpp/sleigh.cc
        // Sleigh::initialize() owns FormatDecode and calls SleighBase::decode().
        translator_->initialize(storage);
    }

    /// Creates the document-store shim accepted by the original Sleigh API.
    void make_document_storage(ghidra::DocumentStorage& storage) const {
        const std::string xml = "<sleigh>" + escape_xml(sla_path_.string()) + "</sleigh>";
        std::istringstream stream(xml);
        ghidra::Document* document = storage.parseDocument(stream);
        if (document == nullptr || document->getRoot() == nullptr) {
            throw std::runtime_error("Unable to construct the SLA document-store shim");
        }
        storage.registerTag(document->getRoot());
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
    return implementation_->decode(address, bytes, context);
}

/// Converts a public opcode into the same names used by Ghidra's p-code text.
std::string_view opcode_name(PcodeOpcode opcode) {
    switch (opcode) {
        case PcodeOpcode::copy:
            return "COPY";
        case PcodeOpcode::load:
            return "LOAD";
        case PcodeOpcode::store:
            return "STORE";
        case PcodeOpcode::branch:
            return "BRANCH";
        case PcodeOpcode::cbranch:
            return "CBRANCH";
        case PcodeOpcode::branch_ind:
            return "BRANCHIND";
        case PcodeOpcode::call:
            return "CALL";
        case PcodeOpcode::call_ind:
            return "CALLIND";
        case PcodeOpcode::call_other:
            return "CALLOTHER";
        case PcodeOpcode::return_op:
            return "RETURN";
        case PcodeOpcode::int_equal:
            return "INT_EQUAL";
        case PcodeOpcode::int_not_equal:
            return "INT_NOTEQUAL";
        case PcodeOpcode::int_sless:
            return "INT_SLESS";
        case PcodeOpcode::int_sless_equal:
            return "INT_SLESSEQUAL";
        case PcodeOpcode::int_less:
            return "INT_LESS";
        case PcodeOpcode::int_less_equal:
            return "INT_LESSEQUAL";
        case PcodeOpcode::int_zext:
            return "INT_ZEXT";
        case PcodeOpcode::int_sext:
            return "INT_SEXT";
        case PcodeOpcode::int_add:
            return "INT_ADD";
        case PcodeOpcode::int_sub:
            return "INT_SUB";
        case PcodeOpcode::int_carry:
            return "INT_CARRY";
        case PcodeOpcode::int_scarry:
            return "INT_SCARRY";
        case PcodeOpcode::int_sborrow:
            return "INT_SBORROW";
        case PcodeOpcode::int_two_comp:
            return "INT_2COMP";
        case PcodeOpcode::int_negate:
            return "INT_NEGATE";
        case PcodeOpcode::int_xor:
            return "INT_XOR";
        case PcodeOpcode::int_and:
            return "INT_AND";
        case PcodeOpcode::int_or:
            return "INT_OR";
        case PcodeOpcode::int_left:
            return "INT_LEFT";
        case PcodeOpcode::int_right:
            return "INT_RIGHT";
        case PcodeOpcode::int_sright:
            return "INT_SRIGHT";
        case PcodeOpcode::int_mult:
            return "INT_MULT";
        case PcodeOpcode::int_div:
            return "INT_DIV";
        case PcodeOpcode::int_sdiv:
            return "INT_SDIV";
        case PcodeOpcode::int_rem:
            return "INT_REM";
        case PcodeOpcode::int_srem:
            return "INT_SREM";
        case PcodeOpcode::bool_negate:
            return "BOOL_NEGATE";
        case PcodeOpcode::bool_xor:
            return "BOOL_XOR";
        case PcodeOpcode::bool_and:
            return "BOOL_AND";
        case PcodeOpcode::bool_or:
            return "BOOL_OR";
        case PcodeOpcode::float_equal:
            return "FLOAT_EQUAL";
        case PcodeOpcode::float_not_equal:
            return "FLOAT_NOTEQUAL";
        case PcodeOpcode::float_less:
            return "FLOAT_LESS";
        case PcodeOpcode::float_less_equal:
            return "FLOAT_LESSEQUAL";
        case PcodeOpcode::float_nan:
            return "FLOAT_NAN";
        case PcodeOpcode::float_add:
            return "FLOAT_ADD";
        case PcodeOpcode::float_div:
            return "FLOAT_DIV";
        case PcodeOpcode::float_mult:
            return "FLOAT_MULT";
        case PcodeOpcode::float_sub:
            return "FLOAT_SUB";
        case PcodeOpcode::float_neg:
            return "FLOAT_NEG";
        case PcodeOpcode::float_abs:
            return "FLOAT_ABS";
        case PcodeOpcode::float_sqrt:
            return "FLOAT_SQRT";
        case PcodeOpcode::float_int_to_float:
            return "INT2FLOAT";
        case PcodeOpcode::float_float_to_float:
            return "FLOAT2FLOAT";
        case PcodeOpcode::float_trunc:
            return "FLOAT_TRUNC";
        case PcodeOpcode::float_ceil:
            return "FLOAT_CEIL";
        case PcodeOpcode::float_floor:
            return "FLOAT_FLOOR";
        case PcodeOpcode::float_round:
            return "FLOAT_ROUND";
        case PcodeOpcode::multiequal:
            return "MULTIEQUAL";
        case PcodeOpcode::indirect:
            return "INDIRECT";
        case PcodeOpcode::piece:
            return "PIECE";
        case PcodeOpcode::subpiece:
            return "SUBPIECE";
        case PcodeOpcode::cast:
            return "CAST";
        case PcodeOpcode::ptradd:
            return "PTRADD";
        case PcodeOpcode::ptrsub:
            return "PTRSUB";
        case PcodeOpcode::segment_op:
            return "SEGMENTOP";
        case PcodeOpcode::cpool_ref:
            return "CPOOLREF";
        case PcodeOpcode::new_op:
            return "NEW";
        case PcodeOpcode::insert:
            return "INSERT";
        case PcodeOpcode::popcount:
            return "POPCOUNT";
        case PcodeOpcode::lzcount:
            return "LZCOUNT";
        case PcodeOpcode::spull:
            return "SPULL";
    }
    return "UNKNOWN";
}

} // namespace sleigh_runtime
