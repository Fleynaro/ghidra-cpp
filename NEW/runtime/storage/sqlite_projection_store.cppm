export module recode.runtime.storage.projection;

import std;
import recode.core;
import recode.core.contracts.projection;
import recode.core.events.event;
import recode.runtime.storage.sqlite_connection;

export namespace recode::runtime::storage {

namespace core = recode::core;

/// Persists a replaceable projection checkpoint and applied-event identity in SQLite.
class SqliteProjectionStore final : public core::contracts::IProjectionStore {
public:
    /// Creates a store over the project projection path.
    explicit SqliteProjectionStore(std::filesystem::path path) : path_(std::move(path)) {}

    /// Opens the database and initializes the durable projection schema.
    [[nodiscard]] core::Result<void> open(const core::ProjectId& project) override {
        auto connection = SqliteConnection::open(path_);
        if (!connection)
            return std::unexpected(connection.error());
        connection_ = std::move(*connection);
        project_ = project;
        const auto schema = connection_->execute(
            "CREATE TABLE IF NOT EXISTS projection_checkpoint(project_id TEXT PRIMARY KEY, sequence INTEGER NOT NULL);"
            "CREATE TABLE IF NOT EXISTS applied_events(event_id TEXT PRIMARY KEY, sequence INTEGER NOT NULL, "
            "event_type TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS projection_events(event_id TEXT PRIMARY KEY, sequence INTEGER NOT NULL, "
            "event_type TEXT NOT NULL, source_service TEXT NOT NULL, payload TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS project_metadata(project_id TEXT PRIMARY KEY, state TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS instructions(entity_id TEXT PRIMARY KEY, space TEXT NOT NULL, address "
            "INTEGER NOT NULL, length INTEGER NOT NULL, mnemonic TEXT NOT NULL, assembly TEXT NOT NULL, bytes TEXT "
            "NOT NULL, instruction_mask TEXT NOT NULL, source_service TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS functions(entity_id TEXT PRIMARY KEY, space TEXT NOT NULL, entry_address "
            "INTEGER NOT NULL, end_address INTEGER NOT NULL, name TEXT NOT NULL, status TEXT NOT NULL, "
            "source_service TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS memory_regions(entity_id TEXT PRIMARY KEY, space TEXT NOT NULL, "
            "start_address INTEGER NOT NULL, end_address INTEGER NOT NULL, name TEXT NOT NULL, readable INTEGER NOT "
            "NULL, writable INTEGER NOT NULL, executable INTEGER NOT NULL);"
            "CREATE TABLE IF NOT EXISTS symbols(entity_id TEXT PRIMARY KEY, space TEXT, address INTEGER, name TEXT "
            "NOT NULL, namespace TEXT NOT NULL, priority INTEGER NOT NULL);"
            "CREATE TABLE IF NOT EXISTS analysis_runs(run_id TEXT PRIMARY KEY, status TEXT NOT NULL, sequence INTEGER "
            "NOT NULL);"
            "CREATE TABLE IF NOT EXISTS references_projection(entity_id TEXT PRIMARY KEY, source_address INTEGER NOT "
            "NULL, target_address INTEGER NOT NULL, kind TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS data_objects(entity_id TEXT PRIMARY KEY, address INTEGER NOT NULL, type TEXT "
            "NOT NULL, value TEXT NOT NULL);");
        if (!schema)
            return std::unexpected(schema.error());
        const std::vector<std::string> project_value{project.value()};
        const auto metadata = connection_->execute(
            "INSERT OR IGNORE INTO project_metadata(project_id,state) VALUES(?, 'ready');", project_value);
        if (!metadata)
            return std::unexpected(metadata.error());
        return {};
    }

    /// Records an applied event identity and advances the durable checkpoint.
    [[nodiscard]] core::Result<void> persist(const core::events::EventEnvelope& event) override {
        if (!connection_ || !project_)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::projection_failure, "SQLite projection store is not open"));
        if (const auto begun = connection_->execute("BEGIN IMMEDIATE;"); !begun)
            return std::unexpected(begun.error());

        const auto rollback = [&]() { static_cast<void>(connection_->execute("ROLLBACK;")); };
        const auto fail = [&](const core::Error& error) -> core::Result<void> {
            rollback();
            return std::unexpected(error);
        };
        const std::vector<std::string> event_values{event.event_id.value(), std::to_string(event.global_sequence),
                                                    event.event_type};
        if (const auto inserted = connection_->execute(
                "INSERT OR IGNORE INTO applied_events(event_id,sequence,event_type) VALUES(?,?,?);", event_values);
            !inserted)
            return fail(inserted.error());

        const std::vector<std::string> event_record_values{event.event_id.value(),
                                                           std::to_string(event.global_sequence), event.event_type,
                                                           event.source_service, event.payload};
        if (const auto recorded = connection_->execute(
                "INSERT OR IGNORE INTO projection_events(event_id,sequence,event_type,source_service,payload) "
                "VALUES(?,?,?,?,?);",
                event_record_values);
            !recorded)
            return fail(recorded.error());

