"""Run build, analysis, FIDB generation, consumer build, and verification for all Boost libraries."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


FIDB_ROOT = Path(__file__).resolve().parents[1]
REPOSITORY_ROOT = FIDB_ROOT.parent
PYTHON = [sys.executable]
PYGHIDRA_WRAPPER = REPOSITORY_ROOT / "TEST" / "run_ghidra_python.bat"
BOOST_CONFIG_PATH = FIDB_ROOT / "boost_config.json"


def run_stage(command: list[str], description: str) -> None:
    """Run one stage and stop immediately with its process diagnostics on failure."""
    print(f"\n== {description} ==")
    completed = subprocess.run(command, cwd=REPOSITORY_ROOT, text=True)
    if completed.returncode != 0:
        raise RuntimeError(f"Stage failed ({completed.returncode}): {description}")


def paths(key: str, config: dict) -> dict[str, Path]:
    """Return stable generated paths for one Boost library."""
    version = config["boost_version"]
    report_root = FIDB_ROOT / "reports" / "boost" / key
    return {
        "metadata": FIDB_ROOT / "build" / "boost" / key / "metadata.json",
        "analysis": report_root / "library_analysis.json",
        "generation": report_root / "fidb_generation.json",
        "verification": report_root / "fidb_verification.json",
        "human_verification": report_root / "fidb_verification.md",
        "analyze_manifest": FIDB_ROOT / "build" / "boost" / key / "analyze_manifest.json",
        "create_manifest": FIDB_ROOT / "build" / "boost" / key / "create_manifest.json",
        "verify_manifest": FIDB_ROOT / "build" / "boost" / key / "verify_manifest.json",
        "project_parent": FIDB_ROOT / "build" / "boost_ghidra_projects" / key,
        "fidb": FIDB_ROOT
        / "libraries"
        / "boost"
        / key
        / version
        / f"boost-{key}-{version}-msvc-x86_64-release.fidb",
    }


def main() -> int:
    """Execute every requested Boost library test without GUI interaction."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="rebuild the pinned Boost source and b2 outputs")
    parser.add_argument("--library", action="append", help="run only this library key; may be repeated")
    args = parser.parse_args()
    if not os.environ.get("GHIDRA_INSTALL_DIR"):
        raise RuntimeError("GHIDRA_INSTALL_DIR must point to the installed Ghidra 12.1.3 directory")
    config = json.loads(BOOST_CONFIG_PATH.read_text(encoding="utf-8"))
    keys = args.library or list(config["libraries"])
    unknown = sorted(set(keys) - set(config["libraries"]))
    if unknown:
        raise RuntimeError(f"Unknown Boost library keys: {unknown}")
    scripts = FIDB_ROOT / "scripts"
    force = ["--force"] if args.force else []
    run_stage(PYTHON + [str(scripts / "build_boost.py")] + force, "build Boost 1.86.0")

    generated_paths = {key: paths(key, config) for key in keys}
    for key in keys:
        item = config["libraries"][key]
        path = generated_paths[key]
        path["analyze_manifest"].write_text(
            json.dumps(
                {
                    "metadata": str(path["metadata"]),
                    "project_parent": str(path["project_parent"]),
                    "project_name": f"boost_{key}_library",
                    "report": str(path["analysis"]),
                    "display_name": item["display_name"],
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        run_stage(
            [
                str(PYGHIDRA_WRAPPER),
                str(scripts / "analyze_library.py"),
                "--manifest",
                str(path["analyze_manifest"]),
            ]
            + force,
            f"analyze {item['display_name']} object files with Ghidra",
        )

    for key in keys:
        item = config["libraries"][key]
        path = generated_paths[key]
        path["create_manifest"].write_text(
            json.dumps(
                {
                    "analysis_report": str(path["analysis"]),
                    "destination": str(path["fidb"]),
                    "family": item["fid_family"],
                    "version": config["boost_version"],
                    "variant": item["fid_variant"],
                    "source_language": config["source_language"],
                    "allow_empty": item.get("allow_empty_fidb", False),
                    "report": str(path["generation"]),
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        run_stage(
            [
                str(PYGHIDRA_WRAPPER),
                str(scripts / "create_fidb.py"),
                "--manifest",
                str(path["create_manifest"]),
            ]
            + force,
            f"generate {item['display_name']} FIDB",
        )

    for key in keys:
        run_stage(
            PYTHON + [str(scripts / "build_boost_consumer.py"), key],
            f"build independent {config['libraries'][key]['display_name']} consumer",
        )

    verification_summaries = {}
    for key in keys:
        item = config["libraries"][key]
        path = generated_paths[key]
        consumer_metadata_path = FIDB_ROOT / "build" / "boost_consumers" / key / "metadata.json"
        consumer = json.loads(consumer_metadata_path.read_text(encoding="utf-8"))
        path["verify_manifest"].write_text(
            json.dumps(
                {
                    "consumer": consumer["executable"],
                    "map": consumer["map_file"],
                    "fidb": str(path["fidb"]),
                    "family": item["fid_family"],
                    "display_name": item["display_name"],
                    "expected": item["expected_functions"],
                    "allow_empty": item.get("allow_empty_fidb", False),
                    "negative_function": item["negative_function"],
                    "source_language": config["source_language"],
                    "compiler": consumer["compiler"],
                    "compiler_version": consumer["compiler_version"],
                    "architecture": consumer["architecture"],
                    "configuration": consumer["configuration"],
                    "linkage": consumer["linkage"],
                    "report": str(path["verification"]),
                    "human_report": str(path["human_verification"]),
                    "project_prefix": f"fidb_boost_{key}_",
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        run_stage(
            [
                str(PYGHIDRA_WRAPPER),
                str(scripts / "verify_fidb.py"),
                "--manifest",
                str(path["verify_manifest"]),
            ],
            f"verify {item['display_name']} Function ID recognition",
        )
        verification_summaries[key] = json.loads(path["verification"].read_text(encoding="utf-8"))

    summary = {
        "stage": "boost_all",
        "boost_version": config["boost_version"],
        "libraries": {
            key: {
                "display_name": config["libraries"][key]["display_name"],
                "fidb": str(generated_paths[key]["fidb"].relative_to(REPOSITORY_ROOT)),
                "expected_match_count": verification_summaries[key]["expected_match_count"],
                "function_count": verification_summaries[key]["function_count"],
                "negative_matches": len(
                    verification_summaries[key]["negative_control"]["false_zlib_matches"]
                ),
                "status": verification_summaries[key]["status"],
            }
            for key in keys
        },
        "status": "PASS",
    }
    summary_path = FIDB_ROOT / "reports" / "boost" / "summary.json"
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    human_lines = [
        "# Boost FIDB Verification Summary",
        "",
        f"> PASS: original Ghidra Function ID verified {len(keys)} Boost libraries from Boost {config['boost_version']}.",
        "",
        "| Library | FIDB | Expected matches | Consumer functions | Negative matches | Status |",
        "| --- | --- | ---: | ---: | ---: | --- |",
    ]
    for key, item in summary["libraries"].items():
        human_lines.append(
            f"| {item['display_name']} | `{item['fidb']}` | {item['expected_match_count']} | "
            f"{item['function_count']} | {item['negative_matches']} | {item['status']} |"
        )
    human_path = FIDB_ROOT / "reports" / "boost" / "summary.md"
    human_path.write_text("\n".join(human_lines) + "\n", encoding="utf-8", newline="\n")
    print(f"\n== SUCCESS ==\nBoost summary: {summary_path}\nVerified libraries: {len(keys)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise
