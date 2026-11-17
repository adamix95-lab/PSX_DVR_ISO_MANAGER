#!/usr/bin/env python3
"""Stage complete pre-release PDIM 1.2 HDD automatic/manual packages.

One application partition: PP.DVRISO. Source runtime assets come from the
verified official HDD 1.0 release; only the Manager is updated to native
HTTPS 1.2. XMB signed KELF is reused unchanged. No HDD writes by this tool.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re
import zipfile
import zlib

PART = "PP.DVRISO"
SOURCE = "PDIM_HDD_V12_INSTALLER"
FILES = SOURCE + "/FILES/"
XMB = SOURCE + "/XMB/"
MAX_FILE = 16 * 1024 * 1024
XMB_PFS = ("EXECUTE.ELF", "EXECUTE.CNF",
           "res/info.sys", "res/jkt_001.png", "res/jkt_002.png")
XMB_HEADER = ("system.cnf", "icon.sys", "info.sys", "jkt_001.png", "jkt_002.png")
BLOCKED_NAMES = {
    "BOOT.CNF", "BOOT.NEXT.CNF", "BOOT.PREV.CNF", "BOOT_RECOVERY.CNF",
    "COVERS.IDX", "COVERS.NEW", "COVERS.BAK", "COVER_SCAN.LOG",
    "DVR_TEST.CNF", "V12_NET.CNF", "PDIM12.INSTALLED", "PDIM12.VERIFIED"
}
CRITICAL = {
    "DVR_ISO_MANAGER_HDD.ELF", "neutrino_hdd.elf",
    "neutrino_hdd_lang.elf", "modules/ee_core_lang.elf",
    "BOOT.ELF", "NET_PROBE.CNF", "config/osdlang.toml",
    "modules/dvrdrv.irx", "modules/dvrfile.irx",
    "modules/dvrfile_fhi.irx", "config/bsd-dvr.toml",
    *XMB_PFS, "N4E.CA"
}

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def check_elf(data: bytes, label: str, require_partition: bool = True) -> None:
    if (len(data) < 20000 or data[:4] != b"\x7fELF"
        or data[4:6] != b"\x01\x01"
        or int.from_bytes(data[18:20], "little") != 8):
        raise ValueError(f"{label}: not ELF32 little-endian MIPS")
    if require_partition:
        if b"hdd0:PP.DVRISO" not in data:
            raise ValueError(f"{label}: PP.DVRISO not embedded")
        if b"PP.DVRISO12" in data or b"PP.TESTDVR" in data:
            raise ValueError(f"{label}: obsolete test target embedded")

def zip_contents(path: Path) -> dict[str, bytes]:
    with zipfile.ZipFile(path) as z:
        if z.testzip() is not None:
            raise ValueError(f"{path}: source ZIP CRC failure")
        return {info.filename: z.read(info) for info in z.infolist()
                if not info.is_dir()}

def match_suffix(objects: dict[str, bytes], tail: str) -> bytes:
    keys = [k for k in objects if k == tail or k.endswith("/" + tail)]
    if len(keys) != 1:
        raise ValueError(f"Expected exactly one {tail!r}: {keys}")
    return objects[keys[0]]

def get_hdd_files(items: dict[str, bytes]) -> dict[str, bytes]:
    marker = "PP.DVRISO-FILES/"
    out = {}
    for key, data in items.items():
        if marker not in key:
            continue
        relative = key.split(marker, 1)[1]
        if not relative or relative.endswith("/"):
            continue
        if (relative in BLOCKED_NAMES or relative.startswith("ART/")
            or relative.startswith("/") or ".." in relative or "\\" in relative):
            if relative in BLOCKED_NAMES or relative.startswith("ART/"):
                continue
            raise ValueError("Unsafe source entry " + relative)
        out[relative] = data
    if not out:
        raise ValueError("Official HDD 1.0 ZIP lacks PP.DVRISO-FILES")
    return out

def assert_sane_payload(path: str, data: bytes) -> None:
    if (not data or len(data) > MAX_FILE or len(path) >= 112
        or not re.fullmatch(r"[A-Za-z0-9_.\-/]+", path)
        or path[0] in "./" or path.endswith("/") or ".." in path
        or "//" in path or path in BLOCKED_NAMES or path.startswith("ART/")):
        raise ValueError(f"Unsafe entry: {path}")

def payload(hdd_zip: Path, xmb_zip: Path, manager_elf: Path,
            ca_bundle: Path) -> tuple[dict[str, bytes], dict[str, bytes]]:
    hdd = zip_contents(hdd_zip)
    xmb = zip_contents(xmb_zip)
    out = get_hdd_files(hdd)
    manager = manager_elf.read_bytes()
    check_elf(manager, "PDIM 1.2 HDD HTTPS Manager")
    if b"1.2 HDD" not in manager or b"N6GCPUR3" not in manager[-16:]:
        raise ValueError("Not a self-contained 1.2 HDD HTTPS Manager")
    out["DVR_ISO_MANAGER_HDD.ELF"] = manager
    for name in ("neutrino_hdd.elf", "neutrino_hdd_lang.elf"):
        check_elf(out[name], name)
        if b"HDD FIX95 ISO argument/path mismatch" not in out[name]:
            raise ValueError(name + " is not the known FIX95 runtime")
    check_elf(out["modules/ee_core_lang.elf"], "English EE core", False)
    if b"ee_core_lang.elf" not in out["config/osdlang.toml"]:
        raise ValueError("Missing English core config")
    out["N4E.CA"] = ca_bundle.read_bytes()

    # The signed XMB launcher is immutable; its existing CNF targets PP.DVRISO.
    launcher = match_suffix(xmb, "PFS_COPY_TO_PP.DVRISO/EXECUTE.ELF")
    if len(launcher) < 100_000:
        raise ValueError("Trusted XMB KELF unexpectedly short")
    out["EXECUTE.ELF"] = launcher
    cnf = match_suffix(xmb, "PFS_COPY_TO_PP.DVRISO/EXECUTE.CNF")
    expected = b"path=hdd0:PP.DVRISO:pfs:/DVR_ISO_MANAGER_HDD.ELF"
    if cnf.count(expected) != 1 or b"PP.DVRISO12" in cnf:
        raise ValueError("Wrong XMB boot configuration")
    out["EXECUTE.CNF"] = cnf

    header = {n: match_suffix(xmb, f"__Headers/PP.DVRISO/{n}")
              for n in XMB_HEADER}
    if b"BOOT2 = pfs:/EXECUTE.ELF" not in header["system.cnf"]:
        raise ValueError("Unrecognized XMB system.cnf")
    icon = header["icon.sys"]
    if not icon.startswith(b"PS2X") or icon.count(b"title1=HDD 1.0") != 1:
        raise ValueError("Unrecognized XMB icon.sys")
    # Only the version label changes. ppaa_plan explicitly recognizes the
    # exact legacy 1.0 header to allow updating the installed XMB entry.
    header["icon.sys"] = icon.replace(b"title1=HDD 1.0", b"title1=HDD 1.2")
    info = header["info.sys"]
    if (info.count(b"title = PSX DVR ISO MANAGER") != 1
        or info.count(b"title_id = PSX-DVRISO") != 1):
        raise ValueError("Unrecognized XMB info.sys")
    out["res/info.sys"] = info
    out["res/jkt_001.png"] = header["jkt_001.png"]
    out["res/jkt_002.png"] = header["jkt_002.png"]
    for name, data in out.items():
        assert_sane_payload(name, data)
    if not CRITICAL.issubset(out):
        raise ValueError("Missing files: " + str(sorted(CRITICAL - set(out))))
    if len(out) != 84:
        raise ValueError(f"Expected fixed 84 files (79+5), got {len(out)}")
    if len([n for n in out if n in XMB_PFS]) != 5:
        raise ValueError("Missing exact XMB phase")
    return out, header

def create_manifest(files: dict[str, bytes]) -> bytes:
    lines = "".join(f"{p}|{len(files[p])}|{zlib.crc32(files[p]) & 0xffffffff:08x}\n"
                    for p in sorted(files))
    if len(lines) >= 30000:
        raise ValueError("Manifest too large")
    return lines.encode()

def write_zip(path: Path, files: dict[str, bytes]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=7) as z:
        for name, data in sorted(files.items()):
            z.writestr(name, data)
    with zipfile.ZipFile(path) as z:
        if z.testzip() is not None or set(z.namelist()) != set(files):
            raise ValueError("Generated ZIP verification failed")
        for name, data in files.items():
            if z.read(name) != data:
                raise ValueError("Readback mismatch: " + name)

def build(args: argparse.Namespace) -> None:
    out, header = payload(args.hdd, args.xmb, args.manager, args.ca)
    manifest = create_manifest(out)
    installer = args.installer.read_bytes()
    check_elf(installer, "PDIM 1.2 HDD installer", True)
    if b"PDIM_HDD_V12_INSTALLER" not in installer:
        raise ValueError("Installer USB package prefix missing")
    automatic = {
        SOURCE + "/PDIM_HDD_V12_INSTALLER.ELF": installer,
        SOURCE + "/MANIFEST.TXT": manifest,
        SOURCE + "/V12_NET.CNF.example": args.config.read_bytes(),
        SOURCE + "/README_PL.txt": args.readme_pl.read_bytes(),
        SOURCE + "/README.txt": args.readme.read_bytes(),
        SOURCE + "/SOURCE_COMMIT": (args.commit + "\n").encode()
    }
    for name, data in out.items():
        automatic[FILES + name] = data
    for name, data in header.items():
        automatic[XMB + name] = data
    automatic[SOURCE + "/ASSETS_CREDITS.txt"] = (
        "Official PDIM 1.0 signed EXECUTE.ELF and XMB artwork unchanged.\n"
        "PDIM 1.2 HDD HTTPS Manager rebuilt from open source.\n"
        "Pre-release test only. No public 1.2 release is authorized.\n").encode()
    automatic[SOURCE + "/SHA256SUMS"] = "".join(
        f"{sha(data)}  {name[len(SOURCE)+1:]}\n"
        for name, data in sorted(automatic.items())
        if name.startswith(SOURCE + "/")).encode()
    automatic[FILES + "ART/"] = b""
    manual = {
        "PP.DVRISO-FILES/" + name: data for name, data in out.items()
    }
    manual["PP.DVRISO-FILES/ART/"] = b""
    manual.update({
        "XMB_HEADERS/PP.DVRISO/" + name: data for name, data in header.items()
    })
    manual.update({
        "README.md": args.readme.read_bytes(),
        "README_PL.md": args.readme_pl.read_bytes(),
        "V12_NET.CNF.example": args.config.read_bytes(),
        "MANIFEST.TXT": manifest,
        "SOURCE_COMMIT": (args.commit + "\n").encode()
    })
    write_zip(args.auto_output, automatic)
    write_zip(args.manual_output, manual)
    print("PASS: production PP.DVRISO, 79 core + 5 XMB, 84 CRC32 manifest entries")
    print("Automatic (TEST ONLY):", args.auto_output, sha(args.auto_output.read_bytes()))
    print("Manual (TEST ONLY):", args.manual_output, sha(args.manual_output.read_bytes()))

def main() -> None:
    p = argparse.ArgumentParser()
    for name in ("hdd", "xmb", "manager", "installer", "ca",
                 "config", "readme", "readme_pl", "auto_output",
                 "manual_output"):
        p.add_argument("--" + name.replace("_", "-"), type=Path, required=True)
    p.add_argument("--commit", required=True)
    build(p.parse_args())

if __name__ == "__main__":
    main()
