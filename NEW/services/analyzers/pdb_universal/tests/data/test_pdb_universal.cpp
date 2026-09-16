// MSVC x64 /Zi PDB fixture for Ghidra's PDB Universal analyzer.
//
// Exact Java implementation:
//   Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbUniversalAnalyzer.java
//   Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/PdbAnalyzerCommon.java
//   Ghidra/Features/PDB/src/main/java/ghidra/app/util/bin/format/pdb2/pdbreader/PdbParser.java
//   Ghidra/Features/PDB/src/main/java/ghidra/app/util/pdb/pdbapplicator/DefaultPdbApplicator.java
//
// Intended behavior and artifacts:
// * /Zi and /DEBUG:FULL place an RSDS record in the PE and complete type, symbol,
//   line, and function-internal records in test_pdb_universal.pdb.
// * The harness sets PdbUniversalAnalyzer.setPdbFileOption() before analysis and
//   enables only the named analyzer. Universal must discover and apply actual PDB
//   symbols/types, then its scheduled internals and reporting stages may add more
//   function metadata. The report records these PDB-derived artifacts explicitly.
// * PdbUniversalRecord, PdbUniversalMode, pdb_universal_global, and the two C
//   functions are retained by observable code so optimization cannot erase the
//   intended PDB discovery surface.

struct PdbUniversalRecord {
    unsigned int identifier;
    long long quantity;
    const char* label;
};

enum PdbUniversalMode { PdbUniversalModeRead = 3, PdbUniversalModeWrite = 5 };

extern "C" volatile unsigned long long pdb_universal_global = 0ULL;

// Consume both fields and the enum so Universal can expose their debug records.
extern "C" __declspec(noinline) long long pdb_universal_compute(const PdbUniversalRecord* record,
                                                                PdbUniversalMode mode) {
    pdb_universal_global += static_cast<unsigned long long>(mode);
    return static_cast<long long>(record->identifier) + record->quantity + static_cast<long long>(pdb_universal_global);
}

// Keep a non-trivial entry point and a source-level call relationship in the PDB.
extern "C" __declspec(noinline) void pdb_universal_entry() {
    PdbUniversalRecord record{29U, 64LL, "universal-fixture"};
    pdb_universal_global = static_cast<unsigned long long>(pdb_universal_compute(&record, PdbUniversalModeRead));
}
