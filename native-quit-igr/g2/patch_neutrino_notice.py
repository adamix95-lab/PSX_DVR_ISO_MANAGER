#!/usr/bin/env python3
"""G2B: change ONLY the existing, pre-game DVR SCMD 0x29's mode and byte count.

The earlier native-quit experiment added extra CDVD calls, retries, aborts and
a second request outside the normal ROM/IOP phase. G2B deliberately does none
of those things. It is a minimally changed game-boot A/B diagnostic.
"""
from pathlib import Path
import argparse
import difflib

OLD = """            sceCdInit(SCECdINoD);
            uint8_t in[4] = {0};
            uint8_t out[16] = {};
            sceCdApplySCmd(0x29, in, 4, out);
            sceCdInit(SCECdEXIT);"""

NEW = """            sceCdInit(SCECdINoD);
            uint8_t in[4] = {1};
            uint8_t out[16] = {};
            sceCdApplySCmd(0x29, in, 1, out);
            sceCdInit(SCECdEXIT);"""


def apply(text: str) -> str:
    if text.count(OLD) != 1:
        raise ValueError("unexpected Neutrino source; require EXACT original ROM CDVD call")
    patched = text.replace(OLD, NEW)
    assert patched.count(NEW) == 1
    assert patched.replace(NEW, OLD) == text, "Unintended change detected"
    assert patched.count("sceCdApplySCmd(0x29, in, 1, out);") == 1
    return patched


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    args = ap.parse_args()
    original = args.source.read_text(encoding="utf-8")
    patched = apply(original)
    args.source.write_text(patched, encoding="utf-8")
    print("G2B: ONLY pre-existing SCMD 0x29 changed (0->1; 4->1 bytes)")
    print("No new RPC, abort, repeated call, hook, thread, IOP reset or HDD write")
    print("".join(difflib.unified_diff(
        original.splitlines(True), patched.splitlines(True),
        fromfile="Neutrino v1.0 DVR", tofile="G2B minimal mode1")))


if __name__ == "__main__":
    main()
