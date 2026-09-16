export module ghidra.service.function_id.database_resource;

import std;
import ghidra.core;

export namespace ghidra::services::function_id {

namespace core = ghidra::core;

/// Identifies one packed FID database before native parsing/lease creation.
struct DatabaseResource {
    std::filesystem::path path;
    core::ResourceIdentity identity;
};

} // namespace ghidra::services::function_id
