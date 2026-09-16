export module ghidra.bindings.cpp.commands;

export import ghidra.core.contracts.command;

export namespace ghidra::bindings::cpp {

/// Names the typed command request exposed by the native facade.
using Command = ghidra::core::contracts::CommandRequest;
/// Names the transient command response exposed by the native facade.
using CommandResponse = ghidra::core::contracts::CommandResponse;

} // namespace ghidra::bindings::cpp
