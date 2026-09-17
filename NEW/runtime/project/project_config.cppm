export module recode.runtime.project.config;

import std;
import recode.core;

export namespace recode::runtime::project {

/// Stores durable project-open options and the primary input identity.
struct ProjectConfig {
    core::ProjectId id;
    std::filesystem::path directory;
    core::BinaryArtifact primary_artifact;
    core::contracts::LoadOptions load_options;
    std::filesystem::path sleigh_specification;
    std::filesystem::path function_id_database;
    bool auto_recover{true};
};

/// Identifies the project resource set captured by one load operation.
struct ProjectResources {
    core::ResourceSetIdentity identity;
    std::string language_id;
    std::string compiler_spec_id;
};

} // namespace recode::runtime::project
