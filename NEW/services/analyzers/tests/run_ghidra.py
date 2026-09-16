#!/usr/bin/env python3
"""Generate one bounded Ghidra oracle for the complete analyzer fixture."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path


# The list mirrors the aggregate native registry. Missing names are reported as
# unavailable because Ghidra distributions differ in optional feature modules.
WANTED_ANALYZERS = (
    "Aggressive Instruction Finder",
    "Apply Data Archives",
    "ASCII Strings",
    "Call Convention ID",
    "Call-Fixup Installer",
    "Constant Propagation",
    "Condense Filler Bytes",
    "Create Address Tables",
    "Data Reference",
    "Decompiler Parameter ID",
    "Decompiler Switch Analysis",
    "Demangler Microsoft",
    "Disassemble Entry Points",
    "Embedded Media",
    "External Entry References",
    "Function ID",
    "Function Start Pre Search",
    "Function Start Search",
    "Function Start Search In Functions",
    "Function Start Search After Code",
    "Function Start Search After Data",
    "Non-Returning Functions - Discovered",
    "Non-Returning Functions - Known",
    "PDB MSDIA",
    "PDB Universal",
    "Reference",
    "Scalar Operand References",
    "Shared Return Calls",
    "Stack",
    "Subroutine References",
    "Variadic Function Signature Override",
    "WindowsPE x86 Propagate External Parameters",
    "WindowsResourceReference",
    "x86 Constant Reference Analyzer",
)

MAX_TOTAL_CHARS = 240_000
MAX_FUNCTIONS = 120
MAX_INSTRUCTIONS_PER_FUNCTION = 24
MAX_REFERENCES_PER_FUNCTION = 20
MAX_STRINGS = 100
MAX_DATA = 100
MAX_SYMBOLS = 120
MAX_TYPES = 80
MAX_SECTION_LINES = 240


def address_text(address) -> str:
    """Render a Ghidra address as a stable fixed-width hexadecimal value."""
    return f"0x{int(address.getOffset()):016X}"


def markdown_text(value: object) -> str:
    """Keep arbitrary Ghidra text inside one bounded Markdown table cell."""
    return str(value).replace("|", r"\|").replace("\r", "").replace("\n", "<br>")


class BoundedReport:
    """Accumulates deterministic Markdown while enforcing global and section limits."""

    def __init__(self) -> None:
        self.lines: list[str] = []
        self.characters = 0
        self.truncated = False

    def add(self, line: str = "") -> None:
        """Append one line unless the explicit total report budget is exhausted."""
        if self.truncated:
            return
        rendered = line + "\n"
        rendered_size = len(rendered.encode("utf-8"))
        # Reserve the final newline added by write_report so the serialized
        # file, not merely the in-memory line sum, stays under the contract.
        budget = MAX_TOTAL_CHARS - 1
        marker = "[TRUNCATED: maximum total report size reached]\n"
        marker_size = len(marker.encode("utf-8"))
        if self.characters + rendered_size > budget:
            if self.characters + marker_size <= budget:
                self.lines.append(marker.rstrip("\n"))
                self.characters += marker_size
            self.truncated = True
            return
        self.lines.append(line)
        self.characters += rendered_size

    def section(self, title: str) -> None:
        """Start a named report section."""
        self.add("")
        self.add(f"## {title}")
        self.add("")


def configure_analysis(project, program) -> tuple[list[str], list[str]]:
    """Enable every matching builtin Ghidra analyzer and record unavailable names."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    available = {str(name) for name in options.getOptionNames()}
    wanted = set(WANTED_ANALYZERS)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) in wanted)
    enabled = sorted(
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE and options.getBoolean(name, False)
    )
    missing = sorted(wanted - available)
    return enabled, missing


def body_ranges(function) -> str:
    """Render the bounded function body address ranges."""
    ranges = []
    for address_range in function.getBody().getAddressRanges():
        ranges.append(
            f"{address_text(address_range.getMinAddress())}-{address_text(address_range.getMaxAddress())}"
        )
    return ", ".join(sorted(ranges))


