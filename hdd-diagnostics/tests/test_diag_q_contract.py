"""DIAG Q: real OSDMenu loader with strict alternative F-only CNF, X gated."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src" / "osdmenu_real_loader_q.c").read_text("utf-8")
CNF = (ROOT / "diag-q" / "DIAG_Q.CNF").read_bytes()
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")
EXPECTED = b"boot=PSX_DVR_HDD_DIAG_F.ELF\n"


class DiagQContract(unittest.TestCase):
    def test_artifact(self):
        self.assertIn("PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF", BUILD)
        self.assertIn("src/osdmenu_real_loader_q.c", BUILD)
        self.assertIn("pad patches fileXio cdvd elf-loader libdebug.a", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-Q-REAL-OSDMENU-TO-F", WORKFLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF", WORKFLOW)
        self.assertIn("hdd-diagnostics/diag-q/DIAG_Q.CNF", WORKFLOW)
        self.assertNotIn("PSX_DVR_HDD_DIAG_Q.ELF", WORKFLOW)

    def test_config_exact_and_only_f_target(self):
        self.assertEqual(CNF, EXPECTED)
        self.assertEqual(CNF.count(b"\n"), 1)
        self.assertNotIn(b"neutrino", CNF.lower())
        self.assertNotIn(b"arg=", CNF)
        self.assertNotIn(b"mass0:", CNF)
        for required in (
            '#define Q_CNF_PATH PFS_MOUNTPOINT "/DIAG_Q.CNF"',
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_Q.CNF"',
            '#define F_TARGET_PATH PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_F.ELF"',
            'static const char q_expected_config[] = "boot=PSX_DVR_HDD_DIAG_F.ELF\\n";',
            "fileXioOpen(Q_CNF_PATH, FIO_O_RDONLY, 0)",
            "if (length != (int)sizeof(q_expected_config) - 1)",
            "memcmp(q_expected_config, q_config_bytes, (size_t)length)",
            "q_config_ok == (int)sizeof(q_expected_config) - 1",
            "f_target_size > 0",
            'f_target_size = inspect_file("Q TEST F", F_TARGET_PATH, 1);',
        ):
            self.assertIn(required, SRC)

    def test_real_osdmenu_launch_not_replaced_with_simulated_loader(self):
        self.assertIn('#define LOADED_ELF "pfs1:/BOOT.ELF"', SRC)
        self.assertIn('char *launcher_args[1] = {(char *)HDD_CNF_ARG};', SRC)
        self.assertIn("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);", SRC)
        self.assertEqual(SRC.count("rv = LoadELFFromFile("), 1)
        self.assertIn("Q BEFORE LoadELFFromFile", SRC)
        self.assertIn("EXPECTED TARGET: DIAG F ONLY.", SRC)
        self.assertNotIn("hdd0:PP.TESTDVR/neutrino.elf", SRC)

    def test_x_requires_successful_preflight_and_fresh_press(self):
        self.assertIn("if (!wait_for_choice(result == 0))", SRC)
        self.assertIn("if (enabled && (fresh & PAD_CROSS)) return 1;", SRC)
        self.assertIn("if (fresh & PAD_START) return 0;", SRC)
        self.assertIn("fresh = 0;", SRC)
        self.assertIn("PRECHECK FAILED; X DISABLED", SRC)
        self.assertIn("X: RUN OSDMENU -> DIAG F", SRC)
        self.assertLess(SRC.index("if (!wait_for_choice(result == 0))"),
                        SRC.index("rv = LoadELFFromFile("))
        self.assertIn("owned_unmount();\n        padPortClose(0, 0);\n        padEnd();\n        return 0;", SRC)

    def test_readonly_launcher_and_own_mount(self):
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", SRC)
        self.assertEqual(SRC.count("fileXioMount("), 1)
        self.assertEqual(SRC.count("fileXioUmount("), 1)
        self.assertIn("if (!have_our_mount) return;", SRC)
        src = re.sub(r"/\*.*?\*/|//[^\n]*", "", SRC, flags=re.S)
        for forb in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                     "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                     "fileXioChstat", "FIO_MT_RDWR", "FIO_O_CREAT",
                     "FIO_O_TRUNC", "FIO_O_WRONLY", "FIO_O_RDWR", "LoadExecPS2("):
            with self.subTest(forbidden=forb):
                self.assertNotIn(forb, src)


if __name__ == "__main__":
    unittest.main()
