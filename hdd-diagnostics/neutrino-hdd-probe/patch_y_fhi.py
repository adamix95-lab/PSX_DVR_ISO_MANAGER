#!/usr/bin/env python3
"""DIAG Y: extend physically verified X LE to FHI backend validation.

Run ONLY after patch_probe.py + patch_w_entry_hold.py + patch_x_dvr_le.py
on a pinned pcm720/neutrino checkout. Keep the artificial dummy ISO.
Call fork's real fhi_config_init, verify DVR FHI settings + CDVD emulator,
then hard-return BEFORE setup_dvd_iso(), any ISO open or game boot.
No production/USB/stable software is modified.
"""
from pathlib import Path
import sys

p=Path(sys.argv[1])
s=p.read_text("utf-8")

def once(old,new):
    global s
    count=s.count(old)
    if count != 1:
        raise SystemExit(f"DIAG Y refused: expected unique anchor, found {count}: {old[:95]!r}")
    s=s.replace(old,new)

once('''    v_stage = 85;
    scr_printf("X85 LE MODULES DONE / STOP\\n");
    return 0; /* SAFE HARD STOP BEFORE FHI, ISO and GAME */

    // Load EE_CORE settings''',
'''    v_stage = 85;
    scr_printf("Y85 LE MODULES READY / FHI NEXT\\n");

    // Load EE_CORE settings''')

once('''    int fhi_active = (fhi_config_init(&drv.mod) == 0);''',
'''    v_stage = 86;
    scr_printf("Y86 FHI CONFIG INIT BEGIN\\n");
    int fhi_init_result = fhi_config_init(&drv.mod);
    int fhi_active = (fhi_init_result == 0);
    v_stage = 87;
    scr_printf("Y87 FHI ACTIVE=%d INIT=%d\\n", fhi_active, fhi_init_result);
    struct SModule *y_dvr_fhi = modlist_get_by_name(&drv.mod, "dvrfile_fhi.irx");
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
    }''')

once('''    struct cdvdman_settings_common *set_cdvdman = module_get_settings(modlist_get_by_name(&drv.mod, "cdvdman_emu.irx"));''',
'''    struct cdvdman_settings_common *set_cdvdman = module_get_settings(modlist_get_by_name(&drv.mod, "cdvdman_emu.irx"));
    v_stage = 90;
    scr_printf("Y90 CDVDMAN SETTINGS=%d\\n", set_cdvdman != NULL);
    if (set_cdvdman == NULL)
        return -90;''')

once('''        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0)
            return -1;''',
'''        /* NOTE: checks below occur only AFTER real FHI backend selection,
         * actual CDVDMAN settings resolution and normal Neutrino safeguards.
         * Never open even the dummy ISO in this diagnostic.
         */
        v_stage = 95;
        if (strcmp(sDVDFile, "dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso") != 0) {
            scr_printf("Y95 REJECT UNEXPECTED ISO PATH\\n");
            return -95;
        }
        scr_printf("Y95 DVR FHI+CDVD READY / SAFE STOP\\n");
        return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */

        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0)
            return -1;''')

once("PSX DVR NEUTRINO X / DVR LE TEST","PSX DVR NEUTRINO Y / FHI TEST")
once("X00 MAIN REACHED: CPU LOOP","Y00 MAIN REACHED: CPU LOOP")
once("X01 ENTRY HOLD COMPLETED","Y01 ENTRY HOLD COMPLETED")
once("X90 CPU LOOP, PHOTO","Y99 CPU LOOP, PHOTO")

hard_stop = s.index("return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */")
iso_call = s.index("if (setup_dvd_iso(sDVDFile")
game_code = s.find("ExecPS2(")
if not(hard_stop < iso_call and (game_code < 0 or hard_stop < game_code)):
    raise SystemExit("DIAG Y refused: hard stop not before ISO/game.")
if 'strcmp(argv[2], "-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso")' not in s:
    raise SystemExit("DIAG Y refused: outer entrypoint dummy ISO gate missing.")
if 'fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)' not in s:
    raise SystemExit("DIAG Y refused: RO mount gate missing.")
if 'return 0; /* SAFE HARD STOP BEFORE FHI, ISO and GAME */' in s:
    raise SystemExit("DIAG Y refused: old X stage 85 stop remains.")
p.write_text(s,encoding="utf-8")
print("DIAG Y: real FHI backend + CDVDMAN checked; hard stop before ISO/game")
