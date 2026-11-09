#!/usr/bin/env python3
"""Static safety guards for the explicit, standalone N3C PFS diagnostics.

Not a proof of runtime disk isolation; actual binaries must be inspected
and write/remount behavior tested on physical PSX DESR hardware.
"""
from pathlib import Path
import re
import unittest

HERE = Path(__file__).resolve().parents[1]
SRC = (HERE / "src/v12_n3c_pfs_probe.c").read_text(encoding="utf-8")
CMAKE = (HERE / "CMakeLists.txt").read_text(encoding="utf-8")


class N3CPfsTest(unittest.TestCase):
    def test_only_exact_test_partition_and_new_files(self):
        self.assertIn('#define N3C_PARTITION "hdd0:PP.DVRISO12"', SRC)
        self.assertIn('#define N3C_SLOT0 "pfs1:/PDIM_N3C_TEST_SLOT0.BIN"', SRC)
        self.assertIn('#define N3C_SLOT1 "pfs1:/PDIM_N3C_TEST_SLOT1.BIN"', SRC)
        self.assertNotRegex(SRC, r'"hdd0:PP\.DVRISO(?:/|")')
        for forbidden in (
            "fileXioFormat(", "fileXioRemove(", "fileXioRename(",
            "fileXioRmdir(", "fileXioMkdir(",
            "dvr_pfs0:/", "BOOT.CNF", "ART/",
            "sceCdWriteClock(", "SifLoadElf(",
        ):
            self.assertNotIn(forbidden, SRC)

    def test_write_only_when_explicitly_compiled(self):
        self.assertIn("#ifdef V12_N3C_WRITE_ONCE", SRC)
        self.assertIn("#error \"Select exactly one N3C PFS diagnostic mode\"", SRC)
        self.assertIn("FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL", SRC)
        self.assertIn("if(a!=-ENOENT || c!=-ENOENT)", SRC)
        self.assertIn("fileXioSync(N3C_MOUNT,0)", SRC)
        self.assertIn("mount_partition(FIO_MT_RDONLY)", SRC)
        self.assertIn("fileXioUmount(N3C_MOUNT)", SRC)
        self.assertIn("return verify_slots();", SRC)
        self.assertIn("v12_rt_journal_select(", SRC)
        self.assertNotIn("FIO_O_TRUNC", SRC)
        self.assertNotIn("FIO_O_APPEND", SRC)

    def test_separate_opt_in_build_targets(self):
        self.assertIn('option(DESR_V12_N3C_PFS_DIAG', CMAKE)
        self.assertIn("V12_N3C_PFS_WRITE_ONCE.ELF", CMAKE)
        self.assertIn("V12_N3C_PFS_VERIFY_RO.ELF", CMAKE)
        self.assertIn("target_compile_definitions(V12_N3C_PFS_VERIFY_RO.ELF PRIVATE V12_N3C_VERIFY_RO=1)", CMAKE)
        self.assertIn("target_compile_definitions(V12_N3C_PFS_WRITE_ONCE.ELF PRIVATE V12_N3C_WRITE_ONCE=1)", CMAKE)
        self.assertIn("EXCLUDE_FROM_ALL", CMAKE)
        self.assertNotIn("DESR_V12_N3C_PFS_DIAG=ON", CMAKE)

    def test_journal_monotonic_guards(self):
        journal = (HERE / "src/net/v12_rt_journal.c").read_text()
        self.assertIn("lo->sequence+1U!=hi->sequence", journal)
        self.assertIn("lo->reservation.last_reserved+1U!=", journal)
        self.assertIn("if(chosen.sequence!=1U)", journal)


if __name__ == "__main__":
    unittest.main(verbosity=2)
