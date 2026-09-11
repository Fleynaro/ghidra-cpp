#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

import pe_loader;

namespace {

/// Loads the checked-in executable fixture so integration tests exercise the real PE pipeline.
[[nodiscard]] pe::LoadedPeImage load_fixture() {
    auto image = pe::PeLoader::load_file(std::filesystem::path(PE_LOADER_TEST_FIXTURE));
    if (!image) {
        ADD_FAILURE() << image.error().message;
        return pe::LoadedPeImage{};
    }
    return std::move(*image);
}

/// Writes a little-endian WORD into a synthetic PE buffer used by parser edge-case tests.
void put_u16(std::vector<pe::Byte>& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<pe::Byte>(value & 0xffU);
    bytes[offset + 1] = static_cast<pe::Byte>(value >> 8U);
}

/// Writes a little-endian DWORD into a synthetic PE buffer used by parser edge-case tests.
void put_u32(std::vector<pe::Byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) {
        bytes[offset + index] = static_cast<pe::Byte>(value >> (index * 8U));
    }
}

/// Writes a little-endian QWORD into a synthetic PE buffer used by PE32+ tests.
void put_u64(std::vector<pe::Byte>& bytes, std::size_t offset, std::uint64_t value) {
    for (std::size_t index = 0; index < 8; ++index) {
        bytes[offset + index] = static_cast<pe::Byte>(value >> (index * 8U));
    }
}

/// Builds a minimal valid PE32 or PE32+ with one raw section and a virtual zero-filled tail.
[[nodiscard]] std::vector<pe::Byte> make_minimal_pe(bool pe32_plus, bool ordinal_import = false,
                                                    std::uint16_t machine_override = 0) {
    constexpr std::size_t nt_offset = 0x80;
    constexpr std::size_t file_header_offset = nt_offset + 4;
    constexpr std::size_t optional_offset = file_header_offset + 20;
    const std::size_t optional_size = pe32_plus ? 0xf0 : 0xe0;
    const std::size_t section_offset = optional_offset + optional_size;
    std::vector<pe::Byte> bytes(0x600, 0);
    put_u16(bytes, 0x00, 0x5a4d);
    put_u32(bytes, 0x3c, static_cast<std::uint32_t>(nt_offset));
    std::memcpy(bytes.data() + nt_offset, "PE\0\0", 4);
    put_u16(bytes, file_header_offset, machine_override != 0 ? machine_override : (pe32_plus ? 0x8664 : 0x14c));
    put_u16(bytes, file_header_offset + 2, 1);
    put_u16(bytes, file_header_offset + 16, static_cast<std::uint16_t>(optional_size));
    put_u16(bytes, file_header_offset + 18, 0x22);
    put_u16(bytes, optional_offset, pe32_plus ? 0x20b : 0x10b);
    bytes[optional_offset + 2] = 14;
    bytes[optional_offset + 3] = 34;
    put_u32(bytes, optional_offset + 4, 0x100);
    put_u32(bytes, optional_offset + 16, 0x1000);
    put_u32(bytes, optional_offset + 20, 0x1000);
    if (pe32_plus) {
        put_u64(bytes, optional_offset + 24, 0x140000000ULL);
    } else {
        put_u32(bytes, optional_offset + 24, 0x2000);
        put_u32(bytes, optional_offset + 28, 0x400000);
    }
    put_u32(bytes, optional_offset + 32, 0x1000);
    put_u32(bytes, optional_offset + 36, 0x200);
    put_u16(bytes, optional_offset + 40, 6);
    put_u16(bytes, optional_offset + 48, 6);
    put_u32(bytes, optional_offset + 56, 0x3000);
    put_u32(bytes, optional_offset + 60, 0x200);
    put_u16(bytes, optional_offset + 68, 3);
    put_u32(bytes, optional_offset + (pe32_plus ? 108 : 92), (ordinal_import || machine_override != 0) ? 4 : 0);
    std::memcpy(bytes.data() + section_offset, ".data", 5);
    put_u32(bytes, section_offset + 8, 0x300);
    put_u32(bytes, section_offset + 12, 0x1000);
    const bool extended_data = ordinal_import || machine_override != 0;
    put_u32(bytes, section_offset + 16, extended_data ? 0x400 : 0x100);
    put_u32(bytes, section_offset + 20, 0x200);
    put_u32(bytes, section_offset + 36, 0xc0000040);
    std::fill(bytes.begin() + 0x200, bytes.begin() + (extended_data ? 0x600 : 0x300), pe::Byte{0xa5});
    if (ordinal_import) {
        const std::size_t directory_offset = optional_offset + (pe32_plus ? 0x70 : 0x60) + 8;
        put_u32(bytes, directory_offset, 0x1100);
        put_u32(bytes, directory_offset + 4, 0x28);
        const std::size_t descriptor = 0x300;
        std::fill(bytes.begin() + descriptor, bytes.begin() + descriptor + 0x28, pe::Byte{0});
        put_u32(bytes, descriptor, pe32_plus ? 0x1180 : 0x1180);
        put_u32(bytes, descriptor + 12, 0x1160);
        put_u32(bytes, descriptor + 16, 0x11a0);
        std::memcpy(bytes.data() + 0x360, "ordinal.dll", 12);
        if (pe32_plus) {
            put_u64(bytes, 0x380, 0x8000000000000123ULL);
            put_u64(bytes, 0x388, 0);
            put_u64(bytes, 0x3a0, 0);
        } else {
            put_u32(bytes, 0x380, 0x80000123U);
            put_u32(bytes, 0x388, 0);
            put_u32(bytes, 0x3a0, 0);
        }
    }
    if (machine_override != 0) {
        const std::size_t directory_offset = optional_offset + 0x70 + 3 * 8;
        put_u32(bytes, directory_offset, 0x1100);
        put_u32(bytes, directory_offset + 4, 8);
        put_u32(bytes, 0x300, 0x1001);
        put_u32(bytes, 0x304, 0x00000007);
    }
    return bytes;
}

/// Writes one optional-header data-directory slot in a synthetic PE image.
void set_directory(std::vector<pe::Byte>& bytes, bool pe32_plus, std::uint32_t index, pe::Rva rva, std::uint32_t size,
                   std::uint32_t count = 16) {
    constexpr std::size_t optional_offset = 0x98;
    const auto directory_offset = optional_offset + (pe32_plus ? 0x70U : 0x60U) + index * 8U;
    put_u32(bytes, optional_offset + (pe32_plus ? 108U : 92U), count);
    put_u32(bytes, directory_offset, rva);
    put_u32(bytes, directory_offset + 4, size);
}

/// Returns the raw file location corresponding to an RVA in the synthetic section.
[[nodiscard]] std::size_t section_raw_offset(pe::Rva rva) {
    return 0x200U + static_cast<std::size_t>(rva - 0x1000U);
}

/// Creates a synthetic PE with enough section bytes for directory-level adversarial cases.
[[nodiscard]] std::vector<pe::Byte> make_extended_pe(bool pe32_plus) {
    auto bytes = make_minimal_pe(pe32_plus);
    const auto section_offset = pe32_plus ? 0x188U : 0x178U;
    put_u32(bytes, section_offset + 16, 0x400);
    std::fill(bytes.begin() + 0x200, bytes.begin() + 0x600, pe::Byte{0xa5});
    return bytes;
}

/// Builds a forwarded export whose forwarder terminator is either inside or outside the directory.
[[nodiscard]] std::vector<pe::Byte> make_export_pe(bool terminator_inside_directory) {
    auto bytes = make_extended_pe(true);
    const auto directory_size = terminator_inside_directory ? 0x80U : 0x70U;
    set_directory(bytes, true, 0, 0x1100, directory_size);
    const auto export_offset = section_raw_offset(0x1100);
    put_u32(bytes, export_offset + 12, 0x1140);
    put_u32(bytes, export_offset + 16, 1);
    put_u32(bytes, export_offset + 20, 1);
    put_u32(bytes, export_offset + 24, 1);
    put_u32(bytes, export_offset + 28, 0x1150);
    put_u32(bytes, export_offset + 32, 0x1154);
    put_u32(bytes, export_offset + 36, 0x1158);
    const auto function_rva = terminator_inside_directory ? 0x1160U : 0x116fU;
    put_u32(bytes, section_raw_offset(0x1150), function_rva);
    put_u32(bytes, section_raw_offset(0x1154), 0x1168);
    put_u16(bytes, section_raw_offset(0x1158), 0);
    std::memcpy(bytes.data() + section_raw_offset(0x1140), "forwarded.dll", 14);
    std::memcpy(bytes.data() + section_raw_offset(0x1168), "Forwarded", 10);
    std::memcpy(bytes.data() + section_raw_offset(function_rva), "KERNEL32.Sleep", 14);
    if (terminator_inside_directory) {
        bytes[section_raw_offset(function_rva) + 14] = 0;
    } else {
        bytes[section_raw_offset(function_rva) + 1] = 0;
    }
    return bytes;
}

