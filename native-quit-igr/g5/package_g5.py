#!/usr/bin/env python3
"""Make G5A and G5B USB-only diagnostics from SHA-256 pinned official USB 1.0.

Differences between A/B: test Manager paths and stage-instrumented Neutrino.
All modules, configs, OSDMenu, ee_core and English Neutrino stay byte-identical
to the published hardware-tested 1.0 USB archive.
"""
import argparse
from pathlib import Path
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g2"))
from package_ab import load_stable, elf_check, sha, USB_RELEASE_SHA256

FOLDERS = ("DESR_G5A", "DESR_G5B")
DIFFS = {"DVR_ISO_MANAGER.ELF", "neutrino.elf"}


def build(args):
    baseline = load_stable(args.release)
    tests = {}
    result = {}
    dirs = []
    for folder, mp, np in (
        ("DESR_G5A", args.manager_g5a, args.neutrino_g5a),
        ("DESR_G5B", args.manager_g5b, args.neutrino_g5b),
    ):
        manager = mp.read_bytes()
        neutrino = np.read_bytes()
        elf_check(folder+" Manager", manager)
        elf_check(folder+" Neutrino", neutrino)
        if neutrino == baseline["neutrino.elf"]:
            raise ValueError(folder+": diagnostic Neutrino unexpectedly identical to official")
        if f"mass0:/{folder}/".encode() not in manager or b"mass0:/DESR_DVR/" in manager:
            raise ValueError(folder+": Manager not isolated in own USB directory")
        if b"PP.TESTDVR" in manager or b"1.0 USB G5" not in manager:
            raise ValueError(folder+": Manager wrong banner or test partition reference")
        if b"dvr_pfs0:/ISO/" not in manager:
            raise ValueError(folder+": DVR ISO source unexpectedly altered")
        runtime=dict(baseline)
        runtime.update({"DVR_ISO_MANAGER.ELF":manager,"neutrino.elf":neutrino})
        if {k for k in runtime if runtime[k]!=baseline[k]} != DIFFS:
            raise ValueError(folder+": changed additional runtime file")
        if baseline["neutrino_lang.elf"] != runtime["neutrino_lang.elf"]:
            raise ValueError("English Neutrino must remain original")
        for mandatory in ("BOOT.ELF","BOOT.CNF","modules/ee_core.elf",
                          "modules/ee_core_lang.elf","modules/dvrfile_fhi.irx",
                          "config/system.toml","config/bsd-dvr.toml","config/osdlang.toml"):
            if runtime[mandatory]!=baseline[mandatory]:
                raise ValueError(folder+": official base mismatch "+mandatory)
        tests[folder] = runtime
        dirs.extend(folder+"/"+x for x in ("","ART/","config/","modules/"))
        result.update({folder+"/"+k:v for k,v in runtime.items()})
    if tests["DESR_G5A"]["neutrino.elf"] == tests["DESR_G5B"]["neutrino.elf"]:
        raise ValueError("A and B binaries must differ")
    differs={k for k in baseline if tests["DESR_G5A"][k]!=tests["DESR_G5B"][k]}
    if differs!=DIFFS:
        raise ValueError("G5A/B difference changed unexpectedly: "+str(differs))
    result["G5_MANIFEST.txt"] = (
        "G5 DVR ISO STAGE DIAGNOSTICS (USB ONLY)\n"
        "G5A: SCMD 0x29 mode 0, one byte (control as G3B)\n"
        "G5B: SCMD 0x29 mode 1, one byte (G2B failure setting)\n"
        "Both probes HALT intentionally before EE game handoff.\n"
        "A successful ISO checkpoint will display STAGE 199; game NEVER starts in G5.\n"
        "Lower checkpoint or freeze reveals earliest incomplete stage.\n"
        "NO HDD writes and no changes to original DESR_DVR/ or release v1.0.\n"
        f"Official USB 1.0 SHA256: {USB_RELEASE_SHA256}\n"
        f"Official Neutrino SHA256: {sha(baseline['neutrino.elf'])}\n"
        f"G5A Neutrino SHA256: {sha(tests['DESR_G5A']['neutrino.elf'])}\n"
        f"G5B Neutrino SHA256: {sha(tests['DESR_G5B']['neutrino.elf'])}\n"
        "Original config, OSDMenu, English, FHI, EE cores are preserved.\n"
        "Physical hardware UNTESTED. Do not install to HDD or XMB.\n"
    ).encode("utf-8")
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output, "w",compression=zipfile.ZIP_DEFLATED,compresslevel=8) as z:
        for d in dirs:
            z.writestr(d,b"")
        for k,v in sorted(result.items()):
            z.writestr(k,v)
    with zipfile.ZipFile(args.output) as z:
        if z.testzip() is not None:
            raise ValueError("ZIP CRC failure")
        if set(z.namelist())!=set(dirs)|set(result):
            raise ValueError("Unexpected/missing G5 ZIP files")
        for d in dirs:
            if not z.getinfo(d).is_dir():
                raise ValueError("Missing ZIP directory: "+d)
        for name,v in result.items():
            if z.read(name)!=v:
                raise ValueError("ZIP byte mismatch: "+name)
    print(f"PASS G5: {len(baseline)} hardware-tested USB base files per test folder")
    print("PASS G5: A/B only differ in local Manager path and SYSTEM Neutrino diagnostics")
    print("PASS G5: all official modules, OSDMenu, English and configs preserved")
    print("PASS G5: no partition, HDD, or XMB updates")
    print("OUTPUT:",args.output,args.output.stat().st_size,"SHA256:",sha(args.output.read_bytes()))


if __name__ == "__main__":
    ap=argparse.ArgumentParser()
    for name in ("release","manager_g5a","manager_g5b","neutrino_g5a","neutrino_g5b","output"):
        ap.add_argument("--"+name.replace("_","-"),type=Path,required=True)
    build(ap.parse_args())
