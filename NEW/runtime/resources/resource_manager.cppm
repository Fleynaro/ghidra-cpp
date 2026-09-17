export module recode.runtime.resources.manager;

import std;
import recode.core;

export namespace recode::runtime::resources {

namespace core = recode::core;

/// Verifies and identifies immutable analyzer resources without owning service state.
class ResourceManager final : public core::contracts::IResourceManager {
public:
    /// Constructs a manager from a fixed configured resource identity set.
    explicit ResourceManager(std::vector<core::ResourceIdentity> resources) : resources_(std::move(resources)) {}

    /// Verifies that a required resource exists and still matches its recorded size/format identity.
    [[nodiscard]] core::Result<void> require(const core::ResourceIdentity& identity) const override {
        if (!identity.managed_location)
            return {};
        std::error_code error;
        const auto size = std::filesystem::file_size(*identity.managed_location, error);
        if (error || size != identity.byte_size)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::resource_mismatch,
                                  "Configured resource is missing or has changed: " + *identity.managed_location,
                                  "Restore the exact resource version recorded by the project."));
        return {};
    }

    /// Returns a deterministic resource-set identity in configuration order.
    [[nodiscard]] core::ResourceSetIdentity resource_set() const override {
        std::string digest;
        for (const auto& resource : resources_)
            digest += resource.kind + ":" + resource.logical_id + ":" + resource.sha256 + ":" +
                      std::to_string(resource.byte_size) + ";";
        return core::ResourceSetIdentity{std::to_string(core::events::checksum(digest)), resources_};
    }

private:
    std::vector<core::ResourceIdentity> resources_;
};

} // namespace recode::runtime::resources
