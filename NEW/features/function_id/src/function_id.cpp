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
constexpr std::string_view fid_zip_entry_name = "FOLDER_ITEM";
/// Matches db.ChainedBuffer.XOR_MASK_BYTES from the original Ghidra database layer.
constexpr std::array<Byte, 128> chained_xor_mask = {
    0x59, 0xea, 0x67, 0x23, 0xda, 0xb8, 0x00, 0xb8, 0xc3, 0x48, 0xdd, 0x8b, 0x21, 0xd6, 0x94, 0x78, 0x35, 0xab, 0x2b,
    0x7e, 0xb2, 0x4f, 0x82, 0x4e, 0x0e, 0x16, 0xc4, 0x57, 0x12, 0x8e, 0x7e, 0xe6, 0xb6, 0xbd, 0x56, 0x91, 0x57, 0x72,
    0xe6, 0x91, 0xdc, 0x52, 0x2e, 0xf2, 0x1a, 0xb7, 0xd6, 0x6f, 0xda, 0xde, 0xe8, 0x48, 0xb1, 0xbb, 0x50, 0x6f, 0xf4,
    0xdd, 0x11, 0xee, 0xf2, 0x67, 0xfe, 0x48, 0x8d, 0xae, 0x69, 0x1a, 0xe0, 0x26, 0x8c, 0x24, 0x8e, 0x17, 0x76, 0x51,
    0xe2, 0x60, 0xd7, 0xe6, 0x83, 0x65, 0xd5, 0xf0, 0x7f, 0xf2, 0xa0, 0xd6, 0x4b, 0xbd, 0x24, 0xd8, 0xab, 0xea, 0x9e,
    0xa6, 0x48, 0x94, 0x3e, 0x7b, 0x2c, 0xf4, 0xce, 0xdc, 0x69, 0x11, 0xf8, 0x3c, 0xa7, 0x3f, 0x5d, 0x77, 0x94, 0x3f,
    0xe4, 0x8e, 0x48, 0x20, 0xdb, 0x56, 0x32, 0xc1, 0x87, 0x01, 0x2e, 0xe3, 0x7f, 0x40,
};

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
    [[maybe_unused]] const auto cleanup =
        std::unique_ptr<z_stream, void (*)(z_stream*)>(&stream, [](z_stream* state) { inflateEnd(state); });

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
    require(name_length == fid_zip_entry_name.size() &&
                std::equal(fid_zip_entry_name.begin(), fid_zip_entry_name.end(), packed.begin() + cursor + 30),
            fid::ErrorCode::invalid_zip, "unexpected ZIP entry name");
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

    /// Returns the number of logical database buffers addressable after the file header.
    [[nodiscard]] std::size_t buffer_count() const noexcept {
        return bytes_.size() / block_size_ - 1;
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
            require(length <= first.size() - 5, fid::ErrorCode::invalid_database,
                    "direct chained DBBuffer exceeds its data node");
            std::copy_n(first.begin() + 5, length, result.begin());
            if (obfuscated) {
                xor_bytes(result, 0, length);
            }
            return result;
        }
        require(first[0] == 8, fid::ErrorCode::invalid_database, "invalid chained DBBuffer node type");
        const auto data_space = buffer_size_ - 1;
        const auto ids_per_index = (buffer_size_ - 9) / 4;
        std::size_t copied = 0;
        std::int32_t index_id = first_id;
        bool first_index = true;
        std::set<std::int32_t> visited_indexes;
        while (index_id >= 0 && copied < length) {
            require(visited_indexes.insert(index_id).second, fid::ErrorCode::invalid_database,
                    "cycle detected in chained DBBuffer index nodes");
            const auto index_buffer = buffer(index_id);
            require(index_buffer[0] == 8, fid::ErrorCode::invalid_database, "invalid chained DBBuffer index node type");
            const auto next = static_cast<std::int32_t>(read_be(index_buffer, 5, 4));
            const auto index_base = std::size_t{9};
            const auto needed = (length - copied + data_space - 1) / data_space;
            const auto count = std::min(ids_per_index, needed);
            for (std::size_t slot = 0; slot < count; ++slot) {
                const auto data_id = static_cast<std::int32_t>(read_be(index_buffer, index_base + slot * 4, 4));
                require(data_id >= 0, fid::ErrorCode::invalid_database, "missing chained data buffer");
                const auto data_buffer = buffer(data_id);
                require(data_buffer[0] == 9, fid::ErrorCode::invalid_database,
                        "invalid chained DBBuffer data node type");
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
        for (std::size_t index = 0; index < length; ++index) {
            data[offset + index] ^= chained_xor_mask[index % chained_xor_mask.size()];
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

/// Validates the schema subset required by the native FunctionID table decoder.
void validate_table_descriptor(const TableDescriptor& descriptor, std::initializer_list<std::uint8_t> fields) {
    constexpr std::uint8_t long_key_type = 3;
    require(descriptor.key_type == long_key_type, fid::ErrorCode::unsupported_schema,
            "FunctionID table does not use a long primary key");
    require(descriptor.indexed_column == -1, fid::ErrorCode::unsupported_schema,
            "FunctionID table is not using its primary key");
    require(descriptor.field_types.size() == fields.size(), fid::ErrorCode::unsupported_schema,
            "FunctionID table has an unsupported field schema extension");
    auto expected = fields.begin();
    for (const auto encoded : descriptor.field_types) {
        require((encoded & 0x0fU) == *expected, fid::ErrorCode::unsupported_schema,
                "FunctionID table field schema does not match the supported contract");
        ++expected;
    }
}

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
            if (length < 0)
                return std::monostate{};
            require(static_cast<std::size_t>(length) <= data.size() - cursor, fid::ErrorCode::malformed_record,
                    "invalid string length");
            std::string value(reinterpret_cast<const char*>(data.data() + cursor), static_cast<std::size_t>(length));
            cursor += static_cast<std::size_t>(length);
            return value;
        }
        case FieldType::binary: {
            require(cursor + 4 <= data.size(), fid::ErrorCode::malformed_record, "truncated binary length");
            const auto length = static_cast<std::int32_t>(signed_be(data, cursor, 4));
            cursor += 4;
            if (length < 0)
                return std::monostate{};
            require(static_cast<std::size_t>(length) <= data.size() - cursor, fid::ErrorCode::malformed_record,
                    "invalid binary length");
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
    if (std::holds_alternative<std::monostate>(value)) {
        static const std::string empty;
        return empty;
    }
    throw ParseException(fid::ErrorCode::malformed_record, "expected string database field");
}

/// Returns a binary schema value, treating Ghidra's null field encoding as an empty value.
const std::vector<Byte>& binary_value(const Value& value) {
    if (const auto* binary = std::get_if<std::vector<Byte>>(&value)) {
        return *binary;
    }
    if (std::holds_alternative<std::monostate>(value)) {
        static const std::vector<Byte> empty;
        return empty;
    }
    throw ParseException(fid::ErrorCode::malformed_record, "expected binary database field");
}

/// Walks the long-key B-tree storage used by all primary FunctionID tables.
template <typename Visitor>
void walk_long_tree(const BufferFile& file, std::int32_t root, std::size_t fixed_record_length,
                    const std::vector<std::uint8_t>& fields, Visitor&& visitor) {
    std::set<std::int32_t> visited;
    std::optional<std::int64_t> previous_key;
    std::function<void(std::int32_t, std::size_t)> walk = [&](std::int32_t buffer_id, std::size_t depth) {
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
            for (std::size_t index = 0; index < count; ++index) {
                walk(static_cast<std::int32_t>(signed_be(buffer, 5 + index * 12 + 8, 4)), depth + 1);
            }
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
        walk(root, 0);
    }
}

/// Splits a comma- or space-separated FunctionID metadata list into a set, preserving empty-as-unrestricted.
std::set<std::string> split_metadata(std::string_view text) {
    std::set<std::string> result;
    std::size_t start = 0;
    while (start < text.size()) {
        while (start < text.size() && (text[start] == ',' || std::isspace(static_cast<unsigned char>(text[start]))))
            ++start;
        const auto token_start = start;
        while (start < text.size() && text[start] != ',' && !std::isspace(static_cast<unsigned char>(text[start])))
            ++start;
        if (token_start < start)
            result.emplace(text.substr(token_start, start - token_start));
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
    if (!library.source_languages.empty() && program.source_languages.has_value()) {
        if (program.source_languages->empty())
            return false;
        bool intersects = false;
        for (const auto& source : *program.source_languages) {
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
            if (instruction.skip) {
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
            instruction.skip = decoded.is_x86 && is_x86_skipped(decoded.bytes);
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
                           descriptor.field_types = binary_value(values[4]);
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
        require(strings_table.version == 6 && library_table.version == 6 && functions_table.version == 6 &&
                    inferior_table.version == 6 && superior_table.version == 6,
                ErrorCode::unsupported_schema, "unsupported FunctionID database schema version");
        validate_table_descriptor(strings_table, {4});
        validate_table_descriptor(library_table, {4, 4, 4, 4, 4, 2, 2, 4});
        validate_table_descriptor(functions_table, {1, 3, 0, 3, 3, 3, 3, 3, 0});
        validate_table_descriptor(inferior_table, {});
        validate_table_descriptor(superior_table, {});

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
        std::map<std::uint64_t, HashQuad> child_hashes;
        std::map<std::uint64_t, HashQuad> parent_hashes;
        for (const auto& child : context.children)
            child_hashes[child.full_hash] = child;
        for (const auto& parent : context.parents)
            parent_hashes[parent.full_hash] = parent;
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
            for (const auto& [child_hash, child] : child_hashes) {
                if (storage_->superior_relations.contains(superior_relation_key(function.id, child_hash))) {
                    child_units += child.code_unit_size;
                }
            }
            if (function.force_relation() && child_units == 0)
                continue;
            int parent_units = 0;
            if (parent_hashes.size() < 500) {
                for (const auto& [parent_hash, parent] : parent_hashes) {
                    if (storage_->inferior_relations.contains(inferior_relation_key(parent_hash, function.id))) {
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
