"""Host tests for the user-approved clean v0.8 USB ZIP."""
import hashlib
import tempfile
import unittest
import zipfile
from pathlib import Path

from build_usb_bundle_v08 import (
    FOLDER, LEGACY_FOLDER, ZIP_ROOT, REQUIRED_LEGACY,
    EXCLUDED_V03_FILES, NEW_MANAGER, content_fingerprint,
    create_bundle, inspect_legacy, USER_UPLOADED_FILES,
)


class BundleTests(unittest.TestCase):
    def make_legacy(self, dest: Path, *, remove: str = "", extra=None):
        with zipfile.ZipFile(dest, "w") as z:
            z.writestr(LEGACY_FOLDER, b"")
            for p in sorted(REQUIRED_LEGACY | EXCLUDED_V03_FILES):
                if p == remove:
                    continue
                z.writestr(LEGACY_FOLDER + p, ("v0.3:" + p).encode())
            z.writestr(LEGACY_FOLDER + "config/other.toml", b"retain unchanged")
            z.writestr("DVR_ISO_MANAGER.ELF", b"\x7fELF" + b"OLD")
            if extra:
                for name, value in extra.items():
                    z.writestr(name, value)

    def fixture(self, root: Path):
        legacy = root / "legacy.zip"
        elf_path = root / "DVR_ISO_MANAGER.ELF"
        net = root / "NET_PROBE.CNF"
        pl = root / "PL.txt"
        en = root / "EN.txt"
        output = root / "complete.zip"
        self.make_legacy(legacy)
        elf = b"\x7fELF" + b"\x00" * 150_000
        elf_path.write_bytes(elf)
        net.write_text(
            "IP=192.168.0.240\nNETMASK=255.255.255.0\n"
            "GATEWAY=192.168.0.1\nSERVER_IP=192.168.0.122\nSERVER_PORT=8000\n"
        )
        pl.write_text("Polski")
        en.write_text("English")
        return legacy, elf_path, net, pl, en, output, elf

    def test_clean_zip_matches_uploaded_folder_layout(self):
        with tempfile.TemporaryDirectory() as root:
            legacy, elf_path, net, pl, en, output, elf = self.fixture(Path(root))
            members = create_bundle(legacy, elf_path, net, pl, en, output)
            with zipfile.ZipFile(output) as z:
                self.assertIsNone(z.testzip())
                self.assertIn(ZIP_ROOT, z.namelist())
                self.assertIn(FOLDER, z.namelist())
                self.assertIn(FOLDER + "ART/", z.namelist())
                self.assertIn(FOLDER + "config/", z.namelist())
                self.assertIn(FOLDER + "modules/", z.namelist())
                self.assertEqual(z.read(NEW_MANAGER), elf)
                self.assertEqual(z.read(FOLDER + "BOOT.ELF"), b"v0.3:BOOT.elf")
                self.assertNotIn(FOLDER + "BOOT.elf", z.namelist())
                self.assertNotIn("DVR_ISO_MANAGER.ELF", z.namelist())
                self.assertEqual(z.read(FOLDER + "config/other.toml"), b"retain unchanged")
                for old_test in EXCLUDED_V03_FILES:
                    self.assertNotIn(FOLDER + old_test, z.namelist())
                self.assertNotIn(FOLDER + "SHA256SUMS.txt", z.namelist())
                self.assertEqual(set(z.namelist()) -
                                 {x for x in z.namelist() if x.endswith("/")},
                                 set(members))
                self.assertEqual(content_fingerprint(members),
                                 content_fingerprint({n: z.read(n) for n in members}))

    def test_strict_user_fingerprint_rejects_other_package(self):
        with tempfile.TemporaryDirectory() as root:
            legacy, elf_path, net, pl, en, output, _ = self.fixture(Path(root))
            with self.assertRaisesRegex(ValueError, "DOES NOT match"):
                create_bundle(legacy, elf_path, net, pl, en, output,
                              verify_user_upload=True)

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
