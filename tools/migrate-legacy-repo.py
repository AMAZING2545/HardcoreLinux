#!/usr/bin/env python3
"""Convert existing Hardcore Linux tar packages into yspkg packages."""

from __future__ import annotations

import argparse
import gzip
import io
import json
import os
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path


def safe_extract(archive: Path, destination: Path) -> None:
    destination = destination.resolve()
    with tarfile.open(archive, "r:*") as tar:
        for member in tar.getmembers():
            target = (destination / member.name).resolve()
            if os.path.commonpath((destination, target)) != str(destination):
                raise RuntimeError(f"unsafe archive member: {member.name}")
            if member.issym() or member.islnk():
                link_target = (destination / member.linkname).resolve()
                if os.path.commonpath((destination, link_target)) != str(destination):
                    raise RuntimeError(f"unsafe archive link: {member.name}")
        tar.extractall(destination)


def locate_pkinfo(root: Path) -> Path:
    candidates = sorted(root.glob("etc/installer/*/pkinfo"))
    if not candidates:
        raise RuntimeError("package has no etc/installer/*/pkinfo")
    return candidates[0]


def parse_metadata(pkinfo: Path) -> tuple[str, str, list[str]]:
    tokens = pkinfo.read_text(encoding="utf-8", errors="replace").split()
    if not tokens:
        raise RuntimeError(f"empty pkinfo: {pkinfo}")
    name = tokens[0]
    version = tokens[1] if len(tokens) > 1 else "0"
    return name, version, tokens[2:]


def patch_metadata(package: Path, dependencies: list[str]) -> None:
    with gzip.open(package, "rb") as source:
        original = tarfile.open(fileobj=source, mode="r:")
        members = original.getmembers()
        payloads: dict[str, bytes] = {}
        for member in members:
            if member.isfile():
                extracted = original.extractfile(member)
                payloads[member.name] = extracted.read() if extracted else b""
        metadata = json.loads(payloads["metadata.json"].decode("utf-8"))
        metadata["dependencies"] = dependencies
        metadata["kind"] = "system"
        metadata["format"] = "yspkg"
        metadata["description"] = metadata.get("description") or metadata["name"]
        payloads["metadata.json"] = (
            json.dumps(metadata, indent=2, sort_keys=True).encode("utf-8") + b"\n"
        )
        headers = {member.name: member for member in members}
        original.close()

    temporary = package.with_suffix(".tmp.yspkg")
    with gzip.open(temporary, "wb", compresslevel=9) as target_gz:
        with tarfile.open(fileobj=target_gz, mode="w") as target:
            for name, header in headers.items():
                data = payloads.get(name)
                header = tarfile.TarInfo.frombuf(
                    header.tobuf(format=tarfile.PAX_FORMAT),
                    encoding="utf-8",
                    errors="surrogateescape",
                )
                if data is not None:
                    header.size = len(data)
                target.addfile(
                    header,
                    io.BytesIO(data) if data is not None else None,
                )
    temporary.replace(package)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default="repo")
    parser.add_argument("--output", default="repo-yspm/releases/1")
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--release", default="1")
    parser.add_argument("--abi", default="yspm-abi-1")
    parser.add_argument("--arch", default="x86_64")
    parser.add_argument("--yspm", default="yspm")
    args = parser.parse_args()

    input_dir = Path(args.input).resolve()
    output_dir = Path(args.output).resolve()
    package_dir = output_dir / "packages"
    package_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="hardcore-legacy-") as temp_name:
        temp = Path(temp_name)
        for archive in sorted(input_dir.glob("*.tar")):
            if archive.name in {"flashpack.tar", "flashman_bin.tar"}:
                continue

            work = temp / archive.stem
            root = work / "root"
            root.mkdir(parents=True)

            try:
                safe_extract(archive, root)
                name, version, dependencies = parse_metadata(locate_pkinfo(root))
                shutil.rmtree(root / "etc" / "installer", ignore_errors=True)

                output = package_dir / f"{name}-{version}-{args.arch}.yspkg"
                subprocess.run(
                    [
                        args.yspm,
                        "build",
                        "--root",
                        str(root),
                        "--output",
                        str(output),
                        "--name",
                        name,
                        "--version",
                        version,
                        "--description",
                        name,
                        "--abi",
                        args.abi,
                        "--arch",
                        args.arch,
                        "--license",
                        "UNKNOWN",
                    ],
                    check=True,
                )
                patch_metadata(output, dependencies)
                print(f"converted {archive.name} -> {output.name}")
            except Exception as exc:
                print(f"skip {archive.name}: {exc}")

    index = output_dir / "index.json"
    subprocess.run(
        [
            args.yspm,
            "repo",
            "index",
            "--dir",
            str(package_dir),
            "--output",
            str(index),
            "--base-url",
            args.base_url,
            "--release",
            args.release,
            "--abi",
            args.abi,
        ],
        check=True,
    )
    print(index)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