/// Builds one delay-import descriptor in either RVA or absolute-VA pointer mode.
[[nodiscard]] std::vector<pe::Byte> make_delay_import_pe(bool uses_rva, bool malformed_thunk = false) {
    auto bytes = make_extended_pe(uses_rva);
    set_directory(bytes, uses_rva, 13, 0x1100, 0x40, 14);
    constexpr pe::Rva name_rva = 0x1200;
    constexpr pe::Rva iat_rva = 0x1220;
    constexpr pe::Rva int_rva = 0x1240;
    constexpr pe::Rva import_name_rva = 0x1260;
    const std::uint32_t image_base = 0x400000;
    const auto pointer_value = [uses_rva](pe::Rva rva) -> std::uint32_t { return uses_rva ? rva : 0x400000U + rva; };
    const auto descriptor = section_raw_offset(0x1100);
    std::fill(bytes.begin() + descriptor, bytes.begin() + descriptor + 64, pe::Byte{0});
    put_u32(bytes, descriptor, uses_rva ? 1U : 0U);
    put_u32(bytes, descriptor + 4, pointer_value(name_rva));
    put_u32(bytes, descriptor + 12, pointer_value(iat_rva));
    put_u32(bytes, descriptor + 16, pointer_value(int_rva));
    std::memcpy(bytes.data() + section_raw_offset(name_rva), "delay.dll", 10);
    if (uses_rva) {
        put_u64(bytes, section_raw_offset(int_rva), malformed_thunk ? 0x0000000100001260ULL : import_name_rva);
        put_u64(bytes, section_raw_offset(iat_rva), 0x1111222233334444ULL);
        put_u64(bytes, section_raw_offset(int_rva) + 8, 0);
    } else {
        put_u32(bytes, section_raw_offset(int_rva),
                malformed_thunk ? image_base + 0x100000U : image_base + import_name_rva);
        put_u32(bytes, section_raw_offset(iat_rva), 0x12345678U);
        put_u32(bytes, section_raw_offset(int_rva) + 4, 0);
    }
    put_u16(bytes, section_raw_offset(import_name_rva), 7);
    std::memcpy(bytes.data() + section_raw_offset(import_name_rva) + 2, "DelayEntry", 11);
    return bytes;
}

/// Builds a CLR directory with a caller-selected declared cb value.
[[nodiscard]] std::vector<pe::Byte> make_clr_pe(std::uint32_t cb) {
    auto bytes = make_extended_pe(true);
    set_directory(bytes, true, 14, 0x1100, 72, 15);
    const auto offset = section_raw_offset(0x1100);
    put_u32(bytes, offset, cb);
    put_u16(bytes, offset + 4, 2);
    put_u16(bytes, offset + 6, 5);
    put_u32(bytes, offset + 8, 0x1300);
    put_u32(bytes, offset + 12, 0x20);
    put_u32(bytes, offset + 16, 1);
    return bytes;
}

/// Builds a file-offset security directory containing two aligned WIN_CERTIFICATE records.
[[nodiscard]] std::vector<pe::Byte> make_certificate_pe(bool malformed) {
    auto bytes = make_extended_pe(true);
    bytes.resize(0x640, 0);
    set_directory(bytes, true, 4, 0x600, 0x20, 5);
    if (malformed) {
        put_u32(bytes, 0x600, 4);
    } else {
        put_u32(bytes, 0x600, 0x10);
        put_u16(bytes, 0x604, 0x0200);
        put_u16(bytes, 0x606, 2);
        std::fill(bytes.begin() + 0x608, bytes.begin() + 0x610, pe::Byte{0x5a});
        put_u32(bytes, 0x610, 0x10);
        put_u16(bytes, 0x614, 0x0100);
        put_u16(bytes, 0x616, 1);
        std::fill(bytes.begin() + 0x618, bytes.begin() + 0x620, pe::Byte{0xa5});
    }
    return bytes;
}

/// Builds a three-level resource tree and a file-backed payload outside the directory itself.
[[nodiscard]] std::vector<pe::Byte> make_resource_pe() {
    auto bytes = make_extended_pe(true);
    set_directory(bytes, true, 2, 0x1100, 0x100, 3);
    const auto base = section_raw_offset(0x1100);
    std::fill(bytes.begin() + base, bytes.begin() + base + 0x100, pe::Byte{0});
    put_u16(bytes, base + 14, 1);
    put_u32(bytes, base + 16, 10);
    put_u32(bytes, base + 20, 0x80000030U);
    put_u16(bytes, base + 0x30 + 14, 1);
    put_u32(bytes, base + 0x30 + 16, 101);
    put_u32(bytes, base + 0x30 + 20, 0x80000050U);
    put_u16(bytes, base + 0x50 + 14, 1);
    put_u32(bytes, base + 0x50 + 16, 1033);
    put_u32(bytes, base + 0x50 + 20, 0x70);
    put_u32(bytes, base + 0x70, 0x1200);
    put_u32(bytes, base + 0x74, 4);
    put_u32(bytes, base + 0x78, 1200);
    std::memcpy(bytes.data() + section_raw_offset(0x1200), "DATA", 4);
    return bytes;
}

/// Builds architecture and global-pointer directories with valid in-image RVAs.
[[nodiscard]] std::vector<pe::Byte> make_architecture_global_pointer_pe() {
    auto bytes = make_extended_pe(true);
    set_directory(bytes, true, 7, 0x1100, 0x20, 9);
    set_directory(bytes, true, 8, 0x1200, 0, 9);
    std::memcpy(bytes.data() + section_raw_offset(0x1100), "Architecture payload", 21);
    return bytes;
}

/// Builds a debug directory entry using the reserved type value that Ghidra names explicitly.
[[nodiscard]] std::vector<pe::Byte> make_reserved_debug_pe() {
    auto bytes = make_extended_pe(true);
    set_directory(bytes, true, 6, 0x1100, 28, 7);
    const auto entry = section_raw_offset(0x1100);
    std::fill(bytes.begin() + entry, bytes.begin() + entry + 28, pe::Byte{0});
    put_u32(bytes, entry + 12, 10);
    return bytes;
}

/// Builds a Rich header with selectable padding and record-area alignment faults.
[[nodiscard]] std::vector<pe::Byte> make_rich_edge_case_pe(bool invalid_padding, bool partial_record) {
    auto bytes = make_minimal_pe(true);
    constexpr std::size_t dans = 0x40;
    constexpr std::uint32_t mask = 0x163115c6U;
    const auto rich = partial_record ? 0x5aU : 0x58U;
    put_u32(bytes, dans, 0x536e6144U ^ mask);
    put_u32(bytes, dans + 4, invalid_padding ? 0U : mask);
    put_u32(bytes, dans + 8, mask);
    put_u32(bytes, dans + 12, mask);
    put_u32(bytes, dans + 16, 0x010200c1U ^ mask);
    put_u32(bytes, dans + 20, 3U ^ mask);
    put_u32(bytes, rich, 0x68636952U);
    put_u32(bytes, rich + 4, mask);
    return bytes;
}

/// Builds a PE whose declared header block extends beyond the available file bytes.
[[nodiscard]] std::vector<pe::Byte> make_truncated_headers_pe() {
    auto bytes = make_minimal_pe(true);
    put_u32(bytes, 0x98 + 60, 0x800);
    constexpr std::size_t section_offset = 0x188;
    put_u32(bytes, section_offset + 16, 0);
    put_u32(bytes, section_offset + 20, 0);
    return bytes;
}

