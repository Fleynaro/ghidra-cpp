export module recode.core.binary;

import std;
import recode.core.bytes;
import recode.core.identifiers;

export namespace recode::core {

/// Identifies a resource by content and parser metadata for reproducible analysis.
struct ResourceIdentity {
    std::string kind;
    std::string logical_id;
    std::string sha256;
    std::uint64_t byte_size{};
    std::string format_version;
    std::string producer_version;
    bool required{};
    std::optional<std::string> managed_location;
};

/// Names the ordered resource set used for one project operation.
struct ResourceSetIdentity {
    std::string digest;
    std::vector<ResourceIdentity> resources;
};

/// Identifies the primary input artifact without owning an open file handle.
struct BinaryArtifact {
    ArtifactId id;
    std::string display_name;
    std::string format;
    std::string locator;
    std::uint64_t byte_size{};
    std::string sha256;
    std::string architecture_hint;
    bool primary{};
};

} // namespace recode::core
