export module analyzer_embedded_media;

import analyzer;
import std;

/// Owns the Embedded Media analyzer declaration and implementation.
export namespace recode::analyzer {
class EmbeddedMediaAnalyzer final : public Analyzer {
public:
    /// Returns the EmbeddedMediaAnalyzer name and block-analysis priority.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Searches loaded initialized memory for validated media containers.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};
} // namespace recode::analyzer

namespace recode::analyzer {
namespace {

/// Describes one media datatype match before it is applied to the listing model.
struct MediaMatch {
    std::string type;
    std::string name;
    std::uint32_t size{};
};

/// Reads a little-endian unsigned integer from a media payload.
[[nodiscard]] std::optional<std::uint32_t> read_le32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4U) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

/// Reads a big-endian unsigned integer from a media payload.
[[nodiscard]] std::optional<std::uint32_t> read_be32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4U) {
        return std::nullopt;
    }
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
           (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) | static_cast<std::uint32_t>(bytes[offset + 3U]);
}

/// Calculates the PNG CRC used by PngResource when validating chunks.
[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
    std::uint32_t value = 0xffffffffU;
    for (const std::uint8_t byte : bytes) {
        value ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            value = (value >> 1U) ^ (0xedb88320U & static_cast<std::uint32_t>(-(value & 1U)));
        }
    }
    return ~value;
}

/// Validates a GIF resource and returns its dynamic data length.
[[nodiscard]] std::optional<std::uint32_t> gif_length(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 13U || std::memcmp(bytes.data(), "GIF", 3U) != 0 || bytes[3] != '8' ||
        (bytes[4] != '7' && bytes[4] != '9') || bytes[5] != 'a') {
        return std::nullopt;
    }
    std::size_t cursor = 13U;
    const std::uint8_t screen_flags = bytes[10];
    if ((screen_flags & 0x80U) != 0U) {
        const std::size_t entries = std::size_t{1} << ((screen_flags & 0x07U) + 1U);
        if (entries > (bytes.size() - cursor) / 3U) {
            return std::nullopt;
        }
        cursor += entries * 3U;
    }
    while (cursor < bytes.size()) {
        const std::uint8_t introducer = bytes[cursor++];
        if (introducer == 0x3bU) {
            return static_cast<std::uint32_t>(cursor);
        }
        if (introducer == 0x21U) {
            if (cursor >= bytes.size()) {
                return std::nullopt;
            }
            ++cursor;
            while (true) {
                if (cursor >= bytes.size()) {
                    return std::nullopt;
                }
                const std::size_t block_size = bytes[cursor++];
                if (block_size == 0U) {
                    break;
                }
                if (block_size > bytes.size() - cursor) {
                    return std::nullopt;
                }
                cursor += block_size;
            }
            continue;
        }
        if (introducer != 0x2cU || bytes.size() - cursor < 9U) {
            return std::nullopt;
        }
        const std::uint8_t image_flags = bytes[cursor + 8U];
        cursor += 9U;
        if ((image_flags & 0x80U) != 0U) {
            const std::size_t entries = std::size_t{1} << ((image_flags & 0x07U) + 1U);
            if (entries > (bytes.size() - cursor) / 3U) {
                return std::nullopt;
            }
            cursor += entries * 3U;
        }
        if (cursor >= bytes.size()) {
            return std::nullopt;
        }
        ++cursor;
        while (true) {
            if (cursor >= bytes.size()) {
                return std::nullopt;
            }
            const std::size_t block_size = bytes[cursor++];
            if (block_size == 0U) {
                break;
            }
            if (block_size > bytes.size() - cursor) {
                return std::nullopt;
            }
            cursor += block_size;
        }
    }
    return std::nullopt;
}

