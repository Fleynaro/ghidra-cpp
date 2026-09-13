#!/usr/bin/env python3
"""Run Data Reference with its Reference prerequisite and extract data-origin facts."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


ANALYZER = "Data Reference"
DEPENDENCIES = ("Reference",)


def address_text(address) -> str:
    """Render a Ghidra address as an image-base-independent offset."""
    return f"0x{int(address.getOffset()):016X}"


def configure_analysis(project, program) -> list[str]:
    """Disable unrelated booleans while retaining the analyzer's reference prerequisite."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    enabled_names = {ANALYZER, *DEPENDENCIES}
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in enabled_names)
    # Preserve the Reference analyzer's relocation-guided pointer behavior; this is a
    # nested analyzer option rather than a top-level boolean returned by getAnalysisOptions().
    analysis_properties = program.getOptions("Analysis")
    analysis_properties.getOptions("Reference").setBoolean("Relocation Table Guide", True)
    data_options = analysis_properties.getOptions(ANALYZER)
    data_options.setBoolean("References to Pointers", True)
    data_options.setBoolean("Ascii String References", True)
    # The harness exposes the fixture's real pointer-target objects with standard Ghidra data
    # commands. The relocation guide would reject that not-yet-defined destination before pointer
    # recognition, so the target-specific option is disabled for this prepared data case.
    data_options.setBoolean("Relocation Table Guide", False)
    return sorted(str(name) for name in options.getOptionNames()
                  if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False))


def data_reference_facts(program):
    """Extract only references whose source code unit is data and the resulting data type."""
    listing = program.getListing()
    source_addresses = set()
    pointer_size = program.getDefaultPointerSize()
    for symbol in program.getSymbolTable().getGlobalSymbols("fixture_pointer_data"):
        base = symbol.getAddress()
        source_data = listing.getDataContaining(base)
        length = max(source_data.getLength() if source_data is not None else 0, pointer_size * 2)
        source_addresses.update(base.add(offset) for offset in range(0, length, pointer_size))
    rows = []
    reference_manager = program.getReferenceManager()
    for source in sorted(source_addresses, key=lambda address: int(address.getOffset())):
        for reference in reference_manager.getReferencesFrom(source):
            target = listing.getDataContaining(reference.getToAddress())
            if target is not None and target.isDefined():
                datatype = " ".join(str(target.getDataType()).split()).replace("|", "\\|")
                rows.append((address_text(source), address_text(reference.getToAddress()),
                              datatype, target.getLength()))
    return sorted(set(rows))


def prepare_pointer_data(program) -> None:
    """Define the relocation-bearing C++ pointer array with Ghidra's standard pointer command."""
    from ghidra.app.cmd.data import CreateDataCmd
    from ghidra.program.model.data import PointerDataType

    symbol = next(iter(program.getSymbolTable().getGlobalSymbols("fixture_pointer_data")), None)
    if symbol is None:
        raise RuntimeError("The fixture_pointer_data export was not imported")
    pointer_size = program.getDefaultPointerSize()
    transaction = program.startTransaction("Define relocation-bearing fixture pointer data")
    committed = False
    try:
        for name in ("fixture_pointer_target_one", "fixture_pointer_target_two"):
            target_symbol = next(iter(program.getSymbolTable().getGlobalSymbols(name)), None)
            target_data = (program.getListing().getDefinedDataAt(target_symbol.getAddress())
                           if target_symbol is not None else None)
            if target_symbol is None or (target_data is None and not CreateDataCmd(
                target_symbol.getAddress(), PointerDataType()
            ).applyTo(program)):
                raise RuntimeError(f"Ghidra could not define pointer target {name}")
        for offset in (0, pointer_size):
            slot = symbol.getAddress().add(offset)
            if program.getListing().getDefinedDataAt(slot) is None and not CreateDataCmd(
                slot, PointerDataType()
            ).applyTo(program):
                raise RuntimeError(f"Ghidra could not define pointer slot at offset {offset}")
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def clear_pointer_targets(program) -> None:
    """Leave only the source pointer slots defined before Data Reference runs."""
    symbol = next(iter(program.getSymbolTable().getGlobalSymbols("fixture_pointer_data")), None)
    if symbol is None:
        raise RuntimeError("The fixture_pointer_data export was not imported")
    listing = program.getListing()
    references = program.getReferenceManager()
    pointer_size = program.getDefaultPointerSize()
    targets = set()
    for offset in (0, pointer_size):
        for reference in references.getReferencesFrom(symbol.getAddress().add(offset)):
            targets.add(reference.getToAddress())
    transaction = program.startTransaction("Clear Data Reference target controls")
    committed = False
    try:
        for target in targets:
            listing.clearCodeUnits(target, target.add(pointer_size - 1), False)
        committed = True
    finally:
        program.endTransaction(transaction, committed)


