#!/usr/bin/env python3
"""G4 dual Sony PSX drive-mode experiment, with official v1.0 modules.

G4A = ChangeSys(2) + NoticeGameStart(0), one byte: control for drive change.
G4B = ChangeSys(2) + NoticeGameStart(1), one byte: full Sony-like sequence.
Run G4B ONLY if G4A boots same known working game.
Only USB folder tests; never overwrite original DESR_DVR or HDD.
"""
import argparse
from pathlib import Path
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g2"))
from package_ab import load_stable, elf_check, sha, USB_RELEASE_SHA256

FOLDERS = ("DESR_G4A", "DESR_G4B")


def build(a):
    original = load_stable(a.release)
    files = {}
    folders = []
    for folder, manager_path, system_path in (
        (FOLDERS[0], a.manager_g4a, a.neutrino_g4a),
        (FOLDERS[1], a.manager_g4b, a.neutrino_g4b)
    ):
        manager, system = manager_path.read_bytes(), system_path.read_bytes()
        elf_check(folder+" Manager", manager)
        elf_check(folder+" SYSTEM", system)
        if manager == original["DVR_ISO_MANAGER.ELF"] or system == original["neutrino.elf"]:
            raise ValueError("Expected new branch-only test ELF in "+folder)
        if f"mass0:/{folder}/".encode() not in manager:
            raise ValueError("Manager targets wrong folder: "+folder)
        if b"mass0:/DESR_DVR/" in manager or b"PP.TESTDVR" in manager:
            raise ValueError("Manager contains original-folder/test-partition reference")
        if b"1.0 USB " + folder[-3:].encode() not in manager:
            raise ValueError("Manager banner incorrect")
        if b"dvr_pfs0:/ISO/" not in manager:
            raise ValueError("Missing stable DVR ISO path")
        # The strings below are emitted in G4 experiment source and used
        # as an integrity guard against accidentally mixing G3 or v1.0.
        for marker in (b"G4: cdinit=%d", b"G4: switch=2 result=%d",
                       b"G4: notice_mode="+ (b"0" if folder==FOLDERS[0] else b"1")):
            if marker not in system:
                raise ValueError("Wrong experimental Neutrino in "+folder+" marker "+str(marker))
        runtime = dict(original)
        runtime["DVR_ISO_MANAGER.ELF"] = manager
        runtime["neutrino.elf"] = system
        for name in original:
            if name not in ("DVR_ISO_MANAGER.ELF", "neutrino.elf") and runtime[name] != original[name]:
                raise ValueError("Corrupted official runtime file: "+name)
        folders.extend(folder+"/"+r for r in ("", "ART/", "config/", "modules/"))
        files.update({folder+"/"+k: v for k,v in runtime.items()})

    if files[FOLDERS[0]+"/neutrino.elf"] == files[FOLDERS[1]+"/neutrino.elf"]:
        raise ValueError("Two G4 Neutrino binaries must not be the same")
    files["G4_MANIFEST.txt"] = (
        "G4 SONY DESR NATIVE QUIT / USB / NOT HARDWARE VERIFIED\n"
        "G3A (mode1, 4 bytes): game black, QUIT returns XMB after delay.\n"
        "G3B (mode0, 1 byte): game starts, QUIT does not respond.\n"
        "G4A: ChangeSys(2) RPC before NoticeGameStart(0), one-byte control.\n"
        "G4B: ChangeSys(2) RPC before NoticeGameStart(1), Sony-like hypothesis.\n"
        "FIRST test G4A; if game fails, STOP and do not test G4B.\n"
        f"Official USB 1.0 input archive SHA256: {USB_RELEASE_SHA256}\n"
        f"Official Neutrino 1.0 SYSTEM SHA256: {sha(original['neutrino.elf'])}\n"
        f"G4A Neutrino SYSTEM SHA256: {sha(files['DESR_G4A/neutrino.elf'])}\n"
        f"G4B Neutrino SYSTEM SHA256: {sha(files['DESR_G4B/neutrino.elf'])}\n"
        "All other runtime modules, OSDMenu, EE cores, TOML are byte-identical to USB 1.0.\n"
        "No HDD/XMB changes. The two test USB managers have isolated BOOT.CNF.\n"
    ).encode()
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(a.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as z:
        for folder in folders:
            z.writestr(folder, b"")
        for name,data in sorted(files.items()):
            z.writestr(name,data)
    with zipfile.ZipFile(a.output) as z:
        if z.testzip():
            raise ValueError("G4 archive has CRC errors")
        if set(z.namelist()) != set(folders) | set(files):
            raise ValueError("Missing/extra archive entries")
        for folder in folders:
            if not z.getinfo(folder).is_dir():
                raise ValueError("Directory entry not marked directory: "+folder)
        for name,contents in files.items():
            if z.read(name) != contents:
                raise ValueError("ZIP mutated file: "+name)
    print("PASS: two isolated USB folders with",len(original),"runtime files each")
    print("PASS: official OSDMenu/modules/configs/EE core binaries copied unchanged")
    print("PASS: only G4 test Managers and SYSTEM Neutrino differ from USB v1.0")
    print("PASS: no HDD, partition, XMB, or release mutations")
    print("OUTPUT:",a.output,a.output.stat().st_size,sha(a.output.read_bytes()))


if __name__ == "__main__":
    ap=argparse.ArgumentParser()
    for name in ("release","manager_g4a","manager_g4b","neutrino_g4a","neutrino_g4b","output"):
        ap.add_argument("--"+name.replace("_","-"),required=True,type=Path)
    build(ap.parse_args())
