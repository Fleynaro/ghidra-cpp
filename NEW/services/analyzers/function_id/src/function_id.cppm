export module analyzer_function_id;

import analyzer;
import function_id;
import std;

// Original sources:
// Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/analyzer/FidAnalyzer.java
// Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/cmd/ApplyFidEntriesCommand.java
// Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidService.java

/// Matches decoded native function bodies against the existing packed Function ID databases.
export namespace recode::analyzer {
class FunctionIdAnalyzer final : public Analyzer {
public:
    /// Returns the byte-analysis priority immediately before Function ID analysis.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Hashes eligible functions, queries configured databases, and applies native labels/bookmarks.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;

private:
    /// Identifies one immutable database file for cache invalidation.
    struct DatabaseFileState {
        std::filesystem::path path;
        std::uintmax_t size{};
        std::filesystem::file_time_type modified{};

        /// Compares path and filesystem metadata without opening the database.
        friend bool operator==(const DatabaseFileState&, const DatabaseFileState&) = default;
    };

    /// Reuses immutable parsed databases until configured paths or file metadata change.
    [[nodiscard]] bool ensure_databases(const std::vector<std::filesystem::path>&, CancellationToken&);

    std::vector<DatabaseFileState> database_files_;
    std::vector<fid::Database> databases_;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Adds every configured `.fidb` file or directory entry to the query list.
void collect_database_paths(const AnalysisContext& context, std::vector<std::filesystem::path>& result) {
    const auto add_path = [&](const std::filesystem::path& path) {
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error) && path.extension() == ".fidb") {
            result.push_back(path);
        } else if (!error && std::filesystem::is_directory(path, error)) {
            std::filesystem::directory_iterator entries(path, error);
            for (const auto& entry : entries) {
                std::error_code entry_error;
                if (entry.is_regular_file(entry_error) && !entry_error && entry.path().extension() == ".fidb") {
                    result.push_back(entry.path());
                }
            }
        }
    };
    if (context.options().fid_database_paths.empty()) {
#if defined(ANALYZER_FUNCTION_ID_DATA_DIR)
        add_path(std::filesystem::path{ANALYZER_FUNCTION_ID_DATA_DIR});
#endif
    } else {
        for (const auto& path : context.options().fid_database_paths) {
            add_path(path);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
}

/// Converts PE base-relocation evidence into the existing Function ID relocation type.
[[nodiscard]] std::vector<fid::Relocation> relocations(const AnalysisContext& context) {
    std::vector<fid::Relocation> result;
    for (const auto& block : context.image().relocations()) {
        for (const auto& entry : block.entries) {
            if (entry.type == 0) {
                continue;
            }
            // IMAGE_REL_BASED_HIGH/LOW are 16-bit relocations, HIGHLOW and HIGHADJ
            // cover 32-bit fields, and DIR64 is the only 64-bit base relocation.
            const auto width = entry.type == 1 || entry.type == 2   ? 2U
                               : entry.type == 3 || entry.type == 4 ? 4U
                               : entry.type == 10                   ? 8U
                                                                    : 0U;
            if (width == 0U) {
                continue;
            }
            result.push_back(fid::Relocation{entry.target_va, width});
        }
    }
    return result;
}

/// Returns the Function ID language identifier used by Ghidra's database filters.
[[nodiscard]] std::string language_id(const AnalysisContext& context) {
    return context.image().coff_header().machine == pe::Machine::amd64 ? "x86:LE:64:default" : "x86:LE:32:default";
}

/// Applies one best match using the native context's existing symbol and bookmark mutations.
void apply_match(AnalysisContext& context, const Function& function, const fid::IdentificationResult& result) {
    if (result.names.empty()) {
        return;
    }
    const auto limit = std::min<std::size_t>(context.options().function_id_maximum_matches, result.names.size());
    const auto multiple = result.names.size() > 1U;
    const auto prefix = multiple ? "Library Function - Multiple Matches,  " : "Library Function - Single Match,  ";
    for (std::size_t index = 0; index < limit; ++index) {
        static_cast<void>(context.add_symbol(
            SymbolRecord{function.entry, function.name, result.names[index], {}, "function_id", false, index == 0}));
    }
    if (!multiple || (!result.matches.empty() && result.matches.front().overall_score() >= 30.0F)) {
        static_cast<void>(context.set_function_name(function.entry, result.names.front()));
    }
    if (context.options().create_analysis_bookmarks) {
        static_cast<void>(context.add_bookmark(
            Bookmark{function.entry, "Function ID Analyzer", std::string(prefix) + result.names.front()}));
    }
}

} // namespace

/// Returns FidAnalyzer's `FUNCTION_ID_ANALYSIS.before()` priority and function lifecycle events.
AnalyzerDescriptor FunctionIdAnalyzer::descriptor() const {
    return {"Function ID", 799, {EventKind::function_added, EventKind::function_changed}, {}};
}

/// Uses the existing autonomous Function ID hasher/database instead of duplicating either subsystem.
void FunctionIdAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                 CancellationToken& cancellation) {
    if (!context.options().function_id) {
        return;
    }
    std::vector<std::filesystem::path> paths;
    collect_database_paths(context, paths);
    if (paths.empty()) {
        return;
    }
    if (!ensure_databases(paths, cancellation)) {
        return;
    }
    const auto relocation_list = relocations(context);
    const fid::ProgramInfo program{{language_id(context)}, std::nullopt, std::nullopt, false};
    for (const auto& [entry, function] : context.functions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (function.external ||
            function.instruction_starts.size() < context.options().function_id_minimum_instructions) {
            continue;
        }
        std::vector<sleigh_runtime::Instruction> instructions;
        instructions.reserve(function.instruction_starts.size());
        for (const auto address : function.instruction_starts) {
            const auto instruction = context.instructions().find(address);
            if (instruction == context.instructions().end()) {
                instructions.clear();
                break;
            }
            instructions.push_back(instruction->second.instruction);
        }
        if (instructions.size() < context.options().function_id_minimum_instructions) {
            continue;
        }
        const auto hash = fid::Hasher::hash_sleigh(instructions, relocation_list);
        if (!hash) {
            continue;
        }
        const fid::FunctionContext query{*hash, {}, {}};
        std::vector<std::string> names;
        fid::IdentificationResult best;
        bool has_match = false;
        for (const auto& database : databases_) {
            const auto identified = database.identify(query, program);
            if (!identified || identified->names.empty() || identified->matches.empty()) {
                continue;
            }
            if (!has_match || identified->matches.front().overall_score() > best.matches.front().overall_score()) {
                best = *identified;
                has_match = true;
            }
            names.insert(names.end(), identified->names.begin(), identified->names.end());
        }
        if (!has_match) {
            continue;
        }
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        best.names = std::move(names);
        apply_match(context, function, best);
        (void)entry;
    }
}

