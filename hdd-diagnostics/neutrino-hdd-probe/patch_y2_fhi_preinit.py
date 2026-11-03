#!/usr/bin/env python3
"""DIAG Y2: repair DIAG Y's false -88 due to fhi_config_init() zeroing magic.

Applied only to a pinned, ephemeral pcm720/neutrino source checkout after
patch_probe.py, patch_w_entry_hold.py, patch_x_dvr_le.py, patch_y_fhi.py.
Captures every candidate's settings pointer BEFORE upstream fhi_config_init(),
then verifies actual selected module. Never opens ISO / executes game.
"""
from pathlib import Path
import sys

main_path, fhi_path = map(Path, sys.argv[1:3])
s = main_path.read_text("utf-8")
f = fhi_path.read_text("utf-8")

def once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"Y2 refuses unsafe patch ({label}): anchor count {count}")
    return text.replace(old, new)

s = once(s,
'''    v_stage = 86;
    scr_printf("Y86 FHI CONFIG INIT BEGIN\\n");
    int fhi_init_result = fhi_config_init(&drv.mod);''',
'''    v_stage = 86;
    scr_printf("Y2 86 BACKEND PRE-SCAN BEGIN\\n");
    /* IMPORTANT: fhi_config_init() memset()s the chosen settings buffer,
     * including MODULE_SETTINGS_MAGIC. Re-scanning it AFTER init is invalid.
     * Preserve the original four settings-presence results BEFORE init. */
    static const char *y2_backend_names[4] = {
        "fhi_bd.irx", "mmcefhi.irx", "udpfs_fhi.irx", "dvrfile_fhi.irx"
    };
    int y2_pre_settings[4] = {0, 0, 0, 0};
    int y2_chosen = -1;
    int y2_i;
    for (y2_i = 0; y2_i < 4; ++y2_i) {
        struct SModule *m = modlist_get_by_name(&drv.mod, y2_backend_names[y2_i]);
        void *ptr = module_get_settings(m);
        y2_pre_settings[y2_i] = ptr != NULL;
        if (ptr && y2_chosen < 0) y2_chosen = y2_i;
        scr_printf("Y2 PRE %d MOD=%d BYTES=%d SET=%d\\n",
                   y2_i, m != NULL, m != NULL ? (int)m->iSize : -1,
                   ptr != NULL);
    }
    scr_printf("Y2 EXPECT SELECT IDX=%d (DVR=3)\\n", y2_chosen);
    scr_printf("Y2 86 FHI CONFIG INIT BEGIN\\n");
    int fhi_init_result = fhi_config_init(&drv.mod);''',
"pre-scan")

s = once(s,
'''    struct SModule *y_dvr_fhi = modlist_get_by_name(&drv.mod, "dvrfile_fhi.irx");
    void *y_dvr_fhi_settings = module_get_settings(y_dvr_fhi);
    scr_printf("Y87 DVR FHI MOD=%d SET=%d\\n",
               y_dvr_fhi != NULL, y_dvr_fhi_settings != NULL);
    if (!fhi_active || y_dvr_fhi == NULL || y_dvr_fhi_settings == NULL) {
        v_stage = 88;
        scr_printf("Y88 ERROR DVR FHI MISSING\\n");
        return -88;
    }
    /* fhi_config_init searches BD/MMCE/UDPFS before DVR.
     * Refuse a misleading PASS if any higher-priority backend is present.
     */
    if (module_get_settings(modlist_get_by_name(&drv.mod, "fhi_bd.irx")) ||
        module_get_settings(modlist_get_by_name(&drv.mod, "mmcefhi.irx")) ||
        module_get_settings(modlist_get_by_name(&drv.mod, "udpfs_fhi.irx"))) {
        v_stage = 89;
        scr_printf("Y89 ERROR OTHER FHI HAS PRIORITY\\n");
        return -89;
    }''',
'''    /* Rescanning the selected settings after fhi_config_init() can return
     * NULL by design, because init() memset() cleared MODULE_SETTINGS_MAGIC.
     */
    int y2_dvr_after = module_get_settings(
        modlist_get_by_name(&drv.mod, "dvrfile_fhi.irx")) != NULL;
    scr_printf("Y2 87 DVR PRE=%d POST=%d CHOSEN=%d\\n",
               y2_pre_settings[3], y2_dvr_after, y2_chosen);
    if (!fhi_active || y2_chosen != 3 || y2_pre_settings[3] != 1) {
        v_stage = 88;
        scr_printf("Y2 88 WRONG FHI BACKEND / STOP\\n");
        return -88;
    }
    scr_printf("Y2 89 CORRECT DVR FHI BACKEND\\n");''',
"repair false-negative backend check")

s = once(s, "PSX DVR NEUTRINO Y / FHI TEST", "PSX DVR NEUTRINO Y2 / FHI TEST", "banner")
s = once(s, "Y95 DVR FHI+CDVD READY / SAFE STOP", "Y2 95 DVR FHI+CDVD READY / SAFE STOP", "success")

# Add a marker inside the ACTUAL upstream backend selection loop, for proof
# that the chosen backend equals the captured pre-scan index.
f = once(f, '#include <string.h>', '#include <string.h>\n#include <debug.h>', "debug include")
f = once(f,
'''        if (!s)
            continue;
        memset(s, 0, candidates[i].settings_size);''',
'''        if (!s)
            continue;
        scr_printf("Y2 ACTUAL SELECT idx=%d %s\\n", i, candidates[i].filename);
        /* This zeroes the magic used by module_get_settings(): expected! */
        memset(s, 0, candidates[i].settings_size);''',
"actual selected FHI backend")

# Guardrails for a diagnostic that must NEVER access the dummy ISO.
hard_stop = s.index("return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */")
setup_call = s.index("if (setup_dvd_iso(sDVDFile")
game_call = s.find("ExecPS2(")
if not (hard_stop < setup_call and (game_call < 0 or hard_stop < game_call)):
    raise SystemExit("Y2 refused: unsafe ordering of ISO/game hard stop")
if 'strcmp(argv[2], "-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso")' not in s:
    raise SystemExit("Y2 refused: missing argv allowlist")
if 'fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)' not in s:
    raise SystemExit("Y2 refused: missing RO PFS mount")
if "Y88 ERROR DVR FHI MISSING" in s:
    raise SystemExit("Y2 refused: false-positive Y88 check still present")

main_path.write_text(s, encoding="utf-8")
fhi_path.write_text(f, encoding="utf-8")
print("Y2: pre-scan all four FHI backends; real select trace; no ISO/game hard stop.")
