"""Shared paths, configuration, and Windows/MSVC process helpers for FIDB."""

from __future__ import annotations

import json
import os
import re
import subprocess
from pathlib import Path


FIDB_ROOT = Path(__file__).resolve().parents[1]
REPOSITORY_ROOT = FIDB_ROOT.parent
CONFIG_PATH = FIDB_ROOT / "config.json"
BUILD_ROOT = FIDB_ROOT / "build"
ZLIB_BUILD_ROOT = BUILD_ROOT / "zlib"
REPORT_ROOT = FIDB_ROOT / "reports"
GHIDRA_LANGUAGE_ID = "x86:LE:64:default"
GHIDRA_COMPILER_SPEC = "windows"


def load_config() -> dict:
    """Load the checked-in, machine-independent FIDB configuration."""
    with CONFIG_PATH.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def write_json(path: Path, value: object) -> None:
    """Write a deterministic UTF-8 JSON report or build manifest."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def portable_path(value: str | Path) -> str:
    """Render repository artifacts relatively and external Ghidra via its env variable."""
    path = Path(value)
    if not path.is_absolute():
        return path.as_posix()
    resolved = path.resolve()
    try:
        return resolved.relative_to(REPOSITORY_ROOT.resolve()).as_posix()
    except ValueError:
        pass
    install_dir = os.environ.get("GHIDRA_INSTALL_DIR")
    if install_dir:
        try:
            relative = resolved.relative_to(Path(install_dir).resolve())
            suffix = relative.as_posix()
            return "$GHIDRA_INSTALL_DIR" if suffix in ("", ".") else f"$GHIDRA_INSTALL_DIR/{suffix}"
        except ValueError:
            pass
    return str(resolved)


def workspace_path(value: str | Path) -> Path:
    """Resolve a report path relative to the repository when it was serialized portably."""
    path = Path(value)
    return path if path.is_absolute() else (REPOSITORY_ROOT / path).resolve()


def find_vcvarsall() -> Path:
    """Find the Visual Studio environment initializer without storing a local path."""
    candidates: list[Path] = []
    vs_install_dir = os.environ.get("VSINSTALLDIR")
    if vs_install_dir:
        candidates.append(Path(vs_install_dir) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat")
    program_files = os.environ.get("ProgramW6432") or os.environ.get("ProgramFiles")
    if program_files:
        candidates.extend(
            Path(program_files).glob(
                "Microsoft Visual Studio/2022/*/VC/Auxiliary/Build/vcvarsall.bat"
            )
        )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise RuntimeError(
        "Visual Studio vcvarsall.bat was not found. Install the MSVC C++ workload "
        "or run this pipeline from a Visual Studio developer command prompt."
    )


def run_msvc(command: str, cwd: Path, check: bool = True) -> subprocess.CompletedProcess[str]:
    """Run one command after initializing the requested x64 MSVC environment."""
    vcvarsall = find_vcvarsall()
    command_line = f'call "{vcvarsall}" x64 && {command}'
    # Passing this as one command line is intentional: list-based Windows
    # argument quoting escapes the quotes required by a spaced vcvarsall path.
    completed = subprocess.run(
        f"cmd.exe /d /c {command_line}",
        cwd=cwd,
        check=False,
        text=True,
        capture_output=True,
        encoding="utf-8",
        errors="replace",
    )
    if check and completed.returncode != 0:
        raise RuntimeError(
            f"MSVC command failed ({completed.returncode}) in {cwd}: {command}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    return completed


def msvc_version() -> str:
    """Return the compiler version reported by the selected x64 MSVC toolchain."""
    completed = run_msvc("cl 2>&1", FIDB_ROOT, check=False)
    output = completed.stdout + completed.stderr
    match = re.search(r"Compiler Version\s+([^\r\n]+)", output)
    if not match:
        raise RuntimeError(f"Could not determine MSVC version from cl output:\n{output}")
    return match.group(1).strip()


def require_ghidra_environment() -> Path:
    """Validate the required Ghidra installation environment and return its path."""
    install_dir = os.environ.get("GHIDRA_INSTALL_DIR")
    if not install_dir:
        raise RuntimeError(
            "GHIDRA_INSTALL_DIR is not set. Set it to the Ghidra 12.1.3 installation "
            "before running a PyGhidra stage."
        )
    path = Path(install_dir)
    if not (path / "Ghidra" / "application.properties").is_file():
        raise RuntimeError(f"Ghidra installation was not found at {path}")
    return path
