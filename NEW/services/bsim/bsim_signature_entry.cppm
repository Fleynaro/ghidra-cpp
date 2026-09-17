export module recode.service.bsim.signature_entry;

import std;
import recode.core.normalized_function;
import recode.service.bsim.signature;

// Ported from Ghidra/Features/Decompiler/src/decompile/cpp/signature.hh and
// signature.cc: SignatureEntry, localHash, hashIn, removeNoise, and shadows.

export namespace recode::services::bsim {

/// Overlay node used by the ported data-flow signature iteration.
/// It stores normalized IDs rather than owning native decompiler Varnodes.
class SignatureEntry final {
public:
    using Index = std::size_t;
    static constexpr Index no_index = std::numeric_limits<Index>::max();

    /// Constructs a real overlay node for one normalized Varnode.
    SignatureEntry(const recode::core::NormalizedFunction& function, const recode::core::NormalizedVarnode& varnode,
                   const std::unordered_map<std::uint32_t, Index>& varnode_map,
                   const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t modifiers);

    /// Constructs the virtual root used by indirect-noise dominator analysis.
    explicit SignatureEntry(Index virtual_index);

    /// Returns the normalized Varnode identity, or zero for the virtual root.
    [[nodiscard]] std::uint32_t varnode_id() const noexcept {
        return varnode_id_;
    }

    /// Returns the current 32-bit hash value.
    [[nodiscard]] std::uint32_t hash() const noexcept {
        return static_cast<std::uint32_t>(hash_[0]);
    }

    /// Returns the previous-round hash value.
    [[nodiscard]] std::uint64_t previous_hash() const noexcept {
        return hash_[1];
    }

    /// Returns whether the value is terminal in the defining-op graph.
    [[nodiscard]] bool terminal() const noexcept {
        return terminal_;
    }

    /// Returns whether the value is suppressed from feature emission.
    [[nodiscard]] bool not_emitted() const noexcept {
        return not_emitted_;
    }

    /// Returns whether the defining operation treats inputs commutatively.
    [[nodiscard]] bool commutative() const noexcept {
        return commutative_;
    }

    /// Returns whether this node is a stand-alone COPY/INDIRECT feature.
    [[nodiscard]] bool standalone_copy() const noexcept {
        return standalone_copy_;
    }

    /// Returns the number of effective data-flow inputs.
    [[nodiscard]] std::size_t input_count() const noexcept {
        return input_size_;
    }

    /// Returns the effective input varnode ID at a zero-based input slot.
    [[nodiscard]] std::uint32_t input_id(const recode::core::NormalizedFunction& function, std::size_t index) const;

    /// Returns the entry index of the effective input, with shadow collapsing.
    [[nodiscard]] Index input_entry(const recode::core::NormalizedFunction& function,
                                    const std::unordered_map<std::uint32_t, Index>& varnode_map, std::size_t index,
                                    const std::vector<SignatureEntry>& entries) const;

    /// Calculates the COPY/INDIRECT/CAST shadow chain used without noise collapse.
    void calculate_shadow(const recode::core::NormalizedFunction& function,
                          const std::unordered_map<std::uint32_t, Index>& varnode_map,
                          const std::unordered_map<std::uint32_t, Index>& operation_map);

    /// Calculates the local hash and emission flags before iterative mixing.
    void local_hash(const recode::core::NormalizedFunction& function,
                    const std::unordered_map<std::uint32_t, Index>& varnode_map,
                    const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t modifiers);

    /// Copies the current hash into the previous-round slot.
    void flip() noexcept {
        hash_[1] = hash_[0];
    }

    /// Mixes previous-round hashes from effective inputs into this node.
    void hash_in(const std::vector<const SignatureEntry*>& neighbors);

    /// Returns the shadow base index, or `no_index` when this is a base.
    [[nodiscard]] Index shadow() const noexcept {
        return shadow_;
    }

    /// Returns whether this node is a marker-graph root.
    [[nodiscard]] bool marker_root() const noexcept {
        return marker_root_;
    }

    /// Removes indirect-copy noise using the original post-order/dominator algorithm.
    static void remove_noise(const recode::core::NormalizedFunction& function,
                             const std::unordered_map<std::uint32_t, Index>& varnode_map,
                             const std::unordered_map<std::uint32_t, Index>& operation_map,
                             std::vector<SignatureEntry>& entries);

private:
    /// Tests whether a normalized COPY/INDIRECT output is stand-alone.
    static bool test_standalone_copy(const recode::core::NormalizedFunction& function,
                                     const recode::core::NormalizedVarnode& varnode,
                                     const recode::core::NormalizedOperation& operation);

