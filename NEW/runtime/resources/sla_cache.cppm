export module recode.runtime.resources.sla_cache;

import std;
import recode.core;

export namespace recode::runtime::resources {

/// Caches the validated location/identity of one compiled SLA resource.
class SlaCache final {
public:
    /// Records the SLA resource identity after project validation.
    void set(core::ResourceIdentity identity) {
        identity_ = std::move(identity);
    }

    /// Returns the currently leased SLA identity.
    [[nodiscard]] std::optional<core::ResourceIdentity> identity() const {
        return identity_;
    }

private:
    std::optional<core::ResourceIdentity> identity_;
};

} // namespace recode::runtime::resources
