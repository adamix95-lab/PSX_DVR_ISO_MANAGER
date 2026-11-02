/*
 * PSX DVR ISO Manager v1.0 / Diagnostic C:
 * Isolate fileXio RPC startup after the physical DESR-7000 test froze at
 * "STEP 2 fileXioInit: BEFORE" when inheriting wLaunchELF R3Z's IOP.
 *
 * This is a VOLATILE IOP REINITIALIZATION TEST, not a PFS test:
 *   reset IOP -> patch -> embedded iomanX -> embedded fileXio -> fileXioInit.
 * No HDD driver, partition mount, open, filesystem write or XMB installer.
 * Uses the known-good v0.8 initialization order, but requires physical testing.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <unistd.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

extern unsigned char iomanX_irx[] __attribute__((aligned(16)));
extern uint32_t size_iomanX_irx;
extern unsigned char fileXio_irx[] __attribute__((aligned(16)));
extern uint32_t size_fileXio_irx;

static int start_module(const char *name, void *module, unsigned size)
{
    int irx_result = 0;
    int id;

    scr_printf("%s: BEFORE\n", name);
    sleep(2);
    id = SifExecModuleBuffer(module, size, 0, NULL, &irx_result);
    scr_printf("%s: id=%d result=%d\n", name, id, irx_result);
    if (id < 0)
        return id;
    if (irx_result < 0)
        return irx_result;
    return 0;
}

int main(void)
{
    int result;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR HDD DIAG C / IOP FILEXIO\n");
    scr_printf("NO HDD ACCESS. NO DISK WRITES.\n");
    scr_printf("TEST: standalone IOP/RPC stack\n");

    scr_printf("STEP C1 IOP RESET: BEFORE\n");
    sleep(2);
    while (!SifIopReset("", 0)) {}
    scr_printf("STEP C1 IOP RESET: SENT\n");

    scr_printf("STEP C2 IOP SYNC: BEFORE\n");
    sleep(2);
    while (!SifIopSync()) {}
    scr_printf("STEP C2 IOP SYNC: DONE\n");

    scr_printf("STEP C3 RPC+PATCHES: BEFORE\n");
    sleep(2);
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    scr_printf("STEP C3 RPC+PATCHES: DONE\n");

    result = start_module("STEP C4 iomanX", iomanX_irx, size_iomanX_irx);
    if (result < 0)
        goto done;
    result = start_module("STEP C5 fileXio", fileXio_irx, size_fileXio_irx);
    if (result < 0)
        goto done;

    scr_printf("STEP C6 fileXioInit: BEFORE\n");
    sleep(2);
    result = fileXioInit();
    scr_printf("STEP C6 fileXioInit=%d\n", result);

done:
    scr_printf("STOP C: result=%d\n", result);
    scr_printf("NO APA/PFS/DVR TEST PERFORMED\n");
    scr_printf("Photograph screen / reboot to exit.\n");
    for (;;)
        sleep(1);
    return 0;
}
