export module recode.service.bsim.weight_factory;

import std;

// Ported from:
// Ghidra/Framework/Generic/src/main/java/generic/lsh/vector/WeightFactory.java
// Original class: WeightFactory.  The serialized ordering and normalized
// penalty parameters remain the same; XML parsing is intentionally outside
// this vector/weight layer unit.

export namespace recode::services::bsim {

/// Owns the fixed TF/IDF coefficient tables and significance parameters.
///
/// Fresh instances intentionally contain Java's zero-initialized arrays and
/// zero scalar fields.  Call set_logarithmic_tf_weights or set before use when
/// a caller has not loaded an original Ghidra weight resource.
class WeightFactory final {
public:
    /// Number of IDF slots used by Ghidra's HashEntry.
    static constexpr std::size_t idf_size_constant = 512;

    /// Number of internal TF slots used by Ghidra's HashEntry.
    static constexpr std::size_t tf_size_constant = 64;

    /// Number of serialized doubles in WeightFactory.toArray().
    static constexpr std::size_t serialized_size_constant = 583;

    /// Constructs Java-equivalent zero-initialized weighting state.
    WeightFactory() = default;

    /// Returns the fixed number of IDF table entries.
    [[nodiscard]] constexpr std::size_t idf_size() const noexcept {
        return idf_size_constant;
    }

    /// Returns the fixed number of TF table entries.
    [[nodiscard]] constexpr std::size_t tf_size() const noexcept {
        return tf_size_constant;
    }

    /// Returns the exact number of values used by Java's serialized array.
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return serialized_size_constant;
    }

    /// Returns an IDF coefficient at the supplied raw table slot.
    [[nodiscard]] double get_idf_weight(std::size_t value) const {
        return idf_weight_.at(value);
    }

    /// Returns a TF coefficient at the supplied internal count-minus-one slot.
    [[nodiscard]] double get_tf_weight(std::size_t value) const {
        return tf_weight_.at(value);
    }

    /// Multiplies the IDF and TF values exactly as WeightFactory.getCoeff does.
    [[nodiscard]] double get_coeff(std::size_t idf, std::size_t tf) const {
        return idf_weight_.at(idf) * tf_weight_.at(tf);
    }

    /// Returns the unnormalized weight-norm resource value.
    [[nodiscard]] double get_weight_norm() const noexcept {
        return weight_norm_;
    }

    /// Returns the first normalized hash-flip penalty.
    [[nodiscard]] double get_flip_norm0() const noexcept {
        return prob_flip0_norm_;
    }

    /// Returns the second normalized hash-flip penalty.
    [[nodiscard]] double get_flip_norm1() const noexcept {
        return prob_flip1_norm_;
    }

    /// Returns the first normalized hash-addition/removal penalty.
    [[nodiscard]] double get_diff_norm0() const noexcept {
        return prob_diff0_norm_;
    }

    /// Returns the second normalized hash-addition/removal penalty.
    [[nodiscard]] double get_diff_norm1() const noexcept {
        return prob_diff1_norm_;
    }

    /// Returns the final score scale used by the vector factory.
    [[nodiscard]] double get_scale() const noexcept {
        return scale_;
    }

    /// Returns the final score addend used by the vector factory.
    [[nodiscard]] double get_addend() const noexcept {
        return addend_;
    }

    /// Returns a raw IDF probability parameter before score normalization.
    [[nodiscard]] double get_prob_flip0() const noexcept {
        return prob_flip0_;
    }

    /// Returns a raw second IDF probability parameter before score normalization.
    [[nodiscard]] double get_prob_flip1() const noexcept {
        return prob_flip1_;
    }

    /// Returns a raw first hash-difference parameter before score normalization.
    [[nodiscard]] double get_prob_diff0() const noexcept {
        return prob_diff0_;
    }

    /// Returns a raw second hash-difference parameter before score normalization.
    [[nodiscard]] double get_prob_diff1() const noexcept {
        return prob_diff1_;
    }

    /// Writes one IDF table slot for resource loaders and deterministic tests.
    void set_idf_weight(std::size_t value, double weight) {
        idf_weight_.at(value) = weight;
    }

    /// Writes one TF table slot for resource loaders and deterministic tests.
    void set_tf_weight(std::size_t value, double weight) {
        tf_weight_.at(value) = weight;
    }

    /// Sets raw score parameters and refreshes their scale-normalized copies.
    void set_significance_parameters(double weight_norm, double prob_flip0, double prob_flip1, double prob_diff0,
                                     double prob_diff1, double scale, double addend) noexcept {
        weight_norm_ = weight_norm;
        prob_flip0_ = prob_flip0;
        prob_flip1_ = prob_flip1;
        prob_diff0_ = prob_diff0;
        prob_diff1_ = prob_diff1;
        scale_ = scale;
        addend_ = addend;
        update_norms();
    }

