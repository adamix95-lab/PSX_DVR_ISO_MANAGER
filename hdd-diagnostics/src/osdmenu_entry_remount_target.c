/*
 * DIAG U: real OSDMenu target with read-only PFS remount attempt.
 * Evidence from real DIAG T: ARGS_OK=1, HDD0=2, PFS0=-19,
 * open pfs0/relative=-19, POSIX errno=22 on entry.
 * U checks whether a READ-ONLY mount restored after OSDMenu's PFS unmount
 * makes config & DVR IRX accessible, and whether chdir("pfs0:/") works.
 * No writes, no ISO open, no Neutrino or game boot.
 */
#include <debug.h>
#include <fcntl.h>
#include <errno.h>
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
#include <io_common.h>
#include <fileXio_rpc.h>

#define IRX(name) \
    extern unsigned char name##_irx[] __attribute__((aligned(16))); \
    extern uint32_t size_##name##_irx

IRX(iomanX);
IRX(fileXio);
IRX(sio2man);
IRX(mcman);
IRX(mcserv);
IRX(freepad);

static unsigned char padbuf[256] __attribute__((aligned(64)));
#define EXPECTED_DVD "-dvd=dvr_pfs0:/ISO/DIAG_U_NO_GAME.iso"
static char saved_argv0[128];
static char saved_argv1[64];
static char saved_argv2[160];
static int arguments_valid;
static char cwd_text[128] = "(unknown)";
static int t_filexio_init = -999;
static int t_pfs0_dir = -999;
static int t_hdd0_dir = -999;
static int t_pfs_cfg = -999;
static int t_pfs_irx = -999;
static int t_rel_cfg = -999;
static int t_rel_irx = -999;
static int t_posix_cfg = -999;
static int t_posix_irx = -999;
static int t_errno_cfg = -999;
static int t_errno_irx = -999;
static int u_mount_code = -999;
static int u_after_root = -999;
static int u_cfg = -999, u_irx = -999;
static int u_chdir = -999;
static int u_rel_cfg = -999, u_rel_irx = -999;
static int u_errno_cfg = -999, u_errno_irx = -999;
static int u_unmount_code = -999;
static int u_mount_owned;
static char u_after_cwd[128] = "(not attempted)";


static void save_arg(char *dst, size_t cap, const char *arg)
{
    if (!arg) arg = "(null)";
    snprintf(dst, cap, "%s", arg);
}

/* These opens never use create/truncate/write flags. Return open FD or
 * negative failure code; successful files are closed immediately.
 */
static int try_open_rpc(const char *path)
{
    int fd, close_rc;
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    if (fd < 0) return fd;
    close_rc = fileXioClose(fd);
    return close_rc < 0 ? close_rc : fd;
}
static int try_open_posix(const char *path, int *saved_errno)
{
    int fd, status;
    errno = 0;
    fd = open(path, O_RDONLY);
    *saved_errno = errno;
    if (fd < 0) return fd;
    status = close(fd);
    return status < 0 ? status : fd;
}
static void inspect_at_elf_entry(void)
{
    char *cwd;
    scr_printf("T0 TARGET ENTRY. BEFORE IOP RESET\n");
    scr_printf("T1 SifInitRpc BEGIN\n");
    SifInitRpc(0);
    scr_printf("T1 SifInitRpc DONE\n");
    cwd = getcwd(cwd_text, sizeof(cwd_text));
    if (!cwd) snprintf(cwd_text, sizeof(cwd_text), "(getcwd failed)");
    scr_printf("T2 CWD: %.64s\n", cwd_text);
    scr_printf("T3 fileXioInit BEGIN\n");
    t_filexio_init = fileXioInit();
    scr_printf("T3 INIT=%d\n", t_filexio_init);
    if (t_filexio_init < 0) return;
    scr_printf("T4 PFS0 ROOT DOPEN BEGIN\n");
    t_pfs0_dir = fileXioDopen("pfs0:/");
    if (t_pfs0_dir >= 0) {
        int fd = t_pfs0_dir;
        fileXioDclose(fd);
    }
    scr_printf("T4 PFS0 DOPEN=%d\n", t_pfs0_dir);
    scr_printf("T5 HDD0 ROOT DOPEN BEGIN\n");
    t_hdd0_dir = fileXioDopen("hdd0:");
    if (t_hdd0_dir >= 0) {
        int fd = t_hdd0_dir;
        fileXioDclose(fd);
    }
    scr_printf("T5 HDD0 DOPEN=%d\n", t_hdd0_dir);
    scr_printf("T6 RPC PFS CFG BEGIN\n");
    t_pfs_cfg = try_open_rpc("pfs0:/config/system.toml");
    scr_printf("T6 CFG=%d\n", t_pfs_cfg);
    scr_printf("T7 RPC PFS IRX BEGIN\n");
    t_pfs_irx = try_open_rpc("pfs0:/modules/dvrfile_fhi.irx");
    scr_printf("T7 IRX=%d\n", t_pfs_irx);
    scr_printf("T8 RPC REL CFG BEGIN\n");
    t_rel_cfg = try_open_rpc("config/system.toml");
    scr_printf("T8 REL CFG=%d\n", t_rel_cfg);
    scr_printf("T9 RPC REL IRX BEGIN\n");
    t_rel_irx = try_open_rpc("modules/dvrfile_fhi.irx");
    scr_printf("T9 REL IRX=%d\n", t_rel_irx);
    scr_printf("T10 POSIX CFG BEGIN\n");
    t_posix_cfg = try_open_posix("config/system.toml", &t_errno_cfg);
    scr_printf("T10 POSIX CFG=%d ERRNO=%d\n", t_posix_cfg, t_errno_cfg);
    scr_printf("T11 POSIX IRX BEGIN\n");
    t_posix_irx = try_open_posix("modules/dvrfile_fhi.irx", &t_errno_irx);
    scr_printf("T11 POSIX IRX=%d ERRNO=%d\n", t_posix_irx, t_errno_irx);
    scr_printf("T12 BEFORE IOP RESET PROBES COMPLETE\n");
}

