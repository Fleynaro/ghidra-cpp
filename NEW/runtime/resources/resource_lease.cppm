export module recode.runtime.resources.lease;

import std;
import recode.core;

export namespace recode::runtime::resources {

/// Keeps an immutable resource identity alive for one operation lifetime.
class ResourceLease final {
public:
    /// Captures a resource identity value without opening mutable handles.
    explicit ResourceLease(core::ResourceSetIdentity identity) : identity_(std::move(identity)) {}

    /// Returns the immutable identity used by the operation.
    [[nodiscard]] const core::ResourceSetIdentity& identity() const noexcept {
        return identity_;
    }

private:
    core::ResourceSetIdentity identity_;
};

} // namespace recode::runtime::resources
