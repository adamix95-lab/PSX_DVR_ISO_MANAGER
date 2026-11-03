/*
 * DIAG Y2 launcher. Uses proven R3/T read-only preflight,
 * strict alternate DIAG_Y2P.CNF with last LF/NUL/01 on ignored pad line ONLY, and
 * targets Neutrino Y2: snapshots backend magic before fhi_config_init, no ISO/game.
 * Does not launch Neutrino or games. USB v0.8 and BOOT.CNF untouched.
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
#define HDD_CNF_ARG "hdd0:PP.TESTDVR/DIAG_Y2P.CNF"
#define Q_CNF_PATH PFS_MOUNTPOINT "/DIAG_Y2P.CNF"
#define F_TARGET_PATH PFS_MOUNTPOINT "/PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF"
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
static int q_last_byte = -999;
static int q_last_is_nul;
static int f_target_size = -999;

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
 * Reject BOM, CRLF, inline comments and any other trailing bytes. The R
 * config is a different file; NEVER overwrite the user's BOOT.CNF.
 */
static const char q_expected_config[] =
    "boot=PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF\n"
    "arg=-bsd=dvr\n"
    "arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso\n"
    "arg=-dev9=NICHDD\n"
    "#PAD\n";
static char q_config_bytes[sizeof(q_expected_config)] __attribute__((aligned(64)));

static int inspect_q_config(void)
{
    int fd, length, got, rc, closing;
    before("R QCNF EXACT CHECK");
    fd = fileXioOpen(Q_CNF_PATH, FIO_O_RDONLY, 0);
    scr_printf("R QCNF OPEN=%d\n", fd);
    if (fd < 0) return fd;
    rc = -340;
    length = fileXioLseek(fd, 0, SEEK_END);
    if (length < 0) { rc = length; goto done_config; }
    if (length != (int)sizeof(q_expected_config) - 1) {
        rc = -341;
        goto done_config;
    }
    if (fileXioLseek(fd, 0, SEEK_SET) < 0) {
        rc = -342;
        goto done_config;
    }
    got = fileXioRead(fd, q_config_bytes, length);
    if (got != length) {
        rc = got < 0 ? got : -343;
        goto done_config;
    }
    /* Hardware: DIAG Y2.CNF (109 B) was read with final 01 (not LF).
     * Use a separate, ignored '#PAD' line after the valid arg lines.
     * OSDMenu's quickboot parser skips every line with no '='.
     * We require all original lines INCLUDING '-dev9=NICHDD\n'
     * and literal '#PAD' byte-for-byte. Only final padding byte
     * may be LF, NUL or observed 01. Never accept 01 in an arg.
     */
    q_last_byte = (unsigned char)q_config_bytes[length - 1];
    if (memcmp(q_expected_config, q_config_bytes, (size_t)(length - 1)) ||
        (q_last_byte != '\n' && q_last_byte != 0 && q_last_byte != 1)) {
        rc = -344;
        goto done_config;
    }
    q_last_is_nul = (q_last_byte == 0);
    rc = length;
done_config:
    closing = fileXioClose(fd);
    if (closing < 0 && rc >= 0) rc = closing;
    scr_printf("V CNF VALID=%d\n", rc);
    return rc;
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
        scr_printf("X: OSDMENU -> Y2 DVR FHI VERIFY\n");
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
    scr_printf("PSX DVR NEUTRINO Y2P / X-GATED LAUNCH\n");
    scr_printf("RO PREFLIGHT. Y2PCNF PADDED; SAME Y2.\n");
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
    scr_printf("PSX DVR NEUTRINO Y2 / RESULTS\n");
    scr_printf("CHECK=%d MOUNT=%d\n", result, mount_code);
    scr_printf("CNF=%d ELF=%d NEUT=%d\n", boot_cnf, boot_elf, neutrino_elf);
    scr_printf("SYS=%d BSD=%d DVD=%d USB=%d\n",
               config_type == 1, bsd_dvr, has_dvd, usb_dep);
    scr_printf("ISO=%d DVR=%d PFS=%d\n", dvd_open_code, iso_code, pfs_code);
    scr_printf("Y2PCNF=%d TARGET ELF BYTES=%d\n", q_config_ok, f_target_size);
        scr_printf("Y2PCNF PREFIX EXACT. LAST=%02X NUL=%d\n",
               q_last_byte, q_last_is_nul);
    scr_printf("Y2 VERIFY DVR FHI. NO ISO.\n");
    if (!wait_for_choice(result == 0)) {
        /* Ordinary START exit: release exactly our own mount, then pad. */
        owned_unmount();
        padPortClose(0, 0);
        padEnd();
        return 0;
    }

    /* X only: all file handles were closed by inspection routines.
     * Keep our read-only pfs1: mount alive until LoadELFFromFile has read
     * BOOT.ELF into EE RAM. PS2SDK's loader then resets IOP for the target,
     * invalidating the in-memory mount. Do not unmount before ELF read.
     * OSDMenu receives an explicit hdd0:.../BOOT.CNF argument, so it
     * initializes APA/PFS itself instead of treating pfs1: as its device.
     *
     * There may be no return from a successful chainload. Launched
     * OSDMenu / Y_TARGET have their own behavior; DIAG Y2P never writes HDD.
     */
    scr_clear();
    scr_printf("DIAG Y2P / PADDED CNF -> OSDMENU\n");
    scr_printf("ELF: pfs1:/BOOT.ELF (PP.TESTDVR)\n");
    scr_printf("CNF: hdd0:PP.TESTDVR/DIAG_Y2P.CNF\n");
    scr_printf("EXPECTED TARGET: Y2 FHI PREINIT TEST.\n");
    scr_printf("Y2P BEFORE ORIGINAL OSDMENU\n");
    sleep(2);
    padPortClose(0, 0);
    padEnd();
    rv = LoadELFFromFile(LOADED_ELF, 1, launcher_args);

    /* If the loader returns, IOP state is not guaranteed; avoid further
     * HDD, pad or IOP RPC calls. Report exact error and request reboot. */
    scr_clear();
    scr_printf("Y2P CHAINLOAD RETURNED=%d\n", rv);
    scr_printf("NO MORE IOP CALLS. PHOTO / REBOOT.\n");
    for (;;) sleep(1);
    return 1;
}
