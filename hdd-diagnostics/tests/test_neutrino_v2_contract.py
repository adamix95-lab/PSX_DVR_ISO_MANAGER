"""HDD Neutrino V2: exact same source as V, only ps2-packer removed."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "neutrino-hdd-probe"
V = (BASE / "DIAG_V.CNF").read_bytes()
V2 = (BASE / "DIAG_V2.CNF").read_bytes()
L = (BASE / "osdmenu_neutrino_v2_launcher.c").read_text("utf-8")
P = (BASE / "patch_probe.py").read_text("utf-8")
WF = (ROOT.parent / ".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM = (ROOT / "CMakeLists.txt").read_text("utf-8")

class NeutrinoV2Contract(unittest.TestCase):
    def test_only_boot_filename_differs_in_cnf(self):
        self.assertEqual(
            V2, V.replace(b"boot=PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF\n",
                          b"boot=PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF\n"))
        self.assertIn(b"arg=-dev9=NICHDD\n",V2)
        self.assertIn(b"DIAG_V_NO_GAME.iso",V2)
        self.assertNotIn(b"DIAG_R_NO_GAME.iso",V2)

    def test_strict_gated_launcher(self):
        for s in (
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_V2.CNF"',
            'PFS_MOUNTPOINT "/DIAG_V2.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF"',
            '"boot=PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF\\n"',
            'q_last_byte != \'\\n\' && q_last_byte != 0',
            'q_config_ok == (int)sizeof(q_expected_config) - 1',
            'if (enabled && (fresh & PAD_CROSS)) return 1;',
            'FIO_MT_RDONLY',
            "CHECK=%d MOUNT=%d",
        ):
            self.assertIn(s, L)

    def test_same_early_stop_neutrino_source_no_iso(self):
        self.assertIn('return 0; /* ABSOLUTE no-game stop:',P)
        self.assertIn('strcmp(argv[2], "-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso")',P)
        self.assertIn('fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)',P)
        self.assertIn('chdir("pfs0:/")',P)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF",WF)
        self.assertIn("ee/loader/neutrino_unpacked.elf",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_V2_LAUNCH.ELF",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_V2_LAUNCH.ELF",CM)
        self.assertIn("e_packed != e_plain",WF)
        self.assertIn("PSX-DVR-HDD-NEUTRINO-V2-UNPACKED-SAFE-PROBE",WF)
        self.assertIn("DIAG_V2.CNF",WF)

if __name__ == "__main__":
    unittest.main()
