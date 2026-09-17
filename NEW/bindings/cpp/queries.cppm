export module recode.bindings.cpp.queries;

import std;
import recode.core;

export namespace recode::bindings::cpp {

/// Stable synchronous read facade over a revision-stamped project query.
class ProjectView final {
public:
    /// Captures the immutable query interface used by this view.
    explicit ProjectView(std::shared_ptr<const core::contracts::IProjectQuery> query) : query_(std::move(query)) {}

    /// Returns the revision represented by the view.
    [[nodiscard]] core::Revision revision() const {
        return query_->current_revision();
    }

    /// Returns current functions as copied canonical values.
    [[nodiscard]] std::vector<core::FunctionSnapshot> functions() const {
        return query_->functions();
    }

    /// Returns current instructions as copied canonical values.
    [[nodiscard]] std::vector<core::Instruction> instructions() const {
        return query_->instructions();
    }

private:
    std::shared_ptr<const core::contracts::IProjectQuery> query_;
};

} // namespace recode::bindings::cpp
