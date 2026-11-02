/*
 * PSX DVR ISO MANAGER v1.0 / DIAG F — START exit-to-XMB proof.
 *
 * DESR-7000 verified: DIAG E mounts PP.TESTDVR RO while DVR ISO is readable.
 * This minimal follow-up proves the requested controller-based exit BEFORE
 * any subsequent ELF embeds the same path into APA/PFS + DVR diagnostics.
 *
 * No HDD driver, HDD mount, filesystem accesses or writes. No XMB install.
 * This uses the same volatile IOP reset / fileXio + pad setup and START-exit
 * pattern as the physically tested Manager v0.8. Whether returning from this
 * standalone ELF reaches XMB/OSD needs separate on-hardware confirmation.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
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

int main(void)
{
    int rv;
    int bits;
    int prev_buttons = 0;
    int pad_sampled = 0;
    int new_press;
    struct padButtonStatus btn;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR v1.0 DIAG F / PAD EXIT\n");
    scr_printf("NO HDD ACCESS / NO DISK WRITES\n");

    before("F1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    scr_printf("F1 RESET SENT\n");
    before("F2 IOP SYNC");
    while (!SifIopSync()) {}
    scr_printf("F2 IOP SYNC DONE\n");
    before("F3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    scr_printf("F3 RPC PATCH DONE\n");

#define LOAD(name, label) \
    do { \
        rv = load_module(label, name##_irx, size_##name##_irx); \
        if (rv < 0) goto failure; \
    } while (0)
    LOAD(iomanX, "F4 iomanX");
    LOAD(fileXio, "F5 fileXio");
#undef LOAD

    before("F6 fileXioInit");
    rv = fileXioInit();
    scr_printf("F6 fileXioInit=%d\n", rv);
    if (rv < 0) goto failure;

#define LOAD(name, label) \
    do { \
        rv = load_module(label, name##_irx, size_##name##_irx); \
        if (rv < 0) goto failure; \
    } while (0)
    LOAD(sio2man, "F7 sio2man");
    LOAD(mcman, "F8 mcman");
    LOAD(mcserv, "F9 mcserv");
    LOAD(freepad, "F10 freepad");
#undef LOAD

    before("F11 padInit");
    rv = padInit(0);
    scr_printf("F11 padInit=%d\n", rv);
    if (!rv) goto failure;

    before("F12 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("F12 padPortOpen=%d\n", rv);
    if (!rv) {
        padEnd();
        goto failure;
    }

    scr_clear();
    scr_printf("PSX DVR HDD DIAG F / EXIT TEST\n");
    scr_printf("PAD INITIALIZED, NO HDD ACCESSED\n");
    scr_printf("PRESS START: EXIT TO XMB/OSD\n");
    scr_printf("Same exit pattern as Manager v0.8\n");
    scr_printf("Take a photo before pressing START.\n");
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
    scr_printf("F FAIL rv=%d / last stage above\n", rv);
    scr_printf("PAD NOT READY: photo / reboot\n");
    for (;;)
        sleep(1);
    return 1;
}
