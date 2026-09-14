export module analyzer_pdb_universal;

import analyzer;
import std;

// This module is a deliberately small, checked MSF/CodeView reader.  It ports
// the raw-file boundary of:
// Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/PdbParser.java,
// PdbNewDebugInfo.java, SymbolRecords.java, and the TPI record classes.  The
// applicator follows PdbUniversalAnalyzer.java and DefaultPdbApplicator.java.

export namespace ghidra::pdb::universal {

/// Reports a malformed or unsupported raw PDB input without exposing parser state.
struct PdbError {
    std::string message;
};

/// Carries the PDB identity stream used to match a CodeView reference.
struct PdbIdentity {
    std::uint32_t version{};
    std::uint32_t signature{};
    std::uint32_t age{};
    std::array<std::uint8_t, 16> guid{};

    /// Formats the CodeView GUID using the Windows textual byte ordering.
    [[nodiscard]] std::string guid_string() const;
};

/// Represents one supported CodeView symbol record after MSF translation.
struct PdbSymbol {
    std::string name;
    std::string kind;
    std::uint16_t section{};
    std::uint32_t offset{};
    std::uint32_t size{};
    std::uint32_t type_index{};
    bool function{};
    bool global{};
};

/// Represents one named TPI declaration and its fields or enumerators.
struct PdbType {
    std::string name;
    std::uint32_t index{};
    std::string kind;
    std::uint32_t size{};
    std::vector<std::pair<std::string, std::string>> fields;
};

/// Contains parsed identity, declarations, symbols, and supported signatures.
struct PdbFile {
    PdbIdentity identity;
    std::vector<PdbType> types;
    std::vector<PdbSymbol> symbols;
    std::map<std::uint32_t, std::pair<std::string, std::vector<std::string>>> procedure_signatures;
    std::map<std::uint32_t, std::string> type_names;

    /// Returns the stable source-level type spelling for a CodeView type index.
    [[nodiscard]] std::string type_name(std::uint32_t type_index) const;

    /// Returns the source-level return and parameter spellings for a procedure type.
    [[nodiscard]] std::optional<std::pair<std::string, std::vector<std::string>>>
    procedure_signature(std::uint32_t type_index) const;
};

/// Reads Microsoft PDB 7.0 MSF containers, TPI records, and CodeView symbols.
class PdbReader final {
public:
    /// Reads a complete raw PDB file and rejects truncated or unsupported streams.
    [[nodiscard]] static std::expected<PdbFile, PdbError> parse(const std::filesystem::path& path);
};

} // namespace ghidra::pdb::universal

export namespace ghidra::analyzer {

/// Applies supported raw PDB symbols, procedures, globals, and TPI declarations.
class PdbUniversalAnalyzer final : public Analyzer {
public:
    /// Returns the PDB Universal priority and whole-image trigger contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Parses the configured raw PDB and applies its supported records to context.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace ghidra::analyzer

namespace ghidra::pdb::universal {
namespace {

/// Reads little-endian integers from a bounded byte span.
class ByteReader final {
public:
    /// Creates a reader over one complete stream or record payload.
    explicit ByteReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    /// Returns the current cursor position.
    [[nodiscard]] std::size_t position() const noexcept {
        return position_;
    }

    /// Returns the remaining byte count.
    [[nodiscard]] std::size_t remaining() const noexcept {
        return bytes_.size() - position_;
    }

    /// Moves the cursor to an absolute bounded position.
    [[nodiscard]] bool seek(std::size_t position) noexcept {
        if (position > bytes_.size())
            return false;
        position_ = position;
        return true;
    }

    /// Reads one unsigned byte.
    [[nodiscard]] std::optional<std::uint8_t> u8() noexcept {
        if (remaining() < 1U)
            return std::nullopt;
        return bytes_[position_++];
    }

    /// Reads one little-endian unsigned 16-bit value.
    [[nodiscard]] std::optional<std::uint16_t> u16() noexcept {
        if (remaining() < 2U)
            return std::nullopt;
        const auto value =
            static_cast<std::uint16_t>(bytes_[position_]) | (static_cast<std::uint16_t>(bytes_[position_ + 1U]) << 8U);
        position_ += 2U;
        return value;
    }

    /// Reads one little-endian unsigned 32-bit value.
    [[nodiscard]] std::optional<std::uint32_t> u32() noexcept {
        if (remaining() < 4U)
            return std::nullopt;
        const auto value = static_cast<std::uint32_t>(bytes_[position_]) |
                           (static_cast<std::uint32_t>(bytes_[position_ + 1U]) << 8U) |
                           (static_cast<std::uint32_t>(bytes_[position_ + 2U]) << 16U) |
                           (static_cast<std::uint32_t>(bytes_[position_ + 3U]) << 24U);
        position_ += 4U;
        return value;
    }

    /// Reads one little-endian signed 32-bit value.
    [[nodiscard]] std::optional<std::int32_t> i32() noexcept {
        const auto value = u32();
        return value ? std::optional{std::bit_cast<std::int32_t>(*value)} : std::nullopt;
    }

    /// Reads a bounded byte span and advances the cursor.
    [[nodiscard]] std::optional<std::span<const std::uint8_t>> bytes(std::size_t count) noexcept {
        if (count > remaining())
            return std::nullopt;
        const auto result = bytes_.subspan(position_, count);
        position_ += count;
        return result;
    }

