/*
 * DIAG S — read-only local Neutrino prerequisites and ISO PVD probe.
 * External UDPFS standalone diagnostic; NEVER executes OSDMenu, Neutrino or a game.
 * Checks sample config paths inferred from ps2max32/neutrino's upstream loader:
 * config/system.toml, config/bsd-dvr.toml, config/compat.toml,
 * config/emu-dvd-file.toml; also checks flat-file fallbacks.
 * This cannot prove the exact packed neutrino.elf version or its working directory.
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
#include <string.h>
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
static int romver_bytes = -999, neutrino_mips = -999;
static int cfg_sys = -999, cfg_sys_flat = -999;
static int cfg_dvr = -999, cfg_dvr_flat = -999;
static int cfg_compat = -999, cfg_compat_flat = -999;
static int cfg_dvd = -999, cfg_dvd_flat = -999, module_dir = -999;
static int iso_pvd = -999, iso_seek = -999, iso_read = -999;
static int iso_file_open = -999;
static unsigned char probe_buf[64] __attribute__((aligned(64)));

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

static int read_config(void)
{
    int fd, n, got, rv;
    int result = -130;
    const char *p;
    before("S21 BOOT.CNF TEXT");
    fd = fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0);
    scr_printf("S21 CNF OPEN=%d\n", fd);
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

static int quiet_file_size(const char *path)
{
    int fd, n, rv;
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    if (fd < 0) return fd;
    n = fileXioLseek(fd, 0, SEEK_END);
    rv = fileXioClose(fd);
    return n < 0 ? n : (rv < 0 ? rv : n);
}

static int quiet_directory(const char *path)
{
    int fd, rv;
    fd = fileXioDopen(path);
    if (fd < 0) return fd;
    rv = fileXioDclose(fd);
    return rv;
}

static int probe_romver(void)
{
    int fd, n, rv;
    before("S21 rom0 ROMVER READ");
    fd = fileXioOpen("rom0:ROMVER", FIO_O_RDONLY, 0);
    if (fd < 0) return fd;
    memset(probe_buf, 0, sizeof(probe_buf));
    n = fileXioRead(fd, probe_buf, 15);
    rv = fileXioClose(fd);
    if (n < 0) return n;
    if (rv < 0) return rv;
    return n;
}

static int probe_neutrino_mips(void)
{
    int fd, n, rv;
    before("S22 NEUTRINO ELF MIPS HEADER");
    fd = fileXioOpen(PFS_MOUNTPOINT "/neutrino.elf", FIO_O_RDONLY, 0);
    if (fd < 0) return fd;
    memset(probe_buf, 0, sizeof(probe_buf));
    n = fileXioRead(fd, probe_buf, 20);
    rv = fileXioClose(fd);
    if (n != 20) return n < 0 ? n : -221;
    if (rv < 0) return rv;
    return (probe_buf[0] == 0x7f && probe_buf[1] == 'E' &&
            probe_buf[2] == 'L' && probe_buf[3] == 'F' &&
            probe_buf[4] == 1 && probe_buf[5] == 1 &&
            probe_buf[18] == 8 && probe_buf[19] == 0) ? 1 : 0;
}

/* Reads a maximum of six bytes from ISO9660 volume descriptor at LBA16.
 * No ISO file is created or opened for write. A negative seek can indicate
 * a driver limitation, not necessarily an invalid disc image.
 */
static int probe_iso9660_pvd(void)
{
    int fd, n, rv, result;
    if (!has_dvd) return -230;
    before("S30 SELECTED ISO PVD RO");
    fd = fileXioOpen(dvd_path, FIO_O_RDONLY, 0);
    iso_file_open = fd;
    if (fd < 0) return fd;
    iso_seek = fileXioLseek(fd, 16 * 2048, SEEK_SET);
    result = iso_seek;
    if (iso_seek != 16 * 2048) {
        if (iso_seek >= 0) result = -231;
        goto close_iso;
    }
    memset(probe_buf, 0, sizeof(probe_buf));
    n = fileXioRead(fd, probe_buf, 6);
    iso_read = n;
    if (n != 6) {
        result = n < 0 ? n : -232;
        goto close_iso;
    }
    result = (probe_buf[0] == 1 &&
              memcmp(probe_buf + 1, "CD001", 5) == 0) ? 1 : 0;
close_iso:
    rv = fileXioClose(fd);
    if (rv < 0 && result >= 0) result = rv;
    return result;
}

static void probe_local_config_candidates(void)
{
    before("S23 LOCAL CONFIG FILES RO");
    cfg_sys = quiet_file_size(PFS_MOUNTPOINT "/config/system.toml");
    cfg_sys_flat = quiet_file_size(PFS_MOUNTPOINT "/system.toml");
    cfg_dvr = quiet_file_size(PFS_MOUNTPOINT "/config/bsd-dvr.toml");
    cfg_dvr_flat = quiet_file_size(PFS_MOUNTPOINT "/bsd-dvr.toml");
    cfg_compat = quiet_file_size(PFS_MOUNTPOINT "/config/compat.toml");
    cfg_compat_flat = quiet_file_size(PFS_MOUNTPOINT "/compat.toml");
    cfg_dvd = quiet_file_size(PFS_MOUNTPOINT "/config/emu-dvd-file.toml");
    cfg_dvd_flat = quiet_file_size(PFS_MOUNTPOINT "/emu-dvd-file.toml");
    module_dir = quiet_directory(PFS_MOUNTPOINT "/modules");
}

