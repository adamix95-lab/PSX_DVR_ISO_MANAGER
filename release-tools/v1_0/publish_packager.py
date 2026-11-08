#!/usr/bin/env python3
"""Build PDIM 1.0 release assets from existing tested GitHub Actions artifacts.

Host-side release packaging only: no PSX disk writes, no recompilation of
Manager, OSDMenu or Neutrino. The original HDD package is preserved verbatim.
"""
from __future__ import annotations
import argparse, hashlib, io, json, re, struct
from pathlib import Path
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

HDD_SHA = "cf2f51a1548c53e5e39086d571c591d782aefc22d5db8af99e9b51b0d7c6f7c0"
USB_SHA = "d5e4ce1706191a46775d19c46f8918ddd38c7b4f6bc7a9ff4fa22854fbb032b7"
LAUNCHER_SHA = "64e45abcd92b1096dc490d38fc0a20ac9959004a4286c4cb661f00019423b3ae"
KELF_SHA = "b335a5010adc94635caf735b33229ce377f146f9c256974cc7fd2ebf38684b49"
XMB_ICON_SHA = "e26fd4323a217f6f0641873a3ff6ea50c80080b01dbe418619b8900d9b924713"
HDD_PACKAGE_SHA = "4ad658817f25b6254b66f0f65f3bd34ca72af70e89d71756eed706d432b7e00f"
KELF_SIZE, HEADER_SIZE, FOOTER_SIZE = 2026464, 928, 224
ROOT = Path(__file__).resolve().parent

def sha(b): return hashlib.sha256(b).hexdigest()

def member(zip_bytes, ending):
    with ZipFile(io.BytesIO(zip_bytes)) as z:
        assert z.testzip() is None, "Bad ZIP"
        found = [n for n in z.namelist() if n.endswith(ending)]
        assert len(found) == 1, (ending, found)
        return z.read(found[0])

def archive(path, entries):
    with ZipFile(path, "w", compression=ZIP_DEFLATED, compresslevel=8) as z:
        for name, data in sorted(entries.items()):
            assert not name.startswith("/") and ".." not in Path(name).parts
            obj = ZipInfo(name, date_time=(2026,11,8,0,0,0))
            obj.compress_type = ZIP_DEFLATED
            obj.external_attr = (0o755 if name.endswith("/") else 0o644) << 16
            z.writestr(obj, data)
    with ZipFile(path) as z:
        assert z.testzip() is None
        for name, data in entries.items(): assert z.read(name) == data

def rebuild_kelf(launcher, kelf_header):
    source = kelf_header.read_text()
    def parse(name, length):
        found = re.search(r"\b" + name + r"\s*\[.*?\]\s*=\s*\{(.*?)\}\s*;", source, re.S)
        assert found, name
        result = bytes(int(x,16) for x in re.findall(r"\b0x([0-9a-fA-F]{2})\b", found.group(1)))
        assert len(result) == length
        return result
    a, b = parse("hdloader_kelf_header", HEADER_SIZE), parse("hdloader_kelf_footer", FOOTER_SIZE)
    k = a + launcher + b"\0" * (KELF_SIZE - len(a) - len(launcher) - len(b)) + b
    assert len(k) == KELF_SIZE and sha(k) == KELF_SHA, ("Unexpected KELF", sha(k))
    return k

