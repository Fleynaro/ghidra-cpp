export module analyzer_registry;

import analyzer_base;
import analyzer_types;
import std;

// MSVC does not emit inline definitions from independently compiled named
// module interfaces unless the owning class is explicitly exported.
#if defined(_MSC_VER)
#define GHIDRA_ANALYZER_MODULE_EXPORT __declspec(dllexport)
#else
#define GHIDRA_ANALYZER_MODULE_EXPORT
#endif

export namespace recode::analyzer {

/// Stores extensible analyzer registrations without manager redesign.
// Preserves the registration-name invariant from
// Ghidra/Framework/Project/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java.
export class GHIDRA_ANALYZER_MODULE_EXPORT AnalyzerRegistry final {
public:
    /// Constructs an empty registry.
    AnalyzerRegistry() = default;

    /// Prevents copying analyzer ownership.
    AnalyzerRegistry(const AnalyzerRegistry&) = delete;

    /// Transfers analyzer ownership without copying registrations.
    AnalyzerRegistry(AnalyzerRegistry&&) noexcept = default;

    /// Prevents copying analyzer ownership by assignment.
    AnalyzerRegistry& operator=(const AnalyzerRegistry&) = delete;

    /// Transfers analyzer ownership by move assignment.
    AnalyzerRegistry& operator=(AnalyzerRegistry&&) noexcept = default;

    /// Registers an analyzer and rejects duplicate names.
    void register_analyzer(std::unique_ptr<Analyzer> analyzer) {
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

    /// Returns analyzers in registration order.
    [[nodiscard]] const std::vector<std::unique_ptr<Analyzer>>& analyzers() const noexcept {
        return analyzers_;
    }

private:
    std::vector<std::unique_ptr<Analyzer>> analyzers_;
};

} // namespace recode::analyzer
