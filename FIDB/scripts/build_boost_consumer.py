"""Build one independent C++ consumer executable for a Boost FID library."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import FIDB_ROOT, msvc_version, run_msvc, write_json  # noqa: E402
from build_boost import BOOST_BUILD_ROOT, load_boost_config  # noqa: E402


def main() -> int:
    """Compile a real Boost API test without consumer debug symbols."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", choices=sorted(load_boost_config()["libraries"]))
    args = parser.parse_args()
    config = load_boost_config()
    item = config["libraries"][args.library]
    metadata = json.loads((BOOST_BUILD_ROOT / args.library / "metadata.json").read_text(encoding="utf-8"))
    source = FIDB_ROOT / item["test_source"]
    output_root = FIDB_ROOT / "build" / "boost_consumers" / args.library
    output_root.mkdir(parents=True, exist_ok=True)
    executable = output_root / f"{args.library}_detection.exe"
    map_file = output_root / f"{args.library}_detection.map"
    object_file = output_root / f"{args.library}_detection.obj"
    for path in (executable, map_file, object_file):
        path.unlink(missing_ok=True)
    stage_lib = Path(metadata["static_library"]).parent
    dependency_paths = []
    for dependency in item["link_libraries"]:
        candidates = sorted(stage_lib.glob(f"*boost_{dependency}*.lib"))
        candidates = [path for path in candidates if "-gd-" not in path.name and "-s-" not in path.name]
        if len(candidates) != 1:
            raise RuntimeError(f"Boost dependency {dependency} is not uniquely staged: {candidates}")
        dependency_paths.append(candidates[0])
    command = (
        f'cl /nologo /O2 /EHsc /std:c++17 /MD /DBOOST_ALL_NO_LIB /I"{metadata["source_root"]}" '
        f'"{source}" /link /OUT:"{executable}" /MAP:"{map_file}" '
        + " ".join(f'"{path}"' for path in dependency_paths)
    )
    completed = run_msvc(command, output_root)
    if not executable.is_file():
        raise RuntimeError(
            f"Boost consumer compilation failed to produce {executable}:\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    execution = subprocess.run(
        [str(executable)],
        cwd=output_root,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if execution.returncode != 0:
        raise RuntimeError(
            f"{item['display_name']} consumer exited with {execution.returncode}:\n"
            f"stdout:\n{execution.stdout}\nstderr:\n{execution.stderr}"
        )
    result = {
        "library_key": args.library,
        "display_name": item["display_name"],
        "source": str(source),
        "executable": str(executable),
        "map_file": str(map_file),
        "boost_version": config["boost_version"],
        "compiler": config["compiler"],
        "compiler_version": msvc_version(),
        "architecture": config["architecture"],
        "configuration": config["configuration"],
        "linkage": config["linkage"],
        "debug_symbols": False,
        "link_libraries": [str(path) for path in dependency_paths],
        "expected_functions": item["expected_functions"],
        "negative_function": item["negative_function"],
        "compile_command": command,
        "build_stdout_tail": completed.stdout[-4000:],
        "build_stderr_tail": completed.stderr[-4000:],
        "execution_stdout": execution.stdout[-4000:],
        "execution_stderr": execution.stderr[-4000:],
    }
    output_metadata = output_root / "metadata.json"
    write_json(output_metadata, result)
    print(f"[+] Built {item['display_name']} consumer: {executable}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
