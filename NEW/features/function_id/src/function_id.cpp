module;
#include <zlib.h>

module function_id;

import std;

namespace {

using fid::Byte;

constexpr std::uint64_t fnv_prime = 1099511628211ULL;
constexpr std::uint32_t scalar_placeholder = 0xfeeddeadU;
constexpr std::uint64_t packed_magic = 0x2e30212634e92c20ULL;
constexpr std::uint64_t buffer_magic = 0x2f30312c34292c2aULL;
constexpr std::string_view fid_content_type = "Function ID Database";

/// Carries an internal parser failure without losing its public error category.
class ParseException final : public std::runtime_error {
public:
    /// Constructs a parser exception with a public error code and diagnostic.
    ParseException(fid::ErrorCode code, std::string message)
        : std::runtime_error(message), error{code, std::move(message)} {}

    fid::Error error;
};

/// Throws a typed parser exception when a storage invariant is false.
void require(bool condition, fid::ErrorCode code, std::string message) {
    if (!condition) {
        throw ParseException(code, std::move(message));
    }
}

/// Reads one big-endian unsigned integer from a bounded byte span.
std::uint64_t read_be(std::span<const Byte> data, std::size_t offset, std::size_t width) {
    require(width <= 8 && offset <= data.size() && width <= data.size() - offset, fid::ErrorCode::malformed_record,
            "truncated big-endian value");
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < width; ++index) {
        value = (value << 8U) | data[offset + index];
    }
    return value;
}

/// Reads one little-endian unsigned integer from a bounded byte span.
std::uint64_t read_le(std::span<const Byte> data, std::size_t offset, std::size_t width) {
    require(width <= 8 && offset <= data.size() && width <= data.size() - offset, fid::ErrorCode::invalid_zip,
            "truncated little-endian value");
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < width; ++index) {
        value |= static_cast<std::uint64_t>(data[offset + index]) << (index * 8U);
    }
    return value;
}

/// Interprets a big-endian byte sequence as a signed integer of the requested width.
std::int64_t signed_be(std::span<const Byte> data, std::size_t offset, std::size_t width) {
    const auto value = read_be(data, offset, width);
    if (width == 8) {
        return static_cast<std::int64_t>(value);
    }
    const auto sign_bit = std::uint64_t{1} << (width * 8U - 1U);
    const auto mask = (std::uint64_t{1} << (width * 8U)) - 1U;
    return static_cast<std::int64_t>((value & sign_bit) != 0 ? value | ~mask : value);
}

/// Reads an entire binary file while preserving the original bytes for bounded parsing.
std::vector<Byte> read_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw ParseException(fid::ErrorCode::io_failure, "unable to open database: " + path.string());
    }
    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    require(size >= 0, fid::ErrorCode::io_failure, "unable to determine database size");
    stream.seekg(0, std::ios::beg);
    std::vector<Byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    require(stream.good() || stream.eof(), fid::ErrorCode::io_failure, "unable to read database");
    return bytes;
}

/// Inflates the raw DEFLATE stream used by ItemSerializer's single ZIP entry.
std::vector<Byte> inflate_raw(std::span<const Byte> compressed, std::size_t expected_size) {
    z_stream stream{};
    const int init_result = inflateInit2(&stream, -MAX_WBITS);
    require(init_result == Z_OK, fid::ErrorCode::invalid_zip, "unable to initialize DEFLATE decoder");

    std::vector<Byte> output;
    output.reserve(expected_size);
    std::array<Byte, 32768> buffer{};
    stream.next_in = const_cast<Byte*>(compressed.data());
    stream.avail_in = static_cast<uInt>(std::min<std::size_t>(compressed.size(), UINT_MAX));
    std::size_t input_offset = stream.avail_in;
    int result = Z_OK;
    while (result != Z_STREAM_END) {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<uInt>(buffer.size());
        result = ::inflate(&stream, Z_NO_FLUSH);
        require(result == Z_OK || result == Z_STREAM_END, fid::ErrorCode::invalid_zip, "invalid DEFLATE payload");
        const auto produced = buffer.size() - stream.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(produced));
        if (result != Z_STREAM_END && stream.avail_in == 0) {
            require(input_offset < compressed.size(), fid::ErrorCode::invalid_zip, "truncated DEFLATE payload");
            const auto remaining = compressed.size() - input_offset;
            const auto chunk = std::min<std::size_t>(remaining, UINT_MAX);
            stream.next_in = const_cast<Byte*>(compressed.data() + input_offset);
            stream.avail_in = static_cast<uInt>(chunk);
            input_offset += chunk;
        }
        if (result != Z_STREAM_END && produced == 0 && stream.avail_in != 0) {
            require(false, fid::ErrorCode::invalid_zip, "DEFLATE decoder made no progress");
        }
    }
    inflateEnd(&stream);
    require(output.size() == expected_size, fid::ErrorCode::invalid_zip,
            "packed item length does not match its header");
    return output;
}

/// Extracts the raw Ghidra buffer-file bytes from an original `.fidb` ItemSerializer container.
std::vector<Byte> unpack_fidb(const std::vector<Byte>& packed) {
    require(packed.size() >= 18, fid::ErrorCode::invalid_packed_item, "packed item is truncated");
    require(read_be(packed, 0, 4) == 0xaced0005U, fid::ErrorCode::invalid_packed_item,
            "missing Java serialization stream header");
    require(read_be(packed, 6, 8) == packed_magic, fid::ErrorCode::invalid_packed_item, "invalid packed-item magic");
    require(read_be(packed, 14, 4) == 1, fid::ErrorCode::invalid_packed_item, "unsupported packed-item format version");

    std::size_t cursor = 18;
    auto read_modified_utf = [&packed, &cursor]() {
        require(cursor + 2 <= packed.size(), fid::ErrorCode::invalid_packed_item,
                "truncated packed-item string length");
        const auto length = static_cast<std::size_t>(read_be(packed, cursor, 2));
        cursor += 2;
        require(length <= packed.size() - cursor, fid::ErrorCode::invalid_packed_item, "truncated packed-item string");
        std::string value(reinterpret_cast<const char*>(packed.data() + cursor), length);
        cursor += length;
        return value;
    };
    const auto item_name = read_modified_utf();
    const auto content_type = read_modified_utf();
    (void)item_name;
    require(content_type == fid_content_type, fid::ErrorCode::invalid_packed_item,
            "packed item is not a Function ID database");
    require(cursor + 12 <= packed.size(), fid::ErrorCode::invalid_packed_item, "truncated packed-item metadata");
    require(read_be(packed, cursor, 4) == 0, fid::ErrorCode::invalid_packed_item, "packed item is not a database file");
    cursor += 4;
    const auto length = read_be(packed, cursor, 8);
    cursor += 8;
    require(length <= std::numeric_limits<std::size_t>::max(), fid::ErrorCode::invalid_packed_item,
            "packed item is too large");
    require(cursor + 30 <= packed.size() && read_le(packed, cursor, 4) == 0x04034b50U, fid::ErrorCode::invalid_zip,
            "missing ZIP local header");
    const auto method = read_le(packed, cursor + 8, 2);
    const auto name_length = static_cast<std::size_t>(read_le(packed, cursor + 26, 2));
    const auto extra_length = static_cast<std::size_t>(read_le(packed, cursor + 28, 2));
    require(method == 8, fid::ErrorCode::invalid_zip, "unsupported ZIP compression method");
    const auto data_offset = cursor + 30 + name_length + extra_length;
    require(data_offset <= packed.size(), fid::ErrorCode::invalid_zip, "truncated ZIP local header");
    const auto data = std::span<const Byte>(packed).subspan(data_offset);
    return inflate_raw(data, static_cast<std::size_t>(length));
}

