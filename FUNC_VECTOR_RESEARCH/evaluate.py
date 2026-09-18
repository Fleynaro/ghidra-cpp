#!/usr/bin/env python3
"""Evaluate anonymized input functions with one or more configured models."""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
from pathlib import Path
from typing import Any

from model_config import DEFAULT_HOST, OllamaError, embed_source, resolve_models, verify_model_available


def cosine(left: list[float], right: list[float]) -> float:
    if len(left) != len(right):
        raise ValueError(f"Embedding dimensions differ: {len(left)} and {len(right)}")
    left_norm = math.sqrt(sum(value * value for value in left))
    right_norm = math.sqrt(sum(value * value for value in right))
    if left_norm == 0 or right_norm == 0:
        raise ValueError("Cannot calculate cosine similarity for a zero vector")
    return sum(a * b for a, b in zip(left, right)) / (left_norm * right_norm)


def expected_function(path: Path) -> str:
    parts = path.stem.split("_")
    if len(parts) < 3 or not parts[0].isdigit():
        raise ValueError(f"Input filename must be <number>_<expected>_<description>.cpp: {path.name}")
    return parts[1]


def format_table(rows: list[tuple[int, str, float]]) -> str:
    name_width = max(17, *(len(name) for _, name, _ in rows))
    lines = [
        f"| Rank | {'Database Function':<{name_width}} | Similarity |",
        f"| ---: | {'-' * name_width} | ---------: |",
    ]
    for rank, name, score in rows:
        lines.append(f"| {rank:4d} | {name:<{name_width}} | {score * 100:10.2f}% |")
    return "\n".join(lines)


def write_report(
    path: Path,
    config: Any,
    expected: str,
    expected_rank: int | None,
    rows: list[tuple[int, str, float]],
) -> None:
    top_name = rows[0][1]
    top_score = rows[0][2]
    top_gap = rows[0][2] - rows[1][2]
    correctness = "Yes" if expected != "NONE" and top_name == expected else "No"
    expected_rank_text = str(expected_rank) if expected_rank is not None else "N/A"
    instruction = config.query_instruction or "(none; raw source was embedded)"
    report = "\n".join(
        [
            f"# {path.stem}",
            "",
            f"- Model: `{config.model}`",
            f"- Query example: `{path.stem}`",
            f"- Query instruction: `{instruction}`",
            f"- Expected function: `{expected}`",
            f"- Expected rank: `{expected_rank_text}`",
            f"- Top match: `{top_name}` ({top_score * 100:.2f}%)",
            f"- Top-1 vs top-2 gap: `{top_gap * 100:.2f}` percentage points",
            f"- Correct top-1 match: `{correctness}`",
            "",
            "Cosine similarity is multiplied by 100 for presentation; it is not a probability.",
            "",
            format_table(rows),
            "",
        ]
    )
    path.write_text(report, encoding="utf-8")


