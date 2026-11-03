"""DIAG S: isolated read-only Neutrino prerequisites and ISO PVD probe."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src/neutrino_boot_prereq_probe.c").read_text("utf-8")
BUILD = (ROOT / "CMakeLists.txt").read_text("utf-8")
WORKFLOW = (ROOT.parent / ".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")


class DiagSContract(unittest.TestCase):
    def test_one_standalone_binary(self):
        self.assertIn("PSX_DVR_HDD_DIAG_S.ELF", BUILD)
        self.assertIn("src/neutrino_boot_prereq_probe.c", BUILD)
        self.assertIn("G_DIAG_EMBEDDED", BUILD)
        self.assertIn("PSX-DVR-HDD-DIAG-S-NEUTRINO-PREREQ-READONLY", WORKFLOW)
        self.assertIn("build-hdd-diag/PSX_DVR_HDD_DIAG_S.ELF", WORKFLOW)

    def test_neutrino_prerequisites_follow_current_upstream_as_candidates(self):
        for token in (
            'fileXioOpen("rom0:ROMVER", FIO_O_RDONLY, 0)',
            'fileXioRead(fd, probe_buf, 15)',
            'PFS_MOUNTPOINT "/neutrino.elf"',
            'probe_buf[18] == 8 && probe_buf[19] == 0',
            'PFS_MOUNTPOINT "/config/system.toml"',
            'PFS_MOUNTPOINT "/system.toml"',
            'PFS_MOUNTPOINT "/config/bsd-dvr.toml"',
            'PFS_MOUNTPOINT "/bsd-dvr.toml"',
            'PFS_MOUNTPOINT "/config/compat.toml"',
            'PFS_MOUNTPOINT "/compat.toml"',
            'PFS_MOUNTPOINT "/config/emu-dvd-file.toml"',
            'PFS_MOUNTPOINT "/emu-dvd-file.toml"',
            'PFS_MOUNTPOINT "/modules"',
            "LOCAL CONFIG CANDIDATES ONLY",
        ):
            self.assertIn(token, SOURCE)

    def test_real_selected_iso_no_dummy_or_write(self):
        for token in (
            'PFS_MOUNTPOINT "/BOOT.CNF"',
            'if (!has_dvd) return -230;',
            'fd = fileXioOpen(dvd_path, FIO_O_RDONLY, 0);',
            'fileXioLseek(fd, 16 * 2048, SEEK_SET);',
            'fileXioRead(fd, probe_buf, 6)',
            'memcmp(probe_buf + 1, "CD001", 5)',
            'iso_file_open = fd',
            "ISO FD=%d PVD=%d",
            "ISO SEEK=%d READ=%d",
        ):
            self.assertIn(token, SOURCE)
        self.assertNotIn("DIAG_R_NO_GAME.iso", SOURCE)

    def test_safe_mount_cleanup_and_no_exec(self):
        self.assertEqual(SOURCE.count("fileXioMount("), 1)
        self.assertEqual(SOURCE.count("fileXioUmount("), 1)
        self.assertIn("fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY)", SOURCE)
        self.assertIn("if (!have_our_mount) return;", SOURCE)
        self.assertIn("wait_for_start();", SOURCE)
        self.assertIn("owned_unmount();", SOURCE)
        self.assertIn("if (fresh & PAD_START) break;", SOURCE)
        self.assertIn("NO TARGET EXECUTION. PHOTO.", SOURCE)
        stripped = re.sub(r"/\*.*?\*/|//[^\n]*", "", SOURCE, flags=re.S)
        for forbidden in (
            "LoadELFFromFile(", "LoadExecPS2(", "ExecPS2(", "SifLoadElf(",
            "fileXioWrite(", "fileXioFormat(", "fileXioRemove(",
            "fileXioRename(", "fileXioMkdir(", "fileXioRmdir(",
            "fileXioDevctl(", "fileXioSync(", "FIO_MT_RDWR",
            "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_CREAT", "FIO_O_TRUNC",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, stripped)


if __name__ == "__main__":
    unittest.main()
