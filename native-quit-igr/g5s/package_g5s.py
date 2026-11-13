#!/usr/bin/env python3
"""G5S single USB diagnostic (post-hardware G5A early return).

Copy every official PSX DVR ISO MANAGER USB v1.0 runtime byte-identically,
except the local G5S Manager path and on-screen G5S Neutrino SYSTEM ELF.
No HDD changes, no new partitions, no XMB installer.
"""
import argparse
import sys
from pathlib import Path
import zipfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"g2"))
from package_ab import load_stable, elf_check, sha, USB_RELEASE_SHA256

FOLDER="DESR_G5S"
DIFFER={"DVR_ISO_MANAGER.ELF","neutrino.elf"}


def build(args):
    baseline=load_stable(args.release)
    manager=args.manager.read_bytes()
    neutrino=args.neutrino.read_bytes()
    elf_check("G5S Manager",manager)
    elf_check("G5S Neutrino SYSTEM",neutrino)
    if f"mass0:/{FOLDER}/".encode() not in manager:
        raise ValueError("Test Manager has wrong USB root")
    if b"mass0:/DESR_DVR/" in manager or b"PP.TESTDVR" in manager:
        raise ValueError("Test Manager references live USB directory/test partition")
    if b"1.0 USB G5S" not in manager or b"dvr_pfs0:/ISO/" not in manager:
        raise ValueError("Wrong G5S Manager banner/DVR read path")
    if neutrino==baseline["neutrino.elf"]:
        raise ValueError("Diagnostic Neutrino cannot equal original")
    modified=dict(baseline)
    modified["DVR_ISO_MANAGER.ELF"]=manager
    modified["neutrino.elf"]=neutrino
    differences={k for k,v in modified.items() if v!=baseline[k]}
    if differences!=DIFFER:
        raise ValueError("Unexpected differences from stable USB 1.0: "+repr(differences))
    for k in ("BOOT.ELF","BOOT.CNF","neutrino_lang.elf","modules/ee_core.elf",
              "modules/ee_core_lang.elf","modules/dvrfile_fhi.irx",
              "config/system.toml","config/bsd-dvr.toml","config/osdlang.toml"):
        if modified[k]!=baseline[k]:
            raise ValueError("Production file changed: "+k)
    entries={f"{FOLDER}/{k}":v for k,v in modified.items()}
    entries["G5S_MANIFEST.txt"]=(
        "G5S USB ONLY - early return diagnostic; physical console UNTESTED\n"
        "Built after G5R reached STAGE 91 but not stage 199.\n"
        "Mode0, 1-byte NoticeGameStart (as G3B / G5A control).\n"
        "Entry STAGE 1, checkpoints 92-98 after SYSTEM.CNF, stop STAGE 105 before EE clear.\n"
        "Existing G5R stages and return trap preserved; G5S intentionally STOPS at stage105.\n"
        "This diagnostic NEVER launches the game, even when test succeeds.\n"
        "Do NOT test G5B until G5S control is understood.\n"
        f"Official USB v1.0 SHA256: {USB_RELEASE_SHA256}\n"
        f"Original Neutrino SHA256: {sha(baseline['neutrino.elf'])}\n"
        f"G5S Neutrino SHA256: {sha(neutrino)}\n"
        "Only TEST Manager and TEST Neutrino differ from official USB 1.0.\n"
        "Do not replace original DESR_DVR/, PP.DVRISO, or XMB application.\n"
    ).encode("utf-8")
    folders=[f"{FOLDER}/{x}" for x in ("","ART/","config/","modules/")]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=8) as z:
        for d in folders:z.writestr(d,b"")
        for name,v in sorted(entries.items()):z.writestr(name,v)
    with zipfile.ZipFile(args.output) as z:
        if z.testzip() is not None:raise ValueError("G5S ZIP CRC failed")
        if set(z.namelist())!=set(folders)|set(entries):
            raise ValueError("G5S ZIP contents invalid")
        for d in folders:
            if not z.getinfo(d).is_dir():raise ValueError("Missing folder "+d)
        for k,v in entries.items():
            if z.read(k)!=v:raise ValueError("ZIP corruption "+k)
    print("PASS G5S: USB base source files",len(baseline))
    print("PASS G5S: changed ONLY test Manager and stage-capturing SYSTEM Neutrino")
    print("PASS G5S: BOOT.ELF, modules, FHI, ee_core, LANG and configs identical to stable")
    print("PASS G5S: no changes to Sony HDD, PP.DVRISO, XMB or release")
    print("OUTPUT:",args.output,args.output.stat().st_size,sha(args.output.read_bytes()))

if __name__=="__main__":
    p=argparse.ArgumentParser()
    for n in ("release","manager","neutrino","output"):
        p.add_argument("--"+n,required=True,type=Path)
    build(p.parse_args())
