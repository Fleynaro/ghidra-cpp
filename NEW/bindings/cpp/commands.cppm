export module recode.bindings.cpp.commands;

export import recode.core.contracts.command;

export namespace recode::bindings::cpp {

/// Names the typed command request exposed by the native facade.
using Command = recode::core::contracts::CommandRequest;
/// Names the transient command response exposed by the native facade.
using CommandResponse = recode::core::contracts::CommandResponse;

} // namespace recode::bindings::cpp