def function_signature(function) -> str:
    """Extract signature, return type, calling convention, and parameter names."""
    try:
        signature = str(function.getSignature())
    except Exception as error:  # pragma: no cover - provider-specific Java API
        signature = f"<unavailable: {error}>"
    try:
        return_type = str(function.getReturnType())
    except Exception:
        return_type = "<unavailable>"
    try:
        convention = str(function.getCallingConventionName())
    except Exception:
        convention = "<unavailable>"
    parameters = []
    try:
        for parameter in function.getParameters():
            parameters.append(f"{parameter.getName()}:{parameter.getDataType()}")
    except Exception as error:  # pragma: no cover - provider-specific Java API
        parameters.append(f"<unavailable: {error}>")
    return f"{signature}; return={return_type}; convention={convention}; parameters={parameters}"


def stack_information(function) -> str:
    """Extract frame sizes and a bounded list of local variables."""
    frame_text = "<unavailable>"
    try:
        frame = function.getStackFrame()
        frame_text = (
            f"frame={frame.getFrameSize()}, local={frame.getLocalSize()}, parameters={function.getParameterCount()}, "
        )
    except Exception as error:  # pragma: no cover - external provider boundary
        frame_text = f"<unavailable: {error}>"
    variables = []
    try:
        for variable in function.getAllVariables():
            variables.append(f"{variable.getName()}:{variable.getDataType()}:{variable.getVariableStorage()}")
    except Exception as error:  # pragma: no cover - external provider boundary
        variables.append(f"<unavailable: {error}>")
    return f"{frame_text} variables={variables[:MAX_REFERENCES_PER_FUNCTION]}"


def function_rows(program) -> list[object]:
    """Return deterministic local functions, retaining a bounded tail marker later."""
    return sorted(
        [function for function in program.getFunctionManager().getFunctions(True) if not function.isExternal()],
        key=lambda function: int(function.getEntryPoint().getOffset()),
    )


def render_functions(report: BoundedReport, program, functions: list[object]) -> None:
    """Render function signatures, frames, instructions, p-code, and local references."""
    report.section("Functions")
    report.add(f"Observed local functions: `{len(functions)}`; maximum rendered: `{MAX_FUNCTIONS}`.")
    listing = program.getListing()
    reference_manager = program.getReferenceManager()
    for index, function in enumerate(functions[:MAX_FUNCTIONS]):
        report.add(
            f"### {index + 1}. `{function.getName()}` at `{address_text(function.getEntryPoint())}`"
        )
        report.add(f"- Body: `{body_ranges(function)}`")
        report.add(f"- Comment: `{markdown_text(function.getComment() or '')}`")
        report.add(f"- Signature: `{markdown_text(function_signature(function))}`")
        report.add(f"- Stack frame: `{markdown_text(stack_information(function))}`")
        report.add("- Instructions:")
        instructions = listing.getInstructions(function.getBody(), True)
        instruction_count = 0
        while instructions.hasNext() and instruction_count < MAX_INSTRUCTIONS_PER_FUNCTION:
            instruction = instructions.next()
            pcode = []
            try:
                pcode = [str(op) for op in instruction.getPcode()]
            except Exception:
                pcode = ["<p-code unavailable>"]
            report.add(
                f"  - `{address_text(instruction.getAddress())}` `{instruction}` "
                f"flow=`{instruction.getFlowType()}` pcode=`{markdown_text(pcode)}`"
            )
            instruction_count += 1
        if instructions.hasNext():
            report.add(f"  - [TRUNCATED: maximum `{MAX_INSTRUCTIONS_PER_FUNCTION}` instructions]")
        report.add("- References:")
        references = []
        all_instructions = listing.getInstructions(function.getBody(), True)
        while all_instructions.hasNext():
            instruction = all_instructions.next()
            for reference in reference_manager.getReferencesFrom(instruction.getAddress()):
                references.append(reference)
        for reference in references[:MAX_REFERENCES_PER_FUNCTION]:
            report.add(
                f"  - `{address_text(reference.getFromAddress())}` -> `{address_text(reference.getToAddress())}` "
                f"type=`{reference.getReferenceType()}` source=`{reference.getSource()}`"
            )
        if len(references) > MAX_REFERENCES_PER_FUNCTION:
            report.add(f"  - [TRUNCATED: maximum `{MAX_REFERENCES_PER_FUNCTION}` references]")
        report.add("- Callers:")
        callers = list(reference_manager.getReferencesTo(function.getEntryPoint()))
        for reference in callers[:MAX_REFERENCES_PER_FUNCTION]:
            report.add(
                f"  - `{address_text(reference.getFromAddress())}` -> `{address_text(reference.getToAddress())}` "
                f"type=`{reference.getReferenceType()}`"
            )
        if len(callers) > MAX_REFERENCES_PER_FUNCTION:
            report.add(f"  - [TRUNCATED: maximum `{MAX_REFERENCES_PER_FUNCTION}` callers]")
    if len(functions) > MAX_FUNCTIONS:
        report.add(f"[TRUNCATED: `{len(functions) - MAX_FUNCTIONS}` additional functions]")


