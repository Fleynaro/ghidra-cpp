export module recode.runtime.projections.diagnostics;

import std;
import recode.core;
import recode.core.diagnostics;

export namespace recode::runtime::projections {

namespace core = recode::core;

/// Retains runtime diagnostics as a bounded current query list.
class DiagnosticsProjection final {
public:
    /// Appends one diagnostic and removes the oldest entry beyond the bound.
    void add(core::Diagnostic diagnostic) {
        diagnostics_.push_back(std::move(diagnostic));
        if (diagnostics_.size() > 1024)
            diagnostics_.erase(diagnostics_.begin());
    }

    /// Returns a copy of all retained diagnostics.
    [[nodiscard]] std::vector<core::Diagnostic> diagnostics() const {
        return diagnostics_;
    }

private:
    std::vector<core::Diagnostic> diagnostics_;
};

} // namespace recode::runtime::projections
