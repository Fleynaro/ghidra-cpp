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


def mapping_delta(before, after):
    """Return added, removed, and changed entries between two property snapshots."""
    before_map = dict(before)
    after_map = dict(after)
    return (
        sorted((name, after_map[name]) for name in set(after_map) - set(before_map)),
        sorted((name, before_map[name]) for name in set(before_map) - set(after_map)),
        sorted(
            (name, before_map[name], after_map[name])
            for name in set(before_map) & set(after_map)
            if before_map[name] != after_map[name]
        ),
    )


def set_delta(before, after):
    """Return exact added and removed rows between two artifact snapshots."""
    return sorted(set(after) - set(before)), sorted(set(before) - set(after))


def render_snapshot(title: str, properties, symbols, functions, types, provenance: str, baseline=None) -> list[str]:
    """Render one complete PDB snapshot and identify its provenance."""
    baseline = baseline or ([], [], [], [])
    baseline_properties, baseline_symbols, baseline_functions, baseline_types = baseline
    baseline_properties = dict(baseline_properties)
    lines = [f"### {title}", "", "#### Program properties", "", "| Name | Value | Provenance |", "| --- | --- | --- |"]
    lines.extend(
        f"| `{name}` | `{value}` | {('PE loader/default artifact' if baseline_properties.get(name) == value else provenance)} |"
        for name, value in properties
    )
    if not properties:
        lines.append("| _(none)_ | | |")
    lines.extend(["", "#### Symbols", "", "| Name | Address | Source | Provenance |", "| --- | --- | --- | --- |"])
    lines.extend(f"| `{name}` | `{address}` | `{source}` | {('PE loader/default artifact' if row in baseline_symbols else provenance)} |" for row in symbols for name, address, source in (row,))
    if not symbols:
        lines.append("| _(none)_ | | | |")
    lines.extend(["", "#### Functions", "", "| Name | Entry | Provenance |", "| --- | --- | --- |"])
    lines.extend(f"| `{name}` | `{address}` | {('PE loader/default artifact' if row in baseline_functions else provenance)} |" for row in functions for name, address in (row,))
    if not functions:
        lines.append("| _(none)_ | | |")
    lines.extend(["", "#### Data types", "", "| Path | Provenance |", "| --- | --- |"])
    lines.extend(f"| `{name}` | {('PE loader/default artifact' if name in baseline_types else provenance)} |" for name in types)
    if not types:
        lines.append("| _(none)_ | |")
    return lines


def render_set_delta(added, removed, columns: str) -> list[str]:
    """Render added and removed rows while treating a single type path as one field."""
    lines = [f"| Change | {columns} |", f"| --- | {' | '.join('---' for _ in columns.split(' | '))} |"]
    for change, rows in (("Added", added), ("Removed", removed)):
        for row in rows:
            values = (row,) if isinstance(row, str) else row
            lines.append("| " + change + " | " + " | ".join(f"`{value}`" for value in values) + " |")
    if not added and not removed:
        lines.append("| _(none)_ | " + " | ".join("" for _ in columns.split(" | ")) + " |")
    return lines


def render(input_path: Path, pdb_path: Path, enabled: list[str], before, after) -> str:
    """Render before/after Universal evidence and exact PDB deltas."""
    before_properties, before_symbols, before_functions, before_types = before
    after_properties, after_symbols, after_functions, after_types = after
    property_added, property_removed, property_changed = mapping_delta(before_properties, after_properties)
    symbol_added, symbol_removed = set_delta(before_symbols, after_symbols)
    function_added, function_removed = set_delta(before_functions, after_functions)
    type_added, type_removed = set_delta(before_types, after_types)
    delta_count = sum(len(rows) for rows in (property_added, property_removed, property_changed, symbol_added, symbol_removed, function_added, function_removed, type_added, type_removed))
    lines = [
        "# PDB Universal Behavioral Fixture",
        "",
        "> Generated from actual Ghidra program-state snapshots before and after raw PDB Universal analysis.",
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
    lines.extend(["", "## Before target analysis"])
    lines.extend(render_snapshot("PDB artifacts visible before target analysis", before_properties, before_symbols, before_functions, before_types, "PE loader/default artifact"))
    lines.extend(["", "## After target analysis"])
    lines.extend(render_snapshot("PDB artifacts visible after target analysis", after_properties, after_symbols, after_functions, after_types, "PDB analyzer delta", before))
    lines.extend(["", "## Delta", "", "Rows present in the before snapshot are loader/default artifacts. Only rows below are attributed to PDB Universal.", ""])
    lines.extend(["### Property delta", "", "| Change | Name | Before | After |", "| --- | --- | --- | --- |"])
    lines.extend(f"| Added | `{name}` | | `{value}` |" for name, value in property_added)
    lines.extend(f"| Removed | `{name}` | `{value}` | |" for name, value in property_removed)
    lines.extend(f"| Changed | `{name}` | `{old}` | `{new}` |" for name, old, new in property_changed)
    if not property_added and not property_removed and not property_changed:
        lines.append("| _(none)_ | | | |")
    for title, added, removed, columns in (("Symbol delta", symbol_added, symbol_removed, "Name | Address | Source"), ("Function delta", function_added, function_removed, "Name | Entry"), ("Data type delta", type_added, type_removed, "Path")):
        lines.extend(["", f"### {title}", ""])
        lines.extend(render_set_delta(added, removed, columns))
    lines.extend(["", "### Delta conclusion", "", f"- **Exact PDB delta rows:** `{delta_count}`.", "- **PDB matching:** The runner validated PE CodeView identity and PDB age before target analysis."])
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
    sys.path.insert(0, str(fixture_dir.parents[2] / "test_support"))
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
        before = artifacts(program)
        project.analyze(program)
        project.save(program)
        after = artifacts(program)
        output_path.write_text(render(input_path, pdb_path, enabled, before, after), encoding="utf-8", newline="\n")
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
