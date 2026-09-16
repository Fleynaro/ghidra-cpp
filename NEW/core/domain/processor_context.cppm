export module ghidra.core.processor_context;

import std;

export namespace ghidra::core {

/// Stores named immutable Sleigh context values for reproducible decode requests.
struct ProcessorContext {
    std::vector<std::pair<std::string, std::uint64_t>> values;

    /// Finds a context value by Sleigh field name.
    [[nodiscard]] std::optional<std::uint64_t> value(std::string_view name) const {
        const auto iterator = std::ranges::find_if(values, [&](const auto& item) { return item.first == name; });
        return iterator == values.end() ? std::nullopt : std::optional{iterator->second};
    }
};

/// Builds a processor context before a decode request is sealed.
class ProcessorContextBuilder final {
public:
    /// Adds or replaces a named context value.
    void set(std::string name, std::uint64_t value) {
        const auto iterator = std::ranges::find_if(values_, [&](const auto& item) { return item.first == name; });
        if (iterator == values_.end())
            values_.emplace_back(std::move(name), value);
        else
            iterator->second = value;
    }

    /// Seals the builder into an immutable request value.
    [[nodiscard]] ProcessorContext build() && {
        return ProcessorContext{std::move(values_)};
    }

private:
    std::vector<std::pair<std::string, std::uint64_t>> values_;
};

} // namespace ghidra::core
