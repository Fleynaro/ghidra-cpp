#!/usr/bin/env python3
"""Extract the Embedded Media analyzer result from the real Ghidra database."""

from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

ANALYZER_NAME = "Embedded Media"
MEDIA_TYPES = {"GIF", "PNG", "JPEG", "WAVE", "MIDI", "AU", "AIFF"}


def offset(address) -> int:
    """Return an image-base-independent address offset from a Ghidra address."""
    return int(address.getOffset())


def close_project(project, program) -> None:
    """Release the analyzed program and close the underlying temporary project.

    GhidraProject.analyze() owns and finalizes the batch transaction itself, so
    its wrapper close() cannot safely end that already-finalized transaction.
    The saved program is released directly, then the underlying project is
    closed cleanly.
    """
    if program is not None:
        program.release(project)
    underlying = project.getProject()
    if underlying is not None:
        underlying.close()


def address_text(address) -> str:
    """Render a Ghidra address as a stable fixed-width hexadecimal offset."""
    return f"0x{offset(address):016X}"


def configure_analysis(project, program) -> list[str]:
    """Disable boolean analyzers and explicitly enable only Embedded Media."""
    from ghidra.framework.options import OptionType

    options = project.getAnalysisOptions(program)
    for name in options.getOptionNames():
        if options.getType(name) == OptionType.BOOLEAN_TYPE:
            options.setBoolean(name, str(name) == ANALYZER_NAME)
    options.setBoolean("Create Analysis Bookmarks", True)
    enabled = [
        str(name)
        for name in options.getOptionNames()
        if options.getType(name) == OptionType.BOOLEAN_TYPE
        and options.getBoolean(name, False)
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


def report(input_path: Path, enabled, data_rows, bookmarks) -> str:
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
    lines.extend(["", "## Fixture Assertions", "", f"- **Successful media data objects:** `{len(data_rows)}`.", f"- **Analyzer bookmarks:** `{len(bookmarks)}`.", ""])
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
        # BYTE analyzers are normally scheduled by AutoAnalysisManager. Invoke
        # the authoritative analyzer once on the complete loaded set before the
        # scheduled pass so its bookmark option is applied on still-undefined
        # data; this is the same added() contract, not a synthetic data writer.
        from ghidra.app.plugin.core.analysis import EmbeddedMediaAnalyzer
        from ghidra.app.util.importer import MessageLog
        from ghidra.util.task import TaskMonitor
        media_analyzer = EmbeddedMediaAnalyzer()
        media_analyzer.optionsChanged(project.getAnalysisOptions(program), program)
        media_analyzer.added(program, program.getMemory(), TaskMonitor.DUMMY, MessageLog())
        project.analyze(program)
        data_rows = media_rows(program)
        bookmarks = bookmark_rows(program)
        project.save(program)
        output_path.write_text(report(input_path, enabled, data_rows, bookmarks), encoding="utf-8", newline="\n")
        print(f"[+] Wrote {output_path}")
    finally:
        if project is not None:
            close_project(project, program)
        shutil.rmtree(parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
