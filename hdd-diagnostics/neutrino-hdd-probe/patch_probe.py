#!/usr/bin/env python3
"""Build-time ONLY patch for pcm720/neutrino bsd-dvr at pinned commit.

Diagnostic V: mount PP.TESTDVR read-only on target entry, change cwd, then
run genuine fork Neutrino config and module loading, and STOP *before*
IOP mode switching, FHI game ISO access or ExecPS2. No ISO opened.
It is NOT a production Neutrino change.
"""
from pathlib import Path
import sys

SOURCE = Path(sys.argv[1])
MAKEFILE = Path(sys.argv[2])
s = SOURCE.read_text("utf-8")
mk = MAKEFILE.read_text("utf-8")

def once(old: str, new: str):
    global s
    if s.count(old) != 1:
        raise SystemExit(f"Refusing patch: anchor count {s.count(old)} for {old[:100]!r}")
    s = s.replace(old, new)

once("#include <sifrpc.h>", "#include <sifrpc.h>\n#include <debug.h>")

# The machine and partition are deliberately fixed to the isolated test
# partition, so this build cannot mount or scan Sony system partitions.
once("int main(int argc, char *argv[])\n{",
'''static volatile int v_stage = 0;
static int neutrino_probe_main(int argc, char *argv[])
{''')

once('    printf("--------------------------------\\\\n");\n    printf("- Neutrino PS2 Device Emulator\\\\n");',
'''    v_stage = 10; /* Neutrino started; ROMVER next */
    scr_printf("V10 Neutrino core entered\\\\n");
    printf("--------------------------------\\\\n");
    printf("- Neutrino PS2 Device Emulator\\\\n");''')

once('    /*\n     * Initialize structures before filling them from config files\n     */',
'''    v_stage = 20; /* ROMVER OK */
    scr_printf("V20 ROMVER OK\\\\n");
    /*
     * Initialize structures before filling them from config files
     */''')

once('    if (load_config_file("system", NULL) < 0) {',
'''    v_stage = 30;
    scr_printf("V30 LOAD SYSTEM BEGIN\\\\n");
    if (load_config_file("system", NULL) < 0) {''')

once('        printf("ERROR: failed to load system settings\\\\n");\n        return -1;\n    }',
'''        printf("ERROR: failed to load system settings\\\\n");
        return -1;
    }
    v_stage = 40;
    scr_printf("V40 SYSTEM OK\\\\n");''')

once('    if (parse_cmdline_args(argc, argv, &iELFArgcStart) < 0)',
'''    v_stage = 45;
    scr_printf("V45 PARSE ARGS\\\\n");
    if (parse_cmdline_args(argc, argv, &iELFArgcStart) < 0)''')

once('    /*\n     * Load backing store driver settings\n     */',
'''    v_stage = 50;
    scr_printf("V50 DRIVER CONFIGS NEXT\\\\n");
    /*
     * Load backing store driver settings
     */''')

once('    /*\n     * Load all needed files before rebooting the IOP\n     */',
'''    v_stage = 60;
    scr_printf("V60 LOAD EE_CORE + IRX\\\\n");
    /*
     * Load all needed files before rebooting the IOP
     */''')

# Critical guard: stops before any IOP mode switch or even file-backed ISO.
# Neutrino genuinely loads config and modules from read-only HDD,
# but may NOT execute game/ISO in this diagnostic artifact.
once('    if (modlist_load(&drv.mod, (sys.bQuickBoot == 0) ? (MOD_ENV_LE | MOD_ENV_EE) : MOD_ENV_EE) < 0) {\n        printf("ERROR: failed to load drv.mod\\\\n");\n        return -1;\n    }',
'''    if (modlist_load(&drv.mod, (sys.bQuickBoot == 0) ? (MOD_ENV_LE | MOD_ENV_EE) : MOD_ENV_EE) < 0) {
        printf("ERROR: failed to load drv.mod\\\\n");
        return -1;
    }
    v_stage = 70;
    scr_printf("V70 MODULES LOADED, SAFE STOP\\\\n");
    return 0; /* ABSOLUTE no-game stop: before IOP reset, FHI or ISO */''')