    /// Reads a nul-terminated UTF-8/ANSI string from the current cursor.
    [[nodiscard]] std::optional<std::string> string() {
        const auto begin = position_;
        while (position_ < bytes_.size() && bytes_[position_] != 0U)
            ++position_;
        if (position_ == bytes_.size())
            return std::nullopt;
        std::string result(reinterpret_cast<const char*>(bytes_.data() + begin), position_ - begin);
        ++position_;
        return result;
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t position_{};
};

/// Returns a parser error with a consistent source-specific prefix.
[[nodiscard]] PdbError error(std::string message) {
    return PdbError{"PDB Universal: " + std::move(message)};
}

/// Reads an entire file without silently accepting partial reads.
[[nodiscard]] std::expected<std::vector<std::uint8_t>, PdbError> read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
        return std::unexpected(error("cannot open '" + path.string() + "'"));
    const auto end = input.tellg();
    if (end < 0)
        return std::unexpected(error("cannot determine the size of '" + path.string() + "'"));
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty() &&
        !input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        return std::unexpected(error("cannot read '" + path.string() + "' completely"));
    return bytes;
}

/// Represents a validated MSF stream directory and its raw streams.
class MsfFile final {
public:
    /// Parses the PDB 7.0 superblock and directory into independent stream bytes.
    [[nodiscard]] static std::expected<MsfFile, PdbError> parse(std::vector<std::uint8_t> bytes) {
        static constexpr std::string_view signature = "Microsoft C/C++ MSF 7.00\r\n\x1a"
                                                      "DS\0\0\0";
        if (bytes.size() < 56U || !std::equal(signature.begin(), signature.end(), bytes.begin()))
            return std::unexpected(error("input is not a Microsoft PDB 7.0 MSF file"));
        ByteReader header(std::span<const std::uint8_t>(bytes).subspan(32));
        const auto block_size = header.u32();
        const auto free_map = header.u32();
        const auto block_count = header.u32();
        const auto directory_bytes = header.u32();
        const auto reserved = header.u32();
        const auto directory_map = header.u32();
        static_cast<void>(free_map);
        static_cast<void>(reserved);
        if (!block_size || !block_count || !directory_bytes || !directory_map || *block_size < 512U ||
            (*block_size & (*block_size - 1U)) != 0U || *block_count == 0U)
            return std::unexpected(error("MSF superblock has invalid sizing fields"));
        const std::uint64_t image_size = static_cast<std::uint64_t>(*block_size) * *block_count;
        if (image_size > bytes.size())
            return std::unexpected(error("MSF block count exceeds the file"));
        const std::size_t directory_page_count =
            (static_cast<std::size_t>(*directory_bytes) + *block_size - 1U) / *block_size;
        const std::size_t map_offset = static_cast<std::size_t>(*directory_map) * *block_size;
        if (directory_page_count > bytes.size() / 4U || map_offset > bytes.size() ||
            directory_page_count * 4U > bytes.size() - map_offset)
            return std::unexpected(error("MSF directory page map is truncated"));
        std::vector<std::uint32_t> directory_pages;
        ByteReader page_map(std::span<const std::uint8_t>(bytes).subspan(map_offset));
        for (std::size_t index = 0; index < directory_page_count; ++index) {
            const auto page = page_map.u32();
            if (!page || *page >= *block_count)
                return std::unexpected(error("MSF directory contains an invalid page number"));
            directory_pages.push_back(*page);
        }
        std::vector<std::uint8_t> directory;
        directory.reserve(*directory_bytes);
        for (const auto page : directory_pages) {
            const auto offset = static_cast<std::size_t>(page) * *block_size;
            const auto count = std::min<std::size_t>(*block_size, *directory_bytes - directory.size());
            if (offset > bytes.size() || count > bytes.size() - offset)
                return std::unexpected(error("MSF directory page is outside the file"));
            directory.insert(directory.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                             bytes.begin() + static_cast<std::ptrdiff_t>(offset + count));
            if (directory.size() == *directory_bytes)
                break;
        }
        ByteReader directory_reader(directory);
        const auto stream_count = directory_reader.u32();
        if (!stream_count || *stream_count > 0x100000U)
            return std::unexpected(error("MSF stream count is invalid"));
        std::vector<std::uint32_t> stream_sizes;
        stream_sizes.reserve(*stream_count);
        for (std::uint32_t index = 0; index < *stream_count; ++index) {
            const auto size = directory_reader.u32();
            if (!size)
                return std::unexpected(error("MSF stream size table is truncated"));
            stream_sizes.push_back(*size);
        }
        MsfFile result;
        result.streams_.resize(*stream_count);
        for (std::size_t stream_index = 0; stream_index < stream_sizes.size(); ++stream_index) {
            const auto size = stream_sizes[stream_index];
            if (size == 0xffffffffU)
                continue;
            const std::size_t page_count = (static_cast<std::size_t>(size) + *block_size - 1U) / *block_size;
            std::vector<std::uint32_t> pages;
            pages.reserve(page_count);
            for (std::size_t page = 0; page < page_count; ++page) {
                const auto page_number = directory_reader.u32();
                if (!page_number || *page_number >= *block_count)
                    return std::unexpected(error("MSF stream page table is truncated or invalid"));
                pages.push_back(*page_number);
            }
            auto& stream = result.streams_[stream_index];
            stream.reserve(size);
            for (const auto page : pages) {
                const auto offset = static_cast<std::size_t>(page) * *block_size;
                const auto count = std::min<std::size_t>(*block_size, static_cast<std::size_t>(size) - stream.size());
                if (offset > bytes.size() || count > bytes.size() - offset)
                    return std::unexpected(error("MSF stream page is outside the file"));
                stream.insert(stream.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                              bytes.begin() + static_cast<std::ptrdiff_t>(offset + count));
                if (stream.size() == size)
                    break;
            }
            if (stream.size() != size)
                return std::unexpected(error("MSF stream is shorter than its directory size"));
        }
        result.block_size_ = *block_size;
        return result;
    }

