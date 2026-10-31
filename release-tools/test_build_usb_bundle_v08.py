"""Host tests for the complete USB release zip assembler."""
import hashlib
import io
import tempfile
import unittest
import zipfile
from pathlib import Path

from build_usb_bundle_v08 import FOLDER, REQUIRED_LEGACY, create_bundle, inspect_legacy


class BundleTests(unittest.TestCase):
    def make_legacy(self, dest: Path, *, remove: str = "", extra=None):
        with zipfile.ZipFile(dest, "w") as z:
            z.writestr(FOLDER, b"")
            for p in sorted(REQUIRED_LEGACY):
                if p == remove:
                    continue
                z.writestr(FOLDER + p, ("v0.3:" + p).encode())
            z.writestr(FOLDER + "config/other.toml", b"retain unchanged")
            z.writestr("DVR_ISO_MANAGER.ELF", b"\x7fELF" + b"OLD")
            if extra:
                for name, value in extra.items():
                    z.writestr(name, value)

    def test_complete_zip_preserves_legacy_and_replaces_manager(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root)
            legacy = path / "legacy.zip"
            elf_path = path / "DVR_ISO_MANAGER.ELF"
            net = path / "NET_PROBE.CNF"
            pl = path / "PL.txt"
            en = path / "EN.txt"
            output = path / "complete.zip"
            self.make_legacy(legacy)
            elf = b"\x7fELF" + b"\x00" * 150_000
            elf_path.write_bytes(elf)
            net.write_text("IP=192.168.0.240\nNETMASK=255.255.255.0\nGATEWAY=192.168.0.1\nSERVER_IP=192.168.0.122\nSERVER_PORT=8000\n")
            pl.write_text("Polski")
            en.write_text("English")
            create_bundle(legacy, elf_path, net, pl, en, output)
            with zipfile.ZipFile(output) as z:
                self.assertIsNone(z.testzip())
                self.assertEqual(z.read("DESR_DVR/DVR_ISO_MANAGER.ELF"), elf)
                self.assertEqual(z.read("DESR_DVR/BOOT.ELF"), b"v0.3:BOOT.elf")
                self.assertNotIn("DESR_DVR/BOOT.elf", z.namelist())
                self.assertNotIn("DVR_ISO_MANAGER.ELF", z.namelist())
                self.assertIn("DESR_DVR/ART/", z.namelist())
                self.assertEqual(z.read("DESR_DVR/config/other.toml"), b"retain unchanged")
                manifest = z.read("DESR_DVR/SHA256SUMS.txt").decode()
                self.assertIn(hashlib.sha256(elf).hexdigest(), manifest)
                self.assertIn("DESR_DVR/NET_PROBE.CNF", manifest)
                self.assertIn("DESR_DVR/modules/dvrdrv.irx", manifest)

    def test_fail_when_original_module_missing(self):
        with tempfile.TemporaryDirectory() as path:
            legacy = Path(path) / "legacy.zip"
            self.make_legacy(legacy, remove="modules/dvrfile_fhi.irx")
            with zipfile.ZipFile(legacy) as z:
                with self.assertRaisesRegex(ValueError, "Missing original v0.3"):
                    inspect_legacy(z)

    def test_fail_on_zip_traversal(self):
        with tempfile.TemporaryDirectory() as path:
            legacy = Path(path) / "legacy.zip"
            self.make_legacy(legacy, extra={"DESR_DVR/../../bad": b"escape"})
            with zipfile.ZipFile(legacy) as z:
                with self.assertRaisesRegex(ValueError, "Unsafe archive member"):
                    inspect_legacy(z)

    def test_fail_on_case_collision(self):
        with tempfile.TemporaryDirectory() as path:
            legacy = Path(path) / "legacy.zip"
            self.make_legacy(legacy, extra={"DESR_DVR/boot.ELF": b"duplicate"})
            with zipfile.ZipFile(legacy) as z:
                with self.assertRaisesRegex(ValueError, "collision"):
                    inspect_legacy(z)


if __name__ == "__main__":
    unittest.main()
