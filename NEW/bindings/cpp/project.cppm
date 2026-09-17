export module recode.bindings.cpp.project;

import std;
import recode.core;
import recode.runtime.api.project;
import recode.runtime.project.config;
import recode.bindings.cpp.commands;
import recode.bindings.cpp.queries;
import recode.bindings.cpp.results;

export namespace recode::bindings::cpp {

namespace runtime = recode::runtime;

/// Stable native project handle used by future language bindings without implementing them here.
class Project final {
public:
    /// Loads the primary executable through the runtime commit path.
    [[nodiscard]] core::Result<LoadResult> load() {
        return facade_->load();
    }

    /// Runs scheduler-backed automatic analysis.
    [[nodiscard]] core::Result<AnalysisResult> analyze() {
        return facade_->analyze();
    }

    /// Returns a stable read-only query view.
    [[nodiscard]] ProjectView view() const {
        return ProjectView{facade_->query()};
    }

    /// Queues decompilation for one selected function.
    [[nodiscard]] core::contracts::Task<core::Result<DecompilationResult>> decompile(core::FunctionKey function) {
        return facade_->decompile(std::move(function));
    }

    /// Executes a typed mutation command through the project commit lane.
    [[nodiscard]] core::Result<CommandResponse> execute(const Command& command) {
        return facade_->execute(command);
    }

    /// Closes the project handle and its manager slot.
    void close() noexcept {
        facade_->close();
    }

private:
    friend class Runtime;

    /// Wraps a facade already opened by the runtime owner.
    explicit Project(std::shared_ptr<runtime::api::ProjectFacade> facade) : facade_(std::move(facade)) {}

    std::shared_ptr<runtime::api::ProjectFacade> facade_;
};

} // namespace recode::bindings::cpp