/// Validates PNG chunks, including their bounded lengths and CRC values.
[[nodiscard]] std::optional<std::uint32_t> png_length(std::span<const std::uint8_t> bytes) {
    constexpr std::array<std::uint8_t, 8> magic{0x89U, 0x50U, 0x4eU, 0x47U, 0x0dU, 0x0aU, 0x1aU, 0x0aU};
    if (bytes.size() < magic.size() || !std::equal(magic.begin(), magic.end(), bytes.begin())) {
        return std::nullopt;
    }
    std::size_t cursor = magic.size();
    while (cursor < bytes.size()) {
        const auto length = read_be32(bytes, cursor);
        if (!length || bytes.size() - cursor < 12U || *length > bytes.size() - cursor - 12U) {
            return std::nullopt;
        }
        const std::size_t chunk_end = cursor + 12U + *length;
        if (chunk_end > bytes.size()) {
            return std::nullopt;
        }
        const auto stored_crc = read_be32(bytes, cursor + 8U + *length);
        if (!stored_crc) {
            return std::nullopt;
        }
        const auto crc_input = bytes.subspan(cursor + 4U, 4U + *length);
        if (crc32(crc_input) != *stored_crc) {
            return std::nullopt;
        }
        const bool end = bytes[cursor + 4U] == 'I' && bytes[cursor + 5U] == 'E' && bytes[cursor + 6U] == 'N' &&
                         bytes[cursor + 7U] == 'D';
        cursor = chunk_end;
        if (end) {
            return static_cast<std::uint32_t>(cursor);
        }
    }
    return std::nullopt;
}

/// Validates the marker and bounded segment structure of a JPEG/JFIF stream.
[[nodiscard]] std::optional<std::uint32_t> jpeg_length(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 11U || bytes[0] != 0xffU || bytes[1] != 0xd8U || bytes[2] != 0xffU || bytes[6] != 'J' ||
        bytes[7] != 'F' || bytes[8] != 'I' || bytes[9] != 'F' || bytes[10] != 0U) {
        return std::nullopt;
    }
    bool scan_seen = false;
    for (std::size_t index = 2U; index + 1U < bytes.size(); ++index) {
        if (bytes[index] != 0xffU) {
            continue;
        }
        const auto marker = bytes[index + 1U];
        scan_seen = scan_seen || marker == 0xdaU;
        if (marker == 0xd9U && scan_seen) {
            return static_cast<std::uint32_t>(index + 2U);
        }
    }
    return std::nullopt;
}

/// Validates a RIFF/WAVE header and its declared little-endian total length.
[[nodiscard]] std::optional<std::uint32_t> wave_length(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 16U || std::memcmp(bytes.data(), "RIFF", 4U) != 0 ||
        std::memcmp(bytes.data() + 8U, "WAVEfmt", 7U) != 0) {
        return std::nullopt;
    }
    const auto payload = read_le32(bytes, 4U);
    if (!payload || *payload > bytes.size() - 8U) {
        return std::nullopt;
    }
    return *payload + 8U;
}

/// Validates a Standard MIDI file and all track chunk lengths needed by it.
[[nodiscard]] std::optional<std::uint32_t> midi_length(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 14U || std::memcmp(bytes.data(), "MThd", 4U) != 0) {
        return std::nullopt;
    }
    const auto header_length = read_be32(bytes, 4U);
    if (!header_length || *header_length != 6U || bytes.size() < 8U + *header_length) {
        return std::nullopt;
    }
    const std::size_t tracks = (static_cast<std::size_t>(bytes[10]) << 8U) | bytes[11];
    std::size_t cursor = 14U;
    std::size_t remaining = tracks;
    while (remaining != 0U) {
        if (bytes.size() - cursor < 8U || std::memcmp(bytes.data() + cursor, "MTrk", 4U) != 0) {
            return std::nullopt;
        }
        const auto length = read_be32(bytes, cursor + 4U);
        if (!length || *length > bytes.size() - cursor - 8U) {
            return std::nullopt;
        }
        cursor += 8U + *length;
        --remaining;
    }
    return static_cast<std::uint32_t>(cursor);
}

/// Validates an AU header whose offsets and lengths are big-endian.
[[nodiscard]] std::optional<std::uint32_t> au_length(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 12U || std::memcmp(bytes.data(), ".snd", 4U) != 0) {
        return std::nullopt;
    }
    const auto offset = read_be32(bytes, 4U);
    const auto length = read_be32(bytes, 8U);
    if (!offset || !length || *offset > std::numeric_limits<std::uint32_t>::max() - *length ||
        static_cast<std::uint64_t>(*offset) + *length > bytes.size()) {
        return std::nullopt;
    }
    return *offset + *length;
}

