"""DIAG L post-HDD-chainload IOP/APA/DVR reinit tests and no-write guarantees."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
TARGET = (ROOT / "src" / "apa_dvr_postboot_diag.c").read_text("utf-8")
LAUNCH = (ROOT / "src" / "apa_dvr_postboot_launcher.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
FLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")

class DiagLContract(unittest.TestCase):
    def test_two_separate_elf_targets(self):
        self.assertIn("PSX_DVR_HDD_DIAG_L.ELF", BUILD)
        self.assertIn("PSX_DVR_HDD_DIAG_L_LAUNCH.ELF", BUILD)
        self.assertIn("src/apa_dvr_postboot_diag.c", BUILD)
        self.assertIn("src/apa_dvr_postboot_launcher.c", BUILD)
        self.assertIn("pad patches fileXio cdvd elf-loader libdebug.a", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-L-POSTBOOT-IOP", FLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_L.ELF", FLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_L_LAUNCH.ELF", FLOW)

    def test_launcher_has_explicit_x_gate_and_target(self):
        self.assertIn('#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_L.ELF"', LAUNCH)
        self.assertIn('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_L.ELF", 1', LAUNCH)
        self.assertIn("target_elf > 0", LAUNCH)
        self.assertIn("if (!wait_for_choice(result == 0))", LAUNCH)
        self.assertIn("if (fresh & PAD_START) return 0;", LAUNCH)
        self.assertIn("if (enabled && (fresh & PAD_CROSS)) return 1;", LAUNCH)
        self.assertIn("fresh = 0;", LAUNCH)
        self.assertIn("PRECHECK FAILED; X DISABLED", LAUNCH)
        self.assertIn("X: RUN HDD DIAG L ONLY", LAUNCH)
        self.assertIn("rv = LoadELFFromFile(LOADED_ELF, 0, NULL);", LAUNCH)
        self.assertEqual(LAUNCH.count("rv = LoadELFFromFile("), 1)
        self.assertLess(LAUNCH.index("if (!wait_for_choice(result == 0))"),
                        LAUNCH.index("rv = LoadELFFromFile("))
        self.assertIn("owned_unmount();\n        padPortClose(0, 0);\n        padEnd();\n        return 0;", LAUNCH)

    def test_target_entry_before_iop_reset(self):
        self.assertIn("int main(int argc, char *argv[])", TARGET)
        self.assertIn("L0 ENTRY: ELF EXECUTING", TARGET)
        self.assertIn("ARGC=%d ARGV0=%.64s", TARGET)
        self.assertLess(TARGET.index("L0 ENTRY: ELF EXECUTING"), TARGET.index('before("L1 IOP RESET")'))
        self.assertLess(TARGET.index('before("L1 IOP RESET")'), TARGET.index("SifIopReset("))
        self.assertIn("PSX DVR HDD DIAG L / POST-BOOT", TARGET)
        self.assertIn("fileXioOpen(PFS_MOUNTPOINT \"/BOOT.CNF\", FIO_O_RDONLY, 0)", TARGET)
        self.assertIn("fileXioOpen(dvd_path, FIO_O_RDONLY, 0)", TARGET)
        self.assertIn("config_type == 1 && bsd_dvr && has_dvd", TARGET)
        self.assertIn("!usb_dep && dvd_open_code >= 0", TARGET)
        self.assertIn("dvr_pfs0:/ISO", TARGET)
        self.assertIn("wait_for_start();", TARGET)

    def test_ro_mount_only_and_own_cleanup(self):
        for src in (TARGET, LAUNCH):
            self.assertEqual(src.count("fileXioMount("), 1)
            self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", src)
            self.assertEqual(src.count("fileXioUmount("), 1)
            self.assertIn("if (!have_our_mount) return;", src)
            self.assertIn("have_our_mount = 1;", src)
            self.assertIn("owned_unmount();", src)

    def test_no_disk_writes_no_osdmenu_or_neutrino_chainload(self):
        for src in (TARGET, LAUNCH):
            code = re.sub(r"/\*.*?\*/|//[^\n]*", "", src, flags=re.S)
            for forbidden in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                              "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                              "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_CREAT",
                              "FIO_O_TRUNC", "FIO_MT_RDWR", "HDIOC_WRITESECTOR",
                              "LoadExecPS2("):
                with self.subTest(forbidden=forbidden):
                    self.assertNotIn(forbidden, code)
            self.assertNotIn('LoadELFFromFile("pfs1:/BOOT.ELF"', code)
        self.assertNotIn("LoadELFFromFile(", TARGET)


if __name__ == "__main__":
    unittest.main()