static void owned_unmount(void)
{
    int rv;
    if (!have_our_mount) return;
    before("S26 OWN PFS1 UMOUNT");
    rv = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("S26 UMOUNT=%d\n", rv);
    /* Never attempt a second unmount of someone else's mount. */
    have_our_mount = 0;
}

static void wait_for_start(void)
{
    int prev_buttons = 0, sampled = 0, bits, fresh;
    struct padButtonStatus btn;
    if (!have_pad) {
        scr_printf("PAD NOT READY: PHOTO/REBOOT\n");
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
}

int main(void)
{
    int result = -999;
    int rv, occupied;
    u32 notice_arg = 0;
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR HDD DIAG S / READ ONLY\n");
    scr_printf("NO NEUTRINO OR GAME EXECUTION\n");
    before("S1 IOP RESET");
    while (!SifIopReset("", 0)) {}
    before("S2 IOP SYNC");
    while (!SifIopSync()) {}
    before("S3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define LOAD(name, label) \
    do { \
        result = module(label, name##_irx, size_##name##_irx, NULL, 0); \
        if (result < 0) goto done; \
    } while (0)
    LOAD(iomanX, "S4 iomanX");
    LOAD(fileXio, "S5 fileXio");
    LOAD(ps2dev9, "S6 ps2dev9");
    LOAD(ps2atad, "S7 ps2atad");
    result = module("S8 ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
    if (result < 0) goto done;
    result = module("S9 ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
    if (result < 0) goto done;
    before("S10 fileXioInit");
    result = fileXioInit();
    if (result < 0) goto done;
    LOAD(sio2man, "S11 sio2man");
    LOAD(mcman, "S12 mcman");
    LOAD(mcserv, "S13 mcserv");
    LOAD(freepad, "S14 freepad");
    before("S15 padInit");
    rv = padInit(0);
    if (!rv) { result = -110; goto done; }
    before("S16 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    if (!rv) { padEnd(); result = -111; goto done; }
    have_pad = 1;

    result = directory("S17 hdd0", "hdd0:");
    if (result < 0) goto done;
    before("S18 PFS1 COLLISION");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    if (occupied >= 0) {
        fileXioDclose(occupied);
        result = -112;
        goto done;
    }
    before("S19 PP.TESTDVR MOUNT RO");
    mount_code = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    result = mount_code;
    if (result < 0) goto done;
    have_our_mount = 1;
    result = directory("S20 PFS1", PFS_MOUNTPOINT "/");
    if (result < 0) goto done;

    boot_cnf = read_config();
    boot_elf = inspect_file("S BOOT.ELF", PFS_MOUNTPOINT "/BOOT.ELF", 1);
    neutrino_elf = inspect_file("S NEUTRINO", PFS_MOUNTPOINT "/neutrino.elf", 1);
    romver_bytes = probe_romver();
    neutrino_mips = probe_neutrino_mips();
    probe_local_config_candidates();

    before("S24 sceCdInit");
    rv = sceCdInit(SCECdINoD);
    if (rv <= 0) { result = -113; goto done; }
    before("S25 CdNoticeGameStart");
    rv = sceCdNoticeGameStart(0, &notice_arg);
    if (rv < 0) { result = rv; goto done; }
    usleep(500000);
    LOAD(dvrdrv, "S26 DVRDRV");
    LOAD(dvrfile, "S27 DVRFILE");
#undef LOAD
    iso_code = directory("S28 DVR ISO DIR", "dvr_pfs0:/ISO");
    pfs_code = directory("S29 PFS AGAIN", PFS_MOUNTPOINT "/");
    iso_pvd = probe_iso9660_pvd();

    /* Result=0 means the read-only probe completed, NOT that the original
     * neutrino.elf initialized or that a game can launch. */
    result = (mount_code >= 0 && boot_cnf > 0 && neutrino_elf > 0 &&
              neutrino_mips == 1 && iso_code >= 0 && pfs_code >= 0 &&
              config_type == 1 && bsd_dvr && has_dvd && !usb_dep) ? 0 : -250;

done:
    scr_clear();
    scr_printf("PSX DVR HDD DIAG S / RESULTS\n");
    scr_printf("CHECK=%d MOUNT=%d\n", result, mount_code);
    scr_printf("CNF=%d OSD=%d NEUT=%d\n", boot_cnf, boot_elf, neutrino_elf);
    scr_printf("ROMVER=%d MIPS=%d\n", romver_bytes, neutrino_mips);
    scr_printf("BSD=%d DVD=%d DVRDIR=%d\n", bsd_dvr, has_dvd, iso_code);
    scr_printf("SYS CFG=%d FLAT=%d\n", cfg_sys, cfg_sys_flat);
    scr_printf("BSD DVR CFG=%d FLAT=%d\n", cfg_dvr, cfg_dvr_flat);
    scr_printf("COMPAT CFG=%d FLAT=%d\n", cfg_compat, cfg_compat_flat);
    scr_printf("DVD CFG=%d FLAT=%d\n", cfg_dvd, cfg_dvd_flat);
    scr_printf("MODULES DIR=%d\n", module_dir);
    scr_printf("ISO FD=%d PVD=%d\n", iso_file_open, iso_pvd);
    scr_printf("ISO SEEK=%d READ=%d\n", iso_seek, iso_read);
    scr_printf("LOCAL CONFIG CANDIDATES ONLY\n");
    scr_printf("NO TARGET EXECUTION. PHOTO.\n");
    scr_printf("START: EXIT TO XMB/OSD\n");
    wait_for_start();
    owned_unmount();
    if (have_pad) {
        padPortClose(0, 0);
        padEnd();
    }
    return 0;
}
