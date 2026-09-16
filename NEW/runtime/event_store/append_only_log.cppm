export module ghidra.runtime.event_store.log;

import std;
import ghidra.core;
import ghidra.core.contracts.event_store;
import ghidra.core.events.event;
import ghidra.core.diagnostics;
import ghidra.runtime.event_store.codec;

export namespace ghidra::runtime::event_store {

namespace core = ghidra::core;

/// Owns one append-only framed binary project history file.
class AppendOnlyLog final : public core::contracts::IEventStore {
public:
    /// Opens an existing log or creates its parent directory and empty file.
    static core::Result<std::shared_ptr<AppendOnlyLog>> open(std::filesystem::path path) {
        auto result = std::shared_ptr<AppendOnlyLog>(new AppendOnlyLog(std::move(path)));
        if (const auto recovered = result->recover(); !recovered)
            return std::unexpected(recovered.error());
        return result;
    }

    /// Appends drafts with consecutive store-assigned identities.
    [[nodiscard]] core::Result<core::contracts::AppendResult>
    append(const core::ProjectId& project, std::span<const core::events::EventDraft> drafts) override {
        std::scoped_lock lock(mutex_);
        if (closed_)
            return std::unexpected(core::Error::make(core::DiagnosticCode::project_closed, "Event store is closed"));
        const auto previous = records_.empty() ? 0 : records_.back().global_sequence;
        core::contracts::AppendResult result{core::Revision{previous}, core::Revision{previous}, {}};
        std::ofstream output(path_, std::ios::binary | std::ios::app);
        if (!output)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure,
                                                     "Unable to open events.log for append",
                                                     "Check the project directory permissions."));
        for (const auto& draft : drafts) {
            if (draft.project != project)
                return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                         "Event draft belongs to a different project"));
            const auto duplicate = std::ranges::find_if(records_, [&](const auto& record) {
                return !draft.idempotency_key.empty() && record.project == project &&
                       record.idempotency_key == draft.idempotency_key;
            });
            if (duplicate != records_.end()) {
                result.batch.events.push_back(*duplicate);
                result.committed_revision = core::Revision{duplicate->global_sequence};
                continue;
            }
            core::events::EventEnvelope event;
            event.event_id = core::EventId{core::make_identifier("event", next_sequence_)};
            event.global_sequence = next_sequence_++;
            event.project = project;
            event.aggregate_kind = draft.aggregate_kind;
            event.aggregate_id = draft.aggregate_id;
            event.event_type = draft.event_type;
            event.schema_version = draft.schema_version;
            event.created_at = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                              std::chrono::system_clock::now().time_since_epoch())
                                                              .count());
            event.correlation = draft.correlation;
            event.causation = draft.causation;
            event.source_service = draft.source_service;
            event.idempotency_key = draft.idempotency_key;
            event.payload = draft.payload;
            event.payload_length = event.payload.size();
            const auto encoded = EventCodec::encode(event);
            event.checksum = core::events::checksum(encoded);
            if (!write_frame(output, encoded, event.checksum))
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::io_failure, "Unable to append a complete event frame"));
            records_.push_back(event);
            result.batch.events.push_back(std::move(event));
        }
        output.flush();
        if (!output)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure, "Unable to flush events.log"));
        result.committed_revision = core::Revision{result.batch.last_sequence()};
        return result;
    }

    /// Returns events at or after a project revision.
    [[nodiscard]] core::Result<core::contracts::EventStream> read(const core::ProjectId& project,
                                                                  core::Revision from) const override {
        std::scoped_lock lock(mutex_);
        core::contracts::EventStream stream;
        for (const auto& event : records_)
            if (event.project == project && event.global_sequence >= from.value)
                stream.events.push_back(event);
        return stream;
    }

    /// Returns the last event sequence for a project.
    [[nodiscard]] core::Result<core::Revision> last_revision(const core::ProjectId& project) const override {
        std::scoped_lock lock(mutex_);
        for (auto iterator = records_.rbegin(); iterator != records_.rend(); ++iterator)
            if (iterator->project == project)
                return core::Revision{iterator->global_sequence};
        return core::Revision{0};
    }

    /// Flushes the physical log after validating that it can be opened.
    [[nodiscard]] core::Result<void> flush() override {
        std::scoped_lock lock(mutex_);
        std::ofstream output(path_, std::ios::binary | std::ios::app);
        if (!output)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure, "Unable to flush events.log"));
        output.flush();
        return output ? core::Result<void>{}
                      : core::Result<void>{std::unexpected(
                            core::Error::make(core::DiagnosticCode::io_failure, "Unable to flush events.log"))};
    }

    /// Marks the store closed; later appends fail through the normal error result.
    void close() noexcept override {
        std::scoped_lock lock(mutex_);
        closed_ = true;
    }

    /// Returns the physical event-log path.
    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    /// Stores the path while deferring file validation to `recover`.
    explicit AppendOnlyLog(std::filesystem::path path) : path_(std::move(path)) {}

    /// Reads complete frames and truncates only an incomplete final frame.
    [[nodiscard]] core::Result<void> recover() {
        std::error_code error;
        std::filesystem::create_directories(path_.parent_path(), error);
        if (error)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure,
                                                     "Unable to create the project event-log directory"));
        if (!std::filesystem::exists(path_)) {
            std::ofstream create(path_, std::ios::binary);
            if (!create)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::io_failure, "Unable to create events.log"));
            return {};
        }
        std::ifstream input(path_, std::ios::binary);
        if (!input)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure, "Unable to open events.log"));
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
        std::size_t offset{};
        while (offset < bytes.size()) {
            const auto frame_start = offset;
            if (bytes.size() - offset <
                magic_.size() + sizeof(std::uint32_t) + sizeof(std::uint64_t) + sizeof(std::uint32_t))
                return truncate_tail(frame_start);
            if (!std::equal(magic_.begin(), magic_.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset)))
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt, "events.log has an invalid frame magic"));
            offset += magic_.size();
            const auto version = read_u32(bytes, offset);
            const auto size = read_u64(bytes, offset);
            const auto expected_checksum = read_u32(bytes, offset);
            if (!version || !size || !expected_checksum || *version != format_version_)
                return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                         "events.log has an unsupported or truncated frame header"));
            if (*size > bytes.size() - offset)
                return truncate_tail(frame_start);
            const std::string payload(reinterpret_cast<const char*>(bytes.data() + offset),
                                      static_cast<std::size_t>(*size));
            offset += static_cast<std::size_t>(*size);
            if (core::events::checksum(payload) != *expected_checksum)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt, "events.log contains a checksum failure"));
            auto decoded = EventCodec::decode(payload);
            if (!decoded)
                return std::unexpected(decoded.error());
            decoded->checksum = *expected_checksum;
            records_.push_back(std::move(*decoded));
            next_sequence_ = std::max(next_sequence_, records_.back().global_sequence + 1);
        }
        return {};
    }

    /// Truncates an incomplete final frame and retains all preceding events.
    [[nodiscard]] core::Result<void> truncate_tail(std::size_t offset) {
        std::error_code error;
        std::filesystem::resize_file(path_, offset, error);
        if (error)
            return std::unexpected(core::Error::make(core::DiagnosticCode::io_failure,
                                                     "Unable to truncate an incomplete events.log tail"));
        return {};
    }

    /// Writes one framed payload with magic/version/size/checksum metadata.
    [[nodiscard]] static bool write_frame(std::ofstream& output, std::string_view payload,
                                          std::uint32_t event_checksum) {
        output.write(magic_.data(), static_cast<std::streamsize>(magic_.size()));
        write_u32(output, format_version_);
        write_u64(output, payload.size());
        write_u32(output, event_checksum);
        output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        return static_cast<bool>(output);
    }

    /// Writes a little-endian unsigned 32-bit frame value.
    static void write_u32(std::ofstream& output, std::uint32_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index)
            output.put(static_cast<char>((value >> (index * 8)) & 0xffU));
    }

    /// Writes a little-endian unsigned 64-bit frame value.
    static void write_u64(std::ofstream& output, std::uint64_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index)
            output.put(static_cast<char>((value >> (index * 8)) & 0xffU));
    }

    /// Reads a little-endian unsigned 32-bit value from recovery bytes.
    [[nodiscard]] static std::optional<std::uint32_t> read_u32(const std::vector<std::uint8_t>& bytes,
                                                               std::size_t& offset) {
        if (bytes.size() - offset < sizeof(std::uint32_t))
            return std::nullopt;
        std::uint32_t result{};
        for (std::size_t index = 0; index < sizeof(result); ++index)
            result |= static_cast<std::uint32_t>(bytes[offset++]) << (index * 8);
        return result;
    }

    /// Reads a little-endian unsigned 64-bit value from recovery bytes.
    [[nodiscard]] static std::optional<std::uint64_t> read_u64(const std::vector<std::uint8_t>& bytes,
                                                               std::size_t& offset) {
        if (bytes.size() - offset < sizeof(std::uint64_t))
            return std::nullopt;
        std::uint64_t result{};
        for (std::size_t index = 0; index < sizeof(result); ++index)
            result |= static_cast<std::uint64_t>(bytes[offset++]) << (index * 8);
        return result;
    }

    static constexpr std::string_view magic_ = "NGHEVT01";
    static constexpr std::uint32_t format_version_ = 1;
    std::filesystem::path path_;
    mutable std::mutex mutex_;
    std::vector<core::events::EventEnvelope> records_;
    std::uint64_t next_sequence_{1};
    bool closed_{};
};

} // namespace ghidra::runtime::event_store