/// Reuses parsed immutable Function ID databases while preserving path and file-change behavior.
bool FunctionIdAnalyzer::ensure_databases(const std::vector<std::filesystem::path>& paths,
                                          CancellationToken& cancellation) {
    std::vector<DatabaseFileState> files;
    files.reserve(paths.size());
    for (const auto& path : paths) {
        std::error_code size_error;
        std::error_code time_error;
        const auto size = std::filesystem::file_size(path, size_error);
        const auto modified = std::filesystem::last_write_time(path, time_error);
        files.push_back(
            DatabaseFileState{path, size_error ? 0U : size, time_error ? std::filesystem::file_time_type{} : modified});
    }
    if (files == database_files_ && !databases_.empty())
        return true;

    std::vector<std::future<std::expected<fid::Database, fid::Error>>> pending;
    pending.reserve(paths.size());
    for (const auto& path : paths) {
        if (cancellation.is_cancelled())
            return false;
        pending.push_back(std::async(std::launch::async, [path] { return fid::Database::open(path); }));
    }

    std::vector<fid::Database> loaded;
    loaded.reserve(paths.size());
    for (auto& future : pending) {
        auto database = future.get();
        if (database)
            loaded.push_back(std::move(*database));
    }
    if (cancellation.is_cancelled())
        return false;
    database_files_ = std::move(files);
    databases_ = std::move(loaded);
    return !databases_.empty();
}

} // namespace recode::analyzer