/* Mount only if T established that PFS0 is not already accessible.
 * We own the mount only on success, and then unmount before resetting IOP.
 */
static void inspect_after_remount(void)
{
    int d;
    char *cwd;
    if (t_filexio_init < 0 || t_hdd0_dir < 0 || t_pfs0_dir >= 0) {
        scr_printf("U REMOUNT SKIPPED: PRECONDITION\n");
        return;
    }
    scr_printf("U1 RO MOUNT BEGIN\n");
    u_mount_code = fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY);
    scr_printf("U1 MOUNT=%d\n", u_mount_code);
    if (u_mount_code < 0) return;
    u_mount_owned = 1;
    scr_printf("U2 PFS0 DOPEN BEGIN\n");
    d = fileXioDopen("pfs0:/");
    u_after_root = d;
    if (d >= 0) fileXioDclose(d);
    scr_printf("U2 PFS0 ROOT=%d\n", u_after_root);
    scr_printf("U3 CONFIG RO OPEN BEGIN\n");
    u_cfg = try_open_rpc("pfs0:/config/system.toml");
    scr_printf("U3 CONFIG=%d\n", u_cfg);
    scr_printf("U4 DVR IRX RO OPEN BEGIN\n");
    u_irx = try_open_rpc("pfs0:/modules/dvrfile_fhi.irx");
    scr_printf("U4 DVR IRX=%d\n", u_irx);
    scr_printf("U5 CHDIR PFS0 BEGIN\n");
    u_chdir = chdir("pfs0:/");
    scr_printf("U5 CHDIR=%d\n", u_chdir);
    if (u_chdir >= 0) {
        cwd = getcwd(u_after_cwd, sizeof(u_after_cwd));
        if (!cwd) snprintf(u_after_cwd, sizeof(u_after_cwd), "(getcwd failed)");
    }
    scr_printf("U6 RELATIVE POSIX CFG BEGIN\n");
    u_rel_cfg = try_open_posix("config/system.toml", &u_errno_cfg);
    scr_printf("U6 POSIX CFG=%d ERRNO=%d\n", u_rel_cfg, u_errno_cfg);
    scr_printf("U7 RELATIVE POSIX IRX BEGIN\n");
    u_rel_irx = try_open_posix("modules/dvrfile_fhi.irx", &u_errno_irx);
    scr_printf("U7 POSIX IRX=%d ERRNO=%d\n", u_rel_irx, u_errno_irx);
    scr_printf("U8 UMOUNT OWN PFS0 BEGIN\n");
    if (u_mount_owned) {
        u_unmount_code = fileXioUmount("pfs0:");
        scr_printf("U8 UMOUNT=%d\n", u_unmount_code);
        if (u_unmount_code == 0) u_mount_owned = 0;
    }
    scr_printf("U9 RO REMOUNT PROBES DONE\n");
}

static void before(const char *label)
{
    scr_printf("%s BEFORE\n", label);
    sleep(2);
}

static int load_module(const char *label, void *data, unsigned size)
{
    int id, rv = -999;
    before(label);
    id = SifExecModuleBuffer(data, size, 0, NULL, &rv);
    scr_printf("%s id=%d rv=%d\n", label, id, rv);
    return id < 0 ? id : rv;
}

