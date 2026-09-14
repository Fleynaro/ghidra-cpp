export module function_id:database;

import std;
import :buffer_file;
import :parse_exception;
import :storage_helpers;
import :types;

// Ghidra references:
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/FidDB.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/FunctionsTable.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/RelationsTable.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidProgramSeeker.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/hash/FidDBUtils.java, and
// Framework/DB/src/main/java/db/LongKeyInteriorNode.java and LongKeyRecordNode.java.

namespace fid::detail {

/// Walks the long-key B-tree storage used by all primary FunctionID tables.
template <typename Visitor>
inline void walk_long_tree(const BufferFile& file, std::int32_t root, std::size_t fixed_record_length,
                           const std::vector<std::uint8_t>& fields, Visitor&& visitor) {
    std::set<std::int32_t> visited;
    std::optional<std::int64_t> previous_key;
    const std::function<void(std::int32_t, std::size_t)> walk = [&](std::int32_t buffer_id, std::size_t depth) {
        require(depth <= file.buffer_count(), fid::ErrorCode::invalid_database,
                "database B-tree exceeds the physical buffer count");
        require(visited.insert(buffer_id).second, fid::ErrorCode::invalid_database,
                "cycle detected in database B-tree");
        const auto buffer = file.buffer(buffer_id);
        const auto node_type = buffer[0];
        const auto count = static_cast<std::size_t>(read_be(buffer, 1, 4));
        if (node_type == 0) {
            require(count <= (buffer.size() - 5) / 12, fid::ErrorCode::invalid_database,
                    "invalid long-key interior node");
            for (std::size_t index = 0; index < count; ++index)
                walk(static_cast<std::int32_t>(signed_be(buffer, 5 + index * 12 + 8, 4)), depth + 1);
            return;
        }
        require(node_type == 1 || node_type == 2, fid::ErrorCode::unsupported_schema,
                "primary table does not use a long-key node");
        require(13 <= buffer.size(), fid::ErrorCode::invalid_database, "invalid long-key leaf header");
        if (node_type == 2) {
            require(fixed_record_length <= std::numeric_limits<std::size_t>::max() - 8,
                    fid::ErrorCode::invalid_database, "fixed record length overflows node layout");
            const auto entry_size = 8 + fixed_record_length;
            require(entry_size != 0 && count <= (buffer.size() - 13) / entry_size, fid::ErrorCode::invalid_database,
                    "invalid fixed-record leaf node");
            for (std::size_t index = 0; index < count; ++index) {
                const auto offset = 13 + index * entry_size;
                const auto key = static_cast<std::int64_t>(signed_be(buffer, offset, 8));
                require(!previous_key || key > *previous_key, fid::ErrorCode::invalid_database,
                        "database B-tree keys are not strictly ascending");
                previous_key = key;
                visitor(key, buffer.subspan(offset + 8, fixed_record_length));
            }
            return;
        }
        constexpr std::size_t entry_size = 13;
        require(count <= (buffer.size() - 13) / entry_size, fid::ErrorCode::invalid_database,
                "invalid variable-record leaf node");
        for (std::size_t index = 0; index < count; ++index) {
            const auto offset = 13 + index * entry_size;
            const auto key = static_cast<std::int64_t>(signed_be(buffer, offset, 8));
            require(!previous_key || key > *previous_key, fid::ErrorCode::invalid_database,
                    "database B-tree keys are not strictly ascending");
            previous_key = key;
            const auto record_offset = static_cast<std::size_t>(read_be(buffer, offset + 8, 4));
            const bool indirect = buffer[offset + 12] != 0;
            if (indirect) {
                require(record_offset <= buffer.size() && buffer.size() - record_offset >= 4,
                        fid::ErrorCode::invalid_database, "indirect variable-record pointer is outside leaf");
                const auto record_id = static_cast<std::int32_t>(signed_be(buffer, record_offset, 4));
                const auto record = file.chained(record_id);
                visitor(key, std::span<const fid::Byte>(record));
                continue;
            }
            require(record_offset <= buffer.size(), fid::ErrorCode::invalid_database,
                    "variable-record offset outside leaf");
            const auto next_offset =
                index == 0 ? buffer.size() : static_cast<std::size_t>(read_be(buffer, offset - entry_size + 8, 4));
            require(record_offset <= next_offset && next_offset <= buffer.size(), fid::ErrorCode::invalid_database,
                    "variable-record offsets are not descending");
            visitor(key, buffer.subspan(record_offset, next_offset - record_offset));
        }
    };
    if (root >= 0)
        walk(root, 0);
}

} // namespace fid::detail

