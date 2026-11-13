#!/usr/bin/env python3
"""G3: one-variable-at-a-time patch of existing Sony DVR SCMD 0x29.

The verified v1.0 Neutrino uses (mode=0, length=4).
Hardware G2B changed BOTH to (mode=1, length=1); it returned to Sony XMB
on physical QUIT but games would not start (black screen).
G3A changes MODE ONLY (1, 4).
G3B changes LENGTH ONLY (0, 1).
No second request, status check, retry, IOP reset or other runtime code.
"""
import argparse
from pathlib import Path
import difflib

OLD = (
    "            sceCdInit(SCECdINoD);\n"
    "            uint8_t in[4] = {0};\n"
    "            uint8_t out[16] = {};\n"
    "            sceCdApplySCmd(0x29, in, 4, out);\n"
    "            sceCdInit(SCECdEXIT);"
)


def replacement(variant):
    if variant == "G3A":  # only original value changed
        return OLD.replace("uint8_t in[4] = {0};", "uint8_t in[4] = {1};")
    if variant == "G3B":  # only original size changed
        return OLD.replace("sceCdApplySCmd(0x29, in, 4, out);",
                           "sceCdApplySCmd(0x29, in, 1, out);")
    raise ValueError("Unknown G3 variant")


def apply(text, variant):
    if text.count(OLD) != 1:
        raise ValueError("Neutrino not exactly pinned source / original SCMD sequence absent")
    new = replacement(variant)
    result = text.replace(OLD, new)
    if result.replace(new, OLD) != text:
        raise AssertionError("Unintended edit")
    assert "sceCdApplySCmd(0x29, in, 4, out);" in result if variant == "G3A" else \
        "sceCdApplySCmd(0x29, in, 1, out);" in result
    assert "uint8_t in[4] = {1};" in result if variant == "G3A" else \
        "uint8_t in[4] = {0};" in result
    return result


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("variant", choices=["G3A", "G3B"])
    args = ap.parse_args()
    source = args.source.read_text(encoding="utf-8")
    changed = apply(source, args.variant)
    args.source.write_text(changed, encoding="utf-8")
    print("G3 single-variable patch", args.variant)
    print("".join(difflib.unified_diff(
        source.splitlines(True), changed.splitlines(True),
        fromfile="1.0 clean", tofile=args.variant)))
