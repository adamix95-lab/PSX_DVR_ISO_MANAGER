#!/usr/bin/env python3
"""DIAG Z: isolated real-ISO launch after physical Y2P PASS.

Applied to ephemeral pinned pcm720/neutrino source after V, W, X, Y, Y2.
Retains the physically proven PFS RO remount and strict argv partition guard.
Removes Y2's pre-ISO stop, validates a single DVD ISO filename under
dvr_pfs0:/ISO/, and executes the fork's unmodified ISO/game startup.
No writes are added. This ELF WILL attempt to run a game.
"""
from pathlib import Path
import sys

p=Path(sys.argv[1])
s=p.read_text("utf-8")
def once(old,new,label):
    global s
    count=s.count(old)
    if count!=1: raise SystemExit(f"DIAG Z REFUSED {label}: anchor count {count}")
    s=s.replace(old,new)

once(
'''        scr_printf("Y2 95 DVR FHI+CDVD READY / SAFE STOP\\n");
        return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */''',
'''        scr_printf("Z95 DVR FHI+CDVD READY, ISO NEXT\\n");''',
"remove stop only after FHI validation")

# Reject unknown media and malformed path at target entry before ANY IOP RPC.
# Filename can have ordinary spaces, brackets etc. Path separators and
# directory traversal are forbidden; do not allow a different device/partition.
validator=r'''
static int z_allowed_iso_arg(const char *a)
{
    static const char prefix[] = "-dvd=dvr_pfs0:/ISO/";
    size_t n, i, off = sizeof(prefix) - 1;
    if (!a || strncmp(a, prefix, off) != 0)
        return 0;
    n = strlen(a);
    if (n <= off + 4 || n > off + 95)
        return 0;
    if (strcmp(a+n-4, ".iso") != 0 && strcmp(a+n-4, ".ISO") != 0)
        return 0;
    for (i=off; i<n; ++i) {
        unsigned char c=(unsigned char)a[i];
        if (c < 0x20 || c > 0x7e || c == '/' || c == '\\' || c == ':')
            return 0;
    }
    if (strstr(a+off, "..") || strchr(a+off,'='))
        return 0;
    return 1;
}

'''
once('int main(int argc, char *argv[])\n{\n    int init_ret',
     validator+'int main(int argc, char *argv[])\n{\n    int init_ret',
     "insert filename allowlist")

once('''        strcmp(argv[2], "-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso") != 0) {''',
     '''        !z_allowed_iso_arg(argv[2])) {''',"replace dummy ISO guard")

once('    scr_printf("PSX DVR NEUTRINO Y2 / FHI TEST\\n");',
     '    scr_printf("PSX DVR NEUTRINO Z / ISO LAUNCH\\n");', "Z banner")
once('    scr_printf("NO ISO OR GAME. READ ONLY.\\n");',
     '    scr_printf("Z TEST: REAL ISO / REAL GAME.\\n");', "visible warning")
# DIAG Y had an intentionally fixed DUMMY ISO allowlist. Y2P never
# reached real ISO setup; Z removes that dummy guard while retaining
# the exact dynamic argv[2] path validation that the launcher verified.
# In upstream Neutrino, sDVDFile = sys.sDVDMode = argv[2]+5 after
# parse_cmdline_args(); check it matches exactly rather than accepting
# an unrelated path or silently skipping safety checks.
once('''        if (strcmp(sDVDFile, "dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso") != 0) {
            scr_printf("Y95 REJECT UNEXPECTED ISO PATH\n");
            return -95;
        }''',
'''        if (argc != 3 || !argv || !argv[2] ||
            !z_allowed_iso_arg(argv[2]) ||
            strcmp(sDVDFile, argv[2] + 5) != 0) {
            scr_printf("Z95 REJECT ISO ARG / MODE MISMATCH\n");
            return -95;
        }
        scr_printf("Z95 EXACT ISO ARG: %.88s\n", sDVDFile);''',
"replace leftover Y dummy-only ISO path check")

once('''        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0)
            return -1;''',
'''        v_stage=96;
        scr_printf("Z96 setup_dvd_iso BEGIN\\n");
        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0) {
            scr_printf("Z96 ISO OPEN/SETUP FAILED\\n");
            return -96;
        }
        v_stage=97;
        scr_printf("Z97 ISO SETUP OK SECTORS=%llu\\n", (unsigned long long)iso_size);''',
"ISO opening stage")

once('''            fd_system_cnf = read_system_cnf(sDVDFile, system_cnf_data, 128);''',
'''            v_stage=98;
            scr_printf("Z98 SYSTEM.CNF ISO READ BEGIN\\n");
            fd_system_cnf = read_system_cnf(sDVDFile, system_cnf_data, 128);
            scr_printf("Z98 SYSTEM.CNF FD=%d\\n", fd_system_cnf);''',
"game SYSTEM.CNF read stage")

once('''    ExecPS2((void *)eh->entry, NULL, ee_core_argc, ee_core_argv);''',
'''    v_stage=99;
    scr_printf("Z99 GAME HANDOFF BEGIN\\n");
    ExecPS2((void *)eh->entry, NULL, ee_core_argc, ee_core_argv);
    scr_printf("Z99 ExecPS2 RETURNED\\n");''',
"actual game handoff marker")

# Diagnostic fallback is executed only if a game is not launched or returns.
once('    scr_printf("NO GAME/ISO EXECUTED.\\n");',
     '    scr_printf("Z: GAME NOT RUNNING / RETURNED.\\n");',
     "fallback result")
once('    scr_printf("W90 CPU LOOP, PHOTO\\n");' if "W90 CPU LOOP, PHOTO" in s else
     '    scr_printf("Y99 CPU LOOP, PHOTO\\n");',
     '    scr_printf("Z90 CPU LOOP, PHOTO\\n");', "fallback screen")

for expected in ('z_allowed_iso_arg(argv[2])',
                 'strcmp(sDVDFile, argv[2] + 5) != 0',
                 'Z95 EXACT ISO ARG:',
                 'scr_printf("Z96 setup_dvd_iso BEGIN',
                 'scr_printf("Z99 GAME HANDOFF BEGIN',
                 'fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)',
                 'Y2 87 DVR PRE=',
                 'Z95 DVR FHI+CDVD READY, ISO NEXT'):
    if expected not in s: raise SystemExit("DIAG Z REFUSED missing "+expected)
if 'Y95 REJECT UNEXPECTED ISO PATH' in s or
   'strcmp(sDVDFile, "dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso")' in s:
    raise SystemExit("DIAG Z REFUSED leftover dummy-only Y95 gate")
if "return 0; /* HARD NO-ISO STOP BEFORE setup_dvd_iso */" in s:
    raise SystemExit("DIAG Z REFUSED old stop still present")
p.write_text(s,encoding="utf-8")
print("DIAG Z patched: real ISO/game after real FHI, mount only PP.TESTDVR RO")
