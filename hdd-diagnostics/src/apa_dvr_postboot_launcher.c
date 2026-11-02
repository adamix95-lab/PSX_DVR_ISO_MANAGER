/*
 * DIAG L LAUNCH: X-gated raw HDD chainload to L target for post-boot IOP diagnostics.
 * DIAG K proved F can execute from PP.TESTDVR. L target has no game launch.
 * X launches only the L target. No OSDMenu/Neutrino or game launch.
 * START cancels. Only our own read-only PFS mount is ever created.
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
#define LOADED_ELF "pfs1:/PSX_DVR_HDD_DIAG_L.ELF"
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
static int target_elf = -999;

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
/*
 * BOOT.CNF is inspected in RAM, but never executed. On explicit X, launch
 * only the known-good standalone DIAG F from the read-only PFS mount.
 * DIAG L target resets IOP and reads PP.TESTDVR/DVD ISO, then returns via START.
 */
static char cnf_text[512] __attribute__((aligned(64)));
static char dvd_path[200] __attribute__((aligned(64)));
static int config_type; /* 1 = SYSTEM, 2 = ENGLISH, 0 = unknown */
static int bsd_dvr;
static int has_dvd;
static int usb_dep;
static int dvd_open_code = -999;

static int read_config(void)
{
    int fd, n, got, rv;
    int result = -130;
    const char *p;
    before("L21 BOOT.CNF TEXT");
    fd = fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0);
    scr_printf("L21 CNF OPEN=%d\n", fd);
    if (fd < 0) return fd;
    n = fileXioLseek(fd, 0, SEEK_END);
    if (n <= 0 || n >= (int)sizeof(cnf_text)) {
        result = -131;
        goto close_config;
    }
    rv = fileXioLseek(fd, 0, SEEK_SET);
    if (rv < 0) { result = rv; goto close_config; }
    got = fileXioRead(fd, cnf_text, n);
    if (got != n) {
        result = got < 0 ? got : -132;
        goto close_config;
    }
    cnf_text[n] = '\0';
    /* Reject NUL, BOM, and non-printable bytes besides CR/LF. */
    if ((n >= 3 && (unsigned char)cnf_text[0] == 0xef &&
         (unsigned char)cnf_text[1] == 0xbb &&
         (unsigned char)cnf_text[2] == 0xbf)) {
        result = -133;
        goto close_config;
    }
    for (rv = 0; rv < n; ++rv) {
        unsigned char ch = (unsigned char)cnf_text[rv];
        if (ch == 0 || (ch < 0x20 && ch != '\n' && ch != '\r') || ch == 0x7f) {
            result = -134;
            goto close_config;
        }
    }
    usb_dep = (strstr(cnf_text, "mass0:") != NULL ||
               strstr(cnf_text, "usb0:") != NULL ||
               strstr(cnf_text, "ata0:") != NULL);
    p = cnf_text;
    while (*p) {
        const char *line_end = strchr(p, '\n');
        int line_len = line_end ? (int)(line_end - p) : (int)strlen(p);
        if (line_len && p[line_len - 1] == '\r') line_len--;
        if (line_len == 17 && !strncmp(p, "boot=neutrino.elf", 17))
            config_type = 1;
        else if (line_len == 22 && !strncmp(p, "boot=neutrino_lang.elf", 22))
            config_type = 2;
        if (line_len == 12 && !strncmp(p, "arg=-bsd=dvr", 12))
            bsd_dvr = 1;
        if (line_len > 9 && !strncmp(p, "arg=-dvd=", 9)) {
            int path_len = line_len - 9;
            if (path_len > 15 && path_len < (int)sizeof(dvd_path) &&
                !strncmp(p + 9, "dvr_pfs0:/ISO/", 14)) {
                memcpy(dvd_path, p + 9, (size_t)path_len);
                dvd_path[path_len] = '\0';
                has_dvd = 1;
            }
        }
        if (!line_end) break;
        p = line_end + 1;
    }
    result = n;
close_config:
    rv = fileXioClose(fd);
    if (rv < 0 && result >= 0) result = rv;
    scr_printf("CNF=%d TYPE=%d BSD=%d DVD=%d USB=%d\n",
               result, config_type, bsd_dvr, has_dvd, usb_dep);
    return result;
}

static int probe_selected_iso(void)
{
    int fd, rv;
    if (!has_dvd) return -135;
    before("I DVD ISO PATH");
    fd = fileXioOpen(dvd_path, FIO_O_RDONLY, 0);
    scr_printf("I ISO FILE OPEN=%d\n", fd);
    if (fd < 0) return fd;
    rv = fileXioClose(fd);
    return rv;
}

