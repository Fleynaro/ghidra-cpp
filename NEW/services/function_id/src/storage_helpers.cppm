module;
#include <zlib.h>

export module function_id:storage_helpers;

import std;
import :parse_exception;
import :types;

#if defined(_MSC_VER)
#pragma optimize("gty", on)
#endif

// Ghidra references:
// Framework/DB/src/main/java/db/buffers/BufferFile.java,
// Framework/DB/src/main/java/db/buffers/LocalBufferFile.java,
// Framework/DB/src/main/java/db/ChainedBuffer.java,
// Framework/FileSystem/src/main/java/ghidra/framework/store/local/ItemSerializer.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/FidFile.java.

export namespace fid::detail {

inline constexpr std::uint64_t fnv_prime = 1099511628211ULL;
inline constexpr std::uint32_t scalar_placeholder = 0xfeeddeadU;
inline constexpr std::uint64_t packed_magic = 0x2e30212634e92c20ULL;
inline constexpr std::uint64_t buffer_magic = 0x2f30312c34292c2aULL;
inline constexpr std::string_view fid_content_type = "Function ID Database";
inline constexpr std::string_view fid_zip_entry_name = "FOLDER_ITEM";

/// Matches db.ChainedBuffer.XOR_MASK_BYTES from the original Ghidra database layer.
inline constexpr std::array<fid::Byte, 128> chained_xor_mask = {
    0x59, 0xea, 0x67, 0x23, 0xda, 0xb8, 0x00, 0xb8, 0xc3, 0x48, 0xdd, 0x8b, 0x21, 0xd6, 0x94, 0x78, 0x35, 0xab, 0x2b,
    0x7e, 0xb2, 0x4f, 0x82, 0x4e, 0x0e, 0x16, 0xc4, 0x57, 0x12, 0x8e, 0x7e, 0xe6, 0xb6, 0xbd, 0x56, 0x91, 0x57, 0x72,
    0xe6, 0x91, 0xdc, 0x52, 0x2e, 0xf2, 0x1a, 0xb7, 0xd6, 0x6f, 0xda, 0xde, 0xe8, 0x48, 0xb1, 0xbb, 0x50, 0x6f, 0xf4,
    0xdd, 0x11, 0xee, 0xf2, 0x67, 0xfe, 0x48, 0x8d, 0xae, 0x69, 0x1a, 0xe0, 0x26, 0x8c, 0x24, 0x8e, 0x17, 0x76, 0x51,
    0xe2, 0x60, 0xd7, 0xe6, 0x83, 0x65, 0xd5, 0xf0, 0x7f, 0xf2, 0xa0, 0xd6, 0x4b, 0xbd, 0x24, 0xd8, 0xab, 0xea, 0x9e,
    0xa6, 0x48, 0x94, 0x3e, 0x7b, 0x2c, 0xf4, 0xce, 0xdc, 0x69, 0x11, 0xf8, 0x3c, 0xa7, 0x3f, 0x5d, 0x77, 0x94, 0x3f,
    0xe4, 0x8e, 0x48, 0x20, 0xdb, 0x56, 0x32, 0xc1, 0x87, 0x01, 0x2e, 0xe3, 0x7f, 0x40,
};

/// Throws a typed parser exception when a storage invariant is false.
inline void require(bool condition, fid::ErrorCode code, std::string message) {
    if (!condition) {
        throw ParseException(code, std::move(message));
    }
}

/// Reads one big-endian unsigned integer from a bounded byte span.
inline std::uint64_t read_be(std::span<const fid::Byte> data, std::size_t offset, std::size_t width) {
    require(width <= 8 && offset <= data.size() && width <= data.size() - offset, fid::ErrorCode::malformed_record,
            "truncated big-endian value");
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < width; ++index) {
        value = (value << 8U) | data[offset + index];
    }
    return value;
}

/// Reads one little-endian unsigned integer from a bounded byte span.
inline std::uint64_t read_le(std::span<const fid::Byte> data, std::size_t offset, std::size_t width) {
    require(width <= 8 && offset <= data.size() && width <= data.size() - offset, fid::ErrorCode::invalid_zip,
            "truncated little-endian value");
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < width; ++index) {
        value |= static_cast<std::uint64_t>(data[offset + index]) << (index * 8U);
    }
    return value;
}

/// Interprets a big-endian byte sequence as a signed integer of the requested width.
inline std::int64_t signed_be(std::span<const fid::Byte> data, std::size_t offset, std::size_t width) {
    const auto value = read_be(data, offset, width);
    if (width == 8) {
        return static_cast<std::int64_t>(value);
    }
    const auto sign_bit = std::uint64_t{1} << (width * 8U - 1U);
    const auto mask = (std::uint64_t{1} << (width * 8U)) - 1U;
    return static_cast<std::int64_t>((value & sign_bit) != 0 ? value | ~mask : value);
}

