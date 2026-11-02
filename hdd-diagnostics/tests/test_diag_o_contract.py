"""DIAG O: constrain read-only handlePFS remount and loader-path preparation."""
from pathlib import Path
import re
import unittest

BASE = Path(__file__).resolve().parents[1]
TARGET = (BASE / "src" / "osdmenu_handlepfs_probe.c").read_text("utf-8")
LAUNCH = (BASE / "src" / "osdmenu_handlepfs_launcher.c").read_text("utf-8")
CMAKE = (BASE / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (BASE.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagOContract(unittest.TestCase):
    def test_separate_binaries_and_artifact(self):
        for value in ("PSX_DVR_HDD_DIAG_O.ELF", "PSX_DVR_HDD_DIAG_O_LAUNCH.ELF",
                      "src/osdmenu_handlepfs_probe.c", "src/osdmenu_handlepfs_launcher.c",
                      "M_OSD_EMBEDDED", "G_DIAG_EMBEDDED"):
            self.assertIn(value, CMAKE)
        for value in ("PSX-DVR-HDD-DIAG-O-HANDLEPFS-READONLY",
                      "build-hdd-diag/PSX_DVR_HDD_DIAG_O.ELF",
                      "build-hdd-diag/PSX_DVR_HDD_DIAG_O_LAUNCH.ELF"):
            self.assertIn(value, WORKFLOW)

    def test_launcher_gated_to_its_target(self):
        for value in (
            '#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_O.ELF"',
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_O.ELF", 1',
            "if (!wait_for_choice(result == 0))",
            "if (enabled && (fresh & PAD_CROSS)) return 1;",
            "if (fresh & PAD_START) return 0;",
            "fresh = 0;",
            "target_elf > 0",
            "rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);",
        ):
            self.assertIn(value, LAUNCH)
        self.assertEqual(LAUNCH.count("rv = LoadELFFromFile("), 1)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile("))

    def test_target_has_distinct_quickboot_and_handlepfs_stages(self):
        for value in (
            '#define PFS_MOUNTPOINT "pfs0:"',
            'fopen(qb_cnf, "r")',
            "fgets(row, sizeof(row), fp)",
            "ret = quickboot_probe(argv[1]);",
            "q_first_umount = unmount_owned_at(32);",
            "ret = handlepfs_probe();",
            'before(33, "HANDLEPFS PATH DISPATCH")',
            'before(34, "HANDLEPFS CHECK PFS0 FREE")',
            'before(35, "HANDLEPFS REMOUNT RO")',
            'before(36, "HANDLEPFS POSIX TRYFILE")',
            "q_final_umount = unmount_owned_at(37);",
            'before(38, "HANDLEPFS BUILD LOADER PATH")',
            'unmount_owned_at(39);',
            'fd = open(norm, O_RDONLY);',
            "rv = close(fd);",
            '"%s:pfs:%s"',
            '"hdd0:PP.TESTDVR:pfs:/neutrino.elf"',
            "q_path_ok",
        ):
            self.assertIn(value, TARGET)
        expected_order = ["q_first_umount = unmount_owned_at(32);",
                          "ret = handlepfs_probe();"]
        self.assertLess(TARGET.index(expected_order[0]), TARGET.index(expected_order[1]))
        for val in ["q_remount = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION,",
                    'fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)']:
            self.assertIn(val, TARGET)
        self.assertEqual(TARGET.count("fileXioMount("), 2)
        self.assertEqual(TARGET.count("fileXioUmount("), 1)
        self.assertIn("if (!own_mount) return 0;", TARGET)
        self.assertIn("own_mount = 1;", TARGET)

    def test_no_game_chainload_or_disk_writes(self):
        self.assertNotIn("LoadELFFromFile(", TARGET)
        for src in (TARGET, LAUNCH):
            clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", src, flags=re.S)
            for token in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                          "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                          "fileXioDevctl(", "fileXioSync(", "FIO_O_WRONLY",
                          "FIO_O_RDWR", "FIO_O_CREAT", "FIO_O_TRUNC",
                          "FIO_MT_RDWR", "HDIOC_WRITESECTOR", "LoadExecPS2("):
                with self.subTest(token=token):
                    self.assertNotIn(token, clean)
            self.assertIn("FIO_MT_RDONLY", src)
        self.assertIn("finish_on_start();", TARGET)


if __name__ == "__main__":
    unittest.main()