/// Provides checked random access to the big-endian Ghidra LocalBufferFile format.
class BufferFile final {
public:
    /// Parses a raw `.fidbf` payload and validates its physical block geometry.
    explicit BufferFile(std::vector<Byte> bytes) : bytes_(std::move(bytes)) {
        require(bytes_.size() >= 32, fid::ErrorCode::invalid_buffer_file, "buffer file header is truncated");
        const auto header = std::span<const Byte>(bytes_);
        require(read_be(header, 0, 8) == buffer_magic, fid::ErrorCode::invalid_buffer_file,
                "invalid buffer-file magic");
        require(read_be(header, 16, 4) == 1, fid::ErrorCode::invalid_buffer_file,
                "unsupported buffer-file header version");
        block_size_ = static_cast<std::size_t>(read_be(header, 20, 4));
        require(block_size_ >= 128 && block_size_ <= bytes_.size(), fid::ErrorCode::invalid_buffer_file,
                "invalid buffer-file block size");
        require(bytes_.size() % block_size_ == 0, fid::ErrorCode::invalid_buffer_file,
                "buffer file is not block aligned");
        buffer_size_ = block_size_ - 5;
        const auto parameter_count = static_cast<std::size_t>(read_be(header, 28, 4));
        std::size_t cursor = 32;
        for (std::size_t index = 0; index < parameter_count; ++index) {
            require(cursor + 4 <= block_size_, fid::ErrorCode::invalid_buffer_file,
                    "buffer-file parameters exceed header block");
            const auto name_size = static_cast<std::size_t>(read_be(header, cursor, 4));
            cursor += 4;
            require(name_size <= block_size_ - cursor && name_size + 4 <= block_size_ - cursor,
                    fid::ErrorCode::invalid_buffer_file, "invalid buffer-file parameter");
            cursor += name_size + 4;
        }
    }

    /// Returns the usable data area of one logical database buffer.
    [[nodiscard]] std::span<const Byte> buffer(std::int32_t id) const {
        require(id >= 0, fid::ErrorCode::invalid_database, "negative database buffer id");
        const auto block = static_cast<std::size_t>(id) + 1;
        require(block < bytes_.size() / block_size_, fid::ErrorCode::invalid_database,
                "database buffer id is outside the file");
        const auto offset = block * block_size_;
        require(bytes_[offset] == 0, fid::ErrorCode::invalid_database, "database references a free buffer");
        const auto stored_id = static_cast<std::int32_t>(read_be(bytes_, offset + 1, 4));
        require(stored_id == id, fid::ErrorCode::invalid_database, "database buffer id mismatch");
        return std::span<const Byte>(bytes_).subspan(offset + 5, buffer_size_);
    }

    /// Returns the database parameter stored in DBParms buffer zero.
    [[nodiscard]] std::int32_t parameter(std::size_t index) const {
        const auto data = buffer(0);
        require(data.size() >= 6 + (index + 1) * 4, fid::ErrorCode::invalid_database, "database parameter is missing");
        require(data[0] == 9 && data[5] == 1, fid::ErrorCode::invalid_database, "invalid DBParms buffer");
        const auto data_length = static_cast<std::int32_t>(read_be(data, 1, 4));
        require(data_length >= 1 + static_cast<std::int32_t>((index + 1) * 4), fid::ErrorCode::invalid_database,
                "database parameter length is invalid");
        return static_cast<std::int32_t>(read_be(data, 6 + index * 4, 4));
    }

    /// Returns all bytes of a chained DBBuffer, including indexed large objects.
    [[nodiscard]] std::vector<Byte> chained(std::int32_t first_id) const {
        const auto first = buffer(first_id);
        const auto raw_length = static_cast<std::int32_t>(read_be(first, 1, 4));
        const bool obfuscated = raw_length < 0;
        const auto length = static_cast<std::size_t>(raw_length & 0x7fffffff);
        require(length > 0, fid::ErrorCode::invalid_database, "empty chained DBBuffer");
        std::vector<Byte> result(length);
        if (first[0] == 9) {
            const auto available = std::min(length, first.size() - 5);
            std::copy_n(first.begin() + 5, available, result.begin());
            if (obfuscated) {
                xor_bytes(result, 0, available);
            }
            return result;
        }
        require(first[0] == 8, fid::ErrorCode::invalid_database, "invalid chained DBBuffer node type");
        const auto data_space = buffer_size_ - 1;
        const auto ids_per_index = (buffer_size_ - 9) / 4;
        std::size_t copied = 0;
        std::int32_t index_id = first_id;
        bool first_index = true;
        while (index_id >= 0 && copied < length) {
            const auto index_buffer = buffer(index_id);
            const auto next = static_cast<std::int32_t>(read_be(index_buffer, 5, 4));
            const auto index_base = std::size_t{9};
            const auto needed = (length - copied + data_space - 1) / data_space;
            const auto count = std::min(ids_per_index, needed);
            for (std::size_t slot = 0; slot < count; ++slot) {
                const auto data_id = static_cast<std::int32_t>(read_be(index_buffer, index_base + slot * 4, 4));
                require(data_id >= 0, fid::ErrorCode::invalid_database, "missing chained data buffer");
                const auto data_buffer = buffer(data_id);
                const auto amount = std::min(data_space, length - copied);
                std::copy_n(data_buffer.begin() + 1, amount, result.begin() + static_cast<std::ptrdiff_t>(copied));
                if (obfuscated) {
                    xor_bytes(result, copied, amount);
                }
                copied += amount;
            }
            require(!first_index || static_cast<std::size_t>(read_be(index_buffer, 1, 4) & 0x7fffffff) == length,
                    fid::ErrorCode::invalid_database, "chained DBBuffer length mismatch");
            first_index = false;
            index_id = next;
        }
        require(copied == length, fid::ErrorCode::invalid_database, "truncated chained DBBuffer");
        return result;
    }

    /// Returns the usable block size used by the database.
    [[nodiscard]] std::size_t buffer_size() const noexcept {
        return buffer_size_;
    }

private:
    /// Applies Ghidra's fixed XOR mask to a contiguous chained-buffer range.
    static void xor_bytes(std::vector<Byte>& data, std::size_t offset, std::size_t length) {
        constexpr std::array<Byte, 16> mask = {0x59, 0xea, 0x67, 0x23, 0xda, 0xb8, 0x00, 0xb8,
                                               0xc3, 0x48, 0xdd, 0x8b, 0x21, 0xd6, 0x94, 0x78};
        for (std::size_t index = 0; index < length; ++index) {
            data[offset + index] ^= mask[index % mask.size()];
        }
    }

    std::vector<Byte> bytes_;
    std::size_t block_size_{};
    std::size_t buffer_size_{};
};

enum class FieldType : std::uint8_t {
    byte = 0,
    short_value = 1,
    int_value = 2,
    long_value = 3,
    string = 4,
    binary = 5,
    boolean = 6,
    fixed10 = 7
};

/// Describes the schema metadata stored in one Ghidra database master-table record.
struct TableDescriptor {
    std::string name;
    std::int32_t version{};
    std::int32_t root_buffer_id{-1};
    std::uint8_t key_type{};
    std::vector<std::uint8_t> field_types;
    std::int32_t indexed_column{-1};
    std::int32_t record_count{};
};

using Value =
    std::variant<std::monostate, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::string, std::vector<Byte>>;

