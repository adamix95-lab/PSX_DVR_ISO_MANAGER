/*
 * DIAG R2: read-only DIAG_R.CNF byte-for-byte mismatch inspector.
 * DIAG Q loaded F through real OSDMenu. R tests argv, not Neutrino.
 * Does not launch BOOT.ELF, R_ARG, Neutrino or game under any condition.
 * That CNF is validated byte-for-byte BEFORE X is enabled; it only points to
 * DIAG R_ARG, not Neutrino or a game. No changes to BOOT.CNF/ELF or ISO.
 * Normal START cancel unmounts only our own read-only PFS1.
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
#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_R.CNF"
#define Q_CNF_PATH PFS_MOUNTPOINT "/DIAG_R.CNF"
#define F_TARGET_PATH PFS_MOUNTPOINT "/PSX_DVR_HDD_DIAG_R_ARG.ELF"
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
static int q_config_ok = -999;
static int f_target_size = -999;
static int r2_expected_len = -999, r2_real_len = -999;
static int r2_read_len = -999;
static int r2_mismatch = -999;
static int r2_expected_byte = -999, r2_actual_byte = -999;
static int r2_cr_count, r2_lf_count, r2_nul_count;
static unsigned r2_exp_sum, r2_got_sum;
static int r2_window_start, r2_window_count;
static unsigned char r2_exp_window[8], r2_got_window[8];


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
 * BOOT.CNF is inspected in RAM only. On explicit X, launch via the mounted
 * PFS1 file, with an explicit HDD BOOT.CNF arg (OSDMenu config handler).
 * This avoids passing the temporary pfs1:/ path as Quickboot argv[0].
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
    before("R21 BOOT.CNF TEXT");
    fd = fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0);
    scr_printf("R21 CNF OPEN=%d\n", fd);
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


/* The ONLY permitted Q config, no extra boot=, path= or arg= lines.
 * Reject UTF-8 BOM, CRLF, inline comments or trailing bytes. The Q
 * config is a different file; NEVER overwrite the user's BOOT.CNF.
 */
static const char q_expected_config[] =
    "boot=PSX_DVR_HDD_DIAG_R_ARG.ELF\n"
    "arg=-bsd=dvr\n"
    "arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso\n";
static char q_config_bytes[sizeof(q_expected_config)] __attribute__((aligned(64)));

static int inspect_q_config(void)
{
    int fd, length, got, rc, closing, i;
    before("R2 DIAG_R.CNF RAW BYTES");
    r2_expected_len = (int)sizeof(q_expected_config) - 1;
    fd = fileXioOpen(Q_CNF_PATH, FIO_O_RDONLY, 0);
    if (fd < 0) return fd;
    rc = -340;
    length = fileXioLseek(fd, 0, SEEK_END);
    r2_real_len = length;
    if (length < 0) { rc = length; goto done_config; }
    if (length != r2_expected_len) { rc = -341; goto done_config; }
    if (fileXioLseek(fd, 0, SEEK_SET) < 0) { rc = -342; goto done_config; }
    memset(q_config_bytes, 0xa5, sizeof(q_config_bytes));
    got = fileXioRead(fd, q_config_bytes, length);
    r2_read_len = got;
    if (got != length) { rc = got < 0 ? got : -343; goto done_config; }

    r2_mismatch = -1;
    for (i = 0; i < length; i++) {
        unsigned char a = (unsigned char)q_expected_config[i];
        unsigned char b = (unsigned char)q_config_bytes[i];
        r2_exp_sum += a;
        r2_got_sum += b;
        if (b == '\r') r2_cr_count++;
        if (b == '\n') r2_lf_count++;
        if (b == 0) r2_nul_count++;
        if (r2_mismatch < 0 && a != b) {
            r2_mismatch = i;
            r2_expected_byte = (int)a;
            r2_actual_byte = (int)b;
        }
    }
    r2_window_start = (r2_mismatch < 0) ? 0 :
                      (r2_mismatch >= 3 ? r2_mismatch - 3 : 0);
    r2_window_count = length - r2_window_start;
    if (r2_window_count > 8) r2_window_count = 8;
    for (i = 0; i < r2_window_count; i++) {
        r2_exp_window[i] =
            (unsigned char)q_expected_config[r2_window_start + i];
        r2_got_window[i] =
            (unsigned char)q_config_bytes[r2_window_start + i];
    }
    rc = r2_mismatch >= 0 ? -344 : length;

done_config:
    closing = fileXioClose(fd);
    if (closing < 0 && rc >= 0) rc = closing;
    return rc;
}

static void print_r2_bytes(const char *label, const unsigned char *data, int n)
{
    int i;
    scr_printf("%s", label);
    for (i = 0; i < n; ++i) scr_printf("%02X ", (unsigned)data[i]);
    scr_printf("\n");
}

