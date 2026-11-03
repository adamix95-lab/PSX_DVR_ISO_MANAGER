/*
 * DIAG Q: real OSDMenu BOOT.ELF chainloads known-good DIAG F via BOOTQ.CNF.
 * Only a strict verified test config is allowed; it never references a game.
 * No XMB/system partition changes. Our only mount is read-only pfs1:.
 */
#include <debug.h>
#include <elf-loader.h>
#include <iopcontrol.h>
#include <libcdvd.h>
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
#define PFS_MOUNTPOINT "pfs1:"
#define LOADED_ELF "pfs1:/BOOT.ELF"
#define HDD_CNF_ARG "hdd0:PP.TESTDVR/BOOTQ.CNF"
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
static int target_f = -999;

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
/* Never launch arbitrary Quickboot entries: exact-match only this F-only
 * config. OSDMenu's -dev9=NICHDD preserves HDD/DEV9 power during handoff.
 */
static const char expected_q_cnf[] =
    "boot=PSX_DVR_HDD_DIAG_F.ELF\n"
    "arg=-dev9=NICHDD\n";

static int read_q_config(void)
{
    int fd, length, got, rv, result;
    static char config_bytes[128] __attribute__((aligned(64)));
    before("Q21 BOOTQ.CNF EXACT VERIFY");
    fd = fileXioOpen(PFS_MOUNTPOINT "/BOOTQ.CNF", FIO_O_RDONLY, 0);
    scr_printf("Q21 OPEN=%d\n", fd);
    if (fd < 0) return fd;
    result = -130;
    length = fileXioLseek(fd, 0, SEEK_END);
    scr_printf("Q21 BYTES=%d\n", length);
    if (length != (int)(sizeof(expected_q_cnf) - 1) ||
        length <= 0 || length > (int)sizeof(config_bytes)) {
        result = -131;
        goto done_config;
    }
    rv = fileXioLseek(fd, 0, SEEK_SET);
    if (rv < 0) { result = rv; goto done_config; }
    got = fileXioRead(fd, config_bytes, length);
    if (got != length) {
        result = got < 0 ? got : -132;
        goto done_config;
    }
    result = memcmp(config_bytes, expected_q_cnf, (size_t)length) ? -133 : length;
done_config:
    rv = fileXioClose(fd);
    if (rv < 0 && result >= 0) result = rv;
    scr_printf("Q21 CONFIG RESULT=%d\n", result);
    return result;
}

static void owned_unmount(void)
{
    int rv;
    if (!have_our_mount) return;
    before("Q26 OWN PFS1 UMOUNT");
    rv = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("Q26 UMOUNT=%d\n", rv);
    /* Never attempt a second unmount of someone else's mount. */
    have_our_mount = 0;
}
/* Fresh presses only: holding X or START during entry cannot auto-launch. */
static int wait_for_choice(int enabled)
{
    int prev_buttons = 0;
    int sampled = 0;
    int bits, fresh;
    struct padButtonStatus btn;

    if (enabled) {
        scr_printf("X: REAL OSDMENU -> DIAG F ONLY\n");
        scr_printf("START: CANCEL TO XMB/OSD\n");
    } else {
        scr_printf("PRECHECK FAILED; X DISABLED\n");
        scr_printf("START: EXIT TO XMB/OSD\n");
    }
    if (!have_pad) {
        scr_printf("PAD NOT READY: PHOTO / REBOOT\n");
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
            if (fresh & PAD_START) return 0;
            if (enabled && (fresh & PAD_CROSS)) return 1;
        }
        usleep(20000);
    }
}

