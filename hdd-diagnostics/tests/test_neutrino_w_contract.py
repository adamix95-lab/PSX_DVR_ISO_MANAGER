"""Neutrino W contract: same early-stop Neutrino V, visible main entry delay."""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
NP=ROOT/"neutrino-hdd-probe"
CNF=(NP/"DIAG_W.CNF").read_bytes()
LAUNCH=(NP/"osdmenu_neutrino_w_launcher.c").read_text("utf-8")
PATCH=(NP/"patch_probe.py").read_text("utf-8")
HOLD=(NP/"patch_w_entry_hold.py").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM=(ROOT/"CMakeLists.txt").read_text("utf-8")

class NeutrinoWContract(unittest.TestCase):
    def test_cnf_only_targets_w_dummy_neutrino(self):
        self.assertEqual(CNF,
            (b"boot=PSX_DVR_HDD_NEUTRINO_W_ENTRY.ELF\n"
             b"arg=-bsd=dvr\n"
             b"arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso\n"
             b"arg=-dev9=NICHDD\n"))
        for token in (
            '#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_W.CNF"',
            'PFS_MOUNTPOINT "/DIAG_W.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_W_ENTRY.ELF"',
            'q_config_ok == (int)sizeof(q_expected_config) - 1',
            "q_last_byte != '\\n' && q_last_byte != 0",
            "if (enabled && (fresh & PAD_CROSS)) return 1;",
        ):
            self.assertIn(token,LAUNCH)

    def test_kernel_hold_is_before_filexio_and_has_exit_hold(self):
        for token in ("DelayThread(15000000)", "DelayThread(30000000)",
                      "W00 MAIN REACHED", "W01 ENTRY HOLD COMPLETED"):
            self.assertIn(token,HOLD)
        self.assertIn('scr_printf("NO ISO OR GAME. READ ONLY.',HOLD)
        self.assertIn("    SifInitRpc(0);",PATCH)
        self.assertIn("return 0; /* ABSOLUTE no-game stop:",PATCH)
        self.assertIn('fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)',PATCH)
        self.assertNotIn("ExecPS2(",HOLD)
        self.assertNotIn("fileXioWrite(",HOLD)

    def test_w_built_separately_from_unpacked_ee_fork(self):
        for token in ("patch_probe.py", "patch_w_entry_hold.py",
                      "make -C ee/loader clean", "make -C ee/loader all",
                      "ee/loader/neutrino_unpacked.elf",
                      "PSX_DVR_HDD_NEUTRINO_W_ENTRY.ELF",
                      "PSX_DVR_HDD_NEUTRINO_W_LAUNCH.ELF",
                      "PSX-DVR-HDD-NEUTRINO-W-EARLY-ENTRY"):
            self.assertIn(token,WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_W_LAUNCH.ELF",CM)
        self.assertIn("FIO_MT_RDONLY",LAUNCH)
        self.assertNotIn("FIO_MT_RDWR",LAUNCH)

if __name__=="__main__":
    unittest.main()
