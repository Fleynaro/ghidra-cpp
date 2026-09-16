export module ghidra.core.events.symbol;

import std;
import ghidra.core.events.event;
import ghidra.core.identifiers;
import ghidra.core.symbol;

export namespace ghidra::core::events {

/// Creates a symbol upsert event with source-priority information.
[[nodiscard]] inline EventDraft symbol_state_changed(const ProjectId& project, const Symbol& symbol,
                                                     const CorrelationId& correlation) {
    return EventDraft{project,
                      "symbol",
                      symbol.id.value(),
                      "SymbolStateChanged",
                      1,
                      correlation,
                      std::nullopt,
                      "symbol",
                      "symbol-" + symbol.id.value(),
                      encode_fields({{"id", symbol.id.value()},
                                     {"address", symbol.address ? std::to_string(symbol.address->offset) : ""},
                                     {"name", symbol.name},
                                     {"namespace", symbol.namespace_name},
                                     {"priority", std::to_string(symbol.source_priority)}})};
}

} // namespace ghidra::core::events
