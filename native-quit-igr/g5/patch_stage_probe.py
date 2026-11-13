#!/usr/bin/env python3
"""G5 DISPLAY/PHASE DIAGNOSTIC — Sony PSX DESR Neutrino DVR, USB only.

G5A (mode=0,len=1): expected functioning DVR control (as hardware G3B).
G5B (mode=1,len=1): native QUIT mode which previously black-screened (G2B).
Same instrumentation and official USB 1.0 modules in BOTH variants.

Screen checkpoints isolate: ROM/IOP reset, SCMD transport+status, second IOP
reset, LE module startup, DVR FHI availability, and ISO FD/PVD/FHI attach.
Intentionally STOPS before the actual game's EE handoff, even on success.
NO HDD or USB log writes, no new partitions, no NULD hooks or extra resets.
The test Manager separately writes its own USB-only BOOT.CNF as usual.
"""
import argparse
import difflib
from pathlib import Path

NOTICE_OLD = """            sceCdInit(SCECdINoD);
            uint8_t in[4] = {0};
            uint8_t out[16] = {};
            sceCdApplySCmd(0x29, in, 4, out);
            sceCdInit(SCECdEXIT);"""

HELPER = r"""
/*
 * G5 investigation only: visible progress survives a blocked RPC or failed
 * LE/ISO handoff. This code is NOT present in production Neutrino 1.0.
 * In this diagnostic all paths intentionally STOP before the game EE handoff.
 * No file writes, no HDD mount and no global kernel hooks are added.
 */
static int g5_cdinit = -999, g5_scmd_ret = -999, g5_scmd_status = -999;
static int g5_stage = 0;

static void g5_note(int code, const char *name, int value)
{
    g5_stage = code;
    init_scr();
    scr_setbgcolor(0);
    scr_setfontcolor(0xFFFFFF);
    scr_clear();
    scr_setCursor(0);
    scr_printf("PSX DESR NATIVE QUIT G5-VARIANT / USB ONLY\n");
    scr_printf("ISO LOAD DIAGNOSTIC - GAME WILL NOT START\n");
    scr_printf("---------------------------------------\n");
    scr_printf("STAGE: %d  %s\n", code, name);
    scr_printf("VALUE: %d (0x%08x)\n", value, (unsigned)value);
    scr_printf("CDINIT: %d  NOTICE RPC: %d\n",g5_cdinit,g5_scmd_ret);
    scr_printf("NOTICE STATUS: %d (0x%02x)\n",g5_scmd_status,
               (unsigned)g5_scmd_status & 0xff);
    scr_printf("No disk writes. Photograph last stage.\n");
    scr_printf("When frozen at END, power cycle normally.\n");
}

static void g5_stop(int code, const char *name, int value)
{
    g5_note(code, name, value);
    scr_printf("INTENTIONAL G5 STOP; NO GAME WAS LAUNCHED.\n");
    for (;;) {
        volatile unsigned long spin;
        for (spin = 0; spin < 1000000UL; ++spin)
            __asm__ __volatile__("nop");
    }
}
"""


def once(source, old, new, label):
    count = source.count(old)
    if count != 1:
        raise ValueError(f"{label}: expected one exact source anchor; found {count}")
    return source.replace(old, new)


