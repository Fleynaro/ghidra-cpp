#!/usr/bin/env python3
"""Measure pairwise cosine similarity among a fixed random sample of functions."""

from __future__ import annotations

import argparse
import json
import math
import random
import statistics
import sys
from pathlib import Path
from typing import Any

from model_config import ModelConfig, resolve_models


SEED = 20260915
SELECTION_SIZE = 10
EXPECTED_PAIR_COUNT = SELECTION_SIZE * (SELECTION_SIZE - 1) // 2


def cosine(left: list[float], right: list[float]) -> float:
    if len(left) != len(right):
        raise ValueError(f"Embedding dimensions differ: {len(left)} and {len(right)}")
    left_norm = math.sqrt(sum(value * value for value in left))
    right_norm = math.sqrt(sum(value * value for value in right))
    if left_norm == 0 or right_norm == 0:
        raise ValueError("Cannot calculate cosine similarity for a zero vector")
    return sum(a * b for a, b in zip(left, right)) / (left_norm * right_norm)


def load_database(config: ModelConfig, database_path: Path) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    database = json.loads(database_path.read_text(encoding="utf-8"))
    if database.get("model") != config.model:
        raise ValueError(f"Database model does not match {config.model}: {database_path}")
    records = database.get("records")
    dimension = database.get("embedding_dimension")
    if not isinstance(records, list) or len(records) < SELECTION_SIZE or not isinstance(dimension, int):
        raise ValueError(f"Database must contain at least {SELECTION_SIZE} records and a dimension: {database_path}")
    if not all(isinstance(record, dict) for record in records):
        raise ValueError(f"Malformed record list in {database_path}")
    normalized_records = sorted(records, key=lambda record: record.get("file", ""))
    for record in normalized_records:
        if not isinstance(record, dict) or not isinstance(record.get("file"), str) or not isinstance(record.get("function"), str):
            raise ValueError(f"Malformed record in {database_path}: {record!r}")
        vector = record.get("embedding")
        if not isinstance(vector, list) or len(vector) != dimension or not all(isinstance(value, (int, float)) for value in vector):
            raise ValueError(f"Malformed embedding in {database_path}: {record.get('file')}")
    return database, normalized_records


def select_records(records: list[dict[str, Any]], seed: int) -> list[dict[str, Any]]:
    generator = random.Random(seed)
    selected = generator.sample(records, SELECTION_SIZE)
    if len(selected) != SELECTION_SIZE:
        raise AssertionError(f"Expected {SELECTION_SIZE} selected records, got {len(selected)}")
    return selected


def analyze(config: ModelConfig, database_path: Path, seed: int) -> dict[str, Any]:
    database, records = load_database(config, database_path)
    selected = select_records(records, seed)
    names = [record["function"] for record in selected]
    vectors = {record["function"]: [float(value) for value in record["embedding"]] for record in selected}
    if len(vectors) != SELECTION_SIZE:
        raise AssertionError("Selected function names are not unique")

    matrix: dict[str, dict[str, float]] = {name: {} for name in names}
    for left_name in names:
        for right_name in names:
            matrix[left_name][right_name] = 1.0 if left_name == right_name else cosine(vectors[left_name], vectors[right_name])

    for left_name in names:
        if matrix[left_name][left_name] != 1.0:
            raise AssertionError(f"Diagonal is not exactly 1.0 for {left_name}")
        for right_name in names:
            if matrix[left_name][right_name] != matrix[right_name][left_name]:
                raise AssertionError(f"Matrix is not symmetric for {left_name}, {right_name}")

    pairs: list[tuple[str, str, float]] = []
    for left_index, left_name in enumerate(names):
        for right_name in names[left_index + 1:]:
            pairs.append((left_name, right_name, matrix[left_name][right_name]))
    pairs.sort(key=lambda pair: (-pair[2], pair[0], pair[1]))
    if len(pairs) != EXPECTED_PAIR_COUNT:
        raise AssertionError(f"Expected {EXPECTED_PAIR_COUNT} unique pairs, got {len(pairs)}")
    if any(left == right for left, right, _ in pairs):
        raise AssertionError("Self-pair found in pair list")
    if len({frozenset((left, right)) for left, right, _ in pairs}) != EXPECTED_PAIR_COUNT:
        raise AssertionError("Duplicate unordered pair found")
    if any(pairs[index][2] < pairs[index + 1][2] for index in range(len(pairs) - 1)):
        raise AssertionError("Pair list is not sorted in descending order")

    scores = [score for _, _, score in pairs]
    return {
        "config": config,
        "database": database,
        "database_path": database_path,
        "seed": seed,
        "selected": selected,
        "names": names,
        "dimension": database["embedding_dimension"],
        "matrix": matrix,
        "pairs": pairs,
        "statistics": {
            "maximum": max(scores),
            "minimum": min(scores),
            "mean": statistics.mean(scores),
            "median": statistics.median(scores),
            "standard_deviation": statistics.pstdev(scores),
        },
    }


def matrix_table(names: list[str], matrix: dict[str, dict[str, float]]) -> str:
    lines = [
        "| Function | " + " | ".join(names) + " |",
        "| -------- | " + " | ".join("------:" for _ in names) + " |",
    ]
    for row_name in names:
        lines.append(
            f"| {row_name} | " + " | ".join(f"{matrix[row_name][column_name]:.4f}" for column_name in names) + " |"
        )
    return "\n".join(lines)


