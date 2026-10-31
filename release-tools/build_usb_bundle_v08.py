#!/usr/bin/env python3
"""Assemble a verifiable full USB bundle from this repository's v0.3 and v0.8 releases.

Original OSDMenu/Neutrino modules and configs are copied byte-for-byte;
only BOOT.elf's spelling is normalized to BOOT.ELF to match the Manager.
The v0.8 Manager ELF comes from the already published tested release.
No game ISOs, artwork or personal USB state are included.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath

FOLDER = "DESR_DVR/"
OLD_MANAGER = "DVR_ISO_MANAGER.ELF"
NEW_MANAGER = FOLDER + OLD_MANAGER
MAX_FILES = 250
MAX_UNPACKED = 80 * 1024 * 1024

REQUIRED_LEGACY = {
    "BOOT.elf",
    "BOOT.CNF",
    "BOOT_RECOVERY.CNF",
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


def zip_entry(name: str, is_dir: bool = False) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, date_time=(2026, 10, 31, 0, 0, 0))
    info.create_system = 3
    info.external_attr = ((0o755 if is_dir else 0o644) << 16)
    info.compress_type = zipfile.ZIP_DEFLATED
    return info


def inspect_legacy(z: zipfile.ZipFile) -> dict[str, bytes]:
    """Fail closed if the legacy release does not have the expected layout."""
    items = z.infolist()
    if len(items) < len(REQUIRED_LEGACY) + 1 or len(items) > MAX_FILES:
        raise ValueError("Unexpected v0.3 archive entry count")
    result: dict[str, bytes] = {}
    names: set[str] = set()
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
            # Superseded v0.3 manager at archive root, not part of USB package.
            continue
        if not raw_name.startswith(FOLDER):
            raise ValueError(f"Unexpected file outside DESR_DVR: {raw_name}")
        relative = raw_name[len(FOLDER):]
        if not relative or relative.endswith("/"):
            raise ValueError("Malformed legacy release entry")
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
    names_original = {x.filename.removeprefix(FOLDER)
                      for x in items if x.filename.startswith(FOLDER) and not x.is_dir()}
    missing = REQUIRED_LEGACY - names_original
    if missing:
        raise ValueError(f"Missing original v0.3 files: {sorted(missing)}")
    if NEW_MANAGER in result:
        raise ValueError("v0.3 already has manager ELF inside DESR_DVR")
    return result


def create_bundle(
    legacy_zip: Path,
    manager_elf: Path,
    network_config: Path,
    readme_pl: Path,
    readme_en: Path,
    destination: Path,
) -> dict[str, bytes]:
    elf = manager_elf.read_bytes()
    if len(elf) < 100_000 or not elf.startswith(b"\x7fELF"):
        raise ValueError("Current Manager is not a valid sizeable ELF")
    with zipfile.ZipFile(legacy_zip) as z:
        z.testzip()
        members = inspect_legacy(z)
    net = network_config.read_bytes()
    required_settings = ("IP=", "NETMASK=", "GATEWAY=", "SERVER_IP=", "SERVER_PORT=")
    net_lines = net.decode("ascii").splitlines()
    if len(net) > 512 or len(net_lines) != 5 or any(
        not any(line.startswith(key) for line in net_lines) for key in required_settings
    ):
        raise ValueError("Incomplete NET_PROBE.CNF example")
    members.update({
        NEW_MANAGER: elf,
        FOLDER + "NET_PROBE.CNF": net,
        FOLDER + "START_HERE_PL.txt": readme_pl.read_bytes(),
        FOLDER + "START_HERE_EN.txt": readme_en.read_bytes(),
    })
    casefold = [name.casefold() for name in members]
    if len(casefold) != len(set(casefold)):
        raise ValueError("Case-insensitive FAT path collision in final package")
    manifest = "\n".join(
        f"{hashlib.sha256(data).hexdigest()}  {name}"
        for name, data in sorted(members.items())
    ) + "\n"
    members[FOLDER + "SHA256SUMS.txt"] = manifest.encode("ascii")

    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=8) as output:
        output.writestr(zip_entry(FOLDER, is_dir=True), b"")
        output.writestr(zip_entry(FOLDER + "ART/", is_dir=True), b"")
        for name, data in sorted(members.items()):
            output.writestr(zip_entry(name), data)
    with zipfile.ZipFile(destination) as check:
        if check.testzip() is not None:
            raise RuntimeError("Generated ZIP failed CRC verification")
        for name, data in members.items():
            if check.read(name) != data:
                raise RuntimeError(f"Generated ZIP does not preserve: {name}")
        if OLD_MANAGER in check.namelist():
            raise RuntimeError("Legacy root manager must not be shipped")
        if not check.read(NEW_MANAGER).startswith(b"\x7fELF"):
            raise RuntimeError("v0.8 Manager ELF missing in output")
        if FOLDER + "ART/" not in check.namelist():
            raise RuntimeError("Missing empty ART directory")
    print(f"USB PACKAGE: {destination}")
    print(f"ORIGINAL v0.3 FILES COPIED: {len(members) - 5}")
    print(f"PACKAGED FILES: {len(members)}")
    print(f"MANAGER ELF SHA-256: {hashlib.sha256(elf).hexdigest()}")
    print(f"PACKAGE SHA-256: {hashlib.sha256(destination.read_bytes()).hexdigest()}")
    return members


def main() -> None:
    ap = argparse.ArgumentParser(description="Create complete v0.8 USB ZIP")
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
        )
    except (ValueError, OSError, RuntimeError, zipfile.BadZipFile) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


if __name__ == "__main__":
    main()