/// Decodes one schema field from a Ghidra DB record payload.
Value read_value(std::span<const Byte> data, std::size_t& cursor, std::uint8_t encoded_type) {
    const auto type = static_cast<FieldType>(encoded_type & 0x0fU);
    switch (type) {
        case FieldType::byte:
            require(cursor + 1 <= data.size(), fid::ErrorCode::malformed_record, "truncated byte field");
            return static_cast<std::int8_t>(data[cursor++]);
        case FieldType::short_value:
            require(cursor + 2 <= data.size(), fid::ErrorCode::malformed_record, "truncated short field");
            {
                const auto value = static_cast<std::int16_t>(signed_be(data, cursor, 2));
                cursor += 2;
                return value;
            }
        case FieldType::int_value:
            require(cursor + 4 <= data.size(), fid::ErrorCode::malformed_record, "truncated int field");
            {
                const auto value = static_cast<std::int32_t>(signed_be(data, cursor, 4));
                cursor += 4;
                return value;
            }
        case FieldType::long_value:
            require(cursor + 8 <= data.size(), fid::ErrorCode::malformed_record, "truncated long field");
            {
                const auto value = static_cast<std::int64_t>(signed_be(data, cursor, 8));
                cursor += 8;
                return value;
            }
        case FieldType::string: {
            require(cursor + 4 <= data.size(), fid::ErrorCode::malformed_record, "truncated string length");
            const auto length = static_cast<std::int32_t>(signed_be(data, cursor, 4));
            cursor += 4;
            require(length >= 0 && static_cast<std::size_t>(length) <= data.size() - cursor,
                    fid::ErrorCode::malformed_record, "invalid string length");
            std::string value(reinterpret_cast<const char*>(data.data() + cursor), static_cast<std::size_t>(length));
            cursor += static_cast<std::size_t>(length);
            return value;
        }
        case FieldType::binary: {
            require(cursor + 4 <= data.size(), fid::ErrorCode::malformed_record, "truncated binary length");
            const auto length = static_cast<std::int32_t>(signed_be(data, cursor, 4));
            cursor += 4;
            require(length >= 0 && static_cast<std::size_t>(length) <= data.size() - cursor,
                    fid::ErrorCode::malformed_record, "invalid binary length");
            std::vector<Byte> value(data.begin() + static_cast<std::ptrdiff_t>(cursor), data.begin() + cursor + length);
            cursor += static_cast<std::size_t>(length);
            return value;
        }
        case FieldType::boolean:
            require(cursor + 1 <= data.size(), fid::ErrorCode::malformed_record, "truncated boolean field");
            return static_cast<std::int8_t>(data[cursor++]);
        case FieldType::fixed10:
            require(cursor + 10 <= data.size(), fid::ErrorCode::malformed_record, "truncated fixed field");
            {
                std::vector<Byte> value(data.begin() + static_cast<std::ptrdiff_t>(cursor), data.begin() + cursor + 10);
                cursor += 10;
                return value;
            }
    }
    throw ParseException(fid::ErrorCode::unsupported_schema, "unsupported Ghidra DB field type");
}

/// Decodes every ordinary field in a schema, stopping before schema extensions.
std::vector<Value> read_record_fields(std::span<const Byte> data, const std::vector<std::uint8_t>& fields) {
    std::vector<Value> values;
    std::size_t cursor = 0;
    for (const auto type : fields) {
        if (type == 0xffU) {
            break;
        }
        values.push_back(read_value(data, cursor, type));
    }
    return values;
}

/// Returns a value as a signed 64-bit integer for fixed-width master-table fields.
std::int64_t integer_value(const Value& value) {
    return std::visit(
        [](const auto& item) -> std::int64_t {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_arithmetic_v<T>)
                return static_cast<std::int64_t>(item);
            throw ParseException(fid::ErrorCode::malformed_record, "expected integer database field");
        },
        value);
}

/// Returns a string value from a schema field and rejects accidental type confusion.
const std::string& string_value(const Value& value) {
    if (const auto* string = std::get_if<std::string>(&value)) {
        return *string;
    }
    throw ParseException(fid::ErrorCode::malformed_record, "expected string database field");
}

/// Walks the long-key B-tree storage used by all primary FunctionID tables.
template <typename Visitor>
void walk_long_tree(const BufferFile& file, std::int32_t root, std::size_t fixed_record_length,
                    const std::vector<std::uint8_t>& fields, Visitor&& visitor) {
    std::set<std::int32_t> visited;
    std::function<void(std::int32_t)> walk = [&](std::int32_t buffer_id) {
        require(visited.insert(buffer_id).second, fid::ErrorCode::invalid_database,
                "cycle detected in database B-tree");
        const auto buffer = file.buffer(buffer_id);
        const auto node_type = buffer[0];
        const auto count = static_cast<std::size_t>(read_be(buffer, 1, 4));
        if (node_type == 0) {
            require(5 + count * 12 <= buffer.size(), fid::ErrorCode::invalid_database,
                    "invalid long-key interior node");
            for (std::size_t index = 0; index < count; ++index) {
                walk(static_cast<std::int32_t>(signed_be(buffer, 5 + index * 12 + 8, 4)));
            }
            return;
        }
        require(node_type == 1 || node_type == 2, fid::ErrorCode::unsupported_schema,
                "primary table does not use a long-key node");
        require(13 <= buffer.size(), fid::ErrorCode::invalid_database, "invalid long-key leaf header");
        if (node_type == 2) {
            const auto entry_size = 8 + fixed_record_length;
            require(entry_size != 0 && 13 + count * entry_size <= buffer.size(), fid::ErrorCode::invalid_database,
                    "invalid fixed-record leaf node");
            for (std::size_t index = 0; index < count; ++index) {
                const auto offset = 13 + index * entry_size;
                const auto key = static_cast<std::int64_t>(signed_be(buffer, offset, 8));
                visitor(key, buffer.subspan(offset + 8, fixed_record_length));
            }
            return;
        }
        constexpr std::size_t entry_size = 13;
        require(13 + count * entry_size <= buffer.size(), fid::ErrorCode::invalid_database,
                "invalid variable-record leaf node");
        for (std::size_t index = 0; index < count; ++index) {
            const auto offset = 13 + index * entry_size;
            const auto key = static_cast<std::int64_t>(signed_be(buffer, offset, 8));
            const auto record_offset = static_cast<std::size_t>(read_be(buffer, offset + 8, 4));
            const bool indirect = buffer[offset + 12] != 0;
            if (indirect) {
                const auto record_id = static_cast<std::int32_t>(signed_be(buffer, record_offset, 4));
                const auto record = file.chained(record_id);
                visitor(key, std::span<const Byte>(record));
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
    if (root >= 0) {
        walk(root);
    }
}

/// Splits a comma-separated FunctionID metadata list into a set, preserving empty-as-unrestricted.
std::set<std::string> split_metadata(std::string_view text) {
    std::set<std::string> result;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto end = text.find(',', start);
        const auto token_end = end == std::string_view::npos ? text.size() : end;
        if (token_end > start) {
            result.emplace(text.substr(start, token_end - start));
        }
        if (end == std::string_view::npos)
            break;
        start = end + 1;
    }
    return result;
}

/// Decodes the two filter lists stored in LibraryRecord.Library Metadata.
void parse_metadata(fid::LibraryRecord& library) {
    const auto separator = library.metadata.find(':');
    if (separator == std::string::npos) {
        library.compiler_specs = split_metadata(library.metadata);
        return;
    }
    library.compiler_specs = split_metadata(std::string_view(library.metadata).substr(0, separator));
    library.source_languages = split_metadata(std::string_view(library.metadata).substr(separator + 1));
}

/// Removes the final language variant component as ProcessorSizeComparator does.
std::string language_architecture(std::string_view language) {
    const auto separator = language.rfind(':');
    return separator == std::string_view::npos ? std::string(language) : std::string(language.substr(0, separator));
}

/// Returns true when a library language matches Ghidra's architecture-only comparison.
bool language_matches(std::string_view library, const std::optional<std::string>& target) {
    return !target.has_value() || language_architecture(library) == language_architecture(*target);
}

/// Returns true when compiler and source-language filters accept a library.
bool library_matches(const fid::LibraryRecord& library, const fid::ProgramInfo& program) {
    if (!language_matches(library.language_id, program.language_id)) {
        return false;
    }
    if (program.ignore_database_filters) {
        return true;
    }
    if (program.compiler_spec && !library.compiler_specs.empty() &&
        !library.compiler_specs.contains(*program.compiler_spec)) {
        return false;
    }
    if (!library.source_languages.empty() && !program.source_languages.empty()) {
        bool intersects = false;
        for (const auto& source : program.source_languages) {
            intersects |= library.source_languages.contains(source);
        }
        if (!intersects)
            return false;
    }
    return true;
}

/// Returns a mixed 32-bit operand value with Java integer overflow semantics.
std::uint32_t scalar_mix(std::int32_t value) noexcept {
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(static_cast<std::uint32_t>(value)) + 1234567U) *
                                      67999U);
}

