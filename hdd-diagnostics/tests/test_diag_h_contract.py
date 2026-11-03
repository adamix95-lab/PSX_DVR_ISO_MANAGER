"""DIAG H: static safety and staged-file read-only validation."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CODE = (ROOT / "src" / "apa_dvr_staged_files_diag.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
FLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagHContract(unittest.TestCase):
    def test_separate_h_elf_and_artifact(self):
        self.assertIn("PSX_DVR_HDD_DIAG_H.ELF", BUILD)
        self.assertIn("src/apa_dvr_staged_files_diag.c", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-H-STAGED-FILES-READONLY", FLOW)
        self.assertIn("pad patches fileXio cdvd libdebug.a", BUILD)

    def test_three_staged_paths_and_readonly_content(self):
        for path in ('PFS_MOUNTPOINT "/BOOT.CNF"',
                     'PFS_MOUNTPOINT "/BOOT.ELF"',
                     'PFS_MOUNTPOINT "/neutrino.elf"'):
            self.assertIn(path, CODE)
        self.assertIn("static int inspect_file(", CODE)
        self.assertIn("fileXioOpen(path, FIO_O_RDONLY, 0)", CODE)
        self.assertIn("fileXioLseek(fd, 0, SEEK_END)", CODE)
        self.assertIn("fileXioRead(fd, header, 16)", CODE)
        self.assertIn("fileXioClose(fd)", CODE)
        self.assertIn("header[0] == 0x7f", CODE)
        self.assertIn("require_elf", CODE)
        self.assertIn("boot_cnf > 0 && boot_elf > 0 && neutrino_elf > 0", CODE)

    def test_mount_cleanup_and_pad(self):
        self.assertEqual(CODE.count("fileXioMount("), 1)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", CODE)
        self.assertEqual(CODE.count("fileXioUmount("), 1)
        self.assertIn("if (!have_our_mount) return;", CODE)
        self.assertIn("have_our_mount = 1;", CODE)
        self.assertLess(CODE.index("owned_unmount();\n    scr_clear();"), CODE.index("wait_for_start();"))
        self.assertIn("if (fresh & PAD_START) break;", CODE)
        self.assertIn("padPortClose(0, 0);", CODE)
        self.assertIn("padEnd();", CODE)
        self.assertIn("return 0;", CODE)

    def test_no_storage_writes_or_chainloading(self):
        no_comments = re.sub(r"/\*.*?\*/|//[^\n]*", "", CODE, flags=re.S)
        for forbidden in (
            "fileXioWrite", "fileXioFormat", "fileXioRemove", "fileXioRename",
            "fileXioMkdir", "fileXioRmdir", "fileXioChstat", "fileXioDevctl",
            "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_CREAT", "FIO_O_TRUNC",
            "FIO_MT_RDWR", "HDIOC_WRITESECTOR", "LoadELFFromFile", "mass0:",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, no_comments)


if __name__ == "__main__":
    unittest.main()
