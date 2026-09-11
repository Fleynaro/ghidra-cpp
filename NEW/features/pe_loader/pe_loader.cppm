module;

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

export module pe_loader;

// Ported/adapted from Ghidra:
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/PortableExecutable.java
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/NTHeader.java
// ../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/OptionalHeader.java
// The public model intentionally replaces Program, Memory, AddressFactory, and DB objects with
// owned value types and checked translation methods.

export namespace pe {

using Byte = std::uint8_t;
using Rva = std::uint32_t;
using FileOffset = std::uint64_t;
using Va = std::uint64_t;

/// Identifies a failure found while validating or decoding a PE file.
enum class ParseErrorCode {
    empty_input,
    io_failure,
    truncated,
    invalid_dos_signature,
    invalid_pe_signature,
    invalid_dos_offset,
    invalid_optional_header,
    invalid_alignment,
    invalid_image_size,
    invalid_section_table,
    invalid_section_range,
    invalid_directory,
    invalid_import,
    invalid_export,
    invalid_relocation,
    invalid_debug_data,
    invalid_resource,
    invalid_certificate,
    unsupported,
    limit_exceeded,
};

/// Describes whether a successfully returned image contains every requested parse result.
enum class ParseStatus {
    complete,
    partial,
};

/// Carries a stable error category, source offset, and actionable diagnostic.
struct ParseError {
    ParseErrorCode code{};
    FileOffset offset{};
    std::string message;
};

/// Identifies a failed translation between PE address spaces.
enum class AddressErrorCode {
    overflow,
    outside_image,
    unmapped,
    not_file_backed,
};

/// Describes why an RVA, VA, or file offset cannot be translated.
struct AddressError {
    AddressErrorCode code{};
    Va address{};
    std::uint64_t size{};
    std::string message;
};

/// Describes why an image-memory read could not be completed.
enum class MemoryErrorCode {
    overflow,
    unmapped,
    outside_image,
};

/// Carries a checked memory-access failure.
struct MemoryError {
    MemoryErrorCode code{};
    Va address{};
    std::uint64_t size{};
    std::string message;
};

/// Enumerates PE machine values while retaining unknown values in the raw header.
enum class Machine : std::uint16_t {
    unknown = 0,
    i386 = 0x014c,
    amd64 = 0x8664,
    arm = 0x01c0,
    armnt = 0x01c4,
    thumb = 0x01c2,
    arm64 = 0xaa64,
    arm64ec = 0xa641,
    arm64x = 0xa64e,
    ia64 = 0x0200,
    mips16 = 0x0266,
    mipsfpu = 0x0366,
    mipsfpu16 = 0x0466,
    powerpc = 0x01f0,
    powerpcfp = 0x01f1,
    sh3 = 0x01a2,
    sh4 = 0x01a6,
    ebc = 0x0ebc,
    riscv32 = 0x5032,
    riscv64 = 0x5064,
    riscv128 = 0x5128,
    loongarch32 = 0x6232,
    loongarch64 = 0x6264,
};

/// Returns the conventional architecture name for a PE machine value.
[[nodiscard]] std::string_view machine_name(Machine machine) noexcept;

/// Enumerates the sixteen standard PE data-directory slots.
enum class DirectoryIndex : std::uint32_t {
    export_table = 0,
    import_table = 1,
    resource_table = 2,
    exception_table = 3,
    security = 4,
    base_relocation_table = 5,
    debug = 6,
    architecture = 7,
    global_pointer = 8,
    tls_table = 9,
    load_config = 10,
    bound_import = 11,
    import_address_table = 12,
    delay_import = 13,
    clr_runtime_header = 14,
    reserved = 15,
};

/// Returns the Windows symbolic name for a directory index.
[[nodiscard]] std::string_view directory_name(DirectoryIndex index) noexcept;

/// Decodes the fixed IMAGE_DOS_HEADER fields without exposing file readers.
struct DosHeader {
    std::uint16_t e_magic{};
    std::uint16_t e_cblp{};
    std::uint16_t e_cp{};
    std::uint16_t e_crlc{};
    std::uint16_t e_cparhdr{};
    std::uint16_t e_minalloc{};
    std::uint16_t e_maxalloc{};
    std::uint16_t e_ss{};
    std::uint16_t e_sp{};
    std::uint16_t e_csum{};
    std::uint16_t e_ip{};
    std::uint16_t e_cs{};
    std::uint16_t e_lfarlc{};
    std::uint16_t e_ovno{};
    std::uint16_t e_res[4]{};
    std::uint16_t e_oemid{};
    std::uint16_t e_oeminfo{};
    std::uint16_t e_res2[10]{};
    std::uint32_t e_lfanew{};
};

/// Represents one decoded record from the XOR-protected Microsoft Rich header.
struct RichRecord {
    std::uint32_t comp_id{};
    std::uint16_t product_id{};
    std::uint16_t build{};
    std::uint32_t object_count{};
};

/// Represents the optional Rich header found between the DOS stub and NT header.
struct RichHeader {
    FileOffset offset{};
    std::uint32_t size{};
    std::uint32_t mask{};
    std::vector<RichRecord> records;
};

/// Decodes the fixed 20-byte IMAGE_FILE_HEADER fields.
struct CoffHeader {
    std::uint16_t machine_raw{};
    Machine machine{Machine::unknown};
    std::uint16_t number_of_sections{};
    std::uint32_t time_date_stamp{};
    std::uint32_t pointer_to_symbol_table{};
    std::uint32_t number_of_symbols{};
    std::uint16_t size_of_optional_header{};
    std::uint16_t characteristics{};
    FileOffset file_offset{};
};

/// Describes one RVA/size pair from the optional-header data-directory array.
struct DataDirectory {
    DirectoryIndex index{DirectoryIndex::reserved};
    std::uint32_t index_raw{};
    Rva rva{};
    std::uint32_t size{};
    bool address_is_file_offset{};
    std::optional<FileOffset> file_offset;
};

/// Represents the ASCII payload of IMAGE_DIRECTORY_ENTRY_ARCHITECTURE.
struct ArchitectureDirectory {
    Rva rva{};
    std::uint32_t size{};
    std::string copyright;
    std::optional<FileOffset> file_offset;
};

/// Represents the validated RVA stored by IMAGE_DIRECTORY_ENTRY_GLOBALPTR.
struct GlobalPointerDirectory {
    Rva rva{};
    std::uint32_t size{};
    Va global_pointer_va{};
    std::optional<FileOffset> file_offset;
};

/// Decodes PE32 and PE32+ optional headers while preserving directory values.
struct OptionalHeader {
    std::uint16_t magic{};
    bool pe32_plus{};
    std::uint8_t major_linker_version{};
    std::uint8_t minor_linker_version{};
    std::uint32_t size_of_code{};
    std::uint32_t size_of_initialized_data{};
    std::uint32_t size_of_uninitialized_data{};
    Rva address_of_entry_point{};
    Rva base_of_code{};
    std::optional<Rva> base_of_data;
    Va image_base{};
    std::uint32_t section_alignment{};
    std::uint32_t file_alignment{};
    std::uint16_t major_operating_system_version{};
    std::uint16_t minor_operating_system_version{};
    std::uint16_t major_image_version{};
    std::uint16_t minor_image_version{};
    std::uint16_t major_subsystem_version{};
    std::uint16_t minor_subsystem_version{};
    std::uint32_t win32_version_value{};
    std::uint32_t size_of_image{};
    std::uint32_t size_of_headers{};
    std::uint32_t check_sum{};
    std::uint16_t subsystem{};
    std::uint16_t dll_characteristics{};
    std::uint64_t size_of_stack_reserve{};
    std::uint64_t size_of_stack_commit{};
    std::uint64_t size_of_heap_reserve{};
    std::uint64_t size_of_heap_commit{};
    std::uint32_t loader_flags{};
    std::uint32_t number_of_rva_and_sizes{};
    std::vector<DataDirectory> data_directories;
    FileOffset file_offset{};
    std::uint16_t declared_size{};
};

/// Represents section permission and image characteristics bits.
namespace section_characteristics {
inline constexpr std::uint32_t type_no_pad = 0x00000008;
inline constexpr std::uint32_t contains_code = 0x00000020;
inline constexpr std::uint32_t contains_initialized_data = 0x00000040;
inline constexpr std::uint32_t contains_uninitialized_data = 0x00000080;
inline constexpr std::uint32_t memory_discardable = 0x02000000;
inline constexpr std::uint32_t memory_not_cached = 0x04000000;
inline constexpr std::uint32_t memory_not_paged = 0x08000000;
inline constexpr std::uint32_t memory_shared = 0x10000000;
inline constexpr std::uint32_t memory_execute = 0x20000000;
inline constexpr std::uint32_t memory_read = 0x40000000;
inline constexpr std::uint32_t memory_write = 0x80000000;
} // namespace section_characteristics

/// Represents one IMAGE_SECTION_HEADER and both its memory-block and aligned virtual extents.
struct Section {
    std::string name;
    std::uint32_t virtual_size{};
    Rva virtual_address{};
    std::uint32_t raw_size{};
    std::uint32_t file_backed_size{};
    FileOffset raw_offset{};
    std::uint32_t pointer_to_relocations{};
    std::uint32_t pointer_to_line_numbers{};
    std::uint16_t number_of_relocations{};
    std::uint16_t number_of_line_numbers{};
    std::uint32_t characteristics{};
    /// Size exposed by Ghidra's loaded memory block and used for mapped reads.
    std::uint32_t loaded_size{};
    /// SectionAlignment-rounded extent used for image-range validation.
    std::uint32_t virtual_extent{};
    std::size_t index{};
};

/// Describes a loaded image range and its effective permissions.
struct MemoryRegion {
    std::string name;
    Va start{};
    std::uint64_t size{};
    bool readable{};
    bool writable{};
    bool executable{};
    bool initialized{};
    bool headers{};
    std::optional<std::size_t> section_index;
};

/// Represents one imported symbol and both thunk locations.
struct ImportedSymbol {
    std::string name;
    std::optional<std::uint16_t> ordinal;
    std::uint16_t hint{};
    bool imported_by_ordinal{};
    Rva int_slot_rva{};
    Rva iat_slot_rva{};
    Va iat_slot_va{};
    std::optional<FileOffset> iat_slot_file_offset;
    std::uint64_t thunk_value{};
    std::optional<Rva> import_by_name_rva;
    std::optional<FileOffset> import_by_name_file_offset;
};

/// Represents one raw slot in the import address table directory.
struct IatEntry {
    Rva slot_rva{};
    Va slot_va{};
    std::optional<FileOffset> file_offset;
    std::uint64_t value{};
};

/// Represents one IMAGE_IMPORT_DESCRIPTOR and all decoded thunk entries.
struct ImportDescriptor {
    std::string dll_name;
    Rva name_rva{};
    std::optional<FileOffset> name_file_offset;
    Rva original_first_thunk_rva{};
    Rva first_thunk_rva{};
    std::uint32_t time_date_stamp{};
    std::uint32_t forwarder_chain{};
    bool bound{};
    std::vector<ImportedSymbol> symbols;
};

/// Represents IMAGE_EXPORT_DIRECTORY header fields.
struct ExportDirectory {
    std::uint32_t characteristics{};
    std::uint32_t time_date_stamp{};
    std::uint16_t major_version{};
    std::uint16_t minor_version{};
    Rva name_rva{};
    std::string name;
    std::uint32_t ordinal_base{};
    std::uint32_t number_of_functions{};
    std::uint32_t number_of_names{};
    Rva address_of_functions_rva{};
    Rva address_of_names_rva{};
    Rva address_of_name_ordinals_rva{};
    std::optional<FileOffset> address_of_functions_file_offset;
    std::optional<FileOffset> address_of_names_file_offset;
    std::optional<FileOffset> address_of_name_ordinals_file_offset;
};

/// Represents one named or ordinal-only exported function.
struct ExportedSymbol {
    std::uint32_t ordinal{};
    std::optional<std::string> name;
    Rva address_rva{};
    Va address_va{};
    std::optional<FileOffset> file_offset;
    bool forwarded{};
    std::optional<std::string> forwarder;
};

/// Represents one IMAGE_BASE_RELOCATION block entry.
struct RelocationEntry {
    std::uint16_t raw_value{};
    std::uint16_t type{};
    std::uint16_t offset{};
    Rva target_rva{};
    Va target_va{};
    std::optional<FileOffset> target_file_offset;
};

/// Represents one IMAGE_BASE_RELOCATION block.
struct RelocationBlock {
    Rva page_rva{};
    std::uint32_t block_size{};
    std::optional<FileOffset> file_offset;
    std::vector<RelocationEntry> entries;
};

/// Enumerates standard IMAGE_DEBUG_TYPE values without rejecting extensions.
enum class DebugType : std::uint32_t {
    unknown = 0,
    coff = 1,
    code_view = 2,
    fpo = 3,
    misc = 4,
    exception = 5,
    fixup = 6,
    omap_to_src = 7,
    omap_from_src = 8,
    borland = 9,
    reserved10 = 10,
    clsid = 11,
};

/// Represents decoded CodeView payload when a debug entry contains one.
struct CodeViewInfo {
    std::string signature;
    std::vector<Byte> identifier;
    std::uint32_t age{};
    std::string path;
    bool valid{};
};

/// Represents one IMAGE_DEBUG_DIRECTORY record and its raw payload location.
struct DebugDirectoryEntry {
    std::uint32_t characteristics{};
    std::uint32_t time_date_stamp{};
    std::uint16_t major_version{};
    std::uint16_t minor_version{};
    std::uint32_t type_raw{};
    DebugType type{DebugType::unknown};
    std::uint32_t size_of_data{};
    Rva address_of_raw_data{};
    std::uint32_t pointer_to_raw_data{};
    std::optional<FileOffset> data_file_offset;
    std::optional<CodeViewInfo> code_view;
};

/// Represents one x64/ARM64 or x86 runtime-function table row.
struct RuntimeFunction {
    Rva begin_rva{};
    Rva end_rva{};
    Rva unwind_info_rva{};
    Va begin_va{};
    Va end_va{};
    Va unwind_info_va{};
    std::optional<FileOffset> unwind_info_file_offset;
    std::optional<std::uint32_t> packed_unwind_data;
};

/// Represents IMAGE_TLS_DIRECTORY32/64 and decoded callback addresses.
struct TlsDirectory {
    Va start_address_of_raw_data{};
    Va end_address_of_raw_data{};
    Va address_of_index{};
    Va address_of_callbacks{};
    std::uint32_t size_of_zero_fill{};
    std::uint32_t characteristics{};
    std::vector<Va> callback_addresses;
};

/// Represents fields shared by PE32 and PE32+ load-config records.
struct LoadConfigDirectory {
    std::uint32_t size{};
    std::uint32_t available_size{};
    std::uint32_t time_date_stamp{};
    std::uint16_t major_version{};
    std::uint16_t minor_version{};
    std::uint32_t critical_section_default_timeout{};
    Va security_cookie{};
    Va se_handler_table{};
    Va se_handler_count{};
    Va guard_cf_check_function_pointer{};
    Va guard_cf_dispatch_function_pointer{};
    Va guard_cf_function_table{};
    Va guard_cf_function_count{};
    std::uint32_t guard_flags{};
    Va guard_address_taken_iat_entry_table{};
    Va guard_address_taken_iat_entry_count{};
    Va guard_long_jump_target_table{};
    Va guard_long_jump_target_count{};
    Va chpe_metadata_pointer{};
    Va arm64ec_metadata_pointer{};
    Va guard_rf_failure_routine{};
    Va guard_rf_failure_routine_function_pointer{};
    Va guard_rf_verify_stack_pointer_function_pointer{};
    Va dynamic_value_reloc_table{};
    std::uint32_t dynamic_value_reloc_table_offset{};
    std::uint16_t dynamic_value_reloc_table_section{};
    std::vector<Byte> raw_bytes;
};

/// Identifies a resource path component by numeric ID or UTF-16 name.
struct ResourceIdentifier {
    bool named{};
    std::uint32_t id{};
    std::string name;
};

/// Represents one IMAGE_RESOURCE_DATA_ENTRY and its type/name/language path.
struct ResourceLeaf {
    ResourceIdentifier type;
    ResourceIdentifier name;
    ResourceIdentifier language;
    Rva data_rva{};
    std::uint32_t size{};
    std::uint32_t code_page{};
    std::uint32_t reserved{};
    std::optional<FileOffset> file_offset;
};

/// Represents the root metadata and all leaves of an RT resource tree.
struct ResourceDirectory {
    std::uint32_t characteristics{};
    std::uint32_t time_date_stamp{};
    std::uint16_t major_version{};
    std::uint16_t minor_version{};
    std::uint16_t number_of_named_entries{};
    std::uint16_t number_of_id_entries{};
    std::vector<ResourceLeaf> leaves;
};

/// Represents one WIN_CERTIFICATE record in the file-offset security directory.
struct SecurityCertificate {
    std::uint32_t length{};
    std::uint16_t revision{};
    std::uint16_t type{};
    FileOffset file_offset{};
    std::vector<Byte> certificate_bytes;
};

/// Represents one IMAGE_BOUND_IMPORT_DESCRIPTOR and forwarder references.
struct BoundImport {
    std::uint32_t time_date_stamp{};
    std::uint16_t module_name_offset{};
    std::uint16_t number_of_forwarder_refs{};
    std::string module_name;
    std::vector<std::string> forwarder_modules;
};

/// Represents one IMAGE_DELAYLOAD_DESCRIPTOR and its decoded delay imports.
struct DelayImportDescriptor {
    std::uint32_t attributes{};
    std::string dll_name;
    Rva name_rva{};
    Rva module_handle_rva{};
    Rva import_address_table_rva{};
    Rva import_name_table_rva{};
    Rva bound_import_address_table_rva{};
    Rva unload_information_table_rva{};
    std::uint32_t time_date_stamp{};
    bool uses_rva{};
    std::vector<ImportedSymbol> symbols;
};

/// Represents one IMAGE_COR20_HEADER for managed PE files.
struct ClrHeader {
    std::uint32_t size{};
    std::uint16_t major_runtime_version{};
    std::uint16_t minor_runtime_version{};
    Rva metadata_rva{};
    std::uint32_t metadata_size{};
    std::uint32_t flags{};
    bool entry_point_is_native{};
    std::uint32_t entry_point_token_or_rva{};
    Rva resources_rva{};
    std::uint32_t resources_size{};
    Rva strong_name_signature_rva{};
    std::uint32_t strong_name_signature_size{};
    Rva code_manager_table_rva{};
    std::uint32_t code_manager_table_size{};
    Rva vtable_fixups_rva{};
    std::uint32_t vtable_fixups_size{};
    Rva export_address_table_jumps_rva{};
    std::uint32_t export_address_table_jumps_size{};
    Rva managed_native_header_rva{};
    std::uint32_t managed_native_header_size{};
};

/// Represents one COFF symbol table record when a PE contains one.
struct CoffSymbol {
    std::string name;
    std::uint32_t value{};
    std::int16_t section_number{};
    std::uint16_t type{};
    std::uint8_t storage_class{};
    std::uint8_t auxiliary_count{};
    FileOffset file_offset{};
};

/// Controls parser strictness and resource limits for attacker-controlled input.
struct LoadOptions {
    bool parse_rich_header{true};
    bool parse_directories{true};
    bool strict{true};
    std::uint64_t maximum_image_size{1ULL << 30};
    std::uint32_t maximum_directory_entries{1U << 20};
    std::uint32_t maximum_resource_depth{32};
};

/// Owns a validated PE file and its zero-filled, section-aware loaded image.
class LoadedPeImage {
public:
    /// Opaque storage type used to keep parser implementation details out of the API.
    struct Storage;

