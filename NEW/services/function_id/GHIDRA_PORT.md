# Ghidra Port Evidence

The service preserves `FidDB`, `FidHashQuad`, `FidProgramSeeker`, `MessageDigestFidHasher`, and relation scoring behavior from `Features/FunctionID/src/main/java/ghidra/feature/fid`. The packed `.fidb` parser and focused parity tests remain in [`src`](src) and [`tests`](tests).

The service deliberately returns candidates/evidence instead of directly renaming functions. A runtime command may accept a selected candidate and append a source-priority function/symbol event.
