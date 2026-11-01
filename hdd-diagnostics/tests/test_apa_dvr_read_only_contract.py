"""Stage E: read-only dual PS2 APA/PFS + PSX DVR diagnostic guards."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "apa_dvr_coexist_diag.c"
BUILD = ROOT / "CMakeLists.txt"


class DualHDDCoexistenceContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = SOURCE.read_text("utf-8")
        source = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
        cls.code = re.sub(r"//[^\n]*", "", source)
        cls.build = BUILD.read_text("utf-8")

    def test_known_good_iop_stack_and_repairbox_hdd_parameters(self):
        text = self.code
        self.assertLess(text.index('LOAD(iomanX, "E4 iomanX")'),
                        text.index('LOAD(fileXio, "E5 fileXio")'))
        self.assertLess(text.index('LOAD(ps2dev9, "E6 ps2dev9")'),
                        text.index('LOAD(ps2atad, "E7 ps2atad")'))
        self.assertLess(text.index('module("E9 ps2fs"'), text.index("fileXioInit()"))
        self.assertLess(text.index("fileXioInit()"), text.index("fileXioMount("))
        self.assertIn('"-o\\0" "4\\0" "-n\\0" "20"', text)
        self.assertIn('"-m\\0" "4\\0" "-o\\0" "10\\0" "-n\\0" "40"', text)

    def test_only_owned_pfs1_read_only_mount(self):
        text = self.code
        self.assertIn('"hdd0:PP.TESTDVR"', text)
        self.assertIn('"pfs1:"', text)
        mounts = re.findall(r"\bfileXioMount\s*\([^;]*\);", text, flags=re.S)
        self.assertEqual(len(mounts), 1)
        self.assertIn("FIO_MT_RDONLY", mounts[0])
        self.assertEqual(len(re.findall(r"\bfileXioUmount\s*\(", text)), 1)
        self.assertIn("if (!have_our_mount)", text)
        self.assertIn("have_our_mount = 1", text)
        self.assertIn("owned_unmount()", text)
        self.assertIn('fileXioDopen(PFS_MOUNTPOINT "/")', text)

    def test_dvr_starts_after_ps2_ro_mount(self):
        text = self.code
        self.assertLess(text.index("fileXioMount("), text.index("sceCdInit("))
        self.assertLess(text.index("sceCdInit("), text.index("sceCdNoticeGameStart("))
        self.assertLess(text.index("sceCdNoticeGameStart("),
                        text.index('module("E17 dvrdrv"'))
        self.assertLess(text.index('module("E17 dvrdrv"'),
                        text.index('module("E18 dvrfile"'))
        self.assertLess(text.index('module("E18 dvrfile"'),
                        text.index('"dvr_pfs0:/ISO"'))
        self.assertLess(text.index('"dvr_pfs0:/ISO"'),
                        text.index('directory("E22 PS2 PFS1"'))
        self.assertIn('directory("E19 DVR HDD", "dvr_hdd0:")', text)
        self.assertIn('directory("E20 DVR PFS", "dvr_pfs0:/")', text)
        self.assertIn('directory("E21 DVR ISO", "dvr_pfs0:/ISO")', text)

    def test_no_disk_write_or_unsafe_extra_driver(self):
        for token in (
            "fileXioWrite", "fileXioFormat", "fileXioRemove", "fileXioRename",
            "fileXioMkdir", "fileXioRmdir", "fileXioChstat", "fileXioDevctl",
            "HDIOC_WRITESECTOR", "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_CREAT",
            "FIO_O_TRUNC", "FIO_MT_RDWR", "dev9_ns", "LoadELFFromFile",
            "PSX_XMB", "mass0:", "open(", "fopen(", "sceCdWrite",
        ):
            with self.subTest(token=token):
                if token in ("open(", "fopen("):
                    self.assertNotIn(token, self.code)
                else:
                    self.assertNotRegex(self.code, r"\b" + re.escape(token) + r"\b")

    def test_original_dvr_modules_embedded(self):
        self.assertIn("PSX_DVR_HDD_DIAG_E.ELF", self.build)
        self.assertIn("foreach(module dvrdrv dvrfile)", self.build)
        self.assertIn("dvr-iso-manager/modules/", self.build)


if __name__ == "__main__":
    unittest.main()
