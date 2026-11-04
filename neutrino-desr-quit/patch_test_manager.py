#!/usr/bin/env python3
"""Build-only adapter: original Manager launch chain, separate QUIT loaders."""
from pathlib import Path
import sys

root = Path(sys.argv[1])
names = {
    "neutrino.elf": "neutrino_quit.elf",
    "neutrino_lang.elf": "neutrino_quit_lang.elf",
    "neutrino_hdd.elf": "neutrino_hdd_quit.elf",
    "neutrino_hdd_lang.elf": "neutrino_hdd_quit_lang.elf",
}
for rel in ("include/runtime_storage.h", "src/iso_utils.c", "src/hdd_boot_config.c"):
    p = root / rel
    s = p.read_text()
    found = 0
    for old, new in names.items():
        found += s.count(old)
        s = s.replace(old, new)
    if not found:
        raise SystemExit(f"No known Neutrino targets in {rel}")
    if rel == "include/runtime_storage.h":
        s = s.replace('"1.0 HDD"', '"1.0 HDD QUIT TEST"')
        s = s.replace('"1.0 USB"', '"1.0 USB QUIT TEST"')
    p.write_text(s)
print("Test Manager: only loader names and visible QUIT TEST title changed")
