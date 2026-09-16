export module ghidra.core.events.project;

import std;
import ghidra.core.binary;
import ghidra.core.events.event;
import ghidra.core.identifiers;

export namespace ghidra::core::events {

/// Creates the immutable project creation event.
[[nodiscard]] inline EventDraft project_created(const ProjectId& project, const CorrelationId& correlation) {
    return EventDraft{
        project,     "project",    project.value(), "ProjectCreated",  1,
        correlation, std::nullopt, "runtime",       "project-created", encode_fields({{"project", project.value()}})};
}

/// Creates an input-manifest update event with reproducibility metadata.
[[nodiscard]] inline EventDraft project_inputs_changed(const ProjectId& project, const BinaryArtifact& artifact,
                                                       const CorrelationId& correlation) {
    return EventDraft{project,
                      "project",
                      project.value(),
                      "ProjectInputsChanged",
                      1,
                      correlation,
                      std::nullopt,
                      "pe_loader",
                      "inputs-" + artifact.sha256,
                      encode_fields({{"artifact", artifact.id.value()},
                                     {"name", artifact.display_name},
                                     {"format", artifact.format},
                                     {"locator", artifact.locator},
                                     {"sha256", artifact.sha256},
                                     {"size", std::to_string(artifact.byte_size)}})};
}

} // namespace ghidra::core::events
