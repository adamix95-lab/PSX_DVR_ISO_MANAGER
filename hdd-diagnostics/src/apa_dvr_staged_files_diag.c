/*
 * DIAG H: verify staged OSDMenu and Neutrino file sizes and ELF headers.
 * DESR-7000 hardware validation pending. No filesystem writes.
 * PP.TESTDVR contains manually staged files. Only own RO pfs1: mount is removed.
 */
#include <debug.h>
#include <iopcontrol.h>
#include <libcdvd.h>
#include <libpad.h>
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
static int inspect_file(const char *step, const char *path, int require_elf)
{
    int fd, rv, length, read_count, result;
    static unsigned char header[64] __attribute__((aligned(64)));
    before(step);
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    scr_printf("%s OPEN=%d\n", step, fd);
    if (fd < 0) return fd;
    result = -119;
    length = fileXioLseek(fd, 0, SEEK_END);
    scr_printf("%s SIZE=%d\n", step, length);
    if (length <= 0) {
        result = length < 0 ? length : -120;
        goto close_file;
    }
    rv = fileXioLseek(fd, 0, SEEK_SET);
    if (rv < 0) { result = rv; goto close_file; }
    read_count = fileXioRead(fd, header, 16);
    scr_printf("%s READ=%d\n", step, read_count);
    if (read_count < 4) {
        result = read_count < 0 ? read_count : -121;
        goto close_file;
    }
    if (require_elf &&
        !(header[0] == 0x7f && header[1] == 'E' &&
          header[2] == 'L' && header[3] == 'F')) {
        scr_printf("%s ELF HEADER INVALID\n", step);
        result = -122;
        goto close_file;
    }
    result = length;
close_file:
    rv = fileXioClose(fd);
    if (rv < 0) {
        scr_printf("%s CLOSE=%d\n", step, rv);
        if (result >= 0) result = rv;
    }
    return result;
}
static void owned_unmount(void)
{
    int rv;
    if (!have_our_mount) return;
    before("H26 OWN PFS1 UMOUNT");
    rv = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("H26 UMOUNT=%d\n", rv);
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
    scr_printf("PSX DVR HDD DIAG H / RO + START\n");
    scr_printf("NO DISK WRITES / NO LAUNCH\n");
    before("H1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    before("H2 IOP SYNC");
    while (!SifIopSync()) {}
    before("H3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, label) \
    do { \
        result = module(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "H4 iomanX");
    LOAD(fileXio, "H5 fileXio");
    LOAD(ps2dev9, "H6 ps2dev9");
    LOAD(ps2atad, "H7 ps2atad");
    result = module("H8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("H9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;
    before("H10 fileXioInit");
    result = fileXioInit();
    scr_printf("H10 fileXioInit=%d\n", result);
    if (result < 0) goto done;
    LOAD(sio2man, "H11 sio2man");
    LOAD(mcman, "H12 mcman");
    LOAD(mcserv, "H13 mcserv");
    LOAD(freepad, "H14 freepad");
    before("H15 padInit");
    rv = padInit(0);
    scr_printf("H15 padInit=%d\n", rv);
    if (!rv) { result = -110; goto done; }
    before("H16 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("H16 padPortOpen=%d\n", rv);
    if (!rv) { padEnd(); result = -111; goto done; }
    have_pad = 1;

    result = directory("H17 hdd0", "hdd0:");
    if (result < 0) goto done;
    before("H18 PFS1 COLLISION");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("H18 PFS1 DOPEN=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        scr_printf("PFS1 ALREADY OWNED BY OTHER; STOP\n");
        result = -112;
        goto done;
    }
    before("H19 PP.TESTDVR MOUNT RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("H19 MOUNT=%d\n", mount_code);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;
    result = directory("H20 PS2 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto done;
    boot_cnf = inspect_file("H21 BOOT.CNF", PFS_MOUNTPOINT "/BOOT.CNF", 0);
    boot_elf = inspect_file("H22 BOOT.ELF", PFS_MOUNTPOINT "/BOOT.ELF", 1);
    neutrino_elf = inspect_file("H23 neutrino.elf", PFS_MOUNTPOINT "/neutrino.elf", 1);

    before("H24 sceCdInit");
    rv = sceCdInit(SCECdINoD);
    scr_printf("H24 CdInit=%d\n", rv);
    if (rv <= 0) { result = -113; goto done; }
    before("H25 CdNoticeGameStart");
    rv = sceCdNoticeGameStart(0, &notice_arg);
    scr_printf("H25 NOTICE=%d arg=%u\n", rv, notice_arg);
    if (rv < 0) { result = rv; goto done; }
    usleep(500000);
    LOAD(dvrdrv, "H DVRDRV");
    LOAD(dvrfile, "H DVRFILE");
    iso_code = directory("H DVR ISO", "dvr_pfs0:/ISO");
    pfs_code = directory("H PFS AGAIN", PFS_MOUNTPOINT "/");
    result = (iso_code >= 0 && pfs_code >= 0 &&
              boot_cnf > 0 && boot_elf > 0 && neutrino_elf > 0) ? 0 : -123;

done:
    owned_unmount();
    scr_clear();
    scr_printf("PSX DVR HDD DIAG H / RESULTS\n");
    scr_printf("RESULT=%d MOUNT=%d\n", result, mount_code);
    scr_printf("CNF bytes=%d ELF bytes=%d\n", boot_cnf, boot_elf);
    scr_printf("NEUTRINO bytes=%d\n", neutrino_elf);
    scr_printf("DVR ISO=%d PS2 PFS=%d\n", iso_code, pfs_code);
    scr_printf("NO DISK WRITES. PHOTO RESULTS.\n");
    /* Cleanup before START and normal process return, as in Manager v0.8. */
    wait_for_start();
    return 0;
}