def write_result(path: Path, config: Any, summaries: list[dict[str, Any]]) -> None:
    ordered = sorted(summaries, key=lambda item: item["example"])
    lines = [
        f"# Vector Function ID Results: {config.alias}",
        "",
        f"Model: `{config.model}`",
        f"Query instruction: `{config.query_instruction or '(none; raw source)'}`",
        f"Passage instruction: `{config.passage_instruction or '(none; raw source)'}`",
        "",
        "| Example | Expected Function | Top Match | Similarity | Margin (pp) | Expected Rank | Correct? |",
        "| ------- | ----------------- | --------- | ---------: | ----------: | ------------: | :------: |",
    ]
    for item in ordered:
        expected_rank = item["expected_rank"] if item["expected_rank"] is not None else "N/A"
        lines.append(
            f"| {item['example']} | {item['expected']} | {item['top_match']} | "
            f"{item['similarity'] * 100:.2f}% | {item['margin'] * 100:.2f} | {expected_rank} | {item['correct']} |"
        )
    lines.extend(
        [
            "",
            "Similarity is cosine similarity converted to a percentage for presentation.",
            "Margin is top-1 minus top-2 in percentage points; it measures ranking separation, not calibrated confidence.",
            "",
            "## Retrieval Metrics",
            "",
        ]
    )
    positives = [item for item in ordered if item["expected"] != "NONE"]
    negatives = [item for item in ordered if item["expected"] == "NONE"]
    correct_top1 = sum(item["correct"] == "Yes" for item in positives)
    correct_top3 = sum(item["expected_rank"] is not None and item["expected_rank"] <= 3 for item in positives)
    ranked_positives = [item for item in positives if item["expected_rank"] is not None]
    lines.append(f"- Positive Top-1 accuracy: {correct_top1}/{len(positives)}.")
    lines.append(f"- Positive Top-3 accuracy: {correct_top3}/{len(positives)}.")
    if ranked_positives:
        lines.append(f"- Average positive expected rank: {sum(item['expected_rank'] for item in ranked_positives) / len(ranked_positives):.2f}.")
        lines.append(f"- Average positive Top-1 similarity: {sum(item['similarity'] for item in positives) / len(positives) * 100:.2f}%.")
    if negatives:
        lines.append(f"- Unknown-example Top-1 similarity range: {min(item['similarity'] for item in negatives) * 100:.2f}% to {max(item['similarity'] for item in negatives) * 100:.2f}%.")
    lines.append(f"- Overall Top-1 vs Top-2 margin range: {min(item['margin'] for item in ordered) * 100:.2f} to {max(item['margin'] for item in ordered) * 100:.2f} percentage points.")
    lines.extend(["", "## Measured Observations", ""])
    if positives:
        lines.append(f"- Positive Top-1 similarity range: {min(item['similarity'] for item in positives) * 100:.2f}% to {max(item['similarity'] for item in positives) * 100:.2f}%.")
    if negatives:
        strongest_negative = max(negatives, key=lambda item: item["similarity"])
        lines.append("- Unknown functions are reported as Top-1 misses because no rejection threshold is applied.")
        lines.append(f"- Strongest unknown attraction: `{strongest_negative['example']}` -> `{strongest_negative['top_match']}` at {strongest_negative['similarity'] * 100:.2f}%.")
    narrowest = min(ordered, key=lambda item: item["margin"])
    widest = max(ordered, key=lambda item: item["margin"])
    lines.append(f"- Narrowest Top-1/Top-2 separation: `{narrowest['example']}` at {narrowest['margin'] * 100:.2f} percentage points.")
    lines.append(f"- Widest Top-1/Top-2 separation: `{widest['example']}` at {widest['margin'] * 100:.2f} percentage points.")
    lines.append("- These observations describe this dataset and run only; they do not establish general-purpose function identification performance.")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def evaluate_one(host: str, config: Any, database_path: Path, input_directory: Path, output_directory: Path, result_path: Path) -> None:
    verify_model_available(host, config)
    database = json.loads(database_path.read_text(encoding="utf-8"))
    if database.get("format") not in {"vector-fid-1", "vector-fid-2"} or database.get("model") != config.model:
        raise SystemExit(f"Embedding database does not match {config.model}: {database_path}")
    records = database.get("records")
    dimension = database.get("embedding_dimension")
    if not isinstance(records, list) or len(records) < 2 or not isinstance(dimension, int):
        raise SystemExit(f"Invalid or incomplete embedding database: {database_path}")
    for record in records:
        if not isinstance(record, dict) or not isinstance(record.get("file"), str) or not isinstance(record.get("embedding"), list):
            raise SystemExit(f"Malformed embedding record in {database_path}: {record!r}")
        if len(record["embedding"]) != dimension:
            raise SystemExit(f"Malformed embedding dimension in record {record.get('file')}")

    files = sorted(input_directory.glob("*.cpp"))
    if not files:
        raise SystemExit(f"No input source files found in {input_directory}")
    output_directory.mkdir(parents=True, exist_ok=True)
    summaries: list[dict[str, Any]] = []
    for path in files:
        source = path.read_text(encoding="utf-8")
        if not source.strip():
            raise SystemExit(f"Input source is empty: {path}")
        prepared_source = config.prepare_source(source, "query")
        vector = embed_source(host, config, prepared_source, str(path))
        if len(vector) != dimension:
            raise SystemExit(f"Input embedding dimension differs for {path}: expected {dimension}, got {len(vector)}")
        rows = sorted(
            ((record["file"], cosine(vector, [float(value) for value in record["embedding"]])) for record in records),
            key=lambda item: item[1],
            reverse=True,
        )
        ranked = [(rank, Path(name).stem, score) for rank, (name, score) in enumerate(rows, start=1)]
        expected = expected_function(path)
        expected_rank = next((rank for rank, name, _ in ranked if name == expected), None) if expected != "NONE" else None
        write_report(output_directory / f"{path.stem}.md", config, expected, expected_rank, ranked)
        summaries.append({
            "example": path.stem,
            "expected": expected,
            "top_match": ranked[0][1],
            "similarity": ranked[0][2],
            "margin": ranked[0][2] - ranked[1][2],
            "expected_rank": expected_rank,
            "correct": "Yes" if expected != "NONE" and ranked[0][1] == expected else "No",
        })
        print(f"[{config.alias}] Evaluated {path.name}: {ranked[0][1]} ({ranked[0][2] * 100:.2f}%)")
    write_result(result_path, config, summaries)
    print(f"[{config.alias}] Wrote {len(summaries)} reports to {output_directory} and summary to {result_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="qwen", help="Model alias: qwen, jina, or all")
    parser.add_argument("--host", default=os.environ.get("OLLAMA_HOST", DEFAULT_HOST))
    parser.add_argument("--database", type=Path, default=None, help="Custom database, only valid for one selected model")
    parser.add_argument("--input", type=Path, default=Path("input"))
    parser.add_argument("--output", type=Path, default=None, help="Custom output directory, only valid for one selected model")
    parser.add_argument("--result", type=Path, default=None, help="Custom summary path, only valid for one selected model")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    configs = resolve_models(args.model)
    if len(configs) != 1 and any(value is not None for value in (args.database, args.output, args.result)):
        raise SystemExit("--database, --output, and --result cannot be combined with --model all")
    for config in configs:
        database_path = args.database or Path(f"embeddings_{config.slug}.json")
        output_directory = args.output or Path(f"output_{config.slug}")
        result_path = args.result or Path(f"RESULT_{config.slug}.md")
        evaluate_one(args.host, config, database_path, args.input, output_directory, result_path)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OllamaError, ValueError, OSError, json.JSONDecodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
