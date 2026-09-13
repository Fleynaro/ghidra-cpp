# FunctionID CLI

`function_id_cli` identifies a raw function extent using the autonomous Sleigh decoder and original
Ghidra `.fidb` databases.

## Usage

Run from the repository root or from `NEW`:

```text
NEW/build/features/function_id/function_id_cli.exe --hex "48 8b d9 c3" --fidb "TEST/fid/*.fidb"
```

Use `--sla` to select another compiled Sleigh language, `--language` and `--compiler` to filter
library records, and `--context name=value` for processor context overrides. The CLI accepts direct
database paths and single-directory `*` or `?` globs.
