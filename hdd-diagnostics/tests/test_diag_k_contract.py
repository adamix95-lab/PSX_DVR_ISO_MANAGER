"""DIAG K: proof of raw HDD -> known-good DIAG F chainload, not OSDMenu."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "src" / "apa_dvr_raw_elf_diag.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
FLOW = (ROOT.parent / ".github" / "workflows" / "build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagKContract(unittest.TestCase):
    def test_independent_build_target_and_upload_includes_target_f(self):
        self.assertIn("PSX_DVR_HDD_DIAG_K.ELF", BUILD)
        self.assertIn("src/apa_dvr_raw_elf_diag.c", BUILD)
        self.assertIn("pad patches fileXio cdvd elf-loader libdebug.a", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-K-RAW-ELF-TEST", FLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_F.ELF", FLOW)

    def test_known_good_target_and_readonly_probe(self):
        self.assertIn('#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_F.ELF"', SRC)
        self.assertIn('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_F.ELF", 1', SRC)
        self.assertIn("target_elf > 0", SRC)
        self.assertIn("TARGET F BYTES=%d", SRC)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", SRC)
        self.assertEqual(SRC.count("fileXioMount("), 1)
        self.assertEqual(SRC.count("fileXioUmount("), 1)

    def test_x_gate_and_start_cancel(self):
        self.assertIn("if (!wait_for_choice(result == 0))", SRC)
        self.assertIn("if (enabled && (fresh & PAD_CROSS)) return 1;", SRC)
        self.assertIn("if (fresh & PAD_START) return 0;", SRC)
        self.assertIn("fresh = 0;", SRC)
        self.assertIn("X: RUN HDD DIAG F ONLY", SRC)
        self.assertIn("PRECHECK FAILED; X DISABLED", SRC)
        after_gate = SRC[SRC.index("if (!wait_for_choice(result == 0))"):]
        self.assertRegex(after_gate,
                         r"owned_unmount\(\);\s*padPortClose\(0, 0\);\s*padEnd\(\);\s*return 0;")
        self.assertLess(SRC.index("if (!wait_for_choice(result == 0))"),
                        SRC.index("rv = LoadELFFromFile(LOADED_ELF, 0, NULL);"))

    def test_no_launcher_or_neutrino_exec(self):
        self.assertEqual(SRC.count("rv = LoadELFFromFile(LOADED_ELF, 0, NULL);"), 1)
        self.assertNotIn("LoadELFFromFile(LOADED_ELF, 1,", SRC)
        self.assertNotIn("launcher_args", SRC)
        self.assertNotIn("HDD_CNF_ARG", SRC)
        self.assertIn("RUNS ONLY DIAG F, NOT A GAME.", SRC)
        self.assertIn("K CHAINLOAD RETURNED=%d", SRC)

    def test_no_disk_writes(self):
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
