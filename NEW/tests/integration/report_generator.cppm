module;

#include <sqlite3.h>

export module recode.tests.integration.report;

import recode.core;
import recode.runtime.api.project;
import recode.runtime.project.config;
import recode.runtime.project.runtime_core;
import recode.runtime.project.state;
import recode.runtime.workers.pool;
import std;

namespace recode::tests::integration::report {

namespace core = recode::core;
namespace api = recode::runtime::api;
namespace project = recode::runtime::project;
namespace workers = recode::runtime::workers;

/// Bounds every potentially large section of a golden report and supplies the
/// per-test seed used by deterministic random sampling.
export struct ReportLimits {
    std::size_t max_functions{50};
    std::size_t max_decompilations{10};
    std::size_t max_instructions{1000};
    std::size_t max_sqlite_instructions{200};
    std::size_t max_memory_regions{500};
    std::size_t max_symbols{500};
    std::size_t max_references{500};
    std::size_t max_data_objects{500};
    std::size_t max_event_types{100};
    std::size_t max_event_records{500};
    std::size_t max_variables{100};
    std::size_t max_text_chars{100};
    std::size_t max_decompilation_chars{10000};
    std::uint64_t seed{0x5245434f44455f31ULL};
};

/// Holds durable projection rows collected after the project releases its
/// SQLite writer connection.
export struct SqliteReportData {
    std::uint64_t checkpoint{};
    std::vector<std::vector<std::string>> event_types;
    std::vector<std::vector<std::string>> event_records;
    std::vector<std::vector<std::string>> memory_regions;
    std::vector<std::vector<std::string>> functions;
    std::vector<std::vector<std::string>> instructions;
    std::vector<std::vector<std::string>> symbols;
    std::vector<std::vector<std::string>> analysis_runs;
    std::vector<std::vector<std::string>> references;
    std::vector<std::vector<std::string>> data_objects;
};

/// Couples one selected function with the complete structured result of its
/// asynchronous decompiler task.
export struct DecompilationReportData {
    core::FunctionSnapshot function;
    core::Decompilation result;
};

/// Describes all runtime and durable facts consumed by the common report writer.
export struct ReportInput {
    std::string test_name;
    std::string fixture_label;
    std::string database_label;
    project::LoadSummary loaded;
    project::AnalysisSummary analyzed;
    std::shared_ptr<const core::contracts::IProjectQuery> query;
    SqliteReportData sqlite;
    std::vector<DecompilationReportData> decompilations;
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

/// Loads every durable projection table in deterministic SQL order for the
/// report contract, including event payloads and currently empty entity kinds.
export [[nodiscard]] std::expected<SqliteReportData, std::string> read_projection(const std::filesystem::path& path,
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
    auto event_records =
        query("SELECT sequence,event_type,source_service,payload FROM projection_events ORDER BY sequence,event_id;");
    auto memory_regions =
        query("SELECT space,start_address,end_address,name,readable,writable,executable FROM memory_regions ORDER BY "
              "start_address;");
    auto functions = query(
        "SELECT space,entry_address,end_address,name,status,source_service FROM functions ORDER BY entry_address;");
    auto instructions = query("SELECT space,address,length,mnemonic,assembly,bytes,instruction_mask,flow_kind,"
                              "flow_fallthrough,flow_terminal,COALESCE(flow_target,''),pcode_count,source_service "
                              "FROM instructions ORDER BY address;");
    auto symbols = query(
        "SELECT COALESCE(space,''),COALESCE(address,''),name,namespace,priority FROM symbols ORDER BY entity_id;");
    auto analysis_runs = query("SELECT run_id,status,sequence FROM analysis_runs ORDER BY sequence,run_id;");
    auto references = query(
        "SELECT source_address,target_address,kind FROM references_projection ORDER BY source_address,target_address;");
    auto data_objects = query("SELECT address,type,value FROM data_objects ORDER BY address;");
    if (!event_types || !event_records || !memory_regions || !functions || !instructions || !symbols ||
        !analysis_runs || !references || !data_objects)
        return std::unexpected(!event_types      ? event_types.error()
                               : !event_records  ? event_records.error()
                               : !memory_regions ? memory_regions.error()
                               : !functions      ? functions.error()
                               : !instructions   ? instructions.error()
                               : !symbols        ? symbols.error()
                               : !analysis_runs  ? analysis_runs.error()
                               : !references     ? references.error()
                                                 : data_objects.error());
    result.event_types = std::move(*event_types);
    result.event_records = std::move(*event_records);
    result.memory_regions = std::move(*memory_regions);
    result.functions = std::move(*functions);
    result.instructions = std::move(*instructions);
    result.symbols = std::move(*symbols);
    result.analysis_runs = std::move(*analysis_runs);
    result.references = std::move(*references);
    result.data_objects = std::move(*data_objects);
    return result;
}

/// Reads a generated or checked-in Markdown artifact as canonical bytes.
export [[nodiscard]] std::expected<std::string, std::string> read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return std::unexpected("Unable to open Markdown artifact: " + path.string());
    std::ostringstream contents;
    contents << input.rdbuf();
    if (!input.good() && !input.eof())
        return std::unexpected("Unable to read Markdown artifact: " + path.string());
    return contents.str();
}

/// Derives a stable domain-specific seed without relying on implementation-defined
/// string hashing or process addresses.
[[nodiscard]] std::uint64_t domain_seed(std::uint64_t seed, std::string_view domain) {
    std::uint64_t value = seed ^ 0x9e3779b97f4a7c15ULL;
    for (const auto character : domain) {
        value ^= static_cast<std::uint8_t>(character);
        value *= 1099511628211ULL;
        value ^= value >> 29U;
    }
    return value;
}

/// Returns sorted, deterministic random sample indexes without replacement.
[[nodiscard]] std::vector<std::size_t> sample_indexes(std::size_t count, std::size_t limit, std::uint64_t seed,
                                                      std::string_view domain) {
    const auto sample_count = std::min(count, limit);
    std::vector<std::size_t> indexes(count);
    std::ranges::iota(indexes, 0U);
    std::mt19937_64 generator(domain_seed(seed, domain));
    for (std::size_t index = 0; index < sample_count; ++index) {
        std::uniform_int_distribution<std::size_t> distribution(index, count - 1U);
        std::swap(indexes[index], indexes[distribution(generator)]);
    }
    indexes.resize(sample_count);
    std::ranges::sort(indexes);
    return indexes;
}

/// Selects at most the configured number of functions with the same stable
/// sampler used by the common report writer.
export [[nodiscard]] std::vector<core::FunctionSnapshot>
sample_functions(const std::vector<core::FunctionSnapshot>& functions, const ReportLimits& limits) {
    std::vector<core::FunctionSnapshot> selected;
    for (const auto index : sample_indexes(functions.size(), limits.max_functions, limits.seed, "functions"))
        selected.push_back(functions[index]);
    return selected;
}

/// Selects the bounded, stable-random function subset used for asynchronous
/// decompilation so orchestration and report output share exactly one policy.
export [[nodiscard]] std::vector<core::FunctionSnapshot>
sample_decompilation_functions(const std::vector<core::FunctionSnapshot>& functions, const ReportLimits& limits) {
    const auto candidates = sample_functions(functions, limits);
    std::vector<core::FunctionSnapshot> selected;
    for (const auto index : sample_indexes(candidates.size(), limits.max_decompilations, limits.seed, "decompilations"))
        selected.push_back(candidates[index]);
    return selected;
}

/// Truncates one report cell while preserving an explicit deterministic marker.
[[nodiscard]] std::string shorten(std::string_view value, std::size_t limit) {
    if (value.size() <= limit)
        return std::string(value);
    if (limit <= 3U)
        return std::string(value.substr(0, limit));
    return std::string(value.substr(0, limit - 3U)) + "...";
}

/// Removes platform and compiler-generated trailing whitespace from multiline
/// evidence while preserving line boundaries and semantic content.
[[nodiscard]] std::string canonical_text(std::string_view value) {
    std::string result;
    std::size_t line_start{};
    while (line_start < value.size()) {
        const auto line_end = value.find('\n', line_start);
        auto line = value.substr(line_start, line_end == std::string_view::npos ? value.size() - line_start
                                                                                : line_end - line_start);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.remove_suffix(1U);
        result.append(line);
        result.push_back('\n');
        if (line_end == std::string_view::npos)
            break;
        line_start = line_end + 1U;
    }
    return result;
}

/// Escapes Markdown table syntax after applying the report text limit.
[[nodiscard]] std::string markdown_cell(std::string_view value, const ReportLimits& limits) {
    const auto bounded = shorten(value, limits.max_text_chars);
    std::string result;
    result.reserve(bounded.size());
    for (const char character : bounded) {
        if (character == '|')
            result += "\\|";
        else if (character == '\n' || character == '\r')
            result.push_back(' ');
        else
            result.push_back(character);
    }
    return result;
}

/// Formats a storage location for signatures, variables, and ABI evidence.
[[nodiscard]] std::string storage_text(const core::StorageLocation& storage) {
    return storage.space.name() + ":0x" + std::format("{:x}", storage.offset) + ":" + std::to_string(storage.size);
}

/// Formats all available parameter, return-type, ABI, and variadic signature facts.
[[nodiscard]] std::string signature_text(const std::optional<core::FunctionSignature>& signature) {
    if (!signature)
        return "<none>";
    std::string result = signature->return_type.value().empty() ? "<unknown>" : signature->return_type.value();
    result += " (";
    for (std::size_t index = 0; index < signature->parameters.size(); ++index) {
        if (index != 0)
            result += "; ";
        const auto& parameter = signature->parameters[index];
        result += parameter.name + ":" + (parameter.type.value().empty() ? "<unknown>" : parameter.type.value());
        result += parameter.indirect ? " indirect" : "";
        result += " @";
        for (std::size_t storage_index = 0; storage_index < parameter.storage.size(); ++storage_index) {
            if (storage_index != 0)
                result += ",";
            result += storage_text(parameter.storage[storage_index]);
        }
    }
    result += ") [cc=" + signature->calling_convention.name + ", variadic=" + (signature->variadic ? "true" : "false") +
              ", no_return=" + (signature->no_return ? "true" : "false") + "]";
    return result;
}

/// Formats one variable with type, storage, source, and isolation facts.
[[nodiscard]] std::string variable_text(const core::VariableDescription& variable) {
    std::string result = variable.name + ":" + variable.type.value() + " @";
    for (std::size_t index = 0; index < variable.storage.pieces.size(); ++index) {
        if (index != 0)
            result += ",";
        result += storage_text(variable.storage.pieces[index]);
    }
    result += " [source=" + variable.source + ", isolated=" + (variable.isolated ? "true" : "false") + "]";
    return result;
}

/// Formats all address ranges in one function body.
[[nodiscard]] std::string ranges_text(const core::AddressRangeSet& body) {
    std::string result;
    for (std::size_t index = 0; index < body.ranges().size(); ++index) {
        if (index != 0)
            result += ",";
        const auto& range = body.ranges()[index];
        result += range.start.space.name() + ":" + std::to_string(range.start.offset) + "-" +
                  std::to_string(range.end.offset);
    }
    return result.empty() ? "<none>" : result;
}

/// Extracts the first C declaration line so a report still exposes inferred
/// parameter and return types when the structured signature contract is empty.
[[nodiscard]] std::string source_signature_text(std::string_view source) {
    std::size_t line_start{};
    while (line_start < source.size()) {
        const auto line_end = source.find('\n', line_start);
        const auto line = source.substr(line_start, line_end == std::string_view::npos ? source.size() - line_start
                                                                                       : line_end - line_start);
        const auto first = line.find_first_not_of(" \t\r");
        if (first != std::string_view::npos) {
            const auto candidate = line.substr(first);
            if (candidate.find('(') != std::string_view::npos && candidate.find(')') != std::string_view::npos &&
                candidate.find('{') == std::string_view::npos)
                return std::string(candidate);
        }
        if (line_end == std::string_view::npos)
            break;
        line_start = line_end + 1U;
    }
    return "<none>";
}

/// Replaces machine-specific project-input locators with the report's stable
/// repository-relative fixture label before payload sampling.
[[nodiscard]] std::vector<std::vector<std::string>>
stable_event_records(const std::vector<std::vector<std::string>>& records, std::string_view fixture_label) {
    auto stable = records;
    for (auto& row : stable) {
        if (row.size() < 4U)
            continue;
        const auto locator = row[3].find("locator=");
        if (locator == std::string::npos)
            continue;
        const auto value_start = locator + std::string_view{"locator="}.size();
        const auto value_end = row[3].find(';', value_start);
        row[3].replace(value_start, value_end == std::string::npos ? std::string::npos : value_end - value_start,
                       "<" + std::string{fixture_label} + ">");
    }
    return stable;
}

/// Writes a bounded Markdown table and records total/selected counts in its heading.
void write_table(std::ostream& output, std::string_view title, const std::vector<std::string>& headers,
                 const std::vector<std::vector<std::string>>& rows, std::size_t limit, std::uint64_t seed,
                 std::string_view domain, const ReportLimits& limits) {
    const auto indexes = sample_indexes(rows.size(), limit, seed, domain);
    output << "### " << title << "\n\n"
           << "- Total rows: " << rows.size() << "\n"
           << "- Rows shown: " << indexes.size() << "\n"
           << "- Sampling: " << (indexes.size() == rows.size() ? "all" : "stable-random") << "\n\n|";
    for (const auto& header : headers)
        output << ' ' << header << " |";
    output << "\n|";
    for (std::size_t index = 0; index < headers.size(); ++index)
        output << " --- |";
    output << "\n";
    for (const auto row_index : indexes) {
        const auto& row = rows[row_index];
        output << '|';
        for (std::size_t index = 0; index < headers.size(); ++index) {
            const auto value = index < row.size() ? row[index] : std::string{};
            output << ' ' << markdown_cell(value, limits) << " |";
        }
        output << '\n';
    }
    if (rows.empty())
        output << "| _none_ |\n";
    output << '\n';
}

/// Writes a bounded list with full count metadata for diagnostics and variables.
void write_list(std::ostream& output, std::string_view title, const std::vector<std::string>& values, std::size_t limit,
                std::uint64_t seed, std::string_view domain, const ReportLimits& limits) {
    const auto indexes = sample_indexes(values.size(), limit, seed, domain);
    output << "#### " << title << "\n\n"
           << "- Total items: " << values.size() << "\n"
           << "- Items shown: " << indexes.size() << "\n\n";
    for (const auto index : indexes)
        output << "- `" << markdown_cell(values[index], limits) << "`\n";
    if (values.empty())
        output << "- _none_\n";
    output << '\n';
}

/// Generates the shared full-characterization Markdown structure for any
/// integration test that supplies the same contract data.
export [[nodiscard]] std::expected<std::string, std::string> generate_report(const ReportInput& input,
                                                                             const ReportLimits& limits) {
    if (!input.query)
        return std::unexpected("Cannot generate report without an in-memory project query");
    const bool reduced_analyzer_profile = input.analyzed.analyzers.size() < 2U;
    const bool missing_reference_data_profile = input.query->data_objects().empty() && input.sqlite.references.empty();
    const bool failed_decompilation = std::ranges::any_of(input.decompilations, [](const auto& item) {
        return item.result.status != core::DecompilationStatus::complete;
    });
    std::ostringstream output;
    output << "# ReCode Integration Report\n\n"
           << "- **Status:** "
           << (reduced_analyzer_profile || missing_reference_data_profile || failed_decompilation ? "PARTIAL" : "PASS")
           << "\n"
           << "- **Test:** `" << input.test_name << "`\n"
           << "- **Fixture:** `" << input.fixture_label << "`\n"
           << "- **SQLite projection:** `" << input.database_label << "`\n"
           << "- **Sampling seed:** `0x" << std::hex << limits.seed << std::dec << "`\n"
           << "- **Limits:** functions=" << limits.max_functions << ", decompilations=" << limits.max_decompilations
           << ", instructions=" << limits.max_instructions << ", sqlite_instructions=" << limits.max_sqlite_instructions
           << ", text_chars=" << limits.max_text_chars << "\n"
           << "- **Load revision:** " << input.loaded.revision.value << "\n"
           << "- **Analysis revision:** " << input.analyzed.committed_revision.value << "\n"
           << "- **Analyzer profile:** " << (reduced_analyzer_profile ? "reduced" : "full") << " ("
           << input.analyzed.analyzers.size() << " registered)\n"
           << "- **Entity profile:** " << (missing_reference_data_profile ? "missing references/data" : "complete")
           << "\n"
           << "- **SQLite checkpoint:** " << input.sqlite.checkpoint << "\n\n";

    output << "## Pipeline\n\n"
           << "1. `ProjectFacade::open` created the project and event/projection stores.\n"
           << "2. `ProjectFacade::load` parsed the PE, materialized regions/symbols, decoded entry/export seeds, and "
              "committed events.\n"
           << "3. `ProjectFacade::analyze` executed the registered runtime analyzers.\n"
           << "4. Selected asynchronous `Task<Result<Decompilation>>` operations completed through the worker pool.\n\n"
           << "### Executed Analyzers\n\n";
    for (const auto& analyzer : input.analyzed.analyzers)
        output << "- `" << markdown_cell(analyzer, limits) << "`\n";
    if (input.analyzed.analyzers.empty())
        output << "- _none_\n";
    output << '\n';
    std::vector<std::string> analysis_diagnostics;
    for (const auto& diagnostic : input.analyzed.diagnostics)
        analysis_diagnostics.push_back(diagnostic.message);
    write_list(output, "Analysis Diagnostics", analysis_diagnostics, limits.max_event_records, limits.seed,
               "analysis-diagnostics", limits);

    output << "## Coverage Contract\n\n"
           << "- All sections use deterministic SQL/order plus stable-random sampling when a limit is exceeded.\n"
           << "- Text cells and diagnostic/source fields are truncated to the configured limit; omitted content is not "
              "a runtime failure.\n"
           << "- References and data objects are reported even when their current event set is empty.\n\n";

    output << "## In-Memory Projection\n\n"
           << "- Revision: " << input.query->current_revision().value << "\n"
           << "- Functions: " << input.query->functions().size() << "\n"
           << "- Instructions: " << input.query->instructions().size() << "\n"
           << "- Memory regions: " << input.query->memory_regions().size() << "\n"
           << "- Symbols: " << input.query->symbols().size() << "\n"
           << "- Data objects: " << input.query->data_objects().size() << "\n\n";

    const auto functions = input.query->functions();
    const auto instructions = input.query->instructions();
    std::unordered_map<std::string, std::string> sampled_signatures;
    for (const auto& decompilation : input.decompilations)
        if (signature_text(decompilation.result.recovered_signature) == "<none>")
            sampled_signatures.emplace(std::to_string(decompilation.function.key.entry.offset),
                                       "C source: " + source_signature_text(decompilation.result.c_source));
    std::vector<std::vector<std::string>> function_rows;
    function_rows.reserve(functions.size());
    for (const auto& function : functions) {
        std::vector<std::string> variables;
        for (const auto& variable : function.variables)
            variables.push_back(variable_text(variable));
        const auto instruction_count = std::ranges::count_if(instructions, [&](const auto& instruction) {
            return std::ranges::any_of(function.body.ranges(), [&](const auto& range) {
                return instruction.key.address.space == range.start.space &&
                       instruction.key.address.offset >= range.start.offset &&
                       instruction.key.address.offset <= range.end.offset;
            });
        });
        auto function_signature = signature_text(function.signature);
        if (function_signature == "<none>") {
            const auto sampled_signature = sampled_signatures.find(std::to_string(function.key.entry.offset));
            if (sampled_signature != sampled_signatures.end())
                function_signature = sampled_signature->second;
        }
        function_rows.push_back(
            {function.key.entry.space.name(), std::to_string(function.key.entry.offset), function.name,
             function.namespace_name, function.external ? "true" : "false", function.no_return ? "true" : "false",
             function.analysis_status, std::to_string(instruction_count), ranges_text(function.body),
             function_signature,
             std::format("blocks={}, thunk={}, variables={}", function.blocks.size(),
                         function.thunk_target ? function.thunk_target->entity.value() : "<none>", variables.size())});
    }
    write_table(output, "Functions and Signatures",
                {"Space", "Entry", "Name", "Namespace", "External", "No return", "Status", "Instructions", "Body",
                 "Signature (return, parameters, ABI)", "Structure"},
                function_rows, limits.max_functions, limits.seed, "functions", limits);
    write_table(output, "SQLite Event Types", {"Event type", "Rows"}, input.sqlite.event_types, limits.max_event_types,
                limits.seed, "event-types", limits);
    const auto event_records = stable_event_records(input.sqlite.event_records, input.fixture_label);
    write_table(output, "SQLite Event Records", {"Sequence", "Event type", "Service", "Payload"}, event_records,
                limits.max_event_records, limits.seed, "event-records", limits);
    write_table(output, "SQLite Memory Regions", {"Space", "Start", "End", "Name", "Read", "Write", "Execute"},
                input.sqlite.memory_regions, limits.max_memory_regions, limits.seed, "memory-regions", limits);
    write_table(output, "SQLite Functions", {"Space", "Entry", "End", "Name", "Status", "Producer"},
                input.sqlite.functions, limits.max_functions, limits.seed, "sqlite-functions", limits);
    auto sqlite_instructions = input.sqlite.instructions;
    for (auto& row : sqlite_instructions) {
        if (row.size() < 2U)
            continue;
        const auto iterator = std::ranges::find_if(instructions, [&](const auto& instruction) {
            return std::to_string(instruction.key.address.offset) == row[1];
        });
        if (iterator == instructions.end())
            continue;
        std::vector<std::string> operands;
        for (const auto& operand : iterator->operands)
            operands.push_back(operand.text);
        row.push_back(std::format("operands={}, flow={}, target={}, pcode={}", operands.size(),
                                  std::to_underlying(iterator->flow.kind),
                                  iterator->flow.target ? std::to_string(iterator->flow.target->offset) : "<none>",
                                  iterator->pcode.operations.size()));
    }
    write_table(output, "SQLite Instructions",
                {"Space", "Address", "Length", "Mnemonic", "Assembly", "Bytes", "Mask", "Flow", "Fallthrough",
                 "Terminal", "Target", "P-code", "Producer", "Metadata"},
                sqlite_instructions, limits.max_sqlite_instructions, limits.seed, "sqlite-instructions", limits);
    write_table(output, "SQLite Symbols", {"Space", "Address", "Name", "Namespace", "Priority"}, input.sqlite.symbols,
                limits.max_symbols, limits.seed, "symbols", limits);
    write_table(output, "SQLite Analysis Runs", {"Run", "Status", "Sequence"}, input.sqlite.analysis_runs,
                limits.max_event_records, limits.seed, "analysis-runs", limits);
    write_table(output, "SQLite References", {"Source", "Target", "Kind"}, input.sqlite.references,
                limits.max_references, limits.seed, "references", limits);
    write_table(output, "SQLite Data Objects", {"Address", "Type", "Value"}, input.sqlite.data_objects,
                limits.max_data_objects, limits.seed, "data-objects", limits);

    output << "## Decompilations\n\n"
           << "- Total results: " << input.decompilations.size() << "\n"
           << "- Results shown: " << std::min(input.decompilations.size(), limits.max_decompilations) << "\n\n";
    const auto decompilation_indexes =
        sample_indexes(input.decompilations.size(), limits.max_decompilations, limits.seed, "decompilations");
    for (const auto index : decompilation_indexes) {
        const auto& decompilation = input.decompilations[index];
        const auto recovered_signature = signature_text(decompilation.result.recovered_signature);
        const auto displayed_signature = recovered_signature == "<none>"
                                             ? "C source: " + source_signature_text(decompilation.result.c_source)
                                             : recovered_signature;
        output << "### `0x" << std::hex << decompilation.function.key.entry.offset << std::dec << "` "
               << markdown_cell(decompilation.function.name, limits) << "\n\n"
               << "- Status: `"
               << (decompilation.result.status == core::DecompilationStatus::complete ? "complete" : "failed") << "`\n"
               << "- Read revision: " << decompilation.result.read_revision.value << "\n"
               << "- Signature: `" << markdown_cell(displayed_signature, limits) << "`\n"
               << "- Raw instructions: " << decompilation.result.raw_instructions.size() << "\n"
               << "- Control-flow chars: " << decompilation.result.control_flow_text.size() << "\n"
               << "- Switches: " << decompilation.result.switches.size() << "\n"
               << "- Evidence: " << decompilation.result.evidence.size() << "\n"
               << "- Cache identity: `" << markdown_cell(decompilation.result.cache_identity, limits) << "`\n\n"
               << "```c\n"
               << shorten(canonical_text(decompilation.result.c_source), limits.max_decompilation_chars) << "\n```\n\n"
               << "```text\n"
               << shorten(canonical_text(decompilation.result.control_flow_text), limits.max_decompilation_chars)
               << "\n```\n\n";
        std::vector<std::string> variables;
        for (const auto& variable : decompilation.result.recovered_variables)
            variables.push_back(variable_text(variable));
        write_list(output, "Recovered Variables", variables, limits.max_variables, limits.seed,
                   "decompilation-variables-" + decompilation.function.key.entity.value(), limits);
        std::vector<std::string> diagnostics;
        for (const auto& diagnostic : decompilation.result.diagnostics)
            diagnostics.push_back(diagnostic.message);
        write_list(output, "Diagnostics", diagnostics, limits.max_event_records, limits.seed,
                   "decompilation-diagnostics-" + decompilation.function.key.entity.value(), limits);
    }
    output.flush();
    if (!output)
        return std::unexpected("Unable to flush generated Markdown report");
    auto text = output.str();
    while (text.size() >= 2U && text.ends_with("\n\n"))
        text.pop_back();
    return text;
}

/// Generates and writes one canonical LF Markdown report artifact.
export [[nodiscard]] std::expected<void, std::string>
write_report(const std::filesystem::path& path, const ReportInput& input, const ReportLimits& limits) {
    const auto generated = generate_report(input, limits);
    if (!generated)
        return std::unexpected(generated.error());
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return std::unexpected("Unable to create report directory: " + error.message());
    std::ofstream output(path, std::ios::trunc | std::ios::binary);
    if (!output)
        return std::unexpected("Unable to create Markdown report: " + path.string());
    output.write(generated->data(), static_cast<std::streamsize>(generated->size()));
    output.flush();
    if (!output)
        return std::unexpected("Unable to flush Markdown report: " + path.string());
    return {};
}

/// Executes the reusable PE facade pipeline and verifies its complete golden
/// report contract, keeping GTest-specific boilerplate out of this module's
/// callers so future integration tests can reuse the same workflow.
export [[nodiscard]] std::expected<void, std::string>
run_full_pipeline_report(const std::filesystem::path& root, const std::filesystem::path& report_path,
                         const std::filesystem::path& golden_path) {
    const auto fixture = root / "services" / "analyzers" / "tests" / "data" / "test_analyzers_integration.exe";
    const auto sla = root / "services" / "sleigh" / "specifications" / "x86-64.sla";
    const auto directory = report_path.parent_path() / "project";
    const auto database = directory / "projection.sqlite";
    const ReportLimits limits{50, 10, 1000, 200, 500, 500, 500, 500, 100, 100, 100, 100, 10000, 0x415243485f504531ULL};
    if (!std::filesystem::exists(fixture))
        return std::unexpected("Missing integration PE fixture: " + fixture.string());
    if (!std::filesystem::exists(sla))
        return std::unexpected("Missing integration SLA fixture: " + sla.string());
    std::error_code error;
    std::filesystem::remove_all(report_path.parent_path(), error);
    if (error)
        return std::unexpected("Unable to reset integration report directory: " + error.message());

    project::ProjectConfig config;
    config.id = core::ProjectId{"architecture-integration"};
    config.directory = directory;
    config.primary_artifact = core::BinaryArtifact{
        core::ArtifactId{"integration-input"}, "fixture.exe", "PE", fixture.string(), 0, "fixture", "x86-64", true};
    config.sleigh_specification = sla;
    auto runtime = std::make_shared<project::RuntimeCore>(project::RuntimeConfig{workers::WorkerPoolConfig{2, 64}});
    auto facade = api::ProjectFacade::open(runtime, config);
    if (!facade)
        return std::unexpected("ProjectFacade::open failed: " + facade.error().message);
    const auto loaded = (*facade)->load();
    if (!loaded)
        return std::unexpected("ProjectFacade::load failed: " + loaded.error().message);
    const auto analyzed = (*facade)->analyze();
    if (!analyzed)
        return std::unexpected("ProjectFacade::analyze failed: " + analyzed.error().message);
    const auto query = (*facade)->query();
    if (!query)
        return std::unexpected("ProjectFacade returned a null project query");
    const auto functions = query->functions();
    if (functions.empty() || query->instructions().empty() || query->memory_regions().empty() ||
        query->symbols().empty())
        return std::unexpected("Project query projection is missing required PE entities");

    const auto targets = sample_decompilation_functions(functions, limits);
    std::vector<core::contracts::Task<core::Result<core::Decompilation>>> tasks;
    tasks.reserve(targets.size());
    for (const auto& function : targets) {
        auto task = (*facade)->decompile(function.key);
        if (!task.valid() || !task.control())
            return std::unexpected("ProjectFacade returned an invalid decompilation task");
        tasks.push_back(std::move(task));
    }
    std::vector<DecompilationReportData> decompilations;
    decompilations.reserve(tasks.size());
    for (std::size_t index = 0; index < tasks.size(); ++index) {
        tasks[index].wait();
        if (tasks[index].control()->status() != core::contracts::OperationStatus::completed)
            return std::unexpected("A decompilation task did not reach completed status");
        core::Result<core::Decompilation> result;
        try {
            result = tasks[index].get();
        } catch (const std::exception& exception) {
            return std::unexpected(std::string("Decompilation task threw: ") + exception.what());
        }
        if (!result)
            return std::unexpected("Decompilation task failed: " + result.error().message);
        if (result->c_source.empty())
            return std::unexpected("Decompilation task returned empty C source");
        decompilations.push_back(DecompilationReportData{targets[index], std::move(*result)});
    }

    (*facade)->close();
    runtime->shutdown();
    if (!std::filesystem::exists(database))
        return std::unexpected("Project close did not leave projection.sqlite for inspection");
    const auto sqlite = read_projection(database, config.id);
    if (!sqlite)
        return std::unexpected("SQLite projection read failed: " + sqlite.error());
    if (sqlite->checkpoint != query->current_revision().value)
        return std::unexpected("SQLite checkpoint does not match the in-memory query revision");
    if (sqlite->functions.size() != functions.size())
        return std::unexpected("SQLite function count does not match the in-memory projection");
    if (sqlite->instructions.size() != query->instructions().size())
        return std::unexpected("SQLite instruction count does not match the in-memory projection");
    if (sqlite->memory_regions.size() != query->memory_regions().size())
        return std::unexpected("SQLite memory-region count does not match the in-memory projection");
    if (sqlite->symbols.size() != query->symbols().size())
        return std::unexpected("SQLite symbol count does not match the in-memory projection");
    if (sqlite->data_objects.size() != query->data_objects().size())
        return std::unexpected("SQLite data-object count does not match the in-memory projection");
    if (sqlite->functions.empty() || sqlite->instructions.empty() || sqlite->memory_regions.empty() ||
        sqlite->symbols.empty() || sqlite->analysis_runs.empty())
        return std::unexpected("SQLite projection is missing required durable entities");

    const ReportInput input{"ArchitectureIntegrationTest.FullPeRuntimePipelineProducesProjectionAndReport",
                            "services/analyzers/tests/data/test_analyzers_integration.exe",
                            "build/test-reports/project/projection.sqlite",
                            *loaded,
                            *analyzed,
                            query,
                            *sqlite,
                            std::move(decompilations)};
    const auto written = write_report(report_path, input, limits);
    if (!written)
        return std::unexpected(written.error());
    const auto generated_text = read_text_file(report_path);
    const auto golden_text = read_text_file(golden_path);
    if (!generated_text)
        return std::unexpected(generated_text.error());
    if (!golden_text)
        return std::unexpected(golden_text.error());
    if (*generated_text != *golden_text)
        return std::unexpected("Generated Markdown differs from golden report: " + golden_path.string());
    return {};
}

} // namespace recode::tests::integration::report
