# Ghidra Port Evidence

The service preserves `FidDB`, `FidHashQuad`, `FidProgramSeeker`, `MessageDigestFidHasher`, and relation scoring behavior from `Features/FunctionID/src/main/java/ghidra/feature/fid`. The packed `.fidb` parser and focused parity tests remain in [`src`](src) and [`tests`](tests).

The service deliberately returns candidates/evidence instead of directly renaming functions. A runtime command may accept a selected candidate and append a source-priority function/symbol event.

The portable operand-object facts now come from [`../../core/domain/operand.cppm`](../../core/domain/operand.cppm), while Function ID keeps its algorithm-specific masks, skip flags, and hash records private. The service performs candidate lookup through [`../../core/contracts/function_id.cppm`](../../core/contracts/function_id.cppm).