/// Updates an FNV-1a state with a big-endian Java int representation.
void update_int(std::uint64_t& state, std::uint32_t value) noexcept {
    for (int shift = 24; shift >= 0; shift -= 8) {
        state ^= static_cast<Byte>(value >> shift);
        state *= fnv_prime;
    }
}

/// Updates an FNV-1a state with raw bytes.
void update_bytes(std::uint64_t& state, std::span<const Byte> bytes) noexcept {
    for (const auto value : bytes) {
        state ^= value;
        state *= fnv_prime;
    }
}

/// Returns the fixed set of processor NOP encodings skipped by X86InstructionSkipper.
bool is_x86_skipped(std::span<const Byte> bytes) {
    constexpr std::array<std::array<Byte, 9>, 17> patterns = {{
        {{0x90}},
        {{0x8b, 0xc0}},
        {{0x8b, 0xc9}},
        {{0x8b, 0xd2}},
        {{0x8b, 0xdb}},
        {{0x8b, 0xe4}},
        {{0x8b, 0xed}},
        {{0x8b, 0xf6}},
        {{0x8b, 0xff}},
        {{0x66, 0x90}},
        {{0x0f, 0x1f, 0x00}},
        {{0x0f, 0x1f, 0x40, 0x00}},
        {{0x0f, 0x1f, 0x44, 0x00, 0x00}},
        {{0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00}},
        {{0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00}},
        {{0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {{0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}},
    }};
    constexpr std::array<std::size_t, 17> lengths = {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 4, 5, 6, 7, 8, 9};
    for (std::size_t index = 0; index < patterns.size(); ++index) {
        if (bytes.size() == lengths[index] && std::equal(bytes.begin(), bytes.end(), patterns[index].begin())) {
            return true;
        }
    }
    return false;
}

/// Describes the variable-width ModRM/SIB portion of one x86 instruction.
#if 0
// Retired: FunctionID now consumes decoded Sleigh metadata rather than maintaining
// a parallel processor decoder. The implementation is intentionally excluded from
// the build while the source migration is completed.
struct ModRmInfo {
    std::size_t end{};
    std::size_t modrm_offset{};
    std::optional<std::size_t> sib_offset;
    std::optional<std::size_t> displacement_offset;
    std::size_t displacement_size{};
    int mod{};
    int reg{};
    int rm{};
    bool absolute_address{};
    std::vector<fid::OperandObject> reg_objects;
    std::vector<fid::OperandObject> rm_objects;
};

/// Returns the register widths selected by an x86 opcode and its operand-size context.
std::pair<int, int> x86_modrm_widths(std::uint8_t opcode, std::uint8_t opcode2, bool two_byte,
                                     bool rex_w, bool operand16) {
    const int default_width = rex_w ? 8 : (operand16 ? 2 : 4);
    if (!two_byte) {
        if (opcode == 0x88 || opcode == 0x8a || opcode == 0xc0 || opcode == 0xd0 || opcode == 0xd2) {
            return {1, 1};
        }
        if (opcode == 0xf6) return {1, 1};
        return {default_width, default_width};
    }
    if (opcode2 == 0xb6 || opcode2 == 0xbe) return {default_width, 1};
    if (opcode2 == 0xb7 || opcode2 == 0xbf) return {default_width, 2};
    return {default_width, default_width};
}

/// Creates the register-space offset used by Ghidra's x86 language definition.
fid::OperandObject x86_register(int index, int width = 8) {
    // The '_' entries in ia.sinc reserve the remaining bytes of each
    // eight-byte register slot, so every general register keeps an 8-byte
    // stride even when the selected view is E*, W*, or B*.
    (void)width;
    const auto offset = index < 8 ? index * 8 : 0x80 + (index - 8) * 8;
    return fid::OperandObject{fid::OperandObjectKind::register_value, offset, true, false, false};
}

/// Parses ModRM, SIB, and displacement bytes while preserving operand register identity.
ModRmInfo parse_modrm(std::span<const Byte> bytes, std::size_t offset, bool address32, std::uint8_t rex,
                      int register_width, int rm_width) {
    require(offset < bytes.size(), fid::ErrorCode::invalid_input, "truncated x86 ModRM byte");
    ModRmInfo result;
    result.modrm_offset = offset;
    const auto modrm = bytes[offset++];
    result.mod = modrm >> 6U;
    result.reg = ((modrm >> 3U) & 7U) | ((rex & 4U) != 0 ? 8 : 0);
    result.rm = (modrm & 7U) | ((rex & 1U) != 0 ? 8 : 0);
    result.reg_objects.push_back(x86_register(result.reg, register_width));
    if (result.mod == 3) {
        result.rm_objects.push_back(x86_register(result.rm, rm_width));
        result.end = offset;
        return result;
    }
    const auto base_rm = modrm & 7U;
    if (base_rm == 4) {
        require(offset < bytes.size(), fid::ErrorCode::invalid_input, "truncated x86 SIB byte");
        result.sib_offset = offset;
        const auto sib = bytes[offset++];
        const auto index = ((sib >> 3U) & 7U) | ((rex & 2U) != 0 ? 8 : 0);
        const auto base = (sib & 7U) | ((rex & 1U) != 0 ? 8 : 0);
        if ((sib >> 3U & 7U) != 4U) result.rm_objects.push_back(x86_register(index, address32 ? 4 : 8));
        if (result.mod == 0 && (sib & 7U) == 5U) {
            result.absolute_address = true;
            result.displacement_size = 4;
        } else {
            result.rm_objects.push_back(x86_register(base, address32 ? 4 : 8));
        }
    } else if (result.mod == 0 && base_rm == 5U) {
        result.absolute_address = true;
        result.displacement_size = 4;
    } else {
        result.rm_objects.push_back(x86_register(result.rm, address32 ? 4 : 8));
    }
    if (result.mod == 1) result.displacement_size = 1;
    if (result.mod == 2) result.displacement_size = address32 ? 4 : 4;
    if (result.displacement_size != 0) {
        result.displacement_offset = offset;
        require(offset + result.displacement_size <= bytes.size(), fid::ErrorCode::invalid_input,
                "truncated x86 displacement");
        offset += result.displacement_size;
        auto value = static_cast<std::int64_t>(read_le(bytes, *result.displacement_offset, result.displacement_size));
        if (result.displacement_size == 1 && (value & 0x80) != 0) value -= 0x100;
        if (result.displacement_size == 4 && (value & 0x80000000LL) != 0) value -= 0x100000000LL;
        result.rm_objects.push_back(fid::OperandObject{fid::OperandObjectKind::scalar, value, false,
                                                        result.absolute_address, result.absolute_address});
    }
    result.end = offset;
    return result;
}

/// Returns whether an opcode consumes a ModRM byte in the supported x86 subset.
bool x86_has_modrm(std::uint8_t opcode, std::uint8_t opcode2, bool two_byte, bool three_byte) {
    if (three_byte) return true;
    if (two_byte) {
        return (opcode2 >= 0x10 && opcode2 <= 0x11) || (opcode2 >= 0x28 && opcode2 <= 0x2f) ||
               (opcode2 >= 0x40 && opcode2 <= 0x4f) || (opcode2 >= 0x58 && opcode2 <= 0x5f) ||
               (opcode2 >= 0x60 && opcode2 <= 0x6f) || opcode2 == 0x70 || opcode2 == 0x71 ||
               opcode2 == 0x72 || opcode2 == 0x73 || opcode2 == 0x7e || opcode2 == 0x7f ||
               (opcode2 >= 0x90 && opcode2 <= 0x9f) || opcode2 == 0xaf || opcode2 == 0xb6 ||
               opcode2 == 0xb7 || opcode2 == 0xbe || opcode2 == 0xbf || opcode2 == 0xba ||
               opcode2 == 0xc0 || opcode2 == 0xc1 || opcode2 == 0xc7;
    }
    return (opcode <= 0x03) || (opcode >= 0x08 && opcode <= 0x0b) ||
           (opcode >= 0x10 && opcode <= 0x13) || (opcode >= 0x18 && opcode <= 0x1b) ||
           (opcode >= 0x20 && opcode <= 0x23) || (opcode >= 0x28 && opcode <= 0x2b) ||
           (opcode >= 0x30 && opcode <= 0x33) || (opcode >= 0x38 && opcode <= 0x3b) ||
           opcode == 0x62 || opcode == 0x63 || opcode == 0x69 || opcode == 0x6b ||
           (opcode >= 0x80 && opcode <= 0x83) || (opcode >= 0x84 && opcode <= 0x8f) ||
           opcode == 0xc0 || opcode == 0xc1 || opcode == 0xc6 || opcode == 0xc7 ||
           (opcode >= 0xd0 && opcode <= 0xd3) || opcode == 0xf6 || opcode == 0xf7 ||
           opcode == 0xfe || opcode == 0xff;
}

/// Returns the immediate width for an x86 opcode after prefixes and ModRM were parsed.
std::size_t x86_immediate_size(std::uint8_t opcode, std::uint8_t opcode2, bool two_byte,
                                bool rex_w, bool operand16, int modrm_reg) {
    const auto operand_size = rex_w ? std::size_t{8} : (operand16 ? std::size_t{2} : std::size_t{4});
    if (!two_byte) {
        if (opcode == 0x68 || opcode == 0xa9) return operand_size == 8 ? 4 : operand_size;
        if (opcode == 0x69 || opcode == 0x81) return operand_size == 8 ? 4 : operand_size;
        if (opcode == 0x6a || opcode == 0x80 || opcode == 0x82 || opcode == 0x83 || opcode == 0xc0 ||
            opcode == 0xc1 || opcode == 0xc6 || opcode == 0x6b) return 1;
        if (opcode == 0xc2 || opcode == 0xca) return 2;
        if (opcode == 0xc7 || opcode == 0xf7) return modrm_reg <= 1 ? (rex_w ? 4 : operand_size) : 0;
        if (opcode >= 0xb0 && opcode <= 0xb7) return 1;
        if (opcode >= 0xb8 && opcode <= 0xbf) return rex_w ? 8 : operand_size;
        if (opcode == 0xe8 || opcode == 0xe9) return 4;
        if (opcode == 0xeb || (opcode >= 0x70 && opcode <= 0x7f)) return 1;
        if (opcode == 0xa0 || opcode == 0xa1 || opcode == 0xa2 || opcode == 0xa3) return rex_w ? 8 : 4;
        return 0;
    }
    if ((opcode2 >= 0x80 && opcode2 <= 0x8f)) return 4;
    if (opcode2 == 0x70 || opcode2 == 0x71 || opcode2 == 0x72 || opcode2 == 0x73 || opcode2 == 0xba) return 1;
    return 0;
}

/// Constructs a complete instruction mask while masking ModRM register fields and values.
std::vector<Byte> x86_instruction_mask(std::size_t length, std::optional<ModRmInfo> modrm,
                                        std::size_t immediate_offset, std::size_t immediate_size,
                                        bool rex_present, std::size_t rex_offset,
                                        bool fixed_modrm_reg) {
    std::vector<Byte> mask(length, 0xffU);
    // The REX dispatcher fixes the 0100 high nibble and rex.W, while rex.R/X/B
    // are variable context fields selected by the register productions.
    if (rex_present) mask[rex_offset] = 0xf8U;
    if (modrm) {
        mask[modrm->modrm_offset] &= 0xc0U;
        if (fixed_modrm_reg) mask[modrm->modrm_offset] |= 0x38U;
        if (modrm->mod != 3 && (modrm->rm == 4 || (modrm->mod == 0 && modrm->rm == 5))) {
            mask[modrm->modrm_offset] |= static_cast<Byte>(modrm->rm & 7);
        }
        if (modrm->sib_offset) mask[*modrm->sib_offset] = 0;
        if (modrm->displacement_offset) {
            std::fill(mask.begin() + static_cast<std::ptrdiff_t>(*modrm->displacement_offset),
                      mask.begin() + static_cast<std::ptrdiff_t>(*modrm->displacement_offset + modrm->displacement_size), 0);
        }
    }
    if (immediate_size != 0) {
        std::fill(mask.begin() + static_cast<std::ptrdiff_t>(immediate_offset),
                  mask.begin() + static_cast<std::ptrdiff_t>(immediate_offset + immediate_size), 0);
    }
    return mask;
}

/// Creates a non-empty operand mask for an instruction operand.
std::vector<Byte> x86_operand_mask(std::size_t length) { return std::vector<Byte>(length, 0xffU); }

#endif
} // namespace

namespace fid {

/// Tests the terminator flag defined by FunctionRecord.java.
bool FunctionRecord::has_terminator() const noexcept {
    return (flags & 0x01U) != 0;
}
/// Tests the auto-pass flag defined by FunctionRecord.java.
bool FunctionRecord::auto_pass() const noexcept {
    return (flags & 0x02U) != 0;
}
/// Tests the auto-fail flag defined by FunctionRecord.java.
bool FunctionRecord::auto_fail() const noexcept {
    return (flags & 0x04U) != 0;
}
/// Tests the force-specific flag defined by FunctionRecord.java.
bool FunctionRecord::force_specific() const noexcept {
    return (flags & 0x08U) != 0;
}
/// Tests the force-relation flag defined by FunctionRecord.java.
bool FunctionRecord::force_relation() const noexcept {
    return (flags & 0x10U) != 0;
}
/// Returns the sum of all score components.
float Match::overall_score() const noexcept {
    return function_score + child_score + parent_score;
}

/// Hashes instruction bytes, operand objects, and masks as MessageDigestFidHasher does.
std::expected<HashQuad, Error> Hasher::hash(std::span<const Instruction> instructions, std::int8_t short_limit) {
    try {
        require(short_limit > 0, ErrorCode::invalid_input, "short hash limit must be positive");
        require(instructions.size() >= static_cast<std::size_t>(short_limit), ErrorCode::invalid_input,
                "function has too few code units for a FID hash");
        std::uint64_t full = 14695981039346656037ULL;
        std::uint64_t specific = full;
        int effective_count = 0;
        int call_count = 0;
        int specific_count = 0;
        for (const auto& instruction : instructions) {
            if (instruction.skip || is_x86_skipped(instruction.bytes)) {
                continue;
            }
            require(instruction.bytes.size() <= 110000, ErrorCode::invalid_input,
                    "instruction exceeds FunctionID hasher buffer");
            const bool missing_mask = instruction.instruction_mask.empty();
            const auto mask =
                missing_mask ? std::vector<Byte>(instruction.bytes.size(), 0) : instruction.instruction_mask;
            require(mask.size() == instruction.bytes.size(), ErrorCode::invalid_input,
                    "instruction mask length differs from instruction bytes");
            require(instruction.operand_masks.size() == instruction.operands.size(), ErrorCode::invalid_input,
                    "operand mask and object counts differ");
            ++effective_count;
            if (instruction.is_call)
                ++call_count;
            for (std::size_t operand_index = 0; operand_index < instruction.operands.size(); ++operand_index) {
                const auto& operand_mask = instruction.operand_masks[operand_index];
                if (operand_mask.empty())
                    continue;
                std::uint32_t specific_update = static_cast<std::uint32_t>((operand_index + 1) * 7777U);
                std::uint32_t full_update = specific_update;
                for (const auto& object : instruction.operands[operand_index]) {
                    if (object.kind == OperandObjectKind::scalar) {
                        auto value = static_cast<std::int32_t>(object.value);
                        if (object.relocated || object.address_scalar) {
                            value = static_cast<std::int32_t>(scalar_placeholder);
                        } else if (object.whole_scalar) {
                            ++specific_count;
                        } else if (value >= 256 || value <= -256) {
                            value = static_cast<std::int32_t>(scalar_placeholder);
                        } else {
                            ++specific_count;
                        }
                        specific_update += scalar_mix(value);
                        full_update += scalar_placeholder;
                    } else if (object.kind == OperandObjectKind::register_value) {
                        const auto mixed = static_cast<std::uint32_t>(
                            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(object.value)) + 7654321U) * 98777U);
                        specific_update += mixed;
                        full_update += mixed;
                    } else {
                        specific_update += scalar_mix(static_cast<std::int32_t>(scalar_placeholder));
                        full_update += scalar_placeholder;
                    }
                }
                update_int(full, full_update);
                update_int(specific, specific_update);
            }
            std::vector<Byte> masked(instruction.bytes.size());
            if (missing_mask) {
                std::fill(masked.begin(), masked.end(), 0xa5U);
            } else {
                for (std::size_t index = 0; index < masked.size(); ++index) {
                    masked[index] = instruction.bytes[index] & mask[index];
                }
            }
            update_bytes(full, masked);
            update_bytes(specific, masked);
            if (effective_count >= std::numeric_limits<std::int16_t>::max() - 1)
                break;
        }
        require(effective_count >= short_limit, ErrorCode::invalid_input,
                "function has too few effective code units for a FID hash");
        HashQuad result;
        result.code_unit_size = static_cast<std::int16_t>(effective_count - call_count);
        result.full_hash = full;
        result.specific_hash_additional_size = static_cast<std::int8_t>(std::min(specific_count, 127));
        result.specific_hash = specific;
        return result;
    } catch (const ParseException& exception) {
        return std::unexpected(exception.error);
    }
}

