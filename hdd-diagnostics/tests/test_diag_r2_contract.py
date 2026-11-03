"""DIAG R2 — verify exact byte-level CNF diagnostic without any chainload."""
import unittest
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT/"src"/"osdmenu_r2_cnf_byte_probe.c").read_text("utf-8")
CMAKE = (ROOT/"CMakeLists.txt").read_text("utf-8")
FLOW = (ROOT.parent/".github"/"workflows"/"build-hdd-readonly-diag.yml").read_text("utf-8")
CONFIG = (ROOT/"diag-r"/"DIAG_R.CNF").read_bytes()


class R2Contract(unittest.TestCase):
    def test_separate_standalone_build(self):
        for token in ("PSX_DVR_HDD_DIAG_R2_CNF.ELF",
                      "src/osdmenu_r2_cnf_byte_probe.c",
                      "G_DIAG_EMBEDDED"):
            self.assertIn(token, CMAKE)
        self.assertIn("PSX-DVR-HDD-DIAG-R2-CNF-BYTE-INSPECTOR", FLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_R2_CNF.ELF", FLOW)

    def test_expected_cnf_identical_to_original(self):
        self.assertEqual(CONFIG, (b"boot=PSX_DVR_HDD_DIAG_R_ARG.ELF\n"
                                  b"arg=-bsd=dvr\n"
                                  b"arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso\n"))
        self.assertEqual(len(CONFIG), 87)
        self.assertIn('"boot=PSX_DVR_HDD_DIAG_R_ARG.ELF\\n"', SRC)
        self.assertIn('"arg=-bsd=dvr\\n"', SRC)
        self.assertIn('"arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso\\n"', SRC)

    def test_byte_mismatch_is_recorded(self):
        for token in ("r2_real_len = length;", "r2_read_len = got;",
                      "r2_expected_len = (int)sizeof(q_expected_config) - 1;",
                      "r2_mismatch = i;", "r2_expected_byte = (int)a;",
                      "r2_actual_byte = (int)b;",
                      "rc = r2_mismatch >= 0 ? -344 : length;",
                      "r2_cr_count++", "r2_lf_count++", "r2_nul_count++",
                      "r2_exp_sum += a;", "r2_got_sum += b;",
                      'print_r2_bytes("E: ",',
                      'print_r2_bytes("A: ",',
                      "DIFF OFFSET=%d E=%02X A=%02X",
                      "SIZE=%d EXPECT=%d READ=%d",
                      "READONLY. NO ELF LAUNCH."):
            self.assertIn(token, SRC)

    def test_x_is_always_disabled(self):
        self.assertIn("if (!wait_for_choice(0))", SRC)
        self.assertNotIn("if (!wait_for_choice(result == 0))", SRC)
        self.assertIn("PRECHECK FAILED; X DISABLED", SRC)
        self.assertNotIn("LoadELFFromFile(", SRC)
        self.assertNotIn("LoadExecPS2(", SRC)
        self.assertNotIn("ExecPS2(", SRC)
        self.assertNotIn("SifLoadElf(", SRC)
        self.assertNotIn("launcher_args", SRC)

    def test_readonly_mount_and_no_applevel_writes(self):
        self.assertEqual(SRC.count("fileXioMount("), 1)
        self.assertEqual(SRC.count("fileXioUmount("), 1)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", SRC)
        self.assertIn("if (!have_our_mount) return;", SRC)
        code = re.sub(r"/\*.*?\*/|//[^\n]*", "", SRC, flags=re.S)
        for bad in ("fileXioWrite", "fileXioFormat", "fileXioRemove", "fileXioRename",
                    "fileXioMkdir", "fileXioRmdir", "FIO_O_RDWR", "FIO_MT_RDWR",
                    "FIO_O_CREAT", "FIO_O_TRUNC", "FIO_O_WRONLY", "HDIOC_WRITESECTOR"):
            self.assertNotIn(bad, code)


if __name__ == "__main__":
    unittest.main()
