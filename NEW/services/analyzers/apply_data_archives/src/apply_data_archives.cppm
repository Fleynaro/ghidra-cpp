export module analyzer_apply_data_archives;

import analyzer;
import std;

// Original source:
// Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ApplyDataArchiveAnalyzer.java

/// Selects and validates user datatype archives through the native archive state boundary.
export namespace recode::analyzer {
class ApplyDataArchivesAnalyzer final : public Analyzer {
public:
    /// Returns the Function ID analysis successor priority and memory event contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Records explicit archive selections and clear validation/application diagnostics.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Validates one archive path without reading an unsupported private Ghidra GDT format.
[[nodiscard]] DataArchiveRecord validate_archive(const std::filesystem::path& path, std::string_view source_language) {
    DataArchiveRecord record;
    record.path = path;
    record.name = path.filename().string();
    record.source_language = std::string(source_language);
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) {
        record.error = "Archive path is not a readable regular file";
        return record;
    }
    if (path.extension() != ".gdt") {
        record.error = "Archive is not a Ghidra .gdt file";
        return record;
    }
    const auto size = std::filesystem::file_size(path, error);
    if (error || size == 0U) {
        record.error = "Archive is empty or its size could not be read";
        return record;
    }
    record.error = "Native DataTypeManagerService is unavailable; archive selection was recorded but not applied";
    return record;
}

} // namespace

/// Returns ApplyDataArchiveAnalyzer's `FUNCTION_ID_ANALYSIS.after()` priority and default-disabled contract.
AnalyzerDescriptor ApplyDataArchivesAnalyzer::descriptor() const {
    return {"Apply Data Archives", 801, {EventKind::memory_added}, {}};
}

/// Performs path validation and records each unique archive through the existing public method.
void ApplyDataArchivesAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                        CancellationToken& cancellation) {
    if (!context.options().apply_data_archives) {
        return;
    }
    for (const auto& path : context.options().data_archive_paths) {
        if (cancellation.is_cancelled()) {
            return;
        }
        static_cast<void>(context.add_data_archive(validate_archive(path, context.options().source_language)));
    }
}

} // namespace recode::analyzer
