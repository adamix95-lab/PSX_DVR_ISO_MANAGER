"""DIAG I: check read-only OSDMenu BOOT.CNF parsing and non-launching guarantees."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CODE = (ROOT / "src" / "apa_dvr_boot_path_diag.c").read_text(encoding="utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
FLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text(encoding="utf-8")


class DiagIContract(unittest.TestCase):
    def test_isolated_build_and_action(self):
        self.assertIn("PSX_DVR_HDD_DIAG_I.ELF", BUILD)
        self.assertIn("src/apa_dvr_boot_path_diag.c", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-I-BOOT-PATH-READONLY", FLOW)
        self.assertIn("pad patches fileXio cdvd libdebug.a", BUILD)

    def test_canonical_hdd_launch_path_is_displayed_not_executed(self):
        self.assertIn("hdd0:PP.TESTDVR/BOOT.ELF", CODE)
        self.assertIn("NOT LAUNCHED. RO ONLY.", CODE)
        self.assertNotIn("LoadELFFromFile", CODE)
        self.assertNotIn("LoadExecPS2", CODE)
        self.assertNotIn("ExecPS2(", CODE)

    def test_cnf_inspection_expected_flags(self):
        for s in ("boot=neutrino.elf", "boot=neutrino_lang.elf", "arg=-bsd=dvr",
                  "arg=-dvd=", "dvr_pfs0:/ISO/", "mass0:", "usb0:"):
            self.assertIn(s, CODE)
        self.assertIn('fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0)', CODE)
        self.assertIn("fileXioLseek(fd, 0, SEEK_END)", CODE)
        self.assertIn("fileXioRead(fd, cnf_text, n)", CODE)
        self.assertIn("fileXioClose(fd)", CODE)
        self.assertIn("dvd_open_code = probe_selected_iso()", CODE)
        self.assertIn("fileXioOpen(dvd_path, FIO_O_RDONLY, 0)", CODE)
        self.assertIn("config_type == 1 && bsd_dvr && has_dvd", CODE)
        self.assertIn("!usb_dep && dvd_open_code >= 0", CODE)

    def test_ro_and_cleanup(self):
        self.assertEqual(CODE.count("fileXioMount("), 1)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", CODE)
        self.assertEqual(CODE.count("fileXioUmount("), 1)
        self.assertIn("if (!have_our_mount) return;", CODE)
        self.assertIn("have_our_mount = 1;", CODE)
        self.assertLess(CODE.index("owned_unmount();\n    scr_clear();"),
                        CODE.index("wait_for_start();"))
        self.assertIn("padPortClose(0, 0);", CODE)
        self.assertIn("padEnd();", CODE)

    def test_never_write_to_hdd_or_dvr(self):
        clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", CODE, flags=re.S)
        for forbidden in ("fileXioWrite", "fileXioFormat", "fileXioRemove",
                          "fileXioRename", "fileXioMkdir", "fileXioRmdir",
                          "fileXioChstat", "fileXioDevctl", "FIO_O_WRONLY",
                          "FIO_O_RDWR", "FIO_O_CREAT", "FIO_O_TRUNC",
                          "FIO_MT_RDWR", "HDIOC_WRITESECTOR"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, clean)


if __name__ == "__main__":
    unittest.main()
