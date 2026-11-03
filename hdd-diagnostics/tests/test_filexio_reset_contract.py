"""Diagnostic C is permitted to reset volatile IOP, but never to touch HDD.

This is a source-level guardrail, not a guarantee of safe hardware behavior.
"""
from pathlib import Path
import re
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "src" / "filexio_reset_diag.c"
BUILD = Path(__file__).resolve().parents[1] / "CMakeLists.txt"


class FileXioResetContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = SOURCE.read_text(encoding="utf-8")
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        cls.code = re.sub(r"//[^\n]*", "", text)

    def test_reset_and_embedded_rpc_service_before_filexio_init(self):
        steps = [
            "SifIopReset(", "SifIopSync(", "SifInitRpc(",
            "sbv_patch_enable_lmb(", "sbv_patch_disable_prefix_check(",
            "sbv_patch_fileio(", 'start_module("STEP C4 iomanX"',
            'start_module("STEP C5 fileXio"', "fileXioInit("
        ]
        positions = [self.code.index(step) for step in steps]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("STEP C6 fileXioInit: BEFORE", self.code)

    def test_no_disk_or_network_operations(self):
        forbidden = (
            "fileXioOpen", "fileXioDopen", "fileXioMount", "fileXioUmount",
            "fileXioFormat", "fileXioWrite", "fileXioRemove",
            "fileXioRename", "fileXioMkdir", "fileXioDevctl",
            "HDIOC_WRITESECTOR", "FIO_O_RDWR", "FIO_O_WRONLY",
            "dvr_pfs0:", "hdd0:", "PP.TESTDVR", "mass0:",
            "sceCdNoticeGameStart", "LoadELFFromFile",
        )
        for token in forbidden:
            with self.subTest(token=token):
                self.assertNotIn(token, self.code)

    def test_minimal_embedded_modules_only(self):
        build = BUILD.read_text(encoding="utf-8")
        self.assertIn("foreach(module iomanX fileXio)", build)
        self.assertIn("PSX_DVR_HDD_DIAG_C.ELF", build)


if __name__ == "__main__":
    unittest.main()