/// Builds an x86 CHPE image whose exception directory uses the packed ARM record shape.
[[nodiscard]] std::vector<pe::Byte> make_chpe_pe() {
    auto bytes = make_extended_pe(true);
    set_directory(bytes, true, 3, 0x1200, 8, 11);
    set_directory(bytes, true, 10, 0x1100, 208, 11);
    const auto load_config = section_raw_offset(0x1100);
    put_u32(bytes, load_config, 208);
    put_u64(bytes, load_config + 200, 0x140001800ULL);
    const auto exception = section_raw_offset(0x1200);
    put_u32(bytes, exception, 0x1001);
    put_u32(bytes, exception + 4, 7);
    return bytes;
}

/// Verifies that the primary fixture can be decoded as a PE32+ executable.
TEST(PeLoaderFixture, LoadsHeaders) {
    const auto image = load_fixture();

    EXPECT_EQ(image.dos_header().e_magic, 0x5a4d);
    EXPECT_EQ(image.dos_header().e_lfanew, 0x110U);
    EXPECT_EQ(image.coff_header().machine, pe::Machine::amd64);
    EXPECT_EQ(image.coff_header().number_of_sections, 15);
    EXPECT_TRUE(image.optional_header().pe32_plus);
    EXPECT_EQ(image.optional_header().image_base, 0x140000000ULL);
    EXPECT_EQ(image.optional_header().address_of_entry_point, 0x2eebU);
}

/// Verifies the XOR-decoded Rich records and every non-empty directory RVA/raw-offset pair.
TEST(PeLoaderFixture, ParsesRichHeaderAndDirectories) {
    const auto image = load_fixture();

    ASSERT_TRUE(image.rich_header().has_value());
    EXPECT_EQ(image.rich_header()->offset, 0x80U);
    EXPECT_EQ(image.rich_header()->size, 0x80U);
    EXPECT_EQ(image.rich_header()->mask, 0x163115c6U);
    ASSERT_EQ(image.rich_header()->records.size(), 13U);
    EXPECT_EQ(image.rich_header()->records[0].comp_id, 0x1058179U);
    EXPECT_EQ(image.rich_header()->records[0].product_id, 0x105U);
    EXPECT_EQ(image.rich_header()->records[0].build, 33145U);
    EXPECT_EQ(image.rich_header()->records[0].object_count, 142U);
    EXPECT_EQ(image.rich_header()->records[11].comp_id, 0xff7cc1U);
    EXPECT_EQ(image.rich_header()->records[11].product_id, 0xffU);
    EXPECT_EQ(image.rich_header()->records[11].build, 31937U);
    EXPECT_EQ(image.rich_header()->records[12].comp_id, 0x1027cc1U);
    EXPECT_EQ(image.rich_header()->records[12].product_id, 0x102U);
    EXPECT_EQ(image.rich_header()->records[12].build, 31937U);

    const std::array<std::tuple<pe::DirectoryIndex, pe::Rva, std::uint32_t, pe::FileOffset>, 9> expected = {{
        {pe::DirectoryIndex::export_table, 0x9c8b0U, 0x18dU, 0x9beb0U},
        {pe::DirectoryIndex::import_table, 0xa8528U, 0x64U, 0xa2f28U},
        {pe::DirectoryIndex::resource_table, 0xb3000U, 0x4a9U, 0xa6e00U},
        {pe::DirectoryIndex::exception_table, 0xa2000U, 0x4ae8U, 0x9d600U},
        {pe::DirectoryIndex::base_relocation_table, 0xb4000U, 0x898U, 0xa7400U},
        {pe::DirectoryIndex::debug, 0x930c0U, 0x38U, 0x926c0U},
        {pe::DirectoryIndex::tls_table, 0x93a20U, 0x28U, 0x93020U},
        {pe::DirectoryIndex::load_config, 0x92f40U, 0x140U, 0x92540U},
        {pe::DirectoryIndex::import_address_table, 0xa8000U, 0x528U, 0xa2a00U},
    }};
    ASSERT_EQ(image.data_directories().size(), 16U);
    for (const auto& [index, rva, size, raw_offset] : expected) {
        const auto it = std::find_if(image.data_directories().begin(), image.data_directories().end(),
                                     [index](const pe::DataDirectory& directory) { return directory.index == index; });
        ASSERT_NE(it, image.data_directories().end());
        EXPECT_EQ(it->rva, rva);
        EXPECT_EQ(it->size, size);
        ASSERT_TRUE(it->file_offset.has_value());
        EXPECT_EQ(*it->file_offset, raw_offset);
    }
    EXPECT_EQ(image.data_directories()[4].rva, 0U);
    EXPECT_EQ(image.data_directories()[4].size, 0U);
    EXPECT_FALSE(image.data_directories()[4].file_offset.has_value());
    EXPECT_EQ(image.data_directories()[15].index, pe::DirectoryIndex::reserved);
    EXPECT_FALSE(image.data_directories()[12].address_is_file_offset);
}

/// Verifies concrete section layout, permissions, and raw-to-virtual extents from the reference report.
TEST(PeLoaderFixture, MapsSectionsAndAddresses) {
    const auto image = load_fixture();

    ASSERT_EQ(image.sections().size(), 15U);
    EXPECT_EQ(image.sections()[0].name, ".text");
    EXPECT_EQ(image.sections()[0].raw_offset, 0x600U);
    EXPECT_EQ(image.sections()[0].raw_size, 0x84000U);
    EXPECT_EQ(image.sections()[0].virtual_address, 0x1000U);
    EXPECT_EQ(image.sections()[0].virtual_size, 0x83ecdU);
    EXPECT_EQ(image.sections()[0].loaded_size, 0x84000U);
    EXPECT_EQ(image.sections()[14].name, ".reloc");
    EXPECT_EQ(image.sections()[14].virtual_address, 0xb4000U);
    EXPECT_EQ(image.sections()[14].loaded_size, 0x2000U);

    const auto file_offset = image.rva_to_file_offset(0x2eebU);
    ASSERT_TRUE(file_offset.has_value());
    EXPECT_EQ(*file_offset, 0x24ebU);
    const auto entry_va = image.rva_to_va(image.optional_header().address_of_entry_point);
    ASSERT_TRUE(entry_va.has_value());
    EXPECT_EQ(*entry_va, 0x140002eebULL);
    const auto round_trip = image.va_to_rva(*entry_va);
    ASSERT_TRUE(round_trip.has_value());
    EXPECT_EQ(*round_trip, image.optional_header().address_of_entry_point);
}

/// Verifies all fifteen fixture section names, raw ranges, virtual ranges, and permission bits.
TEST(PeLoaderFixture, MatchesCompleteSectionTable) {
    const auto image = load_fixture();
    struct ExpectedSection {
        std::string_view name;
        pe::FileOffset raw_offset;
        std::uint32_t raw_size;
        pe::Rva rva;
        std::uint32_t virtual_size;
        std::uint32_t characteristics;
    };
    const std::array<ExpectedSection, 15> expected = {{
        {".text", 0x600, 0x84000, 0x1000, 0x83ecd, 0x60000020},
        {".rdata", 0x84600, 0x17c00, 0x85000, 0x17a3d, 0x40000040},
        {".data", 0x9c200, 0x1400, 0x9d000, 0x45d9, 0xc0000040},
        {".pdata", 0x9d600, 0x5400, 0xa2000, 0x5388, 0x40000040},
        {".idata", 0xa2a00, 0x1600, 0xa8000, 0x1407, 0x40000040},
        {".neon", 0xa4000, 0x400, 0xaa000, 0x233, 0xc0000040},
        {".bss", 0xa4400, 0x1400, 0xab000, 0x1333, 0xc0000040},
        {".tls", 0xa5800, 0x400, 0xad000, 0x202, 0xc0000040},
        {".00cfg", 0xa5c00, 0x200, 0xae000, 0x175, 0x40000040},
        {"_RDATA", 0xa5e00, 0x400, 0xaf000, 0x29f, 0x40000040},
        {".fptable", 0xa6200, 0x400, 0xb0000, 0x233, 0xc0000040},
        {"_guard_c", 0xa6600, 0x400, 0xb1000, 0x234, 0xc0000040},
        {"_guard_d", 0xa6a00, 0x400, 0xb2000, 0x234, 0xc0000040},
        {".rsrc", 0xa6e00, 0x600, 0xb3000, 0x4a9, 0x40000040},
        {".reloc", 0xa7400, 0x1200, 0xb4000, 0x1125, 0x42000040},
    }};
    ASSERT_EQ(image.sections().size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& actual = image.sections()[index];
        EXPECT_EQ(actual.name, expected[index].name);
        EXPECT_EQ(actual.raw_offset, expected[index].raw_offset);
        EXPECT_EQ(actual.raw_size, expected[index].raw_size);
        EXPECT_EQ(actual.file_backed_size, expected[index].raw_size);
        EXPECT_EQ(actual.virtual_address, expected[index].rva);
        EXPECT_EQ(actual.virtual_size, expected[index].virtual_size);
        EXPECT_EQ(actual.characteristics, expected[index].characteristics);
        const auto expected_loaded_size = actual.virtual_size != 0
                                              ? (static_cast<std::uint32_t>(actual.virtual_size + 0xfffU) & ~0xfffU)
                                              : (static_cast<std::uint32_t>(actual.raw_size + 0xfffU) & ~0xfffU);
        EXPECT_EQ(actual.loaded_size, expected_loaded_size);
    }
}

