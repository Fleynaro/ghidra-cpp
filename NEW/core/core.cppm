// Aggregates the public core modules used by runtime and service consumers.
// The module preserves the stable `recode.core` import used throughout the
// project while keeping each domain, contract, and event module independent.
export module recode.core;

export import recode.core.domain;
export import recode.core.contracts;
export import recode.core.events;
