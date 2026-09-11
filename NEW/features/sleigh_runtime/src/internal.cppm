/// Primary private module interface. Each runtime concern is kept in its own
/// self-contained partition while this interface re-exports the complete
/// implementation to the adapter.
export module sleigh_runtime:internal;

export import :types;
export import :error;
export import :partmap;
export import :opcodes;
export import :marshal;
export import :compression;
export import :space;
export import :address;
export import :varnode;
export import :loadimage;
export import :translate;
export import :globalcontext;
export import :context;
export import :slghpattern;
export import :slghpatexpress;
export import :slaformat;
export import :semantics;
export import :slghsymbol;
export import :sleighbase;
export import :sleigh;