static void owned_unmount(void)
{
    int rv;
    if (!have_our_mount) return;
    before("R26 OWN PFS1 UMOUNT");
    rv = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("R26 UMOUNT=%d\n", rv);
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
        scr_printf("X: OSDMENU -> R ARGV TEST\n");
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
    scr_printf("PSX DVR HDD DIAG R / X-GATED LAUNCH\n");
    scr_printf("RO PREFLIGHT. RCNF -> R ARG ONLY.\n");
    before("R1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    before("R2 IOP SYNC");
    while (!SifIopSync()) {}
    before("R3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, label) \
    do { \
        result = module(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "R4 iomanX");
    LOAD(fileXio, "R5 fileXio");
    LOAD(ps2dev9, "R6 ps2dev9");
    LOAD(ps2atad, "R7 ps2atad");
    result = module("R8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("R9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;
    before("R10 fileXioInit");
    result = fileXioInit();
    scr_printf("R10 fileXioInit=%d\n", result);
    if (result < 0) goto done;
    LOAD(sio2man, "R11 sio2man");
    LOAD(mcman, "R12 mcman");
    LOAD(mcserv, "R13 mcserv");
    LOAD(freepad, "R14 freepad");
    before("R15 padInit");
    rv = padInit(0);
    scr_printf("R15 padInit=%d\n", rv);
    if (!rv) { result = -110; goto done; }
    before("R16 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("R16 padPortOpen=%d\n", rv);
    if (!rv) { padEnd(); result = -111; goto done; }
    have_pad = 1;

    result = directory("R17 hdd0", "hdd0:");
    if (result < 0) goto done;
    before("R18 PFS1 COLLISION");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("R18 PFS1 DOPEN=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        scr_printf("PFS1 ALREADY OWNED BY OTHER; STOP\n");
        result = -112;
        goto done;
    }
    before("R19 PP.TESTDVR MOUNT RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("R19 MOUNT=%d\n", mount_code);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;
    result = directory("R20 PS2 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto done;
    boot_cnf = read_config();
    boot_elf = inspect_file("R22 BOOT.ELF", PFS_MOUNTPOINT "/BOOT.ELF", 1);
    neutrino_elf = inspect_file("R23 neutrino.elf", PFS_MOUNTPOINT "/neutrino.elf", 1);
    q_config_ok = inspect_q_config();
    f_target_size = inspect_file("R TEST F", F_TARGET_PATH, 1);

    before("R24 sceCdInit");
    rv = sceCdInit(SCECdINoD);
    scr_printf("R24 CdInit=%d\n", rv);
    if (rv <= 0) { result = -113; goto done; }
    before("R25 CdNoticeGameStart");
    rv = sceCdNoticeGameStart(0, &notice_arg);
    scr_printf("R25 NOTICE=%d arg=%u\n", rv, notice_arg);
    if (rv < 0) { result = rv; goto done; }
    usleep(500000);
    LOAD(dvrdrv, "R DVRDRV");
    LOAD(dvrfile, "R DVRFILE");
    iso_code = directory("R DVR ISO", "dvr_pfs0:/ISO");
    pfs_code = directory("R PFS AGAIN", PFS_MOUNTPOINT "/");
    if (has_dvd) dvd_open_code = probe_selected_iso();
    result = -136;
    if (iso_code >= 0 && pfs_code >= 0 &&
        boot_cnf > 0 && boot_elf > 0 && neutrino_elf > 0 &&
        q_config_ok == (int)sizeof(q_expected_config) - 1 && f_target_size > 0 &&
        config_type == 1 && bsd_dvr && has_dvd &&
        !usb_dep && dvd_open_code >= 0)
        result = 0;

done:
    scr_clear();
    scr_printf("PSX DVR DIAG R2 / CNF BYTES\n");
    scr_printf("CHECK=%d MOUNT=%d RCNF=%d\n",
               result, mount_code, q_config_ok);
    scr_printf("SIZE=%d EXPECT=%d READ=%d\n",
               r2_real_len, r2_expected_len, r2_read_len);
    scr_printf("DIFF OFFSET=%d E=%02X A=%02X\n",
               r2_mismatch, r2_expected_byte, r2_actual_byte);
    scr_printf("CR=%d LF=%d NUL=%d\n",
               r2_cr_count, r2_lf_count, r2_nul_count);
    scr_printf("SUM EXPECT=%u READ=%u\n", r2_exp_sum, r2_got_sum);
    scr_printf("WINDOW AT %d (UP TO 8 BYTES):\n", r2_window_start);
    print_r2_bytes("E: ", r2_exp_window, r2_window_count);
    print_r2_bytes("A: ", r2_got_window, r2_window_count);
    scr_printf("READONLY. NO ELF LAUNCH.\n");
    if (!wait_for_choice(0)) {
        /* Ordinary START exit: release exactly our own mount, then pad. */
        owned_unmount();
        padPortClose(0, 0);
        padEnd();
        return 0;
    }

    /* wait_for_choice(0) never returns 1: there is no launch path. */
    return 0;
}
