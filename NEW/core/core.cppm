// Aggregates the public core modules used by runtime and service consumers.
// The module preserves the stable `ghidra.core` import used throughout the
// project while keeping each domain, contract, and event module independent.
export module ghidra.core;

export import ghidra.core.domain;
export import ghidra.core.contracts;
export import ghidra.core.events;