def main():
    a = argparse.ArgumentParser()
    for p in ("hdd", "usb", "usb_v08", "kelf_header", "license", "osd_license", "out"):
        a.add_argument("--" + p.replace("_","-"),type=Path,required=True)
    v = a.parse_args()
    v.out.mkdir(parents=True,exist_ok=True)
    hdd_bytes = member(v.hdd.read_bytes(), "PSX-DVR-ISO-Manager-1.0-HDD.zip")
    assert sha(hdd_bytes) == HDD_PACKAGE_SHA, "HDD ZIP differs from hardware-tested original"
    hdd_path = v.out / "PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip"
    hdd_path.write_bytes(hdd_bytes)
    with ZipFile(io.BytesIO(hdd_bytes)) as z:
        h = {n:z.read(n) for n in z.namelist()}
    hdd_manager = [value for name,value in h.items() if name.endswith("/DVR_ISO_MANAGER_HDD.ELF")]
    launcher = [value for name,value in h.items() if name.endswith("/BOOT.ELF")]
    assert len(hdd_manager) == len(launcher) == 1
    hdd_manager, launcher = hdd_manager[0], launcher[0]
    assert sha(hdd_manager) == HDD_SHA and sha(launcher) == LAUNCHER_SHA
    assert b"1.0 HDD" in hdd_manager and b"1.0 HDD NETFIX" not in hdd_manager
    assert b"NETFIX: USB DEV9 / pinned SDK / isolated RX / TCP checksum" in hdd_manager

    base_zip = member(v.usb_v08.read_bytes(), "PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE.zip")
    new_usb = member(v.usb.read_bytes(), "/DVR_ISO_MANAGER.ELF")
    assert sha(new_usb) == USB_SHA and b"1.0 USB" in new_usb
    with ZipFile(io.BytesIO(base_zip)) as z:
        assert z.testzip() is None
        old = {p:z.read(p) for p in z.namelist()}
    old_root = next(iter({p.split("/")[0] for p in old if "/" in p})) + "/"
    usb_root = "PSX-DVR-ISO-Manager-v1.0-USB-COMPLETE/"
    usb_files={}
    for name,data in old.items():
        n=usb_root + name[len(old_root):]
        if n.endswith("/DVR_ISO_MANAGER.ELF"): data=new_usb
        elif n.endswith("/START_HERE_PL.txt"):
            data=("PSX DVR ISO MANAGER 1.0 USB — test na DESR-7000: PASS\n"
                  "Skopiuj wewnetrzny folder DESR_DVR na pendrive FAT32.\n"
                  "Uruchom mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF przez wLE.\n"
                  "Gry ISO: dvr_pfs0:/ISO/; okladki: mass0:/DESR_DVR/ART/.\n"
                  "Wymaga komputera z ART Server i poprawnego NET_PROBE.CNF.\n"
                  "Zrob backup BOOT.CNF, NET_PROBE.CNF, ART i indeksow przed aktualizacja.\n").encode()
        elif n.endswith("/START_HERE_EN.txt"):
            data=("PSX DVR ISO MANAGER 1.0 USB — DESR-7000 hardware: PASS\n"
                  "Copy the inner DESR_DVR folder to the root of FAT32 USB.\n"
                  "Launch mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF through wLE.\n"
                  "Game ISOs: dvr_pfs0:/ISO/; covers: mass0:/DESR_DVR/ART/.\n"
                  "Requires the Windows ART Server and configured NET_PROBE.CNF.\n"
                  "Back up BOOT.CNF, NET_PROBE.CNF, ART and indexes before upgrading.\n").encode()
        usb_files[n]=data
    assert sum(n.endswith("/DVR_ISO_MANAGER.ELF") for n in usb_files)==1
    usb_path=v.out/"PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip"
    archive(usb_path, usb_files)

    kelf=rebuild_kelf(launcher, v.kelf_header)
    png=(ROOT / "jkt_002.png").read_bytes()
    assert sha(png)==XMB_ICON_SHA
    assert png[:8]==b"\x89PNG\r\n\x1a\n" and struct.unpack_from(">II",png,16)==(76,108)
    assert png[24:26]==b"\x08\x03"
    from PIL import Image
    im=Image.open(io.BytesIO(png)).convert("RGB").resize((256,256),Image.Resampling.LANCZOS)
    im=im.quantize(colors=256)
    out=io.BytesIO()
    im.save(out,format="PNG",optimize=True)
    png256=out.getvalue()
    system=b"BOOT2 = pfs:/EXECUTE.ELF\nVER = 1.00\nVMODE = NTSC\nHDDUNITPOWER = NICHDD\n"
    icon_sys=("PS2X\ntitle0=PSX DVR ISO MANAGER\ntitle1=HDD 1.0\n"
              "bgcola=0\nbgcol0=0,0,0\nbgcol1=0,0,0\nbgcol2=0,0,0\nbgcol3=0,0,0\n"
              "lightdir0=1.0,-1.0,1.0\nlightdir1=-1.0,1.0,-1.0\nlightdir2=0.0,0.0,0.0\n"
              "lightcolamb=64,64,64\nlightcol0=64,64,64\nlightcol1=16,16,16\nlightcol2=0,0,0\n"
              "uninstallmes0=Remove PSX DVR ISO MANAGER\nuninstallmes1=\nuninstallmes2=\n").encode()
    info=("title = PSX DVR ISO MANAGER\ntitle_id = PSX-DVRISO\ntitle_sub_id = 0\n"
          "release_date = 20261106\ndeveloper_id = Homebrew\n"
          "publisher_id = PSX DVR ISO MANAGER\nnote = Homebrew DVR ISO browser for Sony PSX DESR\n"
          "content_web = \nimage_topviewflag = 0\nimage_type = 0\n"
          "image_count = 1\nimage_viewsec = 600\ncopyright_viewflag = 0\n"
          "copyright_imgcount = 0\ngenre = Utility\nparental_lock = 1\n"
          "effective_date = 0\nexpire_date = 0\narea = J\nviolence_flag = 0\n"
          "content_type = 255\ncontent_subtype = 0\n").encode()
    quick=b"path=hdd0:PP.DVRISO:pfs:/DVR_ISO_MANAGER_HDD.ELF\narg=-dev9=NICHDD\n"
    pl=("PSX DVR ISO MANAGER 1.0 — INSTALACJA XMB (wLE-R3Z 4.80)\n"
        "Wykonaj kopie naglowka APA i plikow partycji. Zmieniaj TYLKO PP.DVRISO.\n"
        "1. Utworz partycje hdd0:PP.DVRISO PFS (np. 256 MB), jesli jej nie ma.\n"
        "2. Skopiuj __Headers/PP.DVRISO/ do korzenia USB FAT32.\n"
        "3. HDDManager: zaznacz PP.DVRISO, R1 -> Inject Header -> This partition only.\n"
        "4. Skopiuj do PP.DVRISO pliki EXECUTE.ELF i EXECUTE.CNF z folderu\n"
        "   PFS_COPY_TO_PP.DVRISO oraz kompletna paczke HDD osobno.\n"
        "5. Wlacz XMB, wybierz PSX DVR ISO MANAGER.\n"
        "NIE uzywaj Format, Expand, ani Matching/Create Missing Partitions.\n"
        "Jesli XMB juz dziala, aktualizuj tylko DVR_ISO_MANAGER_HDD.ELF.\n").encode()
    en=("PSX DVR ISO MANAGER 1.0 — XMB INSTALL (wLE-R3Z 4.80)\n"
        "Back up the APA partition header and important files. Target ONLY PP.DVRISO.\n"
        "1. If missing, create PFS partition hdd0:PP.DVRISO (e.g. 256 MB).\n"
        "2. Put __Headers/PP.DVRISO/ at the root of FAT32 USB.\n"
        "3. HDDManager: select PP.DVRISO -> R1 -> Inject Header -> This partition only.\n"
        "4. Copy EXECUTE.ELF and EXECUTE.CNF from PFS_COPY_TO_PP.DVRISO/ to\n"
        "   PP.DVRISO, along with the separate complete HDD package.\n"
        "5. Restart to XMB and select PSX DVR ISO MANAGER.\n"
        "NEVER use Format, Expand, or Matching/Create Missing Partitions.\n"
        "For existing XMB installations update only DVR_ISO_MANAGER_HDD.ELF.\n").encode()
    xmb={
        "__Headers/PP.DVRISO/system.cnf":system,
        "__Headers/PP.DVRISO/icon.sys":icon_sys,
        "__Headers/PP.DVRISO/info.sys":info,
        "__Headers/PP.DVRISO/jkt_002.png":png,
        "__Headers/PP.DVRISO/jkt_001.png":png256,
        "PFS_COPY_TO_PP.DVRISO/EXECUTE.ELF":kelf,
        "PFS_COPY_TO_PP.DVRISO/EXECUTE.CNF":quick,
        "OPTIONAL_MANAGER_IF_MISSING/DVR_ISO_MANAGER_HDD.ELF":hdd_manager,
        "INSTALL_XMB_PL.txt":pl,
        "INSTALL_XMB_EN.txt":en,
        "LICENSES/hdl_dump_GPL-2.0.txt":v.license.read_bytes(),
        "LICENSES/OSDMenu_AFL-3.0.txt":v.osd_license.read_bytes(),
        "REBUILD_SOURCES/kelf_reference.h":v.kelf_header.read_bytes(),
        "REBUILD_SOURCES/README.txt":b"Source: hdl-dump GPLv2 KELF template and original OSDMenu Launcher (AFL 3.0).\n"
    }
    xmb["MANIFEST.json"] = json.dumps({"tag":"1.0", "xmb_title":"PSX DVR ISO MANAGER",
               "hdd_manager_sha256":HDD_SHA, "kelf_sha256":KELF_SHA,
               "jkt_002_sha256":XMB_ICON_SHA, "files":{n:sha(b) for n,b in xmb.items()}},indent=2).encode()
    assert not any(n.endswith("/BOOT.ELF") or n.endswith("/BOOT.CNF") for n in xmb)
    xmb_path=v.out/"PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip"
    archive(xmb_path,xmb)
    assets=[hdd_path,usb_path,xmb_path]
    sums="".join(f"{sha(f.read_bytes())}  {f.name}\n" for f in assets)
    (v.out/"SHA256SUMS.txt").write_text(sums)
    for f in assets:
        with ZipFile(f) as archive_obj: assert archive_obj.testzip() is None
        print("PASS:",f.name,f.stat().st_size,sha(f.read_bytes()))
    print("PASS: HDD unchanged; USB 1.0 preserved; XMB KELF and 76x108 PDIM icon match tested source")

if __name__ == "__main__":
    main()