/// Verifies exports, all import descriptors, and the complete import count from the fixture.
TEST(PeLoaderFixture, ParsesImportsAndExports) {
    const auto image = load_fixture();

    ASSERT_TRUE(image.exports().has_value());
    EXPECT_EQ(image.exports()->name, "neon_heist.exe");
    EXPECT_EQ(image.exports()->ordinal_base, 1U);
    ASSERT_EQ(image.exported_symbols().size(), 2U);
    EXPECT_EQ(image.exported_symbols()[0].name, "NeonExportedChecksum");
    EXPECT_EQ(image.exported_symbols()[0].address_rva, 0x2af9U);
    EXPECT_EQ(image.exported_symbols()[1].name, "NeonExportedTransform");

    ASSERT_EQ(image.imports().size(), 4U);
    EXPECT_EQ(image.imports()[0].dll_name, "USER32.dll");
    EXPECT_EQ(image.imports()[1].dll_name, "ADVAPI32.dll");
    EXPECT_EQ(image.imports()[2].dll_name, "bcrypt.dll");
    EXPECT_EQ(image.imports()[3].dll_name, "KERNEL32.dll");
    EXPECT_EQ(image.imports()[0].symbols.size(), 1U);
    EXPECT_EQ(image.imports()[1].symbols.size(), 2U);
    EXPECT_EQ(image.imports()[2].symbols.size(), 1U);
    EXPECT_EQ(image.imports()[3].symbols.size(), 98U);
    EXPECT_EQ(image.imports()[3].symbols.front().name, "CreateFileW");
    EXPECT_EQ(image.imports()[3].symbols.front().hint, 230U);
    EXPECT_EQ(image.imports()[3].symbols.front().iat_slot_rva, 0xa8068U);
    EXPECT_EQ(image.imports()[3].symbols.back().name, "HeapReAlloc");
    ASSERT_EQ(image.iat_entries().size(), 165U);
    EXPECT_EQ(image.iat_entries().front().slot_rva, 0xa8000U);
    EXPECT_EQ(image.iat_entries().front().slot_va, 0x1400a8000ULL);
}

/// Verifies the complete ordered KERNEL32 import set instead of relying on the compact report samples.
TEST(PeLoaderFixture, MatchesCompleteKernel32ImportSet) {
    const auto image = load_fixture();
    ASSERT_EQ(image.imports().size(), 4U);
    const auto& symbols = image.imports()[3].symbols;
    const std::array<std::string_view, 98> expected = {{
        "CreateFileW",
        "ExitProcess",
        "ReadConsoleW",
        "ReadFile",
        "GetTickCount",
        "LoadResource",
        "LockResource",
        "SizeofResource",
        "FindResourceA",
        "SetConsoleTitleA",
        "CloseHandle",
        "QueryPerformanceCounter",
        "GetCurrentProcessId",
        "GetCurrentThreadId",
        "GetSystemTimeAsFileTime",
        "InitializeSListHead",
        "RtlCaptureContext",
        "RtlLookupFunctionEntry",
        "RtlVirtualUnwind",
        "IsDebuggerPresent",
        "UnhandledExceptionFilter",
        "SetUnhandledExceptionFilter",
        "GetStartupInfoW",
        "IsProcessorFeaturePresent",
        "GetModuleHandleW",
        "GetCurrentProcess",
        "TerminateProcess",
        "RtlPcToFileHeader",
        "RaiseException",
        "RtlUnwindEx",
        "InterlockedPushEntrySList",
        "InterlockedFlushSList",
        "GetLastError",
        "SetLastError",
        "EncodePointer",
        "EnterCriticalSection",
        "LeaveCriticalSection",
        "DeleteCriticalSection",
        "InitializeCriticalSectionAndSpinCount",
        "TlsAlloc",
        "TlsGetValue",
        "TlsSetValue",
        "TlsFree",
        "FreeLibrary",
        "GetProcAddress",
        "LoadLibraryExW",
        "RtlUnwind",
        "GetStdHandle",
        "WriteFile",
        "GetModuleFileNameW",
        "WriteConsoleW",
        "GetModuleHandleExW",
        "GetCommandLineA",
        "GetCommandLineW",
        "GetCurrentThread",
        "HeapAlloc",
        "HeapFree",
        "GetTempPathW",
        "FlsAlloc",
        "FlsGetValue",
        "FlsSetValue",
        "FlsFree",
        "IsThreadAFiber",
        "InitializeCriticalSectionEx",
        "VirtualProtect",
        "GetDateFormatW",
        "GetTimeFormatW",
        "CompareStringW",
        "LCMapStringW",
        "GetLocaleInfoW",
        "IsValidLocale",
        "GetUserDefaultLCID",
        "EnumSystemLocalesW",
        "GetFileType",
        "OutputDebugStringW",
        "FindClose",
        "FindFirstFileExW",
        "FindNextFileW",
        "IsValidCodePage",
        "GetACP",
        "GetOEMCP",
        "GetCPInfo",
        "MultiByteToWideChar",
        "WideCharToMultiByte",
        "GetEnvironmentStringsW",
        "FreeEnvironmentStringsW",
        "SetEnvironmentVariableW",
        "SetStdHandle",
        "GetStringTypeW",
        "GetProcessHeap",
        "SetConsoleCtrlHandler",
        "FlushFileBuffers",
        "GetConsoleOutputCP",
        "GetConsoleMode",
        "GetFileSizeEx",
        "SetFilePointerEx",
        "HeapSize",
        "HeapReAlloc",
    }};
    ASSERT_EQ(symbols.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(symbols[index].name, expected[index]);
        EXPECT_FALSE(symbols[index].imported_by_ordinal);
        EXPECT_EQ(symbols[index].iat_slot_rva, 0xa8068U + static_cast<std::uint32_t>(index * 8));
        ASSERT_TRUE(symbols[index].import_by_name_rva.has_value());
        EXPECT_EQ(symbols[index].thunk_value, *symbols[index].import_by_name_rva);
    }
}

