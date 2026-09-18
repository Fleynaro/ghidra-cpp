# Vector Function ID Research Prototype

This prototype compares `qwen3-embedding:4b` and `hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16` for identifying C/C++ functions by algorithmic and control-flow structure after semantic naming information has been removed.

## Prerequisites

- Python 3.9 or newer
- Ollama running at `http://localhost:11434`
- The models installed with `ollama pull qwen3-embedding:4b` and `ollama pull hf.co/jinaai/jina-code-embeddings-1.5b-GGUF:BF16`

The Python scripts use only the standard library. They send only the anonymized source text to Ollama. Database filenames, input filenames, and expected labels are not included in embedding requests.

The exact Jina GGUF must be served by an Ollama build whose `/api/embed` route supports this Qwen2-based embedding model. On the validation machine, the installed Ollama `0.34.0` reports this model as `completion` and returns HTTP 501 from `/api/embed`; an isolated Ollama `0.7.1` server on `http://127.0.0.1:11436` was used instead. This is still the Ollama local API and uses the same Ollama model store. The scripts fail explicitly when the selected exact model is unavailable or its API rejects embeddings.

## Hypothesis

An embedding of anonymized source code will place structurally similar implementations near one another, allowing a database function to be identified without relying on semantic function, parameter, local, comment, include, or filename information.

## Run

From this directory, build and evaluate either model independently:

```text
python build_embeddings.py --model qwen
python evaluate.py --model qwen

python build_embeddings.py --model jina --host http://127.0.0.1:11436
python evaluate.py --model jina --host http://127.0.0.1:11436
```

To run both models in sequence, use:

```text
python build_embeddings.py --model all --host http://127.0.0.1:11436
python evaluate.py --model all --host http://127.0.0.1:11436
```

For PowerShell, start the embedding-capable Ollama instance with `$env:OLLAMA_HOST='127.0.0.1:11436'; $env:OLLAMA_MODELS='C:\Users\<user>\.ollama\models'; ollama serve` before the Jina commands. Do not use a different model name as a fallback.

`build_embeddings.py` reads each `database/*.cpp` file and writes a self-contained model-specific database: `embeddings_qwen3-embedding-4b.json` or `embeddings_jina-code-embeddings-1.5b.json`. `evaluate.py` reads the selected database and each `input/*.cpp` file, then writes reports under `output_qwen3-embedding-4b/` or `output_jina-code-embeddings-1.5b/` and summaries named `RESULT_qwen3-embedding-4b.md` or `RESULT_jina-code-embeddings-1.5b.md`.

The scripts accept `--host` and `--model` options. Model selection accepts `qwen`, `jina`, or `all`; the exact Ollama model names are recorded in every JSON database and report. The scripts query Ollama's `/api/tags` first and fail if the exact selected model is absent; they never fall back silently.

Qwen uses the original raw anonymized source as both database passage and input query, preserving the original experiment. Jina uses its code-to-code retrieval format: database functions are sent as `Candidate code snippet:\n` followed by the source, while input functions are sent as `Find an equivalent code snippet given the following code snippet:\n` followed by the source. Filenames and expected labels are used only for report metadata, never in embedding requests.

Input filenames use the form `<case>_<expected-or-none>_<description>.cpp` for report metadata only. For negative examples, the expected label is `NONE`.

## Pairwise Similarity Analysis

`pairwise_similarity.py` does not call Ollama or regenerate embeddings. It samples exactly 10 functions from the existing model-specific database with seed `20260915`, computes all 45 unique off-diagonal cosine similarities, and writes a full matrix plus sorted pair list:

```text
python pairwise_similarity.py --model qwen
python pairwise_similarity.py --model jina
python pairwise_similarity.py --model all
```

The individual reports are `pairwise_qwen.md` and `pairwise_jina.md`; `--model all` also writes `pairwise_comparison.md`. The same seeded selection is verified for both models.
