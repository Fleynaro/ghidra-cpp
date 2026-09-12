export module function_id;

import std;
import sleigh_runtime;

export namespace fid {

using Byte = std::uint8_t;

/// Identifies a failure while hashing, opening, or querying a FunctionID database.
enum class ErrorCode : std::uint8_t {
    invalid_input,
    io_failure,
    invalid_packed_item,
    invalid_zip,
    invalid_buffer_file,
    invalid_database,
    unsupported_schema,
    unsupported_architecture,
    malformed_record,
};

/// Carries a stable error category and an actionable diagnostic.
struct Error {
    ErrorCode code{};
    std::string message;
};

/// Identifies the object classes understood by the specific-hash algorithm.
enum class OperandObjectKind : std::uint8_t {
    scalar,
    register_value,
    address,
};

/// Supplies one Ghidra instruction operand object to the autonomous hasher.
/// Scalar values are signed exactly as Ghidra's Scalar.getSignedValue() returns them.
struct OperandObject {
    OperandObjectKind kind{OperandObjectKind::scalar};
    std::int64_t value{};
    bool whole_scalar{true};
    bool address_scalar{};
    bool relocated{};
};

/// Describes one instruction in the order used by a FunctionBodyFunctionExtentGenerator.
/// The instruction mask and operand masks use the same byte ordering as the instruction bytes.
struct Instruction {
    std::vector<Byte> bytes;
    std::vector<Byte> instruction_mask;
    std::vector<std::vector<Byte>> operand_masks;
    std::vector<std::vector<OperandObject>> operands;
    bool is_call{};
    bool skip{};
};

/// Describes a relocation range supplied by the host program's memory model.
struct Relocation {
    std::uint64_t address{};
    std::size_t size{};
};

/// Contains the two FNV-1a digests and the size metadata stored in a FID record.
struct HashQuad {
    std::int16_t code_unit_size{};
    std::uint64_t full_hash{};
    std::int8_t specific_hash_additional_size{};
    std::uint64_t specific_hash{};

    /// Compares all FunctionID hash fields.
    friend bool operator==(const HashQuad&, const HashQuad&) = default;
};

/// Hashes a caller-provided instruction extent using Ghidra's FunctionID algorithm.
class Hasher final {
public:
    /// Hashes instructions, returning no value when fewer than four effective units exist.
    [[nodiscard]] static std::expected<HashQuad, Error> hash(std::span<const Instruction> instructions,
                                                             std::int8_t short_limit = 4);

    /// Hashes the abstract instruction records emitted by the autonomous Sleigh runtime.
    [[nodiscard]] static std::expected<HashQuad, Error>
    hash_sleigh(std::span<const sleigh_runtime::Instruction> instructions, std::span<const Relocation> relocations = {},
                std::int8_t short_limit = 4);
};

/// Identifies one source/compiler context used for library filtering.
struct ProgramInfo {
    std::optional<std::string> language_id;
    std::optional<std::string> compiler_spec;
    /// Null means that every source language is acceptable; an empty set means none are acceptable.
    std::optional<std::set<std::string>> source_languages;
    bool ignore_database_filters{};
};

/// Describes one function's already-hashed call neighborhood.
struct FunctionContext {
    HashQuad hash;
    std::vector<HashQuad> children;
    std::vector<HashQuad> parents;
};

/// Carries all fields stored by the FunctionID Libraries Table.
struct LibraryRecord {
    std::int64_t id{};
    std::string family_name;
    std::string version;
    std::string variant;
    std::string ghidra_version;
    std::string language_id;
    std::int32_t language_version{};
    std::int32_t language_minor_version{};
    std::string metadata;
    std::set<std::string> compiler_specs;
    std::set<std::string> source_languages;
};

/// Carries all fields stored by the FunctionID Functions Table.
struct FunctionRecord {
    std::int64_t id{};
    HashQuad hash;
    std::int64_t library_id{};
    std::string name;
    std::int64_t entry_point{};
    std::string domain_path;
    std::uint8_t flags{};

    /// Returns whether the function body contained a terminator.
    [[nodiscard]] bool has_terminator() const noexcept;
    /// Returns whether the record receives the medium-size automatic pass floor.
    [[nodiscard]] bool auto_pass() const noexcept;
    /// Returns whether this candidate is rejected before scoring.
    [[nodiscard]] bool auto_fail() const noexcept;
    /// Returns whether the specific hash must match.
    [[nodiscard]] bool force_specific() const noexcept;
    /// Returns whether at least one child relation must match.
    [[nodiscard]] bool force_relation() const noexcept;
};

/// Identifies the lookup mode used for a successful candidate.
enum class MatchMode : std::uint8_t {
    full,
    specific,
};

/// Contains the exact score components calculated by FidProgramSeeker.scoreMatch().
struct Match {
    FunctionRecord function;
    LibraryRecord library;
    MatchMode mode{MatchMode::full};
    float function_score{};
    float child_score{};
    float parent_score{};

    /// Returns the total score used for descending candidate ordering.
    [[nodiscard]] float overall_score() const noexcept;
};

/// Returns all best-scoring candidates and their deduplicated raw names.
struct IdentificationResult {
    HashQuad hash;
    std::vector<Match> matches;
    std::vector<std::string> names;
};

/// Opens and queries one original Ghidra packed `.fidb` database.
class Database final {
public:
    /// Opens a packed `.fidb` path read-only without converting or rewriting it.
    [[nodiscard]] static std::expected<Database, Error> open(const std::filesystem::path& path);

    /// Releases the immutable database storage.
    ~Database();
    Database(Database&&) noexcept;
    Database& operator=(Database&&) noexcept;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /// Returns the decoded library records in primary-key order.
    [[nodiscard]] std::span<const LibraryRecord> libraries() const noexcept;
    /// Returns all records sharing a full hash, preserving database order.
    [[nodiscard]] std::vector<FunctionRecord> find_full_hash(std::uint64_t hash) const;
    /// Returns all records sharing a specific hash by the original full-table scan semantics.
    [[nodiscard]] std::vector<FunctionRecord> find_specific_hash(std::uint64_t hash) const;
    /// Returns all decoded function records.
    [[nodiscard]] std::span<const FunctionRecord> functions() const noexcept;
    /// Returns the raw database buffer size for diagnostics and tests.
    [[nodiscard]] std::size_t buffer_size() const noexcept;

    /// Identifies one function using Ghidra's candidate filtering and scoring behavior.
    [[nodiscard]] std::expected<IdentificationResult, Error> identify(const FunctionContext& context,
                                                                      const ProgramInfo& program,
                                                                      float score_threshold = 14.6F,
                                                                      std::int16_t medium_limit = 24) const;

private:
    class Storage;
    std::unique_ptr<Storage> storage_;
    explicit Database(std::unique_ptr<Storage> storage);
};

/// Computes the signed-64-bit relation key used by the original RelationsTable.
[[nodiscard]] std::uint64_t superior_relation_key(std::int64_t superior_id, std::uint64_t inferior_full_hash) noexcept;
/// Computes the signed-64-bit reverse relation key used by the original RelationsTable.
[[nodiscard]] std::uint64_t inferior_relation_key(std::uint64_t superior_full_hash, std::int64_t inferior_id) noexcept;

} // namespace fid