    /// Creates an empty object used internally before successful parsing.
    LoadedPeImage();

    /// Transfers ownership of all parsed metadata and image bytes.
    LoadedPeImage(LoadedPeImage&& other) noexcept;

    /// Transfers ownership from another loaded image.
    LoadedPeImage& operator=(LoadedPeImage&& other) noexcept;

    /// Releases owned file and mapped-image storage.
    ~LoadedPeImage();

    /// Wraps parser-owned opaque storage in an image object.
    explicit LoadedPeImage(std::unique_ptr<Storage> storage);

    LoadedPeImage(const LoadedPeImage&) = delete;
    LoadedPeImage& operator=(const LoadedPeImage&) = delete;

    /// Returns the decoded DOS header.
    [[nodiscard]] const DosHeader& dos_header() const noexcept;

    /// Returns the optional Rich header, if one was found and requested.
    [[nodiscard]] const std::optional<RichHeader>& rich_header() const noexcept;

    /// Returns the decoded COFF/file header.
    [[nodiscard]] const CoffHeader& coff_header() const noexcept;

    /// Returns the decoded PE32 or PE32+ optional header.
    [[nodiscard]] const OptionalHeader& optional_header() const noexcept;

    /// Returns section headers in file order.
    [[nodiscard]] const std::vector<Section>& sections() const noexcept;

