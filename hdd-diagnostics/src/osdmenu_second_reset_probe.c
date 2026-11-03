/*
 * DIAG P TARGET - OSDMenu-style second IOP reset inside handlePFS after Quickboot.
 *
 * The original BOOT.ELF/BOOT.CNF/Neutrino and Sony system partitions remain
 * untouched. No application disk writes, no execution of OSDMenu, Neutrino or game.
 *
 * WARNING: Third-party IOP driver internals are not proven write-free.
 * Run only with a backed-up disk, at the owner's explicit discretion.
 */
#include <debug.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
    scr_printf("O%d %s BEFORE\n", n, label);
    sleep(2);
}
static int modload(int n, const char *label, void *buf, uint32_t len,
                   const void *args, uint32_t argc)
{
    int id, status = -999;
    before(n, label);
    id = SifExecModuleBuffer(buf, len, argc, (void *)args, &status);
    scr_printf("P%d %s id=%d rv=%d\n", n, label, id, status);
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
    scr_printf("P%d DOPEN=%d\n", n, fd);
    if (fd < 0) return fd;
    rc = fileXioDclose(fd);
    scr_printf("P%d DCLOSE=%d\n", n, rc);
    return rc;
}
static int file_size(int n, const char *path, int elf)
{
    int fd, size, got, rv, rc;
    static unsigned char magic[16] __attribute__((aligned(64)));
    before(n, path);
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    scr_printf("P%d OPEN=%d\n", n, fd);
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
    scr_printf("P%d SIZE/RC=%d\n", n, rc);
    return rc;
}
static int read_cnf(void)
{
    int fd, size, rv, got, rc;
    before(24, "BOOT.CNF READ ONLY");
    fd = fileXioOpen(PFS_MOUNTPOINT "/BOOT.CNF", FIO_O_RDONLY, 0);
    scr_printf("P24 CNF OPEN=%d\n", fd);
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
    scr_printf("P24 CNF BYTES=%d\n", rc);
    return rc;
}
static int unmount_owned_at(int step)
{
    int rc;
    if (!own_mount) return 0;
    before(step, "OWN PFS0 UMOUNT");
    rc = fileXioUmount(PFS_MOUNTPOINT);
    scr_printf("O%d UMOUNT=%d\n", step, rc);
    /* No unmount is repeated against a failed/unverifiable mount. */
    own_mount = 0;
    return rc;
}

/* Read-only Quickboot path/stdio probe, modeled after upstream
 * pcm720/OSDMenu launcher/src/handler_quickboot.c. Never launch the target.
 */
