export module function_id;

// Public API boundary corresponding to the data and query contracts in:
// Features/FunctionID/src/main/java/ghidra/feature/fid/db/FidDB.java,
// Features/FunctionID/src/main/java/ghidra/feature/fid/hash/FidHashQuad.java, and
// Features/FunctionID/src/main/java/ghidra/feature/fid/service/FidProgramSeeker.java.
// The public interface is intentionally a small umbrella. Its value types and
// the two public service classes are implemented in dedicated module partitions.
export import :types;
export import :hasher;
export import :database;
