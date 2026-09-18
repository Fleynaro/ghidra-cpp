#!/usr/bin/env python3
"""Run the Ghidra BSim retrieval experiment for the combined research PE files."""

from __future__ import annotations

import math
import shutil
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SIGNATURE_SETTINGS = 0x49
TIMEOUT_SECONDS = 60
DATABASE_PREFIX = "db_"
INPUT_PREFIX = "input_"
BSIM_MODEL = "Ghidra BSim x64"


def load_vector_factory(program):
    """Load the architecture-selected Ghidra weighted BSim vector factory."""
    from generic.lsh.vector import WeightedLSHCosineVectorFactory
    from ghidra.features.bsim.query import GenSignatures
    from ghidra.util.xml import SpecXmlUtils
    from ghidra.xml import NonThreadedXmlPullParserImpl

    weights_file = GenSignatures.getWeightsFile(program.getLanguageID(), program.getLanguageID())
    if weights_file is None:
        raise RuntimeError(f"No Ghidra BSim weights for {program.getLanguageID()}")
    factory = WeightedLSHCosineVectorFactory()
    stream = weights_file.getInputStream()
    try:
        parser = NonThreadedXmlPullParserImpl(
            stream,
            "FUNC_VECTOR_RESEARCH BSim weights",
            SpecXmlUtils.getXmlHandler(),
            False,
        )
        factory.readWeights(parser)
    finally:
        stream.close()
    if factory.getSettings() != SIGNATURE_SETTINGS:
        raise RuntimeError(f"Unexpected Ghidra BSim settings: 0x{factory.getSettings():X}")
    return factory, str(weights_file.getName())


def open_program(project, input_path: Path):
    """Import, save, reopen, and analyze one combined PE fixture."""
    from ghidra.base.project import GhidraProject
    from java.io import File

    imported = project.importProgram(File(str(input_path)))
    if imported is None:
        raise RuntimeError(f"Ghidra failed to import {input_path}")
    project.saveAs(imported, "/", input_path.name, True)
    project.close(imported)
    program = project.openProgram("/", input_path.name, False)
    if program is None:
        raise RuntimeError(f"Ghidra failed to reopen {input_path}")
    project.analyze(program)
    return program


def enumerate_exported_functions(program, prefix: str) -> dict[str, object]:
    """Return analyzed non-external functions whose exported names use prefix."""
    functions = {
        str(function.getName()): function
        for function in program.getFunctionManager().getFunctions(True)
        if not function.isExternal() and str(function.getName()).startswith(prefix)
    }
    if not functions:
        raise RuntimeError(f"No analyzed functions with prefix {prefix!r} in {program.getName()}")
    return dict(sorted(functions.items()))


def generate_vectors(program, functions: dict[str, object]):
    """Generate real Ghidra SignatureResult features and weighted BSim vectors."""
    from ghidra.app.decompiler import DecompInterface
    from ghidra.util.task import TaskMonitor

    factory, weights_name = load_vector_factory(program)
    decomp = DecompInterface()
    decomp.setSignatureSettings(SIGNATURE_SETTINGS)
    decomp.toggleSyntaxTree(False)
    if not decomp.openProgram(program):
        raise RuntimeError(f"Ghidra decompiler failed to open {program.getName()}: {decomp.getLastMessage()}")

    vectors: dict[str, object] = {}
    feature_counts: dict[str, int] = {}
    try:
        for name, function in functions.items():
            result = decomp.generateSignatures(function, True, TIMEOUT_SECONDS, TaskMonitor.DUMMY)
            if result is None:
                raise RuntimeError(f"Signature generation failed for {name}: {decomp.getLastMessage()}")
            if len(result.features) == 0:
                raise RuntimeError(f"Ghidra generated no BSim features for {name}")
            vectors[name] = factory.buildVector(result.features)
            feature_counts[name] = len(result.features)
    finally:
        decomp.closeProgram()
        decomp.dispose()
    return vectors, feature_counts, weights_name


def cosine(vectors, first: str, second: str) -> float:
    """Calculate one Ghidra weighted BSim cosine similarity."""
    from generic.lsh.vector import VectorCompare

    comparison = VectorCompare()
    value = float(vectors[first].compare(vectors[second], comparison))
    if not math.isfinite(value):
        raise RuntimeError(f"Non-finite BSim cosine for {first} versus {second}")
    return value