static char qb_cnf[128], qb_hdd_elf[192], qb_pfs_elf[192];
static int qb_stdio = -999, qb_errno, qb_boots, qb_paths, qb_args;
static int qb_bsd, qb_dvd, qb_usb, qb_elf_size = -999;
static int q_first_umount = -999;
static int q_remount = -999;
static int q_tryfile = -999;
static int q_final_umount = -999;
static int q_path_ok;
static int q_second_reset_done;
static int q_second_modules_done;
static int q_second_pad_ready;
static char q_loader_path[192];
static int quickboot_probe(const char *argument)
{
    char config[160], partition[160], basedir[160], row[256];
    char *split, *slash, *eq, *value;
    FILE *fp;
    size_t len;
    int n;
    before(28, "QUICKBOOT NORMALIZE");
    if (!argument || strncmp(argument, "hdd", 3) != 0) return -260;
    len = strlen(argument);
    if (len < 5 || len >= sizeof(config)) return -261;
    memcpy(config, argument, len + 1);
    if (strcmp(config + len - 4, ".CNF") != 0 &&
        strcmp(config + len - 4, ".cnf") != 0) return -262;
    memcpy(partition, config, len + 1);
    split = strstr(partition, ":pfs:");
    if (!split) split = strchr(partition, '/');
    if (!split) return -263;
    *split = '\0';
    if (strcmp(partition, TEST_PARTITION) != 0) return -264;
    slash = strchr(config, '/');
    if (!slash || strchr(slash + 1, '/')) return -265;
    n = snprintf(qb_cnf, sizeof(qb_cnf), "%s%s", PFS_MOUNTPOINT, slash);
    if (n < 0 || n >= (int)sizeof(qb_cnf)) return -266;
    scr_printf("P CNF PATH: %s\n", qb_cnf);

    before(29, "QUICKBOOT STDIP FOPEN");
    errno = 0;
    fp = fopen(qb_cnf, "r");
    if (!fp) {
        qb_errno = errno;
        qb_stdio = -1;
        scr_printf("O FOPEN FAIL errno=%d\n", qb_errno);
        return -267;
    }
    qb_stdio = 0;
    /* Upstream launchTarget strips trailing '/BOOT.CNF'. */
    memcpy(basedir, config, len + 1);
    slash = strrchr(basedir, '/');
    if (!slash) { fclose(fp); return -268; }
    *slash = '\0';

    before(30, "QUICKBOOT FGETS PARSE");
    while (fgets(row, sizeof(row), fp)) {
        eq = strchr(row, '=');
        if (!eq) continue;
        *eq = '\0';
        value = eq;
        do { value++; } while (*value && isspace((unsigned char)*value));
        value[strcspn(value, "\r\n")] = '\0';

        if (!strncmp(row, "boot", 4) && *value) {
            if (qb_boots == 0) {
                if (value[0] == '/' || strchr(value, ':') ||
                    strstr(value, "..")) {
                    fclose(fp);
                    return -269;
                }
                n = snprintf(qb_hdd_elf, sizeof(qb_hdd_elf),
                             "%s/%s", basedir, value);
                if (n < 0 || n >= (int)sizeof(qb_hdd_elf)) {
                    fclose(fp); return -270;
                }
                n = snprintf(qb_pfs_elf, sizeof(qb_pfs_elf),
                             "%s/%s", PFS_MOUNTPOINT, value);
                if (n < 0 || n >= (int)sizeof(qb_pfs_elf)) {
                    fclose(fp); return -271;
                }
            }
            qb_boots++;
        } else if (!strncmp(row, "path", 4) && *value) {
            qb_paths++;
        } else if (!strncmp(row, "arg", 3) && *value) {
            qb_args++;
            if (!strcmp(value, "-bsd=dvr")) qb_bsd = 1;
            if (!strncmp(value, "-dvd=dvr_pfs0:/ISO/", 19)) qb_dvd = 1;
            if (strstr(value, "mass0:") || strstr(value, "usb0:"))
                qb_usb = 1;
        }
    }
    if (ferror(fp)) { fclose(fp); return -272; }
    if (fclose(fp) != 0) return -273;
    scr_printf("P BOOT=%d PATH=%d ARGS=%d\n", qb_boots, qb_paths, qb_args);
    scr_printf("P BSD=%d DVD=%d USB=%d\n", qb_bsd, qb_dvd, qb_usb);
    if (qb_boots != 1 || !qb_bsd || !qb_dvd || qb_usb) return -274;

    before(31, "QUICKBOOT ELF TARGET");
    scr_printf("P TARGET HDD: %s\n", qb_hdd_elf);
    qb_elf_size = file_size(31, qb_pfs_elf, 1);
    return qb_elf_size > 0 ? 0 : (qb_elf_size < 0 ? qb_elf_size : -275);
}


/* Isolates upstream handlePFS() AFTER Quickboot released its pfs0:.
 * This DIAG P now performs the SECOND IOP reset, restarts the known-good
 * OSDMenu-like module order, reinitializes the controller, and checks the
 * remount/target file and path, all without executing Neutrino.
 *
 * Upstream handler_pfs.c:
 *  - rejects "hdd?:/" malformed path
 *  - remounts APA to pfs0:
 *  - tryFile("pfs0:/neutrino.elf")
 *  - deinitPFS()
 *  - constructs "hdd0:PP.TESTDVR:pfs:/neutrino.elf"
 *  - calls a loader (that last step is intentionally excluded here).
 */
