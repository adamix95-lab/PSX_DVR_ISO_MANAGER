"""DIAG M: standalone OSDMenu APA module-order and BOOT.CNF argv probe."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
TARGET = (ROOT / "src" / "osdmenu_module_probe.c").read_text("utf-8")
LAUNCH = (ROOT / "src" / "osdmenu_module_probe_launcher.c").read_text("utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagMContract(unittest.TestCase):
    def test_separate_target_and_launcher_uploaded_together(self):
        for text in ("PSX_DVR_HDD_DIAG_M.ELF", "PSX_DVR_HDD_DIAG_M_LAUNCH.ELF",
                     "src/osdmenu_module_probe.c", "src/osdmenu_module_probe_launcher.c",
                     "ps2hdd-osd.irx", "ata_bd", "secrsif"):
            self.assertIn(text, CMAKE)
        for text in ("PSX-DVR-HDD-DIAG-M-OSDMENU-MODULE-PROBE",
                     "build-hdd-diag/PSX_DVR_HDD_DIAG_M.ELF",
                     "build-hdd-diag/PSX_DVR_HDD_DIAG_M_LAUNCH.ELF"):
            self.assertIn(text, WORKFLOW)

    def test_specific_osdmenu_order(self):
        steps = ['LOAD(iomanX, 4', 'LOAD(fileXio, 5',
                 'LOAD(sio2man, 6', 'LOAD(mcman, 7',
                 'LOAD(mcserv, 8', 'LOAD(freepad, 9',
                 'LOAD(ps2dev9, 12', 'LOAD(bdm, 13',
                 'LOAD(bdmfs_fatfs, 14', 'LOAD(ata_bd, 15',
                 'modload(16, "ps2hdd-osd"', 'modload(17, "ps2fs"',
                 'LOAD(secrsif, 18', 'fileXioInit()']
        offsets = [TARGET.index(text) for text in steps]
        self.assertEqual(offsets, sorted(offsets))
        self.assertIn("sleep(1);", TARGET)
        self.assertIn('#define PFS_MOUNTPOINT "pfs0:"', TARGET)
        self.assertIn('static const char hdd_arguments[] = "-o\\0" "4\\0" "-n\\0" "20";', TARGET)
        self.assertIn('static const char pfs_arguments[] = "-o\\0" "10\\0" "-n\\0" "40";', TARGET)
        self.assertIn("if (status == 1) return -201;", TARGET)

    def test_launcher_checks_target_and_passes_explicit_cnf_argv(self):
        self.assertIn('#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_M.ELF"', LAUNCH)
        self.assertIn('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"', LAUNCH)
        self.assertIn('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_M.ELF", 1', LAUNCH)
        self.assertIn('char *launcher_args[1] = {(char *)HDD_CNF_ARG};', LAUNCH)
        self.assertIn("target_elf > 0", LAUNCH)
        self.assertIn("if (!wait_for_choice(result == 0))", LAUNCH)
        self.assertIn("if (enabled && (fresh & PAD_CROSS)) return 1;", LAUNCH)
        self.assertIn("if (fresh & PAD_START) return 0;", LAUNCH)
        self.assertIn("fresh = 0;", LAUNCH)
        self.assertIn("LoadELFFromFile(LOADED_ELF, 1, launcher_args)", LAUNCH)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile("))
        self.assertIn("owned_unmount();\n        padPortClose(0, 0);\n        padEnd();", LAUNCH)

    def test_target_checks_passed_path_and_reads_test_partition_only(self):
        self.assertIn('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"', TARGET)
        self.assertIn('strcmp(argv[1], HDD_CNF_ARG) == 0', TARGET)
        self.assertIn("M0 ENTRY FROM HDD", TARGET)
        self.assertIn("CNF ARG MATCH=%d", TARGET)
        self.assertIn("arg_match ? 0 : -250", TARGET)
        self.assertIn("fileXioOpen(PFS_MOUNTPOINT \"/BOOT.CNF\", FIO_O_RDONLY, 0)", TARGET)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", TARGET)
        self.assertIn("fileXioDopen(PFS_MOUNTPOINT \"/\")", TARGET)
        self.assertEqual(TARGET.count("fileXioMount("), 1)
        self.assertEqual(TARGET.count("fileXioUmount("), 1)
        self.assertIn("if (!own_mount) return 0;", TARGET)
        self.assertIn("own_mount = 1;", TARGET)
        self.assertIn("M22 MOUNT=%d", TARGET)
        self.assertIn("PSX DVR HDD DIAG M / RESULTS", TARGET)
        self.assertIn("finish_on_start();", TARGET)

    def test_no_game_launch_or_application_level_storage_mutations(self):
        self.assertNotIn("LoadELFFromFile(", TARGET)
        self.assertNotIn("SifExecModuleBuffer(dvrdrv_irx", TARGET)
        for src in (TARGET, LAUNCH):
            stripped = re.sub(r"/\*.*?\*/|//[^\n]*", "", src, flags=re.S)
            for forbidden in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                              "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                              "fileXioDevctl(", "FIO_O_WRONLY", "FIO_O_RDWR",
                              "FIO_O_CREAT", "FIO_O_TRUNC", "FIO_MT_RDWR",
                              "HDIOC_WRITESECTOR", "LoadExecPS2("):
                with self.subTest(forbidden=forbidden):
                    self.assertNotIn(forbidden, stripped)


if __name__ == "__main__":
    unittest.main()
