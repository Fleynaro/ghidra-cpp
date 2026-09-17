module;

#include <gtest/gtest.h>
#include <sqlite3.h>

export module recode.tests.integration;

import recode.core;
import recode.runtime.api.project;
import recode.runtime.project.config;
import recode.runtime.project.runtime_core;
import recode.runtime.project.state;
import recode.runtime.workers.pool;
import std;

namespace recode::tests::integration {
namespace {

namespace api = recode::runtime::api;
namespace project = recode::runtime::project;
namespace workers = recode::runtime::workers;

/// Stores the rows needed to make the durable projection and decompiler output inspectable.
struct SqliteReportData {
    std::uint64_t checkpoint{};
    std::vector<std::vector<std::string>> event_types;
    std::vector<std::vector<std::string>> memory_regions;
    std::vector<std::vector<std::string>> functions;
    std::vector<std::vector<std::string>> instructions;
    std::vector<std::vector<std::string>> symbols;
    std::vector<std::vector<std::string>> analysis_runs;
    std::vector<std::vector<std::string>> references;
    std::vector<std::vector<std::string>> data_objects;
};

/// Reads a parameterized SQLite query into owned text rows for report generation.
[[nodiscard]] std::expected<std::vector<std::vector<std::string>>, std::string>
read_sqlite_rows(const std::filesystem::path& path, std::string_view sql,
                 std::span<const std::string> parameters = {}) {
    sqlite3* database{};
    if (sqlite3_open_v2(path.string().c_str(), &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, nullptr) !=
        SQLITE_OK) {
        const std::string error = database ? sqlite3_errmsg(database) : "SQLite could not open the report database";
        if (database)
            sqlite3_close(database);
        return std::unexpected(error);
    }
    sqlite3_stmt* statement{};
    if (sqlite3_prepare_v2(database, std::string(sql).c_str(), -1, &statement, nullptr) != SQLITE_OK) {
        const std::string error = sqlite3_errmsg(database);
        sqlite3_close(database);
        return std::unexpected(error);
    }
    for (std::size_t index = 0; index < parameters.size(); ++index)
        sqlite3_bind_text(statement, static_cast<int>(index + 1), parameters[index].c_str(), -1, SQLITE_TRANSIENT);
    std::vector<std::vector<std::string>> rows;
    for (;;) {
        const auto result = sqlite3_step(statement);
        if (result == SQLITE_DONE)
            break;
        if (result != SQLITE_ROW) {
            const std::string error = sqlite3_errmsg(database);
            sqlite3_finalize(statement);
            sqlite3_close(database);
            return std::unexpected(error);
        }
        std::vector<std::string> row;
        const auto columns = sqlite3_column_count(statement);
        row.reserve(static_cast<std::size_t>(columns));
        for (int column = 0; column < columns; ++column) {
            const auto* value = sqlite3_column_text(statement, column);
            row.emplace_back(value ? reinterpret_cast<const char*>(value) : "");
        }
        rows.push_back(std::move(row));
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
    return rows;
}

/// Opens every current projection table after the project has released its SQLite writer.
[[nodiscard]] std::expected<SqliteReportData, std::string> read_projection(const std::filesystem::path& path,
                                                                           const core::ProjectId& project_id) {
    const std::vector<std::string> project{project_id.value()};
    const auto checkpoint_rows =
        read_sqlite_rows(path, "SELECT COALESCE(sequence,0) FROM projection_checkpoint WHERE project_id=?;", project);
    if (!checkpoint_rows)
        return std::unexpected(checkpoint_rows.error());
    if (checkpoint_rows->empty() || checkpoint_rows->front().empty())
        return std::unexpected("SQLite projection has no checkpoint row");
    SqliteReportData result;
    try {
        result.checkpoint = std::stoull(checkpoint_rows->front().front());
    } catch (const std::exception& error) {
        return std::unexpected(std::string("Invalid SQLite checkpoint: ") + error.what());
    }
    const auto query =
        [&](std::string_view statement) -> std::expected<std::vector<std::vector<std::string>>, std::string> {
        return read_sqlite_rows(path, statement);
    };
    auto event_types = query("SELECT event_type,COUNT(*) FROM applied_events GROUP BY event_type ORDER BY event_type;");
    auto memory_regions =
        query("SELECT space,start_address,end_address,name,readable,writable,executable FROM memory_regions ORDER BY "
              "start_address;");
    auto functions = query(
        "SELECT space,entry_address,end_address,name,status,source_service FROM functions ORDER BY entry_address;");
    auto instructions =
        query("SELECT space,address,length,mnemonic,assembly,source_service FROM instructions ORDER BY address;");
    auto symbols = query(
        "SELECT COALESCE(space,''),COALESCE(address,''),name,namespace,priority FROM symbols ORDER BY entity_id;");
    auto analysis_runs = query("SELECT run_id,status,sequence FROM analysis_runs ORDER BY sequence;");
    auto references = query(
        "SELECT source_address,target_address,kind FROM references_projection ORDER BY source_address,target_address;");
    auto data_objects = query("SELECT address,type,value FROM data_objects ORDER BY address;");
    if (!event_types || !memory_regions || !functions || !instructions || !symbols || !analysis_runs || !references ||
        !data_objects)
        return std::unexpected(!event_types      ? event_types.error()
                               : !memory_regions ? memory_regions.error()
                               : !functions      ? functions.error()
                               : !instructions   ? instructions.error()
                               : !symbols        ? symbols.error()
                               : !analysis_runs  ? analysis_runs.error()
                               : !references     ? references.error()
                                                 : data_objects.error());
    result.event_types = std::move(*event_types);
    result.memory_regions = std::move(*memory_regions);
    result.functions = std::move(*functions);
    result.instructions = std::move(*instructions);
    result.symbols = std::move(*symbols);
    result.analysis_runs = std::move(*analysis_runs);
    result.references = std::move(*references);
    result.data_objects = std::move(*data_objects);
    return result;
}

/// Escapes a value for a Markdown table without changing the recorded entity text.
[[nodiscard]] std::string markdown_cell(std::string value) {
    std::string result;
    result.reserve(value.size());
    for (const char character : value) {
        if (character == '|')
            result += "\\|";
        else if (character == '\n' || character == '\r')
            result.push_back(' ');
        else
            result.push_back(character);
    }
    return result;
}

/// Writes one SQLite row collection as a stable Markdown table.
void write_table(std::ofstream& output, std::string_view title, const std::vector<std::string>& headers,
                 const std::vector<std::vector<std::string>>& rows) {
    output << "### " << title << "\n\n|";
    for (const auto& header : headers)
        output << ' ' << header << " |";
    output << "\n|";
    for (std::size_t index = 0; index < headers.size(); ++index)
        output << " --- |";
    output << "\n";
    for (const auto& row : rows) {
        output << '|';
        for (std::size_t index = 0; index < headers.size(); ++index) {
            const auto value = index < row.size() ? row[index] : std::string{};
            output << ' ' << markdown_cell(value) << " |";
        }
        output << '\n';
    }
    if (rows.empty())
        output << "| _none_ |\n";
    output << '\n';
}

/// Converts a decompilation result into a short report record while retaining all generated source text.
struct DecompilationReportData {
    core::FunctionSnapshot function;
    core::Decompilation result;
};

/// Reads a Markdown artifact as bytes so the golden comparison covers content and
/// canonical LF newlines rather than silently normalizing platform line endings.
[[nodiscard]] std::expected<std::string, std::string> read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return std::unexpected("Unable to open Markdown artifact: " + path.string());
    std::ostringstream contents;
    contents << input.rdbuf();
    if (!input.good() && !input.eof())
        return std::unexpected("Unable to read Markdown artifact: " + path.string());
    return contents.str();
}

/// Writes the complete pipeline evidence, projection entities, and decompiler artifacts to Markdown.
[[nodiscard]] std::expected<void, std::string>
write_report(const std::filesystem::path& path, const project::LoadSummary& loaded,
             const project::AnalysisSummary& analyzed,
             const std::shared_ptr<const core::contracts::IProjectQuery>& query, const SqliteReportData& sqlite,
             const std::vector<DecompilationReportData>& decompilations) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return std::unexpected("Unable to create report directory: " + error.message());
    std::ofstream output(path, std::ios::trunc | std::ios::binary);
    if (!output)
        return std::unexpected("Unable to create Markdown report: " + path.string());
    output << "# Full Runtime PE Pipeline Report\n\n"
           << "- **Status:** PASS\n"
           << "- **Fixture:** `services/analyzers/tests/data/test_analyzers_integration.exe`\n"
           << "- **SQLite projection:** `build/test-reports/project/projection.sqlite`\n"
           << "- **Load revision:** " << loaded.revision.value << "\n"
           << "- **Analysis revision:** " << analyzed.committed_revision.value << "\n"
           << "- **SQLite checkpoint:** " << sqlite.checkpoint << "\n\n";
    output << "## Pipeline\n\n"
           << "1. `ProjectFacade::open` created the project and event/projection stores.\n"
           << "2. `ProjectFacade::load` parsed the PE, materialized PE regions/symbols, decoded entry/export seeds, "
              "and committed listing/function events.\n"
           << "3. `ProjectFacade::analyze` executed the registered runtime analyzer(s).\n"
           << "4. Two `Task<Result<Decompilation>>` operations completed through the shared worker pool.\n\n"
           << "### Executed Analyzers\n\n";
    for (const auto& analyzer : analyzed.analyzers)
        output << "- `" << analyzer << "`\n";
    if (analyzed.analyzers.empty())
        output << "- _none_\n";
    output << "\n### Analysis Diagnostics\n\n";
    if (analyzed.diagnostics.empty())
        output << "- _none_\n\n";
    else {
        for (const auto& diagnostic : analyzed.diagnostics)
            output << "- " << diagnostic.message << "\n";
        output << '\n';
    }
    output << "## Coverage Notes\n\n"
           << "- The current native runtime registry executes `runtime.entry_materialization`; the broader legacy "
              "analyzer suite is validated separately by `analyzer_global_integration_tests` and is not implicitly "
              "claimed by this facade test.\n"
           << "- References and data objects are represented by durable tables and are currently empty for this "
              "runtime event set.\n\n";

