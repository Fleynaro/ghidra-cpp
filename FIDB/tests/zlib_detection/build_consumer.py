"""Build the independent MSVC x86-64 zlib consumer executable."""

from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from common import FIDB_ROOT, ZLIB_BUILD_ROOT, load_config, msvc_version, run_msvc, write_json  # noqa: E402


def main() -> int:
    """Compile and link the real C++ zlib consumer without a debug symbol database."""
    config = load_config()
    metadata_path = ZLIB_BUILD_ROOT / "metadata.json"
    if not metadata_path.is_file():
        raise RuntimeError("zlib metadata is missing; run scripts/build_zlib.py first")
    zlib_metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    source_path = Path(__file__).resolve().parent / "test.cpp"
    output_root = FIDB_ROOT / "build" / "consumer"
    output_root.mkdir(parents=True, exist_ok=True)
    executable = output_root / "zlib_detection.exe"
    map_file = output_root / "zlib_detection.map"
    for path in (executable, map_file, output_root / "zlib_detection.obj"):
        path.unlink(missing_ok=True)

    command = (
        f'cl /nologo /O2 /W4 /EHsc /MD /I"{zlib_metadata["source_root"]}" '
        f'"{source_path}" /link /OUT:"{executable}" '
        f'/LIBPATH:"{zlib_metadata["source_root"]}" zlib.lib /MAP:"{map_file}"'
    )
    completed = run_msvc(command, output_root)
    if not executable.is_file():
        raise RuntimeError(
            f"consumer compilation produced no executable:\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    metadata = {
        "executable": str(executable),
        "map_file": str(map_file),
        "source": str(source_path),
        "compiler": "MSVC",
        "compiler_version": msvc_version(),
        "architecture": config["architecture"],
        "configuration": config["configuration"],
        "linkage": config["linkage"],
        "debug_symbols": False,
        "expected_zlib_functions": config["expected_functions"],
        "non_zlib_function": config["non_zlib_function"],
        "compile_command": command,
        "build_stdout_tail": completed.stdout[-4000:],
        "build_stderr_tail": completed.stderr[-4000:],
    }
    output_metadata = output_root / "metadata.json"
    write_json(output_metadata, metadata)
    print(f"[+] Built independent consumer: {executable}")
    print(f"[+] Consumer metadata: {output_metadata}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
