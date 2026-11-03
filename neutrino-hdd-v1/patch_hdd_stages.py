#!/usr/bin/env python3
"""Diagnostic Neutrino HDD: identify which upstream phase returns generic -1.

Apply after patch_hdd_runtime.py and before patch_hdd_error_screen.py.
Only tags upstream transitions; does NOT change config, ISO, FHI or loader
success branches. All stages are EE-side writes to a volatile int.
Separate from hardware-proven DIAG Z in Draft PR #26.
"""
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("Usage: patch_hdd_stages.py ee/loader/src/main.c")
path = Path(sys.argv[1])
s = path.read_text(encoding="utf-8")


def once(old: str, new: str, label: str) -> None:
    global s
    n = s.count(old)
    if n != 1:
        raise SystemExit(f"HDD stage marker REFUSED {label}, matches={n}")
    s = s.replace(old, new)


# FIX95 runtime has already renamed original Neutrino main().
once(
    "static int desr_iso_arg_allowed(const char *arg);\n\n"
    "static int neutrino_hdd_upstream_main(int argc, char *argv[])\n{",
    "/* Stage 0=entry, 10=ROMVER, 20=layout, 30=system, 40=argv,\n"
    " * 45=compat, 50=override, 55=GSM, 60=BSD, 65=DVD config,\n"
    " * 70=EE core, 72=modules, 75=IOP LE reset, 76=LE start,\n"
    " * 80=FHI, 82=DVD backend, 85=ISO setup, 87=SYSTEM.CNF,\n"
    " * 90=game config, 95=game setup, 99=game handoff.\n"
    " */\n"
    "static volatile int desr_hdd_stage = 0;\n"
    "static int desr_iso_arg_allowed(const char *arg);\n\n"
    "static int neutrino_hdd_upstream_main(int argc, char *argv[])\n{",
    "global marker",
)

anchors = [
    ("        if ((fd_ROMVER = open(\"rom0:ROMVER\", O_RDONLY)) < 0) {",
     10, "ROMVER open"),
    ("        int fd = open(\"config/system.toml\", O_RDONLY);",
     20, "config root check"),
    ("    if (load_config_file(\"system\", NULL) < 0) {",
     30, "system TOML"),
    ("    if (parse_cmdline_args(argc, argv, &iELFArgcStart) < 0)",
     40, "command-line parser"),
    ("    toml_result_t toml_compat = load_config_file_toml(\"compat\", NULL);",
     45, "compat config"),
    ("    if (sys.sCFGFile != NULL) {",
     50, "user settings"),
    ("    if (parse_gsm_flags() < 0)",
     55, "GSM validation"),
    ("        if (load_config_file(\"bsd\", sys.sBSD) < 0) {",
     60, "BSD driver config"),
    ("    } else if (load_config_file(\"emu-dvd\", sys.sDVDMode) < 0) {",
     65, "DVD emulator config"),
    ("    mod_ee_core.sFileName = sys.eecore_elf;",
     70, "EE core load"),
    ("    if (modlist_load(&drv.mod, (sys.bQuickBoot == 0) ? "
     "(MOD_ENV_LE | MOD_ENV_EE) : MOD_ENV_EE) < 0) {",
     72, "module load"),
    ("        SifExitIopHeap();",
     75, "IOP LE transition"),
    ("        /*\n"
     "         * Start load environment modules\n"
     "         */\n"
     "        for (i = 0; i < drv.mod.count; i++) {\n"
     "            struct SModule *pm = &drv.mod.mod[i];",
     76, "LE module start"),
    ("    int fhi_active = (fhi_config_init(&drv.mod) == 0);",
     80, "DVR FHI init"),
    ("    if (sDVDFile != NULL) {\n"
     "        if (!fhi_active) {",
     82, "DVD FHI required"),
    ("        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0)",
     85, "ISO open"),
    ("            fd_system_cnf = read_system_cnf(sDVDFile, system_cnf_data, 128);",
     87, "read ISO SYSTEM.CNF"),
    ("        toml_datum_t tbl_gc = toml_get(toml_compat.toptab, sGameID);",
     90, "per-game settings"),
    ("    ResetDeckardXParams();",
     95, "game setup"),
    ("    ExecPS2((void *)eh->entry, NULL, ee_core_argc, ee_core_argv);",
     99, "game handoff"),
]
for target, code, label in anchors:
    once(target, f"    desr_hdd_stage = {code}; /* {label} */\n"+target, label)

# Add stage guard to wrapper early; leave original return codes untouched.
once("    rc=neutrino_hdd_upstream_main(argc,argv);",
     "    desr_hdd_stage=1; /* wrapper successfully mounted PP.DVRISO */\n"
     "    rc=neutrino_hdd_upstream_main(argc,argv);",
     "upstream invocation")

if 'strcmp(sDVDFile, argv[2] + 5) != 0' not in s:
    raise SystemExit("FIX95 was lost in stage patch")
if 'hdd0:PP.DVRISO' not in s or 'PP.TESTDVR' in s:
    raise SystemExit("Unexpected partition in stage patch")
path.write_text(s, encoding="utf-8")
print("HDD STAGES: 20 fail-stage markers; real ISO/FHI untouched")
