export module recode.service.bsim.block_signature;

import std;
import recode.core.normalized_function;
import recode.service.bsim.signature;

// Ported from Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh and
// signature.cc: BlockSignatureEntry::localHash and hashIn.

export namespace recode::services::bsim {

/// Overlay node used by the ported control-flow signature iteration.
class BlockSignatureEntry final {
public:
    /// Constructs an overlay for one normalized basic block.
    explicit BlockSignatureEntry(std::uint32_t block_id) : block_id_(block_id) {}

    /// Returns the normalized block identity.
    [[nodiscard]] std::uint32_t block_id() const noexcept {
        return block_id_;
    }

    /// Returns the current block hash as a 32-bit value.
    [[nodiscard]] std::uint32_t hash() const noexcept {
        return static_cast<std::uint32_t>(hash_[0]);
    }

    /// Copies the current block hash into the previous-round slot.
    void flip() noexcept {
        hash_[1] = hash_[0];
    }

    /// Initializes the local predecessor/successor count hash.
    void local_hash(const recode::core::NormalizedBasicBlock& block) noexcept {
        hash_[0] = (static_cast<std::uint64_t>(block.predecessors.size()) << 8U) |
                   static_cast<std::uint64_t>(block.successors.size());
    }

    /// Mixes incoming block hashes and conditional edge direction markers.
    void hash_in(const recode::core::NormalizedBasicBlock& block,
                 const std::vector<const BlockSignatureEntry*>& incoming,
                 const std::vector<const recode::core::NormalizedBasicBlock*>& incoming_blocks) noexcept {
        auto current = hash_[1];
        std::uint64_t accumulated = 0xbafabacaULL;
        for (std::size_t index = 0; index < incoming.size(); ++index) {
            auto mixed = hash_mixin(current, incoming[index]->hash_[1]);
            const auto& predecessor = *incoming_blocks[index];
            if (predecessor.successors.size() == 2U) {
                const auto edge = std::ranges::find(predecessor.successors, block.id);
                if (edge != predecessor.successors.end() &&
                    static_cast<std::size_t>(std::distance(predecessor.successors.begin(), edge)) == 0U)
                    mixed = hash_mixin(mixed, 0x777ULL ^ 0x7abc7abcULL);
                else
                    mixed = hash_mixin(mixed, 0x777ULL);
            }
            accumulated += mixed;
        }
        hash_[0] = hash_mixin(current, accumulated);
    }

private:
    std::uint32_t block_id_{};
    std::uint64_t hash_[2]{};
};

} // namespace recode::services::bsim
