#!/usr/bin/env python3
"""Second, diagnostic-only transform for Neutrino W entry marker test.

Applied after patch_probe.py has already inserted its unconditional pre-game
stage-70 stop. W makes the same V test deterministic about reachability of
main() by using a bounded volatile CPU loop, independent of PS2SDK timers and libc sleep().
"""
from pathlib import Path
import sys

p=Path(sys.argv[1])
s=p.read_text("utf-8")

entry='''int main(int argc, char *argv[])
{
    int init_ret'''
prefix='''/* Time-limited pure EE CPU loop, independent of timers disabled by
 * DISABLE_EXTRA_TIMERS_FUNCTIONS(). This is not an accurate clock.
 * No filesystem, IOP or ROM calls in the loop.
 */
static void w_busy_hold(unsigned blocks)
{
    unsigned block;
    for (block = 0; block < blocks; ++block) {
        volatile unsigned n;
        for (n = 0; n < 30000000u; ++n)
            __asm__ __volatile__("nop");
    }
}

int main(int argc, char *argv[])
{
    int init_ret'''
if s.count(entry)!=1:
    raise SystemExit("Refusing W patch: wrapper main anchor missing")
s=s.replace(entry,prefix)
old='''    scr_printf("NO ISO OR GAME. READ ONLY.\\n");

    if (argc != 3'''
new='''    scr_printf("NO ISO OR GAME. READ ONLY.\\n");
    scr_printf("W00 MAIN REACHED: CPU LOOP\\n");
    w_busy_hold(8);
    scr_printf("W01 ENTRY HOLD COMPLETED\\n");

    if (argc != 3'''
if s.count(old)!=1:
    raise SystemExit("Refusing W patch: unexpected entry marker")
s=s.replace(old,new)
old='''    scr_printf("PHOTO RESULTS. AUTO EXIT IN 45 S.\\n");
    sleep(45);'''
new='''    scr_printf("W90 CPU LOOP, PHOTO\\n");
    w_busy_hold(16);'''
if s.count(old)!=1:
    raise SystemExit("Refusing W patch: unexpected end marker")
s=s.replace(old,new)
if "return 0; /* ABSOLUTE no-game stop:" not in s:
    raise SystemExit("Unsafe build: missing hard no-game stop")
if 'fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)' not in s:
    raise SystemExit("Unsafe build: missing RO mount")
p.write_text(s,encoding="utf-8")
print("Neutrino W: mandatory kernel DelayThread entry+result hold added")