/// Reads an entire binary file while preserving the original bytes for bounded parsing.
inline std::vector<fid::Byte> read_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw ParseException(fid::ErrorCode::io_failure, "unable to open database: " + path.string());
    }
    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    require(size >= 0, fid::ErrorCode::io_failure, "unable to determine database size");
    stream.seekg(0, std::ios::beg);
    std::vector<fid::Byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    require(stream.good() || stream.eof(), fid::ErrorCode::io_failure, "unable to read database");
    return bytes;
}

/// Inflates the raw DEFLATE stream used by ItemSerializer's single ZIP entry.
inline std::vector<fid::Byte> inflate_raw(std::span<const fid::Byte> compressed, std::size_t expected_size) {
    z_stream stream{};
    const int init_result = inflateInit2(&stream, -MAX_WBITS);
    require(init_result == Z_OK, fid::ErrorCode::invalid_zip, "unable to initialize DEFLATE decoder");
    [[maybe_unused]] const auto cleanup =
        std::unique_ptr<z_stream, void (*)(z_stream*)>(&stream, [](z_stream* state) { inflateEnd(state); });

    std::vector<fid::Byte> output;
    output.reserve(expected_size);
    std::array<fid::Byte, 32768> buffer{};
    stream.next_in = const_cast<Bytef*>(compressed.data());
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
            stream.next_in = const_cast<Bytef*>(compressed.data() + input_offset);
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

/// Extracts raw Ghidra buffer-file bytes from an original `.fidb` ItemSerializer container.
inline std::vector<fid::Byte> unpack_fidb(const std::vector<fid::Byte>& packed) {
    require(packed.size() >= 18, fid::ErrorCode::invalid_packed_item, "packed item is truncated");
    require(read_be(packed, 0, 4) == 0xaced0005U, fid::ErrorCode::invalid_packed_item,
            "missing Java serialization stream header");
    require(read_be(packed, 6, 8) == packed_magic, fid::ErrorCode::invalid_packed_item, "invalid packed-item magic");
    require(read_be(packed, 14, 4) == 1, fid::ErrorCode::invalid_packed_item, "unsupported packed-item format version");

    std::size_t cursor = 18;
    const auto read_modified_utf = [&packed, &cursor]() {
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
    const auto data = std::span<const fid::Byte>(packed).subspan(data_offset);
    return inflate_raw(data, static_cast<std::size_t>(length));
}

/// Identifies the Ghidra database field encodings used by the FunctionID tables.
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
inline void validate_table_descriptor(const TableDescriptor& descriptor, std::initializer_list<std::uint8_t> fields) {
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

/// Holds the value alternatives emitted by the schema-aware database decoder.
using Value = std::variant<std::monostate, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::string,
                           std::vector<fid::Byte>>;

/// Decodes one schema field from a Ghidra database record payload.
inline Value read_value(std::span<const fid::Byte> data, std::size_t& cursor, std::uint8_t encoded_type) {
    const auto type = static_cast<FieldType>(encoded_type & 0x0fU);
    switch (type) {
        case FieldType::byte:
            require(cursor + 1 <= data.size(), fid::ErrorCode::malformed_record, "truncated byte field");
            return static_cast<std::int8_t>(data[cursor++]);
        case FieldType::short_value: {
            require(cursor + 2 <= data.size(), fid::ErrorCode::malformed_record, "truncated short field");
            const auto value = static_cast<std::int16_t>(signed_be(data, cursor, 2));
            cursor += 2;
            return value;
        }
        case FieldType::int_value: {
            require(cursor + 4 <= data.size(), fid::ErrorCode::malformed_record, "truncated int field");
            const auto value = static_cast<std::int32_t>(signed_be(data, cursor, 4));
            cursor += 4;
            return value;
        }
        case FieldType::long_value: {
            require(cursor + 8 <= data.size(), fid::ErrorCode::malformed_record, "truncated long field");
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
            std::vector<fid::Byte> value(data.begin() + static_cast<std::ptrdiff_t>(cursor),
                                         data.begin() + cursor + length);
            cursor += static_cast<std::size_t>(length);
            return value;
        }
        case FieldType::boolean:
            require(cursor + 1 <= data.size(), fid::ErrorCode::malformed_record, "truncated boolean field");
            return static_cast<std::int8_t>(data[cursor++]);
        case FieldType::fixed10: {
            require(cursor + 10 <= data.size(), fid::ErrorCode::malformed_record, "truncated fixed field");
            std::vector<fid::Byte> value(data.begin() + static_cast<std::ptrdiff_t>(cursor),
                                         data.begin() + cursor + 10);
            cursor += 10;
            return value;
        }
    }
    throw ParseException(fid::ErrorCode::unsupported_schema, "unsupported Ghidra DB field type");
}

/// Decodes every ordinary field in a schema, stopping before schema extensions.
inline std::vector<Value> read_record_fields(std::span<const fid::Byte> data, const std::vector<std::uint8_t>& fields) {
    std::vector<Value> values;
    std::size_t cursor = 0;
    for (const auto type : fields) {
        if (type == 0xffU)
            break;
        values.push_back(read_value(data, cursor, type));
    }
    return values;
}

/// Returns a value as a signed 64-bit integer for fixed-width master-table fields.
inline std::int64_t integer_value(const Value& value) {
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
inline const std::string& string_value(const Value& value) {
    if (const auto* string = std::get_if<std::string>(&value))
        return *string;
    if (std::holds_alternative<std::monostate>(value)) {
        static const std::string empty;
        return empty;
    }
    throw ParseException(fid::ErrorCode::malformed_record, "expected string database field");
}

/// Returns a binary schema value, treating Ghidra's null field encoding as an empty value.
inline const std::vector<fid::Byte>& binary_value(const Value& value) {
    if (const auto* binary = std::get_if<std::vector<fid::Byte>>(&value))
        return *binary;
    if (std::holds_alternative<std::monostate>(value)) {
        static const std::vector<fid::Byte> empty;
        return empty;
    }
    throw ParseException(fid::ErrorCode::malformed_record, "expected binary database field");
}

/// Splits a comma- or space-separated FunctionID metadata list into a set.
inline std::set<std::string> split_metadata(std::string_view text) {
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
inline void parse_metadata(fid::LibraryRecord& library) {
    const auto separator = library.metadata.find(':');
    if (separator == std::string::npos) {
        library.compiler_specs = split_metadata(library.metadata);
        return;
    }
    library.compiler_specs = split_metadata(std::string_view(library.metadata).substr(0, separator));
    library.source_languages = split_metadata(std::string_view(library.metadata).substr(separator + 1));
}

/// Removes the final language variant component as ProcessorSizeComparator does.
inline std::string language_architecture(std::string_view language) {
    const auto separator = language.rfind(':');
    return separator == std::string_view::npos ? std::string(language) : std::string(language.substr(0, separator));
}

/// Returns true when a library language matches Ghidra's architecture-only comparison.
inline bool language_matches(std::string_view library, const std::optional<std::string>& target) {
    return !target.has_value() || language_architecture(library) == language_architecture(*target);
}

/// Returns true when compiler and source-language filters accept a library.
inline bool library_matches(const fid::LibraryRecord& library, const fid::ProgramInfo& program) {
    if (!language_matches(library.language_id, program.language_id))
        return false;
    if (program.ignore_database_filters)
        return true;
    if (program.compiler_spec && !library.compiler_specs.empty() &&
        !library.compiler_specs.contains(*program.compiler_spec))
        return false;
    if (!library.source_languages.empty() && program.source_languages.has_value()) {
        if (program.source_languages->empty())
            return false;
        bool intersects = false;
        for (const auto& source : *program.source_languages)
            intersects |= library.source_languages.contains(source);
        if (!intersects)
            return false;
    }
    return true;
}

/// Returns a mixed 32-bit operand value with Java integer overflow semantics.
inline std::uint32_t scalar_mix(std::int32_t value) noexcept {
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(static_cast<std::uint32_t>(value)) + 1234567U) *
                                      67999U);
}

/// Updates an FNV-1a state with a big-endian Java int representation.
inline void update_int(std::uint64_t& state, std::uint32_t value) noexcept {
    for (int shift = 24; shift >= 0; shift -= 8) {
        state ^= static_cast<fid::Byte>(value >> shift);
        state *= fnv_prime;
    }
}

/// Updates an FNV-1a state with raw bytes.
inline void update_bytes(std::uint64_t& state, std::span<const fid::Byte> bytes) noexcept {
    for (const auto value : bytes) {
        state ^= value;
        state *= fnv_prime;
    }
}

/// Returns the fixed set of processor NOP encodings skipped by X86InstructionSkipper.
inline bool is_x86_skipped(std::span<const fid::Byte> bytes) {
    constexpr std::array<std::array<fid::Byte, 9>, 17> patterns = {{
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
        if (bytes.size() == lengths[index] && std::equal(bytes.begin(), bytes.end(), patterns[index].begin()))
            return true;
    }
    return false;
}

/// Computes the superior relation key exactly as FidDBUtils.generateSuperiorFullHashSmash().
inline std::uint64_t superior_relation_key(std::int64_t superior_id, std::uint64_t inferior_full_hash) noexcept {
    return static_cast<std::uint64_t>(superior_id) * fnv_prime ^ inferior_full_hash;
}

/// Computes the inferior relation key exactly as FidDBUtils.generateInferiorFullHashSmash().
inline std::uint64_t inferior_relation_key(std::uint64_t superior_full_hash, std::int64_t inferior_id) noexcept {
    return static_cast<std::uint64_t>(inferior_id) * fnv_prime ^ superior_full_hash;
}

} // namespace fid::detail

#if defined(_MSC_VER)
#pragma optimize("", off)
#endif