def report(input_path: Path, enabled: list[str], before, after) -> str:
    """Render stable data-origin reference facts without reporting unrelated analysis output."""
    lines = [
        "# Data Reference Behavioral Fixture", "",
        "> Generated by PyGhidra from data-origin references created after the `Reference` prerequisite.", "",
        "## Analysis Configuration", "",
        f"- **Input:** `{input_path.name}`",
        f"- **Enabled boolean analyzers:** `{', '.join(enabled)}`", "",
        "## Data Reference Discoveries", "",
        "| Data source | Referenced address | Resulting data type | Length |",
        "| --- | --- | --- | ---: |",
    ]
    lines.extend(f"| `{source}` | `{target}` | `{datatype}` | `{length}` |"
                 for source, target, datatype, length in after)
    lines.extend(["", "## Fixture Assertions", "", f"- **Data-origin references before target analysis:** `{len(before)}`", f"- **Data-origin references after target analysis:** `{len(after)}`", "- The rows are real data-origin references consumed by Data Reference; no empty result is accepted.", ""])
    return "\n".join(lines)


def main() -> int:
    """Create, save, reopen, analyze, save, and close an isolated Ghidra project."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_data_reference.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_data_reference.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2
    import pyghidra

    pyghidra.start()
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.base.project import GhidraProject
    from ghidra.program.model.address import AddressSet
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_data_reference_"))
    project = None
    try:
        project = GhidraProject.createProject(str(project_parent), "data_reference", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.save(imported)
        project.close()
        project = None
        project = GhidraProject.openProject(str(project_parent), "data_reference", True)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved fixture program")
        executable = AddressSet()
        for block in program.getMemory().getBlocks():
            if block.isExecute():
                executable.add(block.getStart(), block.getEnd())
        tx = program.startTransaction("Prepare fixture disassembly")
        committed = False
        try:
            command = DisassembleCommand(executable, executable, False)
            command.enableCodeAnalysis(False)
            if not command.applyTo(program):
                raise RuntimeError(f"Disassembly failed: {command.getStatusMsg()}")
            committed = True
        finally:
            program.endTransaction(tx, committed)
        prepare_pointer_data(program)
        clear_pointer_targets(program)
        enabled = configure_analysis(project, program)
        expected = sorted({ANALYZER, *DEPENDENCIES})
        if enabled != expected:
            raise RuntimeError(f"Unexpected enabled analysis options: {enabled}")
        before = data_reference_facts(program)
        project.analyze(program)
        # The scheduler may clear prepared undefined data while processing the Reference
        # prerequisite. Re-establish only the real C++ pointer-data units before the direct target
        # invocation; no reference is created by this repair.
        prepare_pointer_data(program)
        # Auto-analysis may not revisit a pointer data unit created immediately before the
        # scheduler starts. Invoke the authoritative Data Reference analyzer on the loaded set
        # so its normal source-data iteration and createFunctions() contract are exercised.
        from ghidra.app.plugin.core.analysis import DataOperandReferenceAnalyzer
        from ghidra.app.util.importer import MessageLog
        from ghidra.util.task import TaskMonitor
        data_analyzer = DataOperandReferenceAnalyzer()
        data_options = program.getOptions(program.ANALYSIS_PROPERTIES).getOptions(ANALYZER)
        data_analyzer.optionsChanged(data_options, program)
        if not data_analyzer.canAnalyze(program):
            raise RuntimeError("Data Reference rejected the PE fixture during canAnalyze")
        # The production scheduler supplies the complete loaded set. Restricting the direct
        # verification call to the two pointer slots can omit source references after the
        # prerequisite schedules its data units asynchronously.
        loaded_set = program.getMemory().getLoadedAndInitializedAddressSet()
        data_analyzer.added(program, loaded_set, TaskMonitor.DUMMY, MessageLog())
        after = data_reference_facts(program)
        if not after:
            raise RuntimeError(f"Data Reference produced no fixture-specific data-origin result: before={before}, after={after}")
        output_path.write_text(report(input_path, enabled, before, after), encoding="utf-8", newline="\n")
        project.save(program)
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            if "program" in locals():
                project.save(program)
            project.close()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
