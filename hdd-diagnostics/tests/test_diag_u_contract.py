"""DIAG U contract — only controlled read-only PP.TESTDVR remount after OSDMenu."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC=(ROOT/"src/osdmenu_entry_remount_target.c").read_text("utf-8")
LAUNCH=(ROOT/"src/osdmenu_entry_remount_launcher.c").read_text("utf-8")
CNF=(ROOT/"diag-u/DIAG_U.CNF").read_bytes()
BUILD=(ROOT/"CMakeLists.txt").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")

EXPECTED=(b"boot=PSX_DVR_HDD_DIAG_U_TARGET.ELF\n"
          b"arg=-bsd=dvr\n"
          b"arg=-dvd=dvr_pfs0:/ISO/DIAG_U_NO_GAME.iso\n"
          b"arg=-dev9=NICHDD\n")

class DiagUContract(unittest.TestCase):
    def test_isolated_config_and_build(self):
        self.assertEqual(CNF,EXPECTED)
        self.assertEqual(len(CNF),107)
        for phrase in ('"hdd0:PP.TESTDVR/DIAG_U.CNF"', '"/DIAG_U.CNF"',
                       '"boot=PSX_DVR_HDD_DIAG_U_TARGET.ELF\\n"',
                       '"arg=-dev9=NICHDD\\n"', 'q_last_byte != \'\\n\' && q_last_byte != 0',
                       'q_config_ok == (int)sizeof(q_expected_config) - 1'):
            self.assertIn(phrase,LAUNCH)
        for file in ("PSX_DVR_HDD_DIAG_U_TARGET.ELF","PSX_DVR_HDD_DIAG_U_LAUNCH.ELF"):
            self.assertIn(file,BUILD)
            self.assertIn(file,WF)
        self.assertIn("PSX-DVR-HDD-DIAG-U-RO-REMOUNT",WF)
        self.assertIn('CNF: hdd0:PP.TESTDVR/DIAG_U.CNF',LAUNCH)
        self.assertIn("RO PREFLIGHT. UCNF -> U TARGET ONLY.",LAUNCH)

    def test_mount_only_when_pfs0_absent_and_hdd_reachable(self):
        self.assertIn("inspect_at_elf_entry();", SRC)
        self.assertIn("inspect_after_remount();", SRC)
        self.assertLess(SRC.index("inspect_at_elf_entry();"), SRC.index("inspect_after_remount();"))
        self.assertLess(SRC.index("inspect_after_remount();"), SRC.index('while (!SifIopReset("", 0))'))
        self.assertIn("t_hdd0_dir < 0 || t_pfs0_dir >= 0",SRC)
        self.assertEqual(SRC.count("fileXioMount("),1)
        self.assertIn('fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)',SRC)
        self.assertEqual(SRC.count("fileXioUmount("),1)
        self.assertIn("if (u_mount_owned)",SRC)
        self.assertIn("if (u_unmount_code == 0) u_mount_owned = 0;",SRC)

    def test_file_visibility_after_remount(self):
        for term in ("try_open_rpc(\"pfs0:/config/system.toml\")",
                     "try_open_rpc(\"pfs0:/modules/dvrfile_fhi.irx\")",
                     'u_chdir = chdir("pfs0:/")',
                     'try_open_posix("config/system.toml"',
                     'try_open_posix("modules/dvrfile_fhi.irx"',
                     "U1 RO MOUNT BEGIN", "U4 DVR IRX RO OPEN BEGIN",
                     "U8 UMOUNT OWN PFS0 BEGIN", "NO NEUTRINO/GAME."):
            self.assertIn(term,SRC)

    def test_no_write_or_real_iso(self):
        for source in (SRC,LAUNCH):
            clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
            for bad in ("FIO_MT_RDWR", "FIO_O_WRONLY", "FIO_O_RDWR", "FIO_O_CREAT",
                        "FIO_O_TRUNC", "fileXioWrite(", "fileXioFormat(",
                        "fileXioRemove(", "fileXioMkdir(", "fileXioRename("):
                self.assertNotIn(bad,clean)
        for bad in ("LoadELFFromFile(", "ExecPS2(", "SifLoadElf("):
            self.assertNotIn(bad,SRC)
        self.assertIn("DIAG_U_NO_GAME.iso",SRC)

if __name__ == "__main__":
    unittest.main()
