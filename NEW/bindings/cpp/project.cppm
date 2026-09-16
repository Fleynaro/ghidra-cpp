export module ghidra.bindings.cpp.project;

import std;
import ghidra.core;
import ghidra.runtime.api.project;
import ghidra.runtime.project.config;
import ghidra.bindings.cpp.commands;
import ghidra.bindings.cpp.queries;
import ghidra.bindings.cpp.results;

export namespace ghidra::bindings::cpp {

namespace runtime = ghidra::runtime;

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

} // namespace ghidra::bindings::cpp
