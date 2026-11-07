#!/usr/bin/env python3
"""Prepare a conservative DESR-7000 PP.DVRISO test payload, not an XMB installer.

Retains upstream OSDMenu/config/modules from the user's v0.3 release.
Substitutes HDD-aware Manager + Neutrino SYSTEM/ENGLISH FIX95 artifacts.
Leaves BOOT.CNF absent so the new HDD Manager creates it only after X.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import sys
import zipfile

from build_usb_bundle_v08 import inspect_legacy, FOLDER as LEGACY_BASE

ROOT = "PSX-DVR-ISO-Manager-v1.0-HDD/"
APP = ROOT + "PP.DVRISO-FILES/"
LIMIT = 80 * 1024 * 1024

REQUIRED = (
    "BOOT.ELF",
    "config/system.toml",
    "config/bsd-dvr.toml",
    "modules/ee_core.elf",
    "modules/dvrfile_fhi.irx",
    "modules/dvrdrv.irx",
    "modules/dvrfile.irx",
    "modules/dev9_ns.irx",
)
REMOVE_LEGACY = {
    "BOOT.CNF",
    "BOOT_RECOVERY.CNF",
    "DVR_TEST.CNF",
    "COVERS.IDX",
    "COVERS.NEW",
    "COVERS.BAK",
    "COVER_SCAN.LOG",
    "DVR_ISO_MANAGER.ELF",
    "neutrino.elf",
    "neutrino_lang.elf",
    "modules/ee_core_lang.elf",
    "config/osdlang.toml",
    "NET_PROBE.CNF",
    "BOOT.NEXT.CNF",
    "BOOT.PREV.CNF",
}


def read_elf(path: Path, minimum: int = 4096) -> bytes:
    b = path.read_bytes()
    if len(b) < minimum or b[:4] != b"\x7fELF" or b[4:6] != b"\x01\x01":
        raise ValueError(f"Not a sizeable PS2 ELF: {path}")
    if int.from_bytes(b[18:20], "little") != 8:
        raise ValueError(f"Not MIPS ELF: {path}")
    return b


def build(args: argparse.Namespace) -> None:
    with zipfile.ZipFile(args.legacy) as z:
        if z.testzip() is not None:
            raise ValueError("Legacy v0.3 source ZIP CRC failure")
        old = inspect_legacy(z)
    retained = {key[len(LEGACY_BASE):]: value for key, value in old.items()
                if key.startswith(LEGACY_BASE)}
    for key in REQUIRED:
        if key not in retained or not retained[key]:
            raise ValueError(f"Missing proven v0.3 dependency: {key}")
    payload = {k: v for k, v in retained.items()
               if k not in REMOVE_LEGACY and not k.endswith(".PART")}
    payload.update({
        "DVR_ISO_MANAGER_HDD.ELF": read_elf(args.manager, 100_000),
        "neutrino_hdd.elf": read_elf(args.system, 50_000),
        "neutrino_hdd_lang.elf": read_elf(args.english, 50_000),
        "modules/ee_core_lang.elf": read_elf(args.core),
        "config/osdlang.toml": args.osdlang.read_bytes(),
        "NET_PROBE.CNF": args.network.read_bytes(),
    })
    # A PFS-mounted OSDMenu must be supplied an explicit hdd0:PP.DVRISO/BOOT.CNF
    # by the Manager, never the old USB BOOT.CNF.
    for absent in ("BOOT.CNF", "neutrino.elf", "neutrino_lang.elf"):
        if absent in payload:
            raise ValueError(f"Unexpected legacy runtime retained: {absent}")
    for name in payload:
        p = PurePosixPath(name)
        if (name.startswith("/") or ".." in p.parts or "\\" in name
                or name.endswith("/") or not name):
            raise ValueError(f"Unsafe file path: {name}")
    names = [name.lower() for name in payload]
    if len(names) != len(set(names)):
        raise ValueError("Case-insensitive collision")
    if sum(len(v) for v in payload.values()) > LIMIT:
        raise ValueError("Unexpectedly large package")

    # Run-time bytes must point exclusively to PP.DVRISO, not the test partition.
    for name in ("neutrino_hdd.elf", "neutrino_hdd_lang.elf"):
        b = payload[name]
        if b"hdd0:PP.DVRISO" not in b or b"PP.TESTDVR" in b:
            raise ValueError(f"Unsafe partition target: {name}")
        if b"HDD FIX95 ISO argument/path mismatch" not in b:
            raise ValueError(f"FIX95 protection missing: {name}")
    manager_file = "DVR_ISO_MANAGER_HDD.ELF"
    if b"hdd0:PP.DVRISO" not in payload[manager_file] or b"PP.TESTDVR" in payload[manager_file]:
        raise ValueError("Wrong Manager partition")
    if (b"pfs0:/neutrino_hdd.elf" in payload[manager_file] or
            b"pfs0:/neutrino_hdd_lang.elf" in payload[manager_file]):
        raise ValueError("Wrong Neutrino PFS0 path")
    # The interactive Manager MUST reference BOTH binaries on its own
    # pfs1: mount. OSDMenu/Neutrino themselves later use pfs0:.
    test_manager = payload[manager_file]
    if b"1.0 HDD NETFIX" not in test_manager:
        raise ValueError("Rollback Manager must include validated NETFIX")
    for required_path in (
            b"pfs1:/neutrino_hdd.elf",
            b"pfs1:/neutrino_hdd_lang.elf"):
        if required_path not in test_manager:
            raise ValueError(f"Wrong HDD TEST Neutrino mount: {required_path!r}")
    if b"ee_core_lang.elf" not in payload["config/osdlang.toml"]:
        raise ValueError("Language config does not reference English EE core")
    note = args.readme.read_bytes()
    members = {APP + key: value for key, value in payload.items()}
    members[ROOT + "START_HERE_PL.txt"] = note
    dirs = {
        ROOT, APP, APP + "ART/", APP + "config/", APP + "modules/",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=7) as z:
        for directory in sorted(dirs):
            z.writestr(directory, b"")
        for name, data in sorted(members.items()):
            z.writestr(name, data)
    with zipfile.ZipFile(args.output) as z:
        if z.testzip() is not None:
            raise ValueError("Generated ZIP failed CRC")
        if set(z.namelist()) != set(members) | dirs:
            raise ValueError("Missing/extra archive entries")
        for key, value in members.items():
            if z.read(key) != value:
                raise ValueError(f"Data mismatch: {key}")
    print("PASS: one HDD Manager + NETFIX rollback; no preview; BOOT.ELF and dual FIX95")
    print("PAYLOAD FILES:", len(payload))
    print("BOOT.CNF: deliberately absent; generated after X on PP.DVRISO")
    print("ZIP:", args.output)
    print("SHA256:", hashlib.sha256(args.output.read_bytes()).hexdigest())


def main() -> None:
    p = argparse.ArgumentParser()
    for item in ("legacy", "manager", "system", "english",
                 "core", "osdlang", "network", "readme", "output"):
        p.add_argument("--" + item, type=Path, required=True)
    try:
        build(p.parse_args())
    except (OSError, ValueError, zipfile.BadZipFile) as err:
        print("ERROR:", err, file=sys.stderr)
        raise SystemExit(1) from err


if __name__ == "__main__":
    main()
