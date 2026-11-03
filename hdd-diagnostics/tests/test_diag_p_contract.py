"""DIAG P: isolate the second IOP reset in OSDMenu handlePFS, RO only."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src/osdmenu_second_reset_probe.c").read_text("utf-8")
LAUNCH = (ROOT / "src/osdmenu_second_reset_launcher.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (ROOT.parent / ".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagPContract(unittest.TestCase):
    def test_isolated_two_elf_build_and_artifact(self):
        for text in ("PSX_DVR_HDD_DIAG_P.ELF",
                     "PSX_DVR_HDD_DIAG_P_LAUNCH.ELF",
                     "src/osdmenu_second_reset_probe.c",
                     "src/osdmenu_second_reset_launcher.c",
                     "M_OSD_EMBEDDED", "G_DIAG_EMBEDDED"):
            self.assertIn(text, BUILD)
        for text in ("PSX-DVR-HDD-DIAG-P-SECOND-IOP-RESET-READONLY",
                     "build-hdd-diag/PSX_DVR_HDD_DIAG_P.ELF",
                     "build-hdd-diag/PSX_DVR_HDD_DIAG_P_LAUNCH.ELF"):
            self.assertIn(text, WORKFLOW)

    def test_launch_requires_preflight_and_fresh_x(self):
        self.assertIn('#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_P.ELF"', LAUNCH)
        self.assertIn('#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"', LAUNCH)
        self.assertIn('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_P.ELF", 1', LAUNCH)
        self.assertIn("if (!wait_for_choice(result == 0))", LAUNCH)
        self.assertIn("fresh = 0;", LAUNCH)
        self.assertIn("if (enabled && (fresh & PAD_CROSS)) return 1;", LAUNCH)
        self.assertIn("if (fresh & PAD_START) return 0;", LAUNCH)
        self.assertIn("target_elf > 0", LAUNCH)
        self.assertIn("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);", LAUNCH)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);"))

    def test_quickboot_unmount_before_second_reset(self):
        self.assertIn("ret = quickboot_probe(argv[1]);", SRC)
        self.assertIn("q_first_umount = unmount_owned_at(32);", SRC)
        self.assertIn("ret = handlepfs_probe();", SRC)
        self.assertLess(SRC.index("q_first_umount = unmount_owned_at(32);"),
                        SRC.index("ret = handlepfs_probe();"))
        self.assertIn('before(40, "SECOND IOP RESET");', SRC)
        self.assertIn('before(41, "SECOND IOP SYNC");', SRC)
        self.assertIn('before(42, "SECOND RPC PATCH");', SRC)
        self.assertIn("while (!SifIopReset(\"\", 0)) {}", SRC)
        self.assertEqual(SRC.count('while (!SifIopReset("", 0)) {}'), 2)
        self.assertIn("if (own_mount) return -300;", SRC)
        self.assertIn("padPortClose(0, 0);", SRC)
        self.assertIn("padEnd();", SRC)
        self.assertIn("have_pad = 0;", SRC)
        self.assertIn("q_second_reset_done = 1;", SRC)

    def test_second_module_stack_and_post_reset_pad(self):
        steps = ('SECOND_LOAD(iomanX, 43', 'SECOND_LOAD(fileXio, 44',
                 'SECOND_LOAD(sio2man, 45', 'SECOND_LOAD(mcman, 46',
                 'SECOND_LOAD(mcserv, 47', 'SECOND_LOAD(freepad, 48',
                 'before(49, "SECOND padInit")', 'before(50, "SECOND padPortOpen")',
                 'SECOND_LOAD(ps2dev9, 51', 'SECOND_LOAD(bdm, 52',
                 'SECOND_LOAD(bdmfs_fatfs, 53', 'SECOND_LOAD(ata_bd, 54',
                 'modload(55, "2ND ps2hdd-osd"', 'modload(56, "2ND ps2fs"',
                 'SECOND_LOAD(secrsif, 57', 'before(58, "SECOND fileXioInit")',
                 'before(59, "SECOND HDD DOPEN")',
                 'before(60, "SECOND PFS0 COLLISION")')
        offsets = [SRC.index(k) for k in steps]
        self.assertEqual(offsets, sorted(offsets))
        for k in ("q_second_modules_done = 1;", "q_second_pad_ready = 1;",
                  "fileXioInit();", "padInit(0)", "padPortOpen(0, 0, padbuf)"):
            self.assertIn(k, SRC)

    def test_repeated_ro_mount_target_and_prepared_loader_string(self):
        for x in ('before(61, "HANDLEPFS REMOUNT RO")',
                  "q_remount = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION,",
                  'before(62, "HANDLEPFS POSIX TRYFILE")',
                  "fd = open(norm, O_RDONLY);",
                  "q_final_umount = unmount_owned_at(63);",
                  'before(64, "HANDLEPFS BUILD LOADER PATH")',
                  '"hdd0:PP.TESTDVR:pfs:/neutrino.elf"',
                  "unmount_owned_at(65);",
                  "RESET2=%d MOD2=%d PAD2=%d",
                  "TRYFD=%d UM2=%d PATH=%d"):
            self.assertIn(x, SRC)
        self.assertEqual(SRC.count("fileXioMount("), 2)
        self.assertEqual(SRC.count("fileXioUmount("), 1)
        self.assertIn("FIO_MT_RDONLY", SRC)
        self.assertIn("if (!own_mount) return 0;", SRC)

    def test_no_app_level_writes_no_game_load(self):
        self.assertNotIn("LoadELFFromFile(", re.sub(r"/\*.*?\*/|//[^\n]*", "", SRC, flags=re.S))
        for code in (SRC, LAUNCH):
            clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", code, flags=re.S)
            for forbidden in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                              "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                              "fileXioDevctl(", "fileXioSync(", "FIO_O_WRONLY",
                              "FIO_O_RDWR", "FIO_O_CREAT", "FIO_O_TRUNC",
                              "FIO_MT_RDWR", "HDIOC_WRITESECTOR", "LoadExecPS2("):
                with self.subTest(forbidden=forbidden):
                    self.assertNotIn(forbidden, clean)


if __name__ == "__main__":
    unittest.main()
