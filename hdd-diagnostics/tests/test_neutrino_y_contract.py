"""Neutrino Y contract: real DVR FHI backend and CDVD without ISO/game."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
NP=ROOT/"neutrino-hdd-probe"
PATCH=(NP/"patch_y_fhi.py").read_text("utf-8")
X=(NP/"patch_x_dvr_le.py").read_text("utf-8")
CNF=(NP/"DIAG_Y.CNF").read_bytes()
LAUNCH=(NP/"osdmenu_neutrino_y_launcher.c").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM=(ROOT/"CMakeLists.txt").read_text("utf-8")

class NeutrinoYContract(unittest.TestCase):
    def test_allowlist_cnf_and_strict_launcher(self):
        self.assertEqual(CNF,
            b"boot=PSX_DVR_HDD_NEUTRINO_Y_FHI.ELF\n"
            b"arg=-bsd=dvr\n"
            b"arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso\n"
            b"arg=-dev9=NICHDD\n")
        for token in (
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_Y.CNF"',
            'PFS_MOUNTPOINT "/DIAG_Y.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_Y_FHI.ELF"',
            "q_config_ok == (int)sizeof(q_expected_config) - 1",
            "q_last_byte != '\\n' && q_last_byte != 0",
            "if (enabled && (fresh & PAD_CROSS)) return 1;",
            "FIO_MT_RDONLY",
        ):
            self.assertIn(token,LAUNCH)

    def test_fork_backend_true_check_and_iso_guard(self):
        for marker in ("Y85 LE MODULES READY", "Y86 FHI CONFIG INIT",
                       "Y87 FHI ACTIVE", "Y87 DVR FHI MOD",
                       "Y88 ERROR DVR FHI MISSING",
                       "Y89 ERROR OTHER FHI HAS PRIORITY",
                       "Y90 CDVDMAN SETTINGS",
                       "Y95 DVR FHI+CDVD READY / SAFE STOP"):
            self.assertIn(marker,PATCH)
        for fragment in ("fhi_config_init(&drv.mod)",
                         'modlist_get_by_name(&drv.mod, "dvrfile_fhi.irx")',
                         'modlist_get_by_name(&drv.mod, "cdvdman_emu.irx")',
                         'strcmp(sDVDFile, "dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso")',
                         "return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */",
                         's.index("if (setup_dvd_iso(sDVDFile")'):
            self.assertIn(fragment,PATCH)
        self.assertIn("DISABLE_EXTRA_TIMERS_FUNCTIONS()",X)
        self.assertNotIn("fileXioWrite(",PATCH)
        self.assertNotIn("FIO_MT_RDWR",PATCH)

    def test_ci_rebuild_and_isolation(self):
        for token in ("patch_probe.py", "patch_w_entry_hold.py",
                      "patch_x_dvr_le.py", "patch_y_fhi.py",
                      "PSX_DVR_HDD_NEUTRINO_Y_FHI.ELF",
                      "PSX_DVR_HDD_NEUTRINO_Y_LAUNCH.ELF",
                      "PSX-DVR-HDD-NEUTRINO-Y-FHI-NO-ISO"):
            self.assertIn(token,WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_Y_LAUNCH.ELF",CM)
        self.assertIn("DIAG_Y.CNF",WF)

if __name__ == "__main__":
    unittest.main()