    /// Returns one parsed stream, or an empty stream for a nil index.
    [[nodiscard]] const std::vector<std::uint8_t>* stream(std::size_t index) const noexcept {
        return index < streams_.size() ? &streams_[index] : nullptr;
    }

private:
    std::uint32_t block_size_{};
    std::vector<std::vector<std::uint8_t>> streams_;
};

/// Describes one internal type record while retaining its original leaf bytes.
struct TypeNode {
    std::uint16_t leaf{};
    std::vector<std::uint8_t> payload;
};

/// Reads a CodeView numeric leaf and returns its value and new cursor.
[[nodiscard]] std::optional<std::pair<std::int64_t, std::size_t>> numeric_leaf(std::span<const std::uint8_t> bytes,
                                                                               std::size_t position) {
    if (position + 2U > bytes.size())
        return std::nullopt;
    ByteReader reader(bytes.subspan(position));
    const auto marker = reader.u16();
    if (!marker)
        return std::nullopt;
    if (*marker < 0x8000U)
        return std::pair{static_cast<std::int64_t>(*marker), position + 2U};
    const auto read_signed = [&](auto value) -> std::optional<std::pair<std::int64_t, std::size_t>> {
        return value ? std::optional{std::pair{static_cast<std::int64_t>(*value), position + reader.position()}}
                     : std::nullopt;
    };
    switch (*marker) {
        case 0x8000U: {
            const auto value = reader.u8();
            return value ? std::optional{std::pair{static_cast<std::int64_t>(std::bit_cast<std::int8_t>(*value)),
                                                   position + reader.position()}}
                         : std::nullopt;
        }
        case 0x8001U: {
            const auto value = reader.u16();
            return value ? std::optional{std::pair{static_cast<std::int64_t>(std::bit_cast<std::int16_t>(*value)),
                                                   position + reader.position()}}
                         : std::nullopt;
        }
        case 0x8002U: {
            const auto value = reader.u16();
            return value ? std::optional{std::pair{static_cast<std::int64_t>(*value), position + reader.position()}}
                         : std::nullopt;
        }
        case 0x8003U: {
            const auto value = reader.u32();
            return value ? std::optional{std::pair{static_cast<std::int64_t>(std::bit_cast<std::int32_t>(*value)),
                                                   position + reader.position()}}
                         : std::nullopt;
        }
        case 0x8004U: {
            const auto value = reader.u32();
            return value ? std::optional{std::pair{static_cast<std::int64_t>(*value), position + reader.position()}}
                         : std::nullopt;
        }
        case 0x8009U: {
            const auto low = reader.u32();
            const auto high = reader.u32();
            if (!low || !high)
                return std::nullopt;
            return std::pair{static_cast<std::int64_t>(static_cast<std::uint64_t>(*low) |
                                                       (static_cast<std::uint64_t>(*high) << 32U)),
                             position + reader.position()};
        }
        case 0x800aU: {
            const auto low = reader.u32();
            const auto high = reader.u32();
            if (!low || !high)
                return std::nullopt;
            return std::pair{static_cast<std::int64_t>(static_cast<std::uint64_t>(*low) |
                                                       (static_cast<std::uint64_t>(*high) << 32U)),
                             position + reader.position()};
        }
        default:
            return std::nullopt;
    }
}

/// Resolves the primitive CodeView type indices emitted by MSVC.
[[nodiscard]] std::string primitive_type_name(std::uint32_t index) {
    switch (index) {
        case 0x0003:
            return "void";
        case 0x0010:
        case 0x0070:
            return "char";
        case 0x0030:
            return "bool";
        case 0x0072:
            return "short";
        case 0x0073:
            return "unsigned short";
        case 0x0074:
            return "int";
        case 0x0075:
            return "unsigned int";
        case 0x0012:
        case 0x0013:
            return "__int64";
        case 0x0022:
        case 0x0023:
            return "unsigned __int64";
        case 0x0040:
            return "float";
        case 0x0041:
            return "double";
        default:
            return {};
    }
}

/// Provides safe cursor access to a type-node payload.
[[nodiscard]] std::span<const std::uint8_t> payload(const TypeNode& node) {
    return node.payload;
}

/// Parses a type stream and returns its indexed records.
[[nodiscard]] std::expected<std::map<std::uint32_t, TypeNode>, PdbError>
parse_types(const std::vector<std::uint8_t>& stream) {
    if (stream.size() < 20U)
        return std::unexpected(error("TPI stream is truncated"));
    ByteReader header(stream);
    const auto version = header.u32();
    const auto header_size = header.u32();
    const auto first_index = header.u32();
    const auto last_index = header.u32();
    const auto record_bytes = header.u32();
    if (!version || !header_size || !first_index || !last_index || !record_bytes || *header_size < 20U ||
        *header_size > stream.size() || *last_index < *first_index ||
        static_cast<std::uint64_t>(*header_size) + *record_bytes > stream.size())
        return std::unexpected(error("TPI header is invalid"));
    ByteReader records(std::span<const std::uint8_t>(stream).subspan(*header_size, *record_bytes));
    std::map<std::uint32_t, TypeNode> result;
    std::uint32_t index = *first_index;
    while (records.remaining() > 0U && index < *last_index) {
        const auto length = records.u16();
        if (!length || *length < 2U || *length - 2U > records.remaining())
            return std::unexpected(error("TPI record length is invalid"));
        const auto leaf = records.u16();
        if (!leaf)
            return std::unexpected(error("TPI record leaf is truncated"));
        const auto record = records.bytes(*length - 2U);
        if (!record)
            return std::unexpected(error("TPI record payload is truncated"));
        result.emplace(index++, TypeNode{*leaf, std::vector<std::uint8_t>(record->begin(), record->end())});
    }
    if (index != *last_index)
        return std::unexpected(error("TPI record count does not match the header"));
    return result;
}

/// Resolves a CodeView type index into a readable spelling with cycle protection.
[[nodiscard]] std::string describe_type(const std::map<std::uint32_t, TypeNode>& types, std::uint32_t index,
                                        std::set<std::uint32_t>& visiting);

/// Parses the members of an LF_FIELDLIST record.
[[nodiscard]] std::vector<std::pair<std::string, std::string>>
parse_field_list(const std::map<std::uint32_t, TypeNode>& types, std::uint32_t field_index,
                 std::set<std::uint32_t>& visiting) {
    const auto iterator = types.find(field_index);
    if (iterator == types.end() || iterator->second.leaf != 0x1203U)
        return {};
    const auto bytes = payload(iterator->second);
    std::vector<std::pair<std::string, std::string>> fields;
    std::size_t position = 0;
    while (position + 2U <= bytes.size()) {
        while (position < bytes.size() && bytes[position] >= 0xf0U)
            position += std::max<std::size_t>(1U, bytes[position] & 0x0fU);
        if (position + 2U > bytes.size())
            break;
        const auto leaf =
            static_cast<std::uint16_t>(bytes[position]) | (static_cast<std::uint16_t>(bytes[position + 1U]) << 8U);
        position += 2U;
        if (leaf == 0x150dU) { // LF_MEMBER
            if (position + 6U > bytes.size())
                break;
            const auto type_index = static_cast<std::uint32_t>(bytes[position + 2U]) |
                                    (static_cast<std::uint32_t>(bytes[position + 3U]) << 8U) |
                                    (static_cast<std::uint32_t>(bytes[position + 4U]) << 16U) |
                                    (static_cast<std::uint32_t>(bytes[position + 5U]) << 24U);
            position += 6U;
            const auto offset = numeric_leaf(bytes, position);
            if (!offset)
                break;
            position = offset->second;
            const auto name_begin = position;
            while (position < bytes.size() && bytes[position] != 0U)
                ++position;
            if (position >= bytes.size())
                break;
            fields.emplace_back(
                std::string(reinterpret_cast<const char*>(bytes.data() + name_begin), position - name_begin),
                describe_type(types, type_index, visiting) + " @" + std::to_string(offset->first));
            ++position;
        } else if (leaf == 0x1502U) { // LF_ENUMERATE
            if (position + 2U > bytes.size())
                break;
            position += 2U;
            const auto value = numeric_leaf(bytes, position);
            if (!value)
                break;
            position = value->second;
            const auto name_begin = position;
            while (position < bytes.size() && bytes[position] != 0U)
                ++position;
            if (position >= bytes.size())
                break;
            fields.emplace_back(
                std::string(reinterpret_cast<const char*>(bytes.data() + name_begin), position - name_begin),
                std::to_string(value->first));
            ++position;
        } else if (leaf == 0x150eU) { // LF_STMEMBER
            if (position + 6U > bytes.size())
                break;
            const auto type_index = static_cast<std::uint32_t>(bytes[position + 2U]) |
                                    (static_cast<std::uint32_t>(bytes[position + 3U]) << 8U) |
                                    (static_cast<std::uint32_t>(bytes[position + 4U]) << 16U) |
                                    (static_cast<std::uint32_t>(bytes[position + 5U]) << 24U);
            position += 6U;
            const auto name_begin = position;
            while (position < bytes.size() && bytes[position] != 0U)
                ++position;
            if (position >= bytes.size())
                break;
            fields.emplace_back(
                std::string(reinterpret_cast<const char*>(bytes.data() + name_begin), position - name_begin),
                describe_type(types, type_index, visiting) + " static");
            ++position;
        } else {
            // Unknown field-list entries are deliberately not guessed.  The
            // record remains parsed, while unsupported members are omitted.
            break;
        }
    }
    return fields;
}

/// Resolves a type index using the supported primitive, modifier, pointer,
/// array, enum, structure, and procedure CodeView records.
[[nodiscard]] std::string describe_type(const std::map<std::uint32_t, TypeNode>& types, std::uint32_t index,
                                        std::set<std::uint32_t>& visiting) {
    if (const auto primitive = primitive_type_name(index); !primitive.empty())
        return primitive;
    const auto iterator = types.find(index);
    if (iterator == types.end())
        return "type_0x" + [&] {
            std::ostringstream stream;
            stream << std::hex << index;
            return stream.str();
        }();
    if (!visiting.insert(index).second)
        return "recursive_type";
    const auto cleanup = [&] { visiting.erase(index); };
    const auto bytes = payload(iterator->second);
    std::string result;
    ByteReader reader(bytes);
    switch (iterator->second.leaf) {
        case 0x1001U: { // LF_MODIFIER
            const auto underlying = reader.u32();
            result = underlying ? describe_type(types, *underlying, visiting) : "modifier";
            break;
        }
        case 0x1002U: { // LF_POINTER
            const auto underlying = reader.u32();
            const auto attributes = reader.u32();
            static_cast<void>(attributes);
            result = underlying ? describe_type(types, *underlying, visiting) + " *" : "void *";
            break;
        }
        case 0x1503U: { // LF_ARRAY
            const auto element = reader.u32();
            const auto index_type = reader.u32();
            static_cast<void>(index_type);
            const auto size = numeric_leaf(bytes, reader.position());
            if (size)
                static_cast<void>(reader.seek(size->second));
            const auto name = reader.string();
            static_cast<void>(name);
            result = element
                         ? describe_type(types, *element, visiting) + "[" + std::to_string(size ? size->first : 0) + "]"
                         : "array";
            break;
        }
        case 0x1504U:
        case 0x1505U:
        case 0x1506U:
        case 0x1507U: {
            if (iterator->second.leaf == 0x1507U) {
                static_cast<void>(reader.seek(12U));
            } else {
                const auto size = numeric_leaf(bytes, 16U);
                if (!size || !reader.seek(size->second)) {
                    result = "anonymous";
                    break;
                }
            }
            const auto name = reader.string();
            result = name ? *name : "anonymous";
            break;
        }
        case 0x1008U: { // LF_PROCEDURE
            const auto return_type = reader.u32();
            const auto call_convention = reader.u8();
            const auto attributes = reader.u8();
            const auto count = reader.u16();
            const auto argument_list = reader.u32();
            static_cast<void>(call_convention);
            static_cast<void>(attributes);
            static_cast<void>(count);
            result = (return_type ? describe_type(types, *return_type, visiting) : "void") + " __cdecl";
            if (argument_list)
                result += " (" + describe_type(types, *argument_list, visiting) + ")";
            break;
        }
        case 0x1201U: { // LF_ARGLIST
            const auto count = reader.u32();
            std::vector<std::string> arguments;
            if (count && *count < 0x10000U) {
                for (std::uint32_t argument = 0; argument < *count; ++argument) {
                    const auto type_index = reader.u32();
                    if (!type_index)
                        break;
                    arguments.push_back(describe_type(types, *type_index, visiting));
                }
            }
            result = std::accumulate(
                arguments.begin(), arguments.end(), std::string{},
                [](std::string left, const std::string& right) { return left.empty() ? right : left + ", " + right; });
            break;
        }
        default:
            result = "";
            break;
    }
    cleanup();
    return result;
}

/// Extracts a null-terminated name from a symbol payload at a fixed offset.
[[nodiscard]] std::optional<std::string> payload_string(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset >= bytes.size())
        return std::nullopt;
    ByteReader reader(bytes.subspan(offset));
    return reader.string();
}

/// Converts one CodeView procedure calling-convention byte to a Ghidra spelling.
[[nodiscard]] std::string calling_convention(std::uint8_t value) {
    switch (value) {
        case 0x04:
            return "__fastcall";
        case 0x07:
        case 0x08:
            return "__stdcall";
        case 0x0b:
            return "__thiscall";
        default:
            return "__cdecl";
    }
}

/// Parses supported procedure/data/public/section CodeView records.
void parse_symbol_records(std::span<const std::uint8_t> bytes, std::vector<PdbSymbol>& output) {
    ByteReader reader(bytes);
    while (reader.remaining() >= 4U) {
        const auto record_start = reader.position();
        const auto length = reader.u16();
        const auto kind = reader.u16();
        if (!length || !kind || *length < 2U || *length - 2U > reader.remaining())
            break;
        const auto record = reader.bytes(*length - 2U);
        if (!record)
            break;
        const auto payload_bytes = *record;
        PdbSymbol symbol;
        if (*kind == 0x110fU || *kind == 0x1110U || *kind == 0x1146U || *kind == 0x1147U) {
            if (payload_bytes.size() < 35U)
                continue;
            auto read32 = [&](std::size_t offset) {
                return static_cast<std::uint32_t>(payload_bytes[offset]) |
                       (static_cast<std::uint32_t>(payload_bytes[offset + 1U]) << 8U) |
                       (static_cast<std::uint32_t>(payload_bytes[offset + 2U]) << 16U) |
                       (static_cast<std::uint32_t>(payload_bytes[offset + 3U]) << 24U);
            };
            symbol.kind = "procedure";
            symbol.function = true;
            symbol.type_index = read32(24U);
            symbol.offset = read32(28U);
            symbol.section =
                static_cast<std::uint16_t>(payload_bytes[32U]) | (static_cast<std::uint16_t>(payload_bytes[33U]) << 8U);
            symbol.size = read32(12U);
            if (const auto name = payload_string(payload_bytes, 35U))
                symbol.name = *name;
        } else if (*kind == 0x110cU || *kind == 0x110dU) { // S_LDATA32/S_GDATA32
            if (payload_bytes.size() < 10U)
                continue;
            symbol.kind = "data";
            symbol.type_index = static_cast<std::uint32_t>(payload_bytes[0]) |
                                (static_cast<std::uint32_t>(payload_bytes[1U]) << 8U) |
                                (static_cast<std::uint32_t>(payload_bytes[2U]) << 16U) |
                                (static_cast<std::uint32_t>(payload_bytes[3U]) << 24U);
            symbol.offset = static_cast<std::uint32_t>(payload_bytes[4U]) |
                            (static_cast<std::uint32_t>(payload_bytes[5U]) << 8U) |
                            (static_cast<std::uint32_t>(payload_bytes[6U]) << 16U) |
                            (static_cast<std::uint32_t>(payload_bytes[7U]) << 24U);
            symbol.section =
                static_cast<std::uint16_t>(payload_bytes[8U]) | (static_cast<std::uint16_t>(payload_bytes[9U]) << 8U);
            if (const auto name = payload_string(payload_bytes, 10U))
                symbol.name = *name;
        } else if (*kind == 0x110eU) { // S_PUB32
            if (payload_bytes.size() < 10U)
                continue;
            symbol.kind = "public";
            symbol.global = true;
            symbol.offset = static_cast<std::uint32_t>(payload_bytes[4U]) |
                            (static_cast<std::uint32_t>(payload_bytes[5U]) << 8U) |
                            (static_cast<std::uint32_t>(payload_bytes[6U]) << 16U) |
                            (static_cast<std::uint32_t>(payload_bytes[7U]) << 24U);
            symbol.section =
                static_cast<std::uint16_t>(payload_bytes[8U]) | (static_cast<std::uint16_t>(payload_bytes[9U]) << 8U);
            if (const auto name = payload_string(payload_bytes, 10U))
                symbol.name = *name;
        } else if (*kind == 0x1137U) { // S_COFFGROUP
            if (payload_bytes.size() < 14U)
                continue;
            symbol.kind = "section";
            symbol.size = static_cast<std::uint32_t>(payload_bytes[0]) |
                          (static_cast<std::uint32_t>(payload_bytes[1U]) << 8U) |
                          (static_cast<std::uint32_t>(payload_bytes[2U]) << 16U) |
                          (static_cast<std::uint32_t>(payload_bytes[3U]) << 24U);
            symbol.offset = static_cast<std::uint32_t>(payload_bytes[8U]) |
                            (static_cast<std::uint32_t>(payload_bytes[9U]) << 8U) |
                            (static_cast<std::uint32_t>(payload_bytes[10U]) << 16U) |
                            (static_cast<std::uint32_t>(payload_bytes[11U]) << 24U);
            symbol.section =
                static_cast<std::uint16_t>(payload_bytes[12U]) | (static_cast<std::uint16_t>(payload_bytes[13U]) << 8U);
            if (const auto name = payload_string(payload_bytes, 14U))
                symbol.name = *name;
        }
        if (!symbol.name.empty() && symbol.section != 0U) {
            static_cast<void>(record_start);
            output.push_back(std::move(symbol));
        }
    }
}

/// Reads the DBI module records and their local symbol streams.
void parse_module_symbols(const std::vector<std::uint8_t>& dbi, const MsfFile& msf, std::vector<PdbSymbol>& symbols) {
    if (dbi.size() < 64U)
        return;
    const auto read32 = [&](std::size_t offset) {
        return static_cast<std::uint32_t>(dbi[offset]) | (static_cast<std::uint32_t>(dbi[offset + 1U]) << 8U) |
               (static_cast<std::uint32_t>(dbi[offset + 2U]) << 16U) |
               (static_cast<std::uint32_t>(dbi[offset + 3U]) << 24U);
    };
    const auto module_bytes = read32(24U);
    const std::size_t module_begin = 64U;
    if (module_bytes > dbi.size() - module_begin)
        return;
    const auto module_end = module_begin + module_bytes;
    std::size_t position = module_begin;
    while (position + 64U <= module_end) {
        const std::size_t record_begin = position;
        position += 32U; // module pointer and SC600 contribution
        position += 2U;  // flags
        const auto stream =
            static_cast<std::uint16_t>(dbi[position]) | (static_cast<std::uint16_t>(dbi[position + 1U]) << 8U);
        position += 2U;
        const auto local_size = read32(position);
        position += 12U; // local symbols, old lines, C13 lines
        position += 2U;  // contributing files
        position = (position + 3U) & ~std::size_t{3U};
        position += 4U; // Reserved CodeView name-table offset fields.
        position += 8U; // name table indexes
        auto skip_string = [&] {
            while (position < module_end && dbi[position] != 0U)
                ++position;
            if (position < module_end)
                ++position;
        };
        skip_string();
        skip_string();
        position = (position + 3U) & ~std::size_t{3U};
        if (position <= record_begin || position > module_end)
            break;
        if (const auto stream_bytes = msf.stream(stream); stream_bytes && local_size <= stream_bytes->size()) {
            const std::size_t symbol_begin = 4U;
            if (symbol_begin <= stream_bytes->size() && local_size <= stream_bytes->size() - symbol_begin)
                parse_symbol_records(std::span<const std::uint8_t>(*stream_bytes).subspan(symbol_begin, local_size),
                                     symbols);
        }
    }
}

/// Builds a procedure signature from an LF_PROCEDURE and LF_ARGLIST pair.
[[nodiscard]] std::optional<std::pair<std::string, std::vector<std::string>>>
procedure_from_nodes(const std::map<std::uint32_t, TypeNode>& nodes, std::uint32_t index);

/// Parses PDB identity, TPI declarations, DBI module symbols, and global symbols.
[[nodiscard]] std::expected<PdbFile, PdbError> parse_pdb(const std::filesystem::path& path) {
    auto bytes = read_file(path);
    if (!bytes)
        return std::unexpected(bytes.error());
    auto msf = MsfFile::parse(std::move(*bytes));
    if (!msf)
        return std::unexpected(msf.error());
    const auto info = msf->stream(1U);
    const auto tpi = msf->stream(2U);
    const auto dbi = msf->stream(3U);
    if (!info || info->size() < 28U || !tpi || !dbi)
        return std::unexpected(error("PDB is missing the required PDB, TPI, or DBI stream"));
    ByteReader info_reader(*info);
    const auto version = info_reader.u32();
    const auto signature = info_reader.u32();
    const auto age = info_reader.u32();
    const auto guid_bytes = info_reader.bytes(16U);
    if (!version || !signature || !age || !guid_bytes)
        return std::unexpected(error("PDB identity stream is truncated"));
    PdbFile file;
    file.identity.version = *version;
    file.identity.signature = *signature;
    file.identity.age = *age;
    std::copy(guid_bytes->begin(), guid_bytes->end(), file.identity.guid.begin());
    auto type_nodes = parse_types(*tpi);
    if (!type_nodes)
        return std::unexpected(type_nodes.error());
    std::set<std::uint32_t> visiting;
    for (const auto& [index, node] : *type_nodes) {
        visiting.clear();
        file.type_names.emplace(index, describe_type(*type_nodes, index, visiting));
    }
    for (const auto& [index, node] : *type_nodes) {
        const auto bytes_node = payload(node);
        ByteReader reader(bytes_node);
        if (node.leaf == 0x1507U) { // LF_ENUM
            if (bytes_node.size() < 12U)
                continue;
            static_cast<void>(reader.seek(12U));
            const auto name = reader.string();
            if (!name)
                continue;
            const auto field_list = static_cast<std::uint32_t>(bytes_node[4U]) |
                                    (static_cast<std::uint32_t>(bytes_node[5U]) << 8U) |
                                    (static_cast<std::uint32_t>(bytes_node[6U]) << 16U) |
                                    (static_cast<std::uint32_t>(bytes_node[7U]) << 24U);
            file.types.push_back(
                PdbType{*name, index, "enum", 4U, parse_field_list(*type_nodes, field_list, visiting)});
        } else if (node.leaf == 0x1504U || node.leaf == 0x1505U || node.leaf == 0x1506U) {
            if (bytes_node.size() < 16U)
                continue;
            const auto field_list = static_cast<std::uint32_t>(bytes_node[4U]) |
                                    (static_cast<std::uint32_t>(bytes_node[5U]) << 8U) |
                                    (static_cast<std::uint32_t>(bytes_node[6U]) << 16U) |
                                    (static_cast<std::uint32_t>(bytes_node[7U]) << 24U);
            const auto size_value = numeric_leaf(bytes_node, 16U);
            if (size_value)
                static_cast<void>(reader.seek(size_value->second));
            const auto name = reader.string();
            if (!name || name->empty() || (size_value && size_value->first == 0 && field_list == 0U))
                continue;
            const std::string kind = node.leaf == 0x1506U ? "union" : "struct";
            file.types.push_back(PdbType{*name, index, kind,
                                         static_cast<std::uint32_t>(size_value ? size_value->first : 0),
                                         parse_field_list(*type_nodes, field_list, visiting)});
        }
        static_cast<void>(index);
    }
    std::vector<PdbSymbol> symbols;
    parse_module_symbols(*dbi, *msf, symbols);
    if (dbi->size() >= 22U) {
        const auto symbol_stream =
            static_cast<std::uint16_t>((*dbi)[20U]) | (static_cast<std::uint16_t>((*dbi)[21U]) << 8U);
        if (const auto stream = msf->stream(symbol_stream); stream && stream->size() >= 4U)
            parse_symbol_records(std::span<const std::uint8_t>(*stream).subspan(4U), symbols);
    }
    std::sort(symbols.begin(), symbols.end(), [](const PdbSymbol& left, const PdbSymbol& right) {
        return std::tie(left.section, left.offset, left.name) < std::tie(right.section, right.offset, right.name);
    });
    symbols.erase(std::unique(symbols.begin(), symbols.end(),
                              [](const PdbSymbol& left, const PdbSymbol& right) {
                                  return left.section == right.section && left.offset == right.offset &&
                                         left.name == right.name;
                              }),
                  symbols.end());
    for (const auto& symbol : symbols)
        if (symbol.function)
            if (const auto procedure = procedure_from_nodes(*type_nodes, symbol.type_index))
                file.procedure_signatures.emplace(symbol.type_index, *procedure);
    file.symbols = std::move(symbols);
    return file;
}

/// Returns a procedure's source spelling and argument types from TPI records.
[[nodiscard]] std::optional<std::pair<std::string, std::vector<std::string>>>
procedure_from_nodes(const std::map<std::uint32_t, TypeNode>& nodes, std::uint32_t index) {
    const auto iterator = nodes.find(index);
    if (iterator == nodes.end() || iterator->second.leaf != 0x1008U)
        return std::nullopt;
    const auto bytes = payload(iterator->second);
    if (bytes.size() < 12U)
        return std::nullopt;
    const auto return_index = static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1U]) << 8U) |
                              (static_cast<std::uint32_t>(bytes[2U]) << 16U) |
                              (static_cast<std::uint32_t>(bytes[3U]) << 24U);
    const auto callconv = bytes[4U];
    const auto argument_list = static_cast<std::uint32_t>(bytes[8U]) | (static_cast<std::uint32_t>(bytes[9U]) << 8U) |
                               (static_cast<std::uint32_t>(bytes[10U]) << 16U) |
                               (static_cast<std::uint32_t>(bytes[11U]) << 24U);
    std::set<std::uint32_t> visiting;
    const auto return_type = describe_type(nodes, return_index, visiting);
    std::vector<std::string> arguments;
    if (const auto args = nodes.find(argument_list); args != nodes.end() && args->second.leaf == 0x1201U) {
        ByteReader reader(payload(args->second));
        const auto count = reader.u32();
        if (count && *count < 0x10000U) {
            for (std::uint32_t argument = 0; argument < *count; ++argument) {
                const auto type_index = reader.u32();
                if (!type_index)
                    break;
                visiting.clear();
                arguments.push_back(describe_type(nodes, *type_index, visiting));
            }
        }
    }
    return std::pair{return_type + " " + calling_convention(callconv), arguments};
}

} // namespace