int main(void)
{
    int result = -999;
    int rv;
    int occupied;
    u32 notice_arg = 0;
    char *launcher_args[1] = {(char *)HDD_CNF_ARG};
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR HDD DIAG Q / X-GATED LAUNCH\n");
    scr_printf("RO PREFLIGHT. F-ONLY TEST.\n");
    before("Q1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    before("Q2 IOP SYNC");
    while (!SifIopSync()) {}
    before("Q3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, label) \
    do { \
        result = module(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "Q4 iomanX");
    LOAD(fileXio, "Q5 fileXio");
    LOAD(ps2dev9, "Q6 ps2dev9");
    LOAD(ps2atad, "Q7 ps2atad");
    result = module("Q8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("Q9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;
    before("Q10 fileXioInit");
    result = fileXioInit();
    scr_printf("Q10 fileXioInit=%d\n", result);
    if (result < 0) goto done;
    LOAD(sio2man, "Q11 sio2man");
    LOAD(mcman, "Q12 mcman");
    LOAD(mcserv, "Q13 mcserv");
    LOAD(freepad, "Q14 freepad");
    before("Q15 padInit");
    rv = padInit(0);
    scr_printf("Q15 padInit=%d\n", rv);
    if (!rv) { result = -110; goto done; }
    before("Q16 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("Q16 padPortOpen=%d\n", rv);
    if (!rv) { padEnd(); result = -111; goto done; }
    have_pad = 1;

    result = directory("Q17 hdd0", "hdd0:");
    if (result < 0) goto done;
    before("Q18 PFS1 COLLISION");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("Q18 PFS1 DOPEN=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        scr_printf("PFS1 ALREADY OWNED BY OTHER; STOP\n");
        result = -112;
        goto done;
    }
    before("Q19 PP.TESTDVR MOUNT RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("Q19 MOUNT=%d\n", mount_code);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;
    result = directory("Q20 PS2 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto done;
    boot_cnf = read_q_config();
    boot_elf = inspect_file("Q22 BOOT.ELF", PFS_MOUNTPOINT "/BOOT.ELF", 1);
    target_f = inspect_file("Q23 KNOWN GOOD F", PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_F.ELF", 1);

    before("Q24 sceCdInit");
    rv = sceCdInit(SCECdINoD);
    scr_printf("Q24 CdInit=%d\n", rv);
    if (rv <= 0) { result = -113; goto done; }
    before("Q25 CdNoticeGameStart");
    rv = sceCdNoticeGameStart(0, &notice_arg);
    scr_printf("Q25 NOTICE=%d arg=%u\n", rv, notice_arg);
    if (rv < 0) { result = rv; goto done; }
    usleep(500000);
    LOAD(dvrdrv, "J DVRDRV");
    LOAD(dvrfile, "J DVRFILE");
    iso_code = directory("J DVR ISO", "dvr_pfs0:/ISO");
    pfs_code = directory("J PFS AGAIN", PFS_MOUNTPOINT "/");
    result = -136;
    if (iso_code >= 0 && pfs_code >= 0 &&
        boot_cnf == (int)(sizeof(expected_q_cnf) - 1) &&
        boot_elf > 0 && target_f > 0)
        result = 0;

done:
    scr_clear();
    scr_printf("PSX DVR HDD DIAG Q / RESULTS\n");
    scr_printf("CHECK=%d MOUNT=%d\n", result, mount_code);
    scr_printf("BOOTQ=%d OSD ELF=%d\n", boot_cnf, boot_elf);
    scr_printf("TARGET F=%d DVR=%d PFS=%d\n", target_f, iso_code, pfs_code);
    scr_printf("RO PRECHECK, NO GAME BOOT.\n");
    if (!wait_for_choice(result == 0)) {
        /* Ordinary START exit: release exactly our own mount, then pad. */
        owned_unmount();
        padPortClose(0, 0);
        padEnd();
        return 0;
    }

    /* X only: preserve our read-only pfs1: for the proven EE loader to
     * load the original OSDMenu BOOT.ELF. OSDMenu then uses its own real
     * Quickboot/handlePFS/embedded loader. Exact BOOTQ.CNF contains only
     * benign diagnostic F, never Neutrino or a game. IOP state may change.
     */
    scr_clear();
    scr_printf("DIAG Q / CHAINLOAD REQUESTED\n");
    scr_printf("OSDMENU: pfs1:/BOOT.ELF\n");
    scr_printf("CNF: hdd0:PP.TESTDVR/BOOTQ.CNF\n");
    scr_printf("F ONLY. NEVER BOOT GAME.\n");
    scr_printf("J BEFORE LoadELFFromFile\n");
    sleep(2);
    padPortClose(0, 0);
    padEnd();
    rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);

    /* If the loader returns, IOP state is not guaranteed; avoid further
     * HDD, pad or IOP RPC calls. Report exact error and request reboot. */
    scr_clear();
    scr_printf("J CHAINLOAD RETURNED=%d\n", rv);
    scr_printf("NO MORE IOP CALLS. PHOTO / REBOOT.\n");
    for (;;) sleep(1);
    return 1;
}
