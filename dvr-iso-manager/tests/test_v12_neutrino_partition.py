#!/usr/bin/env python3
"""Host-only checks for v1.2 Neutrino source retargeting (no PSX disk I/O)."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "neutrino-hdd-v1" / "patch_hdd_runtime_v12.py"

SOURCE = """
static int main(int argc, char *argv[]) {
    const char *system_target = "hdd0:PP.DVRISO:pfs:/neutrino_hdd.elf";
    const char *english_target = "hdd0:PP.DVRISO:pfs:/neutrino_hdd_lang.elf";
    int mount_rc = fileXioMount("pfs0:", "hdd0:PP.DVRISO", FIO_MT_RDONLY);
    if (strcmp(sDVDFile, argv[2] + 5) != 0) return -95;
    return mount_rc;
}
"""

class TestV12Neutrino(unittest.TestCase):
    def run_patch(self, source: str):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "main.c"
            p.write_text(source, encoding="utf-8")
            run = subprocess.run([sys.executable, str(SCRIPT), str(p)],
                                 text=True, capture_output=True)
            return run, p.read_text(encoding="utf-8")

    def test_retargets_only_the_three_known_paths(self):
        result, output = self.run_patch(SOURCE)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output.count("hdd0:PP.DVRISO12"), 3)
        self.assertIn('fileXioMount("pfs0:", "hdd0:PP.DVRISO12", FIO_MT_RDONLY)', output)
        self.assertNotIn("hdd0:PP.DVRISO:pfs:", output)
        self.assertIn("strcmp(sDVDFile, argv[2] + 5) != 0", output)

    def test_rejects_missing_or_extra_paths(self):
        for source in (SOURCE.replace('hdd0:PP.DVRISO:pfs:/neutrino_hdd.elf',
                                      'hdd0:PP.TESTDVR:pfs:/neutrino_hdd.elf'),
                       SOURCE + '"hdd0:PP.DVRISO";\n', ""):
            result, _ = self.run_patch(source)
            self.assertNotEqual(result.returncode, 0)

if __name__ == "__main__":
    unittest.main(verbosity=2)