export namespace fid {

/// Opens and queries one original Ghidra packed `.fidb` database.
class Database final {
private:
    /// Owns parsed immutable tables and relation indexes for one database.
    class Storage final {
    public:
        /// Transfers ownership of the validated raw buffer file.
        explicit Storage(detail::BufferFile file) : file(std::move(file)) {}

        detail::BufferFile file;
        std::vector<LibraryRecord> libraries;
        std::vector<FunctionRecord> functions;
        std::unordered_map<std::int64_t, LibraryRecord> libraries_by_id;
        std::unordered_map<std::uint64_t, std::vector<std::size_t>> full_index;
        std::unordered_set<std::uint64_t> superior_relations;
        std::unordered_set<std::uint64_t> inferior_relations;
    };

    std::unique_ptr<Storage> storage_;

    /// Constructs a database from immutable parsed storage.
    explicit Database(std::unique_ptr<Storage> storage) : storage_(std::move(storage)) {}

public:
    /// Opens a packed `.fidb` path read-only without converting or rewriting it.
    [[nodiscard]] static std::expected<Database, Error> open(const std::filesystem::path& path) {
        try {
            auto bytes = detail::read_file(path);
            std::vector<Byte> raw;
            if (bytes.size() >= 8 && detail::read_be(bytes, 0, 8) == detail::buffer_magic)
                raw = std::move(bytes);
            else
                raw = detail::unpack_fidb(bytes);
            detail::BufferFile buffer_file(std::move(raw));
            auto storage = std::make_unique<Storage>(std::move(buffer_file));

            const std::vector<std::uint8_t> master_fields = {4, 2, 2, 0, 5, 4, 2, 3, 2};
            std::vector<detail::TableDescriptor> descriptors;
            const auto master_root = storage->file.parameter(0);
            detail::walk_long_tree(
                storage->file, master_root, 0, master_fields,
                [&descriptors, &master_fields](std::int64_t key, std::span<const Byte> data) {
                    const auto values = detail::read_record_fields(data, master_fields);
                    detail::require(values.size() == 9, ErrorCode::malformed_record,
                                    "master table record has an unexpected field count");
                    detail::TableDescriptor descriptor;
                    descriptor.name = detail::string_value(values[0]);
                    descriptor.version = static_cast<std::int32_t>(detail::integer_value(values[1]));
                    descriptor.root_buffer_id = static_cast<std::int32_t>(detail::integer_value(values[2]));
                    descriptor.key_type = static_cast<std::uint8_t>(detail::integer_value(values[3]));
                    descriptor.field_types = detail::binary_value(values[4]);
                    descriptor.indexed_column = static_cast<std::int32_t>(detail::integer_value(values[6]));
                    descriptor.record_count = static_cast<std::int32_t>(detail::integer_value(values[8]));
                    (void)key;
                    descriptors.push_back(std::move(descriptor));
                });
            const auto find_table = [&descriptors](std::string_view name) -> const detail::TableDescriptor& {
                const auto iterator = std::find_if(descriptors.begin(), descriptors.end(),
                                                   [name](const auto& descriptor) { return descriptor.name == name; });
                detail::require(iterator != descriptors.end(), ErrorCode::invalid_database,
                                "required FunctionID table is missing: " + std::string(name));
                return *iterator;
            };
            const auto& library_table = find_table("Libraries Table");
            const auto& strings_table = find_table("Strings Table");
            const auto& functions_table = find_table("Functions Table");
            const auto& inferior_table = find_table("Inferior Table");
            const auto& superior_table = find_table("Superior Table");
            detail::require(strings_table.version == 6 && library_table.version == 6 && functions_table.version == 6 &&
                                inferior_table.version == 6 && superior_table.version == 6,
                            ErrorCode::unsupported_schema, "unsupported FunctionID database schema version");
            detail::validate_table_descriptor(strings_table, {4});
            detail::validate_table_descriptor(library_table, {4, 4, 4, 4, 4, 2, 2, 4});
            detail::validate_table_descriptor(functions_table, {1, 3, 0, 3, 3, 3, 3, 3, 0});
            detail::validate_table_descriptor(inferior_table, {});
            detail::validate_table_descriptor(superior_table, {});

            std::unordered_map<std::int64_t, std::string> strings;
            const std::vector<std::uint8_t> string_fields = {4};
            detail::walk_long_tree(storage->file, strings_table.root_buffer_id, 0, string_fields,
                                   [&strings, &string_fields](std::int64_t key, std::span<const Byte> data) {
                                       const auto values = detail::read_record_fields(data, string_fields);
                                       detail::require(values.size() == 1, ErrorCode::malformed_record,
                                                       "invalid strings-table record");
                                       strings.emplace(key, detail::string_value(values[0]));
                                   });

            const std::vector<std::uint8_t> library_fields = {4, 4, 4, 4, 4, 2, 2, 4};
            detail::walk_long_tree(
                storage->file, library_table.root_buffer_id, 0, library_fields,
                [&storage, &library_fields](std::int64_t key, std::span<const Byte> data) {
                    const auto values = detail::read_record_fields(data, library_fields);
                    detail::require(values.size() == 8, ErrorCode::malformed_record, "invalid libraries-table record");
                    LibraryRecord library;
                    library.id = key;
                    library.family_name = detail::string_value(values[0]);
                    library.version = detail::string_value(values[1]);
                    library.variant = detail::string_value(values[2]);
                    library.ghidra_version = detail::string_value(values[3]);
                    library.language_id = detail::string_value(values[4]);
                    library.language_version = static_cast<std::int32_t>(detail::integer_value(values[5]));
                    library.language_minor_version = static_cast<std::int32_t>(detail::integer_value(values[6]));
                    library.metadata = detail::string_value(values[7]);
                    detail::parse_metadata(library);
                    storage->libraries_by_id.emplace(key, library);
                    storage->libraries.push_back(std::move(library));
                });

            const std::vector<std::uint8_t> function_fields = {1, 3, 0, 3, 3, 3, 3, 3, 0};
            constexpr std::size_t function_record_length = 52;
            detail::walk_long_tree(
                storage->file, functions_table.root_buffer_id, function_record_length, function_fields,
                [&storage, &strings, &function_fields](std::int64_t key, std::span<const Byte> data) {
                    const auto values = detail::read_record_fields(data, function_fields);
                    detail::require(values.size() == 9, ErrorCode::malformed_record, "invalid functions-table record");
                    FunctionRecord function;
                    function.id = key;
                    function.hash.code_unit_size = static_cast<std::int16_t>(detail::integer_value(values[0]));
                    function.hash.full_hash = static_cast<std::uint64_t>(detail::integer_value(values[1]));
                    function.hash.specific_hash_additional_size =
                        static_cast<std::int8_t>(detail::integer_value(values[2]));
                    function.hash.specific_hash = static_cast<std::uint64_t>(detail::integer_value(values[3]));
                    function.library_id = detail::integer_value(values[4]);
                    const auto name_id = detail::integer_value(values[5]);
                    const auto path_id = detail::integer_value(values[7]);
                    function.name = strings.contains(name_id) ? strings.at(name_id) : std::string{};
                    function.entry_point = detail::integer_value(values[6]);
                    function.domain_path = strings.contains(path_id) ? strings.at(path_id) : std::string{};
                    function.flags = static_cast<std::uint8_t>(detail::integer_value(values[8]));
                    storage->full_index[function.hash.full_hash].push_back(storage->functions.size());
                    storage->functions.push_back(std::move(function));
                });

            const auto read_relations = [&storage](const detail::TableDescriptor& descriptor, auto& destination) {
                detail::walk_long_tree(storage->file, descriptor.root_buffer_id, 0, {},
                                       [&destination](std::int64_t key, std::span<const Byte>) {
                                           destination.insert(static_cast<std::uint64_t>(key));
                                       });
            };
            read_relations(inferior_table, storage->inferior_relations);
            read_relations(superior_table, storage->superior_relations);
            return Database(std::move(storage));
        } catch (const detail::ParseException& exception) {
            return std::unexpected(exception.error);
        } catch (const std::exception& exception) {
            return std::unexpected(Error{ErrorCode::invalid_database, exception.what()});
        }
    }

