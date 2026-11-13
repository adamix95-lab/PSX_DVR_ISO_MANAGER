#!/usr/bin/env python3
"""G5R follow-up to hardware G5A: return-to-XMB before expected stage 199.

Reuses exactly the G5A stage instrumentor (SCMD 0x29 mode0, len1) and augments it:
* Entry screen BEFORE first ROMVER/config/module operations.
* Phase labels (no additional RPC or IOP changes) before early failure paths.
* Wrap main() to trap any normal return with result code and last phase.
* Initialize libdebug GS ONLY ONCE (avoid repeated GS initialization effects).
* Stop before ExecPS2 exactly as G5 did.
No extra IOP resets, no filesystem writes, no HDD/XMB modification.
"""
import argparse
from pathlib import Path
import sys
import difflib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "g5"))
from patch_stage_probe import instrument, patch_makefile, once


def patch(source: str) -> str:
    s = instrument(source, mode=0)

    s = once(s, "PSX DESR NATIVE QUIT G5A / USB ONLY",
             "PSX DESR NATIVE QUIT G5R / USB ONLY",
             "separate G5R screen identity")
    s = once(s, "static int g5_stage = 0;",
             "static int g5_stage = 0;\nstatic int g5_screen_ready = 0;",
             "single GS initialize guard")
    s = once(s,
             "    g5_stage = code;\n    init_scr();",
             "    g5_stage = code;\n    if (!g5_screen_ready) {\n"
             "        init_scr();\n"
             "        g5_screen_ready = 1;\n"
             "    }",
             "single GS init")
    s = once(s,
             "    g5_note(code, name, value);\n    scr_printf(\"INTENTIONAL G5 STOP; NO GAME WAS LAUNCHED.\\n\");",
             "    int last_stage = g5_stage;\n"
             "    g5_note(code, name, value);\n"
             "    scr_printf(\"LAST PHASE BEFORE STOP: %d\\n\", last_stage);\n"
             "    scr_printf(\"INTENTIONAL G5 STOP; NO GAME WAS LAUNCHED.\\n\");",
             "last checkpoint of return")

    # These are only integer assignments. They do not call GS/IOP/CDVD while
    # preparing the familiar v1.0 runtime before G5's stage10 checkpoint.
    early = [
        ('        int fd_ROMVER;',
         '        g5_stage = 2; /* ROMVER read */\n        int fd_ROMVER;',
         "ROMVER"),
        ('    memset(&sys, 0, sizeof(struct SSystemSettings));',
         '    g5_stage = 3; /* parse config */\n'
         '    memset(&sys, 0, sizeof(struct SSystemSettings));',
         "config setup"),
        ('    if (load_config_file("system", NULL) < 0) {',
         '    g5_stage = 4; /* system.toml */\n'
         '    if (load_config_file("system", NULL) < 0) {',
         "system TOML"),
        ('    if (parse_cmdline_args(argc, argv, &iELFArgcStart) < 0)',
         '    g5_stage = 5; /* parse -bsd/-dvd args */\n'
         '    if (parse_cmdline_args(argc, argv, &iELFArgcStart) < 0)',
         "command line"),
        ('    toml_result_t toml_compat = load_config_file_toml("compat", NULL);',
         '    g5_stage = 6; /* compatibility configuration */\n'
         '    toml_result_t toml_compat = load_config_file_toml("compat", NULL);',
         "compat TOML"),
        ('        if (load_config_file("bsd", sys.sBSD) < 0) {',
         '        g5_stage = 7; /* DVR backing store config */\n'
         '        if (load_config_file("bsd", sys.sBSD) < 0) {',
         "DVR backing store config"),
        ('    } else if (load_config_file("emu-dvd", sys.sDVDMode) < 0) {',
         '    } else if ((g5_stage = 8, load_config_file("emu-dvd", sys.sDVDMode)) < 0) {',
         "DVD driver config"),
        ('    mod_ee_core.sFileName = sys.eecore_elf;',
         '    g5_stage = 9; /* EE core and LE module preloads */\n'
         '    mod_ee_core.sFileName = sys.eecore_elf;',
         "pre-IOP module preloads")
    ]
    for old, new, label in early:
        s = once(s,old,new,label)
    s = once(s, "int main(int argc, char *argv[])\n{",
             "static int g5r_upstream_main(int argc, char *argv[])\n{",
             "wrapper main")
    s += r"""

/* Top-level guard catches ordinary returns that previously fell back to XMB.
 * The first screen proves the patched Neutrino ELF was actually entered.
 * If XMB appears with no G5R screen, investigate OSDMenu/ELF handoff.
 */
int main(int argc, char *argv[])
{
    g5_stage = 1;
    g5_note(1, "NEUTRINO ENTRY before ROMVER", argc);
    int rc = g5r_upstream_main(argc, argv);
    g5_stop(298, "Neutrino returned instead of EE", rc);
    return rc;
}
"""
    if s.count("g5_stop(199,") != 1 or s.count("g5_stop(298,") != 1:
        raise ValueError("G5R critical exit guards lost")
    if s.count('sceCdApplySCmd(0x29, in, 1, out)') != 1:
        raise ValueError("G5R altered baseline mode0 SCMD")
    if "SifCallRpc(&clientSCmd, 0x2D" in s:
        raise ValueError("G4 ChangeSys not allowed in G5R")
    return s


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("main_c",type=Path)
    p.add_argument("makefile",type=Path)
    a=p.parse_args()
    original=a.main_c.read_text(encoding="utf-8")
    patched=patch(original)
    m0=a.makefile.read_text(encoding="utf-8")
    m1=patch_makefile(m0)
    a.main_c.write_text(patched,encoding="utf-8")
    a.makefile.write_text(m1,encoding="utf-8")
    print("G5R: entry display, early checkpoints, trapped return code + last stage")
    print("G5R: GS init once; ISO/EE handoff stage199 intentionally HALTS")
    print("No extra IOP resets, no partition/HDD/XMB writes.")
    print("".join(difflib.unified_diff(original.splitlines(True),patched.splitlines(True),
                                   fromfile="pinned Neutrino v1.0",tofile="G5R diagnostic")))
