export module recode.runtime.resources.fid_cache;

import std;
import recode.core;

export namespace recode::runtime::resources {

/// Caches the exact Function ID database identity used by one project.
class FidCache final {
public:
    /// Records the validated packed database identity.
    void set(core::ResourceIdentity identity) {
        identity_ = std::move(identity);
    }

    /// Returns the current Function ID resource identity.
    [[nodiscard]] std::optional<core::ResourceIdentity> identity() const {
        return identity_;
    }

private:
    std::optional<core::ResourceIdentity> identity_;
};

} // namespace recode::runtime::resources
