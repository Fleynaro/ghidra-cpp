# Aggressive Instruction Finder

Ports the opt-in generic and ARM aggressive finders from
[`AggressiveInstructionFinderAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/AggressiveInstructionFinderAnalyzer.java)
and [`ArmAggressiveInstructionFinderAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/prototype/analysis/ArmAggressiveInstructionFinderAnalyzer.java).

The implementation hashes masked bytes from the first two instructions of
known functions, scans only undefined executable bytes, validates candidate
pseudo-flow with a 4000-instruction bound, rejects data overlap and short
non-informative routines, and disassembles/bookmarks accepted candidates. ARM
filler and duplicate-instruction guards are retained for non-x86 provider
instructions.

Implementation: [`src/aggressive_instruction_finder.cppm`](src/aggressive_instruction_finder.cppm)
Build/tests: [`CMakeLists.txt`](CMakeLists.txt)
Build wrapper: [`build.bat`](build.bat)
Focused tests: [`tests/aggressive_instruction_finder_tests.cppm`](tests/aggressive_instruction_finder_tests.cppm)
Fixture evidence: [`tests/data/`](tests/data/)
Shared parent APIs: [`../shared/`](../shared/)
Port evidence: [`GHIDRA_PORT.md`](GHIDRA_PORT.md)
