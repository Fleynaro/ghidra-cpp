export module analyzer_aggressive_instruction_finder;

import analyzer;
import std;

/// Owns the Aggressive Instruction Finder analyzer declaration and implementation.
export namespace recode::analyzer {
class AggressiveInstructionFinderAnalyzer final : public Analyzer {
public:
    /// Returns the late, opt-in aggressive finder contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Searches undefined executable bytes for repeated, valid function starts.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Describes the bounded pseudo-flow validation result for one candidate.
struct CandidateProof {
    std::set<Address> instructions;
    std::size_t count{};
    bool adds_information{};
    bool valid{true};
};

/// Builds the masked instruction-byte key used by the original two-instruction hash.
[[nodiscard]] std::vector<std::uint8_t> masked_bytes(const sleigh_runtime::Instruction& instruction) {
    std::vector<std::uint8_t> result = instruction.bytes;
    if (instruction.instruction_mask.size() == 1U && !result.empty()) {
        result[0] &= 0xffU;
        return result;
    }
    if (instruction.instruction_mask.size() == result.size()) {
        for (std::size_t index = 0; index < result.size(); ++index) {
            result[index] &= instruction.instruction_mask[index];
        }
    }
    return result;
}

/// Appends the masked bytes of two adjacent instructions to one hash key.
[[nodiscard]] std::optional<std::vector<std::uint8_t>> start_key(AnalysisContext& context, Address entry) {
    const auto first = context.instructions().find(entry);
    const auto first_decoded =
        first == context.instructions().end()
            ? context.decode(entry)
            : std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError>(first->second.instruction);
    if (!first_decoded || first_decoded->length == 0U ||
        first_decoded->address > std::numeric_limits<Address>::max() - first_decoded->length) {
        return std::nullopt;
    }
    const Address second_address = first_decoded->address + first_decoded->length;
    const auto second = context.instructions().find(second_address);
    const auto second_decoded =
        second == context.instructions().end()
            ? context.decode(second_address)
            : std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError>(second->second.instruction);
    if (!second_decoded || second_decoded->length == 0U) {
        return std::nullopt;
    }
    auto key = masked_bytes(*first_decoded);
    const auto suffix = masked_bytes(*second_decoded);
    key.insert(key.end(), suffix.begin(), suffix.end());
    return key;
}

/// Builds a decoder-stable fallback key when the native Sleigh provider does
/// not expose instruction masks for relative operands.
[[nodiscard]] std::optional<std::vector<std::uint8_t>> instruction_shape_key(AnalysisContext& context, Address entry) {
    const auto first = context.decode(entry);
    if (!first || first->length == 0U) {
        return std::nullopt;
    }
    const auto second = context.decode(first->address + first->length);
    if (!second || second->length == 0U) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> result;
    const auto append = [&](const sleigh_runtime::Instruction& instruction) {
        result.insert(result.end(), instruction.mnemonic.begin(), instruction.mnemonic.end());
        result.push_back(0U);
        result.push_back(instruction.length);
    };
    append(*first);
    append(*second);
    return result;
}

/// Resolves a Sleigh flow varnode into a mapped preferred virtual address.
[[nodiscard]] std::optional<Address> flow_target(const AnalysisContext& context,
                                                 const std::optional<sleigh_runtime::Varnode>& target) {
    if (!target) {
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

/// Reports whether an address is already owned by code or data.
[[nodiscard]] bool is_defined(const AnalysisContext& context, Address address) {
    const auto instruction = context.instructions().upper_bound(address);
    if (instruction != context.instructions().begin()) {
        const auto& [start, record] = *std::prev(instruction);
        if (address < start + record.instruction.length) {
            return true;
        }
    }
    const auto data = context.data().upper_bound(address);
    if (data != context.data().begin()) {
        const auto& [start, record] = *std::prev(data);
        if (address < start + record.size) {
            return true;
        }
    }
    return false;
}

/// Implements ArmAggressiveInstructionFinderAnalyzer's filler-instruction guard.
[[nodiscard]] bool is_arm_filler(const sleigh_runtime::Instruction& instruction) {
    std::string mnemonic = instruction.mnemonic;
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (mnemonic == "nop") {
        return true;
    }
    if (mnemonic != "mov" && mnemonic != "movs") {
        return false;
    }
    if (instruction.operands.size() < 2U) {
        return false;
    }
    std::string left = instruction.operands[0].text;
    std::string right = instruction.operands[1].text;
    std::transform(left.begin(), left.end(), left.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    std::transform(right.begin(), right.end(), right.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return left == right;
}

/// Returns an instruction key that is stable enough to reject repeated ARM loops.
[[nodiscard]] std::string instruction_identity(const sleigh_runtime::Instruction& instruction) {
    std::ostringstream stream;
    stream << instruction.mnemonic << ':' << instruction.assembly << ':' << instruction.length;
    return stream.str();
}

/// Pseudo-disassembles a candidate with the original 4000-instruction safety bound.
[[nodiscard]] CandidateProof prove_candidate(AnalysisContext& context, Address entry) {
    CandidateProof proof;
    std::deque<Address> work{entry};
    std::set<Address> visited;
    std::map<std::string, std::size_t> duplicate_counts;
    while (!work.empty() && proof.count < 4000U) {
        const Address address = work.front();
        work.pop_front();
        if (!visited.insert(address).second) {
            continue;
        }
        const auto decoded = context.decode(address);
        if (!decoded || decoded->length == 0U) {
            proof.valid = false;
            break;
        }
        if (!decoded->is_x86 && is_arm_filler(*decoded)) {
            proof.valid = false;
            break;
        }
        const auto identity = instruction_identity(*decoded);
        if (++duplicate_counts[identity] > 4U) {
            proof.valid = false;
            break;
        }
        proof.instructions.insert(address);
        ++proof.count;
        const auto kind = decoded->flow.kind;
        if (kind == sleigh_runtime::FlowKind::call || kind == sleigh_runtime::FlowKind::conditional_call ||
            kind == sleigh_runtime::FlowKind::indirect_call) {
            proof.adds_information = true;
        }
        // Ghidra's PseudoDisassembler accepts a terminal instruction before
        // examining its synthetic return target. A RET commonly carries a
        // register/stack target that is not a mapped program address.
        if (decoded->flow.terminal) {
            continue;
        }
        if (const auto target = flow_target(context, decoded->flow.target)) {
            if (kind == sleigh_runtime::FlowKind::branch || kind == sleigh_runtime::FlowKind::conditional_branch ||
                kind == sleigh_runtime::FlowKind::indirect_branch) {
                if (context.instructions().contains(*target)) {
                    proof.adds_information = true;
                }
            }
            work.push_back(*target);
        } else if (decoded->flow.target) {
            proof.valid = false;
            break;
        }
        if (decoded->flow.has_fallthrough || kind == sleigh_runtime::FlowKind::none) {
            if (address > std::numeric_limits<Address>::max() - decoded->length) {
                proof.valid = false;
                break;
            }
            work.push_back(address + decoded->length);
        }
    }
    if (proof.count >= 4000U && !work.empty()) {
        proof.valid = false;
    }
    return proof;
}

/// Returns whether a pseudo-flow body intersects an existing defined data object.
[[nodiscard]] bool body_contains_data(AnalysisContext& context, const std::set<Address>& starts) {
    for (const Address start : starts) {
        const auto instruction = context.instructions().find(start);
        const auto decoded = instruction == context.instructions().end()
                                 ? context.decode(start)
                                 : std::expected<sleigh_runtime::Instruction, sleigh_runtime::DecodeError>(
                                       instruction->second.instruction);
        if (!decoded) {
            continue;
        }
        for (std::size_t offset = 0; offset < decoded->length; ++offset) {
            for (const auto& [data_start, data] : context.data()) {
                if (start + offset >= data_start && start + offset < data_start + data.size) {
                    return true;
                }
            }
        }
    }
    return false;
}

/// Collects repeated two-instruction start keys from already known functions.
[[nodiscard]] std::map<std::vector<std::uint8_t>, std::size_t> function_start_frequencies(AnalysisContext& context) {
    std::map<std::vector<std::uint8_t>, std::size_t> frequencies;
    for (const auto& [entry, function] : context.functions()) {
        static_cast<void>(function);
        const auto key = instruction_shape_key(context, entry);
        if (key) {
            ++frequencies[*key];
        }
    }
    return frequencies;
}

} // namespace

/// Returns the opt-in late byte-analysis contract for Aggressive Instruction Finder.
AnalyzerDescriptor AggressiveInstructionFinderAnalyzer::descriptor() const {
    return {"Aggressive Instruction Finder",
            901,
            {EventKind::memory_added, EventKind::code_added, EventKind::function_added, EventKind::data_added},
            {}};
}

/// Searches undefined executable bytes using repeated starts and conservative pseudo-flow evidence.
void AggressiveInstructionFinderAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                                  CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/AggressiveInstructionFinderAnalyzer.java
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/ArmAggressiveInstructionFinderAnalyzer.java
    // Relevant methods: added(), checkExecBlocks(), function-start hashing, checkValidARMTMode(),
    // doValidStart(), and the PseudoDisassembler flow processor.
    if (!context.options().aggressive_instruction_finder) {
        return;
    }
    constexpr std::size_t java_minimum_functions = 20U;
    const std::size_t minimum_functions =
        std::max<std::size_t>(java_minimum_functions, context.options().aggressive_minimum_functions);
    if (context.functions().size() < minimum_functions || context.instructions().empty()) {
        return;
    }
    const auto frequencies = function_start_frequencies(context);
    if (frequencies.empty()) {
        return;
    }
    for (const auto& region : context.image().memory_regions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!region.executable || region.size == 0U) {
            continue;
        }
        for (std::uint64_t offset = 0; offset < region.size; ++offset) {
            if (cancellation.is_cancelled()) {
                return;
            }
            const Address entry = region.start + offset;
            if (is_defined(context, entry)) {
                continue;
            }
            const auto key = instruction_shape_key(context, entry);
            if (!key) {
                continue;
            }
            const auto frequency = frequencies.find(*key);
            if (frequency == frequencies.end() || frequency->second < 4U) {
                continue;
            }
            const auto proof = prove_candidate(context, entry);
            if (!proof.valid || proof.count <= 2U || body_contains_data(context, proof.instructions)) {
                continue;
            }
            if (context.disassemble_flow(entry) == 0U) {
                continue;
            }
            if (context.options().create_analysis_bookmarks) {
                static_cast<void>(context.add_bookmark(Bookmark{entry, "Aggressive Instruction Finder", "Found code"}));
            }
            // The Java analyzer schedules the residual undefined set as a
            // one-time follow-up and returns after the first discovery.
            return;
        }
    }
}

} // namespace recode::analyzer
