export module recode.runtime.analysis.registry;

import std;
import recode.core;

export namespace recode::runtime::analysis {

namespace core = recode::core;

/// Owns analyzer registration, descriptor uniqueness, and dependency ordering.
class AnalyzerRegistry final {
public:
    /// Registers one analyzer by stable ID and rejects duplicates.
    [[nodiscard]] core::Result<void> register_analyzer(std::shared_ptr<core::contracts::IAnalyzer> analyzer) {
        if (!analyzer)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Cannot register a null analyzer"));
        const auto descriptor = analyzer->descriptor();
        if (descriptor.stable_id.empty())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Analyzer stable ID must not be empty"));
        if (analyzers_.contains(descriptor.stable_id))
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::conflict, "Analyzer stable ID is already registered"));
        analyzers_.emplace(descriptor.stable_id, Entry{std::move(analyzer), descriptor});
        return {};
    }

    /// Resolves prerequisites into deterministic priority/ID execution order.
    [[nodiscard]] core::Result<std::vector<std::shared_ptr<core::contracts::IAnalyzer>>> ordered() const {
        std::map<std::string, std::size_t> indegree;
        std::map<std::string, std::vector<std::string>> outgoing;
        for (const auto& [id, entry] : analyzers_)
            indegree[id] = 0;
        for (const auto& [id, entry] : analyzers_) {
            for (const auto& prerequisite : entry.descriptor.prerequisites) {
                if (!analyzers_.contains(prerequisite))
                    return std::unexpected(
                        core::Error::make(core::DiagnosticCode::invalid_argument,
                                          "Analyzer prerequisite is not registered: " + prerequisite));
                outgoing[prerequisite].push_back(id);
                ++indegree[id];
            }
        }
        std::set<std::pair<std::int32_t, std::string>> ready;
        for (const auto& [id, degree] : indegree)
            if (degree == 0)
                ready.emplace(analyzers_.at(id).descriptor.priority, id);
        std::vector<std::shared_ptr<core::contracts::IAnalyzer>> result;
        while (!ready.empty()) {
            const auto [unused_priority, id] = *ready.begin();
            ready.erase(ready.begin());
            result.push_back(analyzers_.at(id).analyzer);
            for (const auto& dependent : outgoing[id]) {
                if (--indegree[dependent] == 0)
                    ready.emplace(analyzers_.at(dependent).descriptor.priority, dependent);
            }
        }
        if (result.size() != analyzers_.size())
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::conflict, "Analyzer prerequisites contain a cycle"));
        return result;
    }

    /// Returns descriptor metadata without transferring analyzer ownership.
    [[nodiscard]] std::optional<core::contracts::AnalyzerDescriptor> descriptor(std::string_view stable_id) const {
        const auto iterator = analyzers_.find(std::string(stable_id));
        return iterator == analyzers_.end() ? std::nullopt : std::optional{iterator->second.descriptor};
    }

private:
    /// Stores one analyzer implementation and its immutable scheduler descriptor.
    struct Entry {
        std::shared_ptr<core::contracts::IAnalyzer> analyzer;
        core::contracts::AnalyzerDescriptor descriptor;
    };

    std::map<std::string, Entry> analyzers_;
};

} // namespace recode::runtime::analysis
