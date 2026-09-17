module;

#include <pugixml.hpp>

module decompiler;
import std;
import recode.decompiler;
import recode.decompiler.signature;
import sleigh_runtime;

// Provider frontend provenance: this adapter invokes the ported Ghidra engine
// under this project's explicit provider boundary; analysis algorithms remain in the
// source files mapped by services/decompiler/CMakeLists.txt.

namespace recode::decompiler {
namespace detail {

/// Returns the stable numeric encoding of a canonical p-code opcode.
[[nodiscard]] constexpr std::uint32_t opcode_value(recode::core::PcodeOpcode opcode) noexcept {
    return std::to_underlying(opcode);
}

/// Checks one provider storage record before it is converted to native
/// VarnodeData. Non-empty spaces and non-zero, non-wrapping ranges are
/// required because the native engine cannot represent malformed varnodes.
static void validate_storage(const Storage& storage, std::string_view context) {
    if (storage.space.empty() || storage.size == 0) {
        throw ghidra::BadDataError(std::string(context) + " has an empty space or zero size");
    }
    if (storage.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
        throw ghidra::BadDataError(std::string(context) + " exceeds the native varnode size limit");
    }
    // Constant-space offsets are values, not byte addresses, and may use the
    // full uint64 range without making the represented varnode wrap.
    if (storage.space.name() != "const" &&
        storage.offset > std::numeric_limits<std::uint64_t>::max() - (storage.size - 1U)) {
        throw ghidra::BadDataError(std::string(context) + " address range overflows");
    }
}

/// Validates the provider input count against the native p-code operation
/// contract before `validate_instruction` accepts the record.
static void validate_provider_pcode_arity(const PcodeOperation& operation);

/// Validates one complete provider instruction at the frontend boundary.
/// Opcode values are checked before the native enum cast, while every storage
/// range and the instruction end address are checked for representability.
/// This is the provider-side equivalent of the bounds assumptions in
/// `Ghidra/Features/Decompiler/src/decompile/cpp/translate.cc`.
static void validate_instruction(const Instruction& instruction, std::uint64_t requested_address) {
    if (instruction.address != requested_address) {
        throw ghidra::BadDataError("Provider returned an instruction at a different address");
    }
    if (instruction.length == 0 ||
        instruction.length > static_cast<std::size_t>(std::numeric_limits<ghidra::int4>::max()) ||
        requested_address > std::numeric_limits<std::uint64_t>::max() - instruction.length) {
        throw ghidra::BadDataError("Provider returned an invalid instruction length or overflowing address");
    }
    for (const PcodeOperation& operation : instruction.pcode) {
        if (opcode_value(operation.opcode) == 0 ||
            opcode_value(operation.opcode) >= static_cast<std::uint32_t>(ghidra::CPUI_MAX)) {
            throw ghidra::BadDataError("Provider returned an invalid p-code opcode");
        }
        validate_provider_pcode_arity(operation);
        if (operation.output) {
            validate_storage(*operation.output, "P-code output");
        }
        if (operation.inputs.size() > static_cast<std::size_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw ghidra::BadDataError("P-code input count exceeds the native limit");
        }
        for (const Storage& input : operation.inputs) {
            validate_storage(input, "P-code input");
        }
    }
}

/// Returns whether a legacy LOAD/STORE constant selector names the supplied
/// memory space. `memory_space` is the canonical frontend field, but the old
/// selector is accepted without dropping a real constant address.
static bool has_matching_memory_selector(const PcodeOperation& operation, ghidra::AddrSpace* memory_space,
                                         const ghidra::Translate* translator) {
    if (operation.inputs.empty() || operation.inputs.front().space.name() != "const") {
        return false;
    }
    const ghidra::Address encoded_space = translator->createConstFromSpace(memory_space);
    return operation.inputs.front().offset == encoded_space.getOffset();
}

/// Checks a provider storage range against the actual native address space
/// limit after its name has been resolved.
static void validate_space_range(const Storage& storage, ghidra::AddrSpace* space, std::string_view context) {
    if (space->getType() == ghidra::IPTR_CONSTANT) {
        return;
    }
    if (storage.offset > space->getHighest() || storage.size - 1U > space->getHighest() - storage.offset) {
        throw ghidra::BadDataError(std::string(context) + " exceeds its address space");
    }
}

/// Returns one provider storage sequence while rejecting ambiguous single and
/// split representations. Ordered sequences use the native decompiler's
/// most-significant-to-least-significant convention from `translate.cc`.
static std::vector<Storage> provider_storage_sequence(const std::optional<Storage>& single,
                                                      const std::vector<Storage>& ordered, std::string_view context) {
    if (single && !ordered.empty()) {
        throw ghidra::BadDataError(std::string(context) + " specifies both one storage location and ordered pieces");
    }
    if (!ordered.empty()) {
        return ordered;
    }
    if (single) {
        return {*single};
    }
    return {};
}

/// Validates provider input counts before the native `PcodeEmitFd::dump`
/// implementation can index its `vars` array. The fixed arities mirror the
/// operation contracts in `typeop.cc`; CALL-like and metadata operations retain
/// their original variable-tail forms. LOAD/STORE use one fewer public operand
/// when `memory_space` carries the address-space selector, while a matching
/// leading constant remains accepted for compatibility with native p-code.
/// The frontend rejects malformed records before native emission, preserving
/// the provider boundary's diagnostic rather than exposing a native crash.
/// Original indexing boundary: `Ghidra/Features/Decompiler/src/decompile/cpp/funcdata.cc`,
/// `PcodeEmitFd::dump`, and `translate.cc`, `PcodeEmit::dump` callers.
static void validate_provider_pcode_arity(const PcodeOperation& operation) {
    const auto opname = [&] {
        return std::string(ghidra::get_opname(static_cast<ghidra::OpCode>(opcode_value(operation.opcode))));
    };
    const auto fail_exact = [&](std::size_t expected) {
        if (operation.inputs.size() != expected) {
            throw ghidra::BadDataError("Provider p-code " + opname() + " has " +
                                       std::to_string(operation.inputs.size()) + " inputs; expected " +
                                       std::to_string(expected));
        }
    };
    const auto fail_minimum = [&](std::size_t expected) {
        if (operation.inputs.size() < expected) {
            throw ghidra::BadDataError("Provider p-code " + opname() + " has " +
                                       std::to_string(operation.inputs.size()) + " inputs; expected at least " +
                                       std::to_string(expected));
        }
    };
    const auto fail_memory = [&](std::size_t canonical, std::size_t native) {
        if (!operation.memory_space) {
            fail_exact(native);
            return;
        }
        if (operation.inputs.size() == canonical) {
            return;
        }
        if (operation.inputs.size() == native && !operation.inputs.front().space.empty() &&
            operation.inputs.front().space.name() == "const") {
            return;
        }
        throw ghidra::BadDataError("Provider p-code " + opname() + " has " + std::to_string(operation.inputs.size()) +
                                   " inputs; expected " + std::to_string(canonical) + " canonical operands or " +
                                   std::to_string(native) + " operands with a leading constant-space selector");
    };

    switch (static_cast<ghidra::OpCode>(opcode_value(operation.opcode))) {
        case ghidra::CPUI_COPY:
            fail_exact(1);
            break;
        case ghidra::CPUI_LOAD:
            fail_memory(1, 2);
            break;
        case ghidra::CPUI_STORE:
            fail_memory(2, 3);
            break;
        case ghidra::CPUI_BRANCH:
        case ghidra::CPUI_BRANCHIND:
            fail_exact(1);
            break;
        case ghidra::CPUI_CBRANCH:
            fail_exact(2);
            break;
        case ghidra::CPUI_CALL:
        case ghidra::CPUI_CALLIND:
        case ghidra::CPUI_CALLOTHER:
        case ghidra::CPUI_RETURN:
        case ghidra::CPUI_MULTIEQUAL:
        case ghidra::CPUI_CPOOLREF:
        case ghidra::CPUI_NEW:
            fail_minimum(1);
            break;
        case ghidra::CPUI_INDIRECT:
        case ghidra::CPUI_PIECE:
        case ghidra::CPUI_SUBPIECE:
        case ghidra::CPUI_PTRSUB:
            fail_exact(2);
            break;
        case ghidra::CPUI_PTRADD:
        case ghidra::CPUI_SEGMENTOP:
        case ghidra::CPUI_INSERT:
        case ghidra::CPUI_ZPULL:
        case ghidra::CPUI_SPULL:
            fail_exact(3);
            break;
        case ghidra::CPUI_INT_EQUAL:
        case ghidra::CPUI_INT_NOTEQUAL:
        case ghidra::CPUI_INT_SLESS:
        case ghidra::CPUI_INT_SLESSEQUAL:
        case ghidra::CPUI_INT_LESS:
        case ghidra::CPUI_INT_LESSEQUAL:
        case ghidra::CPUI_INT_ADD:
        case ghidra::CPUI_INT_SUB:
        case ghidra::CPUI_INT_CARRY:
        case ghidra::CPUI_INT_SCARRY:
        case ghidra::CPUI_INT_SBORROW:
        case ghidra::CPUI_INT_XOR:
        case ghidra::CPUI_INT_AND:
        case ghidra::CPUI_INT_OR:
        case ghidra::CPUI_INT_LEFT:
        case ghidra::CPUI_INT_RIGHT:
        case ghidra::CPUI_INT_SRIGHT:
        case ghidra::CPUI_INT_MULT:
        case ghidra::CPUI_INT_DIV:
        case ghidra::CPUI_INT_SDIV:
        case ghidra::CPUI_INT_REM:
        case ghidra::CPUI_INT_SREM:
        case ghidra::CPUI_BOOL_XOR:
        case ghidra::CPUI_BOOL_AND:
        case ghidra::CPUI_BOOL_OR:
        case ghidra::CPUI_FLOAT_EQUAL:
        case ghidra::CPUI_FLOAT_NOTEQUAL:
        case ghidra::CPUI_FLOAT_LESS:
        case ghidra::CPUI_FLOAT_LESSEQUAL:
        case ghidra::CPUI_FLOAT_ADD:
        case ghidra::CPUI_FLOAT_DIV:
        case ghidra::CPUI_FLOAT_MULT:
        case ghidra::CPUI_FLOAT_SUB:
            fail_exact(2);
            break;
        case ghidra::CPUI_INT_ZEXT:
        case ghidra::CPUI_INT_SEXT:
        case ghidra::CPUI_INT_2COMP:
        case ghidra::CPUI_INT_NEGATE:
        case ghidra::CPUI_BOOL_NEGATE:
        case ghidra::CPUI_FLOAT_NAN:
        case ghidra::CPUI_FLOAT_NEG:
        case ghidra::CPUI_FLOAT_ABS:
        case ghidra::CPUI_FLOAT_SQRT:
        case ghidra::CPUI_FLOAT_INT2FLOAT:
        case ghidra::CPUI_FLOAT_FLOAT2FLOAT:
        case ghidra::CPUI_FLOAT_TRUNC:
        case ghidra::CPUI_FLOAT_CEIL:
        case ghidra::CPUI_FLOAT_FLOOR:
        case ghidra::CPUI_FLOAT_ROUND:
        case ghidra::CPUI_CAST:
        case ghidra::CPUI_POPCOUNT:
        case ghidra::CPUI_LZCOUNT:
            fail_exact(1);
            break;
        default:
            throw ghidra::BadDataError("Provider p-code opcode has no native arity contract: " +
                                       std::to_string(opcode_value(operation.opcode)));
    }
}

/// Copies the live native Funcdata provenance graph into stable value records.
///
/// Original sources: `Ghidra/Features/Decompiler/src/decompile/cpp/funcdata.cc`
/// (`Funcdata::beginOpAll`, `Funcdata::beginLoc`) and
/// `Ghidra/Features/Decompiler/src/decompile/cpp/prettyprint.cc`
/// (`EmitMarkup::tagVariable`, `EmitMarkup::tagOp`).  The sequence time is
/// deliberately retained because it is the cross-representation key used by
/// Clang markup `opref` attributes.
static void capture_native_provenance(const ghidra::Funcdata& data, DecompilationResult& result) {
    result.pcode_provenance.clear();
    result.varnode_provenance.clear();

    for (auto iter = data.beginLoc(); iter != data.endLoc(); ++iter) {
        const ghidra::Varnode* varnode = *iter;
        VarnodeProvenance snapshot;
        snapshot.create_index = varnode->getCreateIndex();
        snapshot.space = varnode->getSpace() == nullptr ? std::string{} : varnode->getSpace()->getName();
        snapshot.offset = varnode->getOffset();
        snapshot.size = static_cast<std::uint32_t>(varnode->getSize());
        if (const ghidra::PcodeOp* definition = varnode->getDef(); definition != nullptr) {
            snapshot.defining_op = definition->getTime();
        }
        if (!varnode->isFree()) {
            // Some transient SSA Varnodes intentionally have no HighVariable;
            // the native accessor reports that state with LowlevelError rather
            // than a nullable result, so provenance capture must not alter
            // successful decompilation of those graphs.
            try {
                ghidra::HighVariable* high = varnode->getHigh();
                if (high != nullptr) {
                    if (ghidra::Symbol* symbol = high->getSymbol(); symbol != nullptr) {
                        snapshot.high_variable_name = symbol->getName();
                    }
                }
            } catch (const ghidra::LowlevelError&) {
                snapshot.high_variable_name.clear();
            }
        }
        result.varnode_provenance.push_back(std::move(snapshot));
    }

    for (auto iter = data.beginOpAll(); iter != data.endOpAll(); ++iter) {
        const ghidra::PcodeOp* operation = iter->second;
        PcodeOpProvenance snapshot;
        snapshot.sequence = operation->getTime();
        snapshot.opcode = operation->getOpName();
        snapshot.opcode_value = static_cast<std::uint32_t>(operation->code());
        snapshot.address_space =
            operation->getAddr().getSpace() == nullptr ? std::string{} : operation->getAddr().getSpace()->getName();
        snapshot.address = operation->getAddr().getOffset();
        if (const ghidra::Varnode* output = operation->getOut(); output != nullptr) {
            snapshot.output_varnode = output->getCreateIndex();
        }
        for (ghidra::int4 slot = 0; slot < operation->numInput(); ++slot) {
            snapshot.input_varnodes.push_back(operation->getIn(slot)->getCreateIndex());
        }
        result.pcode_provenance.push_back(std::move(snapshot));
    }
}

/// Reads one signed markup integer while accepting the decimal and hexadecimal
/// forms emitted by the native XML encoder.
static std::optional<std::int64_t> read_markup_signed(const pugi::xml_node& node, const char* name) {
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        const std::string value = attribute.as_string();
        const std::int64_t parsed = std::stoll(value, &consumed, 0);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return parsed;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

/// Reads one unsigned markup reference such as `opref` or `varref`.
static std::optional<std::uint64_t> read_markup_unsigned(const pugi::xml_node& node, const char* name) {
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        const std::string value = attribute.as_string();
        const std::uint64_t parsed = std::stoull(value, &consumed, 0);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return parsed;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

/// Appends one identity to a small deterministic provenance set.
template <typename Value> static void append_unique(std::vector<Value>& values, const Value& value) {
    if (!std::ranges::contains(values, value)) {
        values.push_back(value);
    }
}

/// Identifies markup elements that a viewer can present as clickable tokens.
static bool is_selectable_markup_element(std::string_view element) {
    return element == "variable" || element == "field" || element == "bitfield" || element == "op" ||
           element == "funcname" || element == "value" || element == "syntax" || element == "type" ||
           element == "label" || element == "comment" || element == "return_type";
}

/// Recursively snapshots the native markup tree and returns all operation
/// references below the current element. The returned set lets the caller
/// associate every token in a statement with all of that statement's PcodeOps.
static std::vector<std::uint64_t>
collect_markup_tree(const pugi::xml_node& xml_node, std::optional<std::uint64_t> parent_id,
                    std::optional<std::uint64_t> inherited_statement_id, std::vector<ClangMarkupNodeProvenance>& nodes,
                    std::unordered_map<std::uint64_t, std::vector<std::uint64_t>>& statement_operations) {
    const std::uint64_t node_id = nodes.size();
    const bool statement = std::string_view(xml_node.name()) == "statement";
    const std::optional<std::uint64_t> statement_id = statement ? std::optional{node_id} : inherited_statement_id;
    ClangMarkupNodeProvenance node;
    node.id = node_id;
    node.element = xml_node.name();
    node.content = xml_node.attribute("content").as_string();
    if (node.content.empty()) {
        node.content = xml_node.text().as_string();
    }
    node.selectable = is_selectable_markup_element(node.element);
    node.parent_id = parent_id;
    node.statement_id = statement_id;
    if (const std::optional<std::uint64_t> varnode = read_markup_unsigned(xml_node, "varref"); varnode) {
        node.varnode_ref = static_cast<std::uint32_t>(*varnode);
    }
    node.field_offset = read_markup_signed(xml_node, "off");
    if (const std::optional<std::uint64_t> operation = read_markup_unsigned(xml_node, "opref"); operation) {
        node.primary_operation_ref = *operation;
        node.operation_refs.push_back(*operation);
    }
    nodes.push_back(std::move(node));

    std::vector<std::uint64_t> subtree_operations = nodes.back().operation_refs;
    for (pugi::xml_node child = xml_node.first_child(); child; child = child.next_sibling()) {
        if (child.type() != pugi::node_element) {
            continue;
        }
        const std::vector<std::uint64_t> child_operations =
            collect_markup_tree(child, node_id, statement_id, nodes, statement_operations);
        for (const std::uint64_t operation : child_operations) {
            append_unique(subtree_operations, operation);
        }
    }
    if (statement) {
        statement_operations.emplace(node_id, subtree_operations);
    }
    return subtree_operations;
}

/// Converts native Clang XML markup into stable node records and complete
/// Pcode/ASM reverse indexes for the viewer-facing result.
static void capture_structured_clang_provenance(DecompilationResult& result) {
    result.clang_nodes.clear();
    result.instruction_provenance.clear();
    for (PcodeOpProvenance& operation : result.pcode_provenance) {
        operation.clang_node_ids.clear();
        const auto decoded_instruction =
            std::ranges::find_if(result.raw_instructions, [&operation](const Instruction& candidate) {
                return candidate.address == operation.address;
            });
        const std::uint32_t instruction_length = decoded_instruction == result.raw_instructions.end()
                                                     ? 0
                                                     : static_cast<std::uint32_t>(decoded_instruction->length);
        auto instruction =
            std::ranges::find_if(result.instruction_provenance, [&operation](const InstructionProvenance& candidate) {
                return candidate.address_space == operation.address_space && candidate.address == operation.address;
            });
        if (instruction == result.instruction_provenance.end()) {
            result.instruction_provenance.push_back(InstructionProvenance{
                operation.address_space, operation.address, instruction_length, {operation.sequence}, {}});
        } else {
            append_unique(instruction->pcode_operations, operation.sequence);
        }
    }
    std::ranges::sort(result.instruction_provenance,
                      [](const InstructionProvenance& left, const InstructionProvenance& right) {
                          if (left.address_space != right.address_space) {
                              return left.address_space < right.address_space;
                          }
                          return left.address < right.address;
                      });
    for (VarnodeProvenance& varnode : result.varnode_provenance) {
        varnode.clang_node_ids.clear();
    }

    pugi::xml_document document;
    const pugi::xml_parse_result parsed = document.load_string(result.clang_markup.c_str());
    if (!parsed) {
        throw ghidra::LowlevelError("Native Clang markup could not be parsed: " + std::string(parsed.description()));
    }
    std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> statement_operations;
    for (pugi::xml_node child = document.first_child(); child; child = child.next_sibling()) {
        if (child.type() == pugi::node_element) {
            collect_markup_tree(child, std::nullopt, std::nullopt, result.clang_nodes, statement_operations);
        }
    }
    for (ClangMarkupNodeProvenance& node : result.clang_nodes) {
        if (node.statement_id.has_value()) {
            node.operation_refs = statement_operations.at(*node.statement_id);
        }
    }

    // The original emitter annotates the case value, while the visible `case`
    // keyword is a syntax token. Link that syntax token to its adjacent value
    // so either part of a case label has deterministic viewer provenance.
    for (std::size_t index = 0; index < result.clang_nodes.size(); ++index) {
        ClangMarkupNodeProvenance& node = result.clang_nodes[index];
        if (node.element != "syntax" || (node.content != "case" && node.content != "default") ||
            !node.operation_refs.empty()) {
            continue;
        }
        for (std::size_t candidate = index + 1; candidate < result.clang_nodes.size(); ++candidate) {
            const ClangMarkupNodeProvenance& value = result.clang_nodes[candidate];
            if (value.parent_id == node.parent_id && value.element == "value" && !value.operation_refs.empty()) {
                node.operation_refs = value.operation_refs;
                break;
            }
        }
    }

    for (ClangMarkupNodeProvenance& node : result.clang_nodes) {
        for (const std::uint64_t sequence : node.operation_refs) {
            const auto operation =
                std::ranges::find_if(result.pcode_provenance, [sequence](const PcodeOpProvenance& candidate) {
                    return candidate.sequence == sequence;
                });
            if (operation == result.pcode_provenance.end()) {
                continue;
            }
            append_unique(operation->clang_node_ids, node.id);
            append_unique(node.originating_addresses, operation->address);
            const auto instruction = std::ranges::find_if(
                result.instruction_provenance, [&operation](const InstructionProvenance& candidate) {
                    return candidate.address_space == operation->address_space &&
                           candidate.address == operation->address;
                });
            if (instruction != result.instruction_provenance.end()) {
                append_unique(instruction->clang_node_ids, node.id);
            }
        }
        if (node.varnode_ref.has_value()) {
            const auto varnode =
                std::ranges::find_if(result.varnode_provenance, [&node](const VarnodeProvenance& candidate) {
                    return candidate.create_index == *node.varnode_ref;
                });
            if (varnode != result.varnode_provenance.end()) {
                append_unique(varnode->clang_node_ids, node.id);
            }
        }
    }
}

/// Normalizes native Sleigh LOAD/STORE selector operands into the shared provider contract.
///
/// The runtime retains the native constant-space selector as provenance. The
/// decompiler frontend materializes that selector itself from `memory_space`,
/// so it removes only the leading selector while retaining real constants.
[[nodiscard]] static PcodeOperation normalize_sleigh_operation(const PcodeOperation& operation) {
    PcodeOperation normalized = operation;
    std::size_t first_input = 0;
    if (operation.memory_space.has_value() &&
        (operation.opcode == PcodeOpcode::load || operation.opcode == PcodeOpcode::store) &&
        !operation.inputs.empty() && operation.inputs.front().space.name() == "const") {
        first_input = 1;
    }
    normalized.inputs.assign(operation.inputs.begin() + static_cast<std::ptrdiff_t>(first_input),
                             operation.inputs.end());
    return normalized;
}

/// Converts a provider storage sequence into a native parameter address. The
/// native `ParameterPieces` algorithm creates a formal join address when the
/// physical locations are not contiguous, preserving the original ABI piece
/// ordering and endian behavior described by `fspec.cc` and `translate.cc`.
static ghidra::ParameterPieces make_provider_storage(ghidra::Architecture* architecture, ghidra::Datatype* type,
                                                     const std::vector<Storage>& storage, ghidra::uint4 flags,
                                                     std::string_view context) {
    ghidra::ParameterPieces result{};
    result.type = type;
    result.flags = flags;
    if (storage.empty()) {
        return result;
    }

    std::vector<ghidra::VarnodeData> native_pieces;
    native_pieces.reserve(storage.size());
    for (const Storage& piece : storage) {
        validate_storage(piece, context);
        ghidra::AddrSpace* space = architecture->getSpaceByName(piece.space.name());
        if (space == nullptr) {
            throw std::runtime_error(std::string(context) +
                                     " references an unknown storage space: " + piece.space.name());
        }
        validate_space_range(piece, space, context);
        native_pieces.push_back(ghidra::VarnodeData{space, piece.offset, static_cast<ghidra::uint4>(piece.size)});
    }
    result.assignAddressFromPieces(native_pieces, true, architecture);
    return result;
}

/// Adapts the provider contract to the native Ghidra Translate interface.
class ProviderTranslate final : public ghidra::Translate {
public:
    /// Builds address spaces and register mappings from provider metadata.
    ProviderTranslate(const ArchitectureDescription& description, std::shared_ptr<PcodeProvider> provider)
        : provider_(std::move(provider)), pointer_size_(description.pointer_size) {
        if (!provider_) {
            throw std::invalid_argument("A p-code provider is required");
        }
        if (pointer_size_ == 0 ||
            pointer_size_ > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Architecture pointer size is outside the native range");
        }
        insertSpace(new ghidra::ConstantSpace(this, this));

        std::vector<SpaceDescription> descriptions = description.spaces;
        if (descriptions.empty()) {
            descriptions.push_back(SpaceDescription{"ram", 8, 1, false, 2, 0, true});
            descriptions.push_back(SpaceDescription{"register", 8, 1, false, 3, 0, true});
        }
        // Insert physical spaces first so provider overlays can resolve their
        // containment relationship without depending on XML decode order.
        for (const SpaceDescription& space : descriptions) {
            if (space.name == "const" || space.name == "unique" || space.name.empty() || !space.overlay_base.empty()) {
                continue;
            }
            const int index = space.index < 1 ? numSpaces() : space.index;
            insertSpace(new ghidra::AddrSpace(
                this, this, ghidra::IPTR_PROCESSOR, space.name, space.big_endian, space.address_size, space.word_size,
                index, space.physical ? ghidra::AddrSpace::hasphysical : 0, space.delay, space.delay));
        }
        for (const SpaceDescription& space : descriptions) {
            if (space.overlay_base.empty()) {
                continue;
            }
            ghidra::AddrSpace* base = getSpaceByName(space.overlay_base);
            if (base == nullptr) {
                throw std::invalid_argument("Provider overlay references an unknown base space: " + space.overlay_base);
            }
            const int index = space.index < 1 ? numSpaces() : space.index;
            insertSpace(new ghidra::OverlaySpace(this, this, space.name, index, base));
        }
        insertSpace(new ghidra::UniqueSpace(this, this, numSpaces(), ghidra::AddrSpace::hasphysical));

        code_space_name_ = description.code_space;
        data_space_name_ = description.data_space;
        if (getSpaceByName(code_space_name_) == nullptr) {
            code_space_name_ = firstProcessorSpace();
        }
        if (getSpaceByName(data_space_name_) == nullptr) {
            data_space_name_ = code_space_name_;
        }
        setDefaultCodeSpace(getSpaceByName(code_space_name_)->getIndex());
        setDefaultDataSpace(getSpaceByName(data_space_name_)->getIndex());

        for (const RegisterDescription& register_description : description.registers) {
            ghidra::AddrSpace* space = getSpaceByName(register_description.location.space.name());
            if (space == nullptr) {
                throw std::invalid_argument("Register references an unknown address space: " +
                                            register_description.location.space.name());
            }
            validate_storage(register_description.location, "Register description");
            validate_space_range(register_description.location, space, "Register description");
            registers_.emplace(register_description.name,
                               ghidra::VarnodeData{space, register_description.location.offset,
                                                   static_cast<ghidra::uint4>(register_description.location.size)});
        }
    }

    /// Provides the provider's no-op XML initialization hook.
    void initialize(ghidra::DocumentStorage&) override {}

    /// Resolves a named register location.
    const ghidra::VarnodeData& getRegister(const ghidra::string& name) const override {
        const auto iterator = registers_.find(name);
        if (iterator == registers_.end()) {
            throw ghidra::LowlevelError("Unknown provider register: " + name);
        }
        return iterator->second;
    }

    /// Returns the smallest named register containing a location.
    ghidra::string getRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset, ghidra::int4 size) const override {
        if (size <= 0 || offset > std::numeric_limits<ghidra::uintb>::max() - static_cast<ghidra::uintb>(size)) {
            return {};
        }
        ghidra::string result;
        for (const auto& entry : registers_) {
            const ghidra::VarnodeData& location = entry.second;
            if (location.space == space && location.offset <= offset &&
                location.offset <= std::numeric_limits<ghidra::uintb>::max() - location.size &&
                offset + static_cast<ghidra::uintb>(size) <= location.offset + location.size &&
                (result.empty() || location.size < getRegister(result).size)) {
                result = entry.first;
            }
        }
        return result;
    }

    /// Returns a named register with an exactly matching location.
    ghidra::string getExactRegisterName(ghidra::AddrSpace* space, ghidra::uintb offset,
                                        ghidra::int4 size) const override {
        for (const auto& entry : registers_) {
            const ghidra::VarnodeData& location = entry.second;
            if (location.space == space && location.offset == offset && location.size == size) {
                return entry.first;
            }
        }
        return {};
    }

    /// Copies all named register mappings into the requested map.
    void getAllRegisters(std::map<ghidra::VarnodeData, ghidra::string>& result) const override {
        for (const auto& entry : registers_) {
            result.emplace(entry.second, entry.first);
        }
    }

    /// Returns no architecture-specific user operations.
    void getUserOpNames(std::vector<ghidra::string>& result) const override {
        result.clear();
    }

    /// Decodes only enough information to determine instruction length.
    ghidra::int4 instructionLength(const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        validate_instruction(*result, address.getOffset());
        return static_cast<ghidra::int4>(result->length);
    }

    /// Emits provider p-code as native VarnodeData values.
    ghidra::int4 oneInstruction(ghidra::PcodeEmit& emit, const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        validate_instruction(*result, address.getOffset());
        const bool has_call = std::any_of(result->pcode.begin(), result->pcode.end(), [](const PcodeOperation& op) {
            return opcode_value(op.opcode) == static_cast<ghidra::uint4>(ghidra::CPUI_CALL) ||
                   opcode_value(op.opcode) == static_cast<ghidra::uint4>(ghidra::CPUI_CALLIND);
        });
        for (const PcodeOperation& operation : result->pcode) {
            // x86 CALL semantics materialize the return PC as a STORE before
            // the CALL op. The native compiler specification models this
            // location as the function return-address effect; discard only
            // that exact synthetic store so it cannot become a user local.
            if (has_call && opcode_value(operation.opcode) == static_cast<ghidra::uint4>(ghidra::CPUI_STORE) &&
                !operation.inputs.empty() && operation.inputs.back().space.name() == "const" &&
                address.getOffset() <= std::numeric_limits<std::uint64_t>::max() - result->length &&
                operation.inputs.back().offset == address.getOffset() + result->length) {
                continue;
            }
            std::vector<ghidra::VarnodeData> inputs;
            std::size_t input_index = 0;
            if (operation.memory_space.has_value() &&
                (opcode_value(operation.opcode) == static_cast<ghidra::uint4>(ghidra::CPUI_LOAD) ||
                 opcode_value(operation.opcode) == static_cast<ghidra::uint4>(ghidra::CPUI_STORE))) {
                ghidra::AddrSpace* memory_space = getSpaceByName(operation.memory_space->name());
                if (memory_space == nullptr) {
                    throw ghidra::BadDataError("P-code references an unknown memory space: " +
                                               operation.memory_space->name());
                }
                // The original PcodeEmitFd::dump implementation treats the
                // first input as a native varnode before creating the PcodeOp.
                // Validate any legacy selector after resolving its target so a
                // mismatch becomes a diagnostic, not an array access; see
                // `funcdata.cc`, `PcodeEmitFd::dump`.
                if (operation.inputs.size() ==
                        (opcode_value(operation.opcode) == static_cast<ghidra::uint4>(ghidra::CPUI_LOAD) ? 2U : 3U) &&
                    !has_matching_memory_selector(operation, memory_space, this)) {
                    throw ghidra::BadDataError(
                        "Provider p-code " +
                        std::string(ghidra::get_opname(static_cast<ghidra::OpCode>(opcode_value(operation.opcode)))) +
                        " has a legacy selector for a different memory space");
                }
                const ghidra::Address encoded_space = createConstFromSpace(memory_space);
                inputs.push_back(ghidra::VarnodeData{getConstantSpace(), encoded_space.getOffset(),
                                                     static_cast<ghidra::uint4>(pointer_size_)});
                if (has_matching_memory_selector(operation, memory_space, this)) {
                    input_index = 1;
                }
            }
            inputs.reserve(inputs.size() + operation.inputs.size());
            for (; input_index < operation.inputs.size(); ++input_index) {
                inputs.push_back(materialize(operation.inputs[input_index]));
            }
            ghidra::VarnodeData output{};
            ghidra::VarnodeData* output_pointer = nullptr;
            if (operation.output.has_value()) {
                output = materialize(*operation.output);
                output_pointer = &output;
            }
            emit.dump(address, static_cast<ghidra::OpCode>(opcode_value(operation.opcode)), output_pointer,
                      inputs.empty() ? nullptr : inputs.data(), static_cast<ghidra::int4>(inputs.size()));
        }
        return static_cast<ghidra::int4>(result->length);
    }

    /// Emits provider assembly text through the native assembly callback.
    ghidra::int4 printAssembly(ghidra::AssemblyEmit& emit, const ghidra::Address& address) const override {
        const auto result = provider_->decode(address.getOffset());
        if (!result) {
            throw ghidra::BadDataError(result.error().message);
        }
        validate_instruction(*result, address.getOffset());
        emit.dump(address, result->mnemonic, result->assembly);
        return static_cast<ghidra::int4>(result->length);
    }

private:
    /// Converts one public storage record into the native storage triple.
    ghidra::VarnodeData materialize(const Storage& storage) const {
        validate_storage(storage, "P-code storage");
        ghidra::AddrSpace* space = getSpaceByName(storage.space.name());
        if (space == nullptr) {
            throw ghidra::BadDataError("P-code references an unknown address space: " + storage.space.name());
        }
        validate_space_range(storage, space, "P-code storage");
        return ghidra::VarnodeData{space, storage.offset, static_cast<ghidra::uint4>(storage.size)};
    }

    /// Finds the first processor space when a provider omits a default name.
    ghidra::string firstProcessorSpace() const {
        for (int index = 0; index < numSpaces(); ++index) {
            ghidra::AddrSpace* space = getSpace(index);
            if (space != nullptr && space->getType() == ghidra::IPTR_PROCESSOR) {
                return space->getName();
            }
        }
        throw std::invalid_argument("Architecture description has no processor address space");
    }

    std::shared_ptr<PcodeProvider> provider_;
    std::uint32_t pointer_size_;
    std::map<ghidra::string, ghidra::VarnodeData> registers_;
    ghidra::string code_space_name_;
    ghidra::string data_space_name_;
};

/// Supplies memory to the native LoadImage interface.
class ProviderLoadImage final : public ghidra::LoadImage {
public:
    /// Creates a load image backed by the caller's memory provider.
    explicit ProviderLoadImage(std::shared_ptr<MemoryProvider> memory)
        : ghidra::LoadImage("provider-memory"), memory_(std::move(memory)) {}

    /// Reads bytes from the provider and reports unavailable ranges as native errors.
    void loadFill(ghidra::uint1* destination, ghidra::int4 size, const ghidra::Address& address) override {
        if (destination == nullptr || size < 0) {
            throw ghidra::DataUnavailError("Invalid provider memory request");
        }
        if (!memory_) {
            throw ghidra::DataUnavailError("No memory provider is configured");
        }
        const ghidra::AddrSpace* space = address.getSpace();
        if (space == nullptr) {
            throw ghidra::DataUnavailError("Provider memory request has no address space");
        }
        // The native LoadImage contract always carries an Address, not merely
        // an offset. Preserve its space when crossing into the provider API;
        // this follows `loadimage_ghidra.cc`, `LoadImageGhidra::loadFill`, and
        // the address construction in `emulateutil.cc` and `memstate.cc`.
        const auto result = memory_->read(space->getName(), address.getOffset(), static_cast<std::size_t>(size));
        if (!result) {
            throw ghidra::DataUnavailError(result.error().message);
        }
        std::copy(result->begin(), result->end(), destination);
    }

    /// Identifies the provider-backed image.
    ghidra::string getArchType() const override {
        return "provider";
    }

    /// Leaves provider addresses unchanged.
    void adjustVma(long) override {}

private:
    std::shared_ptr<MemoryProvider> memory_;
};

/// Materializes one provider-owned p-code injection against the native context.
class ProviderInjectPayload final : public ghidra::InjectPayload {
public:
    /// Constructs a payload from parameter names and already materialized operations.
    ProviderInjectPayload(ghidra::Architecture* architecture, const std::string& name, ghidra::int4 type,
                          const std::vector<std::string>& input_names, const std::vector<std::string>& output_names,
                          const std::vector<InjectionOperation>& operations, ghidra::int4 parameter_shift,
                          bool incidental_copy)
        : ghidra::InjectPayload(name, type), architecture_(architecture), operations_(operations) {
        paramshift = parameter_shift;
        this->incidentalCopy = incidental_copy;
        for (const std::string& input_name : input_names) {
            inputlist.emplace_back(input_name, 0);
        }
        for (const std::string& output_name : output_names) {
            output.emplace_back(output_name, 0);
        }
        orderParameters();
    }

    /// Emits each provider operation after resolving context input and output placeholders.
    void inject(ghidra::InjectContext& context, ghidra::PcodeEmit& emit) const override {
        for (const InjectionOperation& operation : operations_) {
            if (operation.opcode == 0 || operation.opcode >= static_cast<std::uint32_t>(ghidra::CPUI_MAX)) {
                throw ghidra::BadDataError("Provider injection contains an invalid p-code opcode");
            }
            std::vector<ghidra::VarnodeData> inputs;
            inputs.reserve(operation.inputs.size());
            for (const InjectionVarnode& varnode : operation.inputs) {
                inputs.push_back(resolve(varnode, context));
            }
            ghidra::VarnodeData output{};
            ghidra::VarnodeData* output_pointer = nullptr;
            if (operation.output) {
                output = resolve(*operation.output, context);
                output_pointer = &output;
            }
            emit.dump(context.baseaddr, static_cast<ghidra::OpCode>(operation.opcode), output_pointer,
                      inputs.empty() ? nullptr : inputs.data(), static_cast<ghidra::int4>(inputs.size()));
        }
    }

    /// Rejects XML decoding because provider payloads are supplied as structured records.
    void decode(ghidra::Decoder&) override {
        throw ghidra::LowlevelError("Provider injection payloads do not decode XML");
    }

    /// Prints a compact operation listing useful in native diagnostics.
    void printTemplate(std::ostream& stream) const override {
        stream << "provider injection " << name << " (" << operations_.size() << " operations)";
    }

    /// Identifies the provider metadata source of this payload.
    ghidra::string getSource() const override {
        return "provider injection: " + name;
    }

private:
    /// Resolves one provider varnode against the current injection context.
    ghidra::VarnodeData resolve(const InjectionVarnode& varnode, const ghidra::InjectContext& context) const {
        if (varnode.kind == InjectionVarnodeKind::input) {
            if (varnode.index >= context.inputlist.size()) {
                throw ghidra::BadDataError("Provider injection input index is out of range");
            }
            return context.inputlist[varnode.index];
        }
        if (varnode.kind == InjectionVarnodeKind::output) {
            if (varnode.index >= context.output.size()) {
                throw ghidra::BadDataError("Provider injection output index is out of range");
            }
            return context.output[varnode.index];
        }
        detail::validate_storage(varnode.storage, "Provider injection storage");
        ghidra::AddrSpace* space = architecture_->getSpaceByName(varnode.storage.space.name());
        if (space == nullptr) {
            throw ghidra::BadDataError("Provider injection references an unknown address space: " +
                                       varnode.storage.space.name());
        }
        detail::validate_space_range(varnode.storage, space, "Provider injection storage");
        return ghidra::VarnodeData{space, varnode.storage.offset, static_cast<ghidra::uint4>(varnode.storage.size)};
    }

    ghidra::Architecture* architecture_;
    std::vector<InjectionOperation> operations_;
};

/// Implements provider-owned call-fixup and callother-fixup registration.
class ProviderInjectLibrary final : public ghidra::PcodeInjectLibrary {
public:
    /// Constructs an injection library and registers supplied payloads before analysis.
    ProviderInjectLibrary(ghidra::Architecture* architecture, ghidra::uint4 temporary_base,
                          std::vector<CallFixupDescription> call_fixups,
                          std::vector<CallOtherFixupDescription> call_other_fixups)
        : ghidra::PcodeInjectLibrary(architecture, temporary_base), architecture_(architecture) {
        for (const CallFixupDescription& description : call_fixups) {
            registerCallFixupDescription(description);
        }
        for (const CallOtherFixupDescription& description : call_other_fixups) {
            registerCallOtherFixupDescription(description);
        }
    }

    /// Rejects dynamic XML allocation because provider payloads are structured records.
    ghidra::int4 allocateInject(const ghidra::string&, const ghidra::string&, ghidra::int4) override {
        throw ghidra::LowlevelError("Provider injection allocation requires a structured provider payload");
    }

    /// Leaves provider payloads finalized during construction.
    void registerInject(ghidra::int4) override {}

    /// Rejects textual SLEIGH because this boundary accepts structured p-code only.
    ghidra::int4 manualCallFixup(const ghidra::string&, const ghidra::string&) override {
        throw ghidra::LowlevelError("Provider call-fixup requires InjectionOperation records, not SLEIGH text");
    }

    /// Rejects textual callother SLEIGH for the same structured-provider reason.
    ghidra::int4 manualCallOtherFixup(const ghidra::string&, const ghidra::string&, const std::vector<ghidra::string>&,
                                      const ghidra::string&) override {
        throw ghidra::LowlevelError("Provider callother-fixup requires InjectionOperation records, not SLEIGH text");
    }

    /// Returns the reusable provider injection context.
    ghidra::InjectContext& getCachedContext() override {
        return context_;
    }

    /// Returns native p-code behaviors for provider payload consumers.
    const std::vector<ghidra::OpBehavior*>& getBehaviors() override {
        if (behaviors_.empty()) {
            architecture_->collectBehaviors(behaviors_);
        }
        return behaviors_;
    }

private:
    /// Registers one call-fixup and publishes its name-to-id mapping.
    void registerCallFixupDescription(const CallFixupDescription& description) {
        if (description.name.empty()) {
            throw std::invalid_argument("Provider call-fixup name must not be empty");
        }
        const ghidra::int4 id = static_cast<ghidra::int4>(injection.size());
        injection.push_back(new ProviderInjectPayload(architecture_, description.name,
                                                      ghidra::InjectPayload::CALLFIXUP_TYPE, description.input_names,
                                                      description.output_names, description.operations,
                                                      description.parameter_shift, description.incidental_copy));
        registerCallFixup(description.name, id);
    }

    /// Registers one callother-fixup and publishes its target-op mapping.
    void registerCallOtherFixupDescription(const CallOtherFixupDescription& description) {
        if (description.name.empty()) {
            throw std::invalid_argument("Provider callother-fixup name must not be empty");
        }
        const std::vector<std::string> outputs = description.output_name.empty()
                                                     ? std::vector<std::string>{}
                                                     : std::vector<std::string>{description.output_name};
        const ghidra::int4 id = static_cast<ghidra::int4>(injection.size());
        injection.push_back(
            new ProviderInjectPayload(architecture_, description.name, ghidra::InjectPayload::CALLOTHERFIXUP_TYPE,
                                      description.input_names, outputs, description.operations, 0, false));
        registerCallOtherFixup(description.name, id);
    }

    /// Concrete context required by the native abstract injection API.
    class EmptyContext final : public ghidra::InjectContext {
    public:
        /// Encodes no state because provider contexts never cross an XML boundary.
        void encode(ghidra::Encoder&) const override {}
    };

    EmptyContext context_;
    std::vector<ghidra::OpBehavior*> behaviors_;
    ghidra::Architecture* architecture_;
};

/// Owns the native Architecture subsystems configured from explicit providers.
class ProviderArchitecture final : public ghidra::Architecture {
public:
    /// Constructs the native engine and all in-memory provider-backed services.
    ProviderArchitecture(const ArchitectureDescription& description, std::shared_ptr<PcodeProvider> provider,
                         std::shared_ptr<MemoryProvider> memory, std::shared_ptr<InjectionProvider> injections,
                         AnalysisOptions analysis_options)
        : description_(description), provider_(std::move(provider)), memory_(std::move(memory)),
          injections_(std::move(injections)), analysis_options_(analysis_options) {
        ghidra::forcePrintCLanguageRegistration();
        if (description_.pointer_size == 0) {
            throw std::invalid_argument("Architecture pointer size must be greater than zero");
        }
        const auto supplied_space = [&](const std::string& name) {
            return std::any_of(description_.spaces.begin(), description_.spaces.end(),
                               [&](const SpaceDescription& space) { return space.name == name; });
        };
        if (!description_.spaces.empty()) {
            const auto first_space = std::find_if(description_.spaces.begin(), description_.spaces.end(),
                                                  [](const SpaceDescription& space) { return !space.name.empty(); });
            if (first_space == description_.spaces.end()) {
                throw std::invalid_argument("Architecture description has no named processor address space");
            }
            if (!supplied_space(description_.code_space)) {
                description_.code_space = first_space->name;
            }
            if (!supplied_space(description_.data_space)) {
                description_.data_space = description_.code_space;
            }
        }
        initialize();
    }

    /// Returns the owned provider translator.
    ProviderTranslate* providerTranslate() const {
        return static_cast<ProviderTranslate*>(const_cast<ghidra::Translate*>(translate));
    }

    /// Prints warnings to the provider diagnostic stream.
    void printWarning(const ghidra::string& message) const override {
        warnings_ << message << '\n';
    }

private:
    /// Applies provider volatile ranges after the compiler-spec bootstrap has
    /// created the final global scope and address-space map.
    void apply_provider_volatile_ranges() {
        if (!memory_) {
            return;
        }
        for (const MemoryRangeDescription& volatile_range : memory_->volatile_ranges()) {
            if (volatile_range.size == 0 ||
                volatile_range.first > std::numeric_limits<std::uint64_t>::max() - (volatile_range.size - 1U)) {
                throw std::invalid_argument("Provider volatile range is empty or overflows its address space");
            }
            ghidra::AddrSpace* space = translate->getSpaceByName(volatile_range.space);
            if (space == nullptr) {
                throw std::invalid_argument("Provider volatile range references an unknown space: " +
                                            volatile_range.space);
            }
            const std::uint64_t last = volatile_range.first + volatile_range.size - 1U;
            if (volatile_range.first > space->getHighest() || last > space->getHighest()) {
                throw std::invalid_argument("Provider volatile range exceeds its address space: " +
                                            volatile_range.space);
            }
            symboltab->setPropertyRange(ghidra::Varnode::volatil, ghidra::Range(space, volatile_range.first, last));
        }
    }

    /// Applies provider-owned analysis options through the native option
    /// dispatcher before action construction. This preserves the original
    /// option validation and side effects instead of assigning only a subset
    /// of the corresponding Architecture fields.
    void apply_provider_analysis_options() {
        if (analysis_options_.readonly_propagate) {
            options->set(ghidra::ElementId::find("readonly", 0), "on");
        }
        if (analysis_options_.integer_format != DisplayFormat::none) {
            const auto format_name = [&]() -> std::string {
                switch (analysis_options_.integer_format) {
                    case DisplayFormat::hexadecimal:
                        return "hex";
                    case DisplayFormat::decimal:
                        return "dec";
                    case DisplayFormat::octal:
                        return "oct";
                    case DisplayFormat::binary:
                        return "bin";
                    case DisplayFormat::character:
                        return "char";
                    case DisplayFormat::none:
                        break;
                }
                throw std::invalid_argument("Provider integer format is invalid");
            }();
            options->set(ghidra::ElementId::find("integerformat", 0), format_name);
        }
        switch (analysis_options_.nan_handling) {
            case NanHandling::native_default:
                break;
            case NanHandling::none:
                options->set(ghidra::ElementId::find("nanignore", 0), "none");
                break;
            case NanHandling::compare:
                options->set(ghidra::ElementId::find("nanignore", 0), "compare");
                break;
            case NanHandling::all:
                options->set(ghidra::ElementId::find("nanignore", 0), "all");
                break;
        }
    }

    /// Builds every native subsystem that does not require XML or Java state.
    void initialize() {
        ghidra::AttributeId::initialize();
        ghidra::ElementId::initialize();
        loader = new ProviderLoadImage(memory_);
        translate = new ProviderTranslate(description_, provider_);
        copySpaces(translate);
        insertSpace(new ghidra::OtherSpace(this, translate, ghidra::OtherSpace::INDEX));
        insertSpace(new ghidra::FspecSpace(this, translate, numSpaces()));
        insertSpace(new ghidra::IopSpace(this, translate, numSpaces()));
        insertSpace(new ghidra::JoinSpace(this, translate, numSpaces()));
        context = new ghidra::ContextInternal();
        types = new ghidra::TypeFactory(this);
        types->setupSizes();
        types->setCoreType("void", 1, ghidra::TYPE_VOID, false);
        types->setCoreType("bool", 1, ghidra::TYPE_BOOL, false);
        types->setCoreType("uint1", 1, ghidra::TYPE_UINT, false);
        types->setCoreType("uint2", 2, ghidra::TYPE_UINT, false);
        types->setCoreType("uint4", 4, ghidra::TYPE_UINT, false);
        types->setCoreType("uint8", 8, ghidra::TYPE_UINT, false);
        types->setCoreType("int1", 1, ghidra::TYPE_INT, false);
        types->setCoreType("int2", 2, ghidra::TYPE_INT, false);
        types->setCoreType("int4", 4, ghidra::TYPE_INT, false);
        types->setCoreType("int8", 8, ghidra::TYPE_INT, false);
        types->setCoreType("float4", 4, ghidra::TYPE_FLOAT, false);
        types->setCoreType("float8", 8, ghidra::TYPE_FLOAT, false);
        types->setCoreType("float10", 10, ghidra::TYPE_FLOAT, false);
        types->setCoreType("float16", 16, ghidra::TYPE_FLOAT, false);
        types->setCoreType("xunknown1", 1, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("xunknown2", 2, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("xunknown4", 4, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("xunknown8", 8, ghidra::TYPE_UNKNOWN, false);
        types->setCoreType("code", 1, ghidra::TYPE_CODE, false);
        types->setCoreType("char", 1, ghidra::TYPE_INT, true);
        types->setCoreType("wchar2", 2, ghidra::TYPE_INT, true);
        types->setCoreType("wchar4", 4, ghidra::TYPE_INT, true);
        types->cacheCoreTypes();
        commentdb = new ghidra::CommentDatabaseInternal();
        stringManager = new ghidra::StringManagerUnicode(this, 4096);
        cpool = new ghidra::ConstantPoolInternal();
        symboltab = new ghidra::Database(this, true);
        symboltab->attachScope(new ghidra::ScopeInternal(0, "", this), nullptr);
        symboltab->addRange(symboltab->getGlobalScope(), getDefaultDataSpace(), 0, getDefaultDataSpace()->getHighest());
        const std::vector<CallFixupDescription> call_fixups =
            injections_ ? injections_->call_fixups() : std::vector<CallFixupDescription>{};
        const std::vector<CallOtherFixupDescription> call_other_fixups =
            injections_ ? injections_->call_other_fixups() : std::vector<CallOtherFixupDescription>{};
        pcodeinjectlib = new ProviderInjectLibrary(this, translate->getUniqueStart(ghidra::Translate::INJECT),
                                                   call_fixups, call_other_fixups);
        userops.initialize(this);
        for (const CallOtherFixupDescription& description : call_other_fixups) {
            const ghidra::int4 payload_id =
                pcodeinjectlib->getPayloadId(ghidra::InjectPayload::CALLOTHERFIXUP_TYPE, description.name);
            if (payload_id < 0) {
                throw std::invalid_argument("Provider callother-fixup was not registered: " + description.name);
            }
            userops.registerInjected(description.name, description.userop_index, payload_id);
        }
        // XML compiler specifications normally register these operations while
        // decoding <volatile>. Provider architectures have no XML document, so
        // install the native defaults explicitly before volatile analysis runs.
        userops.registerBuiltin(ghidra::UserPcodeOp::BUILTIN_VOLATILE_READ);
        userops.registerBuiltin(ghidra::UserPcodeOp::BUILTIN_VOLATILE_WRITE);
        ghidra::DocumentStorage specification;
        // The bootstrap model follows `architecture.cc`'s compiler-spec
        // boundary but is deliberately assembled only from provider metadata.
        // It must not mention x86 registers, `ram`, or `stack` for an
        // architecture that did not supply those names.
        const auto xml_escape = [](std::string_view value) {
            std::string escaped;
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
                    case '\"':
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
        };
        const std::string model_name =
            description_.calling_convention.empty() || description_.calling_convention == "__thiscall"
                ? "provider-default"
                : description_.calling_convention;
        std::string bootstrap_stack_register = description_.stack_register;
        const auto stack_register_exists = [&](std::string_view name) {
            return std::any_of(description_.registers.begin(), description_.registers.end(),
                               [&](const RegisterDescription& reg) { return reg.name == name; });
        };
        const bool has_declared_stack_register = stack_register_exists(description_.stack_register);
        if (!stack_register_exists(bootstrap_stack_register)) {
            const auto first_register = std::find_if(
                description_.registers.begin(), description_.registers.end(),
                [](const RegisterDescription& reg) { return !reg.name.empty() && reg.location.size != 0; });
            if (first_register == description_.registers.end()) {
                throw std::invalid_argument("Architecture bootstrap requires at least one named register");
            }
            // The native ProtoModel requires a stack space even for a
            // provider that has no declared ABI stack register. Use the first
            // provider register only as a generic bootstrap anchor; never
            // synthesize an x86 register or address-space name.
            bootstrap_stack_register = first_register->name;
        }
        std::string input_register_entries;
        std::string output_register_entry;
        std::vector<std::pair<std::string, std::string>> input_register_candidates;
        for (const RegisterDescription& register_description : description_.registers) {
            if (register_description.name.empty() || register_description.location.size == 0) {
                continue;
            }
            const std::string entry = "<pentry minsize=\"1\" maxsize=\"" +
                                      std::to_string(register_description.location.size) + "\"><register name=\"" +
                                      xml_escape(register_description.name) + "\"/></pentry>";
            if (output_register_entry.empty()) {
                output_register_entry = entry;
            } else if (register_description.name != bootstrap_stack_register) {
                input_register_candidates.emplace_back(register_description.name, entry);
            }
        }
        // Keep the bootstrap model small and deterministic. Provider register
        // order remains authoritative; when more than four inputs are offered,
        // retain the first and last two slots rather than introducing an ABI
        // name or architecture-specific calling convention.
        const auto append_input = [&](std::size_t index) {
            input_register_entries += input_register_candidates[index].second;
        };
        if (input_register_candidates.size() <= 4) {
            for (std::size_t index = 0; index < input_register_candidates.size(); ++index) {
                append_input(index);
            }
        } else {
            append_input(0);
            append_input(1);
            append_input(input_register_candidates.size() - 2);
            append_input(input_register_candidates.size() - 1);
        }
        const std::string data_space = xml_escape(description_.data_space);
        const auto has_register = [&](std::string_view name) {
            return std::any_of(description_.registers.begin(), description_.registers.end(),
                               [&](const RegisterDescription& reg) { return reg.name == name; });
        };
        const bool legacy_x86 = description_.data_space == "ram" && description_.stack_register == "RSP" &&
                                has_register("RCX") && has_register("RDX") && has_register("R8") &&
                                has_register("R9") && has_register("RAX");
        std::string specification_xml;
        if (legacy_x86) {
            // Preserve the original provider's x86-64 bootstrap contract and
            // generated C output while allowing generic descriptions below.
            // The GCC compiler specification declares four-byte wchar_t; this
            // controls ArraySequence's wcsncpy-versus-memcpy selection.
            specification_xml = "<compiler_spec><data_organization><wchar_size value=\"4\"/></data_organization>"
                                "<global><range space=\"ram\"/></global>";
            specification_xml += "<stackpointer register=\"RSP\" space=\"ram\"/>";
            specification_xml += "<returnaddress><varnode space=\"stack\" offset=\"0\" size=\"8\"/></returnaddress>";
            specification_xml += "<default_proto><prototype name=\"" + xml_escape(description_.calling_convention) +
                                 "\" extrapop=\"8\" stackshift=\"8\"><input>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"RCX\"/></pentry>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"RDX\"/></pentry>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"R8\"/></pentry>"
                                 "<pentry minsize=\"1\" maxsize=\"8\"><register name=\"R9\"/></pentry>"
                                 "</input><output><pentry minsize=\"1\" maxsize=\"8\"><register name=\"RAX\"/>"
                                 "</pentry></output></prototype></default_proto></compiler_spec>";
        } else {
            specification_xml = "<compiler_spec><global><range space=\"" + data_space + "\"/></global>";
            if (getSpaceByName(description_.data_space) != nullptr) {
                specification_xml += "<stackpointer register=\"" + xml_escape(bootstrap_stack_register) +
                                     "\" space=\"" + data_space + "\"/>";
                if (has_declared_stack_register && getSpaceByName("stack") != nullptr) {
                    specification_xml += "<returnaddress><varnode space=\"stack\" offset=\"0\" size=\"" +
                                         std::to_string(description_.pointer_size) + "\"/></returnaddress>";
                }
            }
            specification_xml += "<default_proto><prototype name=\"" + xml_escape(model_name) +
                                 "\" extrapop=\"0\"><input>" + input_register_entries + "</input><output>" +
                                 output_register_entry + "</output></prototype></default_proto></compiler_spec>";
        }
        std::istringstream specification_text(specification_xml);
        ghidra::Document* document = specification.parseDocument(specification_text);
        specification.registerTag(document->getRoot());
        parseCompilerConfig(specification);
        apply_provider_analysis_options();
        apply_provider_volatile_ranges();
        if (defaultfp != nullptr) {
            defaultfp->setPrintInDecl(true);
        }
        const_cast<ghidra::Translate*>(translate)->setDefaultFloatFormats();
        ghidra::DocumentStorage empty;
        buildInstructions(empty);
        buildAction(empty);
        print->initializeFromArchitecture();
    }

    /// Provides the factory hook for the native Architecture base.
    ghidra::Translate* buildTranslator(ghidra::DocumentStorage&) override {
        return const_cast<ghidra::Translate*>(translate);
    }

    /// Provides the factory hook for the native Architecture base.
    void buildLoader(ghidra::DocumentStorage&) override {}

    /// Returns the already-created empty injection library.
    ghidra::PcodeInjectLibrary* buildPcodeInjectLibrary() override {
        return pcodeinjectlib;
    }

    /// Keeps the provider-created type factory.
    void buildTypegrp(ghidra::DocumentStorage&) override {}

    /// Keeps primitive type defaults supplied by TypeFactory.
    void buildCoreTypes(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory comment database.
    void buildCommentDB(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory string manager.
    void buildStringManager(ghidra::DocumentStorage&) override {}

    /// Keeps the in-memory constant pool.
    void buildConstantPool(ghidra::DocumentStorage&) override {}

    /// Keeps the empty context database.
    void buildContext(ghidra::DocumentStorage&) override {}

    /// Does not import external symbols; callers provide them through the contract.
    void buildSymbols(ghidra::DocumentStorage&) override {}

    /// Does not load processor XML; provider metadata is already materialized.
    void buildSpecFile(ghidra::DocumentStorage&) override {}

    /// Does not mutate provider address spaces after construction.
    void modifySpaces(ghidra::Translate*) override {}

    /// Resolves no architecture fields beyond the explicit description.
    void resolveArchitecture() override {
        archid = description_.name;
    }

    ArchitectureDescription description_;
    std::shared_ptr<PcodeProvider> provider_;
    std::shared_ptr<MemoryProvider> memory_;
    std::shared_ptr<InjectionProvider> injections_;
    AnalysisOptions analysis_options_;
    mutable std::ostringstream warnings_;
};

/// Converts the provider display-format enum to the native decompiler encoding.
///
/// The native `Datatype` and `Scope` APIs use the historical ordinal values
/// 1=hex, 2=decimal, 3=octal, 4=binary, and 5=character.
static ghidra::uint4 native_display_format(DisplayFormat format) {
    switch (format) {
        case DisplayFormat::none:
            return 0;
        case DisplayFormat::hexadecimal:
            return 1;
        case DisplayFormat::decimal:
            return 2;
        case DisplayFormat::octal:
            return 3;
        case DisplayFormat::binary:
            return 4;
        case DisplayFormat::character:
            return 5;
    }
    throw std::invalid_argument("Provider display format is invalid");
}

/// Applies a provider format only when it explicitly forces one.
///
/// Leaving the native default untouched is important for compiler-spec types
/// shared by multiple provider records. The caller owns the returned type.
static ghidra::Datatype* apply_type_display_format(ghidra::TypeFactory* types, ghidra::Datatype* type,
                                                   DisplayFormat format) {
    if (format != DisplayFormat::none) {
        types->setDisplayFormat(type, native_display_format(format));
    }
    return type;
}

/// Resolves a provider type into the native type factory while retaining the
/// provider's composite shape, signedness, explicit sizes, typedef identity,
/// and optional source declaration. Recursive types are inserted into the
/// cache before their fields are resolved so self-referential providers can be
/// represented by the native factory.
/// Composite construction follows `type.cc`'s TypeFactory/TypeStruct/TypeUnion
/// responsibilities rather than reducing provider declarations to integers.
static ghidra::Datatype* resolve_provider_type(const ProviderContext& context, ghidra::TypeFactory* types,
                                               const std::string& name, std::map<std::string, ghidra::Datatype*>& cache,
                                               std::uint32_t pointer_size, std::vector<std::string>* declarations) {
    const auto cached = cache.find(name);
    if (cached != cache.end()) {
        return cached->second;
    }
    if (name == "void") {
        return types->getTypeVoid();
    }

    TypeDescription description;
    description.name = name;
    description.size = pointer_size;
    description.kind = name == "wchar_t" ? TypeKind::unicode_character
                                         : (name.find("unsigned") != std::string::npos ? TypeKind::unsigned_integer
                                                                                       : TypeKind::signed_integer);
    if (context.types) {
        if (const std::optional<TypeDescription> supplied = context.types->type_named(name)) {
            description = *supplied;
        }
    }
    const std::string effective_name = description.name.empty() ? name : description.name;
    if (declarations != nullptr && !description.declaration.empty() &&
        std::find(declarations->begin(), declarations->end(), description.declaration) == declarations->end()) {
        declarations->push_back(description.declaration);
    }
    // Compiler specifications can predeclare provider names. Reusing those
    // native datatypes avoids redefining a composite when several functions
    // reference the same metadata name.
    if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
        cache.emplace(name, existing);
        return apply_type_display_format(types, existing, description.display_format);
    }

    if (description.kind == TypeKind::void_type || name == "void") {
        cache.emplace(name, types->getTypeVoid());
        return types->getTypeVoid();
    }
    if (description.kind == TypeKind::pointer) {
        ghidra::Datatype* pointed_to =
            resolve_provider_type(context, types, description.element_type, cache, pointer_size, declarations);
        const std::uint32_t size = description.size == 0 ? pointer_size : description.size;
        if (size == 0 || size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider pointer type has an invalid size: " + effective_name);
        }
        if (!description.relative_parent_type.empty()) {
            ghidra::Datatype* parent = resolve_provider_type(context, types, description.relative_parent_type, cache,
                                                             pointer_size, declarations);
            if (parent->getMetatype() != ghidra::TYPE_STRUCT && parent->getMetatype() != ghidra::TYPE_ARRAY) {
                throw std::invalid_argument("Provider relative pointer parent is not a structure or array: " +
                                            effective_name);
            }
            if (description.relative_offset == 0 || description.relative_offset >= parent->getSize() ||
                description.relative_offset < -parent->getSize()) {
                throw std::invalid_argument("Provider relative pointer offset is outside its parent: " +
                                            effective_name);
            }
            const ghidra::uint4 word_size = types->getArch()->getDefaultDataSpace()->getWordSize();
            ghidra::TypePointerRel* relative =
                types->getTypePointerRel(static_cast<ghidra::int4>(size), parent, pointed_to, word_size,
                                         description.relative_offset, effective_name);
            cache.emplace(name, relative);
            return apply_type_display_format(types, relative, description.display_format);
        }
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getMetatype() != ghidra::TYPE_PTR || existing->getSize() != static_cast<ghidra::int4>(size)) {
                throw std::invalid_argument("Provider pointer conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return apply_type_display_format(types, existing, description.display_format);
        }
        // `TypeFactory::getTypePointer` stores the addressable-unit width in
        // each pointer. Use the architecture data space, not byte-addressing
        // by default; this is the same invariant used by `type.cc` and the
        // pointer construction in `fspec.cc`.
        const ghidra::uint4 word_size = types->getArch()->getDefaultDataSpace()->getWordSize();
        ghidra::TypePointer* pointer =
            types->getTypePointer(static_cast<ghidra::int4>(size), pointed_to, word_size, effective_name);
        cache.emplace(name, pointer);
        return apply_type_display_format(types, pointer, description.display_format);
    }
    if (description.kind == TypeKind::array) {
        if (description.element_count > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider array has too many elements: " + effective_name);
        }
        ghidra::Datatype* element =
            resolve_provider_type(context, types, description.element_type, cache, pointer_size, declarations);
        // Native TypeArray uses the aligned element stride, not the packed
        // nominal size. Match its constructor in `type.cc` so arrays of types
        // with tail padding retain the original field layout.
        const std::uint64_t element_stride = static_cast<std::uint64_t>(element->getAlignSize());
        if (element_stride == 0 && description.element_count != 0) {
            throw std::invalid_argument("Provider array element has no aligned storage: " + effective_name);
        }
        if (description.element_count != 0 &&
            element_stride > std::numeric_limits<std::uint64_t>::max() / description.element_count) {
            throw std::invalid_argument("Provider array size overflows the native type limit: " + effective_name);
        }
        const std::uint64_t array_size = element_stride * description.element_count;
        if (array_size > static_cast<std::uint64_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider array size overflows the native type limit: " + effective_name);
        }
        if (description.size != 0 && description.size != array_size) {
            throw std::invalid_argument("Provider array size does not match its element layout: " + effective_name);
        }
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getMetatype() != ghidra::TYPE_ARRAY ||
                existing->getSize() != static_cast<ghidra::int4>(array_size)) {
                throw std::invalid_argument("Provider array conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return apply_type_display_format(types, existing, description.display_format);
        }
        ghidra::TypeArray* array = types->getTypeArray(static_cast<ghidra::int4>(description.element_count), element);
        cache.emplace(name, array);
        return apply_type_display_format(types, array, description.display_format);
    }
    if (description.kind == TypeKind::enumeration) {
        const std::uint32_t enum_size = description.size == 0 ? pointer_size : description.size;
        if (enum_size == 0 || enum_size > 8U ||
            enum_size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider enumeration has an invalid size: " + effective_name);
        }
        ghidra::Datatype* existing = types->findByName(effective_name);
        if (existing != nullptr &&
            (!existing->isEnumType() || existing->getSize() != static_cast<ghidra::int4>(enum_size))) {
            throw std::invalid_argument("Provider enumeration conflicts with an existing type: " + effective_name);
        }
        auto* enumeration =
            existing == nullptr
                ? types->getTypeEnum(effective_name, static_cast<ghidra::int4>(enum_size), description.signed_value)
                : static_cast<ghidra::TypeEnum*>(existing);
        const std::uint64_t value_mask = enum_size >= sizeof(std::uint64_t)
                                             ? std::numeric_limits<std::uint64_t>::max()
                                             : (std::uint64_t{1} << (enum_size * 8U)) - 1U;
        std::map<ghidra::uintb, ghidra::string> values;
        for (const TypeEnumValueDescription& value : description.enum_values) {
            if (value.name.empty()) {
                throw std::invalid_argument("Provider enumeration contains an unnamed value: " + effective_name);
            }
            values[static_cast<ghidra::uintb>(static_cast<std::uint64_t>(value.value) & value_mask)] = value.name;
        }
        types->setEnumValues(values, enumeration);
        cache.emplace(name, enumeration);
        return apply_type_display_format(types, enumeration, description.display_format);
    }
    if (description.kind == TypeKind::structure || description.kind == TypeKind::union_type) {
        const bool is_union = description.kind == TypeKind::union_type;
        const ghidra::type_metatype expected_metatype = is_union ? ghidra::TYPE_UNION : ghidra::TYPE_STRUCT;
        ghidra::Datatype* composite = types->findByName(effective_name);
        if (composite != nullptr) {
            if (composite->getMetatype() != expected_metatype) {
                throw std::invalid_argument("Provider type changes composite kind: " + effective_name);
            }
            // A complete native definition may have come from another
            // provider lookup or compiler specification. Reuse it instead of
            // passing it through assignRawFields a second time.
            if (!composite->isIncomplete()) {
                if (description.size != 0 && composite->getSize() != static_cast<ghidra::int4>(description.size)) {
                    throw std::invalid_argument("Provider composite conflicts with an existing type: " +
                                                effective_name);
                }
                cache.emplace(name, composite);
                return apply_type_display_format(types, composite, description.display_format);
            }
        } else {
            composite = is_union ? static_cast<ghidra::Datatype*>(types->getTypeUnion(effective_name))
                                 : static_cast<ghidra::Datatype*>(types->getTypeStruct(effective_name));
        }
        cache.emplace(name, composite);
        std::vector<ghidra::TypeField> fields;
        ghidra::int4 field_id = 0;
        std::vector<ghidra::TypeBitField> bitfields;
        const auto align_up = [&](std::uint64_t value, std::uint64_t alignment) -> std::uint64_t {
            if (alignment <= 1) {
                return value;
            }
            const std::uint64_t remainder = value % alignment;
            if (remainder == 0) {
                return value;
            }
            const std::uint64_t result = value + alignment - remainder;
            if (result < value) {
                throw std::invalid_argument("Provider composite layout overflows: " + effective_name);
            }
            return result;
        };

        std::uint64_t inferred_size = 0;
        std::uint64_t maximum_alignment = 1;
        if (is_union) {
            for (const TypeFieldDescription& field : description.fields) {
                ghidra::Datatype* field_type =
                    resolve_provider_type(context, types, field.type_name, cache, pointer_size, declarations);
                if (field_type->getSize() <= 0) {
                    throw std::invalid_argument("Provider union field has no storage: " + effective_name);
                }
                fields.emplace_back(field_id++, 0, field.name, field_type);
                inferred_size = std::max(inferred_size, static_cast<std::uint64_t>(field_type->getSize()));
                maximum_alignment = std::max(maximum_alignment, static_cast<std::uint64_t>(field_type->getAlignment()));
            }
        } else {
            const bool has_bitfields = !description.bitfields.empty();
            std::uint64_t cursor = 0;
            std::vector<std::uint32_t> adjusted_groups(description.fields.size() + 1U);
            std::uint32_t padding_count = 0;
            for (std::size_t index = 0; index < description.fields.size(); ++index) {
                const TypeFieldDescription& field = description.fields[index];
                ghidra::Datatype* field_type =
                    resolve_provider_type(context, types, field.type_name, cache, pointer_size, declarations);
                if (field_type->getSize() <= 0) {
                    throw std::invalid_argument("Provider structure field has no storage: " + effective_name);
                }
                if (has_bitfields && field.offset != 0) {
                    throw std::invalid_argument(
                        "Provider structures with bitfields must use native declaration-order offsets: " +
                        effective_name);
                }
                const std::uint64_t aligned_cursor =
                    align_up(cursor, static_cast<std::uint64_t>(field_type->getAlignment()));
                const std::uint64_t requested_offset = field.offset;
                if (!has_bitfields && requested_offset < aligned_cursor) {
                    throw std::invalid_argument("Provider structure field offsets overlap or are out of order: " +
                                                effective_name);
                }
                const std::uint64_t target = has_bitfields ? aligned_cursor : requested_offset;
                if (!has_bitfields && target > aligned_cursor) {
                    const std::uint64_t gap = target - aligned_cursor;
                    if (gap > static_cast<std::uint64_t>(std::numeric_limits<ghidra::int4>::max())) {
                        throw std::invalid_argument("Provider structure padding is too large: " + effective_name);
                    }
                    fields.emplace_back(field_id++, -1, "__provider_padding_" + std::to_string(padding_count++),
                                        types->getBase(static_cast<ghidra::int4>(gap), ghidra::TYPE_UNKNOWN));
                    cursor = target;
                }
                if (has_bitfields) {
                    adjusted_groups[index] = static_cast<std::uint32_t>(fields.size());
                }
                fields.emplace_back(field_id++, -1, field.name, field_type);
                cursor = target + static_cast<std::uint64_t>(field_type->getAlignSize());
                maximum_alignment = std::max(maximum_alignment, static_cast<std::uint64_t>(field_type->getAlignment()));
            }
            adjusted_groups.back() = static_cast<std::uint32_t>(fields.size());

            std::optional<std::uint32_t> previous_group;
            std::map<std::uint32_t, std::uint64_t> group_bits;
            std::map<std::uint32_t, std::uint32_t> group_alignment;
            for (const TypeBitFieldDescription& bitfield : description.bitfields) {
                if (bitfield.name.empty() || bitfield.bit_count == 0 || bitfield.group > description.fields.size()) {
                    throw std::invalid_argument("Provider structure contains an invalid bitfield: " + effective_name);
                }
                if (previous_group && bitfield.group < *previous_group) {
                    throw std::invalid_argument("Provider bitfield groups are not in declaration order: " +
                                                effective_name);
                }
                previous_group = bitfield.group;
                ghidra::Datatype* field_type =
                    resolve_provider_type(context, types, bitfield.type_name, cache, pointer_size, declarations);
                const ghidra::type_metatype metatype = field_type->getMetatype();
                if (metatype != ghidra::TYPE_INT && metatype != ghidra::TYPE_UINT && metatype != ghidra::TYPE_BOOL) {
                    throw std::invalid_argument("Provider bitfield type is not an integer: " + effective_name);
                }
                const std::uint64_t type_bits = static_cast<std::uint64_t>(field_type->getSize()) * 8U;
                if (bitfield.bit_count > type_bits) {
                    throw std::invalid_argument("Provider bitfield exceeds its storage type: " + effective_name);
                }
                const std::uint32_t native_group = adjusted_groups[bitfield.group];
                bitfields.emplace_back(
                    static_cast<ghidra::int4>(native_group), static_cast<ghidra::int4>(bitfield.bit_count),
                    types->getArch()->getDefaultDataSpace()->isBigEndian(), bitfield.name, field_type);
                group_bits[native_group] += bitfield.bit_count;
                group_alignment[native_group] =
                    std::max(group_alignment[native_group], static_cast<std::uint32_t>(field_type->getAlignment()));
            }

            // Recalculate the exact layout used by TypeStruct::assignFieldOffsets
            // so an explicitly reported size can be checked before and after
            // native field assignment, including interleaved bitfield groups.
            cursor = 0;
            maximum_alignment = 1;
            for (std::size_t position = 0; position <= fields.size(); ++position) {
                const auto group = group_bits.find(static_cast<std::uint32_t>(position));
                if (group != group_bits.end()) {
                    const std::uint64_t alignment = group_alignment.at(group->first);
                    cursor = align_up(cursor, alignment);
                    cursor += (group->second + 7U) / 8U;
                    maximum_alignment = std::max(maximum_alignment, alignment);
                }
                if (position == fields.size()) {
                    break;
                }
                const ghidra::Datatype* field_type = fields[position].type;
                cursor = align_up(cursor, static_cast<std::uint64_t>(field_type->getAlignment()));
                cursor += static_cast<std::uint64_t>(field_type->getAlignSize());
                maximum_alignment = std::max(maximum_alignment, static_cast<std::uint64_t>(field_type->getAlignment()));
            }
            inferred_size = align_up(cursor, maximum_alignment);
        }
        if (description.size != 0) {
            if (description.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max()) ||
                description.size < inferred_size) {
                throw std::invalid_argument("Provider composite type has an incompatible size: " + effective_name);
            }
            if (is_union) {
                if (description.size > inferred_size) {
                    fields.emplace_back(
                        field_id++, 0, "__provider_padding",
                        types->getBase(static_cast<ghidra::int4>(description.size), ghidra::TYPE_UNKNOWN));
                }
            } else if (description.size > inferred_size) {
                const ghidra::int4 padding_size = static_cast<ghidra::int4>(description.size - inferred_size);
                fields.emplace_back(field_id++, -1, "__provider_padding",
                                    types->getBase(padding_size, ghidra::TYPE_UNKNOWN));
            }
        }
        if (is_union) {
            auto* union_type = static_cast<ghidra::TypeUnion*>(composite);
            types->assignRawFields(union_type, fields);
        } else {
            auto* structure = static_cast<ghidra::TypeStruct*>(composite);
            types->assignRawFields(structure, fields, bitfields);
        }
        if (description.size != 0 && composite->getSize() != static_cast<ghidra::int4>(description.size)) {
            throw std::invalid_argument("Native composite layout does not match provider size: " + effective_name);
        }
        return apply_type_display_format(types, composite, description.display_format);
    }

    if (description.kind == TypeKind::unicode_character) {
        const std::uint32_t size = description.size == 0 ? 2 : description.size;
        if (size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider unicode type has an invalid size: " + effective_name);
        }
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getMetatype() != ghidra::TYPE_INT || existing->getSize() != static_cast<ghidra::int4>(size)) {
                throw std::invalid_argument("Provider unicode type conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return apply_type_display_format(types, existing, description.display_format);
        }
        ghidra::Datatype* result =
            types->getProviderUnicode(effective_name, static_cast<ghidra::int4>(size), ghidra::TYPE_INT);
        cache.emplace(name, result);
        return apply_type_display_format(types, result, description.display_format);
    }
    if (description.kind == TypeKind::typedef_type) {
        if (description.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider typedef has an invalid size: " + effective_name);
        }
        ghidra::Datatype* underlying =
            description.element_type.empty()
                ? types->getBase(static_cast<ghidra::int4>(description.size == 0 ? 1 : description.size),
                                 description.signed_value ? ghidra::TYPE_INT : ghidra::TYPE_UINT)
                : resolve_provider_type(context, types, description.element_type, cache, pointer_size, declarations);
        if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
            if (existing->getTypedef() != underlying || existing->getSize() != underlying->getSize()) {
                throw std::invalid_argument("Provider typedef conflicts with an existing type: " + effective_name);
            }
            cache.emplace(name, existing);
            return apply_type_display_format(types, existing, description.display_format);
        }
        ghidra::Datatype* result = types->getTypedef(underlying, effective_name, 0, 0);
        cache.emplace(name, result);
        return apply_type_display_format(types, result, description.display_format);
    }
    const ghidra::type_metatype metatype = description.kind == TypeKind::boolean            ? ghidra::TYPE_BOOL
                                           : description.kind == TypeKind::floating_point   ? ghidra::TYPE_FLOAT
                                           : description.kind == TypeKind::unsigned_integer ? ghidra::TYPE_UINT
                                           : description.kind == TypeKind::signed_integer && !description.signed_value
                                               ? ghidra::TYPE_UINT
                                               : ghidra::TYPE_INT;
    if (description.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
        throw std::invalid_argument("Provider type has an invalid size: " + effective_name);
    }
    const ghidra::int4 size = static_cast<ghidra::int4>(description.size == 0 ? 1 : description.size);
    if (ghidra::Datatype* existing = types->findByName(effective_name); existing != nullptr) {
        if (existing->getMetatype() != metatype || existing->getSize() != size) {
            throw std::invalid_argument("Provider base type conflicts with an existing type: " + effective_name);
        }
        cache.emplace(name, existing);
        return apply_type_display_format(types, existing, description.display_format);
    }
    ghidra::Datatype* result = types->getBase(size, metatype, effective_name);
    cache.emplace(name, result);
    return apply_type_display_format(types, result, description.display_format);
}

/// Resolves a provider calling-convention name to a native model. The
/// architecture default is used for the explicit `default` value; unknown
/// names are cloned as native unknown models so the requested convention is
/// still retained in generated declarations.
static ghidra::ProtoModel* resolve_provider_model(ghidra::Architecture* architecture, std::string_view convention) {
    if (convention.empty() || convention == "default") {
        return architecture->defaultfp;
    }
    if (ghidra::ProtoModel* model = architecture->getModel(std::string(convention)); model != nullptr) {
        return model;
    }
    return architecture->createUnknownModel(std::string(convention));
}

/// Applies provider types, the requested calling convention, and exact
/// parameter/return storage without asking the bootstrap ABI to reassign any
/// explicitly supplied locations. The model fills omitted locations, while
/// provider locations override those entries exactly.
/// Storage installation follows `fspec.cc`'s FuncProto::setPieces/setParam
/// contract so void returns without explicit storage remain valid native
/// prototypes.
static void apply_provider_prototype(ghidra::Architecture* architecture, ghidra::Funcdata* data,
                                     const std::string& function_name, const PrototypeDescription& prototype,
                                     const ProviderContext& context, std::uint32_t pointer_size,
                                     std::map<std::string, ghidra::Datatype*>& type_cache,
                                     std::vector<std::string>* declarations) {
    ghidra::PrototypePieces pieces{};
    pieces.model = resolve_provider_model(architecture, prototype.calling_convention);
    pieces.name = function_name;
    pieces.outtype = resolve_provider_type(context, architecture->types, prototype.return_type, type_cache,
                                           pointer_size, declarations);
    pieces.firstVarArgSlot = -1;
    if (prototype.hidden_return_storage) {
        if (prototype.return_type.empty() || prototype.return_type == "void") {
            throw std::invalid_argument("A hidden return storage requires a non-void provider return type");
        }
        const std::string hidden_type_name = prototype.return_type + " *";
        ghidra::Datatype* hidden_type = architecture->types->findByName(hidden_type_name);
        if (hidden_type == nullptr) {
            const ghidra::uint4 word_size = architecture->getDefaultDataSpace()->getWordSize();
            hidden_type = architecture->types->getTypePointer(static_cast<ghidra::int4>(pointer_size), pieces.outtype,
                                                              word_size, hidden_type_name);
        }
        pieces.innames.push_back("rethidden");
        pieces.intypes.push_back(hidden_type);
    }
    for (const PrototypeParameterDescription& parameter : prototype.parameters) {
        pieces.innames.push_back(parameter.name);
        pieces.intypes.push_back(resolve_provider_type(context, architecture->types, parameter.type_name, type_cache,
                                                       pointer_size, declarations));
    }

    ghidra::FuncProto& function_prototype = data->getFuncProto();
    const bool has_ordered_storage =
        !prototype.return_storage_pieces.empty() ||
        std::any_of(prototype.parameters.begin(), prototype.parameters.end(),
                    [](const PrototypeParameterDescription& parameter) { return !parameter.storage_pieces.empty(); });
    const std::vector<Storage> return_storage = provider_storage_sequence(
        prototype.return_storage, prototype.return_storage_pieces, "Prototype return storage");
    if (has_ordered_storage && !return_storage.empty()) {
        // A custom storage vector lets FuncProto install one logical parameter
        // over a native join address without allowing the bootstrap model to
        // reinterpret an explicitly supplied ABI. This follows `fspec.cc`'s
        // setCustomPieces path and the original JoinRecord contract.
        std::vector<ghidra::ParameterPieces> storage;
        storage.reserve(prototype.parameters.size() + 2U);
        storage.push_back(make_provider_storage(architecture, pieces.outtype, return_storage,
                                                ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::sizelock,
                                                "Prototype return storage"));
        if (prototype.hidden_return_storage) {
            const std::vector<Storage> hidden_storage =
                provider_storage_sequence(prototype.hidden_return_storage, {}, "Prototype hidden return storage");
            storage.push_back(
                make_provider_storage(architecture, pieces.intypes.front(), hidden_storage,
                                      ghidra::ParameterPieces::hiddenretparm | ghidra::ParameterPieces::typelock |
                                          ghidra::ParameterPieces::namelock | ghidra::ParameterPieces::sizelock,
                                      "Prototype hidden return storage"));
        }
        const std::size_t parameter_type_offset = prototype.hidden_return_storage ? 1U : 0U;
        for (std::size_t index = 0; index < prototype.parameters.size(); ++index) {
            const PrototypeParameterDescription& parameter = prototype.parameters[index];
            const std::vector<Storage> parameter_storage =
                provider_storage_sequence(parameter.storage, parameter.storage_pieces, "Prototype parameter storage");
            if (parameter_storage.empty()) {
                throw ghidra::BadDataError("Ordered provider prototype requires storage for every parameter");
            }
            storage.push_back(
                make_provider_storage(architecture, pieces.intypes[index + parameter_type_offset], parameter_storage,
                                      ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::namelock |
                                          ghidra::ParameterPieces::sizelock,
                                      "Prototype parameter storage"));
        }
        function_prototype.setCustomPieces(pieces, storage);
    } else if (has_ordered_storage) {
        // Without an explicit output location, keep the native output intact
        // and override only explicitly supplied input locations. This covers
        // void functions and non-void prototypes whose ABI model supplies the
        // return location.
        function_prototype.setCustomStorage(true);
        function_prototype.setPieces(pieces);
        if (prototype.hidden_return_storage) {
            const std::vector<Storage> hidden_storage =
                provider_storage_sequence(prototype.hidden_return_storage, {}, "Prototype hidden return storage");
            ghidra::ParameterPieces hidden =
                make_provider_storage(architecture, pieces.intypes.front(), hidden_storage,
                                      ghidra::ParameterPieces::hiddenretparm | ghidra::ParameterPieces::typelock |
                                          ghidra::ParameterPieces::namelock | ghidra::ParameterPieces::sizelock,
                                      "Prototype hidden return storage");
            function_prototype.setParam(0, "rethidden", hidden);
        }
        const std::size_t parameter_type_offset = prototype.hidden_return_storage ? 1U : 0U;
        for (std::size_t index = 0; index < prototype.parameters.size(); ++index) {
            const PrototypeParameterDescription& parameter = prototype.parameters[index];
            const std::vector<Storage> parameter_storage =
                provider_storage_sequence(parameter.storage, parameter.storage_pieces, "Prototype parameter storage");
            if (parameter_storage.empty()) {
                throw ghidra::BadDataError("Ordered provider prototype requires storage for every parameter");
            }
            ghidra::ParameterPieces native_parameter =
                make_provider_storage(architecture, pieces.intypes[index + parameter_type_offset], parameter_storage,
                                      ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::namelock |
                                          ghidra::ParameterPieces::sizelock,
                                      "Prototype parameter storage");
            function_prototype.setParam(static_cast<ghidra::int4>(index + parameter_type_offset), parameter.name,
                                        native_parameter);
        }
    } else {
        function_prototype.setCustomStorage(true);
        function_prototype.setPieces(pieces);
        if (prototype.return_storage) {
            validate_storage(*prototype.return_storage, "Prototype return storage");
            ghidra::AddrSpace* output_space = architecture->getSpaceByName(prototype.return_storage->space.name());
            if (output_space == nullptr) {
                throw std::runtime_error("Prototype references an unknown return storage space: " +
                                         prototype.return_storage->space.name());
            }
            validate_space_range(*prototype.return_storage, output_space, "Prototype return storage");
            ghidra::ParameterPieces output{};
            output.type = pieces.outtype;
            output.flags = ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::sizelock;
            output.addr = ghidra::Address(output_space, prototype.return_storage->offset);
            function_prototype.setOutput(output);
        }
        if (prototype.hidden_return_storage) {
            validate_storage(*prototype.hidden_return_storage, "Prototype hidden return storage");
            ghidra::AddrSpace* hidden_space =
                architecture->getSpaceByName(prototype.hidden_return_storage->space.name());
            if (hidden_space == nullptr) {
                throw std::runtime_error("Prototype references an unknown hidden return storage space: " +
                                         prototype.hidden_return_storage->space.name());
            }
            validate_space_range(*prototype.hidden_return_storage, hidden_space, "Prototype hidden return storage");
            ghidra::ParameterPieces hidden{};
            hidden.type = pieces.intypes.front();
            hidden.flags = ghidra::ParameterPieces::hiddenretparm | ghidra::ParameterPieces::typelock |
                           ghidra::ParameterPieces::namelock | ghidra::ParameterPieces::sizelock;
            hidden.addr = ghidra::Address(hidden_space, prototype.hidden_return_storage->offset);
            function_prototype.setParam(0, "rethidden", hidden);
        }
        const std::size_t parameter_index_offset = prototype.hidden_return_storage ? 1U : 0U;
        for (std::size_t index = 0; index < prototype.parameters.size(); ++index) {
            const PrototypeParameterDescription& parameter = prototype.parameters[index];
            if (!parameter.storage) {
                continue;
            }
            ghidra::ParameterPieces parameter_storage{};
            parameter_storage.type = pieces.intypes[index + parameter_index_offset];
            parameter_storage.flags = ghidra::ParameterPieces::typelock | ghidra::ParameterPieces::namelock |
                                      ghidra::ParameterPieces::sizelock;
            validate_storage(*parameter.storage, "Prototype parameter storage");
            ghidra::AddrSpace* parameter_space = architecture->getSpaceByName(parameter.storage->space.name());
            if (parameter_space == nullptr) {
                throw std::runtime_error("Prototype references an unknown parameter storage space: " +
                                         parameter.storage->space.name());
            }
            validate_space_range(*parameter.storage, parameter_space, "Prototype parameter storage");
            parameter_storage.addr = ghidra::Address(parameter_space, parameter.storage->offset);
            function_prototype.setParam(static_cast<ghidra::int4>(index + parameter_index_offset), parameter.name,
                                        parameter_storage);
        }
    }
    function_prototype.setNoReturn(prototype.no_return);
    function_prototype.setInline(prototype.inline_function);
    if (prototype.call_fixup) {
        const ghidra::int4 injection_id =
            architecture->pcodeinjectlib->getPayloadId(ghidra::InjectPayload::CALLFIXUP_TYPE, *prototype.call_fixup);
        if (injection_id < 0) {
            throw std::invalid_argument("Unknown provider call-fixup: " + *prototype.call_fixup);
        }
        function_prototype.setInjectId(injection_id);
    }
    function_prototype.clearProviderErrors();
}

/// Applies provider flow records before the native `Funcdata::followFlow` pass.
static void apply_flow_description(ghidra::Architecture* architecture, ghidra::Funcdata* data,
                                   const FlowDescription& description) {
    ghidra::AddrSpace* code_space = architecture->getDefaultCodeSpace();
    for (const IndirectCallTargetDescription& target : description.indirect_call_targets) {
        data->getOverride().insertDeindirect(ghidra::Address(code_space, target.call_address),
                                             ghidra::Address(code_space, target.target_address));
    }
    for (const JumpTableDescription& jump_table : description.jump_tables) {
        if (jump_table.target_addresses.empty()) {
            throw std::invalid_argument("Provider jump-table override must contain at least one target");
        }
        std::vector<ghidra::Address> targets;
        targets.reserve(jump_table.target_addresses.size());
        for (const std::uint64_t target : jump_table.target_addresses) {
            targets.emplace_back(code_space, target);
        }
        ghidra::JumpTable* native_table =
            data->installJumpTable(ghidra::Address(code_space, jump_table.branch_address));
        const ghidra::Address normalized = jump_table.normalized_switch_address
                                               ? ghidra::Address(code_space, *jump_table.normalized_switch_address)
                                               : ghidra::Address();
        native_table->setOverride(targets, normalized, jump_table.normalized_switch_hash, jump_table.starting_value);
    }
    for (const FlowOverrideDescription& override_description : description.flow_overrides) {
        if (override_description.type.empty()) {
            throw std::invalid_argument("Provider flow override type must not be empty");
        }
        data->getOverride().insertFlowOverride(ghidra::Address(code_space, override_description.address),
                                               override_description.type);
    }
    for (const DestinationOverrideDescription& override_description : description.destination_overrides) {
        if (override_description.type.empty()) {
            throw std::invalid_argument("Provider destination override type must not be empty");
        }
        data->getOverride().insertDestinationOverride(ghidra::Address(code_space, override_description.address),
                                                      ghidra::Address(code_space, override_description.target_address),
                                                      override_description.type);
    }
    for (const ForceGotoDescription& force_goto : description.force_gotos) {
        data->getOverride().insertForceGoto(ghidra::Address(code_space, force_goto.address),
                                            ghidra::Address(code_space, force_goto.target_address));
    }
}

} // namespace detail

/// Owns the provider and native architecture for a decompilation session.
class Decompiler::State {
public:
    /// Constructs one standalone native architecture.
    State(ArchitectureDescription description, ProviderContext context)
        : description(std::move(description)), context(std::move(context)), provider(this->context.pcode),
          memory(this->context.memory),
          architecture(std::make_unique<detail::ProviderArchitecture>(this->description, this->provider, this->memory,
                                                                      this->context.injections,
                                                                      this->context.analysis_options)) {}

    ArchitectureDescription description;
    ProviderContext context;
    std::shared_ptr<PcodeProvider> provider;
    std::shared_ptr<MemoryProvider> memory;
    std::map<std::string, ghidra::Datatype*> type_cache;
    std::vector<std::string> type_declarations;
    std::unique_ptr<detail::ProviderArchitecture> architecture;
};

/// Owns the stateful Sleigh decoder and its conversion context.
class SleighPcodeProvider::State {
public:
    /// Loads a compiled SLA and stores decoder inputs.
    State(std::filesystem::path path, std::shared_ptr<MemoryProvider> memory,
          std::vector<std::pair<std::string, std::uint64_t>> context)
        : decoder(std::move(path)), memory(std::move(memory)) {
        for (const auto& value : context) {
            processor_context.values.push_back({value.first, value.second});
        }
    }

    sleigh_runtime::Decoder decoder;
    std::shared_ptr<MemoryProvider> memory;
    sleigh_runtime::ProcessorContext processor_context;
};

/// Loads a compiled SLA and stores its provider-backed decoder state.
SleighPcodeProvider::SleighPcodeProvider(std::filesystem::path path, std::shared_ptr<MemoryProvider> memory,
                                         std::vector<std::pair<std::string, std::uint64_t>> context)
    : state_(std::make_unique<State>(std::move(path), std::move(memory), std::move(context))) {}

/// Releases the opaque decoder state.
SleighPcodeProvider::~SleighPcodeProvider() = default;

/// Transfers ownership of decoder state.
SleighPcodeProvider::SleighPcodeProvider(SleighPcodeProvider&&) noexcept = default;

/// Transfers ownership of decoder state.
SleighPcodeProvider& SleighPcodeProvider::operator=(SleighPcodeProvider&&) noexcept = default;

/// Decodes one instruction through the existing public Sleigh runtime.
std::expected<Instruction, ProviderError> SleighPcodeProvider::decode(std::uint64_t address) const {
    if (!state_->memory) {
        return std::unexpected(ProviderError{"SleighPcodeProvider requires a memory provider"});
    }
    std::expected<std::vector<std::uint8_t>, ProviderError> bytes =
        std::unexpected(ProviderError{"No mapped bytes are available"});
    ProviderError last_error{"No mapped bytes are available"};
    for (std::size_t window = 16; window != 0; --window) {
        const auto attempt = state_->memory->read(address, window);
        if (attempt) {
            bytes = *attempt;
            break;
        }
        last_error = attempt.error();
    }
    if (!bytes) {
        return std::unexpected(last_error);
    }
    const auto decoded = state_->decoder.decode(address, *bytes, state_->processor_context);
    if (!decoded) {
        return std::unexpected(ProviderError{decoded.error().message});
    }
    // Sleigh already returns the canonical decode snapshot. Normalize only the
    // native selector encoding while keeping the same domain value types.
    Instruction result = *decoded;
    for (PcodeOperation& operation : result.pcode) {
        operation = detail::normalize_sleigh_operation(operation);
    }
    if (result.length == 0 || result.length > bytes->size()) {
        return std::unexpected(ProviderError{"Sleigh decoder returned an instruction longer than its mapped window"});
    }
    try {
        detail::validate_instruction(result, address);
    } catch (const ghidra::LowlevelError& error) {
        return std::unexpected(ProviderError{error.explain});
    } catch (const std::exception& error) {
        return std::unexpected(ProviderError{error.what()});
    }
    return result;
}

/// Constructs the provider-backed native decompiler.
Decompiler::Decompiler(ArchitectureDescription description, std::shared_ptr<PcodeProvider> provider,
                       std::shared_ptr<MemoryProvider> memory) {
    ghidra::forcePrintCLanguageRegistration();
    static std::once_flag capability_initialization;
    std::call_once(capability_initialization, [] { ghidra::CapabilityPoint::initializeAll(); });
    ProviderContext context;
    context.pcode = std::move(provider);
    context.memory = std::move(memory);
    state_ = std::make_unique<State>(std::move(description), std::move(context));
}

/// Constructs the provider-backed native decompiler with all external services.
Decompiler::Decompiler(ArchitectureDescription description, ProviderContext context) {
    ghidra::forcePrintCLanguageRegistration();
    static std::once_flag capability_initialization;
    std::call_once(capability_initialization, [] { ghidra::CapabilityPoint::initializeAll(); });
    if (!context.pcode) {
        throw std::invalid_argument("A p-code provider is required");
    }
    state_ = std::make_unique<State>(std::move(description), std::move(context));
}

/// Releases the opaque native architecture state.
Decompiler::~Decompiler() = default;

/// Transfers native ownership state.
Decompiler::Decompiler(Decompiler&&) noexcept = default;

/// Transfers native ownership state.
Decompiler& Decompiler::operator=(Decompiler&&) noexcept = default;

/// Runs the original Funcdata pipeline and captures its diagnostic representations.
DecompilationResult Decompiler::decompile(const FunctionDescription& function) const {
    if (function.end <= function.entry) {
        throw std::invalid_argument("FunctionDescription requires an end address greater than entry");
    }
    DecompilationResult result;
    const auto decode_body = [&](const FunctionDescription& body) {
        if (body.end <= body.entry) {
            throw std::invalid_argument("FunctionProvider returned an invalid function range");
        }
        std::vector<Instruction> instructions;
        for (std::uint64_t address = body.entry; address < body.end;) {
            const auto decoded = state_->provider->decode(address);
            if (!decoded) {
                throw std::runtime_error(decoded.error().message);
            }
            try {
                detail::validate_instruction(*decoded, address);
            } catch (const ghidra::LowlevelError& error) {
                throw std::runtime_error(error.explain);
            }
            if (decoded->length > body.end - address) {
                throw std::runtime_error("Provider returned an instruction outside the function range");
            }
            instructions.push_back(*decoded);
            address += decoded->length;
        }
        return instructions;
    };

    std::vector<FunctionDescription> bodies{function};
    if (state_->context.functions) {
        for (const FunctionDescription& supplied : state_->context.functions->functions()) {
            if (supplied.entry == function.entry) {
                if (supplied.end != function.end) {
                    throw std::invalid_argument("FunctionProvider conflicts with the requested root body");
                }
                continue;
            }
            bodies.push_back(supplied);
        }
    }
    std::map<std::uint64_t, std::vector<Instruction>> decoded_bodies;
    for (const FunctionDescription& body : bodies) {
        if (!decoded_bodies.emplace(body.entry, decode_body(body)).second) {
            throw std::invalid_argument("FunctionProvider returned duplicate function entries");
        }
    }
    result.raw_instructions = decoded_bodies.at(function.entry);

    ghidra::AddrSpace* code_space = state_->architecture->getDefaultCodeSpace();
    const ghidra::Address entry(code_space, function.entry);
    auto lookup_symbol = [&](std::uint64_t address) -> std::optional<SymbolDescription> {
        if (!state_->context.symbols) {
            return std::nullopt;
        }
        return state_->context.symbols->symbol_at(address);
    };
    std::vector<SymbolDescription> enumerated_symbols;
    if (state_->context.symbols) {
        for (const SymbolDescription& symbol : state_->context.symbols->symbols()) {
            enumerated_symbols.push_back(symbol);
        }
    }
    const auto find_enumerated_symbol = [&](std::uint64_t address, SymbolKind kind) -> const SymbolDescription* {
        const auto found =
            std::find_if(enumerated_symbols.begin(), enumerated_symbols.end(), [&](const SymbolDescription& candidate) {
                return candidate.address == address && candidate.kind == kind;
            });
        return found == enumerated_symbols.end() ? nullptr : &*found;
    };
    auto add_function_symbol = [&](std::uint64_t address, const std::string& name, const std::string& namespace_name) {
        if (ghidra::Funcdata* existing = state_->architecture->symboltab->getGlobalScope()->queryFunction(
                ghidra::Address(code_space, address))) {
            // Native flow may create a fallback function shell before the
            // provider symbol inventory is applied.  Preserve the shell's
            // analysis state but replace its generated name with the
            // provider-owned symbol, matching the original external-symbol
            // resolution contract used by the decompiler datatests.
            if (!name.empty() && existing->getSymbol()->getName() != name) {
                existing->getSymbol()->setProviderInfo(name, existing->getSymbol()->getType());
            }
            return existing->getSymbol();
        }
        ghidra::Scope* scope = state_->architecture->symboltab->getGlobalScope();
        std::string basename = name;
        if (!namespace_name.empty()) {
            scope = state_->architecture->symboltab->findCreateScopeFromSymbolName(namespace_name + "::" + name,
                                                                                   basename, scope);
        }
        ghidra::FunctionSymbol* created = scope->addFunction(ghidra::Address(code_space, address), basename);
        if (!name.empty() && created->getName() != name) {
            // Native symbols normalize names containing C++ scopes; retain the
            // provider spelling so direct and indirect calls print identically.
            created->setProviderInfo(name, created->getType());
        }
        if (!namespace_name.empty()) {
            // Match `database_ghidra.cc`, `ScopeGhidraNamespace::addMapInternal`:
            // namespace-owned function mappings participate in address lookup.
            state_->architecture->symboltab->addRange(scope, code_space, address, address);
        }
        return created;
    };
    std::map<std::uint64_t, ghidra::Symbol*> provider_data_symbols;
    const auto add_data_symbol = [&](const SymbolDescription& symbol) {
        if (symbol.kind != SymbolKind::data) {
            return;
        }
        if (symbol.name.empty()) {
            throw std::invalid_argument("Provider data symbol has an empty name");
        }
        // Scope::addSymbol maps the object in the scope that owns its name.
        // Use the provider-selected space, falling back to the resolved native
        // data space when older metadata omitted a name.
        // Original namespace and mapping behavior: `database.cc`,
        // `Database::findCreateScopeFromSymbolName` and `Scope::addSymbol`.
        ghidra::AddrSpace* data_space = symbol.space.empty() ? state_->architecture->getDefaultDataSpace()
                                                             : state_->architecture->getSpaceByName(symbol.space);
        if (data_space == nullptr) {
            throw std::runtime_error("Provider data symbol references an unknown space: " + symbol.space);
        }
        ghidra::Datatype* type = nullptr;
        if (!symbol.type_name.empty()) {
            type = detail::resolve_provider_type(state_->context, state_->architecture->types, symbol.type_name,
                                                 state_->type_cache, state_->description.pointer_size,
                                                 &state_->type_declarations);
        } else {
            if (symbol.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
                throw std::invalid_argument("Provider data symbol has an invalid size: " + symbol.name);
            }
            const ghidra::int4 size = static_cast<ghidra::int4>(symbol.size == 0 ? 1 : symbol.size);
            type = state_->architecture->types->getBase(size, ghidra::TYPE_UNKNOWN);
        }
        if (type == nullptr || type->getSize() <= 0 ||
            (symbol.size != 0 && type->getSize() != static_cast<ghidra::int4>(symbol.size))) {
            throw std::invalid_argument("Provider data symbol size does not match its type: " + symbol.name);
        }
        if (symbol.address > data_space->getHighest() ||
            static_cast<std::uint64_t>(type->getSize()) - 1U > data_space->getHighest() - symbol.address) {
            throw std::invalid_argument("Provider data symbol exceeds its address space: " + symbol.name);
        }
        ghidra::Scope* scope = state_->architecture->symboltab->getGlobalScope();
        std::string basename = symbol.name;
        if (!symbol.namespace_name.empty()) {
            scope = state_->architecture->symboltab->findCreateScopeFromSymbolName(
                symbol.namespace_name + "::" + symbol.name, basename, scope);
        }
        const std::uint64_t identity = symbol.alias_identity != 0 ? symbol.alias_identity : symbol.identity;
        ghidra::MapEntry* mapping = nullptr;
        const auto existing_identity = provider_data_symbols.find(identity);
        if (identity != 0 && existing_identity != provider_data_symbols.end()) {
            mapping = scope->addMapPoint(existing_identity->second, ghidra::Address(data_space, symbol.address),
                                         ghidra::Address());
        } else {
            mapping = scope->addSymbol(basename, type, ghidra::Address(data_space, symbol.address), ghidra::Address());
            if (mapping != nullptr && identity != 0) {
                provider_data_symbols.emplace(identity, mapping->getSymbol());
            }
        }
        if (mapping == nullptr) {
            throw std::runtime_error("Native symbol database rejected provider data symbol: " + symbol.name);
        }
        if (!symbol.namespace_name.empty()) {
            // Native Ghidra's `ScopeGhidraNamespace::addMapInternal` adds the
            // mapped range to a namespace so address-based symbol resolution
            // can discover it without flattening the qualified name.
            state_->architecture->symboltab->addRange(
                scope, data_space, symbol.address, symbol.address + static_cast<std::uint64_t>(type->getSize()) - 1U);
        }
        ghidra::Symbol* native_symbol = mapping->getSymbol();
        if (identity != 0) {
            native_symbol->setProviderInfo(native_symbol->getName(), type, identity);
        }
        if (symbol.read_only) {
            scope->setAttribute(native_symbol, ghidra::Varnode::readonly);
        }
        if (symbol.display_format != DisplayFormat::none) {
            scope->setDisplayFormat(native_symbol, detail::native_display_format(symbol.display_format));
        }
    };
    for (const SymbolDescription& symbol : enumerated_symbols) {
        add_data_symbol(symbol);
    }
    std::map<std::uint64_t, ghidra::FunctionSymbol*> native_functions;
    // The original Architecture symbol-loading path installs every function
    // returned by LoadImage::getNextSymbol before flow recovery. Do the same
    // for the provider inventory, including functions not reached by the root
    // body; see `architecture.cc`, `Architecture::readLoaderSymbols` and
    // `database.cc`, `Scope::addFunction`.
    for (const SymbolDescription& symbol : enumerated_symbols) {
        if (symbol.kind != SymbolKind::function) {
            continue;
        }
        const std::uint64_t address = symbol.address;
        const std::string function_name = symbol.name.empty() ? "FUN_" + std::to_string(address) : symbol.name;
        native_functions[address] = add_function_symbol(address, function_name, symbol.namespace_name);
    }
    auto install_external_function = [&](std::uint64_t address, const std::string& fallback_name) {
        std::string external_name = fallback_name;
        std::string external_namespace;
        const SymbolDescription* enumerated = find_enumerated_symbol(address, SymbolKind::function);
        if (enumerated != nullptr && !enumerated->name.empty()) {
            external_name = enumerated->name;
            external_namespace = enumerated->namespace_name;
        } else if (const std::optional<SymbolDescription> symbol = lookup_symbol(address);
                   symbol && symbol->kind == SymbolKind::function && !symbol->name.empty()) {
            external_name = symbol->name;
            external_namespace = symbol->namespace_name;
        }
        ghidra::FunctionSymbol* external_symbol = add_function_symbol(address, external_name, external_namespace);
        native_functions[address] = external_symbol;
        return external_symbol;
    };
    for (const auto& [address, body] : decoded_bodies) {
        const FunctionDescription* supplied = nullptr;
        for (const FunctionDescription& candidate : bodies) {
            if (candidate.entry == address) {
                supplied = &candidate;
                break;
            }
        }
        std::string name = supplied == nullptr ? "FUN_" + std::to_string(address) : supplied->name;
        std::string namespace_name;
        if (const SymbolDescription* enumerated = find_enumerated_symbol(address, SymbolKind::function);
            enumerated != nullptr) {
            name = enumerated->name.empty() ? name : enumerated->name;
            namespace_name = enumerated->namespace_name;
        } else if (const std::optional<SymbolDescription> symbol = lookup_symbol(address);
                   symbol && symbol->kind == SymbolKind::function) {
            name = symbol->name.empty() ? name : symbol->name;
            namespace_name = symbol->namespace_name;
        }
        native_functions[address] = add_function_symbol(address, name, namespace_name);
    }
    for (const auto& [address, body] : decoded_bodies) {
        for (const Instruction& instruction : body) {
            for (const PcodeOperation& operation : instruction.pcode) {
                if ((detail::opcode_value(operation.opcode) != static_cast<std::uint32_t>(ghidra::CPUI_CALL) &&
                     detail::opcode_value(operation.opcode) != static_cast<std::uint32_t>(ghidra::CPUI_CALLIND)) ||
                    operation.inputs.empty()) {
                    continue;
                }
                if (detail::opcode_value(operation.opcode) == static_cast<std::uint32_t>(ghidra::CPUI_CALLIND) &&
                    operation.inputs.front().space.name() != "const" &&
                    operation.inputs.front().space.name() != state_->description.code_space) {
                    continue;
                }
                const std::uint64_t target = operation.inputs.front().offset;
                if (native_functions.find(target) == native_functions.end()) {
                    install_external_function(target, "FUN_" + std::to_string(target));
                }
            }
        }
    }
    for (const FunctionDescription& body : bodies) {
        if (!state_->context.flow) {
            continue;
        }
        const std::optional<FlowDescription> flow = state_->context.flow->flow_at(body.entry);
        if (!flow) {
            continue;
        }
        for (const IndirectCallTargetDescription& target : flow->indirect_call_targets) {
            if (native_functions.find(target.target_address) == native_functions.end()) {
                install_external_function(target.target_address, "FUN_" + std::to_string(target.target_address));
            }
        }
    }
    for (const auto& [address, symbol] : native_functions) {
        ghidra::Funcdata* native_data = symbol->getFunction();
        std::string native_name = symbol->getName();
        if (state_->context.prototypes) {
            const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(address);
            if (prototype) {
                detail::apply_provider_prototype(state_->architecture.get(), native_data, native_name, *prototype,
                                                 state_->context, state_->description.pointer_size, state_->type_cache,
                                                 &state_->type_declarations);
            }
        }
        if (state_->context.flow) {
            const std::optional<FlowDescription> flow = state_->context.flow->flow_at(address);
            if (flow) {
                detail::apply_flow_description(state_->architecture.get(), native_data, *flow);
            }
        }
    }
    ghidra::FunctionSymbol* symbol = native_functions.at(function.entry);
    ghidra::Funcdata* data = symbol->getFunction();
    const std::string function_name = symbol->getName();

    if (state_->context.comments) {
        for (const Instruction& instruction : result.raw_instructions) {
            const std::optional<std::string> comment = state_->context.comments->comment_at(instruction.address);
            if (comment) {
                state_->architecture->commentdb->addComment(ghidra::Comment::user1, entry,
                                                            ghidra::Address(code_space, instruction.address), *comment);
            }
        }
    }
    for (const FunctionDescription& body : bodies) {
        if (body.entry == function.entry) {
            continue;
        }
        native_functions.at(body.entry)
            ->getFunction()
            ->followFlow(ghidra::Address(code_space, body.entry), ghidra::Address(code_space, body.end));
    }
    data->followFlow(entry, ghidra::Address(code_space, function.end));

    if (state_->context.variables) {
        for (const VariableDescription& variable : state_->context.variables->variables_at(function.entry)) {
            ghidra::AddrSpace* variable_space = state_->architecture->getSpaceByName(variable.storage.space.name());
            if (variable_space == nullptr) {
                throw std::runtime_error("Variable references an unknown storage space: " +
                                         variable.storage.space.name());
            }
            ghidra::Address variable_address(variable_space, variable.storage.offset);
            ghidra::Datatype* variable_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, variable.type_name, state_->type_cache,
                state_->description.pointer_size, &state_->type_declarations);
            data->getScopeLocal()->addTypeRecommendation(variable_address, variable_type);
            data->getScopeLocal()->addNameRecommendation(variable_address, entry, variable_type->getSize(),
                                                         variable.name);
        }
        data->getScopeLocal()->applyTypeRecommendations();
    }

    std::ostringstream raw;
    data->printRaw(raw);
    result.raw_pcode = raw.str();

    ghidra::Action* action = state_->architecture->allacts.getCurrent();
    std::string previous_action;
    if (function.generate_signature) {
        // Ported from Ghidra/Features/Decompiler/src/decompile/cpp/signature_ghidra.cc:
        // SignaturesAt runs only normalize before GraphSigManager. Capture the
        // BSim result at that boundary, then restore the caller's action and
        // finish the ordinary decompiler pipeline for the other artifacts.
        previous_action = state_->architecture->allacts.getCurrentName();
        ghidra::Action* normalize = state_->architecture->allacts.setCurrent("normalize");
        if (normalize == nullptr) {
            throw ghidra::LowlevelError("Unable to select the normalize action for signature generation");
        }
        try {
            normalize->reset(*data);
            normalize->perform(*data);
            ghidra::SigManager::setSettings(function.signature_settings);
            ghidra::GraphSigManager signature_manager;
            signature_manager.setMaxIteration(function.signature_max_iterations);
            signature_manager.setMaxBlockIteration(function.signature_max_block_iterations);
            signature_manager.setMaxVarnode(static_cast<ghidra::int4>(function.signature_max_varnodes));
            signature_manager.setCurrentFunction(data);
            signature_manager.generate();
            signature_manager.getSignatureVector(result.signature_features);
            result.signature_overall_hash = signature_manager.getOverallHash();
            result.signature_has_unimplemented = data->hasUnimplemented();
            result.signature_has_bad_data = data->hasBadData();
            for (ghidra::uint4 index = 0; index < data->numCalls(); ++index) {
                const ghidra::Address& call_address = data->getCallSpecs(index)->getEntryAddress();
                if (!call_address.isInvalid())
                    result.signature_call_addresses.push_back(call_address.getOffset());
            }
        } catch (...) {
            state_->architecture->allacts.setCurrent(previous_action);
            throw;
        }
        state_->architecture->allacts.setCurrent(previous_action);
        // Normalize marks the Funcdata as processed. Rebuild the root graph so
        // the normal decompiler action can still produce C output without
        // contaminating the already-captured signature snapshot.
        data->clear();
        data->followFlow(entry, ghidra::Address(code_space, function.end));
    }
    if (action != nullptr) {
        action->reset(*data);
        action->perform(*data);
    }
    // Install provider-owned dynamic constant formats after native action
    // analysis and before C printing. This is the same lifecycle point used by
    // the original `force varnode` and `map convert` interface commands: the
    // p-code use has a stable dynamic hash, while the printer has not yet
    // emitted the function.
    for (const ConstantFormatDescription& format : state_->context.constant_formats) {
        if (format.format == DisplayFormat::none) {
            throw std::invalid_argument("Provider constant format must select an integer representation");
        }
        if (format.size > static_cast<std::uint32_t>(std::numeric_limits<ghidra::int4>::max())) {
            throw std::invalid_argument("Provider constant format has an invalid encoded width");
        }
        const std::uint32_t native_format = detail::native_display_format(format.format);
        if (format.pcode_address > code_space->getHighest()) {
            throw std::invalid_argument("Provider constant format address exceeds the code space");
        }
        bool matched = false;
        for (auto variable = data->beginLoc(); variable != data->endLoc() && !matched; ++variable) {
            ghidra::Varnode* varnode = *variable;
            // A value can occur at the same p-code address with different
            // widths after Sleigh construction. Preserve the provider's
            // optional size discriminator before attaching a dynamic symbol.
            if ((format.size != 0 && varnode->getSize() != format.size) || !varnode->constantMatch(format.value)) {
                continue;
            }
            for (auto use = varnode->beginDescend(); use != varnode->endDescend(); ++use) {
                if ((*use)->getAddr() != ghidra::Address(code_space, format.pcode_address)) {
                    continue;
                }
                data->buildDynamicSymbol(varnode);
                ghidra::HighVariable* high = varnode->getHigh();
                ghidra::Symbol* symbol = high == nullptr ? nullptr : high->getSymbol();
                if (symbol == nullptr) {
                    throw std::runtime_error("Provider constant format could not create a dynamic symbol");
                }
                symbol->getScope()->setDisplayFormat(symbol, native_format);
                symbol->getScope()->setAttribute(symbol, ghidra::Varnode::typelock);
                matched = true;
                break;
            }
        }
        if (!matched) {
            if (format.hash == 0) {
                throw std::invalid_argument("Provider constant format does not identify a p-code use");
            }
            data->getScopeLocal()->addEquateSymbol("", native_format, format.value,
                                                   ghidra::Address(code_space, format.pcode_address), format.hash);
        }
    }
    if (state_->context.variables) {
        data->getScopeLocal()->applyTypeRecommendations();
        data->getScopeLocal()->recoverNameRecommendationsForSymbols();
        for (const VariableDescription& variable : state_->context.variables->variables_at(function.entry)) {
            ghidra::AddrSpace* variable_space = state_->architecture->getSpaceByName(variable.storage.space.name());
            if (variable_space == nullptr) {
                continue;
            }
            ghidra::Datatype* variable_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, variable.type_name, state_->type_cache,
                state_->description.pointer_size, &state_->type_declarations);
            if (ghidra::MapEntry* map = data->getScopeLocal()->findOverlap(
                    ghidra::Address(variable_space, variable.storage.offset), variable_type->getSize())) {
                map->getSymbol()->setProviderInfo(variable.name, variable_type, variable.identity);
                map->getSymbol()->setIsolated(variable.isolated);
            }
        }
    }
    if (state_->context.prototypes) {
        const std::optional<PrototypeDescription> prototype = state_->context.prototypes->prototype_at(function.entry);
        if (prototype) {
            ghidra::Datatype* return_type = detail::resolve_provider_type(
                state_->context, state_->architecture->types, prototype->return_type, state_->type_cache,
                state_->description.pointer_size, &state_->type_declarations);
            if (prototype->return_storage && data->getFuncProto().getOutput() != nullptr) {
                data->getFuncProto().getOutput()->overrideSizeLockType(return_type);
            }
        }
    }

    std::ostringstream high;
    data->printVarnodeTree(high);
    result.high_pcode = high.str();
    result.data_flow = result.high_pcode;
    std::ostringstream flow;
    data->printBlockTree(flow);
    result.control_flow = flow.str();
    std::ostringstream ast;
    data->printLocalRange(ast);
    result.ast = ast.str();
    detail::capture_native_provenance(*data, result);
    std::ostringstream c_output;
    // Provider declarations are source-level metadata rather than native
    // engine objects. Emit each supplied declaration once before the function
    // so typedefs and composite declarations are not silently discarded.
    // Recursive type resolution records aliases before their element types;
    // reverse insertion order restores dependency order for emitted C.
    for (auto declaration = state_->type_declarations.rbegin(); declaration != state_->type_declarations.rend();
         ++declaration) {
        c_output << *declaration;
        if (declaration->empty() || declaration->back() != '\n') {
            c_output << '\n';
        }
    }
    state_->architecture->print->setOutputStream(&c_output);
    state_->architecture->print->setMarkup(false);
    state_->architecture->print->setFlat(false);
    state_->architecture->print->docFunction(data);
    result.c_source = c_output.str();
    if (!function.capture_provenance) {
        return result;
    }

    // Emit the same function through the original Clang XML markup path after
    // plain output has been captured.  Use a fresh native printer so the
    // existing architecture-owned printer retains its established lifecycle;
    // `EmitMarkup` writes the authoritative `varref`/`opref` edges and no
    // C-text parsing is involved in this snapshot.
    std::ostringstream markup_output;
    std::unique_ptr<ghidra::PrintLanguage> markup_print(
        ghidra::PrintLanguageCapability::getDefault()->buildLanguage(state_->architecture.get()));
    markup_print->initializeFromArchitecture();
    markup_print->adjustTypeOperators();
    markup_print->setOutputStream(&markup_output);
    markup_print->setMarkup(true);
    markup_print->setPackedOutput(false);
    markup_print->setFlat(false);
    markup_print->docFunction(data);
    result.clang_markup = markup_output.str();
    detail::capture_structured_clang_provenance(result);
    return result;
}

} // namespace recode::decompiler
