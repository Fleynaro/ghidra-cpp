// MSVC x64 /Zi PDB fixture for the legacy PDB MSDIA analyzer.
//
// Exact Java implementation:
//   Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzer.java
//   Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzerCommon.java
//   Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb/PdbParser.java
//
// Intended behavior and artifacts:
// * The compiler emits an RSDS CodeView record in the PE that names the matching
//   test_pdb_msdia.pdb, its GUID, and its age. PdbAnalyzerCommon.findPdb() is
//   bypassed by the Python harness's explicit PdbAnalyzer.setPdbFileOption().
// * PdbAnalyzer parses the raw PDB through the Windows DIA-backed PdbParser,
//   applies symbols and data types, and marks the program's PDB properties. The
//   report discovers those properties and lists only symbols, functions, and data
//   types actually present after analysis; it does not assert fixed addresses.
// * PdbRecord, PdbKind, pdb_global, pdb_msdia_compute, and pdb_msdia_entry are
//   deliberately retained by observable calls and are the expected discovery
//   surface. Their names and type layouts must come from the PDB, not from PE
//   exports or hard-coded report rows.
//
// Environment limitation: a valid /Zi PDB can be produced without DIA, but the
// legacy Java analyzer cannot consume a raw PDB on Windows without DIA support.

struct PdbRecord {
    int identifier;
    double score;
    const char* label;
};

enum PdbKind { PdbKindPrimary = 7, PdbKindSecondary = 11 };

extern "C" volatile unsigned int pdb_global = 0U;
// The CRT-free link still requires MSVC's floating-point usage marker.
extern "C" int _fltused = 0;

// Return a computation that keeps the user-defined record and enum in debug info.
extern "C" __declspec(noinline) int pdb_msdia_compute(const PdbRecord* record, PdbKind kind) {
    pdb_global += static_cast<unsigned int>(kind);
    return record->identifier + static_cast<int>(record->score) + static_cast<int>(pdb_global);
}

// Construct a stable call graph and retain a global symbol for PDB discovery.
extern "C" __declspec(noinline) void pdb_msdia_entry() {
    PdbRecord record{13, 4.5, "msdia-fixture"};
    pdb_global = static_cast<unsigned int>(pdb_msdia_compute(&record, PdbKindPrimary));
}
