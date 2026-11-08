#!/usr/bin/env python3
"""Retarget ONLY the already-patched Neutrino HDD FIX95 loader to PP.DVRISO12.

Run this AFTER patch_hdd_runtime.py, patch_hdd_stages.py,
patch_hdd_config_trace.py and patch_hdd_error_screen.py. Do not run it on
the 1.0 source, and never change pcm720's DVR FHI / ISO functionality.
"""
from pathlib import Path
import re
import sys

if len(sys.argv) != 2:
    raise SystemExit("Usage: patch_hdd_runtime_v12.py ee/loader/src/main.c")

p = Path(sys.argv[1])
s = p.read_text(encoding="utf-8")

# The original FIX95 wrapper contributes exactly these three link targets:
# the SYSTEM and ENGLISH OSDMenu argv[0] values and the PFS0 mount.
old = "hdd0:PP.DVRISO"
new = "hdd0:PP.DVRISO12"
if s.count(old) != 3 or s.count('fileXioMount("pfs0:", "hdd0:PP.DVRISO"') != 1:
    raise SystemExit("Unexpected Neutrino 1.0 wrapper; refuse unsafe retarget")

s = s.replace(old, new)
if re.search(r"hdd0:PP\.DVRISO(?!12)", s):
    raise SystemExit("Production path still embedded in v1.2 Neutrino source")
for required in (
    '"hdd0:PP.DVRISO12:pfs:/neutrino_hdd.elf"',
    '"hdd0:PP.DVRISO12:pfs:/neutrino_hdd_lang.elf"',
    'fileXioMount("pfs0:", "hdd0:PP.DVRISO12", FIO_MT_RDONLY)',
    "strcmp(sDVDFile, argv[2] + 5) != 0",
):
    if required not in s:
        raise SystemExit(f"Missing v1.2 FIX95 invariant: {required}")

p.write_text(s, encoding="utf-8")
print("PASS: Neutrino v1.2 SYSTEM/ENGLISH and PFS0 mount isolated to PP.DVRISO12")