/// Verifies relocation, exception, debug, TLS, load-config, and resource metadata counts and values.
TEST(PeLoaderFixture, ParsesMetadataDirectories) {
    const auto image = load_fixture();

    ASSERT_EQ(image.relocations().size(), 16U);
    std::size_t relocation_count = 0;
    for (const auto& block : image.relocations()) {
        relocation_count += block.entries.size();
    }
    EXPECT_EQ(relocation_count, 1036U);
    ASSERT_EQ(image.exception_functions().size(), 1598U);
    ASSERT_EQ(image.debug_entries().size(), 2U);
    ASSERT_TRUE(image.debug_entries()[0].code_view.has_value());
    EXPECT_EQ(image.debug_entries()[0].code_view->signature, "RSDS");
    EXPECT_EQ(image.debug_entries()[0].code_view->age, 5U);
    ASSERT_TRUE(image.tls().has_value());
    EXPECT_EQ(image.tls()->start_address_of_raw_data, 0x1400ad000ULL);
    EXPECT_EQ(image.tls()->end_address_of_raw_data, 0x1400ad101ULL);
    EXPECT_EQ(image.tls()->address_of_index, 0x14009f678ULL);
    EXPECT_EQ(image.tls()->address_of_callbacks, 0x1400858b8ULL);
    EXPECT_EQ(image.tls()->size_of_zero_fill, 0U);
    EXPECT_EQ(image.tls()->characteristics, 0x100000U);
    ASSERT_EQ(image.tls()->callback_addresses.size(), 1U);
    EXPECT_EQ(image.tls()->callback_addresses[0], 0x140007520ULL);
    ASSERT_TRUE(image.load_config().has_value());
    EXPECT_EQ(image.load_config()->size, 0x140U);
    EXPECT_EQ(image.load_config()->critical_section_default_timeout, 0U);
    EXPECT_EQ(image.load_config()->security_cookie, 0x14009d018ULL);
    EXPECT_EQ(image.load_config()->se_handler_table, 0U);
    EXPECT_EQ(image.load_config()->se_handler_count, 0U);
    EXPECT_EQ(image.load_config()->guard_flags, 0x100U);
    EXPECT_EQ(image.load_config()->guard_cf_check_function_pointer, 0x1400ae000ULL);
    EXPECT_EQ(image.load_config()->guard_cf_dispatch_function_pointer, 0x1400ae020ULL);
    EXPECT_EQ(image.load_config()->guard_cf_function_table, 0U);
    EXPECT_EQ(image.load_config()->guard_cf_function_count, 0U);
    ASSERT_TRUE(image.resources().has_value());
    ASSERT_EQ(image.resources()->leaves.size(), 2U);
    EXPECT_EQ(image.resources()->leaves[0].type.id, 10U);
    EXPECT_EQ(image.resources()->leaves[1].type.id, 24U);
    EXPECT_TRUE(image.certificates().empty());
    EXPECT_TRUE(image.bound_imports().empty());
    EXPECT_TRUE(image.delay_imports().empty());
    EXPECT_FALSE(image.clr_header().has_value());
}

/// Verifies relocation block geometry and representative exception/debug records from the fixture.
TEST(PeLoaderFixture, MatchesRelocationsExceptionsAndDebug) {
    const auto image = load_fixture();
    const std::array<std::tuple<pe::Rva, std::uint32_t, std::size_t>, 16> relocation_blocks = {{
        {0x85000, 0x2c, 18},
        {0x86000, 0x140, 156},
        {0x87000, 0x54, 38},
        {0x88000, 0x5c, 42},
        {0x89000, 0xb8, 88},
        {0x8a000, 0x3c, 26},
        {0x8b000, 0xe0, 108},
        {0x8c000, 0x158, 168},
        {0x8d000, 0x80, 60},
        {0x8e000, 0x1a8, 208},
        {0x8f000, 0x34, 22},
        {0x92000, 0x48, 32},
        {0x93000, 0x1c, 10},
        {0x9d000, 0x5c, 42},
        {0x9e000, 0x20, 12},
        {0xae000, 0x14, 6},
    }};
    ASSERT_EQ(image.relocations().size(), relocation_blocks.size());
    for (std::size_t index = 0; index < relocation_blocks.size(); ++index) {
        const auto& [page, block_size, entry_count] = relocation_blocks[index];
        EXPECT_EQ(image.relocations()[index].page_rva, page);
        EXPECT_EQ(image.relocations()[index].block_size, block_size);
        EXPECT_EQ(image.relocations()[index].entries.size(), entry_count);
    }
    ASSERT_EQ(image.exception_functions().size(), 1598U);
    const auto exception = std::find_if(image.exception_functions().begin(), image.exception_functions().end(),
                                        [](const pe::RuntimeFunction& function) {
                                            return function.begin_rva == 0x3b658 && function.end_rva == 0x3b781;
                                        });
    ASSERT_NE(exception, image.exception_functions().end());
    EXPECT_EQ(exception->unwind_info_rva, 0x96f44U);
    EXPECT_EQ(exception->unwind_info_file_offset, 0x96544U);
    ASSERT_EQ(image.debug_entries().size(), 2U);
    EXPECT_EQ(image.debug_entries()[0].type, pe::DebugType::code_view);
    EXPECT_EQ(image.debug_entries()[0].size_of_data, 0x70U);
    EXPECT_EQ(image.debug_entries()[0].address_of_raw_data, 0x946d8U);
    EXPECT_EQ(image.debug_entries()[0].pointer_to_raw_data, 0x93cd8U);
    ASSERT_TRUE(image.debug_entries()[0].code_view.has_value());
    EXPECT_EQ(image.debug_entries()[0].code_view->path,
              "C:\\Users\\Fleynaro\\Desktop\\GTA-5-Android\\GTA-5-Android\\tests\\2_neon_heist\\neon_heist.pdb");
    EXPECT_EQ(image.debug_entries()[1].type_raw, 12U);
}

/// Verifies memory reads return file bytes, zero-filled virtual tails, and controlled gap errors.
TEST(PeLoaderFixture, ReadsMappedMemory) {
    const auto image = load_fixture();

    const auto header = image.read_memory(0x140000000ULL, 2);
    ASSERT_TRUE(header.has_value());
    EXPECT_EQ((*header)[0], 0x4d);
    EXPECT_EQ((*header)[1], 0x5a);
    const auto text = image.read_memory(0x140001000ULL, 4);
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ((*text)[0], image.file_bytes()[0x600]);
    const auto gap = image.read_byte(0x140000600ULL);
    EXPECT_FALSE(gap.has_value());
    const auto invalid = image.read_byte(0x1400b6000ULL);
    EXPECT_FALSE(invalid.has_value());
}

/// Verifies the loaded-region names, sizes, and section permissions used by a future Sleigh memory view.
TEST(PeLoaderFixture, MatchesLoadedMemoryRegions) {
    const auto image = load_fixture();
    const std::array<std::tuple<std::string_view, pe::Va, std::uint64_t, bool, bool, bool>, 16> expected = {{
        {"Headers", 0x140000000ULL, 0x600, true, false, false},
        {".text", 0x140001000ULL, 0x84000, true, false, true},
        {".rdata", 0x140085000ULL, 0x18000, true, false, false},
        {".data", 0x14009d000ULL, 0x5000, true, true, false},
        {".pdata", 0x1400a2000ULL, 0x6000, true, false, false},
        {".idata", 0x1400a8000ULL, 0x2000, true, false, false},
        {".neon", 0x1400aa000ULL, 0x1000, true, true, false},
        {".bss", 0x1400ab000ULL, 0x2000, true, true, false},
        {".tls", 0x1400ad000ULL, 0x1000, true, true, false},
        {".00cfg", 0x1400ae000ULL, 0x1000, true, false, false},
        {"_RDATA", 0x1400af000ULL, 0x1000, true, false, false},
        {".fptable", 0x1400b0000ULL, 0x1000, true, true, false},
        {"_guard_c", 0x1400b1000ULL, 0x1000, true, true, false},
        {"_guard_d", 0x1400b2000ULL, 0x1000, true, true, false},
        {".rsrc", 0x1400b3000ULL, 0x1000, true, false, false},
        {".reloc", 0x1400b4000ULL, 0x2000, true, false, false},
    }};
    ASSERT_EQ(image.memory_regions().size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& actual = image.memory_regions()[index];
        const auto& [name, start, size, readable, writable, executable] = expected[index];
        EXPECT_EQ(actual.name, name);
        EXPECT_EQ(actual.start, start);
        EXPECT_EQ(actual.size, size);
        EXPECT_EQ(actual.readable, readable);
        EXPECT_EQ(actual.writable, writable);
        EXPECT_EQ(actual.executable, executable);
        EXPECT_TRUE(actual.initialized);
        EXPECT_FALSE(actual.headers && index != 0);
    }
}

