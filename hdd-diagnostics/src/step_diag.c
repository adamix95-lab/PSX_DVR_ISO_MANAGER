/*
 * PSX DVR ISO Manager v1.0 — stage-by-stage READ-ONLY HDD diagnostic B.
 * Run using wLaunchELF R3Z. Does NOT reset IOP, load modules, write to HDD,
 * create partitions, install XMB apps or launch OSDMenu/Neutrino.
 *
 * The stage marker is printed BEFORE every potentially blocking RPC call.
 * If the system freezes, photograph the last visible "BEFORE" line.
 */
#include <debug.h>
#include <stdio.h>
#include <unistd.h>
#include <sifrpc.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define PS2_PARTITION "hdd0:PP.TESTDVR"
#define DIAG_MOUNT "pfs1:"

static void before(const char *label)
{
    scr_printf("%s: BEFORE\n", label);
    sleep(2); /* Give the GS time to show the marker before a possible hang. */
}

static int directory_check(const char *label, const char *path)
{
    int fd;
    int closed;

    before(label);
    fd = fileXioDopen(path);
    scr_printf("%s: DOPEN=%d\n", label, fd);
    if (fd < 0)
        return fd;
    before("DCLOSE");
    closed = fileXioDclose(fd);
    scr_printf("DCLOSE=%d\n", closed);
    return closed < 0 ? closed : 0;
}

int main(void)
{
    int rc;
    int pfs_check;
    int umount_result;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR v1.0 - HDD DIAG B\n");
    scr_printf("READ ONLY / PP.TESTDVR ONLY\n");
    scr_printf("IF FROZEN: PHOTO LAST BEFORE LINE\n");

    before("STEP 1 SifInitRpc");
    SifInitRpc(0);
    scr_printf("STEP 1: DONE\n");

    before("STEP 2 fileXioInit");
    rc = fileXioInit();
    scr_printf("STEP 2: RESULT=%d\n", rc);

    directory_check("STEP 3 hdd0:", "hdd0:");
    directory_check("STEP 4 DVR ISO", "dvr_pfs0:/ISO");

    /* Never attempt to replace a preexisting pfs1 mount. */
    before("STEP 5 pfs1 check");
    pfs_check = fileXioDopen(DIAG_MOUNT "/");
    scr_printf("STEP 5: DOPEN=%d\n", pfs_check);
    if (pfs_check >= 0) {
        fileXioDclose(pfs_check);
        scr_printf("PFS1 IN USE - SKIP MOUNT\n");
        goto done;
    }

    before("STEP 6 RO mount PP.TESTDVR");
    rc = fileXioMount(DIAG_MOUNT, PS2_PARTITION, FIO_MT_RDONLY);
    scr_printf("STEP 6: MOUNT RESULT=%d\n", rc);
    if (rc < 0)
        goto done;

    directory_check("STEP 7 PFS1 root", DIAG_MOUNT "/");

    before("STEP 8 owned PFS1 umount");
    umount_result = fileXioUmount(DIAG_MOUNT);
    scr_printf("STEP 8: UMOUNT=%d\n", umount_result);

done:
    scr_printf("DIAGNOSTIC END / NO DISK WRITES\n");
    scr_printf("PHOTO RESULTS. RESTART TO EXIT.\n");
    for (;;)
        sleep(1);
    return 0;
}
