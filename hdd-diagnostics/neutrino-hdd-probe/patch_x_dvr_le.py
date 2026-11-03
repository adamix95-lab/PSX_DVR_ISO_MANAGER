#!/usr/bin/env python3
"""Safe-stop DIAG X extension to the already patched pcm720 Neutrino W source.

Hardware precondition: W has passed through V70 on actual DESR-7000.
This script replaces ONLY W's V70 stop with guarded, instrumented upstream
DVR BE->LE IOP transitions and LE module starts. It installs a new mandatory
return before fhi_config_init/setup_dvd_iso/game loader; dummy ISO only.
Changes are made only in an ephemeral source checkout in CI.
"""
from pathlib import Path
import sys
p=Path(sys.argv[1])
s=p.read_text("utf-8")
def once(old,new):
    global s
    n=s.count(old)
    if n!=1: raise SystemExit(f"DIAG X unsafe anchor count {n}: {old[:75]!r}")
    s=s.replace(old,new)

once('''    v_stage = 70;
    scr_printf("V70 MODULES LOADED, SAFE STOP\\n");
    return 0; /* ABSOLUTE no-game stop: before IOP reset, FHI or ISO */''',
'''    v_stage = 70;
    scr_printf("X70 MODULES IN RAM / LE NEXT\\n");
    if (strcmp(sys.sBSD, "dvr") != 0 || sys.bQuickBoot != 0) {
        scr_printf("X70 REJECT BSD / QUICKBOOT\\n");
        return -70;
    }''')

once('''        SifExitIopHeap();
        SifLoadFileExit();''',
'''        v_stage = 71;
        scr_printf("X71 LE RESET PREP\\n");
        SifExitIopHeap();
        SifLoadFileExit();''')

once('''            while (!SifIopReset("rom0:UDNL rom0:OSDCNF", 0)){};
            while (!SifIopSync()){};''',
'''            v_stage = 72;
            scr_printf("X72 PSX OSDCNF RESET BEGIN\\n");
            while (!SifIopReset("rom0:UDNL rom0:OSDCNF", 0)){};
            while (!SifIopSync()){};
            v_stage = 73;
            scr_printf("X73 OSDCNF RESET SYNC OK\\n");''')

once('''            sceCdInit(SCECdINoD);
            uint8_t in[4] = {0};''',
'''            v_stage = 74;
            scr_printf("X74 CDVD INIT + SCMD BEGIN\\n");
            sceCdInit(SCECdINoD);
            uint8_t in[4] = {0};''')

once('''            sceCdApplySCmd(0x29, in, 4, out);
            sceCdInit(SCECdEXIT);''',
'''            sceCdApplySCmd(0x29, in, 4, out);
            sceCdInit(SCECdEXIT);
            v_stage = 75;
            scr_printf("X75 DVR SCMD COMPLETED\\n");''')

once('''        while(!SifIopReset("", 0)){};
        while(!SifIopSync()) {};''',
'''        v_stage = 76;
        scr_printf("X76 CLEAN IOP RESET BEGIN\\n");
        while(!SifIopReset("", 0)){};
        while(!SifIopSync()) {};
        v_stage = 77;
        scr_printf("X77 CLEAN RESET SYNC OK\\n");''')

once('''            if (pm->env & MOD_ENV_LE) {
                if (module_start(pm) < 0)
                    return -1;
            }''',
'''            if (pm->env & MOD_ENV_LE) {
                int x_module_status;
                v_stage = 78;
                scr_printf("X78 LE MODULE START idx=%d\\n", i);
                x_module_status = module_start(pm);
                scr_printf("X79 LE MODULE idx=%d rv=%d\\n", i, x_module_status);
                if (x_module_status < 0)
                    return -1;
            }''')

once('''    // Load EE_CORE settings
    struct ee_core_data *set_ee_core = module_get_settings(&mod_ee_core);''',
'''    /* SAFE HARD STOP: LE modules have been started as in original Neutrino.
     * Do NOT continue into fhi_config_init / setup_dvd_iso / game startup.
     */
    v_stage = 85;
    scr_printf("X85 LE MODULES DONE / STOP\\n");
    return 0; /* SAFE HARD STOP BEFORE FHI, ISO and GAME */

    // Load EE_CORE settings
    struct ee_core_data *set_ee_core = module_get_settings(&mod_ee_core);''')

once("PSX DVR NEUTRINO V / HDD PROBE", "PSX DVR NEUTRINO X / DVR LE TEST")
once("W00 MAIN REACHED: CPU LOOP", "X00 MAIN REACHED: CPU LOOP")
once("W01 ENTRY HOLD COMPLETED", "X01 ENTRY HOLD COMPLETED")
once("W90 CPU LOOP, PHOTO", "X90 CPU LOOP, PHOTO")

assert s.index('return 0; /* SAFE HARD STOP BEFORE FHI, ISO and GAME */') < s.index('fhi_config_init(&drv.mod)'), "hard stop too late"
assert s.index('return 0; /* SAFE HARD STOP BEFORE FHI, ISO and GAME */') < s.index('setup_dvd_iso(sDVDFile'), "hard stop too late"
if "DISABLE_EXTRA_TIMERS_FUNCTIONS()" not in s:
    raise SystemExit("X base diverged: check core timer state")
p.write_text(s,encoding="utf-8")
print("Instrumented Neutrino X: DVR LE IOP stages 71-85, hard stop before FHI/ISO/game")
