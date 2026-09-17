module;

#include <sqlite3.h>

export module recode.runtime.storage.sqlite_connection;

import std;
import recode.core.diagnostics;

export namespace recode::runtime::storage {

namespace core = recode::core;

/// Owns one SQLite connection and translates SQLite failures into core errors.
class SqliteConnection final {
public:
    /// Opens a SQLite database in read/write/create mode.
    static core::Result<std::shared_ptr<SqliteConnection>> open(const std::filesystem::path& path) {
        sqlite3* handle{};
        const auto result =
            sqlite3_open_v2(path.string().c_str(), &handle,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
        if (result != SQLITE_OK) {
            const std::string message = handle ? sqlite3_errmsg(handle) : "SQLite could not allocate a connection";
            if (handle)
                sqlite3_close(handle);
            return std::unexpected(core::Error::make(core::DiagnosticCode::projection_failure, message,
                                                     "Check the projection directory and SQLite installation."));
        }
        auto connection = std::shared_ptr<SqliteConnection>(new SqliteConnection(handle));
        if (const auto journal = connection->execute("PRAGMA journal_mode=WAL;"); !journal)
            return std::unexpected(journal.error());
        if (const auto foreign_keys = connection->execute("PRAGMA foreign_keys=ON;"); !foreign_keys)
            return std::unexpected(foreign_keys.error());
        return connection;
    }

    /// Closes the owned SQLite connection.
    ~SqliteConnection() {
        if (handle_)
            sqlite3_close(handle_);
    }

    /// Prevents copying a native database handle.
    SqliteConnection(const SqliteConnection&) = delete;

    /// Prevents copying a native database handle.
    SqliteConnection& operator=(const SqliteConnection&) = delete;

    /// Executes one SQL statement without returning rows.
    [[nodiscard]] core::Result<void> execute(std::string_view sql) const {
        char* message{};
        const auto result = sqlite3_exec(handle_, std::string(sql).c_str(), nullptr, nullptr, &message);
        if (result != SQLITE_OK) {
            std::string text = message ? message : "SQLite statement failed";
            sqlite3_free(message);
            return std::unexpected(core::Error::make(core::DiagnosticCode::projection_failure, std::move(text)));
        }
        return {};
    }

    /// Executes a parameterized SQL statement with text values.
    [[nodiscard]] core::Result<void> execute(std::string_view sql, std::span<const std::string> values) const {
        sqlite3_stmt* statement{};
        if (sqlite3_prepare_v2(handle_, std::string(sql).c_str(), -1, &statement, nullptr) != SQLITE_OK)
            return sqlite_error("Unable to prepare SQLite statement");
        for (std::size_t index = 0; index < values.size(); ++index)
            sqlite3_bind_text(statement, static_cast<int>(index + 1), values[index].c_str(), -1, SQLITE_TRANSIENT);
        const auto result = sqlite3_step(statement);
        sqlite3_finalize(statement);
        if (result != SQLITE_DONE)
            return sqlite_error("SQLite parameterized statement failed");
        return {};
    }

    /// Returns one unsigned integer column from a parameterized query.
    [[nodiscard]] core::Result<std::uint64_t> scalar_u64(std::string_view sql,
                                                         std::span<const std::string> values = {}) const {
        sqlite3_stmt* statement{};
        if (sqlite3_prepare_v2(handle_, std::string(sql).c_str(), -1, &statement, nullptr) != SQLITE_OK)
            return std::unexpected(sqlite_error("Unable to prepare SQLite scalar query").error());
        for (std::size_t index = 0; index < values.size(); ++index)
            sqlite3_bind_text(statement, static_cast<int>(index + 1), values[index].c_str(), -1, SQLITE_TRANSIENT);
        const auto result = sqlite3_step(statement);
        if (result != SQLITE_ROW) {
            sqlite3_finalize(statement);
            return std::unexpected(sqlite_error("SQLite scalar query returned no row").error());
        }
        const auto value = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 0));
        sqlite3_finalize(statement);
        return value;
    }

private:
    /// Stores an already-open native SQLite connection.
    explicit SqliteConnection(sqlite3* handle) : handle_(handle) {}

    /// Converts the native last-error text into a core error value.
    [[nodiscard]] core::Result<void> sqlite_error(std::string fallback) const {
        return std::unexpected(core::Error::make(core::DiagnosticCode::projection_failure,
                                                 handle_ ? std::string(sqlite3_errmsg(handle_)) : std::move(fallback)));
    }

    sqlite3* handle_{};
};

} // namespace recode::runtime::storage
