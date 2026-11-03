"""DIAG J: guarded first OSDMenu launch from owner-created PP.TESTDVR."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src" / "apa_dvr_hdd_launch_diag.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
FLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagJContract(unittest.TestCase):
    def test_separate_build_target_and_ci(self):
        self.assertIn("PSX_DVR_HDD_DIAG_J.ELF", BUILD)
        self.assertIn("src/apa_dvr_hdd_launch_diag.c", BUILD)
        self.assertIn("pad patches fileXio cdvd elf-loader libdebug.a", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-J-X-ARMED-OSDMENU-LAUNCH", FLOW)

    def test_readonly_preflight_and_owned_mount(self):
        self.assertIn('"hdd0:PP.TESTDVR"', SRC)
        self.assertIn('"pfs1:"', SRC)
        self.assertEqual(SRC.count("fileXioMount("), 1)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", SRC)
        self.assertIn("if (!have_our_mount) return;", SRC)
        self.assertIn("have_our_mount = 1;", SRC)
        self.assertEqual(SRC.count("fileXioUmount("), 1)
        self.assertIn('fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0)', SRC)
        self.assertIn("boot_cnf > 0 && boot_elf > 0 && neutrino_elf > 0", SRC)
        self.assertIn("dvd_open_code >= 0", SRC)
        self.assertIn("config_type == 1 && bsd_dvr && has_dvd", SRC)
        self.assertIn("!usb_dep", SRC)

    def test_never_auto_launch(self):
        self.assertEqual(SRC.count("LoadELFFromFile("), 1)
        self.assertIn("if (!wait_for_choice(result == 0))", SRC)
        self.assertIn("if (enabled && (fresh & PAD_CROSS)) return 1;", SRC)
        self.assertIn("if (fresh & PAD_START) return 0;", SRC)
        self.assertIn("fresh = 0;", SRC)
        self.assertIn("X: LAUNCH HDD OSDMENU/GAME", SRC)
        self.assertIn("PRECHECK FAILED; X DISABLED", SRC)
        self.assertLess(SRC.index("if (!wait_for_choice(result == 0))"),
                        SRC.index("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);"))

    def test_launch_from_pfs_but_target_config_on_hdd(self):
        self.assertIn('#define LOADED_ELF "pfs1:/BOOT.ELF"', SRC)
        self.assertIn('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"', SRC)
        self.assertIn('char *launcher_args[1] = {(char *)HDD_CNF_ARG};', SRC)
        self.assertIn("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);", SRC)
        self.assertIn("J BEFORE LoadELFFromFile", SRC)
        # For the chainload, PFS must remain mounted while the loader reads BOOT.ELF.
        self.assertLess(SRC.index("owned_unmount();", SRC.index("if (!wait_for_choice")),
                        SRC.index("rv = LoadELFFromFile"))
        self.assertNotIn("LoadELFFromFile(LOADED_ELF, 0, NULL)", SRC)

    def test_start_cleans_up_and_no_iop_calls_after_failed_launch(self):
        start_branch = SRC[SRC.index("if (!wait_for_choice(result == 0))"):]
        self.assertRegex(start_branch, r"owned_unmount\(\);\s*padPortClose\(0, 0\);\s*padEnd\(\);\s*return 0;")
        after_loader = SRC.split("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);", 1)[1]
        for function in ("fileXioMount(", "fileXioUmount(", "fileXioDopen(",
                         "fileXioOpen(", "padPortOpen(", "padRead(", "SifIopReset("):
            self.assertNotIn(function, after_loader)
        self.assertIn("J CHAINLOAD RETURNED=%d", after_loader)

    def test_no_write_or_system_partition_mutation_in_diag(self):
        code = re.sub(r"/\*.*?\*/|//[^\n]*", "", SRC, flags=re.S)
        for forbidden in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                          "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                          "fileXioChstat", "FIO_O_WRONLY", "FIO_O_RDWR",
                          "FIO_O_CREAT", "FIO_O_TRUNC", "FIO_MT_RDWR",
                          "HDIOC_WRITESECTOR", "LoadExecPS2("):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, code)


if __name__ == "__main__":
    unittest.main()