    /// Returns all directory slots represented by the optional header.
    [[nodiscard]] const std::vector<DataDirectory>& data_directories() const noexcept;

    /// Returns the architecture-specific directory payload when present.
    [[nodiscard]] const std::optional<ArchitectureDirectory>& architecture_directory() const noexcept;

    /// Returns the validated global-pointer directory when present.
    [[nodiscard]] const std::optional<GlobalPointerDirectory>& global_pointer_directory() const noexcept;

    /// Returns section/header memory ranges created by image mapping.
    [[nodiscard]] const std::vector<MemoryRegion>& memory_regions() const noexcept;

    /// Returns parsed import descriptors, or an empty vector when absent.
    [[nodiscard]] const std::vector<ImportDescriptor>& imports() const noexcept;

    /// Returns every slot represented by IMAGE_DIRECTORY_ENTRY_IAT.
    [[nodiscard]] const std::vector<IatEntry>& iat_entries() const noexcept;

    /// Returns the parsed export directory, if present.
    [[nodiscard]] const std::optional<ExportDirectory>& exports() const noexcept;

    /// Returns all exports indexed by ordinal, including unnamed entries.
    [[nodiscard]] const std::vector<ExportedSymbol>& exported_symbols() const noexcept;

    /// Returns relocation blocks and their individual fixup entries.
    [[nodiscard]] const std::vector<RelocationBlock>& relocations() const noexcept;