    /// Computes the size contribution with optional collapse-to-four behavior.
    static std::uint64_t hash_size(const recode::core::NormalizedVarnode& varnode, std::uint32_t modifiers) noexcept;

    /// Computes the effective defining-op hash.
    [[nodiscard]] std::uint64_t operation_hash(const recode::core::NormalizedFunction& function,
                                               const recode::core::NormalizedOperation& operation) const noexcept;

    /// Computes the special stand-alone COPY hash.
    void standalone_copy_hash(const recode::core::NormalizedFunction& function,
                              const std::unordered_map<std::uint32_t, Index>& varnode_map, std::uint32_t modifiers);

    /// Computes the post-order of the marker-only graph.
    static void noise_post_order(const recode::core::NormalizedFunction& function,
                                 const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                 const std::unordered_map<std::uint32_t, Index>& operation_map,
                                 const std::vector<Index>& roots, std::vector<Index>& post_order,
                                 std::vector<SignatureEntry>& entries);

    /// Computes immediate dominators in the marker-only graph.
    static void noise_dominator(const recode::core::NormalizedFunction& function,
                                const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                const std::vector<Index>& post_order, Index virtual_root,
                                std::vector<SignatureEntry>& entries);

    /// Returns a function varnode by normalized ID.
    [[nodiscard]] static const recode::core::NormalizedVarnode&
    get_varnode(const recode::core::NormalizedFunction& function,
                const std::unordered_map<std::uint32_t, Index>& varnode_map, std::uint32_t id);

    /// Returns a function operation by normalized ID.
    [[nodiscard]] static const recode::core::NormalizedOperation&
    get_operation(const recode::core::NormalizedFunction& function,
                  const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t id);