def pair_table(pairs: list[tuple[str, str, float]], limit: int | None = None) -> str:
    rows = pairs if limit is None else pairs[:limit]
    lines = [
        "| Rank | Function A | Function B | Cosine Similarity |",
        "| ---: | ---------- | ---------- | ----------------: |",
    ]
    for rank, (left, right, score) in enumerate(rows, start=1):
        lines.append(f"| {rank:4d} | {left} | {right} | {score:16.6f} |")
    return "\n".join(lines)


def render_report(analysis: dict[str, Any]) -> str:
    config: ModelConfig = analysis["config"]
    stats = analysis["statistics"]
    selected_lines = ["|   # | Function |", "| --: | -------- |"]
    for index, name in enumerate(analysis["names"], start=1):
        selected_lines.append(f"| {index:3d} | {name} |")
    return "\n".join(
        [
            f"# Pairwise Similarity: {config.alias}",
            "",
            "## Experiment Metadata",
            "",
            f"- Model: `{config.model}`",
            f"- Embedding database: `{analysis['database_path'].name}`",
            f"- Random seed: `{analysis['seed']}`",
            f"- Selected functions: `{len(analysis['names'])}`",
            f"- Embedding dimension: `{analysis['dimension']}`",
            f"- Pairwise comparisons: `{len(analysis['pairs'])}`",
            "",
            "## Selected Functions",
            "",
            "\n".join(selected_lines),
            "",
            "## Pairwise Similarity Matrix",
            "",
            matrix_table(analysis["names"], analysis["matrix"]),
            "",
            "Diagonal entries are exactly `1.0000`; each off-diagonal pair represents two distinct selected functions.",
            "",
            "## Sorted Pair List",
            "",
            pair_table(analysis["pairs"]),
            "",
            "## Basic Statistics",
            "",
            f"- Maximum off-diagonal similarity: `{stats['maximum']:.6f}`",
            f"- Minimum off-diagonal similarity: `{stats['minimum']:.6f}`",
            f"- Mean pairwise similarity: `{stats['mean']:.6f}`",
            f"- Median pairwise similarity: `{stats['median']:.6f}`",
            f"- Population standard deviation: `{stats['standard_deviation']:.6f}`",
            "",
            "### Top 5 Most Similar Distinct-Function Pairs",
            "",
            pair_table(analysis["pairs"], limit=5),
            "",
            "No expected labels, input functions, thresholds, or handcrafted classifications are used in this analysis.",
            "",
        ]
    )


def render_comparison(analyses: list[dict[str, Any]]) -> str:
    if len(analyses) != 2:
        raise ValueError("A comparison requires exactly Qwen and Jina analyses")
    qwen, jina = analyses
    if qwen["names"] != jina["names"]:
        raise AssertionError("Qwen and Jina selected different functions")
    qwen_stats = qwen["statistics"]
    jina_stats = jina["statistics"]
    lines = [
        "# Pairwise Similarity Comparison",
        "",
        f"Random seed: `{qwen['seed']}`",
        f"Selected functions: `{len(qwen['names'])}`",
        f"Pairwise comparisons per model: `{EXPECTED_PAIR_COUNT}`",
        "",
        "The same fixed-seed function selection is used for both models. Values are raw cosine similarities; no threshold or model-specific transformation is applied.",
        "",
        "## Summary Metrics",
        "",
        "| Metric | Qwen3-Embedding:4B | Jina Code Embeddings 1.5B |",
        "| ------------------ | -----------------: | ------------------------: |",
        f"| Mean similarity | {qwen_stats['mean']:.6f} | {jina_stats['mean']:.6f} |",
        f"| Median similarity | {qwen_stats['median']:.6f} | {jina_stats['median']:.6f} |",
        f"| Maximum similarity | {qwen_stats['maximum']:.6f} | {jina_stats['maximum']:.6f} |",
        f"| Minimum similarity | {qwen_stats['minimum']:.6f} | {jina_stats['minimum']:.6f} |",
        "",
        "## Top 10 Pairs: Qwen3-Embedding:4B",
        "",
        pair_table(qwen["pairs"], limit=10),
        "",
        "## Top 10 Pairs: Jina Code Embeddings 1.5B",
        "",
        pair_table(jina["pairs"], limit=10),
        "",
    ]
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="all", help="Model alias: qwen, jina, or all")
    parser.add_argument("--seed", type=int, default=SEED, help=f"Fixed selection seed (default: {SEED})")
    parser.add_argument("--database-directory", type=Path, default=Path("."), help="Directory containing embeddings_MODEL.json")
    parser.add_argument("--output-directory", type=Path, default=Path("."), help="Directory for generated Markdown reports")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    configs = resolve_models(args.model)
    args.output_directory.mkdir(parents=True, exist_ok=True)
    analyses: list[dict[str, Any]] = []
    for config in configs:
        database_path = args.database_directory / f"embeddings_{config.slug}.json"
        analysis = analyze(config, database_path, args.seed)
        output_path = args.output_directory / f"pairwise_{config.alias}.md"
        output_path.write_text(render_report(analysis), encoding="utf-8")
        analyses.append(analysis)
        print(f"[{config.alias}] Selected {len(analysis['names'])} functions and wrote {len(analysis['pairs'])} pairs to {output_path}")

    if len(configs) == 2:
        comparison_path = args.output_directory / "pairwise_comparison.md"
        comparison_path.write_text(render_comparison(analyses), encoding="utf-8")
        print(f"Wrote model comparison to {comparison_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, json.JSONDecodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
