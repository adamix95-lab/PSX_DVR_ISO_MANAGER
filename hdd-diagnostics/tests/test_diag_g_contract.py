"""DIAG G: source guard for read-only mounts, file checks and START exit."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CODE = (ROOT / "src" / "apa_dvr_pad_diag.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagGContract(unittest.TestCase):
    def test_isolated_build_and_artifact(self):
        self.assertIn("PSX_DVR_HDD_DIAG_G.ELF", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-G-RO-PAD-EXIT", WORKFLOW)
        self.assertIn("pad patches fileXio cdvd libdebug.a", BUILD)

    def test_ro_only_and_expected_files(self):
        self.assertIn('"hdd0:PP.TESTDVR"', CODE)
        self.assertIn('"pfs1:"', CODE)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", CODE)
        self.assertEqual(CODE.count("fileXioMount("), 1)
        self.assertEqual(CODE.count("fileXioUmount("), 1)
        self.assertIn("if (!have_our_mount) return;", CODE)
        self.assertIn("have_our_mount = 1;", CODE)
        for text in ("BOOT.CNF", "BOOT.ELF", "neutrino.elf", "dvr_pfs0:/ISO", "FIO_O_RDONLY"):
            self.assertIn(text, CODE)

    def test_exit_after_cleanup(self):
        self.assertLess(CODE.index("owned_unmount();\n    scr_clear();"), CODE.index("wait_for_start();"))
        self.assertIn("if (fresh & PAD_START) break;", CODE)
        self.assertIn("padPortClose(0, 0);", CODE)
        self.assertIn("padEnd();", CODE)
        self.assertIn("return 0;", CODE)
        self.assertIn("fresh = 0;", CODE)

    def test_driver_sequence(self):
        for prev, nxt in [
            ("SifIopReset(", "SifIopSync("),
            ("SifIopSync(", "SifInitRpc("),
            ('LOAD(iomanX, "G4', 'LOAD(fileXio, "G5'),
            ('LOAD(ps2dev9, "G6', 'LOAD(ps2atad, "G7'),
            ('module("G9 ps2fs"', "fileXioInit()"),
            ("fileXioInit()", 'LOAD(sio2man, "G11'),
            ("padPortOpen(0, 0, padbuf)", "fileXioMount("),
            ("fileXioMount(", "sceCdInit("),
            ("sceCdNoticeGameStart(", 'LOAD(dvrdrv, "G DVRDRV")'),
            ('LOAD(dvrdrv, "G DVRDRV")', 'LOAD(dvrfile, "G DVRFILE")'),
        ]:
            self.assertLess(CODE.index(prev), CODE.index(nxt))

    def test_no_writes(self):
        cleaned = re.sub(r"/\*.*?\*/|//[^\n]*", "", CODE, flags=re.S)
        for token in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                      "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                      "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_CREAT", "FIO_MT_RDWR",
                      "HDIOC_WRITESECTOR", "LoadELFFromFile"):
            self.assertNotIn(token, cleaned)


if __name__ == "__main__":
    unittest.main()
