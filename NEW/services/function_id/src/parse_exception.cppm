export module function_id:parse_exception;

import std;
import :types;

// Ghidra references: Framework/DB/src/main/java/db/Database.java and
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/FidDB.java.
// This private exception preserves the native parser's stable public error result.

export namespace fid::detail {

/// Carries a parser error through nested storage helpers without exposing an internal exception type publicly.
class ParseException final : public std::runtime_error {
public:
    /// Constructs a parser exception with a public error code and diagnostic.
    ParseException(fid::ErrorCode code, std::string message)
        : std::runtime_error(message), error{code, std::move(message)} {}

    fid::Error error;
};

} // namespace fid::detail
