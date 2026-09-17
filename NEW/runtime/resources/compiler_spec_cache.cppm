export module recode.runtime.resources.compiler_spec_cache;

import std;
import recode.core;

export namespace recode::runtime::resources {

/// Caches a compiler-spec identity without exposing parsed XML ownership.
class CompilerSpecCache final {
public:
    /// Records the validated compiler-spec resource identity.
    void set(core::ResourceIdentity identity) {
        identity_ = std::move(identity);
    }

    /// Returns the current compiler-spec identity.
    [[nodiscard]] std::optional<core::ResourceIdentity> identity() const {
        return identity_;
    }

private:
    std::optional<core::ResourceIdentity> identity_;
};

} // namespace recode::runtime::resources
