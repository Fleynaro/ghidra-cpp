"""Run PDB Universal and render artifacts that the raw PDB applicator applied."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "PDB Universal"


def configure_analysis(project, program, pdb_path: Path) -> list[str]:
    """Disable unrelated booleans, select Universal, and force the matching raw PDB."""
    from ghidra.app.plugin.core.analysis import PdbUniversalAnalyzer
    from ghidra.framework.options import OptionType
    from java.io import File

    options = project.getAnalysisOptions(program)
    for name in list(options.getOptionNames()):
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, False)
    options.setBoolean(ANALYZER_NAME, True)
    PdbUniversalAnalyzer.setPdbFileOption(program, File(str(pdb_path)))
    return sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE
        and options.getBoolean(name, False)
    )


def import_and_reopen(project, input_path: Path):
    """Import and save once, then exercise the required project.openProgram workflow."""
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError("Ghidra failed to import the executable")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    return project.openProgram("/", input_path.name, False)


def artifacts(program):
    """Collect PDB program properties and applied symbols/functions/types in stable order."""
    from ghidra.program.model.listing import Program

    info = program.getOptions(Program.PROGRAM_INFO)
    properties = sorted(
        (str(name), str(info.getValueAsString(name)))
        for name in info.getOptionNames()
        if "PDB" in str(name).upper() or "CODEVIEW" in str(name).upper()
    )
    symbols = []
    iterator = program.getSymbolTable().getAllSymbols(False)
    while iterator.hasNext():
        symbol = iterator.next()
        if not symbol.isExternal():
            symbols.append((str(symbol.getName()), f"0x{int(symbol.getAddress().getOffset()):016X}", str(symbol.getSource())))
    functions = sorted(
        (str(function.getName()), f"0x{int(function.getEntryPoint().getOffset()):016X}")
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal()
    )
    types = sorted(set(str(data_type.getPathName()) for data_type in program.getDataTypeManager().getAllDataTypes()))
    return properties, sorted(symbols), functions, types


def render(input_path: Path, pdb_path: Path, enabled: list[str], values) -> str:
    """Render Universal-specific evidence and distinguish it from source expectations."""
    properties, symbols, functions, types = values
    lines = [
        "# PDB Universal Behavioral Fixture",
        "",
        "> Generated from actual Ghidra state after raw PDB Universal analysis.",
        "> Rows below are PDB-derived artifacts, not source-level expected data.",
        "",
        "## Input",
        "",
        f"- **Executable:** `{input_path.name}`",
        f"- **Raw PDB:** `{pdb_path.name}`",
        f"- **Executable bytes:** `{input_path.stat().st_size}`",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean option |",
        "| --- |",
    ]
    lines.extend(f"| `{name}` |" for name in enabled)
    lines.extend(["", "## PDB-Derived Program Properties", "", "| Name | Value |", "| --- | --- |"])
    lines.extend(f"| `{name}` | `{value}` |" for name, value in properties)
    lines.extend(["", "## PDB-Derived Symbols", "", "| Name | Address | Source |", "| --- | --- | --- |"])
    lines.extend(f"| `{name}` | `{address}` | `{source}` |" for name, address, source in symbols)
    lines.extend(["", "## PDB-Derived Functions", "", "| Name | Entry |", "| --- | --- |"])
    lines.extend(f"| `{name}` | `{address}` |" for name, address in functions)
    lines.extend(["", "## PDB-Derived Data Types", "", "| Path |", "| --- |"])
    lines.extend(f"| `{name}` |" for name in types)
    return "\n".join(lines) + "\n"


def main() -> int:
    """Run Universal in an isolated temporary project and cleanly save and close it."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_pdb_universal.exe"
    pdb_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_pdb_universal.pdb"
    output_path = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else fixture_dir / "test_pdb_universal.md"
    if not input_path.is_file() or not pdb_path.is_file():
        print(f"ERROR: matching executable and PDB are required: {input_path}, {pdb_path}", file=sys.stderr)
        return 2
    import pyghidra
    pyghidra.start()
    from ghidra.base.project import GhidraProject
    sys.path.insert(0, str(fixture_dir.parent))
    from pdb_validation import validate_pdb_match

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_pdb_universal_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "pdb_universal", False)
        program = import_and_reopen(project, input_path)
        validate_pdb_match(program, pdb_path)
        enabled = configure_analysis(project, program, pdb_path)
        if enabled != [ANALYZER_NAME]:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        project.analyze(program)
        project.save(program)
        output_path.write_text(render(input_path, pdb_path, enabled, artifacts(program)), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    except Exception as error:
        print(f"ERROR: PDB Universal analysis failed: {error}", file=sys.stderr)
        return 1
    finally:
        if project is not None:
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
