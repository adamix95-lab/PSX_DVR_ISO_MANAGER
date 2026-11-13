#!/usr/bin/env python3
"""G5S post-SYSTEM.CNF fine trace, intentionally STOPS before clearing EE RAM.

The physical G5R returned STAGE 91 STOP (exact final screen pending).
G5R had no further visible stage between SYSTEM.CNF parse and EE handoff.
This probe retains its entry/return trap and screens, but inserts detailed
checkpoints after SYSTEM.CNF and STOPS BEFORE the 0x82000-0x100000 EE clear.

Never attempts to hand off the game. No HDD writes. Mode0, 1-byte SCMD 0x29,
same hardware-working control as G3B and G5R. No ChangeSys or extra IOP resets.
"""
from pathlib import Path
import argparse
import difflib
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g5r"))
from patch_early_return import patch as patch_g5r
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g5"))
from patch_stage_probe import patch_makefile, once


def patch(src: str) -> str:
    s = patch_g5r(src)
    s = once(s, "PSX DESR NATIVE QUIT G5R / USB ONLY",
             "PSX DESR NATIVE QUIT G5S / USB ONLY", "G5S visible identity")

    # Insert phase 92 immediately AFTER the completed read() and before any
    # parsing of BOOT2 / game path from SYSTEM.CNF.
    s = once(s,
        '        sys.sELFFile = strstr(system_cnf_data, "cdrom0:");',
        '        g5_note(92, "Before BOOT2 filename parse", 0);\n'
        '        sys.sELFFile = strstr(system_cnf_data, "cdrom0:");',
        "BOOT2 parse entrance")

    # Pin to original failure path. A return is already trapped by G5R, but
    # a dedicated 292 error avoids silently missing a malformed CNF.
    s = once(s,
        '''        if (sys.sELFFile == NULL || fname_end == NULL) {
            printf("ERROR: file name not found in SYSTEM.CNF\\n");
            return -1;
        }''',
        '''        if (sys.sELFFile == NULL || fname_end == NULL) {
            g5_stop(292, "SYSTEM.CNF BOOT2 missing", -1);
            printf("ERROR: file name not found in SYSTEM.CNF\\n");
            return -1;
        }''',
        "BOOT2 validation")
    s = once(s,
        """        fname_end[1] = '1';
        fname_end[2] = '\\0';""",
        """        fname_end[1] = '1';
        fname_end[2] = '\\0';
        g5_note(93, "BOOT2 ELF path located", (int)strlen(sys.sELFFile));""",
        "BOOT2 path parsed")

    # Capture completion of the per-game compatibility TOML and transition to
    # CDVDMAN/media settings.
    s = once(s,
        '''    /*
     * Set CDVDMAN settings
     */''',
        '''    g5_note(94, "Game compat parsed / CDVD next", 0);
    /*
     * Set CDVDMAN settings
     */''',
        "media config entrance")

    s = once(s,
        '''    /*
     * Set deckard compatibility
     */''',
        '''    g5_note(95, "CDVDMAN media configured", 0);
    /*
     * Set deckard compatibility
     */''',
        "media config completion")

    s = once(s,
        '''    /*
     * Enable ATA0 emulation
     */''',
        '''    g5_note(96, "Deckard settings applied", 0);
    /*
     * Enable ATA0 emulation
     */''',
        "Deckard finished")

    s = once(s,
        '''    /*
     * PS2 Logo: determine region and whether a patch is needed.''',
        '''    g5_note(97, "ATA / MC emulation config complete", 0);
    /*
     * PS2 Logo: determine region and whether a patch is needed.''',
        "ATA/MC config")

    s = once(s,
        '''    /*
     * Fill fake module table
     */''',
        '''    g5_note(98, "Logo settings done; fake table next", 0);
    /*
     * Fill fake module table
     */''',
        "logo settings done")

    # The first major destructive memory operation occurs immediately after
    # the fake module table. STOP there to avoid wiping libdebug's framebuffer
    # or executing subsequent EE loader patches before establishing control.
    s = once(s,
        '''#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"''',
        '''    g5_stop(105, "POST-CNF PASS; STOP BEFORE EE CLEAR", drv.fake.count);
    /* G5S never performs the EE scratch memory clear or launches the game. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"''',
        "hard stop before EE RAM clear")

    if s.count('g5_stop(105, "POST-CNF PASS; STOP BEFORE EE CLEAR"') != 1:
        raise ValueError("G5S did not protect EE clear")
    if s.count('g5_stop(298, "Neutrino returned instead of EE"') != 1:
        raise ValueError("G5R wrapper return trap lost")
    if s.count('g5_stop(199, "EE handoff reached; ISO/FHI PASS"') != 1:
        raise ValueError("Original G5 EE handoff guard lost")
    if "sceCdApplySCmd(0x29, in, 4, out);" in s or "SifCallRpc(&clientSCmd, 0x2D" in s:
        raise ValueError("G5S changed CDVD command mode or ChangeSys")
    return s


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("main_c",type=Path)
    p.add_argument("makefile",type=Path)
    a=p.parse_args()
    original=a.main_c.read_text(encoding="utf-8")
    modified=patch(original)
    original_mk=a.makefile.read_text(encoding="utf-8")
    mod_mk=patch_makefile(original_mk)
    a.main_c.write_text(modified,encoding="utf-8")
    a.makefile.write_text(mod_mk,encoding="utf-8")
    print("G5S: mode0 len1, detailed post-CNF checks 92-98, STOP 105 BEFORE EE RAM CLEAR")
    print("G5S: original 298 return trap + USB only, no HDD writes")
    print("".join(difflib.unified_diff(original.splitlines(True),modified.splitlines(True),
                  fromfile="pinned Neutrino", tofile="G5S diagnostics")))
