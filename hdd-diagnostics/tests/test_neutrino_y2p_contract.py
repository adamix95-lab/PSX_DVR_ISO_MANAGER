"""DIAG Y2P — strict CNF prefix with ignored pad line for last-byte anomaly."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
NP=ROOT/"neutrino-hdd-probe"
OLD=(NP/"DIAG_Y2.CNF").read_bytes()
PAD=(NP/"DIAG_Y2P.CNF").read_bytes()
LAUNCH=(NP/"osdmenu_neutrino_y2p_launcher.c").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM=(ROOT/"CMakeLists.txt").read_text("utf-8")
Y2PATCH=(NP/"patch_y2_fhi_preinit.py").read_text("utf-8")

class Y2PContract(unittest.TestCase):
    def test_all_command_lines_identical_and_protected(self):
        self.assertEqual(PAD,OLD+b"#PAD\n")
        self.assertEqual(len(OLD),109)
        self.assertEqual(len(PAD),114)
        self.assertTrue(PAD.endswith(b"arg=-dev9=NICHDD\n#PAD\n"))
        self.assertEqual(PAD.count(b"boot="),1)
        self.assertEqual(PAD.count(b"arg="),3)
        self.assertNotIn(b"=",PAD[-5:])
        self.assertIn(b"DIAG_V_NO_GAME.iso",PAD)

    def test_last_byte_tolerance_is_only_after_exact_pad_prefix(self):
        for phrase in (
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_Y2P.CNF"',
            'PFS_MOUNTPOINT "/DIAG_Y2P.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF"',
            '"arg=-dev9=NICHDD\\n"',
            '"#PAD\\n"',
            "memcmp(q_expected_config, q_config_bytes, (size_t)(length - 1))",
            "(q_last_byte != '\\n' && q_last_byte != 0 && q_last_byte != 1)",
            "q_config_ok == (int)sizeof(q_expected_config) - 1",
            "FIO_MT_RDONLY",
            "if (enabled && (fresh & PAD_CROSS)) return 1;",
        ):
            self.assertIn(phrase,LAUNCH)

    def test_same_neutrino_y2_without_rebuild_or_iso(self):
        self.assertIn("PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF",WF)
        self.assertIn("PSX-DVR-HDD-NEUTRINO-Y2P-PADDED-CNF",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_Y2P_LAUNCH.ELF",CM)
        self.assertIn("return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */",Y2PATCH)
        for forbidden in ("FIO_MT_RDWR", "fileXioWrite(", "fileXioRemove(", "fileXioMkdir("):
            self.assertNotIn(forbidden,LAUNCH)
        self.assertNotIn("ExecPS2(",LAUNCH)

if __name__=="__main__":
    unittest.main()
