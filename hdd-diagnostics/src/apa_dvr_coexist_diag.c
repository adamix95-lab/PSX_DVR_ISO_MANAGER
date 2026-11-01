/*
 * PSX DVR ISO Manager v1.0 — DIAG E: PS2 APA/PFS + DVR, READ ONLY.
 *
 * Verified on DESR-7000 with DIAG D:
 *   hdd0: DOPEN=2, pfs1: before mount=-19,
 *   hdd0:PP.TESTDVR mounted read-only with MOUNT=0,
 *   pfs1:/ DOPEN=3, BOOT.CNF=-2 (expected absent), UMOUNT=0.
 *
 * E isolates coexistence of PS2 APA/PFS and PSX DVR (dvr_pfs0:/ISO).
 * Uses the IOP reset/driver sequence validated by C and D, then
 * the sceCdInit / sceCdNoticeGameStart / DVR modules sequence from
 * RepairBox's src/storage.c. It does not load a second DEV9 module:
 * PS2DEV9 from PS2SDK already manages that hardware.
 *
 * NEVER writes or formats any partition, NEVER changes XMB or USB,
 * NEVER launches a game. PP.TESTDVR is mounted FIO_MT_RDONLY only.
 * Every risky operation prints BEFORE and pauses for freeze photographs.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <libcdvd.h>
#include <stdint.h>
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
IRX(dvrdrv);
IRX(dvrfile);

/* Borrow PS2 HDD/PS2FS arguments from successful DIAG D and RepairBox. */
static const char hdd_arguments[] = "-o\0" "4\0" "-n\0" "20";
static const char pfs_arguments[] = "-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char hdd_arg_len_check[(sizeof(hdd_arguments) == 11) ? 1 : -1];
typedef char pfs_arg_len_check[(sizeof(pfs_arguments) == 17) ? 1 : -1];

static int have_our_mount;
static int mount_code = -999;
static int dvr_hdd_result = -999;
static int dvr_pfs_result = -999;
static int iso_result = -999;
static int pfs_after_result = -999;
static int cdvd_result = -999;
static int notice_result = -999;

static void before(const char *stage)
{
    scr_printf("%s BEFORE\n", stage);
    sleep(2);
}

static int module(const char *stage, void *data, uint32_t size,
                  const void *args, uint32_t arg_len)
{
    int result = -999;
    int id;

    before(stage);
    id = SifExecModuleBuffer(data, size, arg_len, (void *)args, &result);
    scr_printf("%s id=%d rv=%d\n", stage, id, result);
    if (id < 0) return id;
    if (result < 0) return result;
    return 0;
}

static int directory(const char *stage, const char *path)
{
    int fd, close_result;
    before(stage);
    fd = fileXioDopen(path);
    scr_printf("%s DOPEN=%d\n", stage, fd);
    if (fd < 0) return fd;
    close_result = fileXioDclose(fd);
    if (close_result < 0)
        scr_printf("%s DCLOSE=%d\n", stage, close_result);
    return close_result;
}

static void owned_unmount(void)
{
    int result;
    if (!have_our_mount)
        return;
    before("E24 UMOUNT OWN PFS1");
    result = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("E24 UMOUNT=%d\n", result);
    have_our_mount = 0;
}

int main(void)
{
    int result = -999;
    int check;
    int notice_arg = -999;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR v1.0 HDD DIAG E / APA + DVR\n");
    scr_printf("RO ONLY. NO PARTITION WRITES.\n");

    before("E1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    scr_printf("E1 RESET SENT\n");
    before("E2 IOP SYNC");
    while (!SifIopSync()) {}
    scr_printf("E2 SYNC DONE\n");
    before("E3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    scr_printf("E3 RPC PATCH DONE\n");

#define LOAD(name, stage) \
    do { \
        result = module(stage, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "E4 iomanX");
    LOAD(fileXio, "E5 fileXio");
    LOAD(ps2dev9, "E6 ps2dev9");
    LOAD(ps2atad, "E7 ps2atad");
#undef LOAD

    result = module("E8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("E9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;

    before("E10 fileXioInit");
    result = fileXioInit();
    scr_printf("E10 fileXioInit=%d\n", result);
    if (result < 0) goto done;

    /* PAGE TWO: PS2 mount + initialization of DVR modules. */
    sleep(2);
    scr_clear();
    scr_printf("PSX DVR DIAG E / PFS + DVR DRIVERS\n");
    scr_printf("PS2 HDD MODULES: OK\n");

    result = directory("E11 hdd0", "hdd0:");
    if (result < 0) goto done;

    before("E12 pfs1 collision");
    check = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("E12 pfs1 DOPEN=%d\n", check);
    if (check >= 0) {
        fileXioDclose(check);
        scr_printf("PFS1 ALREADY MOUNTED; STOP\n");
        result = -100;
        goto done;
    }

    before("E13 MOUNT TESTDVR RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("E13 MOUNT=%d\n", mount_code);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;

    result = directory("E14 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto cleanup;

    /* RepairBox PSX DVR initialization order. CDVD notice does not
       write HDD files, but it does change volatile CDVD/IOP state. */
    before("E15 sceCdInit");
    cdvd_result = sceCdInit(SCECdINoD);
    scr_printf("E15 CdInit=%d\n", cdvd_result);
    if (cdvd_result <= 0) {
        result = -101;
        goto cleanup;
    }

    before("E16 CdNoticeGameStart");
    notice_result = sceCdNoticeGameStart(0, &notice_arg);
    scr_printf("E16 notice=%d arg=%d\n", notice_result, notice_arg);
    /* A negative result means CDVD negotiation failed. */
    if (notice_result < 0) {
        result = notice_result;
        goto cleanup;
    }
    usleep(500000);

    result = module("E17 dvrdrv", dvrdrv_irx, size_dvrdrv_irx, NULL, 0);
    if (result < 0) goto cleanup;
    result = module("E18 dvrfile", dvrfile_irx, size_dvrfile_irx, NULL, 0);
    if (result < 0) goto cleanup;

    /* PAGE THREE: the primary coexistence evidence. */
    sleep(2);
    scr_clear();
    scr_printf("PSX DVR DIAG E / BOTH FILESYSTEMS\n");
    scr_printf("E13 MOUNT=%d; E16 NOTICE=%d\n", mount_code, notice_result);
    scr_printf("E17/E18 DVR MODULES STARTED\n");

    dvr_hdd_result = directory("E19 DVR HDD", "dvr_hdd0:");
    dvr_pfs_result = directory("E20 DVR PFS", "dvr_pfs0:/");
    iso_result = directory("E21 DVR ISO", "dvr_pfs0:/ISO");
    pfs_after_result = directory("E22 PS2 PFS1", PFS_MOUNTPOINT "/");
    result = (iso_result >= 0 && pfs_after_result >= 0) ? 0 : -102;

cleanup:
    owned_unmount();
done:
    scr_printf("E FINAL: result=%d\n", result);
    if (mount_code >= 0)
        scr_printf("PS2 MOUNT=%d AFTER=%d\n", mount_code, pfs_after_result);
    if (dvr_hdd_result != -999)
        scr_printf("DVR HDD=%d PFS=%d ISO=%d\n",
                   dvr_hdd_result, dvr_pfs_result, iso_result);
    scr_printf("NO DISK WRITES. REBOOT TO EXIT.\n");
    scr_printf("PHOTO LAST BEFORE IF FROZEN.\n");
    for (;;)
        sleep(1);
    return 0;
}
