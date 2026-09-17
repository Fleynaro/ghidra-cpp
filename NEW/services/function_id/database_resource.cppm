export module recode.service.function_id.database_resource;

import std;
import recode.core;

export namespace recode::services::function_id {

namespace core = recode::core;

/// Identifies one packed FID database before native parsing/lease creation.
struct DatabaseResource {
    std::filesystem::path path;
    core::ResourceIdentity identity;
};

} // namespace recode::services::function_id
