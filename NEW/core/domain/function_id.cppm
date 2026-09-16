export module ghidra.core.function_id;

import std;
import ghidra.core.analysis_fact;
import ghidra.core.function;
import ghidra.core.identifiers;

export namespace ghidra::core {

/// Captures parent/child hashes used by relation-aware Function ID scoring.
struct FunctionHashFamily {
    std::string full_hash;
    std::string specific_hash;
    std::vector<std::string> child_hashes;
    std::vector<std::string> parent_hashes;
};

/// Stores one immutable Function ID candidate and score evidence.
struct FunctionIdCandidate {
    std::string library;
    std::string name;
    double score{};
    std::string database;
    bool trusted{};
};

/// Selects thresholds, language filters, and relation policy for a query.
struct FunctionIdOptions {
    double label_threshold{0.0};
    double bookmark_threshold{0.0};
    bool force_relations{};
    std::vector<std::string> language_filters;
    std::vector<std::string> database_filters;
};

/// Returns a structured result without directly renaming a project function.
struct FunctionIdResult {
    FunctionKey function;
    FunctionHashFamily hashes;
    std::vector<FunctionIdCandidate> candidates;
    std::optional<FunctionIdCandidate> selected;
    bool label_threshold_met{};
    AnalysisEvidence evidence;
};

} // namespace ghidra::core