/// Adapts Sleigh's exact decoded prototype metadata to MessageDigestFidHasher's abstract inputs.
// Ghidra references: Ghidra/Features/FunctionID/src/main/java/ghidra/feature/fid/hash/MessageDigestFidHasher.java
// and
// Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java
std::expected<HashQuad, Error> Hasher::hash_sleigh(std::span<const sleigh_runtime::Instruction> instructions,
                                                   std::span<const Relocation> relocations, std::int8_t short_limit) {
    try {
        std::vector<Instruction> converted;
        converted.reserve(instructions.size());
        for (const auto& decoded : instructions) {
            Instruction instruction;
            instruction.bytes = decoded.bytes;
            instruction.instruction_mask = decoded.instruction_mask;
            require(instruction.bytes.size() == decoded.length &&
                        (instruction.instruction_mask.empty() || instruction.instruction_mask.size() == decoded.length),
                    ErrorCode::invalid_input, "Sleigh instruction bytes and mask lengths differ");
            instruction.is_call = decoded.flow.kind == sleigh_runtime::FlowKind::call ||
                                  decoded.flow.kind == sleigh_runtime::FlowKind::indirect_call;
            for (const auto& decoded_operand : decoded.operands) {
                instruction.operand_masks.push_back(decoded_operand.value_mask);
                std::vector<OperandObject> objects;
                std::size_t mask_begin = decoded_operand.value_mask.size();
                std::size_t mask_end = 0;
                for (std::size_t index = 0; index < decoded_operand.value_mask.size(); ++index) {
                    if (decoded_operand.value_mask[index] != 0) {
                        mask_begin = std::min(mask_begin, index);
                        mask_end = index + 1;
                    }
                }
                for (const auto& decoded_object : decoded_operand.hash_objects) {
                    const auto relocated = [&]() {
                        if (mask_begin == decoded_operand.value_mask.size())
                            return decoded_object.relocated;
                        for (const auto& relocation : relocations) {
                            const auto operand_begin = decoded.address + mask_begin;
                            const auto operand_end = decoded.address + mask_end;
                            const auto relocation_end = relocation.address + relocation.size;
                            if (relocation.size != 0 && relocation.address < operand_end &&
                                operand_begin < relocation_end)
                                return true;
                        }
                        return decoded_object.relocated;
                    }();
                    objects.push_back(OperandObject{
                        decoded_object.kind == sleigh_runtime::Operand::HashObject::Kind::scalar
                            ? OperandObjectKind::scalar
                        : decoded_object.kind == sleigh_runtime::Operand::HashObject::Kind::register_value
                            ? OperandObjectKind::register_value
                            : OperandObjectKind::address,
                        decoded_object.value, decoded_object.whole_scalar, decoded_object.address_scalar, relocated});
                }
                instruction.operands.push_back(std::move(objects));
            }
            converted.push_back(std::move(instruction));
        }
        return hash(converted, short_limit);
    } catch (const ParseException& exception) {
        return std::unexpected(exception.error);
    }
}