    /// Releases the immutable database storage.
    ~Database() = default;

    /// Transfers database ownership without copying large table vectors.
    Database(Database&&) noexcept = default;

    /// Releases current storage and transfers ownership from another database.
    Database& operator=(Database&&) noexcept = default;

    /// Prevents copying the owner of immutable parsed storage.
    Database(const Database&) = delete;

    /// Prevents copy assignment of the owner of immutable parsed storage.
    Database& operator=(const Database&) = delete;

    /// Returns the decoded library records in primary-key order.
    [[nodiscard]] std::span<const LibraryRecord> libraries() const noexcept {
        return storage_->libraries;
    }

    /// Returns all records sharing a full hash, preserving database order.
    [[nodiscard]] std::vector<FunctionRecord> find_full_hash(std::uint64_t hash) const {
        std::vector<FunctionRecord> result;
        if (const auto iterator = storage_->full_index.find(hash); iterator != storage_->full_index.end()) {
            result.reserve(iterator->second.size());
            for (const auto index : iterator->second)
                result.push_back(storage_->functions[index]);
        }
        return result;
    }

    /// Returns all records sharing a specific hash by the original full-table scan semantics.
    [[nodiscard]] std::vector<FunctionRecord> find_specific_hash(std::uint64_t hash) const {
        std::vector<FunctionRecord> result;
        for (const auto& function : storage_->functions) {
            if (function.hash.specific_hash == hash)
                result.push_back(function);
        }
        return result;
    }