def render_strings_and_data(report: BoundedReport, program) -> None:
    """Render bounded strings, defined data, globals, and their references."""
    listing = program.getListing()
    report.section("Strings And Data")
    report.add("| Address | Length | Type | Value |")
    report.add("| --- | ---: | --- | --- |")
    strings = []
    data = listing.getDefinedData(True)
    while data.hasNext():
        item = data.next()
        data_type = str(item.getDataType())
        value = ""
        try:
            value = str(item.getValue())
        except Exception:
            value = "<unavailable>"
        row = (int(item.getAddress().getOffset()), int(item.getLength()), data_type, value)
        if "string" in data_type.lower() or "char" in data_type.lower():
            strings.append(row)
    for row in sorted(strings)[:MAX_STRINGS]:
        report.add(f"| `{row[0]:016X}` | `{row[1]}` | `{markdown_text(row[2])}` | `{markdown_text(row[3])}` |")
    if len(strings) > MAX_STRINGS:
        report.add(f"| `(truncated)` | `{len(strings) - MAX_STRINGS}` | `strings omitted` | `limit reached` |")
    report.add("")
    report.add(f"Defined data entries observed: `{listing.getNumDefinedData()}`; maximum rendered: `{MAX_DATA}`.")
    report.add("| Address | Length | Type | Value |")
    report.add("| --- | ---: | --- | --- |")
    data = listing.getDefinedData(True)
    count = 0
    while data.hasNext() and count < MAX_DATA:
        item = data.next()
        try:
            value = str(item.getValue())
        except Exception:
            value = "<unavailable>"
        report.add(
            f"| `{address_text(item.getAddress())}` | `{item.getLength()}` | "
            f"`{markdown_text(item.getDataType())}` | `{markdown_text(value)}` |"
        )
        count += 1
    if data.hasNext():
        report.add("| `(truncated)` | | `data omitted` | `limit reached` |")


def render_symbols_and_externals(report: BoundedReport, program) -> None:
    """Render bounded local symbols, imports, external functions, and data types."""
    report.section("Symbols And External References")
    symbol_rows = []
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        symbol_rows.append((int(symbol.getAddress().getOffset()), str(symbol.getName()), str(symbol.getSymbolType())))
    report.add(f"Symbols observed: `{len(symbol_rows)}`; maximum rendered: `{MAX_SYMBOLS}`.")
    for address, name, kind in sorted(symbol_rows)[:MAX_SYMBOLS]:
        report.add(f"- `{address:016X}` `{markdown_text(name)}` kind=`{kind}`")
    if len(symbol_rows) > MAX_SYMBOLS:
        report.add(f"- [TRUNCATED: `{len(symbol_rows) - MAX_SYMBOLS}` symbols]")
    report.add("")
    report.add("### Imported and external functions")
    for function in list(program.getFunctionManager().getExternalFunctions())[:MAX_DATA]:
        report.add(f"- `{function.getName()}` at `{address_text(function.getEntryPoint())}` signature=`{function.getSignature()}`")
    report.add("")
    report.add("### Data types")
    data_types = list(program.getDataTypeManager().getAllDataTypes())
    for data_type in data_types[:MAX_TYPES]:
        report.add(f"- `{markdown_text(data_type.getPathName())}` size=`{data_type.getLength()}`")
    if len(data_types) > MAX_TYPES:
        report.add(f"- [TRUNCATED: `{len(data_types) - MAX_TYPES}` data types]")


