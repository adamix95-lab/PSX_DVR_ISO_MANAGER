"""DIAG R: real OSDMenu sends Neutrino-style argument strings to a benign echo ELF."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = (ROOT / "src/osdmenu_arg_echo_target.c").read_text("utf-8")
LAUNCH = (ROOT / "src/osdmenu_arg_echo_launcher.c").read_text("utf-8")
CONFIG = (ROOT / "diag-r/DIAG_R.CNF").read_bytes()
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
FLOW = (ROOT.parent / ".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")
EXPECTED = (b"boot=PSX_DVR_HDD_DIAG_R_ARG.ELF\n"
            b"arg=-bsd=dvr\n"
            b"arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso\n")


class DiagRContract(unittest.TestCase):
    def test_exact_no_game_cnf(self):
        self.assertEqual(CONFIG, EXPECTED)
        self.assertEqual(len(CONFIG), 87)
        self.assertNotIn(b"neutrino", CONFIG.lower())
        self.assertNotIn(b"-dev9=", CONFIG)
        self.assertNotIn(b"mass0:", CONFIG)
        self.assertIn(b"DIAG_R_NO_GAME.iso", CONFIG)

    def test_separate_build_targets_and_artifact(self):
        for x in ("PSX_DVR_HDD_DIAG_R_ARG.ELF", "PSX_DVR_HDD_DIAG_R_LAUNCH.ELF",
                  "src/osdmenu_arg_echo_target.c", "src/osdmenu_arg_echo_launcher.c",
                  "EXIT_DIAG_EMBEDDED", "G_DIAG_EMBEDDED"):
            self.assertIn(x, BUILD)
        for x in ("PSX-DVR-HDD-DIAG-R-OSDMENU-ARGV-ONLY",
                  "build-hdd-diag/PSX_DVR_HDD_DIAG_R_ARG.ELF",
                  "build-hdd-diag/PSX_DVR_HDD_DIAG_R_LAUNCH.ELF",
                  "hdd-diagnostics/diag-r/DIAG_R.CNF"):
            self.assertIn(x, FLOW)

    def test_launcher_preflight_matches_actual_cnf_and_target(self):
        self.assertIn('#define LOADED_ELF "pfs1:/BOOT.ELF"', LAUNCH)
        self.assertIn('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_R.CNF"', LAUNCH)
        self.assertIn('#define Q_CNF_PATH PFS_MOUNTPOINT "/DIAG_R.CNF"', LAUNCH)
        self.assertIn('#define F_TARGET_PATH PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_R_ARG.ELF"', LAUNCH)
        for x in ('"boot=PSX_DVR_HDD_DIAG_R_ARG.ELF\\n"',
                  '"arg=-bsd=dvr\\n"',
                  '"arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso\\n"',
                  "memcmp(q_expected_config, q_config_bytes, (size_t)length)",
                  "q_config_ok == (int)sizeof(q_expected_config) - 1",
                  "f_target_size > 0", "boot_elf > 0", "neutrino_elf > 0",
                  "config_type == 1 && bsd_dvr && has_dvd",
                  "if (!wait_for_choice(result == 0))",
                  "if (enabled && (fresh & PAD_CROSS)) return 1;",
                  "if (fresh & PAD_START) return 0;",
                  "fresh = 0;",
                  "rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);"):
            self.assertIn(x, LAUNCH)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);"))

    def test_target_echoes_arguments_without_accessing_game(self):
        for x in ("int main(int argc, char *argv[])",
                  'strcmp(saved_argv1, "-bsd=dvr") == 0',
                  "strcmp(saved_argv2, EXPECTED_DVD) == 0",
                  "ARGC=%d ARGS_OK=%d",
                  "ARGV1: %.53s", "ARGV2: %.61s",
                  "DUMMY ISO PATH - NOT OPENED.",
                  "padPortOpen(0, 0, padbuf)",
                  "padPortClose(0, 0)", "PAD_START"):
            self.assertIn(x, TARGET)
        self.assertNotIn("neutrino.elf", TARGET)
        self.assertNotIn("dvrdrv_irx", TARGET)
        self.assertNotIn("ps2hdd_irx", TARGET)
        self.assertNotIn("fileXioOpen(", TARGET)
        self.assertNotIn("LoadELFFromFile(", TARGET)

    def test_readonly_preflight_and_no_write_api(self):
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", LAUNCH)
        self.assertEqual(LAUNCH.count("fileXioMount("), 1)
        self.assertEqual(LAUNCH.count("fileXioUmount("), 1)
        self.assertIn("if (!have_our_mount) return;", LAUNCH)
        for src in (TARGET, LAUNCH):
            code = re.sub(r"/\*.*?\*/|//[^\n]*", "", src, flags=re.S)
            for x in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                      "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                      "fileXioChstat", "FIO_O_WRONLY", "FIO_O_RDWR",
                      "FIO_O_CREAT", "FIO_O_TRUNC", "FIO_MT_RDWR",
                      "HDIOC_WRITESECTOR", "LoadExecPS2("):
                with self.subTest(x=x):
                    self.assertNotIn(x, code)


if __name__ == "__main__":
    unittest.main()