#if 0
/// Retired: decoding belongs to NEW/features/sleigh_runtime.
std::expected<std::vector<Instruction>, Error> X86Decoder::decode(std::span<const Byte> bytes,
                                                                   std::uint64_t address) {
    try {
        require(!bytes.empty(), ErrorCode::invalid_input, "cannot decode an empty x86 function");
        std::vector<Instruction> result;
        std::size_t cursor = 0;
        while (cursor < bytes.size()) {
            const auto start = cursor;
            bool operand16 = false;
            bool address32 = false;
            bool rex_present = false;
            std::uint8_t rex = 0;
            std::size_t rex_offset = 0;
            while (cursor < bytes.size()) {
                const auto prefix = bytes[cursor];
                if (prefix == 0x66) {
                    operand16 = true;
                    ++cursor;
                } else if (prefix == 0x67) {
                    address32 = true;
                    ++cursor;
                } else if (prefix == 0xf0 || prefix == 0xf2 || prefix == 0xf3 || prefix == 0x2e ||
                           prefix == 0x36 || prefix == 0x3e || prefix == 0x26 || prefix == 0x64 ||
                           prefix == 0x65) {
                    ++cursor;
                } else if (prefix >= 0x40 && prefix <= 0x4f) {
                    rex_present = true;
                    rex = prefix & 0x0fU;
                    rex_offset = cursor++;
                } else {
                    break;
                }
            }
            require(cursor < bytes.size(), ErrorCode::invalid_input, "x86 instruction contains only prefixes");
            const auto opcode_offset = cursor;
            const auto opcode = bytes[cursor++];
            bool two_byte = false;
            bool three_byte = false;
            std::uint8_t opcode2 = 0;
            if (opcode == 0x0f) {
                two_byte = true;
                require(cursor < bytes.size(), ErrorCode::invalid_input, "truncated x86 two-byte opcode");
                opcode2 = bytes[cursor++];
                if (opcode2 == 0x38 || opcode2 == 0x3a) {
                    three_byte = true;
                    require(cursor < bytes.size(), ErrorCode::invalid_input, "truncated x86 three-byte opcode");
                    opcode2 = bytes[cursor++];
                }
            }
            std::optional<ModRmInfo> modrm;
            if (x86_has_modrm(opcode, opcode2, two_byte, three_byte)) {
                const auto [register_width, rm_width] = x86_modrm_widths(
                    opcode, opcode2, two_byte, (rex & 8U) != 0, operand16);
                modrm = parse_modrm(bytes, cursor, address32, rex, register_width, rm_width);
                cursor = modrm->end;
            }
            const auto immediate_size = x86_immediate_size(opcode, opcode2, two_byte,
                                                           (rex & 8U) != 0, operand16,
                                                           modrm ? modrm->reg : 0);
            const auto immediate_offset = cursor;
            require(cursor + immediate_size <= bytes.size(), ErrorCode::invalid_input,
                    "truncated x86 immediate");
            cursor += immediate_size;
            require(cursor > start && cursor - start <= 15, ErrorCode::unsupported_architecture,
                    "x86 instruction exceeds the architectural length limit");

            Instruction instruction;
            instruction.bytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(start),
                                     bytes.begin() + static_cast<std::ptrdiff_t>(cursor));
            instruction.is_call = (!two_byte && (opcode == 0xe8 || opcode == 0x9a)) ||
                                  (!two_byte && opcode == 0xff && modrm && modrm->reg == 2);
            const bool fixed_modrm_reg = modrm &&
                ((!two_byte && (opcode == 0x80 || opcode == 0x81 || opcode == 0x82 || opcode == 0x83 ||
                                 opcode == 0xc0 || opcode == 0xc1 || opcode == 0xc6 || opcode == 0xc7 ||
                                 opcode == 0xf6 || opcode == 0xf7 || opcode == 0xfe || opcode == 0xff ||
                                 opcode == 0x8f)) ||
                 (two_byte && (opcode2 == 0x1f || opcode2 == 0xba)));
            auto local_modrm = modrm;
            if (local_modrm) {
                local_modrm->modrm_offset -= start;
                if (local_modrm->sib_offset) *local_modrm->sib_offset -= start;
                if (local_modrm->displacement_offset) *local_modrm->displacement_offset -= start;
            }
            instruction.instruction_mask = x86_instruction_mask(
                instruction.bytes.size(), local_modrm, immediate_offset - start, immediate_size,
                rex_present, rex_present ? rex_offset - start : 0, fixed_modrm_reg);
            if ((!two_byte && ((opcode >= 0x50 && opcode <= 0x5f) || (opcode >= 0xb0 && opcode <= 0xbf))) &&
                opcode_offset >= start) {
                instruction.instruction_mask[opcode_offset - start] &= 0xf8U;
            }

            if (modrm) {
                const bool one_operand = (!two_byte && (opcode == 0x80 || opcode == 0x81 || opcode == 0x82 ||
                                                        opcode == 0x83 || opcode == 0x8f || opcode == 0xc0 ||
                                                        opcode == 0xc1 || opcode == 0xc6 || opcode == 0xc7 ||
                                                        opcode == 0xf6 || opcode == 0xf7 || opcode == 0xfe ||
                                                        opcode == 0xff)) ||
                                         (two_byte && (opcode2 == 0x1f || opcode2 == 0xba));
                const bool reverse = (!two_byte && (opcode == 0x8b || opcode == 0x8d || opcode == 0x8e)) ||
                                     (two_byte && opcode2 != 0x1f && opcode2 != 0xba);
                if (one_operand) {
                    instruction.operands.push_back(modrm->rm_objects);
                    instruction.operand_masks.push_back(x86_operand_mask(instruction.bytes.size()));
                } else if (reverse) {
                    instruction.operands.push_back(modrm->reg_objects);
                    instruction.operand_masks.push_back(x86_operand_mask(instruction.bytes.size()));
                    instruction.operands.push_back(modrm->rm_objects);
                    instruction.operand_masks.push_back(x86_operand_mask(instruction.bytes.size()));
                } else {
                    instruction.operands.push_back(modrm->rm_objects);
                    instruction.operand_masks.push_back(x86_operand_mask(instruction.bytes.size()));
                    instruction.operands.push_back(modrm->reg_objects);
                    instruction.operand_masks.push_back(x86_operand_mask(instruction.bytes.size()));
                }
            }
            if (immediate_size != 0) {
                const auto raw_value = read_le(bytes, immediate_offset, immediate_size == 8 ? 8 : immediate_size);
                const bool relative = (!two_byte && (opcode == 0xe8 || opcode == 0xe9 || opcode == 0xeb ||
                                                      (opcode >= 0x70 && opcode <= 0x7f))) ||
                                       (two_byte && opcode2 >= 0x80 && opcode2 <= 0x8f);
                std::int64_t value = static_cast<std::int64_t>(raw_value);
                if (relative && immediate_size == 1 && (value & 0x80) != 0) value -= 0x100;
                if (relative && immediate_size == 4 && (value & 0x80000000LL) != 0) value -= 0x100000000LL;
                if (relative) value += static_cast<std::int64_t>(address + cursor);
                instruction.operands.push_back({OperandObject{relative ? OperandObjectKind::address
                                                                       : OperandObjectKind::scalar,
                                                               value, true, relative, false}});
                instruction.operand_masks.push_back(x86_operand_mask(instruction.bytes.size()));
            }
            if (!two_byte && opcode >= 0x50 && opcode <= 0x5f) {
                const auto register_index = static_cast<int>(opcode - 0x50U) + ((rex & 1U) != 0 ? 8 : 0);
                const auto width = operand16 ? 2 : 8;
                instruction.operands.insert(instruction.operands.begin(), {x86_register(register_index, width)});
                instruction.operand_masks.insert(instruction.operand_masks.begin(),
                                                  x86_operand_mask(instruction.bytes.size()));
            } else if (!two_byte && opcode >= 0xb8 && opcode <= 0xbf) {
                const auto register_index = static_cast<int>(opcode - 0xb8U) + ((rex & 1U) != 0 ? 8 : 0);
                const auto width = (rex & 8U) != 0 ? 8 : (operand16 ? 2 : 4);
                instruction.operands.insert(instruction.operands.begin(), {x86_register(register_index, width)});
                instruction.operand_masks.insert(instruction.operand_masks.begin(),
                                                  x86_operand_mask(instruction.bytes.size()));
            }
            instruction.skip = is_x86_skipped(instruction.bytes);
            result.push_back(std::move(instruction));
            address += cursor - start;
        }
        return result;
    } catch (const ParseException& exception) {
        return std::unexpected(exception.error);
    }
}

