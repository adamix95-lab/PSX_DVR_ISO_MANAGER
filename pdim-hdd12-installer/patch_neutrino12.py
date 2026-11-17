#!/usr/bin/env python3
"""Transform only the freshly patched HDD 1.0 Neutrino runtime to PP.DVRISO12.

The original 1.0 patches remain unchanged; this script runs on a build copy
after patch_hdd_runtime.py and before the remaining diagnostics patches.
"""
from pathlib import Path
import sys
if len(sys.argv) != 2:
    raise SystemExit("usage: patch_neutrino12.py ee/loader/src/main.c")
p = Path(sys.argv[1])
s = p.read_text(encoding="utf-8")
# Exactly the runtime/argv references for the test partition must be moved.
needles = {
  '"hdd0:PP.DVRISO:pfs:/neutrino_hdd.elf"':
    '"hdd0:PP.DVRISO12:pfs:/neutrino_hdd.elf"',
  '"hdd0:PP.DVRISO:pfs:/neutrino_hdd_lang.elf"':
    '"hdd0:PP.DVRISO12:pfs:/neutrino_hdd_lang.elf"',
  'fileXioMount("pfs0:", "hdd0:PP.DVRISO", FIO_MT_RDONLY)':
    'fileXioMount("pfs0:", "hdd0:PP.DVRISO12", FIO_MT_RDONLY)',
}
for old, new in needles.items():
    if s.count(old) != 1:
        raise SystemExit("Unexpected Neutrino base, refuse patch: " + old)
    s = s.replace(old, new)
# After these anchors are transformed, any unmodified partition mention must
# be comments only: the target runtime checks and loader all point to HDD12.
assert 'strcmp(sDVDFile, argv[2] + 5) != 0' in s
assert 'fileXioMount("pfs0:", "hdd0:PP.DVRISO12", FIO_MT_RDONLY)' in s
assert 'hdd0:PP.DVRISO:pfs:/' not in s
p.write_text(s, encoding="utf-8")
print("TEST12 Neutrino: PP.DVRISO12 only in runtime/argv, FIX95 still active")