/// Validates an AIFF/AIFC FORM header and its big-endian data size.
[[nodiscard]] std::optional<std::uint32_t> aiff_length(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 12U || std::memcmp(bytes.data(), "FORM", 4U) != 0 ||
        (std::memcmp(bytes.data() + 8U, "AIFF", 4U) != 0 && std::memcmp(bytes.data() + 8U, "AIFC", 4U) != 0)) {
        return std::nullopt;
    }
    const auto length = read_be32(bytes, 4U);
    if (!length || *length == 0U || *length > bytes.size() - 8U) {
        return std::nullopt;
    }
    return *length + 8U;
}

/// Returns every media validation that succeeds at one mapped address.
[[nodiscard]] std::vector<MediaMatch> identify_media(std::span<const std::uint8_t> bytes) {
    std::vector<MediaMatch> matches;
    const auto add = [&](std::string type, std::string name, std::optional<std::uint32_t> length) {
        if (length && *length != 0U) {
            matches.push_back(MediaMatch{std::move(type), std::move(name), *length});
        }
    };
    if (bytes.size() >= 6U && std::memcmp(bytes.data(), "GIF87a", 6U) == 0) {
        add("GIF-Image", "GIF 87", gif_length(bytes));
    } else if (bytes.size() >= 6U && std::memcmp(bytes.data(), "GIF89a", 6U) == 0) {
        add("GIF-Image", "GIF 89", gif_length(bytes));
    }
    add("PNG-Image", "PNG", png_length(bytes));
    add("JPEG-Image", "JPEG", jpeg_length(bytes));
    add("WAVE-Sound", "WAVE", wave_length(bytes));
    add("MIDI-Score", "MIDI", midi_length(bytes));
    add("AU-Sound", "AU", au_length(bytes));
    add("AIFF-Sound", "AIFF", aiff_length(bytes));
    return matches;
}

/// Reports whether the native listing already owns an address as code or data.
[[nodiscard]] bool is_defined(const AnalysisContext& context, Address address) {
    for (const auto& [start, record] : context.instructions()) {
        if (address >= start && address < start + record.instruction.length) {
            return true;
        }
    }
    for (const auto& [start, data] : context.data()) {
        if (address >= start && address < start + data.size) {
            return true;
        }
    }
    return false;
}

} // namespace

/// Returns the Embedded Media analyzer priority and memory-event contract.
AnalyzerDescriptor EmbeddedMediaAnalyzer::descriptor() const {
    return {"Embedded Media", 200, {EventKind::memory_added}, {}};
}

/// Searches every loaded initialized region and applies the first valid media datatype.
void EmbeddedMediaAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                                    CancellationToken& cancellation) {
    // Ported from Ghidra:
    // Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/EmbeddedMediaAnalyzer.java
    // Relevant methods: added(), addByteSearchPattern(), and the media Dynamic datatype contracts in
    // Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/.
    if (!context.options().embedded_media) {
        return;
    }
    for (const auto& region : context.image().memory_regions()) {
        if (cancellation.is_cancelled()) {
            return;
        }
        if (!region.initialized || region.size == 0U) {
            continue;
        }
        for (std::uint64_t offset = 0; offset < region.size; ++offset) {
            if (cancellation.is_cancelled()) {
                return;
            }
            const Address address = region.start + offset;
            if (is_defined(context, address)) {
                continue;
            }
            const auto bytes = context.image().read_memory(address, region.size - offset);
            if (!bytes) {
                continue;
            }
            const auto matches = identify_media(*bytes);
            for (const auto& match : matches) {
                if (match.size > bytes->size() ||
                    context.image().find_memory_region(address, match.size) == std::nullopt) {
                    continue;
                }
                if (!context.add_embedded_media(EmbeddedMediaRecord{address, match.size, match.type, true})) {
                    continue;
                }
                if (context.options().create_analysis_bookmarks) {
                    static_cast<void>(context.add_bookmark(
                        Bookmark{address, "Embedded Media", "Found " + match.name + " Embedded Media"}));
                }
                break;
            }
        }
    }
}

} // namespace recode::analyzer
