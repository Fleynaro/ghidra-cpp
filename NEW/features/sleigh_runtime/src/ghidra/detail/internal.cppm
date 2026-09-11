/// Primary private module interface. Every runtime implementation unit is
/// attached to this module so cross-cutting legacy forward declarations retain
/// one module ownership while each concern remains a named partition.
export module sleigh_runtime.ghidra;

export import :types;
export import :error;
export import :partmap;
export import :xml;
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