    /// Returns parsed IMAGE_DEBUG_DIRECTORY entries.
    [[nodiscard]] const std::vector<DebugDirectoryEntry>& debug_entries() const noexcept;

    /// Returns decoded runtime-function entries from the exception directory.
    [[nodiscard]] const std::vector<RuntimeFunction>& exception_functions() const noexcept;

    /// Returns the parsed TLS directory, if present.
    [[nodiscard]] const std::optional<TlsDirectory>& tls() const noexcept;

    /// Returns the parsed load-config directory, if present.
    [[nodiscard]] const std::optional<LoadConfigDirectory>& load_config() const noexcept;

    /// Returns the parsed resource tree, if present.
    [[nodiscard]] const std::optional<ResourceDirectory>& resources() const noexcept;

    /// Returns Authenticode certificate records, if present.
    [[nodiscard]] const std::vector<SecurityCertificate>& certificates() const noexcept;

    /// Returns bound-import descriptors, if present.
    [[nodiscard]] const std::vector<BoundImport>& bound_imports() const noexcept;

    /// Returns delay-import descriptors, if present.
    [[nodiscard]] const std::vector<DelayImportDescriptor>& delay_imports() const noexcept;

    /// Returns the CLR header, if the PE has a COM descriptor.
    [[nodiscard]] const std::optional<ClrHeader>& clr_header() const noexcept;

