/*
 * PSX DVR ISO Manager v1.0 — HDD diagnostic, stage A (READ ONLY).
 *
 * Run from wLaunchELF R3Z, which already provides the PSX HDD/DVR IOP stack.
 * Attach to that stack without an IOP reset or module replacements.
 *
 * The only target partition is hdd0:PP.TESTDVR, manually created by the
 * console owner. This program cannot create, format or write a partition.
 * Its only mount is pfs1: with FIO_MT_RDONLY. DVR/ISO is read-only.
 *
 * Do not turn this into an installer. Any later writable v1.0 Manager must
 * be separately reviewed and tested before it reaches actual hardware.
 */
#include <debug.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sifrpc.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define PS2_PARTITION "hdd0:PP.TESTDVR"
#define DIAG_MOUNT "pfs1:"
#define DVR_ISO "dvr_pfs0:/ISO"
#define MAX_DIR_PREVIEW 2

static int inspect_directory(const char *label, const char *path)
{
    int fd;
    int found = 0;
    int last = 0;
    iox_dirent_t item;

    fd = fileXioDopen(path);
    scr_printf("%s: DOPEN=%d\n", label, fd);
    if (fd < 0)
        return fd;

    /* Read a few entries; never walk unknown portions of the HDD. */
    while (found < MAX_DIR_PREVIEW) {
        memset(&item, 0, sizeof(item));
        last = fileXioDread(fd, &item);
        if (last <= 0)
            break;
        ++found;
        scr_printf("  %d: %.27s\n", found, item.name);
    }
    if (last < 0)
        scr_printf("  DREAD error=%d\n", last);
    last = fileXioDclose(fd);
    if (last < 0)
        scr_printf("  DCLOSE error=%d\n", last);
    return last < 0 ? last : 0;
}

static void inspect_readable_file(const char *label, const char *path)
{
    int fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    int close_result = 0;
    if (fd >= 0)
        close_result = fileXioClose(fd);
    scr_printf("  %-16s open=%d close=%d\n", label, fd, close_result);
}

static void inspect_target_partition(void)
{
    int mount_result;
    int check;
    int unmount_result;

    /* Never change any existing mount or unmount another program's FS. */
    check = fileXioDopen(DIAG_MOUNT "/");
    if (check >= 0) {
        fileXioDclose(check);
        scr_printf("PFS1 already mounted! SKIP (safety)\n");
        return;
    }

    scr_printf("Mount %s -> %s READ ONLY\n", PS2_PARTITION, DIAG_MOUNT);
    mount_result = fileXioMount(DIAG_MOUNT, PS2_PARTITION, FIO_MT_RDONLY);
    scr_printf("  MOUNT=%d (raw error, incl. -19)\n", mount_result);
    if (mount_result < 0)
        return;

    /* Files can be absent on the newly-created test partition. */
    inspect_directory("PP.TESTDVR root", DIAG_MOUNT "/");
    inspect_readable_file("BOOT.ELF", DIAG_MOUNT "/BOOT.ELF");
    inspect_readable_file("BOOT.CNF", DIAG_MOUNT "/BOOT.CNF");
    inspect_readable_file("neutrino.elf", DIAG_MOUNT "/neutrino.elf");
    inspect_readable_file("DVR manager ELF", DIAG_MOUNT "/DVR_ISO_MANAGER.ELF");

    /* Unmount only the pfs1: mount that THIS program just created. */
    unmount_result = fileXioUmount(DIAG_MOUNT);
    scr_printf("  UMOUNT=%d\n", unmount_result);
}

int main(int argc, char **argv)
{
    int rpc_result;
    (void)argc;
    (void)argv;

    init_scr();
    scr_setCursor(0);
    scr_clear();

    scr_printf("PSX DVR ISO MANAGER v1.0 - HDD DIAG A\n");
    scr_printf("DESR: READ-ONLY TEST. NO HDD WRITES.\n");
    scr_printf("Run from wLaunchELF R3Z (USB).\n");
    scr_printf("--------------------------------------\n");

    /* Use the existing wLaunchELF IOP modules. No reset, no IRX loads. */
    SifInitRpc(0);
    rpc_result = fileXioInit();
    scr_printf("Attach fileXio: %d\n", rpc_result);

    inspect_directory("PS2 APA hdd0:", "hdd0:");
    inspect_directory("DVR ISO:", DVR_ISO);
    inspect_target_partition();

    scr_printf("--------------------------------------\n");
    scr_printf("NO BOOT / NO FORMATTING / NO WRITES\n");
    scr_printf("Photograph the complete results.\n");
    scr_printf("Power off/reboot to leave this screen.\n");

    for (;;)
        sleep(1);
    return 0;
}