int main(int argc, char *argv[])
{
    int rv;
    int bits;
    int prev_buttons = 0;
    int pad_sampled = 0;
    int new_press;
    struct padButtonStatus btn;

    /* Snapshot EE argv before any IOP state changes. No file access. */
    save_arg(saved_argv0, sizeof(saved_argv0), argc > 0 && argv ? argv[0] : NULL);
    save_arg(saved_argv1, sizeof(saved_argv1), argc > 1 && argv ? argv[1] : NULL);
    save_arg(saved_argv2, sizeof(saved_argv2), argc > 2 && argv ? argv[2] : NULL);
    arguments_valid = argc == 3 &&
        strcmp(saved_argv1, "-bsd=dvr") == 0 &&
        strcmp(saved_argv2, EXPECTED_DVD) == 0;

    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();
    scr_printf("PSX DVR DIAG U / ENTRY TEST\n");
    scr_printf("ENTRY: ARGC=%d ARGS_OK=%d\n", argc, arguments_valid);
    scr_printf("READONLY OPEN TEST. NO GAME START\n");

    inspect_at_elf_entry();
    inspect_after_remount();
    before("U10 IOP RESET");
    while (!SifIopReset("", 0)) {}
    scr_printf("R1 RESET SENT\n");
    before("R2 IOP SYNC");
    while (!SifIopSync()) {}
    scr_printf("R2 IOP SYNC DONE\n");
    before("R3 RPC PATCH");
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    scr_printf("R3 RPC PATCH DONE\n");

#define LOAD(name, label) \
    do { \
        rv = load_module(label, name##_irx, size_##name##_irx); \
        if (rv < 0) goto failure; \
    } while (0)
    LOAD(iomanX, "R4 iomanX");
    LOAD(fileXio, "R5 fileXio");
#undef LOAD

    before("R6 fileXioInit");
    rv = fileXioInit();
    scr_printf("R6 fileXioInit=%d\n", rv);
    if (rv < 0) goto failure;

#define LOAD(name, label) \
    do { \
        rv = load_module(label, name##_irx, size_##name##_irx); \
        if (rv < 0) goto failure; \
    } while (0)
    LOAD(sio2man, "R7 sio2man");
    LOAD(mcman, "R8 mcman");
    LOAD(mcserv, "R9 mcserv");
    LOAD(freepad, "R10 freepad");
#undef LOAD

    before("R11 padInit");
    rv = padInit(0);
    scr_printf("R11 padInit=%d\n", rv);
    if (!rv) goto failure;

    before("R12 padPortOpen");
    rv = padPortOpen(0, 0, padbuf);
    scr_printf("R12 padPortOpen=%d\n", rv);
    if (!rv) {
        padEnd();
        goto failure;
    }

    scr_clear();
    scr_printf("PSX DVR HDD DIAG U / RO REMOUNT\n");
    scr_printf("ARGC=%d ARGS_OK=%d\n", argc, arguments_valid);
    scr_printf("CWD ENTRY: %.53s\n", cwd_text);
    scr_printf("FIO=%d PFS_PRE=%d HDD=%d\n",
               t_filexio_init, t_pfs0_dir, t_hdd0_dir);
    scr_printf("MOUNT_RO=%d PFS_AFTER=%d\n", u_mount_code, u_after_root);
    scr_printf("RPC CONFIG=%d DVR IRX=%d\n", u_cfg, u_irx);
    scr_printf("CHDIR=%d NEW CWD: %.47s\n", u_chdir, u_after_cwd);
    scr_printf("POSIX CFG=%d ERRNO=%d\n", u_rel_cfg, u_errno_cfg);
    scr_printf("POSIX IRX=%d ERRNO=%d\n", u_rel_irx, u_errno_irx);
    scr_printf("UMOUNT OWN=%d\n", u_unmount_code);
    scr_printf("RO TEST ONLY. NO NEUTRINO/GAME.\n");
    scr_printf("PHOTO RESULTS. START: XMB/OSD\n");
    /* Mirror Manager v0.8: edge-trigger button polling + return from main. */
    for (;;) {
        if (padRead(0, 0, &btn) > 0) {
            bits = 0xffff ^ btn.btns;
            new_press = bits & ~prev_buttons;
            prev_buttons = bits;
            if (!pad_sampled) {
                pad_sampled = 1;
                new_press = 0; /* Ignore held START when entering screen. */
            }
            if (new_press & PAD_START)
                break;
        }
        usleep(20000);
    }
    padPortClose(0, 0);
    padEnd();
    return 0;

failure:
    scr_printf("T FAIL rv=%d / last stage above\n", rv);
    scr_printf("PAD NOT READY: photo / reboot\n");
    for (;;)
        sleep(1);
    return 1;
}