    /// Returns decoded COFF symbols when the file has a symbol table.
    [[nodiscard]] const std::vector<CoffSymbol>& coff_symbols() const noexcept;

    /// Returns the completeness status, including non-strict directory failures.
    [[nodiscard]] ParseStatus parse_status() const noexcept;

    /// Returns true when non-strict parsing skipped or incompletely decoded data.
    [[nodiscard]] bool is_partial() const noexcept;

    /// Returns diagnostics recorded for failures tolerated by non-strict parsing.
    [[nodiscard]] const std::vector<ParseError>& parse_diagnostics() const noexcept;

    /// Returns an immutable view of the original file bytes.
    [[nodiscard]] std::span<const Byte> file_bytes() const noexcept;

    /// Returns the image size declared by the optional header.
    [[nodiscard]] std::uint32_t image_size() const noexcept;

    /// Converts an RVA backed by file bytes to its raw file offset.
    [[nodiscard]] std::expected<FileOffset, AddressError> rva_to_file_offset(Rva rva) const;

    /// Converts a raw file offset in a header or section to an RVA.
    [[nodiscard]] std::expected<Rva, AddressError> file_offset_to_rva(FileOffset offset) const;

    /// Converts an in-image RVA to its preferred-image virtual address.
    [[nodiscard]] std::expected<Va, AddressError> rva_to_va(Rva rva) const;