    std::uint32_t varnode_id_{};
    std::optional<std::uint32_t> operation_id_;
    std::size_t start_input_{};
    std::size_t input_size_{};
    std::uint64_t hash_[2]{};
    Index shadow_{no_index};
    Index index_{no_index};
    bool terminal_{};
    bool commutative_{};
    bool standalone_copy_{};
    bool not_emitted_{};
    bool marker_root_{};
    bool visited_{};
};

inline SignatureEntry::SignatureEntry(const recode::core::NormalizedFunction& function,
                                      const recode::core::NormalizedVarnode& varnode,
                                      const std::unordered_map<std::uint32_t, Index>&,
                                      const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t)
    : varnode_id_(varnode.id), operation_id_(varnode.defining_operation), index_(no_index) {
    if (!operation_id_) {
        terminal_ = true;
        return;
    }
    const auto& operation = get_operation(function, operation_map, *operation_id_);
    start_input_ = 0;
    input_size_ = operation.inputs.size();
    switch (operation.opcode) {
        case recode::core::PcodeOpcode::copy:
            standalone_copy_ = test_standalone_copy(function, varnode, operation);
            break;
        case recode::core::PcodeOpcode::indirect:
            input_size_ = input_size_ == 0 ? 0 : input_size_ - 1;
            standalone_copy_ = test_standalone_copy(function, varnode, operation);
            break;
        case recode::core::PcodeOpcode::multiequal:
            commutative_ = true;
            break;
        case recode::core::PcodeOpcode::call:
        case recode::core::PcodeOpcode::call_ind:
        case recode::core::PcodeOpcode::call_other:
        case recode::core::PcodeOpcode::store:
        case recode::core::PcodeOpcode::load:
            start_input_ = std::min<std::size_t>(1, input_size_);
            input_size_ -= start_input_;
            break;
        case recode::core::PcodeOpcode::int_left:
        case recode::core::PcodeOpcode::int_right:
        case recode::core::PcodeOpcode::int_sright:
        case recode::core::PcodeOpcode::subpiece:
            if (operation.inputs.size() > 1) {
                const auto& amount = get_varnode(function, {}, operation.inputs[1]);
                if (amount.constant)
                    input_size_ = 1;
            }
            break;
        case recode::core::PcodeOpcode::cpool_ref:
            input_size_ = 0;
            break;
        default:
            commutative_ = operation.commutative;
            break;
    }
}

inline SignatureEntry::SignatureEntry(Index virtual_index) : index_(virtual_index) {}

inline const recode::core::NormalizedVarnode&
SignatureEntry::get_varnode(const recode::core::NormalizedFunction& function,
                            const std::unordered_map<std::uint32_t, Index>& varnode_map, std::uint32_t id) {
    if (!varnode_map.empty())
        return function.varnodes.at(varnode_map.at(id));
    const auto iterator =
        std::ranges::find_if(function.varnodes, [id](const auto& candidate) { return candidate.id == id; });
    if (iterator == function.varnodes.end())
        throw std::out_of_range("Normalized varnode ID is not present");
    return *iterator;
}

inline const recode::core::NormalizedOperation&
SignatureEntry::get_operation(const recode::core::NormalizedFunction& function,
                              const std::unordered_map<std::uint32_t, Index>& operation_map, std::uint32_t id) {
    return function.operations.at(operation_map.at(id));
}

inline bool SignatureEntry::test_standalone_copy(const recode::core::NormalizedFunction& function,
                                                 const recode::core::NormalizedVarnode& varnode,
                                                 const recode::core::NormalizedOperation& operation) {
    if (operation.inputs.empty())
        return false;
    const auto& input = get_varnode(function, {}, operation.inputs.front());
    if (input.written ||
        (input.storage.space == varnode.storage.space && input.storage.offset == varnode.storage.offset))
        return false;
    if (varnode.persistent && operation.opcode == recode::core::PcodeOpcode::indirect)
        return true;
    std::vector<const recode::core::NormalizedOperation*> descendants;
    for (const auto& candidate : function.operations)
        if (std::ranges::find(candidate.inputs, varnode.id) != candidate.inputs.end())
            descendants.push_back(&candidate);
    if (descendants.empty())
        return true;
    if (descendants.size() != 1)
        return false;
    const auto* descendant = descendants.front();
    if (varnode.persistent && descendant->opcode == recode::core::PcodeOpcode::indirect)
        return true;
    if (descendant->opcode != recode::core::PcodeOpcode::copy &&
        descendant->opcode != recode::core::PcodeOpcode::indirect)
        return false;
    if (!descendant->output)
        return false;
    return std::ranges::none_of(function.operations, [&](const auto& candidate) {
        return std::ranges::find(candidate.inputs, *descendant->output) != candidate.inputs.end();
    });
}

inline std::uint64_t SignatureEntry::hash_size(const recode::core::NormalizedVarnode& varnode,
                                               std::uint32_t modifiers) noexcept {
    std::uint64_t value = static_cast<std::uint8_t>(varnode.size);
    if ((modifiers & 0x1U) != 0U && value > 4U)
        value = 4U;
    return value ^ (value << 7U) ^ (value << 14U) ^ (value << 21U);
}

inline std::uint64_t SignatureEntry::operation_hash(const recode::core::NormalizedFunction& function,
                                                    const recode::core::NormalizedOperation& operation) const noexcept {
    auto value = static_cast<std::uint64_t>(operation.opcode);
    if (operation.opcode == recode::core::PcodeOpcode::cpool_ref && !operation.inputs.empty()) {
        const auto iterator = std::ranges::find_if(
            function.varnodes, [&](const auto& candidate) { return candidate.id == operation.inputs.back(); });
        const auto tag_value = iterator == function.varnodes.end() ? 0ULL : iterator->constant_value;
        value = (value + 0xfeedfaceULL) ^ tag_value;
    }
    return value;
}

inline void SignatureEntry::standalone_copy_hash(const recode::core::NormalizedFunction& function,
                                                 const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                                 std::uint32_t modifiers) {
    const auto& varnode = get_varnode(function, varnode_map, varnode_id_);
    auto value = hash_size(varnode, modifiers) ^ 0xaf29e23bULL;
    if (varnode.persistent)
        value ^= 0x55055055ULL;
    if (!operation_id_)
        return;
    const auto& operation = std::ranges::find_if(function.operations,
                                                 [&](const auto& candidate) { return candidate.id == *operation_id_; });
    if (operation != function.operations.end() && !operation->inputs.empty()) {
        const auto input = std::ranges::find_if(
            function.varnodes, [&](const auto& candidate) { return candidate.id == operation->inputs.front(); });
        if (input != function.varnodes.end() && input->constant) {
            value ^= (modifiers & 0x10U) == 0U ? varnode.storage.offset : 0xa0a0a0a0ULL;
        } else if (input != function.varnodes.end() && input->persistent) {
            value ^= 0xd7651ec3ULL;
        }
    }
    hash_[0] = value;
    hash_[1] = value;
}

inline void SignatureEntry::calculate_shadow(const recode::core::NormalizedFunction& function,
                                             const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                             const std::unordered_map<std::uint32_t, Index>& operation_map) {
    std::uint32_t shadow_varnode = varnode_id_;
    for (;;) {
        const auto& varnode = get_varnode(function, varnode_map, shadow_varnode);
        if (!varnode.defining_operation)
            break;
        const auto& operation = get_operation(function, operation_map, *varnode.defining_operation);
        if (operation.opcode != recode::core::PcodeOpcode::copy &&
            operation.opcode != recode::core::PcodeOpcode::indirect &&
            operation.opcode != recode::core::PcodeOpcode::cast)
            break;
        if (operation.inputs.empty())
            break;
        shadow_varnode = operation.inputs.front();
    }
    if (shadow_varnode != varnode_id_)
        shadow_ = varnode_map.at(shadow_varnode);
}

inline void SignatureEntry::local_hash(const recode::core::NormalizedFunction& function,
                                       const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                       const std::unordered_map<std::uint32_t, Index>& operation_map,
                                       std::uint32_t modifiers) {
    const auto& varnode = get_varnode(function, varnode_map, varnode_id_);
    if (varnode.annotation) {
        not_emitted_ = true;
        terminal_ = true;
        hash_[0] = hash_[1] = 0xb7b7b7b7ULL;
        return;
    }
    if (shadow_ != no_index) {
        not_emitted_ = true;
        if (standalone_copy_)
            standalone_copy_hash(function, varnode_map, modifiers);
        return;
    }
    auto value = hash_size(varnode, modifiers);
    if (!varnode.written)
        not_emitted_ = true;
    if (varnode.constant)
        value ^= (modifiers & 0x10U) == 0U ? varnode.constant_value : 0xa0a0a0a0ULL;
    if ((modifiers & 0x40U) == 0U && varnode.persistent)
        value ^= 0x55055055ULL;
    if (varnode.input)
        value ^= 0x10101ULL;
    if (operation_id_) {
        const auto op_hash = operation_hash(function, get_operation(function, operation_map, *operation_id_));
        if (op_hash != 0)
            value ^= op_hash ^ (op_hash << 9U) ^ (op_hash << 18U);
    }
    hash_[0] = hash_[1] = value;
}

inline std::uint32_t SignatureEntry::input_id(const recode::core::NormalizedFunction& function,
                                              std::size_t index) const {
    if (!operation_id_)
        throw std::out_of_range("Terminal signature entry has no inputs");
    const auto iterator = std::ranges::find_if(function.operations,
                                               [&](const auto& candidate) { return candidate.id == *operation_id_; });
    if (iterator == function.operations.end() || index >= input_size_ ||
        start_input_ + index >= iterator->inputs.size())
        throw std::out_of_range("Signature input index is outside the normalized operation");
    return iterator->inputs[start_input_ + index];
}

inline SignatureEntry::Index SignatureEntry::input_entry(const recode::core::NormalizedFunction& function,
                                                         const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                                         std::size_t index,
                                                         const std::vector<SignatureEntry>& entries) const {
    auto result = varnode_map.at(input_id(function, index));
    return entries[result].shadow_ == no_index ? result : entries[result].shadow_;
}

inline void SignatureEntry::hash_in(const std::vector<const SignatureEntry*>& neighbors) {
    auto current = hash_[1];
    if (commutative_) {
        std::uint64_t accumulated = 0;
        for (const auto* entry : neighbors)
            accumulated += hash_mixin(current, entry->hash_[1]);
        current = hash_mixin(current, accumulated);
    } else {
        for (const auto* entry : neighbors)
            current = hash_mixin(current, entry->hash_[1]);
    }
    hash_[0] = current;
}

inline void SignatureEntry::noise_post_order(const recode::core::NormalizedFunction& function,
                                             const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                             const std::unordered_map<std::uint32_t, Index>&,
                                             const std::vector<Index>& roots, std::vector<Index>& post_order,
                                             std::vector<SignatureEntry>& entries) {
    struct Frame {
        Index entry{};
        std::size_t operation_index{};
    };
    std::vector<Frame> stack;
    for (const auto root : roots) {
        if (entries[root].visited_)
            continue;
        entries[root].visited_ = true;
        stack.push_back(Frame{root, 0});
        while (!stack.empty()) {
            auto& frame = stack.back();
            const auto varnode_id = entries[frame.entry].varnode_id_;
            if (frame.operation_index >= function.operations.size()) {
                entries[frame.entry].index_ = post_order.size();
                post_order.push_back(frame.entry);
                stack.pop_back();
                continue;
            }
            const auto& operation = function.operations[frame.operation_index++];
            if (!operation.marker && operation.opcode != recode::core::PcodeOpcode::copy)
                continue;
            if (std::ranges::find(operation.inputs, varnode_id) == operation.inputs.end() || !operation.output)
                continue;
            const auto child = varnode_map.at(*operation.output);
            if (entries[child].visited_)
                continue;
            entries[child].visited_ = true;
            stack.push_back(Frame{child, 0});
        }
    }
}

inline void SignatureEntry::noise_dominator(const recode::core::NormalizedFunction& function,
                                            const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                            const std::vector<Index>& post_order, Index virtual_root,
                                            std::vector<SignatureEntry>& entries) {
    if (post_order.empty())
        return;
    const auto official_root = post_order.back();
    entries[official_root].shadow_ = official_root;
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t reverse = post_order.size() - 1; reverse-- > 0;) {
            const auto current = post_order[reverse];
            if (entries[current].shadow_ == official_root)
                continue;
            const auto& current_vn = function.varnodes.at(varnode_map.at(entries[current].varnode_id_));
            if (!current_vn.defining_operation)
                continue;
            const auto operation = std::ranges::find_if(function.operations, [&](const auto& candidate) {
                return candidate.id == *current_vn.defining_operation;
            });
            if (operation == function.operations.end())
                continue;
            const std::size_t input_count = entries[current].marker_root_ ? 1 : operation->inputs.size();
            Index new_idom = no_index;
            std::size_t first = 0;
            for (; first < input_count; ++first) {
                const auto input_id = entries[current].marker_root_ ? 0U : operation->inputs[first];
                const auto input = entries[current].marker_root_ ? virtual_root : varnode_map.at(input_id);
                if (entries[input].shadow_ != no_index) {
                    new_idom = input;
                    break;
                }
            }
            if (new_idom == no_index)
                continue;
            for (++first; first < input_count; ++first) {
                const auto input_id = operation->inputs[first];
                const auto other = varnode_map.at(input_id);
                if (entries[other].shadow_ == no_index)
                    continue;
                auto finger1 = entries[other].index_;
                auto finger2 = entries[new_idom].index_;
                while (finger1 != finger2) {
                    while (finger1 < finger2)
                        finger1 = entries[post_order[finger1]].shadow_ == no_index
                                      ? finger1
                                      : entries[entries[post_order[finger1]].shadow_].index_;
                    while (finger2 < finger1)
                        finger2 = entries[post_order[finger2]].shadow_ == no_index
                                      ? finger2
                                      : entries[entries[post_order[finger2]].shadow_].index_;
                    if (finger1 == entries[post_order[finger1]].index_ &&
                        finger2 == entries[post_order[finger2]].index_)
                        break;
                }
                new_idom = post_order[finger1];
            }
            if (entries[current].shadow_ != new_idom) {
                entries[current].shadow_ = new_idom;
                changed = true;
            }
        }
    }
}