    output << "## In-Memory Query Projection\n\n"
           << "- Revision: " << query->current_revision().value << "\n"
           << "- Functions: " << query->functions().size() << "\n"
           << "- Instructions: " << query->instructions().size() << "\n"
           << "- Memory regions: " << query->memory_regions().size() << "\n"
           << "- Symbols: " << query->symbols().size() << "\n"
           << "- References: unavailable/empty in the current runtime contract implementation\n"
           << "- Data objects: " << query->data_objects().size() << "\n\n";
    write_table(output, "SQLite Event Types", {"Event type", "Rows"}, sqlite.event_types);
    write_table(output, "SQLite Memory Regions", {"Space", "Start", "End", "Name", "Read", "Write", "Execute"},
                sqlite.memory_regions);
    write_table(output, "SQLite Functions", {"Space", "Entry", "End", "Name", "Status", "Producer"}, sqlite.functions);
    write_table(output, "SQLite Instructions", {"Space", "Address", "Length", "Mnemonic", "Assembly", "Producer"},
                sqlite.instructions);
    write_table(output, "SQLite Symbols", {"Space", "Address", "Name", "Namespace", "Priority"}, sqlite.symbols);
    write_table(output, "SQLite Analysis Runs", {"Run", "Status", "Sequence"}, sqlite.analysis_runs);
    write_table(output, "SQLite References", {"Source", "Target", "Kind"}, sqlite.references);
    write_table(output, "SQLite Data Objects", {"Address", "Type", "Value"}, sqlite.data_objects);