/// Verifies resource-tree paths and reads the exact UTF-16 RCDATA payload from mapped memory.
TEST(PeLoaderFixture, ReadsResourceLeaves) {
    const auto image = load_fixture();
    ASSERT_TRUE(image.resources().has_value());
    ASSERT_EQ(image.resources()->leaves.size(), 2U);
    const auto& rc_data = image.resources()->leaves[0];
    EXPECT_EQ(rc_data.type.id, 10U);
    EXPECT_EQ(rc_data.name.id, 101U);
    EXPECT_EQ(rc_data.language.id, 1033U);
    EXPECT_EQ(rc_data.data_rva, 0xb3338U);
    EXPECT_EQ(rc_data.size, 0x20U);
    ASSERT_TRUE(rc_data.file_offset.has_value());
    EXPECT_EQ(*rc_data.file_offset, 0xa7138U);
    const auto payload = image.read_memory(0x1400b3338ULL, rc_data.size);
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ((*payload)[0], 0x4e);
    EXPECT_EQ((*payload)[1], 0x00);
    EXPECT_EQ((*payload)[0x1e], 0x00);
    EXPECT_EQ((*payload)[0x1f], 0x00);
    const auto& manifest = image.resources()->leaves[1];
    EXPECT_EQ(manifest.type.id, 24U);
    EXPECT_EQ(manifest.name.id, 1U);
    EXPECT_EQ(manifest.language.id, 1033U);
    EXPECT_EQ(manifest.data_rva, 0xb31c0U);
    EXPECT_EQ(manifest.size, 0x173U);
}

/// Verifies malformed signatures and truncation produce errors instead of unchecked reads.
TEST(PeLoaderMalformed, RejectsInvalidHeaders) {
    const std::vector<pe::Byte> empty;
    const auto empty_result = pe::PeLoader::load(empty);
    ASSERT_FALSE(empty_result.has_value());
    EXPECT_EQ(empty_result.error().code, pe::ParseErrorCode::empty_input);

    std::vector<pe::Byte> bad_dos(64, 0);
    const auto bad_dos_result = pe::PeLoader::load(bad_dos);
    ASSERT_FALSE(bad_dos_result.has_value());
    EXPECT_EQ(bad_dos_result.error().code, pe::ParseErrorCode::invalid_dos_signature);
}

/// Verifies PE32 field widths, BaseOfData, virtual-only section tails, and unmapped gaps.
TEST(PeLoaderSynthetic, ParsesPe32AndVirtualTail) {
    const auto bytes = make_minimal_pe(false);
    auto image = pe::PeLoader::load(bytes);
    ASSERT_TRUE(image.has_value()) << (image ? "" : image.error().message);

    EXPECT_EQ(image->coff_header().machine, pe::Machine::i386);
    EXPECT_FALSE(image->optional_header().pe32_plus);
    ASSERT_TRUE(image->optional_header().base_of_data.has_value());
    EXPECT_EQ(*image->optional_header().base_of_data, 0x2000U);
    const auto tail = image->read_memory(0x401200ULL, 0x100);
    ASSERT_TRUE(tail.has_value());
    EXPECT_TRUE(std::all_of(tail->begin(), tail->end(), [](pe::Byte value) { return value == 0; }));
    const auto tail_file_offset = image->rva_to_file_offset(0x1200);
    ASSERT_FALSE(tail_file_offset.has_value());
    EXPECT_EQ(tail_file_offset.error().code, pe::AddressErrorCode::not_file_backed);
    const auto gap = image->read_byte(0x401000ULL - 1);
    EXPECT_FALSE(gap.has_value());
}

/// Verifies Ghidra-compatible truncation of a declared raw section tail with zero-filled mapping.
TEST(PeLoaderSynthetic, TruncatesRawSectionAtEof) {
    auto bytes = make_minimal_pe(true);
    bytes.resize(0x280);
    auto image = pe::PeLoader::load(bytes);
    ASSERT_TRUE(image.has_value()) << (image ? "" : image.error().message);
    ASSERT_EQ(image->sections().size(), 1U);
    EXPECT_EQ(image->sections()[0].raw_size, 0x100U);
    EXPECT_EQ(image->sections()[0].file_backed_size, 0x80U);
    const auto retained = image->read_memory(0x140001000ULL, 0x80);
    ASSERT_TRUE(retained.has_value());
    EXPECT_TRUE(std::all_of(retained->begin(), retained->end(), [](pe::Byte value) { return value == 0xa5; }));
    const auto zero_tail = image->read_memory(0x140001080ULL, 0x80);
    ASSERT_TRUE(zero_tail.has_value());
    EXPECT_TRUE(std::all_of(zero_tail->begin(), zero_tail->end(), [](pe::Byte value) { return value == 0; }));
}

/// Verifies ARM exception rows mask the Thumb bit and preserve packed unwind data without unsafe VA conversion.
TEST(PeLoaderSynthetic, ParsesArmPackedException) {
    const auto bytes = make_minimal_pe(true, false, 0x1c0);
    auto image = pe::PeLoader::load(bytes);
    ASSERT_TRUE(image.has_value()) << (image ? "" : image.error().message);
    ASSERT_EQ(image->exception_functions().size(), 1U);
    EXPECT_EQ(image->exception_functions()[0].begin_rva, 0x1000U);
    ASSERT_TRUE(image->exception_functions()[0].packed_unwind_data.has_value());
    EXPECT_EQ(*image->exception_functions()[0].packed_unwind_data, 7U);
    EXPECT_EQ(image->exception_functions()[0].unwind_info_rva, 0U);
}

/// Verifies that a PE32+ ordinal thunk is preserved without fabricating an import name.
TEST(PeLoaderSynthetic, ParsesOrdinalImport) {
    const auto bytes = make_minimal_pe(true, true);
    pe::LoadOptions no_directory_parse;
    no_directory_parse.parse_directories = false;
    auto mapped = pe::PeLoader::load(bytes, no_directory_parse);
    ASSERT_TRUE(mapped.has_value()) << (mapped ? "" : mapped.error().message);
    const auto mapped_name = mapped->read_memory(0x140001160ULL, 12);
    ASSERT_TRUE(mapped_name.has_value());
    EXPECT_EQ(std::string(mapped_name->begin(), mapped_name->end()).substr(0, 11), "ordinal.dll");
    auto image = pe::PeLoader::load(bytes);
    ASSERT_TRUE(image.has_value()) << (image ? "" : image.error().message);

    ASSERT_EQ(image->imports().size(), 1U);
    ASSERT_EQ(image->imports()[0].symbols.size(), 1U);
    const auto& symbol = image->imports()[0].symbols[0];
    EXPECT_EQ(image->imports()[0].dll_name, "ordinal.dll");
    EXPECT_TRUE(symbol.imported_by_ordinal);
    ASSERT_TRUE(symbol.ordinal.has_value());
    EXPECT_EQ(*symbol.ordinal, 0x123U);
    EXPECT_TRUE(symbol.name.empty());
    EXPECT_EQ(symbol.iat_slot_rva, 0x11a0U);
}

/// Verifies that the mapped virtual section extent follows SectionHeader.alignUp semantics rather than raw sizes.
TEST(PeLoaderSynthetic, AlignsVirtualSectionExtentToSectionAlignment) {
    auto bytes = make_minimal_pe(true);
    constexpr std::size_t section_offset = 0x188;
    put_u32(bytes, section_offset + 8, 0x210);
    put_u32(bytes, section_offset + 16, 0x100);
    auto image = pe::PeLoader::load(bytes);
    ASSERT_TRUE(image.has_value()) << (image ? "" : image.error().message);

    ASSERT_EQ(image->sections().size(), 1U);
    EXPECT_EQ(image->sections()[0].virtual_size, 0x210U);
    EXPECT_EQ(image->sections()[0].loaded_size, 0x1000U);
    EXPECT_EQ(image->sections()[0].file_backed_size, 0x200U);
    const auto virtual_tail = image->read_memory(0x140001200ULL, 0x100);
    ASSERT_TRUE(virtual_tail.has_value());
    EXPECT_TRUE(std::all_of(virtual_tail->begin(), virtual_tail->end(), [](pe::Byte value) { return value == 0; }));
}

/// Verifies that a PE32+ non-ordinal thunk with high bits cannot be truncated into a valid 32-bit RVA.
TEST(PeLoaderSynthetic, RejectsTruncatedPe32PlusImportRva) {
    auto bytes = make_minimal_pe(true, true);
    put_u64(bytes, 0x380, 0x0000000100001180ULL);
    const auto result = pe::PeLoader::load(bytes);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, pe::ParseErrorCode::invalid_import);
}

