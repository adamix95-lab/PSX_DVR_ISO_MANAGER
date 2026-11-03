"""R3 contract: no loosened configuration beyond last LF->NUL."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
LAUNCH = (ROOT/"src/osdmenu_arg_echo_r3_launcher.c").read_text("utf-8")
CM = (ROOT/"CMakeLists.txt").read_text("utf-8")
WF = (ROOT.parent/".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")
CONFIG = (ROOT/"diag-r/DIAG_R.CNF").read_bytes()

class R3Contract(unittest.TestCase):
    def test_expected_unchanged_config_and_target(self):
        self.assertEqual(CONFIG, b"boot=PSX_DVR_HDD_DIAG_R_ARG.ELF\narg=-bsd=dvr\narg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso\n")
        self.assertEqual(len(CONFIG),87)
        self.assertIn('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_R.CNF"', LAUNCH)
        self.assertIn('#define LOADED_ELF "pfs1:/BOOT.ELF"', LAUNCH)
    def test_narrow_terminator_exception(self):
        for x in ("length != (int)sizeof(q_expected_config) - 1",
                  "got != length",
                  "memcmp(q_expected_config, q_config_bytes, (size_t)(length - 1))",
                  r"q_last_byte != '\n' && q_last_byte != 0",
                  "q_last_is_nul = (q_last_byte == 0)",
                  "q_config_ok == (int)sizeof(q_expected_config) - 1",
                  "q_last_byte = (unsigned char)q_config_bytes[length - 1]"):
            self.assertIn(x, LAUNCH)
    def test_original_osdmenu_still_x_gated(self):
        for x in ("if (!wait_for_choice(result == 0))",
                  "if (enabled && (fresh & PAD_CROSS)) return 1;",
                  "if (fresh & PAD_START) return 0;",
                  "fresh = 0;",
                  "rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);",
                  "RCNF PREFIX EXACT. LAST=%02X NUL=%d"):
            self.assertIn(x, LAUNCH)
    def test_no_write_calls_and_separate_artifact(self):
        self.assertIn("PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF", CM)
        self.assertIn("PSX-DVR-HDD-DIAG-R3-LF-OR-NUL", WF)
        self.assertIn("FIO_MT_RDONLY", LAUNCH)
        stripped=re.sub(r"/\*.*?\*/|//[^\n]*","",LAUNCH,flags=re.S)
        for bad in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                    "fileXioRename","FIO_MT_RDWR","FIO_O_WRONLY","FIO_O_TRUNC"):
            self.assertNotIn(bad,stripped)

if __name__ == "__main__":
    unittest.main()
