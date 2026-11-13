#!/usr/bin/env python3
"""G2C isolated USB control: same Neutrino toolchain as G2B, NO SCMD patch.

G2A was official published 1.0 and G2B was a rebuild plus SCMD changes.
This G2C control separates *recompilation* from *mode 1/one-byte SCMD*.
"""
import argparse
from pathlib import Path
import zipfile

from package_ab import load_stable, elf_check, sha, USB_RELEASE_SHA256

FOLDER = "DESR_G2C"

def build(args):
    baseline = load_stable(args.release)
    manager = args.manager.read_bytes()
    neutrino = args.system.read_bytes()
    elf_check("G2C Manager", manager)
    elf_check("G2C Neutrino SYSTEM", neutrino)
    assert f"mass0:/{FOLDER}/".encode() in manager
    assert b"mass0:/DESR_DVR/" not in manager
    assert b"1.0 USB G2C" in manager
    assert b"dvr_pfs0:/ISO/" in manager
    assert neutrino == baseline["neutrino.elf"], "Pinned rebuild differs from the official 1.0 baseline"
    before = {k: sha(v) for k,v in baseline.items()}
    baseline["DVR_ISO_MANAGER.ELF"] = manager
    baseline["neutrino.elf"] = neutrino
    changed = {k for k,v in baseline.items() if sha(v)!=before[k]}
    if changed != {"DVR_ISO_MANAGER.ELF"}:
        raise ValueError(f"Unexpected changes against official 1.0 USB: {changed}")
    assert baseline["neutrino_lang.elf"] and baseline["modules/ee_core.elf"]
    assert baseline["BOOT.ELF"] and baseline["config/bsd-dvr.toml"]
    data = {
        f"{FOLDER}/{name}": value for name,value in baseline.items()
    }
    data["G2C_MANIFEST.txt"] = (
        "G2C: BYTE-IDENTICAL BASELINE REBUILD CONFIRMED (NO QUIT PATCH)\n"
        "Same pinned Neutrino PR228 commit and ps2max/dev:v20260228 "
        "toolchain as G2B.\n"
        "Test SYSTEM only. Original ENGLISH files are present but NOT the control under test.\n"
        f"Official 1.0 USB zip SHA256: {USB_RELEASE_SHA256}\n"
        f"G2C rebuilt SYSTEM Neutrino SHA256: {sha(neutrino)}\n"
        f"Official SYSTEM Neutrino SHA256: {before['neutrino.elf']}\n"
        "Modified files against official USB: DVR_ISO_MANAGER.ELF (isolated folder). Neutrino SYSTEM is identical.\n"
        "No changes to BOOT.ELF, configs, IRX, EE core, HDD, XMB, existing release.\n"
        "Hardware testing NOT yet performed.\n"
    ).encode()
    folders=[f"{FOLDER}/",f"{FOLDER}/ART/",f"{FOLDER}/config/",f"{FOLDER}/modules/"]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=8) as z:
        for name in folders:
            z.writestr(name,b"")
        for k,v in sorted(data.items()):
            z.writestr(k,v)
    with zipfile.ZipFile(args.output) as z:
        assert z.testzip() is None
        assert len(z.namelist())==len(folders)+len(data)
        for n in folders:
            assert z.getinfo(n).is_dir()
        for n,b in data.items():
            assert z.read(n)==b
    print(f"PASS G2C: {len(baseline)} official USB files, only test Manager differs")
    print("PASS G2C: Neutrino recompiled without SCMD patch; binary-identical to 1.0")
    print("PASS G2C: no HDD/XMB/official USB changes")
    print(f"OUTPUT {args.output} bytes={args.output.stat().st_size} sha256={sha(args.output.read_bytes())}")


if __name__=="__main__":
    p=argparse.ArgumentParser()
    for name in ("release","manager","system","output"):
        p.add_argument("--"+name,required=True,type=Path)
    build(p.parse_args())
