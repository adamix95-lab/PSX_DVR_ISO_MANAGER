"""DIAG F: a minimal pad EXIT proof with no disk access or writes."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "pad_exit_diag.c"
BUILD = ROOT / "CMakeLists.txt"


class PadExitContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        raw = SRC.read_text(encoding="utf-8")
        src = re.sub(r"/\*.*?\*/", "", raw, flags=re.S)
        cls.code = re.sub(r"//[^\n]*", "", src)

    def test_known_v08_iop_pad_driver_order(self):
        code = self.code
        tokens = [
            "SifIopReset(", "SifIopSync(", "SifInitRpc(",
            'LOAD(iomanX, "F4 iomanX")',
            'LOAD(fileXio, "F5 fileXio")',
            "fileXioInit()",
            'LOAD(sio2man, "F7 sio2man")',
            'LOAD(mcman, "F8 mcman")',
            'LOAD(mcserv, "F9 mcserv")',
            'LOAD(freepad, "F10 freepad")',
            "padInit(0)", "padPortOpen(0, 0, padbuf)",
        ]
        pos = [code.index(x) for x in tokens]
        self.assertEqual(pos, sorted(pos), tokens)

    def test_start_only_after_successful_pad_init(self):
        code = self.code
        self.assertIn("if (!rv) goto failure;", code)
        self.assertIn("if (new_press & PAD_START)", code)
        self.assertIn("padRead(0, 0, &btn)", code)
        self.assertIn("pad_sampled = 1;", code)
        self.assertIn("new_press = 0;", code)
        self.assertLess(code.index("padPortClose(0, 0)"), code.index("padEnd();\n    return 0;"))
        self.assertIn("PRESS START: EXIT TO XMB/OSD", code)

    def test_does_not_touch_hdd_or_usb(self):
        code = self.code
        forbidden = [
            "fileXioMount", "fileXioUmount", "fileXioOpen",
            "fileXioDopen", "fileXioWrite", "fileXioDevctl",
            "fileXioFormat", "fileXioRemove", "fileXioRename",
            "fileXioMkdir", "fileXioRmdir", "fileXioChstat",
            "LoadELFFromFile", "hdd0:", "dvr_pfs0:",
            "dvr_hdd0:", "PP.TESTDVR", "mass0:",
            "FIO_MT_RDWR", "FIO_O_WRONLY", "FIO_O_RDWR",
            "FIO_O_CREAT", "HDIOC_WRITESECTOR",
        ]
        for token in forbidden:
            with self.subTest(token=token):
                self.assertNotIn(token, code)

    def test_independent_elf_and_pad_library(self):
        build = BUILD.read_text(encoding="utf-8")
        self.assertIn("PSX_DVR_HDD_DIAG_F.ELF", build)
        self.assertIn(
            "foreach(module iomanX fileXio sio2man mcman mcserv freepad)",
            build)
        self.assertIn("pad patches fileXio libdebug.a", build)


if __name__ == "__main__":
    unittest.main()
