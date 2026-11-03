"""DIAG Y2 contract: detect chosen FHI before fhi_config_init wipes magic."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
NP=ROOT/"neutrino-hdd-probe"
PATCH=(NP/"patch_y2_fhi_preinit.py").read_text("utf-8")
CNF=(NP/"DIAG_Y2.CNF").read_bytes()
LAUNCH=(NP/"osdmenu_neutrino_y2_launcher.c").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM=(ROOT/"CMakeLists.txt").read_text("utf-8")

class Y2Contract(unittest.TestCase):
    def test_exact_dummy_config(self):
        self.assertEqual(CNF,
            b"boot=PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF\n"
            b"arg=-bsd=dvr\n"
            b"arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso\n"
            b"arg=-dev9=NICHDD\n")
        for x in ('"hdd0:PP.TESTDVR/DIAG_Y2.CNF"',
                  'PFS_MOUNTPOINT "/DIAG_Y2.CNF"',
                  'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF"',
                  "q_config_ok == (int)sizeof(q_expected_config) - 1",
                  "q_last_byte != '\\n' && q_last_byte != 0",
                  "if (enabled && (fresh & PAD_CROSS)) return 1;",
                  "FIO_MT_RDONLY"):
            self.assertIn(x,LAUNCH)
    def test_capture_settings_before_upstream_memset(self):
        self.assertIn("MODULE_SETTINGS_MAGIC",PATCH)
        self.assertIn("y2_pre_settings[4]",PATCH)
        for name in ("fhi_bd.irx", "mmcefhi.irx", "udpfs_fhi.irx", "dvrfile_fhi.irx"):
            self.assertIn(name,PATCH)
        self.assertLess(PATCH.index("y2_pre_settings[y2_i] = ptr != NULL;"),
                        PATCH.rindex('int fhi_init_result = fhi_config_init(&drv.mod);'))
        self.assertIn('scr_printf("Y2 ACTUAL SELECT idx=%d %s',PATCH)
        self.assertIn("memset(s, 0, candidates[i].settings_size)",PATCH)
        self.assertIn("y2_chosen != 3",PATCH)
        self.assertIn('Y2 95 DVR FHI+CDVD READY / SAFE STOP',PATCH)
        self.assertIn('return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */',PATCH)
        self.assertIn('s.index("if (setup_dvd_iso(sDVDFile")',PATCH)
        self.assertNotIn("fileXioWrite(",PATCH)
        self.assertNotIn("FIO_MT_RDWR",PATCH)

    def test_builds_fork_sources_in_place_only(self):
        self.assertIn("patch_y_fhi.py",WF)
        self.assertIn("patch_y2_fhi_preinit.py",WF)
        self.assertIn("ee/loader/src/fhi_config.c",WF)
        for x in ("PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF",
                  "PSX_DVR_HDD_NEUTRINO_Y2_LAUNCH.ELF",
                  "DIAG_Y2.CNF",
                  "PSX-DVR-HDD-NEUTRINO-Y2-FHI-PREINIT"):
            self.assertIn(x,WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_Y2_LAUNCH.ELF",CM)

if __name__=="__main__":
    unittest.main()