    /// Returns all decoded function records.
    [[nodiscard]] std::span<const FunctionRecord> functions() const noexcept {
        return storage_->functions;
    }

    /// Returns the raw database buffer size for diagnostics and tests.
    [[nodiscard]] std::size_t buffer_size() const noexcept {
        return storage_->file.buffer_size();
    }

    /// Identifies one function using Ghidra's candidate filtering and scoring behavior.
    [[nodiscard]] std::expected<IdentificationResult, Error> identify(const FunctionContext& context,
                                                                      const ProgramInfo& program,
                                                                      float score_threshold = 14.6F,
                                                                      std::int16_t medium_limit = 24) const {
        try {
            detail::require(storage_ != nullptr, ErrorCode::invalid_input, "database is not initialized");
            IdentificationResult result;
            result.hash = context.hash;
            std::vector<Match> candidates;
            std::map<std::uint64_t, HashQuad> child_hashes;
            std::map<std::uint64_t, HashQuad> parent_hashes;
            for (const auto& child : context.children)
                child_hashes[child.full_hash] = child;
            for (const auto& parent : context.parents)
                parent_hashes[parent.full_hash] = parent;
            const auto candidates_by_hash = find_full_hash(context.hash.full_hash);
            for (const auto& function : candidates_by_hash) {
                if (function.auto_fail() ||
                    function.force_specific() && function.hash.specific_hash != context.hash.specific_hash)
                    continue;
                const auto library_iterator = storage_->libraries_by_id.find(function.library_id);
                if (library_iterator == storage_->libraries_by_id.end() ||
                    !detail::library_matches(library_iterator->second, program))
                    continue;
                int child_units = 0;
                for (const auto& [child_hash, child] : child_hashes) {
                    if (storage_->superior_relations.contains(detail::superior_relation_key(function.id, child_hash)))
                        child_units += child.code_unit_size;
                }
                if (function.force_relation() && child_units == 0)
                    continue;
                int parent_units = 0;
                if (parent_hashes.size() < 500) {
                    for (const auto& [parent_hash, parent] : parent_hashes) {
                        if (storage_->inferior_relations.contains(
                                detail::inferior_relation_key(parent_hash, function.id)))
                            parent_units += parent.code_unit_size;
                    }
                }
                int function_units = function.hash.code_unit_size;
                if (function.auto_pass())
                    function_units = std::max<int>(function_units, medium_limit);
                const bool specific = function.hash.specific_hash == context.hash.specific_hash;
                const float function_score = static_cast<float>(function_units) +
                                             (specific ? 0.67F * function.hash.specific_hash_additional_size : 0.0F);
                if (function_score + child_units + parent_units < score_threshold)
                    continue;
                candidates.push_back(Match{function, library_iterator->second,
                                           specific ? MatchMode::specific : MatchMode::full, function_score,
                                           static_cast<float>(child_units), static_cast<float>(parent_units)});
            }
            std::stable_sort(candidates.begin(), candidates.end(), [](const Match& left, const Match& right) {
                return left.overall_score() > right.overall_score();
            });
            if (!candidates.empty()) {
                const auto best_score = candidates.front().overall_score();
                for (const auto& candidate : candidates) {
                    if (candidate.overall_score() != best_score)
                        break;
                    result.matches.push_back(candidate);
                    result.names.push_back(candidate.function.name);
                }
                std::sort(result.names.begin(), result.names.end());
                result.names.erase(std::unique(result.names.begin(), result.names.end()), result.names.end());
            }
            return result;
        } catch (const detail::ParseException& exception) {
            return std::unexpected(exception.error);
        }
    }
};

} // namespace fid
