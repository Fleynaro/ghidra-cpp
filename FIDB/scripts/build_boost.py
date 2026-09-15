"""Download Boost 1.86.0 and build/extract real MSVC static-library object inputs."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
import tarfile
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import BUILD_ROOT, FIDB_ROOT, load_config, msvc_version, run_msvc, write_json  # noqa: E402


BOOST_CONFIG_PATH = FIDB_ROOT / "boost_config.json"
BOOST_BUILD_ROOT = BUILD_ROOT / "boost"


def load_boost_config() -> dict:
    """Load the checked-in Boost version, build flags, and library test contract."""
    return json.loads(BOOST_CONFIG_PATH.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    """Compute the source archive digest used to pin the Boost release."""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def valid_archive(path: Path) -> bool:
    """Check that a cached Boost gzip tar archive is complete."""
    try:
        with tarfile.open(path, "r:gz") as archive:
            archive.getmembers()
        return True
    except (OSError, tarfile.TarError, EOFError):
        return False


def download_archive(config: dict, destination: Path) -> str:
    """Download Boost through ranged HTTPS requests and verify its official digest."""
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.is_file() and not valid_archive(destination):
        destination.unlink()
    if not destination.is_file():
        url = config["boost_archive_url"]
        head = urllib.request.Request(url, method="HEAD", headers={"User-Agent": "FIDB-boost-pipeline/1.0"})
        with urllib.request.urlopen(head, timeout=120) as response:
            total = int(response.headers.get("Content-Length", "0"))
        part = destination.with_suffix(destination.suffix + ".part")
        part.unlink(missing_ok=True)
        downloaded = 0
        with part.open("wb") as target:
            while downloaded < total:
                end = min(downloaded + 1024 * 1024 - 1, total - 1)
                request = urllib.request.Request(
                    url,
                    headers={
                        "User-Agent": "FIDB-boost-pipeline/1.0",
                        "Range": f"bytes={downloaded}-{end}",
                    },
                )
                with urllib.request.urlopen(request, timeout=120) as response:
                    data = response.read()
                    content_range = response.headers.get("Content-Range", "")
                if not data:
                    raise RuntimeError(f"Empty Boost archive range at byte {downloaded}")
                if downloaded == 0 and not content_range and len(data) == total:
                    target.write(data)
                    downloaded = total
                    break
                match = re.match(r"bytes\s+(\d+)-(\d+)/(\d+)", content_range)
                if not match or int(match.group(1)) != downloaded:
                    raise RuntimeError(f"Unexpected Boost archive range response: {content_range}")
                target.write(data)
                downloaded += len(data)
        if downloaded != total:
            part.unlink(missing_ok=True)
            raise RuntimeError(f"Boost archive download was incomplete: {downloaded} of {total} bytes")
        part.replace(destination)
    if not valid_archive(destination):
        raise RuntimeError(f"Boost archive is not a complete gzip tar archive: {destination}")
    digest = sha256(destination)
    if digest != config["boost_archive_sha256"].lower():
        raise RuntimeError(
            f"Boost archive digest mismatch: expected {config['boost_archive_sha256']}, got {digest}"
        )
    return digest


def safe_extract(archive_path: Path, destination: Path) -> None:
    """Extract the pinned Boost archive while rejecting traversal paths."""
    destination.mkdir(parents=True, exist_ok=True)
    root = destination.resolve()
    with tarfile.open(archive_path, "r:gz") as archive:
        members = archive.getmembers()
        for member in members:
            target = (destination / member.name).resolve()
            if root not in target.parents and target != root:
                raise RuntimeError(f"Refusing archive path outside extraction root: {member.name}")
        archive.extractall(destination)


def select_library(stage_dir: Path, b2_name: str) -> Path:
    """Select the one release static library emitted for a requested Boost component."""
    candidates = sorted(stage_dir.glob(f"*boost_{b2_name}*.lib"))
    candidates = [path for path in candidates if "-gd-" not in path.name and "-s-" not in path.name]
    if len(candidates) != 1:
        raise RuntimeError(f"Expected one release Boost {b2_name} library, found {candidates}")
    return candidates[0]


def archive_members(library_path: Path, cwd: Path) -> list[str]:
    """List COFF members using Microsoft LIB.EXE rather than parsing archive bytes."""
    completed = run_msvc(f'lib /nologo /list "{library_path}"', cwd)
    members = []
    for line in completed.stdout.splitlines():
        value = line.strip()
        if value.lower().endswith(".obj"):
            members.append(value)
    if not members:
        raise RuntimeError(f"LIB.EXE returned no object members for {library_path}")
    return members


def extract_objects(library_path: Path, output_dir: Path, cwd: Path) -> list[Path]:
    """Extract every object member through LIB.EXE for Ghidra object import."""
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    objects = []
    for index, member in enumerate(archive_members(library_path, cwd)):
        object_path = output_dir / f"{index:04d}_{Path(member).name}"
        command = f'lib /nologo /extract:"{member}" "{library_path}" /out:"{object_path}"'
        run_msvc(command, cwd)
        if not object_path.is_file():
            raise RuntimeError(f"LIB.EXE did not extract {member} to {object_path}")
        objects.append(object_path)
    return objects


def main() -> int:
    """Build all requested Boost libraries and create per-library source manifests."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="replace downloaded source and Boost build outputs")
    args = parser.parse_args()
    boost = load_boost_config()
    compiler_config = load_config()
    archive = BUILD_ROOT / "downloads" / f"boost_{boost['boost_version'].replace('.', '_')}.tar.gz"
    archive_digest = download_archive(boost, archive)
    source_parent = BUILD_ROOT / "source"
    source_root = source_parent / f"boost_{boost['boost_version'].replace('.', '_')}"
    if args.force and source_root.exists():
        shutil.rmtree(source_root)
    if not source_root.exists():
        safe_extract(archive, source_parent)
    if not (source_root / "bootstrap.bat").is_file():
        raise RuntimeError(f"Boost source root is missing bootstrap.bat: {source_root}")
    b2_path = source_root / "b2.exe"
    if args.force:
        for path in (b2_path, source_root / "bjam.exe"):
            path.unlink(missing_ok=True)
    bootstrap = 'bootstrap.bat --with-toolset=msvc'
    run_msvc(bootstrap, source_root)
    if not b2_path.is_file():
        raise RuntimeError(f"Boost bootstrap did not create {b2_path}")

    b2_build_dir = BOOST_BUILD_ROOT / "b2_build"
    stage_dir = BOOST_BUILD_ROOT / "stage"
    if args.force:
        shutil.rmtree(b2_build_dir, ignore_errors=True)
        shutil.rmtree(stage_dir, ignore_errors=True)
    stage_dir.mkdir(parents=True, exist_ok=True)
    requested = [item["b2_name"] for item in boost["libraries"].values()]
    with_libraries = " ".join(f"--with-{name}" for name in requested)
    flags = " ".join(boost["b2_flags"])
    command = (
        f'b2.exe --build-dir="{b2_build_dir}" --stagedir="{stage_dir}" '
        f"{flags} {with_libraries} stage -j2"
    )
    compiler = msvc_version()
    completed = run_msvc(command, source_root)

    manifests = {}
    for key, item in boost["libraries"].items():
        static_library = select_library(stage_dir / "lib", item["b2_name"])
        objects = extract_objects(static_library, BOOST_BUILD_ROOT / key / "objects", source_root)
        metadata = {
            "library_key": key,
            "display_name": item["display_name"],
            "boost_version": boost["boost_version"],
            "source_archive_url": boost["boost_archive_url"],
            "source_archive_sha256": archive_digest,
            "source_root": str(source_root),
            "static_library": str(static_library),
            "object_files": [str(path) for path in objects],
            "compiler": boost["compiler"],
            "compiler_version": compiler,
            "architecture": boost["architecture"],
            "configuration": boost["configuration"],
            "linkage": boost["linkage"],
            "language_id": boost["ghidra_language_id"],
            "compiler_spec": boost["ghidra_compiler_spec"],
            "b2_command": command,
            "expected_functions": item["expected_functions"],
            "negative_function": item["negative_function"],
            "build_stdout_tail": completed.stdout[-4000:],
            "build_stderr_tail": completed.stderr[-4000:],
        }
        path = BOOST_BUILD_ROOT / key / "metadata.json"
        write_json(path, metadata)
        manifests[key] = metadata
        print(f"[+] Built {item['display_name']} with {len(objects)} extracted object files")
    summary = {
        "boost_version": boost["boost_version"],
        "archive_sha256": archive_digest,
        "compiler_version": compiler,
        "architecture": compiler_config["architecture"],
        "libraries": {key: str(BOOST_BUILD_ROOT / key / "metadata.json") for key in manifests},
    }
    write_json(BOOST_BUILD_ROOT / "metadata.json", summary)
    print(f"[+] Boost build metadata: {BOOST_BUILD_ROOT / 'metadata.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