/// Verifies that CLR parsing honors the declared cb bound and reports tolerated failures explicitly.
TEST(PeLoaderSynthetic, ReportsPartialResultForTruncatedClrHeader) {
    const auto bytes = make_clr_pe(4);
    const auto strict_result = pe::PeLoader::load(bytes);
    ASSERT_FALSE(strict_result.has_value());
    EXPECT_EQ(strict_result.error().code, pe::ParseErrorCode::invalid_directory);

    pe::LoadOptions options;
    options.strict = false;
    const auto non_strict_result = pe::PeLoader::load(bytes, options);
    ASSERT_TRUE(non_strict_result.has_value()) << (non_strict_result ? "" : non_strict_result.error().message);
    EXPECT_TRUE(non_strict_result->is_partial());
    EXPECT_EQ(non_strict_result->parse_status(), pe::ParseStatus::partial);
    ASSERT_FALSE(non_strict_result->parse_diagnostics().empty());
    EXPECT_EQ(non_strict_result->parse_diagnostics().front().code, pe::ParseErrorCode::invalid_directory);
    EXPECT_FALSE(non_strict_result->clr_header().has_value());
}

/// Verifies that section extents near the 32-bit RVA boundary are rejected without wrapped arithmetic.
TEST(PeLoaderSynthetic, RejectsSectionExtentAtRvaLimit) {
    auto bytes = make_minimal_pe(true);
    constexpr std::size_t section_offset = 0x188;
    put_u32(bytes, 0x98 + 56, 0xffffffffU);
    put_u32(bytes, section_offset + 8, 0x10);
    put_u32(bytes, section_offset + 12, 0xfffffff0U);
    put_u32(bytes, section_offset + 16, 0);
    pe::LoadOptions options;
    options.maximum_image_size = std::numeric_limits<std::uint32_t>::max();
    const auto result = pe::PeLoader::load(bytes, options);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, pe::ParseErrorCode::invalid_section_range);
}

/// Verifies forwarded exports and ensures their strings are bounded by the export directory.
TEST(PeLoaderSynthetic, ParsesAndBoundsForwardedExports) {
    const auto valid = pe::PeLoader::load(make_export_pe(true));
    ASSERT_TRUE(valid.has_value()) << (valid ? "" : valid.error().message);
    ASSERT_EQ(valid->exported_symbols().size(), 1U);
    EXPECT_TRUE(valid->exported_symbols()[0].forwarded);
    ASSERT_TRUE(valid->exported_symbols()[0].forwarder.has_value());
    EXPECT_EQ(*valid->exported_symbols()[0].forwarder, "KERNEL32.Sleep");

    const auto invalid = pe::PeLoader::load(make_export_pe(false));
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error().code, pe::ParseErrorCode::invalid_export);
}

/// Verifies both RVA-mode and VA-mode delay imports and rejects an invalid VA thunk.
TEST(PeLoaderSynthetic, ParsesDelayImportsInBothAddressModes) {
    const auto rva_result = pe::PeLoader::load(make_delay_import_pe(true));
    ASSERT_TRUE(rva_result.has_value()) << (rva_result ? "" : rva_result.error().message);
    ASSERT_EQ(rva_result->delay_imports().size(), 1U);
    EXPECT_TRUE(rva_result->delay_imports()[0].uses_rva);
    ASSERT_EQ(rva_result->delay_imports()[0].symbols.size(), 1U);
    EXPECT_EQ(rva_result->delay_imports()[0].symbols[0].name, "DelayEntry");

    const auto va_result = pe::PeLoader::load(make_delay_import_pe(false));
    ASSERT_TRUE(va_result.has_value()) << (va_result ? "" : va_result.error().message);
    ASSERT_EQ(va_result->delay_imports().size(), 1U);
    EXPECT_FALSE(va_result->delay_imports()[0].uses_rva);
    ASSERT_EQ(va_result->delay_imports()[0].symbols.size(), 1U);
    EXPECT_EQ(va_result->delay_imports()[0].symbols[0].name, "DelayEntry");

    const auto malformed = pe::PeLoader::load(make_delay_import_pe(false, true));
    ASSERT_FALSE(malformed.has_value());
    EXPECT_EQ(malformed.error().code, pe::ParseErrorCode::invalid_import);
}

/// Verifies valid aligned certificates and rejection of a certificate length smaller than WIN_CERTIFICATE.
TEST(PeLoaderSynthetic, ParsesAndValidatesSecurityCertificates) {
    const auto valid = pe::PeLoader::load(make_certificate_pe(false));
    ASSERT_TRUE(valid.has_value()) << (valid ? "" : valid.error().message);
    ASSERT_EQ(valid->certificates().size(), 2U);
    EXPECT_EQ(valid->certificates()[0].length, 0x10U);
    EXPECT_EQ(valid->certificates()[0].revision, 0x0200U);
    EXPECT_EQ(valid->certificates()[0].certificate_bytes.size(), 8U);
    EXPECT_EQ(valid->certificates()[1].file_offset, 0x610U);

    const auto invalid = pe::PeLoader::load(make_certificate_pe(true));
    ASSERT_FALSE(invalid.has_value());
    EXPECT_EQ(invalid.error().code, pe::ParseErrorCode::invalid_certificate);
}

/// Verifies ARM, ARMNT, Thumb, ARM64, ARM64EC, and ARM64X exception row decoding.
TEST(PeLoaderSynthetic, ParsesArmArchitectureVariants) {
    const std::array<std::uint16_t, 6> machines = {0x1c0, 0x1c4, 0x1c2, 0xaa64, 0xa641, 0xa64e};
    for (const auto machine : machines) {
        const auto result = pe::PeLoader::load(make_minimal_pe(true, false, machine));
        ASSERT_TRUE(result.has_value()) << "machine 0x" << std::hex << machine << ": "
                                        << (result ? "" : result.error().message);
        ASSERT_EQ(result->exception_functions().size(), 1U);
        EXPECT_EQ(result->exception_functions()[0].begin_rva, 0x1000U);
        EXPECT_EQ(result->exception_functions()[0].end_rva, 0x1000U);
        ASSERT_TRUE(result->exception_functions()[0].packed_unwind_data.has_value());
        EXPECT_EQ(*result->exception_functions()[0].packed_unwind_data, 7U);
    }
}

/// Verifies that x86 CHPE metadata selects the packed ARM exception-record layout before decoding exceptions.
TEST(PeLoaderSynthetic, ParsesChpePackedExceptions) {
    const auto result = pe::PeLoader::load(make_chpe_pe());
    ASSERT_TRUE(result.has_value()) << (result ? "" : result.error().message);
    ASSERT_TRUE(result->load_config().has_value());
    EXPECT_NE(result->load_config()->chpe_metadata_pointer, 0U);
    ASSERT_EQ(result->exception_functions().size(), 1U);
    EXPECT_EQ(result->exception_functions()[0].begin_rva, 0x1000U);
    ASSERT_TRUE(result->exception_functions()[0].packed_unwind_data.has_value());
    EXPECT_EQ(*result->exception_functions()[0].packed_unwind_data, 7U);
}