s += r'''

/* Only the probe wrapper is the real entry point. Do NOT enable for normal
 * Neutrino. This bypasses OSDMenu's prior pfs0 unmount on a test partition
 * and proves exactly how far config/module loading proceeds.
 */
int main(int argc, char *argv[])
{
    int init_ret, mount_ret, chdir_ret, rv = -999;
    int config_fd, irx_fd;
    int own_mount = 0;
    const char *allowed = "hdd0:PP.TESTDVR:pfs:/";

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0);
    scr_clear();
    scr_printf("PSX DVR NEUTRINO V / HDD PROBE\n");
    scr_printf("NO ISO OR GAME. READ ONLY.\n");

    if (argc != 3 || !argv || !argv[0] ||
        strncmp(argv[0], allowed, strlen(allowed)) != 0 ||
        !argv[1] || strcmp(argv[1], "-bsd=dvr") != 0 ||
        !argv[2] ||
        strcmp(argv[2], "-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso") != 0) {
        scr_printf("V00 ARGS INVALID / NO ACTION\n");
        rv = -100;
        goto finish;
    }
    scr_printf("V00 ARGC=3 ARGS_OK=1\n");

    SifInitRpc(0);
    scr_printf("V01 fileXioInit BEGIN\n");
    init_ret = fileXioInit();
    scr_printf("V01 fileXioInit=%d\n", init_ret);
    if (init_ret < 0) { rv = -101; goto finish; }

    scr_printf("V02 MOUNT TEST PFS0 RO BEGIN\n");
    mount_ret = fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY);
    scr_printf("V02 MOUNT=%d\n", mount_ret);
    if (mount_ret < 0) { rv = -102; goto finish; }
    own_mount = 1;

    scr_printf("V03 CHDIR PFS0 BEGIN\n");
    chdir_ret = chdir("pfs0:/");
    scr_printf("V03 CHDIR=%d\n", chdir_ret);
    if (chdir_ret < 0) { rv = -103; goto finish; }

    scr_printf("V04 OPEN CONFIG BEGIN\n");
    config_fd = open("config/system.toml", O_RDONLY);
    scr_printf("V04 CONFIG FD=%d\n", config_fd);
    if (config_fd < 0) { rv = -104; goto finish; }
    close(config_fd);
    scr_printf("V05 OPEN DVR IRX BEGIN\n");
    irx_fd = open("modules/dvrfile_fhi.irx", O_RDONLY);
    scr_printf("V05 DVR IRX FD=%d\n", irx_fd);
    if (irx_fd < 0) { rv = -105; goto finish; }
    close(irx_fd);

    v_stage = 5;
    rv = neutrino_probe_main(argc, argv);
finish:
    scr_printf("V RESULT=%d LAST_STAGE=%d\n", rv, v_stage);
    scr_printf("NO GAME/ISO EXECUTED.\n");
    scr_printf("PHOTO RESULTS. AUTO EXIT IN 45 S.\n");
    sleep(45);
    if (own_mount) {
        /* Mount is ours only; return cwd before unmount */
        chdir("hdd0:");
        fileXioUmount("pfs0:");
    }
    return 0;
}
'''

# Relies on PS2SDK's actual EE libdebug, same lib used by HDD diagnostics.
if mk.count("EE_LIBS = -lfileXio -lpatches -lcdvd") != 1:
    raise SystemExit("Makefile libs anchor missing; refuse patch")
mk = mk.replace("EE_LIBS = -lfileXio -lpatches -lcdvd",
                "EE_LIBS = -lfileXio -lpatches -lcdvd -ldebug")
SOURCE.write_text(s, encoding="utf-8")
MAKEFILE.write_text(mk, encoding="utf-8")
print("Patched pinned pcm720 Neutrino for safe HDD-only stage 70 stop")
