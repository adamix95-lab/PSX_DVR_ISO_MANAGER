"""DIAG T: target-entry test is isolated, read-only and uses real OSDMenu."""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"src/osdmenu_entry_file_access_target.c").read_text("utf-8")
LAUNCH=(ROOT/"src/osdmenu_entry_file_access_launcher.c").read_text("utf-8")
CNF=(ROOT/"diag-t/DIAG_T.CNF").read_bytes()
BUILD=(ROOT/"CMakeLists.txt").read_text("utf-8")
WF=(ROOT.parent/".github/workflows/build-hdd-readonly-diag.yml").read_text("utf-8")
EXPECTED=(b"boot=PSX_DVR_HDD_DIAG_T_TARGET.ELF\n"
          b"arg=-bsd=dvr\n"
          b"arg=-dvd=dvr_pfs0:/ISO/DIAG_T_NO_GAME.iso\n"
          b"arg=-dev9=NICHDD\n")

class DiagTContract(unittest.TestCase):
    def test_exact_allowlisted_config(self):
        self.assertEqual(CNF, EXPECTED)
        self.assertEqual(len(CNF),107)
        self.assertIn('HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_T.CNF"', LAUNCH)
        self.assertIn('PFS_MOUNTPOINT "/DIAG_T.CNF"', LAUNCH)
        self.assertIn('PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_T_TARGET.ELF"', LAUNCH)
        self.assertIn('q_last_byte != \'\\n\' && q_last_byte != 0', LAUNCH)
        self.assertIn('memcmp(q_expected_config, q_config_bytes, (size_t)(length - 1))', LAUNCH)
        self.assertIn('q_config_ok == (int)sizeof(q_expected_config) - 1', LAUNCH)
        self.assertIn('if (enabled && (fresh & PAD_CROSS)) return 1;', LAUNCH)

    def test_first_probe_precedes_first_iop_reset(self):
        i=SRC.index("inspect_at_elf_entry();")
        j=SRC.index('while (!SifIopReset("", 0))')
        self.assertLess(i,j)
        for phrase in ("getcwd(cwd_text, sizeof(cwd_text))",
                       'fileXioDopen("pfs0:/")',
                       'fileXioDopen("hdd0:")',
                       'try_open_rpc("pfs0:/config/system.toml")',
                       'try_open_rpc("pfs0:/modules/dvrfile_fhi.irx")',
                       'try_open_rpc("config/system.toml")',
                       'try_open_rpc("modules/dvrfile_fhi.irx")',
                       'try_open_posix("config/system.toml"',
                       'try_open_posix("modules/dvrfile_fhi.irx"',
                       'fileXioInit()',
                       'if (new_press & PAD_START)'):
            self.assertIn(phrase,SRC)

    def test_no_write_or_game_execution(self):
        self.assertIn("PSX DVR HDD DIAG T / TARGET ENTRY",SRC)
        self.assertIn("DUMMY ISO NOT OPENED. NO GAME.",SRC)
        self.assertIn("FIO_MT_RDONLY",LAUNCH)
        self.assertIn("TEST_PARTITION", LAUNCH)
        self.assertIn("PSX-DVR-HDD-DIAG-T-TARGET-ENTRY-CWD",WF)
        for n in ("PSX_DVR_HDD_DIAG_T_TARGET.ELF","PSX_DVR_HDD_DIAG_T_LAUNCH.ELF"):
            self.assertIn(n,BUILD)
            self.assertIn(n,WF)
        for source in (SRC, LAUNCH):
            cleaned=re.sub(r"/\*.*?\*/|//[^\n]*","",source,flags=re.S)
            for forbidden in ("fileXioWrite(", "fileXioRemove(", "fileXioFormat(",
                              "fileXioMkdir(", "fileXioRename(", "FIO_O_WRONLY",
                              "FIO_O_RDWR", "FIO_O_CREAT", "FIO_O_TRUNC",
                              "FIO_MT_RDWR"):
                self.assertNotIn(forbidden,cleaned)
        for forbidden in ("LoadELFFromFile(", "ExecPS2(", "SifLoadElf(",
                          "fileXioMount(", "fileXioUmount("):
            self.assertNotIn(forbidden,SRC)

if __name__=="__main__":
    unittest.main()
