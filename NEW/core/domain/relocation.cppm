export module recode.core.relocation;

import std;
import recode.core.address;

export namespace recode::core {

/// Represents loader-neutral relocation evidence.
struct Relocation {
    Address target;
    std::uint32_t width{};
    std::string type;
    std::int64_t addend{};
    std::string source_loader;
};

} // namespace recode::core
