#!/usr/bin/env python3
"""Verify the exact v1.0.0 release candidates before public upload.

Usage:
    python3 release-tools/v1_0/verify_release_assets.py /path/to/release-assets
This is a verification helper, not a PSX installer and not a recompilation tool.
The standalone source bundle carries the reproducible release asset builder.
"""
from __future__ import annotations
import hashlib
import io
import pathlib
import struct
import sys
import zipfile

ASSETS = {
    "PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip":
        "4ad658817f25b6254b66f0f65f3bd34ca72af70e89d71756eed706d432b7e00f",
    "PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip":
        "18d4176d2d4e09277864e577735286fa71ae9e9d4561a93dd7f33d4c56493ba7",
    "PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip":
        "9eb31f374c9e71bbad4a94793bb7e20f363d43cb1ffe2183fdbd82271cc7e834",
}
HDD_MANAGER_SHA = "cf2f51a1548c53e5e39086d571c591d782aefc22d5db8af99e9b51b0d7c6f7c0"
KELF_SHA = "b335a5010adc94635caf735b33229ce377f146f9c256974cc7fd2ebf38684b49"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def one(z: zipfile.ZipFile, suffix: str) -> bytes:
    found = [p for p in z.namelist() if p.endswith(suffix)]
    assert len(found) == 1, (suffix, found)
    return z.read(found[0])


def verify(dir_name: pathlib.Path) -> None:
    for asset, digest in ASSETS.items():
        p = dir_name / asset
        assert p.is_file(), p
        assert sha(p.read_bytes()) == digest, (asset, sha(p.read_bytes()))
        with zipfile.ZipFile(p) as z:
            assert z.testzip() is None
            assert len(z.namelist()) == len(set(z.namelist()))
            assert all('..' not in pathlib.PurePosixPath(x).parts for x in z.namelist())
        print('PASS', asset, p.stat().st_size, digest)

    with zipfile.ZipFile(dir_name / 'PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip') as z:
        manager = one(z, '/DVR_ISO_MANAGER_HDD.ELF')
        assert manager.startswith(b'\x7fELF') and sha(manager) == HDD_MANAGER_SHA
        assert b'1.0 HDD' in manager and b'1.0 HDD NETFIX' not in manager
        assert b'NETFIX: USB DEV9 / pinned SDK / isolated RX / TCP checksum' in manager
        assert b'hdd0:PP.DVRISO' in manager and b'pfs1:/neutrino_hdd.elf' in manager
        assert b'pfs1:/neutrino_hdd_lang.elf' in manager
        assert sum(n.endswith('/neutrino_hdd.elf') for n in z.namelist()) == 1
        assert sum(n.endswith('/neutrino_hdd_lang.elf') for n in z.namelist()) == 1
        assert not any(n.endswith('/BOOT.CNF') or 'PREVIEW' in n or 'HDD_TEST.ELF' in n for n in z.namelist())

    with zipfile.ZipFile(dir_name / 'PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip') as z:
        m = one(z, '/DVR_ISO_MANAGER.ELF')
        assert m.startswith(b'\x7fELF')
        assert b'1.0 USB' in m and b'mass0:/DESR_DVR/' in m
        assert b'hdd0:PP.DVRISO' not in m
        assert any(n.endswith('/DESR_DVR/BOOT.ELF') for n in z.namelist())
        assert any('/DESR_DVR/config/' in n for n in z.namelist())
        assert any('/DESR_DVR/modules/' in n for n in z.namelist())

    with zipfile.ZipFile(dir_name / 'PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip') as z:
        kelf = one(z, '/EXECUTE.ELF')
        assert len(kelf) == 2026464 and sha(kelf) == KELF_SHA
        assert sha(one(z, '/DVR_ISO_MANAGER_HDD.ELF')) == HDD_MANAGER_SHA
        assert b'BOOT2 = pfs:/EXECUTE.ELF' in one(z, '/system.cnf')
        assert b'hdd0:PP.DVRISO:pfs:/DVR_ISO_MANAGER_HDD.ELF' in one(z, '/EXECUTE.CNF')
        assert b'PSX DVR ISO MANAGER' in one(z, '/info.sys')
        for name, (width, height) in [('jkt_002.png', (76, 108)), ('jkt_001.png', (256, 256))]:
            image = one(z, '/' + name)
            assert image.startswith(b'\x89PNG\r\n\x1a\n')
            w, h = struct.unpack_from('>II', image, 16)
            assert (w, h) == (width, height)
            assert (image[24], image[25]) == (8, 3), (name, image[24:26])
        assert not any(n.endswith('/BOOT.CNF') or n.endswith('/BOOT.ELF') for n in z.namelist())
    print('PASS all v1.0.0 release invariants; verified HDD and XMB binaries remain unchanged.')


if __name__ == '__main__':
    verify(pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else 'dist'))