def display_name(name: str, prefix: str) -> str:
    """Remove the generated PE symbol prefix for report-compatible names."""
    if not name.startswith(prefix):
        raise ValueError(f"Unexpected generated symbol {name!r} for prefix {prefix!r}")
    return name[len(prefix):]


def expected_function(input_name: str) -> str:
    """Read the expected database stem from the established input filename convention."""
    parts = input_name.split("_")
    if len(parts) < 3 or not parts[0].isdigit():
        raise ValueError(f"Input symbol does not match <number>_<expected>_<description>: {input_name}")
    return parts[1]


def ranking(query_name: str, input_vectors, database_vectors, database_names: list[str]):
    """Return all database functions ranked by descending weighted cosine."""
    rows = [
        (display_name(name, DATABASE_PREFIX), cosine({**database_vectors, query_name: input_vectors[query_name]}, query_name, name))
        for name in database_names
    ]
    rows.sort(key=lambda item: (-item[1], item[0]))
    return [(rank, name, score) for rank, (name, score) in enumerate(rows, start=1)]


def format_table(rows: list[tuple[int, str, float]]) -> str:
    """Render the same full ranking table used by the embedding reports."""
    name_width = max(17, *(len(name) for _, name, _ in rows))
    lines = [
        f"| Rank | {'Database Function':<{name_width}} | Similarity |",
        f"| ---: | {'-' * name_width} | ---------: |",
    ]
    for rank, name, score in rows:
        lines.append(f"| {rank:4d} | {name:<{name_width}} | {score * 100:10.2f}% |")
    return "\n".join(lines)


def write_report(path: Path, expected: str, rows: list[tuple[int, str, float]], feature_count: int) -> None:
    """Write one input retrieval report in the established embedding-report format."""
    top_name = rows[0][1]
    top_score = rows[0][2]
    top_gap = rows[0][2] - rows[1][2]
    expected_rank = next((rank for rank, name, _ in rows if name == expected), None) if expected != "NONE" else None
    correctness = "Yes" if expected != "NONE" and top_name == expected else "No"
    report = "\n".join(
        [
            f"# {path.stem}",
            "",
            f"- Model: `{BSIM_MODEL}`",
            f"- Query example: `{path.stem}`",
            "- Query instruction: `(none; native Ghidra BSim signature)`",
            f"- Expected function: `{expected}`",
            f"- Expected rank: `{expected_rank if expected_rank is not None else 'N/A'}`",
            f"- Top match: `{top_name}` ({top_score * 100:.2f}%)",
            f"- Top-1 vs top-2 gap: `{top_gap * 100:.2f}` percentage points",
            f"- Correct top-1 match: `{correctness}`",
            f"- Query BSim feature count: `{feature_count}`",
            "",
            "Cosine similarity is multiplied by 100 for presentation; it is not a probability.",
            "",
            format_table(rows),
            "",
        ]
    )
    path.write_text(report, encoding="utf-8", newline="\n")


