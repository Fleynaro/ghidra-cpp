# FunctionID CLI

`function_id_cli` identifies a raw function extent using the autonomous Sleigh decoder and original
Ghidra `.fidb` databases.

## Usage

Run from the repository root or from this project root:

```text
build/services/function_id/function_id_cli.exe --hex "48 8b d9 c3" --fidb vs2017_x64.fidb
```

Use `--sla` to select another compiled Sleigh language, `--language` and `--compiler` to filter
library records, and `--context name=value` for processor context overrides. The CLI accepts direct
database names from [`../data/`](../data/), full paths, and single-directory `*` or `?` globs. The
`.fidb` suffix is optional for a database name. Omitting `--fidb`, or passing `--fidb ""`, searches
all databases in [`../data/`](../data/). The equivalent `--fidb=` spelling is also supported; repeat
`--fidb` to search a selected set.
