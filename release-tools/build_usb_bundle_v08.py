#!/usr/bin/env python3
"""Rebuild the user-approved clean v0.8 USB package from verified release assets.

The approved upload contains 81 files beneath:
  PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE/DESR_DVR/

All file contents and paths are checked against a SHA-256 fingerprint
taken from that uploaded archive. ZIP metadata may differ when rebuilt;
the directory tree and every extracted file must match byte-for-byte.
"""
from __future__ import annotations

import argparse
import hashlib
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath

LEGACY_FOLDER = "DESR_DVR/"
ZIP_ROOT = "PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE/"
FOLDER = ZIP_ROOT + LEGACY_FOLDER
OLD_MANAGER = "DVR_ISO_MANAGER.ELF"
NEW_MANAGER = FOLDER + OLD_MANAGER
MAX_FILES = 250
MAX_UNPACKED = 80 * 1024 * 1024

# Files deliberately removed by the user from the uploaded clean ZIP.
# Preserve all modules, Neutrino, configuration TOMLs and boot components.
EXCLUDED_V03_FILES = frozenset({
    "BOOT_RECOVERY.CNF",
    "DVR_TEST.CNF",
    "info.sys",
    "jkt_001.png",
    "jkt_002.png",
    "jkt_cp.png",
})
REQUIRED_LEGACY = {
    "BOOT.elf",
    "BOOT.CNF",
    "neutrino.elf",
    "neutrino_lang.elf",
    "config/system.toml",
    "config/bsd-dvr.toml",
    "config/osdlang.toml",
    "modules/dvrdrv.irx",
    "modules/dvrfile.irx",
    "modules/dvrfile_fhi.irx",
    "modules/ee_core.elf",
    "modules/ee_core_lang.elf",
    "modules/dev9_ns.irx",
}

# SHA-256 over (relative filename + NUL + SHA-256 of file bytes), sorted.
# Obtained from the exact user-supplied 2026-11-01 ZIP (81 files).
USER_UPLOADED_FILES = 81
USER_UPLOADED_CONTENT_FINGERPRINT = (
    "f6ac34abcd6510623251a221db0834f3254f3126c7f3c5e158fe7cd02bdf6401"
)
USER_UPLOADED_ZIP_SHA256 = (
    "0cec6615bc55efe03d909b14d0a18e8d20eec4b54c556e3ff992b355b0899721"
)


def zip_entry(name: str, *, is_dir: bool = False) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, date_time=(2026, 10, 31, 0, 0, 0))
    info.create_system = 3
    info.external_attr = ((0o755 if is_dir else 0o644) << 16)
    info.compress_type = zipfile.ZIP_STORED if is_dir else zipfile.ZIP_DEFLATED
    return info


def inspect_legacy(z: zipfile.ZipFile) -> dict[str, bytes]:
    """Validate the real v0.3 ZIP; omit only named user-removed files."""
    items = z.infolist()
    if len(items) < len(REQUIRED_LEGACY) + 1 or len(items) > MAX_FILES:
        raise ValueError("Unexpected v0.3 archive entry count")
    result: dict[str, bytes] = {}
    names: set[str] = set()
    original: set[str] = set()
    total = 0
    for info in items:
        raw_name = info.filename
        name = PurePosixPath(raw_name)
        if raw_name.startswith("/") or "\\" in raw_name or ".." in name.parts:
            raise ValueError(f"Unsafe archive member: {raw_name!r}")
        mode = (info.external_attr >> 16) & 0o170000
        if stat.S_ISLNK(mode):
            raise ValueError("Symlink in legacy archive not allowed")
        if info.is_dir():
            continue
        if raw_name == OLD_MANAGER:
            continue  # Old v0.3 ELF lives outside DESR_DVR.
        if not raw_name.startswith(LEGACY_FOLDER):
            raise ValueError(f"Unexpected file outside DESR_DVR: {raw_name}")
        relative = raw_name[len(LEGACY_FOLDER):]
        if not relative or relative.endswith("/"):
            raise ValueError("Malformed legacy release entry")
        original.add(relative)
        if relative in EXCLUDED_V03_FILES:
            continue
        if relative.lower() == "boot.elf":
            relative = "BOOT.ELF"
        normalized = relative.casefold()
        if normalized in names:
            raise ValueError(f"Case-insensitive FAT filename collision: {relative}")
        names.add(normalized)
        total += info.file_size
        if total > MAX_UNPACKED or info.file_size > MAX_UNPACKED:
            raise ValueError("Unexpectedly large release archive")
        result[FOLDER + relative] = z.read(info)
    missing = REQUIRED_LEGACY - original
    if missing:
        raise ValueError(f"Missing original v0.3 files: {sorted(missing)}")
    if NEW_MANAGER in result:
        raise ValueError("v0.3 already contains the new Manager ELF")
    return result


