"""Shared model and Ollama configuration for the vector-FID experiment."""

from __future__ import annotations

import json
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any


DEFAULT_HOST = "http://localhost:11434"


class OllamaError(RuntimeError):
    """Raised when Ollama cannot satisfy an embedding request."""


@dataclass(frozen=True)
class ModelConfig:
    alias: str
    model: str
    slug: str
    query_instruction: str | None = None
    passage_instruction: str | None = None

    @property
    def instruction_format(self) -> str:
        if self.query_instruction is None and self.passage_instruction is None:
            return "raw source"
        return "instruction followed by a newline and the source"

    def prepare_source(self, source: str, role: str) -> str:
        if role not in {"query", "passage"}:
            raise ValueError(f"Unsupported embedding role: {role}")
        instruction = self.query_instruction if role == "query" else self.passage_instruction
        return source if instruction is None else f"{instruction}\n{source}"


MODEL_CONFIGS = {
    "qwen": ModelConfig(
        alias="qwen",
        model="qwen3-embedding:4b",
        slug="qwen3-embedding-4b",
    ),
    "jina": ModelConfig(
        alias="jina",
        model="hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16",
        slug="jina-code-embeddings-1.5b",
        query_instruction="Find an equivalent code snippet given the following code snippet:",
        passage_instruction="Candidate code snippet:",
    ),
}


ALIASES = {
    "qwen": "qwen",
    "qwen3-embedding:4b": "qwen",
    "qwen3-embedding-4b": "qwen",
    "jina": "jina",
    "hf.co/jinaai/jina-code-embeddings-1.5b-gguf:bf16": "jina",
    "jina-code-embeddings-1.5b": "jina",
}


def resolve_models(value: str) -> list[ModelConfig]:
    normalized = value.strip().lower()
    if normalized == "all":
        return list(MODEL_CONFIGS.values())
    alias = ALIASES.get(normalized)
    if alias is None:
        supported = ", ".join(["qwen", "jina", "all"])
        raise ValueError(f"Unknown model selection {value!r}; use one of: {supported}")
    return [MODEL_CONFIGS[alias]]


def normalize_host(value: str) -> str:
    host = value.strip().rstrip("/")
    return host if "://" in host else f"http://{host}"


def verify_model_available(host: str, config: ModelConfig) -> None:
    request = urllib.request.Request(f"{normalize_host(host)}/api/tags", method="GET")
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            payload: Any = json.load(response)
    except urllib.error.HTTPError as error:
        details = error.read().decode("utf-8", errors="replace")
        raise OllamaError(f"Could not inspect Ollama models: HTTP {error.code}: {details}") from error
    except urllib.error.URLError as error:
        raise OllamaError(f"Could not reach Ollama while checking {config.model}: {error.reason}") from error
    except json.JSONDecodeError as error:
        raise OllamaError("Ollama returned invalid JSON from /api/tags") from error

    models = payload.get("models") if isinstance(payload, dict) else None
    available = {item.get("name") for item in models or [] if isinstance(item, dict)}
    if config.model not in available:
        names = ", ".join(sorted(name for name in available if isinstance(name, str))) or "none"
        raise OllamaError(
            f"Required Ollama model is not installed: {config.model}. "
            f"Available models: {names}. No fallback model will be used."
        )


def embed_source(host: str, config: ModelConfig, prepared_source: str, label: str) -> list[float]:
    payload = json.dumps({"model": config.model, "input": prepared_source}).encode("utf-8")
    request = urllib.request.Request(
        f"{normalize_host(host)}/api/embed",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=300) as response:
            response_payload: Any = json.load(response)
    except urllib.error.HTTPError as error:
        details = error.read().decode("utf-8", errors="replace")
        raise OllamaError(f"Ollama HTTP error while embedding {label}: {error.code}: {details}") from error
    except urllib.error.URLError as error:
        raise OllamaError(f"Could not reach Ollama while embedding {label}: {error.reason}") from error
    except TimeoutError as error:
        raise OllamaError(f"Ollama timed out while embedding {label}") from error
    except json.JSONDecodeError as error:
        raise OllamaError(f"Ollama returned invalid JSON while embedding {label}") from error

    embeddings = response_payload.get("embeddings") if isinstance(response_payload, dict) else None
    if not isinstance(embeddings, list) or len(embeddings) != 1:
        raise OllamaError(f"Ollama returned no single embedding for {label}: {response_payload!r}")
    vector = embeddings[0]
    if not isinstance(vector, list) or not vector or not all(isinstance(value, (int, float)) for value in vector):
        raise OllamaError(f"Ollama returned an invalid vector for {label}")
    return [float(value) for value in vector]