    /// Converts a preferred-image virtual address to an in-image RVA.
    [[nodiscard]] std::expected<Rva, AddressError> va_to_rva(Va va) const;

    /// Reads bytes from a mapped region and returns zero-filled virtual tails.
    [[nodiscard]] std::expected<std::vector<Byte>, MemoryError> read_memory(Va address, std::uint64_t size) const;

    /// Reads one byte from a mapped image region.
    [[nodiscard]] std::expected<Byte, MemoryError> read_byte(Va address) const;

    /// Copies a resource leaf payload from the mapped image or reports a checked memory error.
    [[nodiscard]] std::expected<std::vector<Byte>, MemoryError> read_resource_payload(const ResourceLeaf& leaf) const;

    /// Copies the indexed resource leaf payload from the parsed resource tree.
    [[nodiscard]] std::expected<std::vector<Byte>, MemoryError> read_resource_payload(std::size_t leaf_index) const;

private:
    std::unique_ptr<Storage> storage_;

    friend class PeLoader;
};

/// Parses PE bytes into an owned, standalone loaded image.
class PeLoader {
public:
    /// Validates and parses a PE byte span without retaining the caller's storage.
    [[nodiscard]] static std::expected<LoadedPeImage, ParseError> load(std::span<const Byte> bytes,
                                                                       const LoadOptions& options = {});

    /// Reads a file and parses it using the same checked loader pipeline.
    [[nodiscard]] static std::expected<LoadedPeImage, ParseError> load_file(const std::filesystem::path& path,
                                                                            const LoadOptions& options = {});
};

} // namespace pe
