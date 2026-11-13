#!/usr/bin/env python3
"""Re-root USB Manager to an isolated G2A/G2B folder; preserve all behavior.

G2A, G2B and G2C use exactly the same Manager code. The experiment changes only
the Neutrino SCMD bytes in G2B; the generated BOOT.CNF continues to name
'neutrino.elf' or 'neutrino_lang.elf' in the current test directory.
"""
from pathlib import Path
import argparse

TARGETS = ("DESR_G2A", "DESR_G2B", "DESR_G2C", "DESR_G3A", "DESR_G3B", "DESR_G4A", "DESR_G4B", "DESR_G5A", "DESR_G5B", "DESR_G5R")


def apply(text: str, folder: str) -> str:
    if folder not in TARGETS:
        raise ValueError("unsupported folder name")
    mapping = {
        '#define DESR_APP_VARIANT "1.0 USB"':
        f'#define DESR_APP_VARIANT "1.0 USB {folder[-3:]}"',
        '#define DESR_APP_ROOT "mass0:/DESR_DVR/"':
        f'#define DESR_APP_ROOT "mass0:/{folder}/"',
        '#define DESR_APP_SYSTEM_NEUTRINO "mass0:/DESR_DVR/neutrino.elf"':
        f'#define DESR_APP_SYSTEM_NEUTRINO "mass0:/{folder}/neutrino.elf"',
        '#define DESR_APP_ENGLISH_NEUTRINO "mass0:/DESR_DVR/neutrino_lang.elf"':
        f'#define DESR_APP_ENGLISH_NEUTRINO "mass0:/{folder}/neutrino_lang.elf"',
    }
    for old, new in mapping.items():
        if text.count(old) != 1:
            raise ValueError(f"unexpected Manager source: {old}")
        text = text.replace(old, new)
    assert "#define DESR_APP_PARTITION \"hdd0:PP.DVRISO\"" in text
    assert f'#define DESR_APP_ROOT "mass0:/{folder}/"' in text
    return text


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("folder", choices=TARGETS)
    args = ap.parse_args()
    args.source.write_text(apply(args.source.read_text(), args.folder))
    print(f"Manager USB root: mass0:/{args.folder}/ (HDD code unchanged)")


if __name__ == "__main__":
    main()
