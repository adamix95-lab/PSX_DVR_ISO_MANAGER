/*
 * PSX DVR DIAG R_ARG: harmless ELF argument-receipt test.
 * Launched by ORIGINAL OSDMenu via the isolated DIAG_R.CNF.
 * Uses DIAG F's proven pad/IOP exit path. No APA/DVR/HDD modules or writes.
 * Never attempts to open or execute its dummy DVD argument.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

#define IRX(name) \
    extern unsigned char name##_irx[] __attribute__((aligned(16))); \
    extern uint32_t size_##name##_irx

IRX(iomanX);
IRX(fileXio);
IRX(sio2man);
IRX(mcman);
IRX(mcserv);
IRX(freepad);

static unsigned char padbuf[256] __attribute__((aligned(64)));
#define EXPECTED_DVD "-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso"
static char saved_argv0[128];
static char saved_argv1[64];
static char saved_argv2[160];
static int arguments_valid;
static void save_arg(char *dst, size_t cap, const char *arg)
{
    if (!arg) arg = "(null)";
    snprintf(dst, cap, "%s", arg);
}

static void before(const char *label)
{
    scr_printf("%s BEFORE\n", label);
    sleep(2);
}

static int load_module(const char *label, void *data, unsigned size)
{
    int id, rv = -999;
    before(label);
    id = SifExecModuleBuffer(data, size, 0, NULL, &rv);
    scr_printf("%s id=%d rv=%d\n", label, id, rv);
    return id < 0 ? id : rv;
}

int main(int argc, char *argv[])
{
    int rv;
    int bits;
    int prev_buttons = 0;
    int pad_sampled = 0;
    int new_press;
    struct padButtonStatus btn;

    /* Snapshot EE argv before any IOP state changes. No file access. */
    save_arg(saved_argv0, sizeof(saved_argv0), argc > 0 && argv ? argv[0] : NULL);
    save_arg(saved_argv1, sizeof(saved_argv1), argc > 1 && argv ? argv[1] : NULL);
    save_arg(saved_argv2, sizeof(saved_argv2), argc > 2 && argv ? argv[2] : NULL);
    arguments_valid = argc == 3 &&
        strcmp(saved_argv1, "-bsd=dvr") == 0 &&
        strcmp(saved_argv2, EXPECTED_DVD) == 0;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR DIAG R / ARGV TARGET\n");
    scr_printf("ENTRY: ARGC=%d ARGS_OK=%d\n", argc, arguments_valid);
    scr_printf("NO HDD ACCESS / NO GAME START\n");

    before("R1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    scr_printf("R1 RESET SENT\n");
    before("R2 IOP SYNC");
    while (!SifIopSync()) {}
    scr_printf("R2 IOP SYNC DONE\n");
    before("R3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    scr_printf("R3 RPC PATCH DONE\n");

#define LOAD(name, label) \
    do { \
        rv = load_module(label, name##_irx, size_##name##_irx); \
        if (rv < 0) goto failure; \
    } while (0)
    LOAD(iomanX, "R4 iomanX");
    LOAD(fileXio, "R5 fileXio");
#undef LOAD

    before("R6 fileXioInit");
    rv = fileXioInit();
    scr_printf("R6 fileXioInit=%d\n", rv);
    if (rv < 0) goto failure;

#define LOAD(name, label) \
    do { \
        rv = load_module(label, name##_irx, size_##name##_irx); \
        if (rv < 0) goto failure; \
    } while (0)
    LOAD(sio2man, "R7 sio2man");
    LOAD(mcman, "R8 mcman");
    LOAD(mcserv, "R9 mcserv");
    LOAD(freepad, "R10 freepad");
#undef LOAD

    before("R11 padInit");
    rv = padInit(0);
    scr_printf("R11 padInit=%d\n", rv);
    if (!rv) goto failure;

    before("R12 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("R12 padPortOpen=%d\n", rv);
    if (!rv) {
        padEnd();
        goto failure;
    }

    scr_clear();
    scr_printf("PSX DVR HDD DIAG R / ARG RESULTS\n");
    scr_printf("ARGC=%d ARGS_OK=%d\n", argc, arguments_valid);
    scr_printf("ARGV0: %.53s\n", saved_argv0);
    scr_printf("ARGV1: %.53s\n", saved_argv1);
    scr_printf("ARGV2: %.61s\n", saved_argv2);
    scr_printf("DUMMY ISO PATH - NOT OPENED.\n");
    scr_printf("START: EXIT TO XMB/OSD\n");
    scr_printf("PHOTO RESULTS.\n");
    /* Mirror Manager v0.8: edge-trigger button polling + return from main. */
    for (;;) {
        if (padRead(0, 0, &btn) > 0) {
            bits = 0xffff ^ btn.btns;
            new_press = bits & ~prev_buttons;
            prev_buttons = bits;
            if (!pad_sampled) {
                pad_sampled = 1;
                new_press = 0; /* Ignore held START when entering screen. */
            }
            if (new_press & PAD_START)
                break;
        }
        usleep(20000);
    }
    padPortClose(0, 0);
    padEnd();
    return 0;

failure:
    scr_printf("R FAIL rv=%d / last stage above\n", rv);
    scr_printf("PAD NOT READY: photo / reboot\n");
    for (;;)
        sleep(1);
    return 1;
}
