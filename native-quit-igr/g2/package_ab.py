#!/usr/bin/env python3
"""Package two isolated USB-only A/B test folders from OFFICIAL v1.0 USB release.

No HDD binaries are rebuilt or uploaded. Every source file except the G2 test
Manager and G2B Neutrino loaders is byte-identical to the official release.
The input archive is checksum-pinned, and any unexpected layout is rejected.
"""
import argparse
import hashlib
import io
from pathlib import Path
import struct
import zipfile

USB_RELEASE_SHA256 = "d3e2b08cdb22d33da9247e21bf0402e280b085693ccc9a623d29e34018f841c6"
TEST_FOLDERS = ("DESR_G2A", "DESR_G2B")
BOOT_FILENAME = "BOOT.CNF"
ORIGINAL_FILES = ("BOOT.ELF", "BOOT.CNF", "neutrino.elf", "neutrino_lang.elf",
                  "modules/ee_core.elf", "modules/ee_core_lang.elf",
                  "modules/dvrfile_fhi.irx", "config/bsd-dvr.toml",
                  "config/osdlang.toml")


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def elf_check(label: str, b: bytes):
    if len(b) < 35_000 or b[:6] != b"\x7fELF\x01\x01" or struct.unpack_from("<H", b, 18)[0] != 8:
        raise ValueError(f"{label}: not ELF32 MIPS or too small")


def load_stable(zip_path: Path) -> dict[str, bytes]:
    raw = zip_path.read_bytes()
    if sha(raw) != USB_RELEASE_SHA256:
        raise ValueError("Official 1.0 USB ZIP mismatch. REFUSE unverified input")
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        if z.testzip():
            raise ValueError("Corrupt official archive")
        entries = z.infolist()
        starts = [i.filename[:-len("neutrino.elf")] for i in entries
                  if i.filename.endswith("/DESR_DVR/neutrino.elf")]
        if len(starts) != 1:
            raise ValueError("Expected exactly one DESR_DVR prefix")
        prefix = starts[0]
        files = {}
        for item in entries:
            if item.is_dir():
                continue
            p = item.filename
            if not p.startswith(prefix) or "../" in p or "\\" in p:
                raise ValueError(f"Unexpected archive path {p}")
            name = p[len(prefix):]
            if not name or name.startswith("/") or ".." in Path(name).parts:
                raise ValueError(f"Invalid relative member: {name}")
            if name in files:
                raise ValueError(f"Duplicate member {name}")
            files[name] = z.read(p)
    for must in ORIGINAL_FILES:
        if must not in files:
            raise ValueError(f"Official USB input missing {must}")
    for name in ("neutrino.elf", "neutrino_lang.elf", "DVR_ISO_MANAGER.ELF"):
        elf_check(name, files[name])
    cnf = files[BOOT_FILENAME]
    if b"boot=neutrino" not in cnf:
        raise ValueError("Official BOOT.CNF cannot be used as initial backup")
    return files


def build(a: argparse.Namespace):
    baseline = load_stable(a.release)
    originals = {k: sha(baseline[k]) for k in ORIGINAL_FILES}
    patched = {
        "neutrino.elf": a.system.read_bytes(),
        "neutrino_lang.elf": a.english.read_bytes(),
    }
    for k, data in patched.items():
        elf_check(k, data)
        if data == baseline[k]:
            raise ValueError(f"Changed Neutrino {k} matches original unexpectedly")
    managers = {
        "DESR_G2A": a.manager_a.read_bytes(),
        "DESR_G2B": a.manager_b.read_bytes(),
    }
    for folder, b in managers.items():
        elf_check(folder + " Manager", b)
        if f"mass0:/{folder}/".encode() not in b:
            raise ValueError(f"Wrong Manager USB destination: {folder}")
        if b"mass0:/DESR_DVR/" in b or b"PP.TESTDVR" in b:
            raise ValueError(f"Manager still refers to original folder/old test: {folder}")
        if b"1.0 USB G2" not in b:
            raise ValueError(f"Test Manager has no experimental banner: {folder}")
        if b"dvr_pfs0:/ISO/" not in b:
            raise ValueError(f"Original DVR ISO backing path missing from Manager: {folder}")

    all_files = {}
    out_manifest = ["# QUIT G2 A/B — verified package", "", 
                    f"Official USB 1.0 input sha256: {USB_RELEASE_SHA256}",
                    "", "Original module hashes (same in BOTH folders):"]
    out_manifest.extend(f"{name}: {value}" for name, value in sorted(originals.items())
                        if name.startswith("modules/") or name.startswith("config/"))
    for folder in TEST_FOLDERS:
        test_files = baseline.copy()
        test_files["DVR_ISO_MANAGER.ELF"] = managers[folder]
        if folder == "DESR_G2B":
            test_files.update(patched)
        if any(k not in test_files for k in ORIGINAL_FILES):
            raise AssertionError("Missing runtime file")
        for rel, data in test_files.items():
            if rel == BOOT_FILENAME:
                # The initial CNF is from the official USB release; the
                # test manager creates an isolated CNF for the chosen game.
                if b"boot=neutrino" not in data:
                    raise ValueError("Invalid original BOOT.CNF")
            all_files[folder + "/" + rel] = data

    # Invariant: for each relative member, A and B differ ONLY in the Manager
    # and the two rebuilt Neutrino loaders. All copied configs, modules and
    # executable OSDMenu boot components remain identical in both folders.
    expected_diffs = {"DVR_ISO_MANAGER.ELF", "neutrino.elf", "neutrino_lang.elf"}
    actual_diffs = {
        rel for rel in baseline
        if all_files["DESR_G2A/" + rel] != all_files["DESR_G2B/" + rel]
    }
    if actual_diffs != expected_diffs:
        raise ValueError(f"A/B differs unexpectedly: {sorted(actual_diffs)}")
    for folder in TEST_FOLDERS:
        assert all_files[folder + "/BOOT.ELF"] == baseline["BOOT.ELF"]
        assert all_files[folder + "/modules/ee_core_lang.elf"] == baseline["modules/ee_core_lang.elf"]
        assert all_files[folder + "/modules/dvrfile_fhi.irx"] == baseline["modules/dvrfile_fhi.irx"]
    out_manifest.extend(["", "A/B differences only:", *sorted(expected_diffs),
                         "", "These builds are NOT hardware-tested.",
                         "Test folder changes cannot be used as proof of native QUIT functionality."])
    all_files["G2_MANIFEST.txt"] = ("\n".join(out_manifest) + "\n").encode("utf-8")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    # Restore explicit directory entries that exist in the original USB
    # release. In particular, ART/ may have no files and must still exist
    # before artwork operations or library scans.
    directory_entries = [
        folder + "/" + name for folder in TEST_FOLDERS
        for name in ("", "ART/", "config/", "modules/")
    ]
    with zipfile.ZipFile(a.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as z:
        for directory in directory_entries:
            z.writestr(directory, b"")
        for name, b in sorted(all_files.items()):
            z.writestr(name, b)
    with zipfile.ZipFile(a.output) as z:
        if z.testzip():
            raise ValueError("Final ZIP fails CRC")
        if len(z.namelist()) != len(all_files) + len(directory_entries):
            raise ValueError("Final ZIP misses files or directories")
        for directory in directory_entries:
            if directory not in z.namelist() or not z.getinfo(directory).is_dir():
                raise ValueError(f"ZIP directory missing: {directory}")
        for name in all_files:
            if z.read(name) != all_files[name]:
                raise ValueError(f"ZIP changed data: {name}")
    print(f"PASS: {len(baseline)} baseline runtime files per folder")
    print(f"PASS: only these A/B differences: {', '.join(sorted(actual_diffs))}")
    print("PASS: no XMB installer and no change to original DESR_DVR or HDD partition")
    print("OUTPUT:", a.output, "bytes", a.output.stat().st_size, "sha256", sha(a.output.read_bytes()))


def main():
    ap = argparse.ArgumentParser()
    for name in ("release", "manager_a", "manager_b", "system", "english", "output"):
        ap.add_argument("--" + name.replace("_", "-"), required=True, type=Path)
    build(ap.parse_args())


if __name__ == "__main__":
    main()