/// Formats the PDB GUID with the same field order used by CodeView RSDS records.
std::string PdbIdentity::guid_string() const {
    const auto u16 = [&](std::size_t offset) {
        return static_cast<std::uint16_t>(guid[offset]) | (static_cast<std::uint16_t>(guid[offset + 1U]) << 8U);
    };
    const auto u32 = [&] {
        return static_cast<std::uint32_t>(guid[0]) | (static_cast<std::uint32_t>(guid[1U]) << 8U) |
               (static_cast<std::uint32_t>(guid[2U]) << 16U) | (static_cast<std::uint32_t>(guid[3U]) << 24U);
    };
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(8) << u32() << '-' << std::setw(4) << u16(4U) << '-'
           << std::setw(4) << u16(6U) << '-';
    for (std::size_t index = 8U; index < 10U; ++index)
        stream << std::setw(2) << static_cast<unsigned>(guid[index]);
    stream << '-';
    for (std::size_t index = 10U; index < 16U; ++index)
        stream << std::setw(2) << static_cast<unsigned>(guid[index]);
    return stream.str();
}

/// Returns a primitive or named type spelling for a type index when available.
std::string PdbFile::type_name(std::uint32_t type_index) const {
    if (const auto primitive = primitive_type_name(type_index); !primitive.empty())
        return primitive;
    const auto iterator = type_names.find(type_index);
    return iterator == type_names.end() || iterator->second.empty() ? "undefined" : iterator->second;
}

