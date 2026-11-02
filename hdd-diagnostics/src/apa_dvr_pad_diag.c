/*
 * DIAG G: independently test APA/PFS + DVR ISO + controller exit.
 * DESR-7000 hardware validation pending. No filesystem writes.
 * PP.TESTDVR is expected to be empty. Only own read-only pfs1: mount is removed.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <libcdvd.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
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
IRX(sio2man);
IRX(mcman);
IRX(mcserv);
IRX(freepad);

static const char hdd_arguments[] = "-o\0" "4\0" "-n\0" "20";
static const char pfs_arguments[] = "-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char hdd_arg_length_check[(sizeof(hdd_arguments) == 11) ? 1 : -1];
typedef char pfs_arg_length_check[(sizeof(pfs_arguments) == 17) ? 1 : -1];
static unsigned char padbuf[256] __attribute__((aligned(64)));
static int have_our_mount;
static int have_pad;
static int mount_code = -999;
static int iso_code = -999;
static int pfs_code = -999;
static int boot_cnf = -999;
static int boot_elf = -999;
static int neutrino_elf = -999;

static void before(const char *step)
{
    scr_printf("%s BEFORE\n", step);
    sleep(2);
}
static int module(const char *step, void *data, uint32_t size,
                  const void *args, uint32_t arg_len)
{
    int rv = -999;
    int id;
    before(step);
    id = SifExecModuleBuffer(data, size, arg_len, (void *)args, &rv);
    scr_printf("%s id=%d rv=%d\n", step, id, rv);
    if (id < 0) return id;
    return rv < 0 ? rv : 0;
}
static int directory(const char *step, const char *path)
{
    int fd, rv;
    before(step);
    fd = fileXioDopen(path);
    scr_printf("%s DOPEN=%d\n", step, fd);
    if (fd < 0) return fd;
    rv = fileXioDclose(fd);
    scr_printf("%s DCLOSE=%d\n", step, rv);
    return rv;
}
static int file_exists(const char *step, const char *path)
{
    int fd, rv;
    before(step);
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    scr_printf("%s OPEN=%d\n", step, fd);
    if (fd < 0) return fd; /* -2: expected on empty PP.TESTDVR */
    rv = fileXioClose(fd);
    scr_printf("%s CLOSE=%d\n", step, rv);
    return rv;
}
static void owned_unmount(void)
{
    int rv;
    if (!have_our_mount) return;
    before("G26 OWN PFS1 UMOUNT");
    rv = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("G26 UMOUNT=%d\n", rv);
    /* Never attempt a second unmount of someone else's mount. */
    have_our_mount = 0;
}
static void wait_for_start(void)
{
    int prev_buttons = 0;
    int sampled = 0;
    int bits, fresh;
    struct padButtonStatus btn;
    scr_printf("START: EXIT TO XMB/OSD\n");
    if (!have_pad) {
        scr_printf("PAD UNAVAILABLE; PHOTO AND REBOOT\n");
        for (;;) sleep(1);
    }
    for (;;) {
        if (padRead(0, 0, &btn) > 0) {
            bits = 0xffff ^ btn.btns;
            fresh = bits & ~prev_buttons;
            prev_buttons = bits;
            if (!sampled) {
                sampled = 1;
                fresh = 0;
            }
            if (fresh & PAD_START) break;
        }
        usleep(20000);
    }
    padPortClose(0, 0);
    padEnd();
}
int main(void)
{
    int result = -999;
    int rv;
    int occupied;
    u32 notice_arg = 0;
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR HDD DIAG G / RO + START\n");
    scr_printf("NO DISK WRITES / NO LAUNCH\n");
    before("G1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    before("G2 IOP SYNC");
    while (!SifIopSync()) {}
    before("G3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, label) \
    do { \
        result = module(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "G4 iomanX");
    LOAD(fileXio, "G5 fileXio");
    LOAD(ps2dev9, "G6 ps2dev9");
    LOAD(ps2atad, "G7 ps2atad");
    result = module("G8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("G9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;
    before("G10 fileXioInit");
    result = fileXioInit();
    scr_printf("G10 fileXioInit=%d\n", result);
    if (result < 0) goto done;
    LOAD(sio2man, "G11 sio2man");
    LOAD(mcman, "G12 mcman");
    LOAD(mcserv, "G13 mcserv");
    LOAD(freepad, "G14 freepad");
#undef LOAD
    before("G15 padInit");
    rv = padInit(0);
    scr_printf("G15 padInit=%d\n", rv);
    if (!rv) { result = -110; goto done; }
    before("G16 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("G16 padPortOpen=%d\n", rv);
    if (!rv) { padEnd(); result = -111; goto done; }
    have_pad = 1;

    result = directory("G17 hdd0", "hdd0:");
    if (result < 0) goto done;
    before("G18 PFS1 COLLISION");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("G18 PFS1 DOPEN=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        scr_printf("PFS1 ALREADY OWNED BY OTHER; STOP\n");
        result = -112;
        goto done;
    }
    before("G19 PP.TESTDVR MOUNT RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("G19 MOUNT=%d\n", mount_code);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;
    result = directory("G20 PS2 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto done;
    boot_cnf = file_exists("G21 BOOT.CNF", PFS_MOUNTPOINT "/BOOT.CNF");
    boot_elf = file_exists("G22 BOOT.ELF", PFS_MOUNTPOINT "/BOOT.ELF");
    neutrino_elf = file_exists("G23 neutrino.elf", PFS_MOUNTPOINT "/neutrino.elf");

    before("G24 sceCdInit");
    rv = sceCdInit(SCECdINoD);
    scr_printf("G24 CdInit=%d\n", rv);
    if (rv <= 0) { result = -113; goto done; }
    before("G25 CdNoticeGameStart");
    rv = sceCdNoticeGameStart(0, &notice_arg);
    scr_printf("G25 NOTICE=%d arg=%u\n", rv, notice_arg);
    if (rv < 0) { result = rv; goto done; }
    usleep(500000);
    LOAD(dvrdrv, "G DVRDRV");
    LOAD(dvrfile, "G DVRFILE");
    iso_code = directory("G DVR ISO", "dvr_pfs0:/ISO");
    pfs_code = directory("G PFS AGAIN", PFS_MOUNTPOINT "/");
    result = (iso_code >= 0 && pfs_code >= 0) ? 0 : -114;

done:
    owned_unmount();
    scr_clear();
    scr_printf("PSX DVR HDD DIAG G / RESULTS\n");
    scr_printf("RESULT=%d MOUNT=%d\n", result, mount_code);
    scr_printf("BOOT.CNF=%d BOOT.ELF=%d\n", boot_cnf, boot_elf);
    scr_printf("NEUTRINO=%d (ABSENT=-2)\n", neutrino_elf);
    scr_printf("DVR ISO=%d PS2 PFS=%d\n", iso_code, pfs_code);
    scr_printf("NO DISK WRITES. PHOTO RESULTS.\n");
    /* Cleanup before START and normal process return, as in Manager v0.8. */
    wait_for_start();
    return 0;
}