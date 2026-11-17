#!/usr/bin/env python3
"""Make isolated PP.DVRISO12 installer package from official HDD 1.0 assets.

The Manager and Neutrino are REBUILT for PP.DVRISO12 from HDD 1.0 sources.
Never ship unmodified PP.DVRISO-only ELF files as HDD12 executables.
No Sony system partition is modified on the build PC.
"""
from __future__ import annotations
import argparse
import hashlib
import io
from pathlib import Path
import re
import zipfile
import zlib

PART = "PP.DVRISO12"
SOURCE = "PDIM_HDD12_INSTALLER"
FILES = SOURCE + "/FILES/"
XMB = SOURCE + "/XMB/"
BLOCKED_NAMES = {
    "BOOT.CNF", "BOOT.NEXT.CNF", "BOOT.PREV.CNF", "BOOT_RECOVERY.CNF",
    "COVERS.IDX", "COVERS.NEW", "COVERS.BAK", "COVER_SCAN.LOG",
    "DVR_TEST.CNF", "V12_NET.CNF", "PDIM12.INSTALLED"
}
CRITICAL = {
    "DVR_ISO_MANAGER_HDD.ELF", "neutrino_hdd.elf",
    "neutrino_hdd_lang.elf", "modules/ee_core_lang.elf",
    "BOOT.ELF", "NET_PROBE.CNF", "config/osdlang.toml",
    "modules/dvrdrv.irx", "modules/dvrfile.irx",
    "modules/dvrfile_fhi.irx", "config/bsd-dvr.toml",
    "EXECUTE.ELF", "EXECUTE.CNF", "res/info.sys",
    "res/jkt_001.png", "res/jkt_002.png",
    "N4E.CA"
}
XMB_ASSETS = ("system.cnf","icon.sys","info.sys","jkt_001.png","jkt_002.png")

def sha(data:bytes)->str:
    return hashlib.sha256(data).hexdigest()
def check_elf(data:bytes,label:str,should_include:bool=True)->None:
    if len(data)<20000 or data[:4]!=b"\x7fELF" or data[4:6]!=b"\x01\x01" or int.from_bytes(data[18:20],"little")!=8:
        raise ValueError(f"{label}: not an ELF32 little-endian MIPS application")
    if should_include:
        if b"hdd0:PP.DVRISO12" not in data:
            raise ValueError(f"{label}: no PP.DVRISO12 path found")
        if re.search(rb"hdd0:PP\.DVRISO(?!12)",data):
            raise ValueError(f"{label}: unsafe PP.DVRISO 1.0 reference remains")
        if b"PP.TESTDVR" in data:
            raise ValueError(f"{label}: stale test partition")
def zip_contents(path:Path)->dict[str,bytes]:
    with zipfile.ZipFile(path) as z:
        if z.testzip() is not None:
            raise ValueError(f"{path}: ZIP CRC failure")
        return {info.filename:z.read(info) for info in z.infolist() if not info.is_dir()}
def match_suffix(objects:dict[str,bytes],tail:str)->bytes:
    keys=[k for k in objects if k==tail or k.endswith("/"+tail)]
    if len(keys)!=1:
        raise ValueError(f"Expected one {tail!r}, found {keys}")
    return objects[keys[0]]
def get_hdd_files(items:dict[str,bytes])->dict[str,bytes]:
    marker="PP.DVRISO-FILES/"
    keys=[k for k in items if marker in k]
    if not keys:
        raise ValueError("HDD 1.0 ZIP lacks PP.DVRISO-FILES directory")
    out={}
    for key in keys:
        relative=key.split(marker,1)[1]
        if not relative or relative.endswith("/"):
            continue
        if relative in BLOCKED_NAMES or relative.startswith("ART/"):
            continue
        if relative.startswith("/") or ".." in relative or "\\" in relative:
            raise ValueError(f"Invalid HDD source path {relative}")
        out[relative]=items[key]
    return out
