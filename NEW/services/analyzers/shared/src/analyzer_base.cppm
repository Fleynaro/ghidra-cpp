export module analyzer_base;

import analyzer_cancellation_token;
import analyzer_context;
import analyzer_types;
import std;

// MSVC does not emit inline definitions from independently compiled named
// module interfaces unless the owning class is explicitly exported.
#if defined(_MSC_VER)
#define GHIDRA_ANALYZER_MODULE_EXPORT __declspec(dllexport)
#else
#define GHIDRA_ANALYZER_MODULE_EXPORT
#endif

export namespace ghidra::analyzer {

/// Defines the analyzer callback implemented by every analysis feature.
// Models the lifecycle and callback contract used by
// Ghidra/Framework/Project/src/main/java/ghidra/app/services/AbstractAnalyzer.java.
export class GHIDRA_ANALYZER_MODULE_EXPORT Analyzer {
public:
    /// Releases the polymorphic analyzer instance.
    virtual ~Analyzer() = default;

    /// Returns the stable name, priority, and event contract.
    [[nodiscard]] virtual AnalyzerDescriptor descriptor() const = 0;

    /// Processes one coalesced event task.
    virtual void analyze(AnalysisContext& context, std::span<const AnalysisEvent> events,
                         CancellationToken& cancellation) = 0;

    /// Processes removed state addresses; the default preserves add-only analyzers.
    virtual void removed(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) {}

    /// Receives the end-of-run lifecycle callback after all eligible tasks drain.
    virtual void analysis_ended(AnalysisContext&, bool) {}
};

} // namespace ghidra::analyzer
