"""Read-only source contract for DIAG D / DESR-7000.

Only checking the first PS2 APA/PFS mount attempt. No DVR module loads
until hardware validates D. This does not substitute for physical testing.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/apa_pfs_diag.c"
BUILD = ROOT / "CMakeLists.txt"


def code_without_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
    return re.sub(r"//[^\n]*", "", source)


class APAPFSReadOnlyContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.src = code_without_comments(SOURCE.read_text(encoding="utf-8"))

    def test_known_good_filexio_and_module_sequence(self):
        self.assertIn("SifIopReset(", self.src)
        self.assertIn("SifIopSync(", self.src)
        self.assertIn("sbv_patch_fileio(", self.src)
        self.assertIn("fileXioInit(", self.src)
        modules = re.findall(r'LOAD0\((\w+), "D\d+ [^"]+"\)', self.src)
        self.assertEqual(modules, ["iomanX", "fileXio", "ps2dev9", "ps2atad"])
        self.assertIn('start_irx("D8 ps2hdd"', self.src)
        self.assertIn('start_irx("D9 ps2fs"', self.src)
        self.assertLess(self.src.index('start_irx("D9 ps2fs"'),
                        self.src.index("fileXioInit("))
        self.assertIn('"-o\\0" "4\\0" "-n\\0" "20"', self.src)
        self.assertIn('"-m\\0" "4\\0" "-o\\0" "10\\0" "-n\\0" "40"', self.src)

    def test_test_partition_only_read_only_mount(self):
        self.assertIn('"hdd0:PP.TESTDVR"', self.src)
        self.assertIn('"pfs1:"', self.src)
        mounts = re.findall(r"\bfileXioMount\s*\([^;]*\);", self.src, flags=re.S)
        self.assertEqual(len(mounts), 1)
        self.assertIn("FIO_MT_RDONLY", mounts[0])
        self.assertEqual(len(re.findall(r"\bfileXioUmount\s*\(", self.src)), 1)
        self.assertIn("if (mounted)", self.src)
        self.assertIn('fileXioDopen(PFS_MOUNTPOINT "/")', self.src)

    def test_no_dvr_or_disk_writes(self):
        forbidden = (
            "fileXioWrite", "fileXioFormat", "fileXioRemove", "fileXioRename",
            "fileXioMkdir", "fileXioRmdir", "fileXioChstat", "fileXioDevctl",
            "FIO_MT_RDWR", "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_TRUNC",
            "FIO_O_CREAT", "dvrdrv", "dvrfile", "dvr_pfs0", "HDIOC_WRITESECTOR",
            "sceCdNoticeGameStart", "LoadELFFromFile",
        )
        for token in forbidden:
            with self.subTest(token=token):
                self.assertNotIn(token, self.src)

    def test_diag_d_has_its_own_elf(self):
        build = BUILD.read_text(encoding="utf-8")
        self.assertIn("PSX_DVR_HDD_DIAG_D.ELF", build)
        self.assertIn("foreach(module iomanX fileXio ps2dev9 ps2atad ps2hdd ps2fs)", build)


if __name__ == "__main__":
    unittest.main()