def render_global_artifacts(report: BoundedReport, program) -> None:
    """Render switches, resources, comments, bookmarks, and analyzer artifacts."""
    report.section("Analyzer Artifacts")
    listing = program.getListing()
    report.add("### Switch and jump-table evidence")
    instructions = listing.getInstructions(program.getMemory().getExecuteSet(), True)
    switch_count = 0
    while instructions.hasNext() and switch_count < MAX_SECTION_LINES:
        instruction = instructions.next()
        if instruction.getFlowType().isJump() or instruction.getFlowType().isComputed():
            report.add(f"- `{address_text(instruction.getAddress())}` flow=`{instruction.getFlowType()}` flows=`{list(instruction.getFlows())}`")
            switch_count += 1
    report.add(f"- Rendered computed/control-flow instructions: `{switch_count}`.")
    report.add("")
    report.add("### Resources and media")
    resource_block = program.getMemory().getBlock(".rsrc")
    if resource_block is None:
        report.add("- Resource directory: `.rsrc block not present`")
    else:
        report.add(
            f"- Resource directory: `.rsrc {address_text(resource_block.getStart())}-"
            f"{address_text(resource_block.getEnd())}`"
        )
    report.add("- Embedded media and resource payloads are represented by defined data/type rows above when the installed analyzers recognize them.")
    report.add("")
    report.add("### Comments and bookmarks")
    bookmark_manager = program.getBookmarkManager()
    bookmarks = bookmark_manager.getBookmarksIterator()
    count = 0
    while bookmarks.hasNext() and count < MAX_SECTION_LINES:
        bookmark = bookmarks.next()
        report.add(
            f"- `{address_text(bookmark.getAddress())}` category=`{bookmark.getCategory()}` "
            f"type=`{bookmark.getTypeString()}` comment=`{markdown_text(bookmark.getComment())}`"
        )
        count += 1
    if bookmarks.hasNext():
        report.add("- [TRUNCATED: bookmark section line limit reached]")


def write_report(output_path: Path, input_path: Path, enabled: list[str], missing: list[str], before_count: int, program) -> None:
    """Write the bounded complete-program oracle after full Ghidra analysis."""
    report = BoundedReport()
    functions = function_rows(program)
    report.add("# All Analyzer Integration Behavioral Oracle")
    report.add("")
    report.add("> Generated automatically with PyGhidra for development comparison only.")
    report.add("> The native Google Test does not read or parse this Markdown file.")
    report.add("")
    report.add("## Input And Limits")
    report.add("")
    report.add(f"- File: `{input_path.name}`")
    report.add(f"- Size: `{input_path.stat().st_size}` bytes")
    report.add(f"- Functions before analysis: `{before_count}`; after: `{len(functions)}`")
    report.add(f"- Maximum total characters: `{MAX_TOTAL_CHARS}`")
    report.add(f"- Maximum functions: `{MAX_FUNCTIONS}`")
    report.add(f"- Maximum instructions/function: `{MAX_INSTRUCTIONS_PER_FUNCTION}`")
    report.add(f"- Maximum references/function: `{MAX_REFERENCES_PER_FUNCTION}`")
    report.add("")
    report.add("## Analysis Configuration")
    report.add("")
    report.add("### Enabled options")
    for name in enabled:
        report.add(f"- `{name}`")
    report.add("")
    report.add("### Requested but unavailable in this Ghidra installation")
    for name in missing or ["None"]:
        report.add(f"- `{name}`")

    render_functions(report, program, functions)
    render_strings_and_data(report, program)
    render_symbols_and_externals(report, program)
    render_global_artifacts(report, program)
    report.section("Truncation Status")
    report.add(f"- Report truncated: `{str(report.truncated).lower()}`")
    report.add(f"- Rendered UTF-8 bytes before final serialization: `{report.characters}`")
    output_path.write_text("\n".join(report.lines) + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    """Import, configure, analyze, summarize, save, and close one temporary project."""
    fixture_dir = Path(__file__).resolve().parent / "data"
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_analyzers_integration.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_analyzers_integration.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_analyzers_integration_"))
    project = None
    program = None
    script_host_acquired = False
    try:
        # WindowsResourceReference delegates to a GhidraScript module. The
        # standalone PyGhidra host needs the same bundle registry lifetime as
        # the focused resource workflow before the aggregate analysis starts.
        from ghidra.app.script import GhidraScriptUtil

        GhidraScriptUtil.acquireBundleHostReference()
        script_host_acquired = True
        project = GhidraProject.createProject(str(project_parent), "analyzers_integration", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the integration fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the saved integration fixture")
        before_count = len(function_rows(program))
        enabled, missing = configure_analysis(project, program)
        project.analyze(program)
        write_report(output_path, input_path, enabled, missing, before_count, program)
        project.save(program)
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            if program is not None:
                project.close(program)
            project.close()
        if script_host_acquired:
            GhidraScriptUtil.releaseBundleHostReference()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