    output << "## Decompilations\n\n";
    for (std::size_t decompilation_index = 0; decompilation_index < decompilations.size(); ++decompilation_index) {
        const auto& decompilation = decompilations[decompilation_index];
        output << "### `0x" << std::hex << decompilation.function.key.entry.offset << std::dec << "` "
               << decompilation.function.name << "\n\n"
               << "- **Status:** "
               << (decompilation.result.status == core::DecompilationStatus::complete ? "complete" : "failed") << "\n"
               << "- **Read revision:** " << decompilation.result.read_revision.value << "\n"
               << "- **Raw instructions:** " << decompilation.result.raw_instructions.size() << "\n\n"
               << "```c\n"
               << decompilation.result.c_source << "\n```\n";
        if (decompilation_index + 1U < decompilations.size())
            output << '\n';
        if (!decompilation.result.diagnostics.empty()) {
            output << "Diagnostics:\n\n";
            for (const auto& diagnostic : decompilation.result.diagnostics)
                output << "- " << diagnostic.message << "\n";
            output << '\n';
        }
    }
    output.flush();
    if (!output)
        return std::unexpected("Unable to flush Markdown report: " + path.string());
    return {};
}

/// Runs the real executable through the public runtime facade, verifies SQLite entities,
/// and records two asynchronous contract-level decompilations for later inspection.
TEST(ArchitectureIntegrationTest, FullPeRuntimePipelineProducesProjectionAndReport) {
    const auto root = std::filesystem::path(RECODE_INTEGRATION_ROOT);
    const auto fixture = root / "services" / "analyzers" / "tests" / "data" / "test_analyzers_integration.exe";
    const auto sla = root / "services" / "sleigh" / "specifications" / "x86-64.sla";
    const auto report = std::filesystem::path(RECODE_INTEGRATION_REPORT_PATH);
    const auto directory = report.parent_path() / "project";
    const auto database = directory / "projection.sqlite";
    std::error_code error;
    std::filesystem::remove_all(report.parent_path(), error);
    ASSERT_TRUE(std::filesystem::exists(fixture)) << "Missing integration PE fixture: " << fixture.string();
    ASSERT_TRUE(std::filesystem::exists(sla)) << "Missing integration SLA fixture: " << sla.string();

    project::ProjectConfig config;
    config.id = core::ProjectId{"architecture-integration"};
    config.directory = directory;
    config.primary_artifact = core::BinaryArtifact{
        core::ArtifactId{"integration-input"}, "fixture.exe", "PE", fixture.string(), 0, "fixture", "x86-64", true};
    config.sleigh_specification = sla;
    auto runtime = std::make_shared<project::RuntimeCore>(project::RuntimeConfig{workers::WorkerPoolConfig{2, 64}});
    auto facade = api::ProjectFacade::open(runtime, config);
    ASSERT_TRUE(facade) << (facade ? "" : facade.error().message);

    const auto loaded = (*facade)->load();
    ASSERT_TRUE(loaded) << (loaded ? "" : loaded.error().message);
    ASSERT_GT(loaded->memory_regions, 0U);
    ASSERT_GT(loaded->decoded_instructions, 0U);

    const auto analyzed = (*facade)->analyze();
    ASSERT_TRUE(analyzed) << (analyzed ? "" : analyzed.error().message);
    ASSERT_FALSE(analyzed->analyzers.empty());
    EXPECT_NE(std::find(analyzed->analyzers.begin(), analyzed->analyzers.end(), "runtime.entry_materialization"),
              analyzed->analyzers.end());

    const auto query = (*facade)->query();
    ASSERT_TRUE(query);
    const auto functions = query->functions();
    ASSERT_GE(functions.size(), 2U) << "PE export/function seeds did not produce two decompilable functions";
    ASSERT_FALSE(query->instructions().empty());
    ASSERT_FALSE(query->memory_regions().empty());
    ASSERT_FALSE(query->symbols().empty());

    std::vector<core::FunctionSnapshot> selected_functions;
    for (const auto name : {std::string_view{"callback_add"}, std::string_view{"callback_rotate"}}) {
        const auto iterator = std::find_if(functions.begin(), functions.end(),
                                           [&](const auto& function) { return function.name == name; });
        if (iterator != functions.end())
            selected_functions.push_back(*iterator);
    }
    if (selected_functions.size() < 2)
        selected_functions.assign(functions.begin(), functions.begin() + 2);

    std::vector<core::contracts::Task<core::Result<core::Decompilation>>> tasks;
    tasks.reserve(2);
    for (std::size_t index = 0; index < 2; ++index) {
        auto task = (*facade)->decompile(selected_functions[index].key);
        ASSERT_TRUE(task.valid());
        ASSERT_TRUE(task.control());
        tasks.push_back(std::move(task));
    }
    std::vector<DecompilationReportData> decompilations;
    for (std::size_t index = 0; index < tasks.size(); ++index) {
        tasks[index].wait();
        ASSERT_EQ(tasks[index].control()->status(), core::contracts::OperationStatus::completed);
        core::Result<core::Decompilation> result;
        try {
            result = tasks[index].get();
        } catch (const std::exception& exception) {
            FAIL() << "Decompilation task threw: " << exception.what();
            return;
        }
        ASSERT_TRUE(result) << (result ? "" : result.error().message);
        ASSERT_EQ(result->status, core::DecompilationStatus::complete)
            << (result->diagnostics.empty() ? "native decompiler returned no diagnostic"
                                            : result->diagnostics.front().message)
            << " function=" << selected_functions[index].name << " entry=0x" << std::hex
            << selected_functions[index].key.entry.offset << std::dec;
        ASSERT_FALSE(result->c_source.empty());
        decompilations.push_back(DecompilationReportData{selected_functions[index], std::move(*result)});
    }

    (*facade)->close();
    runtime->shutdown();
    ASSERT_TRUE(std::filesystem::exists(database)) << "Project close did not leave projection.sqlite for inspection";
    const auto sqlite = read_projection(database, config.id);
    ASSERT_TRUE(sqlite) << (sqlite ? "" : sqlite.error());
    ASSERT_FALSE(sqlite->functions.empty());
    ASSERT_FALSE(sqlite->instructions.empty());
    ASSERT_FALSE(sqlite->memory_regions.empty());
    ASSERT_FALSE(sqlite->symbols.empty());
    ASSERT_FALSE(sqlite->analysis_runs.empty());

    const auto written = write_report(report, *loaded, *analyzed, query, *sqlite, decompilations);
    ASSERT_TRUE(written) << (written ? "" : written.error());
    ASSERT_TRUE(std::filesystem::exists(report));

    const auto golden = std::filesystem::path(RECODE_INTEGRATION_GOLDEN_REPORT_PATH);
    const auto generated_text = read_text_file(report);
    const auto golden_text = read_text_file(golden);
    ASSERT_TRUE(generated_text) << generated_text.error();
    ASSERT_TRUE(golden_text) << golden_text.error();
    ASSERT_EQ(*generated_text, *golden_text)
        << "Generated Markdown differs from the checked-in golden report. Inspect the diff for a logic regression "
           "before updating the baseline: "
        << golden.string();
}

} // namespace
} // namespace recode::tests::integration
