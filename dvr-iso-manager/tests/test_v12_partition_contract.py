#!/usr/bin/env python3
"""Host-side safety contract for the isolated PSX DVR ISO MANAGER 1.2 HDD.

Validates preprocessor output for experimental and stable binaries. This does
not claim the v1.2 binary or TLS support has been tested on DESR hardware.
"""
from __future__ import annotations

import pathlib
import re
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
INCLUDE = ROOT / "include"
STORAGE = INCLUDE / "runtime_storage.h"
CMAKE = ROOT / "CMakeLists.txt"
MAIN = ROOT / "src" / "main.c"


def defines(**flags: int) -> subprocess.CompletedProcess[str]:
    args = ["cc", "-std=c99", "-dM", "-E", "-x", "c", "-I", str(INCLUDE)]
    args.extend(f"-D{name}={value}" for name, value in flags.items())
    args.append("-")
    return subprocess.run(
        args,
        input='#include "runtime_storage.h"\n',
        text=True,
        capture_output=True,
        check=False,
    )


def macro(dump: str, name: str) -> str:
    found = re.search(r"^#define " + re.escape(name) + r" (.+)$", dump, re.M)
    if not found:
        raise AssertionError(f"Missing macro {name}")
    return found.group(1)


class TestV12PartitionIsolation(unittest.TestCase):
    def test_v12_hdd_is_distinct(self) -> None:
        result = defines(DESR_HDD_TEST=1, DESR_HDD_NETFIX=1, DESR_HDD_PREVIEW=1, DESR_HDD_V12=1)
        self.assertEqual(result.returncode, 0, result.stderr)
        values = result.stdout
        self.assertEqual(macro(values, "DESR_APP_VARIANT"), '"1.2 HDD"')
        self.assertEqual(macro(values, "DESR_APP_PARTITION"), '"hdd0:PP.DVRISO12"')
        self.assertEqual(macro(values, "DESR_APP_HDD_CNF_ARG"), '"hdd0:PP.DVRISO12/BOOT.CNF"')
        self.assertEqual(macro(values, "DESR_APP_ROOT"), '"pfs1:/"')
        self.assertEqual(macro(values, "DESR_APP_READONLY"), "1")
        self.assertEqual(macro(values, "DESR_APP_ART_ROOT"), 'DESR_APP_ROOT "ART/"')

    def test_stable_hdd_is_unchanged(self) -> None:
        result = defines(DESR_HDD_TEST=1, DESR_HDD_NETFIX=1)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(macro(result.stdout, "DESR_APP_VARIANT"), '"1.0 HDD"')
        self.assertEqual(macro(result.stdout, "DESR_APP_PARTITION"), '"hdd0:PP.DVRISO"')
        self.assertEqual(macro(result.stdout, "DESR_APP_HDD_CNF_ARG"), '"hdd0:PP.DVRISO/BOOT.CNF"')

    def test_stable_usb_is_unchanged(self) -> None:
        result = defines()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(macro(result.stdout, "DESR_APP_VARIANT"), '"1.0 USB"')
        self.assertEqual(macro(result.stdout, "DESR_APP_ROOT"), '"mass0:/DESR_DVR/"')

    def test_v12_requires_patched_hdd_build(self) -> None:
        for flags in (
            {"DESR_HDD_V12": 1},
            {"DESR_HDD_V12": 1, "DESR_HDD_TEST": 1},
            {"DESR_HDD_V12": 1, "DESR_HDD_NETFIX": 1},
            {"DESR_HDD_V12": 1, "DESR_HDD_PREVIEW": 1, "DESR_HDD_NETFIX": 1},
            {"DESR_HDD_V12": 1, "DESR_HDD_TEST": 1, "DESR_HDD_NETFIX": 1},
        ):
            result = defines(**flags)
            self.assertNotEqual(result.returncode, 0, flags)

    def test_v12_is_opt_in_and_not_main_build(self) -> None:
        cmake = CMAKE.read_text(encoding="utf-8")
        self.assertIn(
            "add_executable(DVR_ISO_MANAGER_V12_HDD.ELF EXCLUDE_FROM_ALL",
            cmake,
        )
        self.assertIn(
            "target_compile_definitions(DVR_ISO_MANAGER_V12_HDD.ELF PRIVATE DESR_HDD_TEST=1 DESR_HDD_NETFIX=1 DESR_HDD_PREVIEW=1 DESR_HDD_V12=1)",
            cmake,
        )
        self.assertIn("add_executable(DVR_ISO_MANAGER.ELF", cmake)
        main = MAIN.read_text(encoding="utf-8")
        self.assertIn('fileXioMount("pfs1:", DESR_APP_PARTITION, FIO_MT_RDONLY)', main)
        self.assertIn('fileXioMount("pfs1:", DESR_APP_PARTITION, FIO_MT_RDWR)', main)
        self.assertIn("(char *)DESR_APP_HDD_CNF_ARG", main)
        self.assertIn("return -230;", main)  # Experimental RW remount veto
        self.assertNotIn('fileXioMount("pfs1:", "hdd0:PP.DVRISO"', main)


if __name__ == "__main__":
    unittest.main(verbosity=2)
