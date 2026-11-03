#!/usr/bin/env python3
"""Second, diagnostic-only transform for Neutrino W entry marker test.

Applied after patch_probe.py has already inserted its unconditional pre-game
stage-70 stop. W makes the same V test deterministic about reachability of
main() by using the PS2 kernel's DelayThread, rather than libc sleep().
"""
from pathlib import Path
import sys

p=Path(sys.argv[1])
s=p.read_text("utf-8")
old='''    scr_printf("NO ISO OR GAME. READ ONLY.\\n");

    if (argc != 3'''
new='''    scr_printf("NO ISO OR GAME. READ ONLY.\\n");
    scr_printf("W00 MAIN REACHED: HOLD 15 SEC\\n");
    DelayThread(15000000);
    scr_printf("W01 ENTRY HOLD COMPLETED\\n");

    if (argc != 3'''
if s.count(old)!=1:
    raise SystemExit("Refusing W patch: unexpected entry marker")
s=s.replace(old,new)
old='''    scr_printf("PHOTO RESULTS. AUTO EXIT IN 45 S.\\n");
    sleep(45);'''
new='''    scr_printf("W90 KERNEL HOLD 30 SEC, PHOTO\\n");
    DelayThread(30000000);'''
if s.count(old)!=1:
    raise SystemExit("Refusing W patch: unexpected end marker")
s=s.replace(old,new)
if "return 0; /* ABSOLUTE no-game stop:" not in s:
    raise SystemExit("Unsafe build: missing hard no-game stop")
if 'fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)' not in s:
    raise SystemExit("Unsafe build: missing RO mount")
p.write_text(s,encoding="utf-8")
print("Neutrino W: mandatory kernel DelayThread entry+result hold added")
