export module ghidra.core.relocation;

import std;
import ghidra.core.address;

export namespace ghidra::core {

/// Represents loader-neutral relocation evidence.
struct Relocation {
    Address target;
    std::uint32_t width{};
    std::string type;
    std::int64_t addend{};
    std::string source_loader;
};

} // namespace ghidra::core
