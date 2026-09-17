#!/usr/bin/env python3
"""Generate the Ghidra BSim cosine reference for the existing PE fixture."""

from __future__ import annotations

import math
import shutil
import sys
import tempfile
from pathlib import Path


SIGNATURE_SETTINGS = 0x49
TIMEOUT_SECONDS = 60

# These names are exported by bsim_fixture.cpp.  The list is deliberately
# explicit so a change to the fixture cannot silently change the differential
# oracle's function set or pair order.
SELECTED_FUNCTIONS = (
    "sort_insertion_ascending",
    "sort_selection_ascending",
    "find_first_linear",
    "find_first_reverse",
    "maximum_scan",
    "maximum_pairwise",
    "absolute_branch",
    "absolute_mask",
    "checksum_forward",
    "checksum_reverse",
    "hash_fnv_indexed",
    "hash_fnv_pointer",
    "population_count",
    "rotate_left32",
    "gcd_unsigned",
    "byte_swap32",
    "dot_product",
)

# The first groups exercise semantically related implementations.  The final
# entries are intentionally unrelated controls that should have lower scores.
PAIR_NAMES = (
    ("sort_insertion_ascending", "sort_selection_ascending"),
    ("find_first_linear", "find_first_reverse"),
    ("maximum_scan", "maximum_pairwise"),
    ("absolute_branch", "absolute_mask"),
    ("checksum_forward", "checksum_reverse"),
    ("hash_fnv_indexed", "hash_fnv_pointer"),
    ("sort_insertion_ascending", "population_count"),
    ("find_first_linear", "rotate_left32"),
    ("maximum_scan", "byte_swap32"),
    ("absolute_branch", "dot_product"),
    ("checksum_forward", "gcd_unsigned"),
    ("hash_fnv_indexed", "population_count"),
)


def sorted_functions(program):
    """Return non-external functions in stable entry-address order."""
    functions = [
        function
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal()
    ]
    return sorted(functions, key=lambda function: int(function.getEntryPoint().getOffset()))


def load_vector_factory(program):
    """Load the architecture-selected Ghidra BSim weight resource."""
    from generic.lsh.vector import WeightedLSHCosineVectorFactory
    from ghidra.features.bsim.query import GenSignatures
    from ghidra.util.xml import SpecXmlUtils
    from ghidra.xml import NonThreadedXmlPullParserImpl

    weights_file = GenSignatures.getWeightsFile(program.getLanguageID(), program.getLanguageID())
    if weights_file is None:
        raise RuntimeError(f"No Ghidra BSim weights for {program.getLanguageID()}")
    vector_factory = WeightedLSHCosineVectorFactory()
    stream = weights_file.getInputStream()
    try:
        parser = NonThreadedXmlPullParserImpl(
            stream,
            "BSim differential weights",
            SpecXmlUtils.getXmlHandler(),
            False,
        )
        vector_factory.readWeights(parser)
    finally:
        stream.close()
    if vector_factory.getSettings() != SIGNATURE_SETTINGS:
        raise RuntimeError(
            f"Unexpected Ghidra BSim settings: 0x{vector_factory.getSettings():X}"
        )
    return vector_factory, str(weights_file.getName())


def generate_vectors(program, functions):
    """Generate raw signatures and weighted vectors through the real Ghidra APIs."""
    from ghidra.app.decompiler import DecompInterface
    from ghidra.util.task import TaskMonitor

    vector_factory, weights_name = load_vector_factory(program)
    decomp = DecompInterface()
    decomp.setSignatureSettings(SIGNATURE_SETTINGS)
    decomp.toggleSyntaxTree(False)
    if not decomp.openProgram(program):
        raise RuntimeError(f"Ghidra decompiler failed to open program: {decomp.getLastMessage()}")

    vectors = {}
    feature_counts = {}
    try:
        for name in SELECTED_FUNCTIONS:
            function = functions.get(name)
            if function is None:
                raise RuntimeError(f"Selected function is missing from Ghidra analysis: {name}")
            result = decomp.generateSignatures(function, True, TIMEOUT_SECONDS, TaskMonitor.DUMMY)
            if result is None:
                raise RuntimeError(
                    f"Ghidra signature generation failed for {name}: {decomp.getLastMessage()}"
                )
            if len(result.features) == 0:
                raise RuntimeError(f"Ghidra generated no BSim features for {name}")
            feature_counts[name] = len(result.features)
            vectors[name] = vector_factory.buildVector(result.features)
    finally:
        decomp.closeProgram()
        decomp.dispose()
    return vectors, feature_counts, weights_name


