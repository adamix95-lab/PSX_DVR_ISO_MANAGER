"""Source-level safety contract for initial HDD diagnostic ELF.

This check is intentionally conservative. It cannot prove safety on hardware;
it catches accidental introduction of write operations before CI produces an ELF.
"""
import pathlib
import re
import unittest

SOURCE = pathlib.Path(__file__).resolve().parents[1] / "src" / "main.c"


class ReadOnlyContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")
        # Ignore C comments when checking prohibited operations.
        src = re.sub(r"/\*.*?\*/", "", cls.source, flags=re.S)
        cls.code = re.sub(r"//[^\n]*", "", src)

    def test_explicit_test_partition_and_read_only_mount(self):
        self.assertIn('"hdd0:PP.TESTDVR"', self.code)
        self.assertIn('"pfs1:"', self.code)
        mounts = re.findall(r"\bfileXioMount\s*\([^;]*\);", self.code, flags=re.S)
        self.assertEqual(len(mounts), 1, mounts)
        self.assertIn("FIO_MT_RDONLY", mounts[0])
        self.assertNotIn("FIO_MT_RDWR", self.code)

    def test_no_disk_writes_partition_operations_or_iop_reset(self):
        forbidden = (
            "fileXioWrite", "fileXioFormat", "fileXioRemove", "fileXioRename",
            "fileXioMkdir", "fileXioRmdir", "fileXioChstat",
            "SifIopReset", "SifExecModuleBuffer", "LoadELFFromFile",
            "fileXioDevctl", "HDIOC_WRITESECTOR", "FIO_O_WRONLY",
            "FIO_O_RDWR", "FIO_O_TRUNC", "FIO_O_CREAT",
        )
        for name in forbidden:
            with self.subTest(name=name):
                self.assertNotRegex(self.code, r"\b" + re.escape(name) + r"\b")

    def test_only_read_only_open_and_owned_mount_is_unmounted(self):
        self.assertRegex(self.code, r"fileXioOpen\s*\(\s*path\s*,\s*FIO_O_RDONLY")
        self.assertRegex(self.code, r"fileXioUmount\s*\(\s*DIAG_MOUNT\s*\)")
        self.assertIn("if (mount_result < 0)", self.code)
        self.assertIn("if (check >= 0)", self.code)
        self.assertIn('"dvr_pfs0:/ISO"', self.code)


if __name__ == "__main__":
    unittest.main()