    /// Initializes TF weights with Ghidra's sqrt(1 + log2(count)) formula.
    void set_logarithmic_tf_weights() noexcept {
        const double log_two = std::log(2.0);
        for (std::size_t index = 0; index < tf_weight_.size(); ++index)
            tf_weight_[index] = std::sqrt(1.0 + std::log(static_cast<double>(index + 1U)) / log_two);
    }

    /// Serializes the factory in WeightFactory.toArray order.
    [[nodiscard]] std::vector<double> to_array() const {
        std::vector<double> result(serialized_size_constant);
        const double scale_sqrt = std::sqrt(scale_);
        for (std::size_t index = 0; index < idf_weight_.size(); ++index)
            result[index] = idf_weight_[index] / scale_sqrt;
        for (std::size_t index = 0; index < tf_weight_.size(); ++index)
            result[idf_weight_.size() + index] = tf_weight_[index];

        result[serialized_size_constant - 7U] = weight_norm_ * scale_;
        result[serialized_size_constant - 6U] = prob_flip0_;
        result[serialized_size_constant - 5U] = prob_flip1_;
        result[serialized_size_constant - 4U] = prob_diff0_;
        result[serialized_size_constant - 3U] = prob_diff1_;
        result[serialized_size_constant - 2U] = scale_;
        result[serialized_size_constant - 1U] = addend_;
        return result;
    }

    /// Restores the factory from WeightFactory.toArray order.
    void set(std::span<const double> values) {
        if (values.size() != serialized_size_constant)
            throw std::invalid_argument("Not enough values in WeightFactory array");

        scale_ = values[serialized_size_constant - 2U];
        addend_ = values[serialized_size_constant - 1U];
        weight_norm_ = values[serialized_size_constant - 7U] / scale_;
        prob_flip0_ = values[serialized_size_constant - 6U];
        prob_flip1_ = values[serialized_size_constant - 5U];
        prob_diff0_ = values[serialized_size_constant - 4U];
        prob_diff1_ = values[serialized_size_constant - 3U];

        const double scale_sqrt = std::sqrt(scale_);
        for (std::size_t index = 0; index < idf_weight_.size(); ++index)
            idf_weight_[index] = values[index] * scale_sqrt;
        for (std::size_t index = 0; index < tf_weight_.size(); ++index)
            tf_weight_[index] = values[idf_weight_.size() + index];
        update_norms();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] std::size_t getIDFSize() const noexcept {
        return idf_size();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] std::size_t getTFSize() const noexcept {
        return tf_size();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] std::size_t getSize() const noexcept {
        return size();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getIDFWeight(std::size_t value) const {
        return get_idf_weight(value);
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getTFWeight(std::size_t value) const {
        return get_tf_weight(value);
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getCoeff(std::size_t idf, std::size_t tf) const {
        return get_coeff(idf, tf);
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getWeightNorm() const noexcept {
        return get_weight_norm();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getFlipNorm0() const noexcept {
        return get_flip_norm0();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getFlipNorm1() const noexcept {
        return get_flip_norm1();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getDiffNorm0() const noexcept {
        return get_diff_norm0();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getDiffNorm1() const noexcept {
        return get_diff_norm1();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getScale() const noexcept {
        return get_scale();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] double getAddend() const noexcept {
        return get_addend();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    void setLogarithmicTFWeights() noexcept {
        set_logarithmic_tf_weights();
    }

    /// Java-style spelling retained for direct source-to-source comparisons.
    [[nodiscard]] std::vector<double> toArray() const {
        return to_array();
    }

private:
    /// Refreshes the four normalized probability values after scale changes.
    void update_norms() noexcept {
        prob_flip0_norm_ = prob_flip0_ * scale_;
        prob_flip1_norm_ = prob_flip1_ * scale_;
        prob_diff0_norm_ = prob_diff0_ * scale_;
        prob_diff1_norm_ = prob_diff1_ * scale_;
    }

    std::array<double, idf_size_constant> idf_weight_{};
    std::array<double, tf_size_constant> tf_weight_{};
    double weight_norm_{};
    double prob_flip0_{};
    double prob_flip1_{};
    double prob_diff0_{};
    double prob_diff1_{};
    double scale_{};
    double addend_{};
    double prob_flip0_norm_{};
    double prob_flip1_norm_{};
    double prob_diff0_norm_{};
    double prob_diff1_norm_{};
};

} // namespace recode::services::bsim