static void owned_unmount(void)
{
    int rv;
    if (!have_our_mount) return;
    before("L26 OWN PFS1 UMOUNT");
    rv = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("L26 UMOUNT=%d\n", rv);
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
        scr_printf("X: RUN HDD DIAG L ONLY\n");
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
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR HDD DIAG L / LAUNCH\n");
    scr_printf("RO PREFLIGHT. NO GAME LAUNCH.\n");
    before("L1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    before("L2 IOP SYNC");
    while (!SifIopSync()) {}
    before("L3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, label) \
    do { \
        result = module(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "L4 iomanX");
    LOAD(fileXio, "L5 fileXio");
    LOAD(ps2dev9, "L6 ps2dev9");
    LOAD(ps2atad, "L7 ps2atad");
    result = module("L8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("L9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;
    before("L10 fileXioInit");
    result = fileXioInit();
    scr_printf("L10 fileXioInit=%d\n", result);
    if (result < 0) goto done;
    LOAD(sio2man, "L11 sio2man");
    LOAD(mcman, "L12 mcman");
    LOAD(mcserv, "L13 mcserv");
    LOAD(freepad, "L14 freepad");
    before("L15 padInit");
    rv = padInit(0);
    scr_printf("L15 padInit=%d\n", rv);
    if (!rv) { result = -110; goto done; }
    before("L16 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("L16 padPortOpen=%d\n", rv);
    if (!rv) { padEnd(); result = -111; goto done; }
    have_pad = 1;

    result = directory("L17 hdd0", "hdd0:");
    if (result < 0) goto done;
    before("L18 PFS1 COLLISION");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("L18 PFS1 DOPEN=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        scr_printf("PFS1 ALREADY OWNED BY OTHER; STOP\n");
        result = -112;
        goto done;
    }
    before("L19 PP.TESTDVR MOUNT RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("L19 MOUNT=%d\n", mount_code);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;
    result = directory("L20 PS2 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto done;
    boot_cnf = read_config();
    boot_elf = inspect_file("L22 BOOT.ELF", PFS_MOUNTPOINT "/BOOT.ELF", 1);
    neutrino_elf = inspect_file("L23 neutrino.elf", PFS_MOUNTPOINT "/neutrino.elf", 1);
    target_elf = inspect_file("L TARGET ELF", PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_L.ELF", 1);

    before("L24 sceCdInit");
    rv = sceCdInit(SCECdINoD);
    scr_printf("L24 CdInit=%d\n", rv);
    if (rv <= 0) { result = -113; goto done; }
    before("L25 CdNoticeGameStart");
    rv = sceCdNoticeGameStart(0, &notice_arg);
    scr_printf("L25 NOTICE=%d arg=%u\n", rv, notice_arg);
    if (rv < 0) { result = rv; goto done; }
    usleep(500000);
    LOAD(dvrdrv, "L DVRDRV");
    LOAD(dvrfile, "L DVRFILE");
    iso_code = directory("L DVR ISO", "dvr_pfs0:/ISO");
    pfs_code = directory("L PFS AGAIN", PFS_MOUNTPOINT "/");
    if (has_dvd) dvd_open_code = probe_selected_iso();
    result = -136;
    if (iso_code >= 0 && pfs_code >= 0 &&
        boot_cnf > 0 && boot_elf > 0 && neutrino_elf > 0 && target_elf > 0 &&
        config_type == 1 && bsd_dvr && has_dvd &&
        !usb_dep && dvd_open_code >= 0)
        result = 0;

done:
    scr_clear();
    scr_printf("PSX DVR HDD DIAG L / LAUNCH\n");
    scr_printf("CHECK=%d MOUNT=%d\n", result, mount_code);
    scr_printf("CNF=%d ELF=%d NEUT=%d\n", boot_cnf, boot_elf, neutrino_elf);
    scr_printf("SYS=%d BSD=%d DVD=%d USB=%d\n",
               config_type == 1, bsd_dvr, has_dvd, usb_dep);
    scr_printf("ISO=%d DVR=%d PFS=%d\n", dvd_open_code, iso_code, pfs_code);
    scr_printf("TARGET L BYTES=%d\n", target_elf);
    scr_printf("NO DIAG WRITES. HDD PRECHECK.\n");
    if (!wait_for_choice(result == 0)) {
        /* Ordinary START exit: release exactly our own mount, then pad. */
        owned_unmount();
        padPortClose(0, 0);
        padEnd();
        return 0;
    }

    /* X only: preserve our read-only pfs1: mount for the EE loader to
     * read the test ELF. The loader will reset IOP while transferring
     * control. The L target is a controlled read-only test of a fresh IOP reset and
     * APA/DVR initialization AFTER the EE target has started from HDD.
     * It never invokes OSDMenu or Neutrino.
     */
    scr_clear();
    scr_printf("DIAG L / HDD TARGET CHAINLOAD\n");
    scr_printf("FILE: pfs1:/PSX_DVR_HDD_DIAG_L.ELF\n");
    scr_printf("RUNS ONLY DIAG L, NOT A GAME.\n");
    scr_printf("L BEFORE LoadELFFromFile\n");
    sleep(2);
    padPortClose(0, 0);
    padEnd();
    rv = LoadELFFromFile(LOADED_ELF, 0, NULL);

    /* If the loader returns, IOP state is not guaranteed; avoid further
     * HDD, pad or IOP RPC calls. Report exact error and request reboot. */
    scr_clear();
    scr_printf("L CHAINLOAD RETURNED=%d\n", rv);
    scr_printf("NO MORE IOP CALLS. PHOTO / REBOOT.\n");
    for (;;) sleep(1);
    return 1;
}