/// Decodes raw x86-64 bytes before applying the normal FunctionID hash pipeline.
std::expected<HashQuad, Error> Hasher::hash_x86_64(std::span<const Byte> bytes, std::uint64_t address) {
    const auto decoded = X86Decoder::decode(bytes, address);
    if (!decoded) return std::unexpected(decoded.error());
    return hash(*decoded);
}
#endif

/// Computes the superior relation key exactly as FidDBUtils.generateSuperiorFullHashSmash().
std::uint64_t superior_relation_key(std::int64_t superior_id, std::uint64_t inferior_full_hash) noexcept {
    return static_cast<std::uint64_t>(superior_id) * fnv_prime ^ inferior_full_hash;
}

/// Computes the inferior relation key exactly as FidDBUtils.generateInferiorFullHashSmash().
std::uint64_t inferior_relation_key(std::uint64_t superior_full_hash, std::int64_t inferior_id) noexcept {
    return static_cast<std::uint64_t>(inferior_id) * fnv_prime ^ superior_full_hash;
}

class Database::Storage {
public:
    /// Owns parsed immutable tables and relation indexes for one database.
    Storage(BufferFile file) : file(std::move(file)) {}

    BufferFile file;
    std::vector<LibraryRecord> libraries;
    std::vector<FunctionRecord> functions;
    std::unordered_map<std::int64_t, LibraryRecord> libraries_by_id;
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> full_index;
    std::unordered_set<std::uint64_t> superior_relations;
    std::unordered_set<std::uint64_t> inferior_relations;
};