def assert_sane_payload(path:str,data:bytes)->None:
    if not data or len(data)>16*1024*1024 or len(path)>111 or not re.fullmatch(r"[A-Za-z0-9_.\-/]+",path):
        raise ValueError(f"Bad HDD12 payload entry: {path}")
    if path.startswith(".") or path.startswith("/") or path.endswith("/") or ".." in path or "//" in path or path in BLOCKED_NAMES or path.startswith("ART/"):
        raise ValueError(f"Unsafe HDD12 payload path: {path}")
def build(args:argparse.Namespace)->None:
    hdd=zip_contents(args.hdd)
    xmb=zip_contents(args.xmb)
    out=get_hdd_files(hdd)
    manager=args.manager.read_bytes()
    system=args.system.read_bytes()
    english=args.english.read_bytes()
    core=args.core.read_bytes()
    check_elf(manager,"HDD12 Manager")
    check_elf(system,"HDD12 SYSTEM")
    check_elf(english,"HDD12 ENGLISH")
    check_elf(core,"English EE Core",False)
    if b"FIX95 ISO argument/path mismatch" not in system or b"FIX95 ISO argument/path mismatch" not in english:
        raise ValueError("Not a proven FIX95 Neutrino path")
    if b"1.0 HDD TEST12" not in manager:
        raise ValueError("Manager test version not identified correctly")
    out.update({
        "DVR_ISO_MANAGER_HDD.ELF":manager,
        "neutrino_hdd.elf":system,
        "neutrino_hdd_lang.elf":english,
        "modules/ee_core_lang.elf":core,
        "N4E.CA":args.ca.read_bytes(),
    })
    if not out.get("config/osdlang.toml"):
        raise ValueError("Missing ENGLISH osdlang.toml")
    if b"ee_core_lang.elf" not in out["config/osdlang.toml"]:
        raise ValueError("Wrong ENGLISH core config")

    # Reuse ALREADY TESTED signed KELF and graphical assets from PDIM HDD 1.0.
    launcher=match_suffix(xmb,"PFS_COPY_TO_PP.DVRISO/EXECUTE.ELF")
    if len(launcher)<100_000:
        raise ValueError("Unexpected short trusted XMB KELF")
    launch_cnf=match_suffix(xmb,"PFS_COPY_TO_PP.DVRISO/EXECUTE.CNF")
    original=b"path=hdd0:PP.DVRISO:pfs:/DVR_ISO_MANAGER_HDD.ELF"
    if launch_cnf.count(original)!=1:
        raise ValueError("Unexpected XMB launcher configuration")
    launch_cnf=launch_cnf.replace(original,
        b"path=hdd0:PP.DVRISO12:pfs:/DVR_ISO_MANAGER_HDD.ELF")
    out["EXECUTE.ELF"]=launcher
    out["EXECUTE.CNF"]=launch_cnf

    header={n:match_suffix(xmb,f"__Headers/PP.DVRISO/{n}") for n in XMB_ASSETS}
    if b"BOOT2 = pfs:/EXECUTE.ELF" not in header["system.cnf"]:
        raise ValueError("Unexpected pretested XMB boot header")
    if b"PS2X" not in header["icon.sys"][:16]:
        raise ValueError("Unexpected XMB icon.sys data")
    # Distinguish the test XMB entry while preserving original PNG8 artworks.
    icon_sys=header["icon.sys"]
    if icon_sys.count(b"title1=HDD 1.0")!=1:
        raise ValueError("HDD 1.0 icon.sys title changed")
    header["icon.sys"]=icon_sys.replace(b"title1=HDD 1.0",b"title1=HDD12 TEST")
    info=header["info.sys"]
    if info.count(b"title = PSX DVR ISO MANAGER")!=1 or info.count(b"title_id = PSX-DVRISO")!=1:
        raise ValueError("XMB info.sys template changed")
    header["info.sys"]=info.replace(b"title = PSX DVR ISO MANAGER",b"title = PDIM HDD12 TEST").replace(
        b"title_id = PSX-DVRISO",b"title_id = PDIM-HDD12")
    out["res/info.sys"]=header["info.sys"]
    out["res/jkt_001.png"]=header["jkt_001.png"]
    out["res/jkt_002.png"]=header["jkt_002.png"]
    for path,payload in out.items():
        assert_sane_payload(path,payload)
    if set(out) < CRITICAL:
        raise ValueError("Incomplete HDD12 test files: "+str(sorted(CRITICAL-set(out))))
    if len(out)>160:
        raise ValueError("Too many files for console installer manifest")
    if any(re.search(rb"hdd0:PP\.DVRISO(?!12)",out[k])
        for k in ("DVR_ISO_MANAGER_HDD.ELF","neutrino_hdd.elf","neutrino_hdd_lang.elf")):
        raise ValueError("Source code failed to isolate PP.DVRISO12")

    manifest="".join(
        f"{p}|{len(out[p])}|{zlib.crc32(out[p])&0xffffffff:08x}\n"
        for p in sorted(out))
    if len(manifest)>30000:
        raise ValueError("Manifest too large")
    archive={
       SOURCE+"/PDIM_HDD12_INSTALLER.ELF":args.installer.read_bytes(),
       SOURCE+"/MANIFEST.TXT":manifest.encode(),
       SOURCE+"/V12_NET.CNF.example":args.config.read_bytes(),
       SOURCE+"/README_PL.txt":args.readme.read_bytes(),
       SOURCE+"/SOURCE_COMMIT":(args.commit+"\n").encode(),
    }
    for p,data in out.items():
        archive[FILES+p]=data
    for p,data in header.items():
        archive[XMB+p]=data
    archive[SOURCE+"/ASSETS_CREDITS.txt"]=(
        "PDIM HDD 1.0 XMB icon PNG8, icon.sys, info.sys, system.cnf and EXECUTE.ELF\n"
        "copied from the tested official v1.0 XMB installer; no new KELF signing.\n"
        "Modules credited to DanielFergisz/RepairBox and PS2SDK; source provenance\n"
        "must be checked before public distribution. THIS IS A PRIVATE TEST ARTIFACT.\n").encode()
    archive[SOURCE+"/SHA256SUMS"]=("".join(
        f"{sha(data)}  {name[len(SOURCE)+1:]}\n"
        for name,data in sorted(archive.items()) if name.startswith(SOURCE+"/")
    )).encode()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=7) as z:
        for path,data in sorted(archive.items()):
            z.writestr(path,data)
        z.writestr(FILES+"ART/",b"")
    with zipfile.ZipFile(args.output) as z:
        if z.testzip() is not None:
            raise ValueError("Generated ZIP CRC failure")
        if set(z.namelist())!=set(archive)|{FILES+"ART/"}:
            raise ValueError("Generated ZIP entries mismatched")
        for path,data in archive.items():
            if z.read(path)!=data:
                raise ValueError("Generated ZIP readback mismatch: "+path)
        if b"UWAGA" not in z.read(SOURCE+"/README_PL.txt"):
            raise ValueError("Missing mandatory destructive warning in README")
    print("PASS: HDD 1.0 contents rebuilt for PP.DVRISO12; primary XMB assets and KELF copied")
    print("PASS: no unmodified HDD1.0 Manager/Neutrino, no active V12_NET.CNF, no mutable ART/BOOT.CNF")
    print("FILES:",len(out),"ZIP SHA256:",sha(args.output.read_bytes()))
def main()->None:
    p=argparse.ArgumentParser()
    for name in ("hdd","xmb","manager","system","english","core","installer","ca","config","readme"):
        p.add_argument("--"+name,type=Path,required=True)
    p.add_argument("--commit",required=True)
    p.add_argument("--output",type=Path,required=True)
    build(p.parse_args())
if __name__=="__main__":
    main()
