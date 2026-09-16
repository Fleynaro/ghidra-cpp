export module ghidra.runtime.storage.projection;

import std;
import ghidra.core;
import ghidra.core.contracts.projection;
import ghidra.core.events.event;
import ghidra.runtime.storage.sqlite_connection;

export namespace ghidra::runtime::storage {

namespace core = ghidra::core;

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
            "CREATE TABLE IF NOT EXISTS project_metadata(project_id TEXT PRIMARY KEY, state TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS instructions(entity_id TEXT PRIMARY KEY, address INTEGER NOT NULL, mnemonic "
            "TEXT NOT NULL, assembly TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS functions(entity_id TEXT PRIMARY KEY, entry_address INTEGER NOT NULL, name "
            "TEXT NOT NULL);"
            "CREATE TABLE IF NOT EXISTS memory_regions(entity_id TEXT PRIMARY KEY, start_address INTEGER NOT NULL, "
            "end_address INTEGER NOT NULL, name TEXT NOT NULL);");
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
        const std::vector<std::string> event_values{event.event_id.value(), std::to_string(event.global_sequence),
                                                    event.event_type};
        if (const auto inserted = connection_->execute(
                "INSERT OR IGNORE INTO applied_events(event_id,sequence,event_type) VALUES(?,?,?);", event_values);
            !inserted)
            return std::unexpected(inserted.error());
        const std::vector<std::string> checkpoint_values{project_->value(), std::to_string(event.global_sequence)};
        return connection_->execute("INSERT INTO projection_checkpoint(project_id,sequence) VALUES(?,?) "
                                    "ON CONFLICT(project_id) DO UPDATE SET sequence=excluded.sequence;",
                                    checkpoint_values);
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

} // namespace ghidra::runtime::storage
