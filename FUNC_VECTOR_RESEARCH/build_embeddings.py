#!/usr/bin/env python3
"""Build a model-specific embedding database from anonymized C++ functions."""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from model_config import DEFAULT_HOST, OllamaError, embed_source, normalize_host, resolve_models, verify_model_available


def build_one(host: str, config: Any, source_directory: Path, output_path: Path) -> None:
    files = sorted(source_directory.glob("*.cpp"))
    if not files:
        raise SystemExit(f"No database source files found in {source_directory}")
    verify_model_available(host, config)

    records: list[dict[str, Any]] = []
    dimension: int | None = None
    for path in files:
        source = path.read_text(encoding="utf-8")
        if not source.strip():
            raise SystemExit(f"Database source is empty: {path}")
        prepared_source = config.prepare_source(source, "passage")
        vector = embed_source(host, config, prepared_source, str(path))
        if dimension is None:
            dimension = len(vector)
        elif len(vector) != dimension:
            raise SystemExit(f"Embedding dimension changed for {path}: expected {dimension}, got {len(vector)}")
        records.append({"file": path.name, "function": path.stem, "embedding": vector})
        print(f"[{config.alias}] Embedded {path.name} ({len(vector)} dimensions)")

    database = {
        "format": "vector-fid-2",
        "model": config.model,
        "model_alias": config.alias,
        "model_slug": config.slug,
        "embedding_endpoint": "/api/embed",
        "ollama_host": normalize_host(host),
        "source_directory": source_directory.as_posix(),
        "embedding_role": "passage",
        "instruction_format": config.instruction_format,
        "passage_instruction": config.passage_instruction,
        "query_instruction": config.query_instruction,
        "created_at": datetime.now(timezone.utc).isoformat(),
        "embedding_dimension": dimension,
        "record_count": len(records),
        "records": records,
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(database, indent=2) + "\n", encoding="utf-8")
    print(f"[{config.alias}] Wrote {len(records)} embeddings to {output_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="qwen", help="Model alias: qwen, jina, or all")
    parser.add_argument("--host", default=os.environ.get("OLLAMA_HOST", DEFAULT_HOST))
    parser.add_argument("--database", type=Path, default=Path("database"))
    parser.add_argument("--output", type=Path, default=None, help="Custom output, only valid for one selected model")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    configs = resolve_models(args.model)
    if args.output is not None and len(configs) != 1:
        raise SystemExit("--output cannot be combined with --model all")
    for config in configs:
        output_path = args.output or Path(f"embeddings_{config.slug}.json")
        build_one(args.host, config, args.database, output_path)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OllamaError, ValueError, OSError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
