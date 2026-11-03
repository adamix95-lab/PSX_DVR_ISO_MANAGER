"""DIAG X guardrail: observe DVR LE IOP transition, never open ISO or game."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
NP=ROOT/"neutrino-hdd-probe"
X=(NP/"patch_x_dvr_le.py").read_text("utf-8")
BASE=(NP/"patch_probe.py").read_text("utf-8")
W=(NP/"patch_w_entry_hold.py").read_text("utf-8")
CNF=(NP/"DIAG_X.CNF").read_bytes()
LAUNCH=(NP/"osdmenu_neutrino_x_launcher.c").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM=(ROOT/"CMakeLists.txt").read_text("utf-8")

class NeutrinoXContract(unittest.TestCase):
    def test_strict_cnf(self):
        self.assertEqual(CNF,
            b"boot=PSX_DVR_HDD_NEUTRINO_X_LE.ELF\n"
            b"arg=-bsd=dvr\n"
            b"arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso\n"
            b"arg=-dev9=NICHDD\n")
        for token in (
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_X.CNF"',
            'PFS_MOUNTPOINT "/DIAG_X.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_X_LE.ELF"',
            'q_config_ok == (int)sizeof(q_expected_config) - 1',
            "q_last_byte != '\\n' && q_last_byte != 0",
            "if (enabled && (fresh & PAD_CROSS)) return 1;",
            "FIO_MT_RDONLY",
        ):
            self.assertIn(token,LAUNCH)
    def test_staged_program_and_safe_stop(self):
        self.assertIn('strcmp(sys.sBSD, "dvr") != 0 || sys.bQuickBoot != 0',X)
        self.assertIn("X70 REJECT BSD / QUICKBOOT",X)
        for stage in ("X71 LE RESET PREP","X72 PSX OSDCNF RESET BEGIN",
                      "X73 OSDCNF RESET SYNC OK","X74 CDVD INIT + SCMD BEGIN",
                      "X75 DVR SCMD COMPLETED","X76 CLEAN IOP RESET BEGIN",
                      "X77 CLEAN RESET SYNC OK","X78 LE MODULE START",
                      "X79 LE MODULE idx=","X85 LE MODULES DONE / STOP"):
            self.assertIn(stage,X)
        self.assertIn('return 0; /* SAFE HARD STOP BEFORE FHI, ISO and GAME */',X)
        self.assertIn("s.index('fhi_config_init(&drv.mod)')",X)
        self.assertIn("s.index('setup_dvd_iso(sDVDFile')",X)
        self.assertIn("FIO_MT_RDONLY",BASE)
        self.assertIn("w_busy_hold(8)",W)
        self.assertNotIn("fileXioWrite(",X)
        self.assertNotIn("FIO_MT_RDWR",X)

    def test_ci_builds_same_source_stages(self):
        self.assertIn("patch_probe.py",WF)
        self.assertIn("patch_w_entry_hold.py",WF)
        self.assertIn("patch_x_dvr_le.py",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_X_LE.ELF",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_X_LAUNCH.ELF",WF)
        self.assertIn("PSX-DVR-HDD-NEUTRINO-X-LE-SAFE-STOP",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_X_LAUNCH.ELF",CM)

if __name__ == "__main__":
    unittest.main()
