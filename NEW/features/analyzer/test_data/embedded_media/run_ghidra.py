#!/usr/bin/env python3
"""Extract the Embedded Media analyzer result from the real Ghidra database."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from evidence import render_evidence

ANALYZER_NAME = "Embedded Media"
MEDIA_TYPES = {"GIF", "PNG", "JPEG", "WAVE", "MIDI", "AU"}


def offset(address) -> int:
    """Return an image-base-independent address offset from a Ghidra address."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Close the analyzed program through the GhidraProject lifecycle API."""
    if program is not None:
        project.close(program)
    else:
        project.close()


def address_text(address) -> str:
    """Render a Ghidra address as a stable fixed-width hexadecimal offset."""
    return f"0x{offset(address):016X}"


def configure_analysis(project, program) -> list[str]:
    """Disable boolean analyzers and explicitly enable only Embedded Media."""
    from ghidra.framework.options import OptionType

    analysis_options = project.getAnalysisOptions(program)
    for name in list(analysis_options.getOptionNames()):
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE:
            analysis_options.setBoolean(name, str(name) == ANALYZER_NAME)
    analysis_options.getOptions(ANALYZER_NAME).setBoolean("Create Analysis Bookmarks", True)
    enabled = [
        str(name)
        for name in analysis_options.getOptionNames()
        if analysis_options.getType(name) == OptionType.BOOLEAN_TYPE
        and analysis_options.getBoolean(name, False)
    ]
    analyzers = [name for name in enabled if name == ANALYZER_NAME]
    if analyzers != [ANALYZER_NAME]:
        raise RuntimeError(f"Embedded Media analyzer is not enabled: {sorted(enabled)}")
    return analyzers


def media_rows(program):
    """Extract successful media DataType applications from the listing."""
    rows = []
    for data in program.getListing().getDefinedData(True):
        name = str(data.getDataType().getName())
        if any(name.startswith(prefix) for prefix in MEDIA_TYPES):
            rows.append((offset(data.getAddress()), name, int(data.getLength())))
    return sorted(rows)


def bookmark_rows(program):
    """Extract only bookmarks created by EmbeddedMediaAnalyzer."""
    rows = []
    iterator = program.getBookmarkManager().getBookmarksIterator()
    while iterator.hasNext():
        bookmark = iterator.next()
        if str(bookmark.getCategory()) == ANALYZER_NAME:
            rows.append((offset(bookmark.getAddress()), str(bookmark.getComment())))
    return sorted(rows)


def function_evidence_rows(program):
    """Capture local functions so target-created function changes remain visible."""
    return [
        (address_text(function.getEntryPoint()), str(function.getName()))
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal()
    ]


def option_evidence_rows(project, program):
    """Capture the enabled analyzer and its bookmark option at both evidence phases."""
    analysis_options = project.getAnalysisOptions(program)
    analyzer_options = analysis_options.getOptions(ANALYZER_NAME)
    return [
        (ANALYZER_NAME, str(analysis_options.getBoolean(ANALYZER_NAME, False)).lower()),
        (
            "Create Analysis Bookmarks",
            str(analyzer_options.getBoolean("Create Analysis Bookmarks", False)).lower(),
        ),
    ]


def evidence_snapshot(project, program):
    """Capture target-specific data, functions, bookmarks, and options."""
    data = [
        (address_text(data.getAddress()), f"{data.getDataType().getName()} ({data.getLength()} bytes)")
        for data in program.getListing().getDefinedData(True)
        if any(str(data.getDataType().getName()).startswith(prefix) for prefix in MEDIA_TYPES)
    ]
    bookmarks = [
        (f"0x{address:016X} Embedded Media", comment)
        for address, comment in bookmark_rows(program)
    ]
    return {
        "Data": data,
        "Functions": function_evidence_rows(program),
        "Bookmarks": bookmarks,
        "Options": option_evidence_rows(project, program),
    }


def report(input_path: Path, enabled, data_rows, bookmarks, before, after) -> str:
    """Render the stable analyzer-specific markdown report."""
    lines = [
        "# Embedded Media Behavioral Fixture",
        "",
        "> Generated from the saved MSVC x64 PE program with PyGhidra.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        "",
        "## Analysis Configuration",
        "",
        "| Enabled boolean analyzer |",
        "| --- |",
        *[f"| `{name}` |" for name in enabled],
        "",
        "## Successful Media Data",
        "",
        "| Offset | Data type | Length |",
        "| --- | --- | --- |",
    ]
    for address, name, length in data_rows:
        lines.append(f"| `0x{address:016X}` | `{name}` | `{length}` |")
    lines.extend(["", "## Embedded Media Bookmarks", "", "| Offset | Comment |", "| --- | --- |"])
    for address, comment in bookmarks:
        lines.append(f"| `0x{address:016X}` | `{comment}` |")
    lines.extend(["", *render_evidence(before, after), "## Fixture Assertions", "", f"- **Successful media data objects:** `{len(data_rows)}`.", f"- **Analyzer bookmarks:** `{len(bookmarks)}`.", ""])
    return "\n".join(lines)


def main() -> int:
    """Import, reopen, analyze, save, and extract the fixture program."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "test_embedded_media.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "test_embedded_media.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    parent = Path(tempfile.mkdtemp(prefix="ghidra_embedded_media_"))
    project = None
    program = None
    try:
        project = GhidraProject.createProject(str(parent), "embedded_media", True)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        enabled = configure_analysis(project, program)
        before = evidence_snapshot(project, program)
        # BYTE analyzers are normally scheduled by AutoAnalysisManager. Invoke
        # the authoritative analyzer once on the complete loaded set before the
        # scheduled pass so its bookmark option is applied on still-undefined
        # data; this is the same added() contract, not a synthetic data writer.
        from ghidra.app.plugin.core.analysis import EmbeddedMediaAnalyzer
        from ghidra.app.util.importer import MessageLog
        from ghidra.util.task import TaskMonitor
        media_analyzer = EmbeddedMediaAnalyzer()
        media_analyzer.optionsChanged(
            project.getAnalysisOptions(program).getOptions(ANALYZER_NAME), program
        )
        media_analyzer.added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())
        project.analyze(program)
        after = evidence_snapshot(project, program)
        data_rows = media_rows(program)
        bookmarks = bookmark_rows(program)
        if sum(name.startswith("GIF") for _, name, _ in data_rows) < 2:
            raise RuntimeError(f"The real GIF87 positive case was not created alongside GIF89a: {data_rows}")
        if not data_rows:
            raise RuntimeError("Embedded Media created no supported positive data objects")
        project.save(program)
        output_path.write_text(report(input_path, enabled, data_rows, bookmarks, before, after), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
