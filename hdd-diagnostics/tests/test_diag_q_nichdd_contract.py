"""DIAG Q NICHDD experiment: original OSDMenu -> benign F with strict isolated CNF."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
LAUNCH = (ROOT / "src/osdmenu_real_loader_f_probe.c").read_text("utf-8")
CNF = (ROOT / "config/BOOTQ.CNF").read_bytes()
CMAKE = (ROOT / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (ROOT.parent / ".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")
EXPECTED = b"boot=PSX_DVR_HDD_DIAG_F.ELF\narg=-dev9=NICHDD\n"


class DiagQSafeContract(unittest.TestCase):
    def test_separate_binary_and_artifact(self):
        self.assertIn("src/osdmenu_real_loader_f_probe.c", CMAKE)
        self.assertIn("PSX_DVR_HDD_DIAG_Q.ELF", CMAKE)
        self.assertIn("pad patches fileXio cdvd elf-loader libdebug.a", CMAKE)
        for x in ("PSX-DVR-HDD-DIAG-Q-ORIGINAL-OSDMENU-TO-F",
                  "build-hdd-diag/PSX_DVR_HDD_DIAG_Q.ELF",
                  "hdd-diagnostics/config/BOOTQ.CNF"):
            self.assertIn(x, WORKFLOW)

    def test_exact_test_only_config_and_size_guard(self):
        self.assertEqual(CNF, EXPECTED)
        self.assertEqual(len(CNF), 45)
        self.assertNotIn(b"neutrino", CNF.lower())
        self.assertNotIn(b"-dvd=", CNF)
        self.assertNotIn(b"mass0:", CNF.lower())
        for x in ('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOTQ.CNF"',
                  '"boot=PSX_DVR_HDD_DIAG_F.ELF\\n"',
                  '"arg=-dev9=NICHDD\\n"',
                  'fileXioOpen(PFS_MOUNTPOINT "/BOOTQ.CNF", FIO_O_RDONLY, 0)',
                  "memcmp(config_bytes, expected_q_cnf, (size_t)length)",
                  "boot_cnf == (int)(sizeof(expected_q_cnf) - 1)",
                  "boot_elf == 197396 && target_f == 1524548",
                  'PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_F.ELF", 1'):
            if x.endswith('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_F.ELF", 1'):
                # The safe variant reads F via inspect_file, not a shared macro.
                self.assertIn('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_F.ELF", 1', LAUNCH)
            else:
                self.assertIn(x, LAUNCH)

    def test_actual_original_osdmenu_is_launched(self):
        self.assertIn('#define LOADED_ELF "pfs1:/BOOT.ELF"', LAUNCH)
        self.assertIn('char *launcher_args[1] = {(char *)HDD_CNF_ARG};', LAUNCH)
        self.assertEqual(LAUNCH.count("rv = LoadELFFromFile("), 1)
        self.assertIn("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);", LAUNCH)
        self.assertIn("Q BEFORE REAL OSDMENU", LAUNCH)
        self.assertIn("F ONLY. NEVER BOOT GAME.", LAUNCH)

    def test_fresh_x_and_start_cancel(self):
        for x in ("if (!wait_for_choice(result == 0))",
                  "if (enabled && (fresh & PAD_CROSS)) return 1;",
                  "if (fresh & PAD_START) return 0;",
                  "fresh = 0;",
                  "PRECHECK FAILED; X DISABLED",
                  "owned_unmount();\n        padPortClose(0, 0);\n        padEnd();\n        return 0;"):
            self.assertIn(x, LAUNCH)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);"))

    def test_no_disk_writes_by_diagnostic(self):
        self.assertEqual(LAUNCH.count("fileXioMount("), 1)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", LAUNCH)
        self.assertEqual(LAUNCH.count("fileXioUmount("), 1)
        self.assertIn("if (!have_our_mount) return;", LAUNCH)
        code = re.sub(r"/\*.*?\*/|//[^\n]*", "", LAUNCH, flags=re.S)
        for x in ("fileXioWrite", "fileXioFormat", "fileXioRemove", "fileXioRename",
                  "fileXioMkdir", "fileXioRmdir", "fileXioChstat", "FIO_MT_RDWR",
                  "FIO_O_CREAT", "FIO_O_TRUNC", "FIO_O_WRONLY", "FIO_O_RDWR",
                  "LoadExecPS2("):
            with self.subTest(x=x):
                self.assertNotIn(x, code)


if __name__ == "__main__":
    unittest.main()
