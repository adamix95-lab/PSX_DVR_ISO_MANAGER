/*
 * DIAG M TARGET - OSDMenu-like APA module initialization after HDD ELF chainload.
 *
 * The original BOOT.ELF/BOOT.CNF/Neutrino and Sony system partitions remain
 * untouched. No app-level writes, no Neutrino launch, no game launch.
 *
 * WARNING: Third-party IOP driver internals are not proven write-free.
 * Run only with a backed-up disk, at the owner's explicit discretion.
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
#include <io_common.h>

#define TEST_PARTITION "hdd0:PP.TESTDVR"
#define PFS_MOUNTPOINT "pfs0:"
#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOT.CNF"

#define IRX(name) \
    extern unsigned char name##_irx[] __attribute__((aligned(16))); \
    extern uint32_t size_##name##_irx
IRX(iomanX);
IRX(fileXio);
IRX(sio2man);
IRX(mcman);
IRX(mcserv);
IRX(freepad);
IRX(ps2dev9);
IRX(bdm);
IRX(bdmfs_fatfs);
IRX(ata_bd);
IRX(ps2hdd_osd);
IRX(ps2fs);
IRX(secrsif);

static const char hdd_arguments[] = "-o\0" "4\0" "-n\0" "20";
static const char pfs_arguments[] = "-o\0" "10\0" "-n\0" "40";
typedef char check_hdd_args[(sizeof(hdd_arguments) == 11) ? 1 : -1];
typedef char check_pfs_args[(sizeof(pfs_arguments) == 12) ? 1 : -1];

static unsigned char padbuf[256] __attribute__((aligned(64)));
static char cnfbuf[512] __attribute__((aligned(64)));
static int have_pad;
static int own_mount;
static int stage;
static int arg_match;
static int mount_rv = -999;
static int cnf_size = -999;
static int boot_elf_size = -999;
static int neutrino_size = -999;
static int hdd_dir = -999;
static int pfs_dir = -999;
static int osd_modules = -999;

static void before(int n, const char *label)
{
    stage = n;
    scr_printf("M%d %s BEFORE\n", n, label);
    sleep(2);
}
static int modload(int n, const char *label, void *buf, uint32_t len,
                   const void *args, uint32_t argc)
{
    int id, status = -999;
    before(n, label);
    id = SifExecModuleBuffer(buf, len, argc, (void *)args, &status);
    scr_printf("M%d %s id=%d rv=%d\n", n, label, id, status);
    if (id < 0) return id;
    /* Upstream OSDMenu treats iopret == 1 as module init failure. */
    if (status == 1) return -201;
    return status < 0 ? status : 0;
}
static int dopen_close(int n, const char *path)
{
    int fd, rc;
    before(n, path);
    fd = fileXioDopen(path);
    scr_printf("M%d DOPEN=%d\n", n, fd);
    if (fd < 0) return fd;
    rc = fileXioDclose(fd);
    scr_printf("M%d DCLOSE=%d\n", n, rc);
    return rc;
}
static int file_size(int n, const char *path, int elf)
{
    int fd, size, got, rv, rc;
    static unsigned char magic[16] __attribute__((aligned(64)));
    before(n, path);
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    scr_printf("M%d OPEN=%d\n", n, fd);
    if (fd < 0) return fd;
    rc = -230;
    size = fileXioLseek(fd, 0, SEEK_END);
    if (size <= 0) { rc = size < 0 ? size : -231; goto close_fd; }
    rv = fileXioLseek(fd, 0, SEEK_SET);
    if (rv < 0) { rc = rv; goto close_fd; }
    got = fileXioRead(fd, magic, 4);
    if (got != 4) { rc = got < 0 ? got : -232; goto close_fd; }
    if (elf && !(magic[0] == 0x7f && magic[1] == 'E' &&
                 magic[2] == 'L' && magic[3] == 'F')) {
        rc = -233;
        goto close_fd;
    }
    rc = size;
close_fd:
    rv = fileXioClose(fd);
    if (rv < 0 && rc >= 0) rc = rv;
    scr_printf("M%d SIZE/RC=%d\n", n, rc);
    return rc;
}
static int read_cnf(void)
{
    int fd, size, rv, got, rc;
    before(24, "BOOT.CNF READ ONLY");
    fd = fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0);
    scr_printf("M24 CNF OPEN=%d\n", fd);
    if (fd < 0) return fd;
    rc = -240;
    size = fileXioLseek(fd, 0, SEEK_END);
    if (size <= 0 || size >= (int)sizeof(cnfbuf)) { rc = -241; goto close_fd; }
    rv = fileXioLseek(fd, 0, SEEK_SET);
    if (rv < 0) { rc = rv; goto close_fd; }
    got = fileXioRead(fd, cnfbuf, size);
    if (got != size) { rc = got < 0 ? got : -242; goto close_fd; }
    cnfbuf[size] = '\0';
    if (memchr(cnfbuf, '\0', (size_t)size) ||
        (size >= 3 && (unsigned char)cnfbuf[0] == 0xef &&
         (unsigned char)cnfbuf[1] == 0xbb &&
         (unsigned char)cnfbuf[2] == 0xbf)) {
        rc = -243;
        goto close_fd;
    }
    rc = size;