/// Opens a packed database, reads its Ghidra schema, and materializes only FunctionID tables.
std::expected<Database, Error> Database::open(const std::filesystem::path& path) {
    try {
        auto bytes = read_file(path);
        std::vector<Byte> raw;
        if (bytes.size() >= 8 && read_be(bytes, 0, 8) == buffer_magic) {
            raw = std::move(bytes);
        } else {
            raw = unpack_fidb(bytes);
        }
        BufferFile buffer_file(std::move(raw));
        auto storage = std::make_unique<Storage>(std::move(buffer_file));

        const std::vector<std::uint8_t> master_fields = {4, 2, 2, 0, 5, 4, 2, 3, 2};
        std::vector<TableDescriptor> descriptors;
        const auto master_root = storage->file.parameter(0);
        walk_long_tree(storage->file, master_root, 0, master_fields,
                       [&descriptors, &master_fields](std::int64_t key, std::span<const Byte> data) {
                           const auto values = read_record_fields(data, master_fields);
                           require(values.size() == 9, ErrorCode::malformed_record,
                                   "master table record has an unexpected field count");
                           TableDescriptor descriptor;
                           descriptor.name = string_value(values[0]);
                           descriptor.version = static_cast<std::int32_t>(integer_value(values[1]));
                           descriptor.root_buffer_id = static_cast<std::int32_t>(integer_value(values[2]));
                           descriptor.key_type = static_cast<std::uint8_t>(integer_value(values[3]));
                           descriptor.field_types = std::get<std::vector<Byte>>(values[4]);
                           descriptor.indexed_column = static_cast<std::int32_t>(integer_value(values[6]));
                           descriptor.record_count = static_cast<std::int32_t>(integer_value(values[8]));
                           (void)key;
                           descriptors.push_back(std::move(descriptor));
                       });
        auto find_table = [&descriptors](std::string_view name) -> const TableDescriptor& {
            const auto iterator = std::find_if(descriptors.begin(), descriptors.end(),
                                               [name](const auto& descriptor) { return descriptor.name == name; });
            require(iterator != descriptors.end(), ErrorCode::invalid_database,
                    "required FunctionID table is missing: " + std::string(name));
            return *iterator;
        };
        const auto& library_table = find_table("Libraries Table");
        const auto& strings_table = find_table("Strings Table");
        const auto& functions_table = find_table("Functions Table");
        const auto& inferior_table = find_table("Inferior Table");
        const auto& superior_table = find_table("Superior Table");
        require(library_table.version == 6 && functions_table.version == 6 && inferior_table.version == 6 &&
                    superior_table.version == 6,
                ErrorCode::unsupported_schema, "unsupported FunctionID database schema version");

        std::unordered_map<std::int64_t, std::string> strings;
        const std::vector<std::uint8_t> string_fields = {4};
        walk_long_tree(storage->file, strings_table.root_buffer_id, 0, string_fields,
                       [&strings, &string_fields](std::int64_t key, std::span<const Byte> data) {
                           const auto values = read_record_fields(data, string_fields);
                           require(values.size() == 1, ErrorCode::malformed_record, "invalid strings-table record");
                           strings.emplace(key, string_value(values[0]));
                       });

        const std::vector<std::uint8_t> library_fields = {4, 4, 4, 4, 4, 2, 2, 4};
        walk_long_tree(storage->file, library_table.root_buffer_id, 0, library_fields,
                       [&storage, &library_fields](std::int64_t key, std::span<const Byte> data) {
                           const auto values = read_record_fields(data, library_fields);
                           require(values.size() == 8, ErrorCode::malformed_record, "invalid libraries-table record");
                           LibraryRecord library;
                           library.id = key;
                           library.family_name = string_value(values[0]);
                           library.version = string_value(values[1]);
                           library.variant = string_value(values[2]);
                           library.ghidra_version = string_value(values[3]);
                           library.language_id = string_value(values[4]);
                           library.language_version = static_cast<std::int32_t>(integer_value(values[5]));
                           library.language_minor_version = static_cast<std::int32_t>(integer_value(values[6]));
                           library.metadata = string_value(values[7]);
                           parse_metadata(library);
                           storage->libraries_by_id.emplace(key, library);
                           storage->libraries.push_back(std::move(library));
                       });

        const std::vector<std::uint8_t> function_fields = {1, 3, 0, 3, 3, 3, 3, 3, 0};
        constexpr std::size_t function_record_length = 52;
        walk_long_tree(storage->file, functions_table.root_buffer_id, function_record_length, function_fields,
                       [&storage, &strings, &function_fields](std::int64_t key, std::span<const Byte> data) {
                           const auto values = read_record_fields(data, function_fields);
                           require(values.size() == 9, ErrorCode::malformed_record, "invalid functions-table record");
                           FunctionRecord function;
                           function.id = key;
                           function.hash.code_unit_size = static_cast<std::int16_t>(integer_value(values[0]));
                           function.hash.full_hash = static_cast<std::uint64_t>(integer_value(values[1]));
                           function.hash.specific_hash_additional_size =
                               static_cast<std::int8_t>(integer_value(values[2]));
                           function.hash.specific_hash = static_cast<std::uint64_t>(integer_value(values[3]));
                           function.library_id = integer_value(values[4]);
                           const auto name_id = integer_value(values[5]);
                           const auto path_id = integer_value(values[7]);
                           function.name = strings.contains(name_id) ? strings.at(name_id) : std::string{};
                           function.entry_point = integer_value(values[6]);
                           function.domain_path = strings.contains(path_id) ? strings.at(path_id) : std::string{};
                           function.flags = static_cast<std::uint8_t>(integer_value(values[8]));
                           storage->full_index[function.hash.full_hash].push_back(storage->functions.size());
                           storage->functions.push_back(std::move(function));
                       });

        const auto read_relations = [&storage](const TableDescriptor& descriptor, auto& destination) {
            walk_long_tree(storage->file, descriptor.root_buffer_id, 0, {},
                           [&destination](std::int64_t key, std::span<const Byte>) {
                               destination.insert(static_cast<std::uint64_t>(key));
                           });
        };
        read_relations(inferior_table, storage->inferior_relations);
        read_relations(superior_table, storage->superior_relations);
        return Database(std::move(storage));
    } catch (const ParseException& exception) {
        return std::unexpected(exception.error);
    } catch (const std::exception& exception) {
        return std::unexpected(Error{ErrorCode::invalid_database, exception.what()});
    }
}

/// Constructs a database from immutable parsed storage.
Database::Database(std::unique_ptr<Storage> storage) : storage_(std::move(storage)) {}
/// Releases the parsed database storage.
Database::~Database() = default;
/// Transfers database ownership without copying large table vectors.
Database::Database(Database&&) noexcept = default;
/// Transfers database ownership without copying large table vectors.
Database& Database::operator=(Database&&) noexcept = default;
/// Returns library records in their decoded primary-key order.
std::span<const LibraryRecord> Database::libraries() const noexcept {
    return storage_->libraries;
}
/// Looks up all functions sharing a full hash through the materialized full-hash index.
std::vector<FunctionRecord> Database::find_full_hash(std::uint64_t hash) const {
    std::vector<FunctionRecord> result;
    if (const auto iterator = storage_->full_index.find(hash); iterator != storage_->full_index.end()) {
        result.reserve(iterator->second.size());
        for (const auto index : iterator->second)
            result.push_back(storage_->functions[index]);
    }
    return result;
}
/// Performs the intentionally unindexed specific-hash scan used by FunctionsTable.
std::vector<FunctionRecord> Database::find_specific_hash(std::uint64_t hash) const {
    std::vector<FunctionRecord> result;
    for (const auto& function : storage_->functions) {
        if (function.hash.specific_hash == hash)
            result.push_back(function);
    }
    return result;
}
/// Returns every decoded function record.
std::span<const FunctionRecord> Database::functions() const noexcept {
    return storage_->functions;
}
/// Returns the usable database buffer size.
std::size_t Database::buffer_size() const noexcept {
    return storage_->file.buffer_size();
}

/// Applies FidProgramSeeker.scoreMatch() to every full-hash candidate.
std::expected<IdentificationResult, Error> Database::identify(const FunctionContext& context,
                                                              const ProgramInfo& program, float score_threshold,
                                                              std::int16_t medium_limit) const {
    try {
        require(storage_ != nullptr, ErrorCode::invalid_input, "database is not initialized");
        IdentificationResult result;
        result.hash = context.hash;
        std::vector<Match> candidates;
        std::set<std::uint64_t> child_hashes;
        std::set<std::uint64_t> parent_hashes;
        for (const auto& child : context.children)
            child_hashes.insert(child.full_hash);
        for (const auto& parent : context.parents)
            parent_hashes.insert(parent.full_hash);
        const auto candidates_by_hash = find_full_hash(context.hash.full_hash);
        for (const auto& function : candidates_by_hash) {
            if (function.auto_fail() ||
                function.force_specific() && function.hash.specific_hash != context.hash.specific_hash) {
                continue;
            }
            const auto library_iterator = storage_->libraries_by_id.find(function.library_id);
            if (library_iterator == storage_->libraries_by_id.end() ||
                !library_matches(library_iterator->second, program)) {
                continue;
            }
            int child_units = 0;
            for (const auto& child : context.children) {
                if (storage_->superior_relations.contains(superior_relation_key(function.id, child.full_hash))) {
                    child_units += child.code_unit_size;
                }
            }
            if (function.force_relation() && child_units == 0)
                continue;
            int parent_units = 0;
            if (parent_hashes.size() < 500) {
                for (const auto& parent : context.parents) {
                    if (storage_->inferior_relations.contains(inferior_relation_key(parent.full_hash, function.id))) {
                        parent_units += parent.code_unit_size;
                    }
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
    } catch (const ParseException& exception) {
        return std::unexpected(exception.error);
    }
}

} // namespace fid
