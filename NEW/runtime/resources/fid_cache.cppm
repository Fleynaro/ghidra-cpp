export module ghidra.runtime.resources.fid_cache;

import std;
import ghidra.core;

export namespace ghidra::runtime::resources {

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

} // namespace ghidra::runtime::resources