close_fd:
    rv = fileXioClose(fd);
    if (rv < 0 && rc >= 0) rc = rv;
    scr_printf("M24 CNF BYTES=%d\n", rc);
    return rc;
}
static int unmount_owned(void)
{
    int rc;
    if (!own_mount) return 0;
    before(27, "OWN PFS0 UMOUNT");
    rc = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("M27 UMOUNT=%d\n", rc);
    own_mount = 0;
    return rc;
}
static void finish_on_start(void)
{
    int sampled = 0;
    int old_buttons = 0;
    int pressed, fresh;
    struct padButtonStatus pad;
    scr_printf("START: EXIT TO XMB/OSD\n");
    if (!have_pad) {
        scr_printf("PAD NOT READY. PHOTO / REBOOT\n");
        for (;;) sleep(1);
    }
    for (;;) {
        if (padRead(0, 0, &pad) > 0) {
            pressed = pad.btns ^ 0xffff;
            fresh = pressed & ~old_buttons;
            old_buttons = pressed;
            if (!sampled) { sampled = 1; fresh = 0; }
            if (fresh & PAD_START) break;
        }
        usleep(20000);
    }
    padPortClose(0, 0);
    padEnd();
}
int main(int argc, char *argv[])
{
    int ret = -999, rv, occupied, umount_rv;
    const char *arg0 = (argc > 0 && argv && argv[0]) ? argv[0] : "(null)";
    const char *arg1 = (argc > 1 && argv && argv[1]) ? argv[1] : "(null)";
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR HDD DIAG M / OSD MODULES\n");
    scr_printf("M0 ENTRY FROM HDD\n");
    scr_printf("argc=%d argv0=%.48s\n", argc, arg0);
    scr_printf("argv1=%.48s\n", arg1);
    arg_match = argc > 1 && argv && argv[1] &&
                strcmp(argv[1], HDD_CNF_ARG) == 0;
    scr_printf("CNF ARG MATCH=%d\n", arg_match);

    before(1, "IOP RESET");
    while (!SifIopReset("", 0)) {}
    before(2, "IOP SYNC");
    while (!SifIopSync()) {}
    before(3, "RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, n, desc) \
    do { ret = modload(n, desc, name##_irx, size_##name##_irx, NULL, 0); \
         if (ret < 0) goto done; } while (0)
    /* Upstream OSDMenu: basic fileXio and controller modules first. */
    LOAD(iomanX, 4, "iomanX");
    LOAD(fileXio, 5, "fileXio");
    LOAD(sio2man, 6, "sio2man");
    LOAD(mcman, 7, "mcman");
    LOAD(mcserv, 8, "mcserv");

    /* Extra diagnostic-only controller module, so later failures are visible. */
    LOAD(freepad, 9, "freepad");
    before(10, "padInit");
    rv = padInit(0);
    scr_printf("M10 PAD INIT=%d\n", rv);
    if (!rv) { ret = -210; goto done; }
    before(11, "padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("M11 PAD OPEN=%d\n", rv);
    if (!rv) { padEnd(); ret = -211; goto done; }
    have_pad = 1;

    LOAD(ps2dev9, 12, "ps2dev9");
    LOAD(bdm, 13, "bdm");
    LOAD(bdmfs_fatfs, 14, "bdmfs_fatfs");
    LOAD(ata_bd, 15, "ata_bd");
    /* Match upstream OSDMenu's delay to prevent PS2HDD hang. */
    sleep(1);
    ret = modload(16, "ps2hdd-osd", ps2hdd_osd_irx,
                  size_ps2hdd_osd_irx, hdd_arguments, sizeof(hdd_arguments));
    if (ret < 0) goto done;
    ret = modload(17, "ps2fs", ps2fs_irx,
                  size_ps2fs_irx, pfs_arguments, sizeof(pfs_arguments));
    if (ret < 0) goto done;
    LOAD(secrsif, 18, "secrsif");
#undef LOAD
    osd_modules = 0;
    before(19, "fileXioInit");
    rv = fileXioInit();
    scr_printf("M19 FILEXIO=%d\n", rv);
    if (rv < 0) { ret = rv; goto done; }

    hdd_dir = dopen_close(20, "hdd0:");
    if (hdd_dir < 0) { ret = hdd_dir; goto done; }
    before(21, "PFS0 COLLISION CHECK");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("M21 PFS0 EXISTS FD=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        ret = -221;
        goto done;
    }
    before(22, "PP.TESTDVR MOUNT RO");
    mount_rv = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("M22 MOUNT=%d\n", mount_rv);
    if (mount_rv < 0) { ret = mount_rv; goto done; }
    own_mount = 1;
    pfs_dir = dopen_close(23, PFS_MOUNTPOINT "/");
    if (pfs_dir < 0) { ret = pfs_dir; goto done; }
    cnf_size = read_cnf();
    if (cnf_size <= 0) { ret = cnf_size < 0 ? cnf_size : -244; goto done; }
    boot_elf_size = file_size(25, PFS_MOUNTPOINT "/BOOT.ELF", 1);
    if (boot_elf_size <= 0) { ret = boot_elf_size; goto done; }
    neutrino_size = file_size(26, PFS_MOUNTPOINT "/neutrino.elf", 1);
    if (neutrino_size <= 0) { ret = neutrino_size; goto done; }
    ret = arg_match ? 0 : -250;
done:
    umount_rv = unmount_owned();
    if (ret == 0 && umount_rv < 0) ret = umount_rv;
    scr_clear();
    scr_printf("PSX DVR HDD DIAG M / RESULTS\n");
    scr_printf("RESULT=%d STAGE=%d\n", ret, stage);
    scr_printf("ARG=%d OSDMOD=%d\n", arg_match, osd_modules);
    scr_printf("HDD=%d MOUNT=%d PFS=%d\n", hdd_dir, mount_rv, pfs_dir);
    scr_printf("CNF=%d ELF=%d NEUT=%d\n", cnf_size, boot_elf_size, neutrino_size);
    scr_printf("OSDMENU DRIVER PROBE ONLY.\n");
    scr_printf("NO NEUTRINO/GAME LAUNCH.\n");
    scr_printf("PHOTO RESULTS.\n");
    finish_on_start();
    return 0;
}
