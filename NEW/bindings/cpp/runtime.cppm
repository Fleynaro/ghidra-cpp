export module ghidra.bindings.cpp.runtime;

import std;
import ghidra.core;
import ghidra.bindings.cpp.project;
import ghidra.runtime.project.config;
import ghidra.runtime.project.runtime_core;

export namespace ghidra::bindings::cpp {

namespace runtime = ghidra::runtime;
namespace core = ghidra::core;

/// Thread-safe native runtime handle that owns the shared C++ runtime instance.
class Runtime final : public std::enable_shared_from_this<Runtime> {
public:
    /// Creates a runtime with a bounded shared worker pool.
    static std::shared_ptr<Runtime> create(runtime::project::RuntimeConfig config = {}) {
        return std::shared_ptr<Runtime>(new Runtime(std::make_shared<runtime::project::RuntimeCore>(config)));
    }

    /// Opens one project and returns a facade-only native project handle.
    [[nodiscard]] core::Result<std::shared_ptr<Project>> open(runtime::project::ProjectConfig config) {
        auto facade = runtime::api::ProjectFacade::open(runtime_, std::move(config));
        if (!facade)
            return std::unexpected(facade.error());
        return std::shared_ptr<Project>(new Project(std::move(*facade)));
    }

    /// Shuts down projects and worker threads deterministically.
    void shutdown() noexcept {
        runtime_->shutdown();
    }

private:
    /// Stores the process-owned runtime implementation.
    explicit Runtime(std::shared_ptr<runtime::project::RuntimeCore> runtime) : runtime_(std::move(runtime)) {}

    std::shared_ptr<runtime::project::RuntimeCore> runtime_;
};

} // namespace ghidra::bindings::cpp
