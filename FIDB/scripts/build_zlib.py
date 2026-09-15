"""Download and build the exact zlib source used as the FID library input."""

from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import sys
import tarfile
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import (  # noqa: E402
    BUILD_ROOT,
    FIDB_ROOT,
    ZLIB_BUILD_ROOT,
    load_config,
    msvc_version,
    run_msvc,
    write_json,
)


def sha256(path: Path) -> str:
    """Compute the SHA-256 digest recorded for reproducible source provenance."""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_extract(archive: Path, destination: Path) -> None:
    """Extract the trusted zlib release while rejecting path traversal entries."""
    destination.mkdir(parents=True, exist_ok=True)
    root = destination.resolve()
    with tarfile.open(archive, "r:gz") as tar:
        for member in tar.getmembers():
            target = (destination / member.name).resolve()
            if root not in target.parents and target != root:
                raise RuntimeError(f"Refusing archive path outside extraction root: {member.name}")
        tar.extractall(destination)


def valid_archive(path: Path) -> bool:
    """Check that a cached archive is complete before reusing it."""
    try:
        with tarfile.open(path, "r:gz") as tar:
            tar.getmembers()
        return True
    except (OSError, tarfile.TarError, EOFError):
        return False


def download_source(config: dict, archive_path: Path) -> str:
    """Download the upstream release archive and return its verified digest."""
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    if archive_path.is_file() and not valid_archive(archive_path):
        archive_path.unlink()
    if not archive_path.is_file():
        # Some corporate HTTPS proxies truncate a single response while the
        # zlib server advertises byte-range support. Reassemble bounded range
        # requests so an apparently successful partial download cannot enter
        # the build or change the source provenance silently.
        url = config["zlib_archive_url"]
        request = urllib.request.Request(url, method="HEAD", headers={"User-Agent": "FIDB-zlib-pipeline/1.0"})
        with urllib.request.urlopen(request, timeout=120) as response:
            advertised_length = int(response.headers.get("Content-Length", "0"))
        part_path = archive_path.with_suffix(archive_path.suffix + ".part")
        part_path.unlink(missing_ok=True)
        downloaded = 0
        with part_path.open("wb") as target:
            while downloaded < advertised_length:
                end = min(downloaded + 262143, advertised_length - 1)
                request = urllib.request.Request(
                    url,
                    headers={
                        "User-Agent": "FIDB-zlib-pipeline/1.0",
                        "Range": f"bytes={downloaded}-{end}",
                    },
                )
                with urllib.request.urlopen(request, timeout=120) as response:
                    data = response.read()
                    content_range = response.headers.get("Content-Range", "")
                if not data:
                    raise RuntimeError(f"Empty source archive range at byte {downloaded}")
                if downloaded == 0 and not content_range and len(data) == advertised_length:
                    target.write(data)
                    downloaded = advertised_length
                    break
                if not content_range:
                    raise RuntimeError(
                        "The source server/proxy ignored the requested byte range; "
                        "retry with a network path that supports ranged HTTPS downloads."
                    )
                match = re.match(r"bytes\s+(\d+)-(\d+)/(\d+)", content_range)
                if not match or int(match.group(1)) != downloaded:
                    raise RuntimeError(f"Unexpected source archive range response: {content_range}")
                target.write(data)
                downloaded += len(data)
        if downloaded != advertised_length:
            part_path.unlink(missing_ok=True)
            raise RuntimeError(
                f"Source archive download was incomplete: {downloaded} of {advertised_length} bytes"
            )
        part_path.replace(archive_path)
    if not valid_archive(archive_path):
        raise RuntimeError(f"Downloaded source archive is not a complete gzip tar archive: {archive_path}")
    digest = sha256(archive_path)
    expected = config.get("zlib_archive_sha256", "").strip().lower()
    if expected and digest != expected:
        raise RuntimeError(
            f"zlib archive digest mismatch: expected {expected}, got {digest}. "
            "Delete the archive and retry from the official release URL."
        )
    return digest


def clean_outputs(source_root: Path) -> None:
    """Remove only generated MSVC outputs so changed build flags cannot reuse stale objects."""
    for pattern in ("*.obj", "*.lib", "*.dll", "*.exp", "*.pdb", "*.res", "*.manifest", "*.exe"):
        for path in source_root.glob(pattern):
            path.unlink(missing_ok=True)


def build(config: dict, force: bool) -> dict:
    """Build zlib's upstream static-library target with x64 MSVC Release settings."""
    archive_path = BUILD_ROOT / "downloads" / f"zlib-{config['zlib_version']}.tar.gz"
    archive_digest = download_source(config, archive_path)
    source_parent = BUILD_ROOT / "source"
    source_root = source_parent / f"zlib-{config['zlib_version']}"
    if force and source_root.exists():
        shutil.rmtree(source_root)
    if not source_root.is_dir():
        safe_extract(archive_path, source_parent)
    if not (source_root / config["zlib_makefile"]).is_file():
        raise RuntimeError(f"The archive does not contain {config['zlib_makefile']}: {source_root}")
    if force:
        clean_outputs(source_root)

    command = f'nmake /nologo -f {config["zlib_makefile"]} zlib.lib'
    compiler = msvc_version()
    completed = run_msvc(command, source_root)
    static_library = source_root / "zlib.lib"
    objects = sorted(source_root.glob("*.obj"))
    if not static_library.is_file() or not objects:
        raise RuntimeError("zlib build completed without zlib.lib and object files")

    metadata = {
        "zlib_version": config["zlib_version"],
        "source_archive_url": config["zlib_archive_url"],
        "source_archive_sha256": archive_digest,
        "source_root": str(source_root),
        "static_library": str(static_library),
        "object_files": [str(path) for path in objects],
        "compiler": "MSVC",
        "compiler_version": compiler,
        "architecture": config["architecture"],
        "configuration": config["configuration"],
        "linkage": config["linkage"],
        "make_command": command,
        "cflags_from_upstream_makefile": config["zlib_cflags"],
        "warning_flags_from_upstream_makefile": config["zlib_warning_flags"],
        "build_stdout_tail": completed.stdout[-4000:],
        "build_stderr_tail": completed.stderr[-4000:],
    }
    metadata_path = ZLIB_BUILD_ROOT / "metadata.json"
    write_json(metadata_path, metadata)
    print(f"[+] Built zlib {config['zlib_version']} with {len(objects)} object files")
    print(f"[+] Static library: {static_library}")
    print(f"[+] Metadata: {metadata_path}")
    return metadata


def main() -> int:
    """Parse build options and execute the reproducible zlib build stage."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="rebuild from the extracted source")
    args = parser.parse_args()
    config = load_config()
    build(config, args.force)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