/// Returns no procedure signature when the compact public result lacks TPI index data.
std::optional<std::pair<std::string, std::vector<std::string>>>
PdbFile::procedure_signature(std::uint32_t type_index) const {
    const auto iterator = procedure_signatures.find(type_index);
    return iterator == procedure_signatures.end() ? std::nullopt : std::optional{iterator->second};
}

/// Opens and parses one raw Microsoft PDB 7.0 file.
std::expected<PdbFile, PdbError> PdbReader::parse(const std::filesystem::path& path) {
    return parse_pdb(path);
}

} // namespace ghidra::pdb::universal

namespace ghidra::analyzer {
namespace {

/// Converts a PDB section/offset pair into a preferred-image address.
[[nodiscard]] std::optional<Address> pdb_address(const AnalysisContext& context, std::uint16_t section,
                                                 std::uint32_t offset) {
    if (section == 0U || section > context.image().sections().size())
        return std::nullopt;
    const auto& pe_section = context.image().sections()[section - 1U];
    const auto rva = static_cast<std::uint64_t>(pe_section.virtual_address) + offset;
    const auto address = context.image().rva_to_va(static_cast<pe::Rva>(rva));
    return address ? std::optional<Address>{*address} : std::nullopt;
}

/// Applies all records that the native AnalysisContext can represent faithfully.
void apply_pdb(AnalysisContext& context, const ghidra::pdb::universal::PdbFile& pdb, CancellationToken& cancellation) {
    for (const auto& type : pdb.types) {
        if (cancellation.is_cancelled())
            return;
        static_cast<void>(context.add_pdb_type(PdbTypeRecord{type.name, type.kind, type.size, type.fields}));
    }
    // Apply procedures before data symbols. PDB streams may contain a data
    // record at the same address as a procedure; the Java applicator resolves
    // that conflict in favor of the function before creating listing data.
    auto symbols = pdb.symbols;
    std::stable_sort(symbols.begin(), symbols.end(),
                     [](const auto& left, const auto& right) { return left.function && !right.function; });
    for (const auto& symbol : symbols) {
        if (cancellation.is_cancelled())
            return;
        const auto address = pdb_address(context, symbol.section, symbol.offset);
        if (!address)
            continue;
        const std::string type = symbol.kind == "procedure" ? "function" : symbol.kind;
        static_cast<void>(context.add_pdb_symbol(
            PdbSymbolRecord{*address, symbol.name, {}, type, symbol.size, symbol.function, false}));
        static_cast<void>(context.add_symbol(SymbolRecord{*address, {}, symbol.name, {}, type, false, true}));
        if (!symbol.function) {
            if (symbol.size != 0U)
                static_cast<void>(context.add_data(DataObject{*address, symbol.size, type}));
            continue;
        }
        if (!context.instructions().contains(*address))
            static_cast<void>(context.disassemble_flow(*address));
        if (!context.functions().contains(*address))
            static_cast<void>(context.create_function(*address, symbol.name));
        static_cast<void>(context.set_function_name(*address, symbol.name));
        if (const auto signature = pdb.procedure_signature(symbol.type_index)) {
            std::vector<FunctionParameter> parameters;
            parameters.reserve(signature->second.size());
            for (std::size_t index = 0; index < signature->second.size(); ++index)
                parameters.push_back(FunctionParameter{
                    "param_" + std::to_string(index + 1U), signature->second[index], {}, 0, 0, false});
            std::string convention = "__cdecl";
            const auto separator = signature->first.rfind(' ');
            if (separator != std::string::npos)
                convention = signature->first.substr(separator + 1U);
            const auto return_type =
                separator == std::string::npos ? signature->first : signature->first.substr(0, separator);
            static_cast<void>(
                context.set_function_signature(*address, convention, return_type, std::move(parameters), false, true));
        }
    }
    if (context.options().create_analysis_bookmarks)
        static_cast<void>(context.add_bookmark(
            Bookmark{context.image().optional_header().image_base, "PDB Universal",
                     "PDB Loaded: " + pdb.identity.guid_string() + " age " + std::to_string(pdb.identity.age)}));
}

} // namespace

/// Returns the platform-independent PDB Universal analyzer contract.
AnalyzerDescriptor PdbUniversalAnalyzer::descriptor() const {
    return {"PDB Universal", 898, {EventKind::memory_added}, {}};
}

/// Parses the configured raw PDB once per event-driven pass and applies records.
void PdbUniversalAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                   CancellationToken& cancellation) {
    if (!context.options().pdb_universal || context.options().pdb_path.empty() || cancellation.is_cancelled())
        return;
    const auto parsed = ghidra::pdb::universal::PdbReader::parse(context.options().pdb_path);
    if (!parsed)
        throw std::runtime_error(parsed.error().message);
    apply_pdb(context, *parsed, cancellation);
}

} // namespace ghidra::analyzer
