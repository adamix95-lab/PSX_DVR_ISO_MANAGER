"""DIAG N: safe Quickboot path, stdio and isolated launcher checks."""
from pathlib import Path
import re
import unittest

BASE = Path(__file__).resolve().parents[1]
TARGET = (BASE / "src/osdmenu_quickboot_probe.c").read_text("utf-8")
LAUNCH = (BASE / "src/osdmenu_quickboot_launcher.c").read_text("utf-8")
CMAKE = (BASE / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (BASE.parent / ".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")

class DiagNContract(unittest.TestCase):
    def test_build_and_artifact(self):
        for tok in ("PSX_DVR_HDD_DIAG_N.ELF", "PSX_DVR_HDD_DIAG_N_LAUNCH.ELF",
                    "src/osdmenu_quickboot_probe.c", "src/osdmenu_quickboot_launcher.c",
                    "M_OSD_EMBEDDED", "G_DIAG_EMBEDDED"):
            self.assertIn(tok, CMAKE)
        self.assertIn("PSX-DVR-HDD-DIAG-N-QUICKBOOT-READONLY", WORKFLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_N.ELF", WORKFLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_N_LAUNCH.ELF", WORKFLOW)

    def test_launcher_x_gated(self):
        for tok in (
            '#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_N.ELF"',
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"',
            "if (!wait_for_choice(result == 0))",
            "if (enabled && (fresh & PAD_CROSS)) return 1;",
            "if (fresh & PAD_START) return 0;",
            "fresh = 0;", "PRECHECK FAILED; X DISABLED", "target_elf > 0",
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_N.ELF", 1',
            "rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);",
        ):
            self.assertIn(tok, LAUNCH)
        self.assertEqual(LAUNCH.count("rv = LoadELFFromFile("), 1)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile("))

    def test_target_quickboot(self):
        for tok in (
            '#define PFS_MOUNTPOINT "pfs0:"',
            'strcmp(argv[1], HDD_CNF_ARG) == 0',
            "if (!arg_match) { ret = -250; goto done; }",
            "ret = quickboot_probe(argv[1]);",
            'fopen(qb_cnf, "r")', "fgets(row, sizeof(row), fp)", "fclose(fp)",
            "ferror(fp)", "qb_boots++", "qb_paths++", "qb_args++",
            "qb_elf_size = file_size(31, qb_pfs_elf, 1)",
            'before(28, "QUICKBOOT NORMALIZE")',
            'before(29, "QUICKBOOT STDIO FOPEN")',
            'before(30, "QUICKBOOT FGETS PARSE")',
            'before(31, "QUICKBOOT ELF TARGET")',
            'before(32, "OWN PFS0 UMOUNT")',
            "qb_errno = errno;", "qb_stdio = 0;",
            "N CNF PATH: %s", "N TARGET HDD: %s",
            "STDIO=%d ERRNO=%d BOOT=%d",
            "QB ELF=%d ARGS=%d BSD=%d DVD=%d",
        ):
            self.assertIn(tok, TARGET)

    def test_read_only_and_cleanup(self):
        self.assertNotIn("LoadELFFromFile(", TARGET)
        for src in (TARGET, LAUNCH):
            stripped = re.sub(r"/\*.*?\*/|//[^\n]*", "", src, flags=re.S)
            for token in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                          "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                          "fileXioDevctl(", "FIO_O_WRONLY", "FIO_O_RDWR",
                          "FIO_O_CREAT", "FIO_O_TRUNC", "FIO_MT_RDWR",
                          "HDIOC_WRITESECTOR", "LoadExecPS2("):
                self.assertNotIn(token, stripped)
            self.assertEqual(src.count("fileXioMount("), 1)
            self.assertEqual(src.count("fileXioUmount("), 1)
            self.assertIn("FIO_MT_RDONLY", src)
        self.assertIn("if (!own_mount) return 0;", TARGET)
        self.assertIn("if (!have_our_mount) return;", LAUNCH)
        self.assertIn("finish_on_start();", TARGET)

if __name__ == "__main__":
    unittest.main()
