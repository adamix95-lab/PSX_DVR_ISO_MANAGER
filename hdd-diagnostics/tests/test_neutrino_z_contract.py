"""Z real ISO test — single filename may vary, everything else strict."""
from pathlib import Path
import unittest

R=Path(__file__).resolve().parents[1]
NP=R/"neutrino-hdd-probe"
CNF=(NP/"DIAG_Z.CNF").read_bytes()
L=(NP/"osdmenu_neutrino_z_launcher.c").read_text("utf-8")
P=(NP/"patch_z_real_iso.py").read_text("utf-8")
WF=(R.parent/".github/workflows/build-hdd-neutrino-probe.yml").read_text("utf-8")
CM=(R/"CMakeLists.txt").read_text("utf-8")

class ZRealGame(unittest.TestCase):
    def test_cnf_template_fixed_prefix_and_padded_end(self):
        self.assertEqual(CNF,
            b"boot=PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF\n"
            b"arg=-bsd=dvr\n"
            b"arg=-dvd=dvr_pfs0:/ISO/NAZWA_GRY.iso\n"
            b"arg=-dev9=NICHDD\n"
            b"#PAD\n")
        self.assertEqual(CNF.count(b"arg="),3)
        self.assertIn("DIAG_Z.CNF",L)
    def test_strict_launcher_and_iso_exists_check(self):
        for t in (
            'PFS_MOUNTPOINT "/DIAG_Z.CNF"',
            'PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF"',
            '"boot=PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF\\n"',
            '"arg=-bsd=dvr\\n"',
            '"arg=-dvd=dvr_pfs0:/ISO/"',
            '"\\narg=-dev9=NICHDD\\n#PAD"',
            "q_last_byte != '\\n' && q_last_byte != 0 && q_last_byte != 1",
            "q_config_ok > 0 && z_selected_iso_open >= 0",
            "fileXioOpen(z_selected_iso,FIO_O_RDONLY,0)",
            "z_valid_name(p, filename_len)",
            'z_selected_iso_open=fd',
            'if (enabled && (fresh & PAD_CROSS)) return 1;',
            "FIO_MT_RDONLY",
        ): self.assertIn(t,L)
        for t in ("FIO_MT_RDWR", "fileXioWrite(", "fileXioRemove(",
                  "fileXioMkdir(", "fileXioFormat("):
            self.assertNotIn(t,L)
    def test_target_game_path_and_boot_are_intentional(self):
        for t in (
            "z_allowed_iso_arg(const char *a)",
            '"-dvd=dvr_pfs0:/ISO/"',
            "strstr(a+off, \"..\")",
            "Z96 setup_dvd_iso BEGIN",
            "Z97 ISO SETUP OK",
            "Z98 SYSTEM.CNF ISO READ BEGIN",
            "Z99 GAME HANDOFF BEGIN",
            'ExecPS2((void *)eh->entry',
        ): self.assertIn(t,P)
        self.assertIn("patch_y2_fhi_preinit.py",WF)
        self.assertIn("patch_z_real_iso.py",WF)
        self.assertIn("PSX-DVR-HDD-NEUTRINO-Z-REAL-GAME",WF)
        self.assertIn("PSX_DVR_HDD_NEUTRINO_Z_LAUNCH.ELF",CM)

if __name__=="__main__":
    unittest.main()