static int handlepfs_probe(void)
{
    char norm[160];
    const char *suffix;
    int rv, fd, n, check_fd;

    before(33, "HANDLEPFS PATH DISPATCH");
    if (strncmp(qb_hdd_elf, TEST_PARTITION, strlen(TEST_PARTITION)))
        return -281;
    suffix = qb_hdd_elf + strlen(TEST_PARTITION);
    if (*suffix != '/' || suffix[1] == '\0' ||
        strchr(suffix + 1, '/') || strstr(suffix, ".."))
        return -282;
    if (strncmp(qb_hdd_elf, "hdd0:", 5) || qb_hdd_elf[5] == '/')
        return -283;
    n = snprintf(norm, sizeof(norm), "%s%s", PFS_MOUNTPOINT, suffix);
    if (n < 0 || n >= (int)sizeof(norm)) return -284;

    /*
     * Unlike DIAG P, emulate upstream handlePFS() -> initPFS() ->
     * initModules(Device_APA): reset IOP again AFTER Quickboot unmounts.
     * EE memory (including qb_hdd_elf, staging buffers and screen) remains.
     * Close the previous pad RPC before reset and initialize it again after
     * reloading freepad. Never operate on the old PFS mount after reset.
     */
    before(40, "SECOND IOP RESET");
    if (own_mount) return -300; /* Quickboot must have unmounted. */
    if (have_pad) {
        padPortClose(0, 0);
        padEnd();
        have_pad = 0;
    }
    while (!SifIopReset("", 0)) {}
    q_second_reset_done = 1;
    before(41, "SECOND IOP SYNC");
    while (!SifIopSync()) {}
    before(42, "SECOND RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();

#define SECOND_LOAD(name, n, desc) \
    do { rv = modload(n, desc, name##_irx, size_##name##_irx, NULL, 0); \
         if (rv < 0) return rv; } while (0)
    SECOND_LOAD(iomanX, 43, "2ND iomanX");
    SECOND_LOAD(fileXio, 44, "2ND fileXio");
    SECOND_LOAD(sio2man, 45, "2ND sio2man");
    SECOND_LOAD(mcman, 46, "2ND mcman");
    SECOND_LOAD(mcserv, 47, "2ND mcserv");
    SECOND_LOAD(freepad, 48, "2ND freepad");
    before(49, "SECOND padInit");
    rv = padInit(0);
    scr_printf("P49 PAD INIT=%d\n", rv);
    if (!rv) return -301;
    before(50, "SECOND padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("P50 PAD OPEN=%d\n", rv);
    if (!rv) {
        padEnd();
        return -302;
    }
    have_pad = 1;
    q_second_pad_ready = 1;

    SECOND_LOAD(ps2dev9, 51, "2ND ps2dev9");
    SECOND_LOAD(bdm, 52, "2ND bdm");
    SECOND_LOAD(bdmfs_fatfs, 53, "2ND bdmfs_fatfs");
    SECOND_LOAD(ata_bd, 54, "2ND ata_bd");
    sleep(1);
    rv = modload(55, "2ND ps2hdd-osd", ps2hdd_osd_irx,
                 size_ps2hdd_osd_irx, hdd_arguments, sizeof(hdd_arguments));
    if (rv < 0) return rv;
    rv = modload(56, "2ND ps2fs", ps2fs_irx,
                 size_ps2fs_irx, pfs_arguments, sizeof(pfs_arguments));
    if (rv < 0) return rv;
    SECOND_LOAD(secrsif, 57, "2ND secrsif");
#undef SECOND_LOAD
    q_second_modules_done = 1;
    before(58, "SECOND fileXioInit");
    rv = fileXioInit();
    scr_printf("P58 FILEXIO=%d\n", rv);
    if (rv < 0) return rv;

    before(59, "SECOND HDD DOPEN");
    fd = fileXioDopen("hdd0:");
    scr_printf("P59 HDD FD=%d\n", fd);
    if (fd < 0) return fd;
    rv = fileXioDclose(fd);
    if (rv < 0) return rv;

    before(60, "SECOND PFS0 COLLISION");
    check_fd = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("P60 DOPEN=%d\n", check_fd);
    if (check_fd >= 0) {
        fileXioDclose(check_fd);
        return -285;
    }

    before(61, "HANDLEPFS REMOUNT RO");
    q_remount = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION,
                             FIO_MT_RDONLY);
    scr_printf("P61 REMOUNT=%d\n", q_remount);
    if (q_remount < 0) return q_remount;
    own_mount = 1;

    /* Match upstream tryFile() rather than N's fileXioOpen(). */
    before(62, "HANDLEPFS POSIX TRYFILE");
    errno = 0;
    fd = open(norm, O_RDONLY);
    q_tryfile = fd;
    scr_printf("P62 TRYFILE FD=%d ERRNO=%d\n", fd, errno);
    if (fd < 0) return -286;
    rv = close(fd);
    if (rv < 0) return -287;

    /* Still skip upstream PDIOC_CLOSEALL and fileXioSync calls;
     * they can introduce extra state changes beyond the RO probe.
     */
    q_final_umount = unmount_owned_at(63);
    if (q_final_umount < 0) return q_final_umount;

    before(64, "HANDLEPFS BUILD LOADER PATH");
    n = snprintf(q_loader_path, sizeof(q_loader_path),
                 "%s:pfs:%s", TEST_PARTITION, suffix);
    if (n < 0 || n >= (int)sizeof(q_loader_path)) return -288;
    q_path_ok = strcmp(q_loader_path,
                       "hdd0:PP.TESTDVR:pfs:/neutrino.elf") == 0;
    scr_printf("P64 PATH=%s\n", q_loader_path);
    return q_path_ok ? 0 : -289;
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
    scr_printf("PSX DVR HDD DIAG P / QUICKBOOT PROBE\n");
    scr_printf("P0 ENTRY FROM HDD\n");
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
    scr_printf("P10 PAD INIT=%d\n", rv);
    if (!rv) { ret = -210; goto done; }
    before(11, "padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("P11 PAD OPEN=%d\n", rv);
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
    scr_printf("P19 FILEXIO=%d\n", rv);
    if (rv < 0) { ret = rv; goto done; }

    hdd_dir = dopen_close(20, "hdd0:");
    if (hdd_dir < 0) { ret = hdd_dir; goto done; }
    before(21, "PFS0 COLLISION CHECK");
    occupied = fileXioDopen(PFS_MOUNTPOINT "/");
    scr_printf("P21 PFS0 EXISTS FD=%d\n", occupied);
    if (occupied >= 0) {
        fileXioDclose(occupied);
        ret = -221;
        goto done;
    }
    before(22, "PP.TESTDVR MOUNT RO");
    mount_rv = fileXioMount(PFS_MOUNTPOINT, TEST_PARTITION, FIO_MT_RDONLY);
    scr_printf("P22 MOUNT=%d\n", mount_rv);
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
    if (!arg_match) { ret = -250; goto done; }
    ret = quickboot_probe(argv[1]);
    if (ret < 0) goto done;
    q_first_umount = unmount_owned_at(32);
    if (q_first_umount < 0) { ret = q_first_umount; goto done; }
    ret = handlepfs_probe();
done:
    umount_rv = unmount_owned_at(65);
    if (ret == 0 && umount_rv < 0) ret = umount_rv;
    scr_clear();
    scr_printf("PSX DVR HDD DIAG P / RESULTS\n");
    scr_printf("RESULT=%d STAGE=%d\n", ret, stage);
    scr_printf("ARG=%d OSDMOD=%d\n", arg_match, osd_modules);
    scr_printf("HDD=%d MOUNT=%d PFS=%d\n", hdd_dir, mount_rv, pfs_dir);
    scr_printf("CNF=%d ELF=%d NEUT=%d\n", cnf_size, boot_elf_size, neutrino_size);
    scr_printf("QB=%d STDIO=%d BOOT=%d\n", qb_elf_size, qb_stdio, qb_boots);
    scr_printf("RESET2=%d MOD2=%d PAD2=%d\n",
               q_second_reset_done, q_second_modules_done, q_second_pad_ready);
    scr_printf("UM1=%d REMOUNT=%d\n", q_first_umount, q_remount);
    scr_printf("TRYFD=%d UM2=%d PATH=%d\n", q_tryfile, q_final_umount, q_path_ok);
    if (q_path_ok) scr_printf("LOADER: %s\n", q_loader_path);
    scr_printf("NO TARGET EXECUTION. PHOTO.\n");
    finish_on_start();
    return 0;
}