def write_result(path: Path, summaries: list[dict[str, object]]) -> None:
    """Write the aggregate RESULT_bsim.md using the neural experiment's metrics."""
    ordered = sorted(summaries, key=lambda item: str(item["example"]))
    lines = [
        "# Vector Function ID Results: bsim",
        "",
        f"Model: `{BSIM_MODEL}`",
        "Query instruction: `(none; native Ghidra BSim signature)`",
        "Passage instruction: `(none; native Ghidra BSim signature)`",
        "",
        "| Example | Expected Function | Top Match | Similarity | Margin (pp) | Expected Rank | Correct? |",
        "| ------- | ----------------- | --------- | ---------: | ----------: | ------------: | :------: |",
    ]
    for item in ordered:
        expected_rank = item["expected_rank"] if item["expected_rank"] is not None else "N/A"
        lines.append(
            f"| {item['example']} | {item['expected']} | {item['top_match']} | "
            f"{float(item['similarity']) * 100:.2f}% | {float(item['margin']) * 100:.2f} | {expected_rank} | {item['correct']} |"
        )

    positives = [item for item in ordered if item["expected"] != "NONE"]
    negatives = [item for item in ordered if item["expected"] == "NONE"]
    correct_top1 = sum(item["correct"] == "Yes" for item in positives)
    correct_top3 = sum(item["expected_rank"] is not None and int(item["expected_rank"]) <= 3 for item in positives)
    lines.extend(["", "Similarity is cosine similarity converted to a percentage for presentation.", "Margin is top-1 minus top-2 in percentage points; it measures ranking separation, not calibrated confidence.", "", "## Retrieval Metrics", ""])
    lines.append(f"- Positive Top-1 accuracy: {correct_top1}/{len(positives)}.")
    lines.append(f"- Positive Top-3 accuracy: {correct_top3}/{len(positives)}.")
    ranked_positives = [item for item in positives if item["expected_rank"] is not None]
    if ranked_positives:
        lines.append(f"- Average positive expected rank: {sum(int(item['expected_rank']) for item in ranked_positives) / len(ranked_positives):.2f}.")
        lines.append(f"- Average positive Top-1 similarity: {sum(float(item['similarity']) for item in positives) / len(positives) * 100:.2f}%.")
    if negatives:
        lines.append(f"- Unknown-example Top-1 similarity range: {min(float(item['similarity']) for item in negatives) * 100:.2f}% to {max(float(item['similarity']) for item in negatives) * 100:.2f}%.")
    lines.append(f"- Overall Top-1 vs Top-2 margin range: {min(float(item['margin']) for item in ordered) * 100:.2f} to {max(float(item['margin']) for item in ordered) * 100:.2f} percentage points.")
    lines.extend(["", "## Measured Observations", ""])
    lines.append(f"- Positive Top-1 similarity range: {min(float(item['similarity']) for item in positives) * 100:.2f}% to {max(float(item['similarity']) for item in positives) * 100:.2f}%.")
    strongest_negative = max(negatives, key=lambda item: float(item["similarity"])) if negatives else None
    if strongest_negative is not None:
        lines.append("- Unknown functions are reported as Top-1 misses because no rejection threshold is applied.")
        lines.append(f"- Strongest unknown attraction: `{strongest_negative['example']}` -> `{strongest_negative['top_match']}` at {float(strongest_negative['similarity']) * 100:.2f}%.")
    narrowest = min(ordered, key=lambda item: float(item["margin"]))
    widest = max(ordered, key=lambda item: float(item["margin"]))
    lines.append(f"- Narrowest Top-1/Top-2 separation: `{narrowest['example']}` at {float(narrowest['margin']) * 100:.2f} percentage points.")
    lines.append(f"- Widest Top-1/Top-2 separation: `{widest['example']}` at {float(widest['margin']) * 100:.2f} percentage points.")
    lines.append("- These observations describe this dataset and run only; they do not establish general-purpose function identification performance.")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def write_pairwise(path: Path, database_vectors, database_names: list[str]) -> None:
    """Write a deterministic all-database pair summary for false-positive inspection."""
    names = [display_name(name, DATABASE_PREFIX) for name in database_names]
    pairs = []
    for index, first in enumerate(database_names):
        for second in database_names[index + 1:]:
            pairs.append((display_name(first, DATABASE_PREFIX), display_name(second, DATABASE_PREFIX), cosine(database_vectors, first, second)))
    pairs.sort(key=lambda item: (-item[2], item[0], item[1]))
    scores = [score for _, _, score in pairs]
    lines = [
        "# Pairwise Similarity: bsim",
        "",
        f"- Model: `{BSIM_MODEL}`",
        f"- Database functions: `{len(names)}`",
        f"- Pairwise comparisons: `{len(pairs)}`",
        "- Values are weighted Ghidra BSim cosine similarities; no threshold or classification was applied.",
        "",
        "## Basic Statistics",
        "",
        f"- Maximum off-diagonal similarity: `{max(scores):.6f}`",
        f"- Minimum off-diagonal similarity: `{min(scores):.6f}`",
        f"- Mean pairwise similarity: `{sum(scores) / len(scores):.6f}`",
        "",
        "## Top 20 Most Similar Distinct-Function Pairs",
        "",
        "| Rank | Function A | Function B | Cosine Similarity |",
        "| ---: | ---------- | ---------- | ----------------: |",
    ]
    for rank, (first, second, score) in enumerate(pairs[:20], start=1):
        lines.append(f"| {rank:4d} | {first} | {second} | {score:16.6f} |")
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main() -> int:
    """Run both PE analyses and write all BSim research reports inside this directory."""
    import pyghidra

    database_path = ROOT / "database.exe"
    input_path = ROOT / "input.exe"
    if not database_path.is_file() or not input_path.is_file():
        raise RuntimeError("database.exe and input.exe must be built with build_bsim.bat first")

    pyghidra.start()
    from ghidra.app.script import GhidraScriptUtil
    from ghidra.base.project import GhidraProject

    project_parent = Path(tempfile.mkdtemp(prefix="func_vector_bsim_"))
    project = None
    acquired = False
    try:
        GhidraScriptUtil.acquireBundleHostReference()
        acquired = True
        project = GhidraProject.createProject(str(project_parent), "func_vector_bsim", False)
        database_program = open_program(project, database_path)
        database_functions = enumerate_exported_functions(database_program, DATABASE_PREFIX)
        if len(database_functions) != 50:
            raise RuntimeError(f"Expected 50 database functions, found {len(database_functions)}")
        database_vectors, database_counts, weights_name = generate_vectors(database_program, database_functions)
        project.close(database_program)

        input_program = open_program(project, input_path)
        input_functions = enumerate_exported_functions(input_program, INPUT_PREFIX)
        if len(input_functions) != 10:
            raise RuntimeError(f"Expected 10 input functions, found {len(input_functions)}")
        input_vectors, input_counts, input_weights_name = generate_vectors(input_program, input_functions)
        if input_weights_name != weights_name:
            raise RuntimeError(f"Database and input use different BSim weights: {weights_name} versus {input_weights_name}")

        database_names = list(database_functions)
        output_directory = ROOT / "output_bsim"
        output_directory.mkdir(parents=True, exist_ok=True)
        summaries: list[dict[str, object]] = []
        for input_name in input_functions:
            rows = ranking(input_name, input_vectors, database_vectors, database_names)
            report_name = display_name(input_name, INPUT_PREFIX)
            expected = expected_function(report_name)
            expected_rank = next((rank for rank, name, _ in rows if name == expected), None) if expected != "NONE" else None
            write_report(output_directory / f"{report_name}.md", expected, rows, input_counts[input_name])
            summaries.append(
                {
                    "example": report_name,
                    "expected": expected,
                    "top_match": rows[0][1],
                    "similarity": rows[0][2],
                    "margin": rows[0][2] - rows[1][2],
                    "expected_rank": expected_rank,
                    "correct": "Yes" if expected != "NONE" and rows[0][1] == expected else "No",
                }
            )
            print(f"[bsim] Evaluated {report_name}: {rows[0][1]} ({rows[0][2] * 100:.2f}%)")

        write_result(ROOT / "RESULT_bsim.md", summaries)
        write_pairwise(ROOT / "pairwise_bsim.md", database_vectors, database_names)
        (ROOT / "bsim_metadata.md").write_text(
            "\n".join(
                [
                    "# BSim Experiment Metadata",
                    "",
                    f"- Model: `{BSIM_MODEL}`",
                    f"- Signature settings: `0x{SIGNATURE_SETTINGS:X}`",
                    f"- Weights: `{weights_name}`",
                    f"- Database executable: `{database_path.name}` ({database_path.stat().st_size} bytes)",
                    f"- Input executable: `{input_path.name}` ({input_path.stat().st_size} bytes)",
                    f"- Database functions: `{len(database_functions)}`",
                    f"- Input functions: `{len(input_functions)}`",
                    "- Features are generated by `DecompInterface.generateSignatures(function, True, 60, TaskMonitor.DUMMY)`.",
                    "- Similarity uses Ghidra `WeightedLSHCosineVectorFactory` and `VectorCompare`.",
                    "",
                ]
            ),
            encoding="utf-8",
            newline="\n",
        )
        print(f"[bsim] Wrote RESULT_bsim.md and {len(summaries)} per-input reports")
    finally:
        if project is not None:
            project.close()
        if acquired:
            GhidraScriptUtil.releaseBundleHostReference()
        shutil.rmtree(project_parent, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