/// Verifies that load-config fields after CodeIntegrity use the architecture-specific offsets from Ghidra.
TEST(PeLoaderSynthetic, ParsesPostCodeIntegrityLoadConfigFields) {
    for (const bool pe32_plus : {false, true}) {
        auto bytes = make_extended_pe(pe32_plus);
        const std::uint32_t directory_size = pe32_plus ? 240U : 152U;
        set_directory(bytes, pe32_plus, 10, 0x1100, directory_size, 11);
        const auto config = section_raw_offset(0x1100);
        put_u32(bytes, config, directory_size);
        if (pe32_plus) {
            put_u64(bytes, config + 160, 0x140001234ULL);
            put_u64(bytes, config + 168, 0x140005678ULL);
            put_u64(bytes, config + 176, 0x140009abcULL);
            put_u64(bytes, config + 184, 0x14000def0ULL);
            put_u64(bytes, config + 192, 0x140001111ULL);
            put_u64(bytes, config + 200, 0x140001222ULL);
            put_u64(bytes, config + 208, 0x140001333ULL);
            put_u64(bytes, config + 216, 0x140001444ULL);
            put_u32(bytes, config + 224, 0x55667788U);
            put_u16(bytes, config + 228, 9);
            put_u64(bytes, config + 232, 0x140001555ULL);
        } else {
            put_u32(bytes, config + 104, 0x1234U);
            put_u32(bytes, config + 108, 0x5678U);
            put_u32(bytes, config + 112, 0x9abcU);
            put_u32(bytes, config + 116, 0xdef0U);
            put_u32(bytes, config + 120, 0x1111U);
            put_u32(bytes, config + 124, 0x2222U);
            put_u32(bytes, config + 128, 0x3333U);
            put_u32(bytes, config + 132, 0x4444U);
            put_u32(bytes, config + 136, 0x55667788U);
            put_u16(bytes, config + 140, 9);
            put_u32(bytes, config + 144, 0x5555U);
        }
        const auto result = pe::PeLoader::load(bytes);
        ASSERT_TRUE(result.has_value()) << (result ? "" : result.error().message);
        ASSERT_TRUE(result->load_config().has_value());
        const auto& load_config = *result->load_config();
        EXPECT_EQ(load_config.guard_address_taken_iat_entry_table, pe32_plus ? 0x140001234ULL : 0x1234ULL);
        EXPECT_EQ(load_config.guard_address_taken_iat_entry_count, pe32_plus ? 0x140005678ULL : 0x5678ULL);
        EXPECT_EQ(load_config.guard_long_jump_target_table, pe32_plus ? 0x140009abcULL : 0x9abcULL);
        EXPECT_EQ(load_config.guard_long_jump_target_count, pe32_plus ? 0x14000def0ULL : 0xdef0ULL);
        EXPECT_EQ(load_config.dynamic_value_reloc_table, pe32_plus ? 0x140001111ULL : 0x1111ULL);
        EXPECT_EQ(load_config.chpe_metadata_pointer, pe32_plus ? 0x140001222ULL : 0x2222ULL);
        EXPECT_EQ(load_config.guard_rf_failure_routine, pe32_plus ? 0x140001333ULL : 0x3333ULL);
        EXPECT_EQ(load_config.guard_rf_failure_routine_function_pointer, pe32_plus ? 0x140001444ULL : 0x4444ULL);
        EXPECT_EQ(load_config.dynamic_value_reloc_table_offset, 0x55667788U);
        EXPECT_EQ(load_config.dynamic_value_reloc_table_section, 9U);
        EXPECT_EQ(load_config.guard_rf_verify_stack_pointer_function_pointer, pe32_plus ? 0x140001555ULL : 0x5555ULL);
    }
}

/// Verifies architecture/global-pointer directory parsing and resource payload extraction APIs.
TEST(PeLoaderSynthetic, ExposesArchitectureGlobalPointerAndResourcePayloads) {
    const auto directories = pe::PeLoader::load(make_architecture_global_pointer_pe());
    ASSERT_TRUE(directories.has_value()) << (directories ? "" : directories.error().message);
    ASSERT_TRUE(directories->architecture_directory().has_value());
    EXPECT_EQ(directories->architecture_directory()->copyright, "Architecture payload");
    ASSERT_TRUE(directories->global_pointer_directory().has_value());
    EXPECT_EQ(directories->global_pointer_directory()->rva, 0x1200U);
    EXPECT_EQ(directories->global_pointer_directory()->global_pointer_va, 0x140001200ULL);

    const auto resources = pe::PeLoader::load(make_resource_pe());
    ASSERT_TRUE(resources.has_value()) << (resources ? "" : resources.error().message);
    ASSERT_TRUE(resources->resources().has_value());
    ASSERT_EQ(resources->resources()->leaves.size(), 1U);
    const auto payload = resources->read_resource_payload(0U);
    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(std::string(payload->begin(), payload->end()), "DATA");
}

/// Verifies that reserved debug type 10 remains a named typed value instead of an invalid enum cast.
TEST(PeLoaderSynthetic, PreservesReservedDebugType) {
    const auto result = pe::PeLoader::load(make_reserved_debug_pe());
    ASSERT_TRUE(result.has_value()) << (result ? "" : result.error().message);
    ASSERT_EQ(result->debug_entries().size(), 1U);
    EXPECT_EQ(result->debug_entries()[0].type_raw, 10U);
    EXPECT_EQ(result->debug_entries()[0].type, pe::DebugType::reserved10);
}

/// Verifies RichTable padding and record alignment validation, plus the optional Rich-header switch.
TEST(PeLoaderSynthetic, ValidatesRichHeaderBounds) {
    const auto invalid_padding = pe::PeLoader::load(make_rich_edge_case_pe(true, false));
    ASSERT_TRUE(invalid_padding.has_value());
    EXPECT_FALSE(invalid_padding->rich_header().has_value());

    const auto partial_record = pe::PeLoader::load(make_rich_edge_case_pe(false, true));
    ASSERT_TRUE(partial_record.has_value());
    EXPECT_FALSE(partial_record->rich_header().has_value());

    pe::LoadOptions options;
    options.parse_rich_header = false;
    const auto disabled = pe::PeLoader::load(make_rich_edge_case_pe(false, false), options);
    ASSERT_TRUE(disabled.has_value());
    EXPECT_FALSE(disabled->rich_header().has_value());
}

/// Verifies that a header block larger than EOF is zero-filled rather than read past the input span.
TEST(PeLoaderSynthetic, BoundsHeaderCopyAtEndOfFile) {
    const auto result = pe::PeLoader::load(make_truncated_headers_pe());
    ASSERT_TRUE(result.has_value()) << (result ? "" : result.error().message);
    const auto missing_header_byte = result->read_byte(0x140000700ULL);
    ASSERT_TRUE(missing_header_byte.has_value());
    EXPECT_EQ(*missing_header_byte, 0);
}

/// Verifies that moving an image leaves the source object in a safe empty state for accessor calls.
TEST(PeLoaderSynthetic, KeepsMovedFromImageSafe) {
    auto result = pe::PeLoader::load(make_minimal_pe(true));
    ASSERT_TRUE(result.has_value()) << (result ? "" : result.error().message);
    pe::LoadedPeImage moved = std::move(*result);
    EXPECT_FALSE(moved.sections().empty());
    EXPECT_EQ(result->parse_status(), pe::ParseStatus::complete);
    EXPECT_TRUE(result->sections().empty());
    EXPECT_TRUE(result->file_bytes().empty());
}

/// Verifies invalid signatures, truncated headers, out-of-file sections, and image-size limits.
TEST(PeLoaderSynthetic, RejectsMalformedRanges) {
    auto bad_signature = make_minimal_pe(true);
    bad_signature[0x80] = 'X';
    auto signature_result = pe::PeLoader::load(bad_signature);
    ASSERT_FALSE(signature_result.has_value());
    EXPECT_EQ(signature_result.error().code, pe::ParseErrorCode::invalid_pe_signature);

    auto truncated = make_minimal_pe(true);
    truncated.resize(0x90);
    auto truncated_result = pe::PeLoader::load(truncated);
    ASSERT_FALSE(truncated_result.has_value());
    EXPECT_TRUE(truncated_result.error().code == pe::ParseErrorCode::truncated ||
                truncated_result.error().code == pe::ParseErrorCode::invalid_pe_signature);

    auto invalid_section = make_minimal_pe(true);
    constexpr std::size_t section_offset = 0x188;
    put_u32(invalid_section, section_offset + 12, 0x4000);
    auto section_result = pe::PeLoader::load(invalid_section);
    ASSERT_FALSE(section_result.has_value());
    EXPECT_EQ(section_result.error().code, pe::ParseErrorCode::invalid_section_range);

    auto oversized_image = make_minimal_pe(true);
    put_u32(oversized_image, 0x98 + 56, 0x100000);
    pe::LoadOptions options;
    options.maximum_image_size = 0x10000;
    auto size_result = pe::PeLoader::load(oversized_image, options);
    ASSERT_FALSE(size_result.has_value());
    EXPECT_EQ(size_result.error().code, pe::ParseErrorCode::invalid_image_size);
}

} // namespace