def cosine(vectors, first_name: str, second_name: str) -> float:
    """Calculate the weighted Ghidra BSim cosine for one selected pair."""
    from generic.lsh.vector import VectorCompare

    comparison = VectorCompare()
    value = vectors[first_name].compare(vectors[second_name], comparison)
    value = float(value)
    if not math.isfinite(value):
        raise RuntimeError(f"Ghidra produced a non-finite cosine for {first_name} vs {second_name}")
    return value


def markdown(
    input_path: Path,
    program,
    functions,
    feature_counts: dict[str, int],
    vectors,
    weights_name: str,
) -> str:
    """Render compact scalar BSim evidence without exposing raw feature arrays."""
    lines = [
        "# BSim Differential Reference",
        "",
        "> Generated automatically from the existing PE fixture through PyGhidra.",
        "> Raw feature hashes and complete vectors are intentionally omitted.",
        "",
        "## Input",
        "",
        f"- **File:** `{input_path.name}`",
        f"- **File size:** `{input_path.stat().st_size}` bytes",
        f"- **Language:** `{program.getLanguageID()}`",
        f"- **Signature settings:** `0x{SIGNATURE_SETTINGS:X}`",
        f"- **Weights:** `{weights_name}`",
        "",
        "## Selected Functions",
        "",
        "| Function | Entry address | Body end | Feature count |",
        "| --- | ---: | ---: | ---: |",
    ]
    for name in SELECTED_FUNCTIONS:
        function = functions[name]
        entry = int(function.getEntryPoint().getOffset())
        body_end = int(function.getBody().getMaxAddress().getOffset())
        lines.append(f"| `{name}` | `0x{entry:016X}` | `0x{body_end:016X}` | {feature_counts[name]} |")

    lines.extend(
        [
            "",
            "## Pairwise Cosine",
            "",
            "| Function A | Function B | Ghidra Cosine |",
            "| --- | --- | ---: |",
        ]
    )
    for first_name, second_name in PAIR_NAMES:
        lines.append(
            f"| `{first_name}` | `{second_name}` | {cosine(vectors, first_name, second_name):.12f} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    """Import, analyze, generate BSim signatures, and write the deterministic report."""
    fixture_dir = Path(__file__).resolve().parent
    input_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else fixture_dir / "bsim_fixture.exe"
    output_path = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else fixture_dir / "bsim_fixture.md"
    if not input_path.is_file():
        print(f"Input file does not exist: {input_path}", file=sys.stderr)
        return 2

    import pyghidra

    pyghidra.start()
    from ghidra.base.project import GhidraProject
    from java.io import File

    project_parent = Path(tempfile.mkdtemp(prefix="ghidra_bsim_differential_"))
    project = None
    program = None
    script_host_acquired = False
    try:
        from ghidra.app.script import GhidraScriptUtil

        GhidraScriptUtil.acquireBundleHostReference()
        script_host_acquired = True
        project = GhidraProject.createProject(str(project_parent), "bsim_differential", False)
        imported = project.importProgram(File(str(input_path)))
        if imported is None:
            raise RuntimeError("Ghidra failed to import the BSim fixture executable")
        project.saveAs(imported, "/", input_path.name, True)
        project.close(imported)
        program = project.openProgram("/", input_path.name, False)
        if program is None:
            raise RuntimeError("Ghidra failed to reopen the BSim fixture program")
        project.analyze(program)

        functions = {str(function.getName()): function for function in sorted_functions(program)}
        selected = {name: functions.get(name) for name in SELECTED_FUNCTIONS}
        if any(function is None for function in selected.values()):
            missing = [name for name, function in selected.items() if function is None]
            raise RuntimeError(f"Ghidra analysis did not expose selected functions: {missing}")
        vectors, feature_counts, weights_name = generate_vectors(program, selected)
        report = markdown(input_path, program, selected, feature_counts, vectors, weights_name)
        project.save(program)
        output_path.write_text(report, encoding="utf-8", newline="\n")
        print(f"[+] Analyzed {input_path}")
        print(f"[+] Enumerated {len(functions)} non-external functions")
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
