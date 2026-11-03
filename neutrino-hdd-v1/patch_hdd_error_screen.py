#!/usr/bin/env python3
"""Add fail-closed *diagnostic* on-screen errors to HDD v1 Neutrino FIX95.

Run only after patch_hdd_runtime.py. Matches the physically proven DIAG W
method: libdebug screen and a bounded-content CPU loop, no timer assumptions.
Does not change DVD/FHI setup, ISO selection, or game handoff on success.
"""
from pathlib import Path
import sys

if len(sys.argv) != 3:
    raise SystemExit("Usage: patch_hdd_error_screen.py main.c Makefile")
source, makefile = map(Path, sys.argv[1:])
s = source.read_text(encoding="utf-8")
mk = makefile.read_text(encoding="utf-8")

def once(old: str, new: str, label: str) -> None:
    global s
    count = s.count(old)
    if count != 1:
        raise SystemExit(f"HDD error screen patch refused {label}: count={count}")
    s = s.replace(old, new)

once("#include <sifrpc.h>", "#include <sifrpc.h>\n#include <debug.h>",
     "debug header")

marker = "int main(int argc, char *argv[])\n{\n    int rc, mount_rc;"
helper = r'''
/* Shown ONLY after an error/return. A failed HDD loader used to return
 * immediately to XMB without an error number. Pure EE spin survives
 * disabled PS2SDK timer functions (proven by DIAG W). Reboot to exit.
 */
static void desr_hdd_show_error(int code, int argc, char *argv[])
{
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0);
    scr_clear();
    scr_printf("PSX DVR NEUTRINO 1.0 HDD / FIX2\n");
    scr_printf("FAILED/RETURNED CODE=%d ARGC=%d\n", code, argc);
    scr_printf("UPSTREAM STAGE=%d (see README stage map)\n",
               (int)desr_hdd_stage);
    scr_printf("ARG0: %.60s\n",
               argv && argv[0] ? argv[0] : "(null)");
    scr_printf("ARG1: %.60s\n",
               argv && argc > 1 && argv[1] ? argv[1] : "(null)");
    scr_printf("ARG2: %.60s\n",
               argv && argc > 2 && argv[2] ? argv[2] : "(null)");
    scr_printf("NO FURTHER IOP OPERATIONS. PHOTO / REBOOT.\n");
    for (;;) {
        volatile unsigned long i;
        for (i = 0; i < 1000000UL; ++i)
            __asm__ __volatile__("nop");
    }
}

'''
once(marker, helper + marker, "wrapper entry")

# Only the isolated generated wrapper is modified, never upstream Neutrino.
wrapper_start = s.index(marker)
head, tail = s[:wrapper_start], s[wrapper_start:]
for code in (-100, -101, -102, -103, -104, -105):
    old = f"return {code};"
    if tail.count(old) != 1:
        raise SystemExit(f"HDD wrapper error {code} count {tail.count(old)}")
    tail = tail.replace(old,
                        f"{{ desr_hdd_show_error({code}, argc, argv); return {code}; }}")
old = '''    rc=neutrino_hdd_upstream_main(argc,argv);
    /* Usually ExecPS2 does not return. If it does, undo only our mount. */
    chdir("hdd0:");
    fileXioUmount("pfs0:");
    return rc;'''
new = '''    rc=neutrino_hdd_upstream_main(argc,argv);
    /* A real game normally never returns. Do not make RPCs or silently
     * jump to XMB if Neutrino returned; show the exact stage result.
     */
    desr_hdd_show_error(rc == 0 ? -106 : rc, argc, argv);
    return rc;'''
if tail.count(old) != 1:
    raise SystemExit("HDD wrapper core-return anchor missing")
tail = tail.replace(old, new)
s = head + tail

lib = "EE_LIBS = -lfileXio -lpatches -lcdvd"
if mk.count(lib) != 1:
    raise SystemExit("Pinned upstream Makefile EE_LIBS anchor missing")
mk = mk.replace(lib, lib + " -ldebug")
if "strcmp(sDVDFile, argv[2] + 5) != 0" not in s:
    raise SystemExit("FIX95 no longer present")
if "hdd0:PP.DVRISO" not in s or "PP.TESTDVR" in s:
    raise SystemExit("Wrong partition in generated source")
source.write_text(s, encoding="utf-8")
makefile.write_text(mk, encoding="utf-8")
print("HDD STAGES diagnostic: exact upstream fail phase, preserve FIX95")