inline void SignatureEntry::remove_noise(const recode::core::NormalizedFunction& function,
                                         const std::unordered_map<std::uint32_t, Index>& varnode_map,
                                         const std::unordered_map<std::uint32_t, Index>& operation_map,
                                         std::vector<SignatureEntry>& entries) {
    std::vector<Index> roots;
    for (Index index = 0; index < entries.size(); ++index) {
        auto& entry = entries[index];
        if (entry.varnode_id_ == 0 && index >= function.varnodes.size())
            continue;
        const auto& varnode = get_varnode(function, varnode_map, entry.varnode_id_);
        if (varnode.input || varnode.constant) {
            roots.push_back(index);
            entry.marker_root_ = true;
        } else if (varnode.written && varnode.defining_operation) {
            const auto& operation = get_operation(function, operation_map, *varnode.defining_operation);
            if ((!operation.marker) && operation.opcode != recode::core::PcodeOpcode::copy) {
                roots.push_back(index);
                entry.marker_root_ = true;
            }
        }
    }
    if (roots.empty())
        return;
    std::vector<Index> post_order;
    noise_post_order(function, varnode_map, operation_map, roots, post_order, entries);
    const Index virtual_root = entries.size();
    entries.emplace_back(virtual_root);
    entries[virtual_root].index_ = post_order.size();
    post_order.push_back(virtual_root);
    for (const auto root : roots)
        entries[root].shadow_ = virtual_root;
    noise_dominator(function, varnode_map, post_order, virtual_root, entries);
    post_order.pop_back();
    for (const auto index : post_order)
        if (entries[index].shadow_ == virtual_root)
            entries[index].shadow_ = no_index;
    for (const auto index : post_order) {
        auto base = index;
        while (entries[base].shadow_ != no_index)
            base = entries[base].shadow_;
        auto current = index;
        while (entries[current].shadow_ != no_index) {
            const auto next = entries[current].shadow_;
            entries[current].shadow_ = base;
            current = next;
        }
    }
    entries.pop_back();
}

} // namespace recode::services::bsim
