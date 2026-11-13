/*
 * NATIVE QUIT MODE 1 — ultra-minimal PSX DESR probe.
 *
 * This is a standalone USB-booted experiment. Does NOT open files,
 * mount HDD/PFS, change XMB, write BOOT.CNF, load games, or reset the IOP.
 * It sends the same one-byte 0x29 / mode 1 request used by
 * PSX OSD initialization examples. The console may reboot when
 * the physical QUIT GAME button is pressed afterward.
 *
 * IMPORTANT: a failure here is inconclusive if the IOP lacks the DESR
 * ROM CDVD RPC service. A successful transport response is not
 * necessarily proof of the game return path.
 */

#include <debug.h>
#include <kernel.h>
#include <libcdvd.h>
#include <sifrpc.h>
#include <tamtypes.h>

int main(int argc, char **argv)
{
    int init_result;
    int transport = 0;
    u32 status = 0xffffffff;

    (void)argc;
    (void)argv;

    init_scr();
    scr_setCursor(0);
    scr_printf("PSX DESR QUIT / MODE 1 / USB PROBE\n");
    scr_printf("Read-only: no HDD, PFS, CNF or game changes.\n\n");

    sceSifInitRpc(0);
    init_result = sceCdInit(SCECdINoD);
    scr_printf("sceCdInit(SCECdINoD): %d\n", init_result);

    if (init_result > 0) {
        transport = sceCdNoticeGameStart(1, &status);
        scr_printf("sceCdNoticeGameStart(1): %d\n", transport);
        scr_printf("MechaCon status: 0x%08lx\n\n", (unsigned long)status);
        sceCdInit(SCECdEXIT);
    } else {
        scr_printf("No CDVD RPC. Skipping QUIT request.\n\n");
    }

    if (init_result > 0 && transport == 1 && status == 0) {
        scr_printf("Request accepted by the available RPC path.\n");
        scr_printf("If safe, press PHYSICAL QUIT GAME once.\n");
        scr_printf("Does the console reset to DVR XMB?\n\n");
    } else {
        scr_printf("Request NOT confirmed. Do NOT infer QUIT support.\n\n");
    }

    scr_printf("Photograph the screen before pressing QUIT.\n");
    scr_printf("No files or partitions were modified.\n");
    scr_printf("Power off/reset normally to exit if needed.\n");

    for (;;) {
        SleepThread();
    }
    return 0;
}