def content_fingerprint(members: dict[str, bytes]) -> str:
    """Hash paths and uncompressed bytes, independent of ZIP metadata."""
    digest = hashlib.sha256()
    for full_name, data in sorted(members.items()):
        if not full_name.startswith(FOLDER):
            raise ValueError(f"Unsafe package member: {full_name}")
        relative = full_name[len(FOLDER):]
        digest.update(relative.encode("utf-8"))
        digest.update(b"\x00")
        digest.update(hashlib.sha256(data).digest())
    return digest.hexdigest()


def create_bundle(
    legacy_zip: Path,
    manager_elf: Path,
    network_config: Path,
    readme_pl: Path,
    readme_en: Path,
    destination: Path,
    *,
    verify_user_upload: bool = False,
) -> dict[str, bytes]:
    elf = manager_elf.read_bytes()
    if len(elf) < 100_000 or not elf.startswith(b"\x7fELF"):
        raise ValueError("Current Manager is not a valid sizeable ELF")
    with zipfile.ZipFile(legacy_zip) as z:
        if z.testzip() is not None:
            raise ValueError("Invalid original v0.3 ZIP CRC")
        members = inspect_legacy(z)

    net = network_config.read_bytes()
    required_settings = ("IP=", "NETMASK=", "GATEWAY=", "SERVER_IP=", "SERVER_PORT=")
    net_lines = net.decode("ascii").splitlines()
    if len(net) > 512 or len(net_lines) != 5 or any(
        not any(line.startswith(key) for line in net_lines)
        for key in required_settings
    ):
        raise ValueError("Incomplete NET_PROBE.CNF")
    members.update({
        NEW_MANAGER: elf,
        FOLDER + "NET_PROBE.CNF": net,
        FOLDER + "START_HERE_PL.txt": readme_pl.read_bytes(),
        FOLDER + "START_HERE_EN.txt": readme_en.read_bytes(),
    })
    casefold = [name.casefold() for name in members]
    if len(casefold) != len(set(casefold)):
        raise ValueError("Case-insensitive FAT collision in package")

    fingerprint = content_fingerprint(members)
    if verify_user_upload and (
        len(members) != USER_UPLOADED_FILES
        or fingerprint != USER_UPLOADED_CONTENT_FINGERPRINT
    ):
        raise ValueError(
            "Rebuilt ZIP content DOES NOT match the user-supplied clean ZIP: "
            f"{len(members)} files, SHA-256 fingerprint {fingerprint}"
        )

    directories = (
        ZIP_ROOT,
        FOLDER,
        FOLDER + "ART/",
        FOLDER + "config/",
        FOLDER + "modules/",
    )
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=8) as output:
        for directory in directories:
            output.writestr(zip_entry(directory, is_dir=True), b"")
        for name, data in sorted(members.items()):
            output.writestr(zip_entry(name), data)
    with zipfile.ZipFile(destination) as check:
        if check.testzip() is not None:
            raise RuntimeError("Generated ZIP failed CRC verification")
        if set(check.namelist()) != set(directories) | set(members):
            raise RuntimeError("Unexpected/missing ZIP entries")
        for name, data in members.items():
            if check.read(name) != data:
                raise RuntimeError(f"Generated ZIP changed member: {name}")
        if OLD_MANAGER in check.namelist():
            raise RuntimeError("Legacy root manager must not be shipped")
        if not check.read(NEW_MANAGER).startswith(b"\x7fELF"):
            raise RuntimeError("Current Manager ELF missing")
    print(f"USB PACKAGE: {destination}")
    print(f"FILES: {len(members)}; DIRECTORIES: {len(directories)}")
    print(f"CONTENT FINGERPRINT: {fingerprint}")
    if verify_user_upload:
        print("PASS: all filenames and extracted bytes match the user upload")
        print("Note: ZIP container SHA-256 may differ due to recompression/metadata.")
    print(f"MANAGER ELF SHA-256: {hashlib.sha256(elf).hexdigest()}")
    print(f"ZIP SHA-256: {hashlib.sha256(destination.read_bytes()).hexdigest()}")
    return members


def main() -> None:
    ap = argparse.ArgumentParser(description="Create approved clean v0.8 USB ZIP")
    ap.add_argument("--legacy", required=True, type=Path)
    ap.add_argument("--manager", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    args = ap.parse_args()
    root = Path(__file__).resolve().parent
    try:
        create_bundle(
            args.legacy,
            args.manager,
            root / "NET_PROBE.CNF",
            root / "USB_README_PL.txt",
            root / "USB_README_EN.txt",
            args.output,
            verify_user_upload=True,
        )
    except (ValueError, OSError, RuntimeError, zipfile.BadZipFile) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


if __name__ == "__main__":
    main()
