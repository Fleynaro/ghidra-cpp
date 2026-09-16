export module function_id:types;

import std;

// Ported/adapted from Ghidra:
// Features/FunctionID/src/main/java/ghidra/feature/fid/hash/FunctionRecord.java
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/LibraryRecord.java
// Features/FunctionID/src/main/java/ghidra/feature/fid/hash/FidHashQuad.java
// Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidProgramSeeker.java

export namespace fid {

/// Names one byte in a FunctionID instruction or database record.
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

/// Describes one instruction in the order used by FunctionBodyFunctionExtentGenerator.
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

    /// Returns whether the function body contained a terminator, matching FunctionRecord.hasTerminator().
    [[nodiscard]] bool has_terminator() const noexcept {
        return (flags & 0x01U) != 0;
    }

    /// Returns whether the record receives the medium-size automatic pass floor, matching autoPass().
    [[nodiscard]] bool auto_pass() const noexcept {
        return (flags & 0x02U) != 0;
    }

    /// Returns whether this candidate is rejected before scoring, matching autoFail().
    [[nodiscard]] bool auto_fail() const noexcept {
        return (flags & 0x04U) != 0;
    }

    /// Returns whether the specific hash must match, matching forceSpecific().
    [[nodiscard]] bool force_specific() const noexcept {
        return (flags & 0x08U) != 0;
    }

    /// Returns whether at least one child relation must match, matching forceRelation().
    [[nodiscard]] bool force_relation() const noexcept {
        return (flags & 0x10U) != 0;
    }
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
    [[nodiscard]] float overall_score() const noexcept {
        return function_score + child_score + parent_score;
    }
};

/// Returns all best-scoring candidates and their deduplicated raw names.
struct IdentificationResult {
    HashQuad hash;
    std::vector<Match> matches;
    std::vector<std::string> names;
};

} // namespace fid