def instrument(src: str, mode: int) -> str:
    if mode not in (0, 1):
        raise ValueError("G5 mode must be 0 or 1")
    src = once(src, "#include <sifrpc.h>",
               "#include <sifrpc.h>\n#include <debug.h>", "debug include")
    src = once(src, "void print_usage()", HELPER.replace("G5-VARIANT",f"G5{'A' if mode==0 else 'B'}")
               + "void print_usage()", "diagnostic helper")

    # First controlled stage is after all TOML/IRX files have been loaded into
    # EE, but before the Sony OSDSYS IOP reset; this limits side effects.
    src = once(src,
        """    if (sys.bQuickBoot == 0) {
        /*
         * Reboot IOP into Load Environment (LE)
         */""",
        """    if (sys.bQuickBoot == 0) {
        g5_note(10, "BE->LE about to reset IOP", 0);
        /*
         * Reboot IOP into Load Environment (LE)
         */""",
        "before ROM transition")

    # Keep exact stock number/order of IOP resets.
    src = once(src,
        """            while (!SifIopReset("rom0:UDNL rom0:OSDCNF", 0)){};
            while (!SifIopSync()){};
            SifInitRpc(0);""",
        """            g5_note(11, "Before ROM OSDCNF IOP reset", 0);
            while (!SifIopReset("rom0:UDNL rom0:OSDCNF", 0)){};
            while (!SifIopSync()){};
            SifInitRpc(0);
            g5_note(12, "ROM OSDCNF IOP ready", 1);""",
        "ROM IOP reset")

    replacement = f"""            g5_note(20, "Before NoticeGameStart", {mode});
            g5_cdinit = sceCdInit(SCECdINoD);
            if (g5_cdinit <= 0)
                g5_stop(201, "CDVD init failed", g5_cdinit);
            uint8_t in[4] = {{{mode}}};
            uint8_t out[16] = {{0xff}};
            g5_scmd_ret = sceCdApplySCmd(0x29, in, 1, out);
            g5_scmd_status = out[0];
            g5_note(21, "NoticeGameStart returned", g5_scmd_ret);
            sceCdInit(SCECdEXIT);
            g5_note(22, "CDVD closed; LE IOP next", g5_scmd_status);"""
    src = once(src, NOTICE_OLD, replacement, "Sony SCMD site")

    src = once(src,
        """        while(!SifIopReset("", 0)){};
        while(!SifIopSync()) {};
        SifInitRpc(0);""",
        """        g5_note(30, "Before LE IOP reset", 0);
        while(!SifIopReset("", 0)){};
        while(!SifIopSync()) {};
        SifInitRpc(0);
        g5_note(31, "LE IOP reset returned", 1);""",
        "second IOP reset")

    src = once(src,
        """        for (i = 0; i < drv.mod.count; i++) {
            struct SModule *pm = &drv.mod.mod[i];
            if (pm->env & MOD_ENV_LE) {
                if (module_start(pm) < 0)
                    return -1;
            }
        }""",
        """        g5_note(40, "Before LE module starts", drv.mod.count);
        for (i = 0; i < drv.mod.count; i++) {
            struct SModule *pm = &drv.mod.mod[i];
            if (pm->env & MOD_ENV_LE) {
                g5_note(41, "Starting LE module index", i);
                if (module_start(pm) < 0)
                    g5_stop(241, "LE module failed index", i);
            }
        }
        g5_note(42, "LE module startup complete", drv.mod.count);""",
        "LE module loop")

    src = once(src, """    int fhi_active = (fhi_config_init(&drv.mod) == 0);""",
        """    g5_note(50, "Before DVR FHI configuration", 0);
    int fhi_active = (fhi_config_init(&drv.mod) == 0);
    g5_note(51, "DVR FHI active (1 is good)", fhi_active);""", "FHI detection")

    src = once(src,
        """        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0)
            return -1;""",
        """        g5_note(60, "Before ISO file open/PVD/FHI", 0);
        if (setup_dvd_iso(sDVDFile, &iso_size, &layer1_lba_start) < 0)
            g5_stop(260, "ISO file setup failed", -1);
        g5_note(80, "ISO setup and FHI attach PASS", (int)(iso_size >> 20));""",
        "ISO setup caller")

    src = once(src,
        """    printf("Loading %s...\\n", path);
    for (i = 0; i < 1000; i++) {""",
        """    printf("Loading %s...\\n", path);
    g5_note(61, "ISO open() about to start", 0);
    for (i = 0; i < 1000; i++) {""",
        "ISO open entrance")

    src = once(src, """    if (fd < 0) {
        printf("Unable to open %s\\n", path);
        return -1;
    }""",
        """    if (fd < 0) {
        g5_stop(261, "ISO open FAILED", fd);
        return -1;
    }
    g5_note(62, "ISO file descriptor opened", fd);""",
        "ISO open result")

    src = once(src,
        """    *out_iso_size = lseek64(fd, 0, SEEK_END);
    printf("- size = %dMiB\\n", (int)(*out_iso_size / (1024 * 1024)));""",
        """    g5_note(63, "Before ISO seek end", fd);
    *out_iso_size = lseek64(fd, 0, SEEK_END);
    g5_note(64, "ISO length MiB", (int)(*out_iso_size >> 20));
    printf("- size = %dMiB\\n", (int)(*out_iso_size / (1024 * 1024)));""",
        "ISO size")

    src = once(src,
        """    lseek64(fd, 16 * 2048, SEEK_SET);
    if (read(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {""",
        """    g5_note(65, "Before ISO sector 16 PVD read", 16);
    lseek64(fd, 16 * 2048, SEEK_SET);
    if (read(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {""",
        "PVD read enter")

    src = once(src,
        """        printf("Unable to read ISO\\n");
        return -1;""",
        """        g5_stop(265, "ISO PVD read FAILED", -1);
        return -1;""",
        "PVD read fail")

    src = once(src,
        """        printf("File is not a valid ISO\\n");
        return -1;""",
        """        g5_stop(266, "Invalid ISO PVD", -1);
        return -1;""",
        "PVD invalid")

    src = once(src,
        """    // Get layer0 LBA size""",
        """    g5_note(66, "ISO PVD is valid", 1);

    // Get layer0 LBA size""", "PVD validated")

    src = once(src,
        """        printf("ISO invalid\\n");
        return -1;""",
        """        g5_stop(267, "ISO layer0 size read FAILED", -1);
        return -1;""",
        "ISO layer0 error")

    src = once(src,
        """    if (fhi_add_file_fd(FHI_FID_CDVD, fd, path) < 0)
        return -1;""",
        """    g5_note(68, "Before FHI add_file_fd", fd);
    if (fhi_add_file_fd(FHI_FID_CDVD, fd, path) < 0)
        g5_stop(268, "FHI add_file_fd FAILED", -1);
    g5_note(69, "FHI add_file_fd PASS", fd);""",
        "FHI attach")

    src = once(src,
        """            fd_system_cnf = read_system_cnf(sDVDFile, system_cnf_data, 128);
            if (fd_system_cnf < 0) {""",
        """            g5_note(90, "Before game SYSTEM.CNF read", 0);
            fd_system_cnf = read_system_cnf(sDVDFile, system_cnf_data, 128);
            if (fd_system_cnf < 0) {""",
        "SYSTEM.CNF read")

    src = once(src,
        """                printf("ERROR: Unable to read SYSTEM.CNF from ISO\\n");
                return -1;""",
        """                g5_stop(290, "SYSTEM.CNF read FAILED", fd_system_cnf);
                return -1;""",
        "SYSTEM.CNF fail")

    src = once(src,
        """        } else {
            // Read SYSTEM.CNF from CD/DVD""",
        """            g5_note(91, "SYSTEM.CNF read returned", fd_system_cnf);
        } else {
            // Read SYSTEM.CNF from CD/DVD""",
        "SYSTEM.CNF success")

    # Final gate: DO NOT invoke ExecPS2 or launch the actual game in G5.
    # A complete pass reaches stage 199 and can be photographed on the TV.
    src = once(src,
        """    ExecPS2((void *)eh->entry, NULL, ee_core_argc, ee_core_argv);""",
        """    g5_stop(199, "EE handoff reached; ISO/FHI PASS", ee_core_argc);
    /* G5 deliberate stop: original ExecPS2 NOT called. */
    ExecPS2((void *)eh->entry, NULL, ee_core_argc, ee_core_argv);""",
        "STOP before EE handoff")

    if src.count("g5_stop(199,") != 1:
        raise ValueError("G5 did not guard EE handoff")
    return src


def patch_makefile(mk: str) -> str:
    old = "EE_LIBS = -lfileXio -lpatches -lcdvd"
    return once(mk, old, old + " -ldebug", "libdebug link")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("main_c", type=Path)
    ap.add_argument("makefile", type=Path)
    ap.add_argument("variant", choices=("G5A","G5B"))
    args = ap.parse_args()
    mode = 0 if args.variant == "G5A" else 1
    before = args.main_c.read_text(encoding="utf-8")
    after = instrument(before, mode)
    mk = patch_makefile(args.makefile.read_text(encoding="utf-8"))
    args.main_c.write_text(after, encoding="utf-8")
    args.makefile.write_text(mk, encoding="utf-8")
    print(args.variant, "STAGE markers: 10->199 / stop before EE game")
    print("One-byte Sony 0x29 parameter mode=",mode)
    print("No HDD writes, no new IOP reset, no NULD, no extra MechaCon RPC")
    print("Patched source differs in these Neutrino files: main.c and Makefile")
    print("".join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),
                                   fromfile="Neutrino pinned DVR",tofile=args.variant)))


if __name__=="__main__":
    main()
