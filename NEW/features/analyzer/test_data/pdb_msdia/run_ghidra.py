"""Run the legacy PDB MSDIA analyzer and render observed PDB artifacts."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "PDB MSDIA"


def configure_analysis(project, program, pdb_path: Path) -> list[str]:
    """Disable unrelated boolean analyzers, select MSDIA, and set its PDB override."""
    from ghidra.app.plugin.core.analysis import PdbAnalyzer
    from ghidra.framework.options import OptionType
    from java.io import File

    options = project.getAnalysisOptions(program)
    for name in list(options.getOptionNames()):
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, False)
    options.setBoolean(ANALYZER_NAME, True)
    PdbAnalyzer.setPdbFileOption(program, File(str(pdb_path)))
    return sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE
        and options.getBoolean(name, False)
    )


def import_and_reopen(project, input_path: Path):
    """Import, persist, close, and reopen the executable through GhidraProject.openProgram."""
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError("Ghidra failed to import the executable")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    return project.openProgram("/", input_path.name, False)


def artifact_rows(program):
    """Extract stable PDB properties, symbols, functions, and applied data types."""
    from ghidra.program.model.listing import Program

    info = program.getOptions(Program.PROGRAM_INFO)
    properties = []
    for name in sorted(info.getOptionNames()):
        if "PDB" in str(name).upper() or "CODEVIEW" in str(name).upper():
            properties.append((str(name), str(info.getValueAsString(name))))

    symbols = []
    iterator = program.getSymbolTable().getAllSymbols(False)
    while iterator.hasNext():
        symbol = iterator.next()
        if not symbol.isExternal():
            symbols.append((str(symbol.getName()), f"0x{int(symbol.getAddress().getOffset()):016X}", str(symbol.getSource())))

    functions = []
    for function in program.getFunctionManager().getFunctions(True):
        if not function.isExternal():
            functions.append((str(function.getName()), f"0x{int(function.getEntryPoint().getOffset()):016X}"))

    data_types = []
    for data_type in program.getDataTypeManager().getAllDataTypes():
        data_types.append(str(data_type.getPathName()))
    return properties, sorted(symbols), sorted(functions), sorted(set(data_types))


def render(input_path: Path, pdb_path: Path, enabled: list[str], artifacts) -> str:
    """Render observed legacy analyzer output without asserting environment-dependent rows."""
    properties, symbols, functions, data_types = artifacts
    loaded = any(name == "PDB Loaded" and value.lower() == "true" for name, value in properties)
    lines = [
        "# PDB MSDIA Behavioral Fixture",
        "",
        "> Generated from actual Ghidra program state; no PDB-derived addresses are hard-coded.",
        "",
        "## Input",
        "",
        f"- **Executable:** `{input_path.name}`",
        f"- **PDB:** `{pdb_path.name}`",
        f"- **Executable bytes:** `{input_path.stat().st_size}`",
        f"- **PDB applied by MSDIA:** `{str(loaded).lower()}`",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean option |",
        "| --- |",
    ]
    lines.extend(f"| `{name}` |" for name in enabled)
    lines.extend(["", "## PDB Program Properties", "", "| Name | Value |", "| --- | --- |"])
    lines.extend(f"| `{name}` | `{value}` |" for name, value in properties)
    symbol_heading = "PDB-Derived Symbols" if loaded else "Observed Symbols Without PDB Application"
    function_heading = "PDB-Derived Functions" if loaded else "Observed Functions Without PDB Application"
    type_heading = "PDB-Derived Data Types" if loaded else "Observed Data Types Without PDB Application"
    lines.extend(["", f"## {symbol_heading}", "", "| Name | Address | Source |", "| --- | --- | --- |"])
    lines.extend(f"| `{name}` | `{address}` | `{source}` |" for name, address, source in symbols)
    lines.extend(["", f"## {function_heading}", "", "| Name | Entry |", "| --- | --- |"])
    lines.extend(f"| `{name}` | `{address}` |" for name, address in functions)
    lines.extend(["", f"## {type_heading}", "", "| Path |", "| --- |"])
    lines.extend(f"| `{name}` |" for name in data_types)
    if not loaded:
        lines.extend(
            [
                "",
                "## Environment Limitation",
                "",
                "The legacy analyzer did not set `PDB Loaded` and no PDB-derived symbols or user types were observed. This run therefore does not claim raw-PDB extraction; install/configure the Windows DIA SDK or provide a matching preprocessed `.pdb.xml` for MSDIA processing.",
            ]
        )
    return "\n".join(lines) + "\n"


def main() -> int:
    """Create a temporary project, analyze, save the report, and clean up on every path."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_pdb_msdia.exe"
    pdb_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_pdb_msdia.pdb"
    output_path = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else fixture_dir / "test_pdb_msdia.md"
    if not input_path.is_file() or not pdb_path.is_file():
        print(f"ERROR: matching executable and PDB are required: {input_path}, {pdb_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    sys.path.insert(0, str(fixture_dir.parent))
    from pdb_validation import validate_pdb_match

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_pdb_msdia_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "pdb_msdia", False)
        program = import_and_reopen(project, input_path)
        validate_pdb_match(program, pdb_path)
        enabled = configure_analysis(project, program, pdb_path)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        project.analyze(program)
        project.save(program)
        observed = artifact_rows(program)
        output_path.write_text(render(input_path, pdb_path, enabled, observed), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
        if not any(name == "PDB Loaded" and value.lower() == "true" for name, value in observed[0]):
            print("WARNING: PDB MSDIA did not apply the raw PDB; see the generated environment-limitation section.", file=sys.stderr)
    except Exception as error:
        print(f"ERROR: PDB MSDIA analysis failed; DIA may be unavailable: {error}", file=sys.stderr)
        return 1
    finally:
        if project is not None:
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
