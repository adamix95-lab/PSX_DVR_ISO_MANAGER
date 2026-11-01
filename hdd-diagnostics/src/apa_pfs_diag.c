/*
 * PSX DVR ISO Manager v1.0 — DIAG D: PS2 APA/PFS only, READ ONLY.
 *
 * Physical DIAG C on DESR-7000: iomanX id=25, fileXio id=26,
 * fileXioInit=0 after the full IOP reset/patch/module sequence.
 *
 * This stage extends C with the PS2 HDD module sequence used by RepairBox:
 * ps2dev9 -> ps2atad -> ps2hdd -> ps2fs.  Exact parameters from RepairBox.
 * It does NOT load DVR modules yet; simultaneous DVR access will be a
 * separate stage only after user confirms this basic PS2 APA mount.
 *
 * NO disk writes, NO formatting, NO partition creation, NO XMB changes.
 * Only hdd0:PP.TESTDVR is mounted, at pfs1: with FIO_MT_RDONLY.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define TEST_PARTITION "hdd0:PP.TESTDVR"
#define PFS_MOUNTPOINT "pfs1:"

#define IRX(name) \
    extern unsigned char name##_irx[] __attribute__((aligned(16))); \
    extern uint32_t size_##name##_irx

IRX(iomanX);
IRX(fileXio);
IRX(ps2dev9);
IRX(ps2atad);
IRX(ps2hdd);
IRX(ps2fs);

/* Same binary argument layout as RepairBox src/storage.c (PS2 APA). */
static const char hdd_arguments[] = "-o\0" "4\0" "-n\0" "20";
static const char pfs_arguments[] = "-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char check_hdd_arguments_size[(sizeof(hdd_arguments) == 11) ? 1 : -1];
typedef char check_pfs_arguments_size[(sizeof(pfs_arguments) == 17) ? 1 : -1];

static void step_before(const char *name)
{
    scr_printf("%s BEFORE\n", name);
    sleep(2); /* freeze photos can identify the last attempted operation */
}

static int start_irx(const char *name, const void *data, uint32_t size,
                     const void *arguments, uint32_t argument_size)
{
    int start_result = -999;
    int module_id;

    step_before(name);
    module_id = SifExecModuleBuffer((void *)data, size,
                                   argument_size, (void *)arguments,
                                   &start_result);
    scr_printf("%s id=%d rv=%d\n", name, module_id, start_result);
    if (module_id < 0)
        return module_id;
    if (start_result < 0)
        return start_result;
    return 0;
}

static int open_directory(const char *name, const char *path)
{
    int fd;
    int close_result;

    step_before(name);
    fd = fileXioDopen(path);
    scr_printf("%s DOPEN=%d\n", name, fd);
    if (fd < 0)
        return fd;
    close_result = fileXioDclose(fd);
    scr_printf("%s DCLOSE=%d\n", name, close_result);
    return close_result;
}

int main(void)
{
    int result;
    int mounted = 0;
    int check;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR v1.0 HDD DIAG D / APA RO\n");
    scr_printf("NO DVR TEST. NO HDD WRITES.\n");

    step_before("D1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    scr_printf("D1 RESET SENT\n");
    step_before("D2 IOP SYNC");
    while (!SifIopSync()) {}
    scr_printf("D2 SYNC DONE\n");
    step_before("D3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    scr_printf("D3 RPC PATCH DONE\n");

#define LOAD0(name, label) \
    do { \
        result = start_irx(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto failure; \
    } while (0)
    LOAD0(iomanX, "D4 iomanX");
    LOAD0(fileXio, "D5 fileXio");
    LOAD0(ps2dev9, "D6 ps2dev9");
    LOAD0(ps2atad, "D7 ps2atad");
#undef LOAD0

    result = start_irx("D8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                       hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto failure;
    result = start_irx("D9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                       pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto failure;

    step_before("D10 fileXioInit");
    result = fileXioInit();
    scr_printf("D10 fileXioInit=%d\n", result);
    if (result < 0) goto failure;

    /* New screen for disk checks; the module list has already passed. */
    sleep(3);
    scr_clear();
    scr_printf("PSX DVR DIAG D: PS2 APA PHASE\n");
    scr_printf("DRIVERS LOADED / IOP RPC READY\n");

    result = open_directory("D11 hdd0:", "hdd0:");
    if (result < 0) goto failure;

    /* Never replace an existing mount of another application. */
    step_before("D12 pfs1 collision");
    check = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("D12 pfs1 DOPEN=%d\n", check);
    if (check >= 0) {
        fileXioDclose(check);
        scr_printf("PFS1 IN USE; STOP WITHOUT MOUNT\n");
        result = -100;
        goto failure;
    }

    step_before("D13 MOUNT PP.TESTDVR RO");
    result = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("D13 MOUNT=%d\n", result);
    if (result < 0) goto failure;
    mounted = 1;

    result = open_directory("D14 PFS1 ROOT", PFS_MOUNTPOINT "/");
    if (result < 0) goto cleanup;

    /* Never open the test partition for writing. */
    step_before("D15 BOOT.CNF read");
    check = fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0);
    scr_printf("D15 BOOT.CNF OPEN=%d\n", check);
    if (check >= 0)
        fileXioClose(check);
    /* Missing BOOT.CNF is expected on a fresh empty PP.TESTDVR. */

cleanup:
    if (mounted) {
        step_before("D16 UMOUNT owned PFS1");
        check = fileXioUmount(PFS_MOUNTPOINT);
        scr_printf("D16 UMOUNT=%d\n", check);
        if (check < 0)
            result = check;
    }
    if (result < 0) goto failure;

    scr_printf("PS2 APA/PFS RO CHECK COMPLETE\n");
    scr_printf("NO DVR TEST PERFORMED\n");
    goto done;

failure:
    scr_printf("STOP RESULT=%d\n", result);
    scr_printf("PHOTO LAST BEFORE / ERRORS\n");
done:
    scr_printf("NO HDD WRITES. REBOOT TO EXIT\n");
    for (;;)
        sleep(1);
    return 0;
}
