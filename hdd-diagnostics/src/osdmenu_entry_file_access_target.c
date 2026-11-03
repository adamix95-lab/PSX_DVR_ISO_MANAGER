/*
 * DIAG T target: executed by real OSDMenu from PP.TESTDVR through DIAG_T.CNF.
 * First probe cwd/config/modules BEFORE any IOP reset, before loading IOP
 * modules or starting a game. Read-only. Then reset IOP and load only
 * known R target pad modules so results remain on screen until START.
 * A failed file-open does not prove the file is absent; PFS may be unmounted.
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
#define EXPECTED_DVD "-dvd=dvr_pfs0:/ISO/DIAG_T_NO_GAME.iso"
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
    scr_printf("PSX DVR DIAG T / ENTRY TEST\n");
    scr_printf("ENTRY: ARGC=%d ARGS_OK=%d\n", argc, arguments_valid);
    scr_printf("READONLY OPEN TEST. NO GAME START\n");

    inspect_at_elf_entry();
    before("T13 IOP RESET");
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
    scr_printf("PSX DVR HDD DIAG T / TARGET ENTRY\n");
    scr_printf("ARGC=%d ARGS_OK=%d\n", argc, arguments_valid);
    scr_printf("ARGV0: %.55s\n", saved_argv0);
    scr_printf("CWD: %.59s\n", cwd_text);
    scr_printf("fileXioInit=%d PFS0=%d HDD0=%d\n",
               t_filexio_init, t_pfs0_dir, t_hdd0_dir);
    scr_printf("RPC PFS CFG=%d IRX=%d\n", t_pfs_cfg, t_pfs_irx);
    scr_printf("RPC REL CFG=%d IRX=%d\n", t_rel_cfg, t_rel_irx);
    scr_printf("POSIX CFG=%d ER=%d\n", t_posix_cfg, t_errno_cfg);
    scr_printf("POSIX IRX=%d ER=%d\n", t_posix_irx, t_errno_irx);
    scr_printf("DUMMY ISO NOT OPENED. NO GAME.\n");
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