        const auto fields = core::events::decode_fields(event.payload);
        const auto field = [&](std::string_view name) -> std::string {
            const auto iterator = fields.find(std::string{name});
            return iterator == fields.end() ? std::string{} : iterator->second;
        };
        const auto number = [&](std::string_view name) -> core::Result<std::string> {
            const auto value = field(name);
            if (value.empty())
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt,
                                      "SQLite projection event is missing field " + std::string{name}));
            std::uint64_t parsed{};
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (error != std::errc{} || end != value.data() + value.size())
                return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                         "SQLite projection event contains an invalid number"));
            return value;
        };
        core::Result<void> entity_result{};
        if (event.event_type == "MemoryStateChanged") {
            auto start = number("start");
            auto end = number("end");
            if (!start || !end)
                return fail((!start ? start.error() : end.error()));
            const std::vector<std::string> values{field("id"),
                                                  field("space"),
                                                  *start,
                                                  *end,
                                                  field("name"),
                                                  fields.contains("r") && fields.at("r") == "1" ? "1" : "0",
                                                  fields.contains("w") && fields.at("w") == "1" ? "1" : "0",
                                                  fields.contains("x") && fields.at("x") == "1" ? "1" : "0"};
            entity_result = connection_->execute(
                "INSERT INTO memory_regions(entity_id,space,start_address,end_address,name,readable,writable,"
                "executable) VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(entity_id) DO UPDATE SET space=excluded.space,"
                "start_address=excluded.start_address,end_address=excluded.end_address,name=excluded.name,"
                "readable=excluded.readable,writable=excluded.writable,executable=excluded.executable;",
                values);
        } else if (event.event_type == "ListingStateChanged") {
            auto address = number("address");
            auto length = number("length");
            if (!address || !length)
                return fail((!address ? address.error() : length.error()));
            const std::vector<std::string> values{
                field("id"),         field("space"),    *address,       *length,
                field("mnemonic"),   field("assembly"), field("bytes"), field("instruction_mask"),
                event.source_service};
            entity_result = connection_->execute(
                "INSERT INTO instructions(entity_id,space,address,length,mnemonic,assembly,bytes,instruction_mask,"
                "source_service) VALUES(?,?,?,?,?,?,?,?,?) ON CONFLICT(entity_id) DO UPDATE SET space=excluded.space,"
                "address=excluded.address,length=excluded.length,mnemonic=excluded.mnemonic,assembly=excluded.assembly,"
                "bytes=excluded.bytes,instruction_mask=excluded.instruction_mask,source_service=excluded.source_"
                "service;",
                values);
        } else if (event.event_type == "FunctionStateChanged") {
            auto entry = number("entry");
            auto end = number("end");
            if (!entry || !end)
                return fail((!entry ? entry.error() : end.error()));
            const std::vector<std::string> values{field("id"),     field("space"),      *entry, *end, field("name"),
                                                  field("status"), event.source_service};
            entity_result = connection_->execute(
                "INSERT INTO functions(entity_id,space,entry_address,end_address,name,status,source_service) "
                "VALUES(?,?,?,?,?,?,?) ON CONFLICT(entity_id) DO UPDATE SET space=excluded.space,"
                "entry_address=excluded.entry_address,end_address=excluded.end_address,name=excluded.name,"
                "status=excluded.status,source_service=excluded.source_service;",
                values);
        } else if (event.event_type == "SymbolStateChanged") {
            const auto address = field("address");
            const std::vector<std::string> values{field("id"),   address.empty() ? "" : "ram", address,
                                                  field("name"), field("namespace"),           field("priority")};
            entity_result = connection_->execute(
                "INSERT INTO symbols(entity_id,space,address,name,namespace,priority) VALUES(?,?,?,?,?,?) "
                "ON CONFLICT(entity_id) DO UPDATE SET space=excluded.space,address=excluded.address,"
                "name=excluded.name,namespace=excluded.namespace,priority=excluded.priority;",
                values);
        } else if (event.event_type == "AnalysisRunStateChanged") {
            const std::vector<std::string> values{field("run"), field("status"), std::to_string(event.global_sequence)};
            entity_result = connection_->execute(
                "INSERT INTO analysis_runs(run_id,status,sequence) VALUES(?,?,?) ON CONFLICT(run_id) DO UPDATE SET "
                "status=excluded.status,sequence=excluded.sequence;",
                values);
        }
        if (!entity_result)
            return fail(entity_result.error());

        const std::vector<std::string> checkpoint_values{project_->value(), std::to_string(event.global_sequence)};
        if (const auto checkpoint = connection_->execute(
                "INSERT INTO projection_checkpoint(project_id,sequence) VALUES(?,?) ON CONFLICT(project_id) DO "
                "UPDATE SET sequence=MAX(projection_checkpoint.sequence,excluded.sequence);",
                checkpoint_values);
            !checkpoint)
            return fail(checkpoint.error());
        if (const auto committed = connection_->execute("COMMIT;"); !committed) {
            rollback();
            return std::unexpected(committed.error());
        }
        return {};
    }

    /// Reads the durable checkpoint for the opened project.
    [[nodiscard]] core::Result<core::Revision> checkpoint(const core::ProjectId& project) const override {
        if (!connection_)
            return core::Revision{0};
        const std::vector<std::string> values{project.value()};
        auto sequence = connection_->scalar_u64(
            "SELECT COALESCE(sequence,0) FROM projection_checkpoint WHERE project_id=?;", values);
        if (!sequence) {
            if (sequence.error().message.find("returned no row") != std::string::npos)
                return core::Revision{0};
            return std::unexpected(sequence.error());
        }
        return core::Revision{*sequence};
    }

    /// Flushes the SQLite connection using an explicit checkpoint command.
    [[nodiscard]] core::Result<void> flush() override {
        if (!connection_)
            return {};
        return connection_->execute("PRAGMA wal_checkpoint(PASSIVE);");
    }

private:
    std::filesystem::path path_;
    std::shared_ptr<SqliteConnection> connection_;
    std::optional<core::ProjectId> project_;
};

} // namespace recode::runtime::storage
