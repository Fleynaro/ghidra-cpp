export module function_id:buffer_file;

import std;
import :storage_helpers;
import :types;

#if defined(_MSC_VER)
#pragma optimize("gty", on)
#endif

// Ghidra references:
// Framework/DB/src/main/java/db/buffers/BufferFile.java,
// Framework/DB/src/main/java/db/buffers/LocalBufferFile.java, and
// Framework/DB/src/main/java/db/ChainedBuffer.java.

export namespace fid::detail {

/// Provides checked random access to the big-endian Ghidra LocalBufferFile format.
class BufferFile final {
public:
    /// Parses a raw `.fidbf` payload and validates its physical block geometry.
    explicit BufferFile(std::vector<fid::Byte> bytes) : bytes_(std::move(bytes)) {
        require(bytes_.size() >= 32, fid::ErrorCode::invalid_buffer_file, "buffer file header is truncated");
        const auto header = std::span<const fid::Byte>(bytes_);
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
    [[nodiscard]] std::span<const fid::Byte> buffer(std::int32_t id) const {
        require(id >= 0, fid::ErrorCode::invalid_database, "negative database buffer id");
        const auto block = static_cast<std::size_t>(id) + 1;
        require(block < bytes_.size() / block_size_, fid::ErrorCode::invalid_database,
                "database buffer id is outside the file");
        const auto offset = block * block_size_;
        require(bytes_[offset] == 0, fid::ErrorCode::invalid_database, "database references a free buffer");
        const auto stored_id = static_cast<std::int32_t>(read_be(bytes_, offset + 1, 4));
        require(stored_id == id, fid::ErrorCode::invalid_database, "database buffer id mismatch");
        return std::span<const fid::Byte>(bytes_).subspan(offset + 5, buffer_size_);
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
    [[nodiscard]] std::vector<fid::Byte> chained(std::int32_t first_id) const {
        const auto first = buffer(first_id);
        const auto raw_length = static_cast<std::int32_t>(read_be(first, 1, 4));
        const bool obfuscated = raw_length < 0;
        const auto length = static_cast<std::size_t>(raw_length & 0x7fffffff);
        require(length > 0, fid::ErrorCode::invalid_database, "empty chained DBBuffer");
        std::vector<fid::Byte> result(length);
        if (first[0] == 9) {
            require(length <= first.size() - 5, fid::ErrorCode::invalid_database,
                    "direct chained DBBuffer exceeds its data node");
            std::copy_n(first.begin() + 5, length, result.begin());
            if (obfuscated)
                xor_bytes(result, 0, length);
            return result;
        }
        require(first[0] == 8, fid::ErrorCode::invalid_database, "invalid chained DBBuffer node type");
        const auto data_space = buffer_size_ - 1;
        const auto ids_per_index = (buffer_size_ - 9) / 4;
        std::size_t copied = 0;
        std::int32_t index_id = first_id;
        bool first_index = true;
        std::unordered_set<std::int32_t> visited_indexes;
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
                if (obfuscated)
                    xor_bytes(result, copied, amount);
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
    static void xor_bytes(std::vector<fid::Byte>& data, std::size_t offset, std::size_t length) {
        for (std::size_t index = 0; index < length; ++index)
            data[offset + index] ^= chained_xor_mask[index % chained_xor_mask.size()];
    }

    std::vector<fid::Byte> bytes_;
    std::size_t block_size_{};
    std::size_t buffer_size_{};
};

} // namespace fid::detail

#if defined(_MSC_VER)
#pragma optimize("", off)
#endif
