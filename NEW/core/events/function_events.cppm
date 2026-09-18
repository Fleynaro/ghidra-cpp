export module recode.core.events.function;

import std;
import recode.core.events.event;
import recode.core.function;
import recode.core.identifiers;

export namespace recode::core::events {

/// Creates a function snapshot upsert event.
[[nodiscard]] inline EventDraft function_state_changed(const ProjectId& project, const FunctionSnapshot& function,
                                                       const CorrelationId& correlation,
                                                       std::string source = "analysis") {
    std::string end = "0";
    if (!function.body.ranges().empty()) {
        // CFG decoding can insert a backward branch target before the entry
        // range; the event end is the maximum body endpoint, not the first
        // range's endpoint.
        const auto maximum = std::ranges::max_element(function.body.ranges(), [](const auto& left, const auto& right) {
            return left.end.offset < right.end.offset;
        });
        end = std::to_string(maximum->end.offset);
    }
    return EventDraft{project,
                      "function",
                      function.key.entity.value(),
                      "FunctionStateChanged",
                      1,
                      correlation,
                      std::nullopt,
                      source,
                      "function-" + function.key.entity.value() + "-" + function.name + "-" + source,
                      encode_fields({{"id", function.key.entity.value()},
                                     {"space", function.key.entry.space.name()},
                                     {"entry", std::to_string(function.key.entry.offset)},
                                     {"name", function.name},
                                     {"end", end},
                                     {"status", function.analysis_status}})};
}

} // namespace recode::core::events
