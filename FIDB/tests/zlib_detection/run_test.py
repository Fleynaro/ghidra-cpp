"""Run the complete zlib -> Ghidra FIDB -> independent consumer verification pipeline."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


FIDB_ROOT = Path(__file__).resolve().parents[2]
REPOSITORY_ROOT = FIDB_ROOT.parent
PYTHON = sys.executable
PYGHIDRA_WRAPPER = REPOSITORY_ROOT / "TEST" / "run_ghidra_python.bat"


def run_stage(command: list[str], description: str) -> None:
    """Run one pipeline stage and preserve its useful diagnostics on failure."""
    print(f"\n== {description} ==")
    completed = subprocess.run(command, cwd=REPOSITORY_ROOT, text=True)
    if completed.returncode != 0:
        raise RuntimeError(f"Stage failed ({completed.returncode}): {description}")


def main() -> int:
    """Build, analyze, generate, compile, and verify without GUI interaction."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="rebuild source, project, and FIDB")
    args = parser.parse_args()
    if not os.environ.get("GHIDRA_INSTALL_DIR"):
        raise RuntimeError("GHIDRA_INSTALL_DIR must point to the installed Ghidra 12.1.3 directory")

    python = [PYTHON]
    scripts = FIDB_ROOT / "scripts"
    run_stage(python + [str(scripts / "build_zlib.py")] + (["--force"] if args.force else []), "build zlib")
    run_stage(
        [str(PYGHIDRA_WRAPPER), str(scripts / "analyze_library.py")] + (["--force"] if args.force else []),
        "analyze zlib object files with Ghidra",
    )
    run_stage(
        [str(PYGHIDRA_WRAPPER), str(scripts / "create_fidb.py")] + (["--force"] if args.force else []),
        "generate packed Ghidra FIDB",
    )
    run_stage(python + [str(FIDB_ROOT / "tests" / "zlib_detection" / "build_consumer.py")], "build independent consumer")

    config = json.loads((FIDB_ROOT / "config.json").read_text(encoding="utf-8"))
    fid_path = (
        FIDB_ROOT
        / "libraries"
        / "zlib"
        / config["zlib_version"]
        / f"zlib-{config['zlib_version']}-{config['fid_library_variant']}.fidb"
    )
    consumer_metadata = json.loads(
        (FIDB_ROOT / "build" / "consumer" / "metadata.json").read_text(encoding="utf-8")
    )
    run_stage(
        [
            str(PYGHIDRA_WRAPPER),
            str(scripts / "verify_fidb.py"),
            "--consumer",
            consumer_metadata["executable"],
            "--map",
            consumer_metadata["map_file"],
            "--fidb",
            str(fid_path),
        ],
        "verify FIDB and original Function ID recognition",
    )
    generation = json.loads((FIDB_ROOT / "reports" / "fidb_generation.json").read_text(encoding="utf-8"))
    verification = json.loads((FIDB_ROOT / "reports" / "fidb_verification.json").read_text(encoding="utf-8"))
    print("\n== SUCCESS ==")
    print(f"FIDB: {fid_path}")
    print(f"Ingested records: {generation['function_record_count']}")
    print(f"Expected zlib matches: {verification['expected_match_count']}")
    print(f"False zlib matches for negative control: {len(verification['negative_control']['false_zlib_matches'])}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise
