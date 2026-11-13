#!/usr/bin/env python3
"""Package independent G3A/G3B USB test roots from exact official v1.0 USB ZIP.

Never alters published 1.0 HDD/USB/XMB or the original DESR_DVR folder.
G3A: only SCMD mode=1, len=4.
G3B: only SCMD mode=0, len=1.
ENGLISH Neutrino intentionally remains official 1.0; test SYSTEM only.
"""
import argparse
import hashlib
from pathlib import Path
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g2"))
from package_ab import load_stable, elf_check, sha, USB_RELEASE_SHA256

FOLDERS = {"G3A": "DESR_G3A", "G3B": "DESR_G3B"}
APP = "DVR_ISO_MANAGER.ELF"
SYSTEM = "neutrino.elf"

def build(a):
    base = load_stable(a.release)
    expected_original = {
        k: sha(base[k]) for k in
        ("BOOT.ELF", "BOOT.CNF", "neutrino_lang.elf", "modules/ee_core.elf",
         "modules/ee_core_lang.elf", "modules/dvrfile_fhi.irx",
         "config/osdlang.toml", "config/bsd-dvr.toml")
    }
    roots = {}
    for v, folder in FOLDERS.items():
        manager = getattr(a, "manager_" + v.lower()).read_bytes()
        loader = getattr(a, "neutrino_" + v.lower()).read_bytes()
        elf_check(v + " Manager", manager)
        elf_check(v + " Neutrino SYSTEM", loader)
        if loader == base[SYSTEM]:
            raise ValueError(v + ": expected an experimentally changed SYSTEM ELF")
        if f"mass0:/{folder}/".encode() not in manager:
            raise ValueError(v + ": Manager does not target own USB directory")
        if b"mass0:/DESR_DVR/" in manager or b"PP.TESTDVR" in manager:
            raise ValueError(v + ": Manager may affect old USB root/partition")
        if b"1.0 USB " + v.encode() not in manager:
            raise ValueError(v + ": not labelled experimental")
        if b"dvr_pfs0:/ISO/" not in manager:
            raise ValueError(v + ": changed DVR ISO store")
        root = dict(base)
        root[APP] = manager
        root[SYSTEM] = loader
        for k,h in expected_original.items():
            if sha(root[k]) != h:
                raise ValueError(v + ": altered official runtime file: " + k)
        roots[v] = root
    if roots["G3A"][SYSTEM] == roots["G3B"][SYSTEM]:
        raise ValueError("G3A and G3B must have distinct patched Neutrino binaries")
    diffs = {k for k in base if roots["G3A"][k] != roots["G3B"][k]}
    if diffs != {APP, SYSTEM}:
        raise ValueError("Unexpected A/B differences: " + repr(diffs))
    out_files = {}
    directories = []
    for v,folder in FOLDERS.items():
        directories.extend(folder+"/"+s for s in ("", "ART/", "config/", "modules/"))
        for path, content in roots[v].items():
            out_files[folder+"/"+path] = content
    out_files["G3_MANIFEST.txt"] = (
        "G3: Native Quit on DESR-7000 -- SCMD 0x29 mode/length matrix.\n"
        "G2B on console: black screen before game start; physical QUIT returned to XMB.\n"
        "G3A mode=1, bytes=4; G3B mode=0, bytes=1.\n"
        "Both SYSTEM versions use pinned Neutrino PR228 + EXACTLY one operand edit.\n"
        "Test SYSTEM mode ONLY; ENGLISH is original and is NOT part of this experiment.\n"
        f"Official source USB ZIP SHA256: {USB_RELEASE_SHA256}\n"
        f"Original SYSTEM ELF SHA256: {sha(base[SYSTEM])}\n"
        f"G3A SYSTEM ELF SHA256: {sha(roots['G3A'][SYSTEM])}\n"
        f"G3B SYSTEM ELF SHA256: {sha(roots['G3B'][SYSTEM])}\n"
        "Unchanged: original OSDMenu BOOT.ELF, all configs, IRX, EE cores, HDD, XMB.\n"
        "TEST UNVERIFIED ON PHYSICAL CONSOLE. Never replace working version 1.0.\n"
    ).encode()
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(a.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as z:
        for name in directories:
            z.writestr(name, b"")
        for k,v in sorted(out_files.items()):
            z.writestr(k, v)
    with zipfile.ZipFile(a.output) as z:
        if z.testzip() is not None:
            raise ValueError("Archive integrity test failed")
        if set(z.namelist()) != set(directories) | set(out_files):
            raise ValueError("Wrong ZIP contents")
        for name in directories:
            if not z.getinfo(name).is_dir():
                raise ValueError("Missing directory " + name)
        for name,data in out_files.items():
            if z.read(name) != data:
                raise ValueError("ZIP member changed " + name)
    print(f"PASS: copied {len(base)} official runtime files into each of two isolated USB roots")
    print("PASS: only A/B differences:", ", ".join(sorted(diffs)))
    print("PASS: original OSDMenu/IRX/EE core, official game files and HDD/XMB unchanged")
    print("PASS: sample both SCMD parameter axes independently")
    print("OUTPUT:", a.output, a.output.stat().st_size, sha(a.output.read_bytes()))

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    for item in ("release", "manager_g3a", "manager_g3b",
                 "neutrino_g3a", "neutrino_g3b", "output"):
        ap.add_argument("--"+item.replace("_","-"), type=Path, required=True)
    build(ap.parse_args())
