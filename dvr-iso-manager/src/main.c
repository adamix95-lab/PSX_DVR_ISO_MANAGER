/* DESR DVR ISO Manager - stage 1. USB launcher + read-only DVR scanner.
   Uses binaries from the user's working Neutrino DVR PR #228 build.
   DOES NOT MODIFY DVR, APA, partitions, or game ISO files.
   Only explicit X confirmation writes mass0:/DESR_DVR/BOOT.CNF. */

#include "iso_utils.h"
#include <debug.h>
#include <libcdvd.h>
#include <fcntl.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define USB_CNF "mass0:/DESR_DVR/BOOT.CNF"
#define USB_NEUTRINO "mass0:/DESR_DVR/neutrino.elf"
#define DVR_FOLDER "dvr_pfs0:/ISO"

#define IRX(name) \
  extern unsigned char name##_irx[] __attribute__((aligned(16))); \
  extern uint32_t size_##name##_irx
IRX(iomanX);
IRX(fileXio);
IRX(sio2man);
IRX(mcman);
IRX(mcserv);
IRX(freepad);
IRX(bdm);
IRX(bdmfs_fatfs);
IRX(usbd_mini);
IRX(usbmass_bd_mini);
IRX(dev9_ns);
IRX(dvrdrv);
IRX(dvrfile);

static char games[DESR_GAME_CAP][DESR_FILENAME_CAP];
static int count;
static int selected;
static char status[120] = "Starting...";
static unsigned char padbuf[256] __attribute__((aligned(64)));
static unsigned prev_buttons;
static int dvr_probe_hdd = -999, dvr_probe_root = -999;
static int dvr_notice_game_start = -999;

static int load_irx(const char *label, void *irx, unsigned size) {
  int rv = 0, id;
  scr_printf("Loading %-18s ", label);
  id = SifExecModuleBuffer(irx, size, 0, NULL, &rv);
  if (id < 0 || rv < 0) {
    scr_printf("FAIL id=%d rv=%d\n", id, rv);
    return id < 0 ? id : rv;
  }
  scr_printf("OK\n");
  return 0;
}

static int init_drivers(void) {
  int r;
  scr_printf("Resetting IOP (no disk writes)...\n");
  while (!SifIopReset("", 0)) { }
  while (!SifIopSync()) { }
  sceSifInitRpc(0);
  sbv_patch_enable_lmb();
  sbv_patch_disable_prefix_check();
  sbv_patch_fileio();

#define LOAD(name) do { r = load_irx(#name, name##_irx, size_##name##_irx); if (r < 0) return r; } while (0)
  LOAD(iomanX);
  LOAD(fileXio);
  fileXioInit();
  LOAD(sio2man);
  LOAD(mcman);
  LOAD(mcserv);
  LOAD(freepad);
  /* usbmass_bd requires BDM; FAT filesystem handles mass0: USB access. */
  LOAD(bdm);
  LOAD(bdmfs_fatfs);
  LOAD(usbd_mini);
  LOAD(usbmass_bd_mini);
  LOAD(dev9_ns);
  LOAD(dvrdrv);
  LOAD(dvrfile);
#undef LOAD
  /* wLaunchELF R3Z calls this after its PSX DVR stack is loaded. */
  dvr_notice_game_start = sceCdNoticeGameStart(0, NULL);
  scr_printf("DVR game-start notice: %d\n", dvr_notice_game_start);
  /* Let USB/DVR devices register. */
  sleep(2);
  return 0;
}

static int name_compare(const void *a, const void *b) {
  const char *x = (const char *)a, *y = (const char *)b;
  while (*x && *y) {
    unsigned char a0 = (unsigned char)*x++, b0 = (unsigned char)*y++;
    if (a0 >= 'a' && a0 <= 'z') a0 -= 32;
    if (b0 >= 'a' && b0 <= 'z') b0 -= 32;
    if (a0 != b0) return (int)a0 - (int)b0;
  }
  return (unsigned char)*x - (unsigned char)*y;
}

static int scan_dvr(void) {
  int fd, n;
  iox_dirent_t ent;
  count = 0;
  selected = 0;
  /* Diagnostics distinguish driver/device failure from an ISO folder error. */
  dvr_probe_hdd = fileXioDopen("dvr_hdd0:");
  if (dvr_probe_hdd >= 0) {
    fileXioDclose(dvr_probe_hdd);
    dvr_probe_hdd = 0;
  }
  dvr_probe_root = fileXioDopen("dvr_pfs0:/");
  if (dvr_probe_root >= 0) {
    fileXioDclose(dvr_probe_root);
    dvr_probe_root = 0;
  }
  fd = fileXioDopen(DVR_FOLDER);
  if (fd < 0) {
    snprintf(status, sizeof(status), "Can't open %s (%d)", DVR_FOLDER, fd);
    return fd;
  }
  while ((n = fileXioDread(fd, &ent)) > 0) {
    size_t len;
    ent.name[sizeof(ent.name) - 1] = '\0';
    if (!desr_filename_is_safe(ent.name)) continue;
    if (count >= DESR_GAME_CAP) break;
    len = strlen(ent.name);
    if (len >= DESR_FILENAME_CAP) continue;
    memcpy(games[count], ent.name, len + 1);
    ++count;
  }
  fileXioDclose(fd);
  qsort(games, (size_t)count, sizeof(games[0]), name_compare);
  if (n < 0) {
    snprintf(status, sizeof(status), "Directory read error %d", n);
  } else {
    snprintf(status, sizeof(status), "Found %d ISO file(s), DVR read-only", count);
  }
  return n < 0 ? n : count;
}

static void draw(void) {
  int i, first;
  scr_clear();
  scr_printf("DESR7000 DVR ISO MANAGER   v0.1 TEST\n");
  scr_printf("------------------------------------------------\n");
  scr_printf("DVR: %s\n", DVR_FOLDER);
  scr_printf("DVRP notice=%d  hdd0=%d  pfs0 root=%d\n",
             dvr_notice_game_start, dvr_probe_hdd, dvr_probe_root);
  scr_printf("%s\n\n", status);
  if (!count) {
    scr_printf("No ISO files detected.\n");
    scr_printf("Make sure your DVR contains __xcontents/ISO/*.ISO\n");
  } else {
    first = (selected / 12) * 12;
    for (i = first; i < count && i < first + 12; i++) {
      scr_printf("%c %3d  %.55s\n", i == selected ? '>' : ' ', i + 1, games[i]);
    }
    scr_printf("\nSelected %d / %d\n", selected + 1, count);
  }
  scr_printf("\nUP/DOWN: select   TRIANGLE: rescan\n");
  scr_printf("X: write BOOT.CNF on USB   START: exit\n");
  scr_printf("No DVR writes. X overwrites USB BOOT.CNF!\n");
  scr_printf("After saving, launch BOOT.ELF via wLE.\n");
}

static int usb_ready(void) {
  int fd = fileXioOpen(USB_NEUTRINO, O_RDONLY, 0);
  if (fd < 0) return fd;
  fileXioClose(fd);
  return 0;
}

static int save_cnf(void) {
  char cnf[320], verify[320];
  int len, fd, got, sent, ready;
  if (!count) return -1;
  len = desr_make_boot_cnf(games[selected], cnf, sizeof(cnf));
  if (len < 0) return -2;
  ready = usb_ready();
  if (ready < 0) return ready;
  fd = fileXioOpen(USB_CNF, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) return fd;
  sent = fileXioWrite(fd, cnf, len);
  fileXioClose(fd);
  if (sent != len) return -3;

  /* Read-back: assures successful file write and absence of BOM. */
  fd = fileXioOpen(USB_CNF, O_RDONLY, 0);
  if (fd < 0) return fd;
  got = fileXioRead(fd, verify, sizeof(verify));
  fileXioClose(fd);
  if (got != len || memcmp(verify, cnf, (size_t)len)) return -4;
  if (got >= 3 && (unsigned char)verify[0] == 0xEF &&
      (unsigned char)verify[1] == 0xBB && (unsigned char)verify[2] == 0xBF)
    return -5;
  return 0;
}

int main(int argc, char *argv[]) {
  int rc, bits, new_press;
  struct padButtonStatus btn;
  (void)argc; (void)argv;
  init_scr();
  scr_printf("DESR DVR ISO Manager - read-only scanner\n");
  rc = init_drivers();
  if (rc < 0) {
    scr_printf("\nDriver initialization error %d\n", rc);
    scr_printf("No HDD modifications were requested. Reboot console.\n");
    sleep(60);
    return 1;
  }
  scan_dvr();
  if (!padInit(0) || !padPortOpen(0, 0, padbuf)) {
    scr_printf("\nUnable to initialize gamepad\n");
    sleep(60);
    return 2;
  }
  prev_buttons = 0;
  draw();
  while (1) {
    if (padRead(0, 0, &btn) > 0) {
      bits = 0xffff ^ btn.btns;
      new_press = bits & ~prev_buttons;
      prev_buttons = bits;
      if (new_press & PAD_START) break;
      if (new_press & PAD_TRIANGLE) { scan_dvr(); draw(); }
      if (count && (new_press & PAD_UP)) {
        selected = (selected + count - 1) % count; draw();
      }
      if (count && (new_press & PAD_DOWN)) {
        selected = (selected + 1) % count; draw();
      }
      if (count && (new_press & PAD_CROSS)) {
        rc = save_cnf();
        if (!rc) {
          snprintf(status, sizeof(status), "Saved BOOT.CNF (no BOM): %.60s", games[selected]);
        } else {
          snprintf(status, sizeof(status), "USB CNF write/verify failed: %d", rc);
        }
        draw();
      }
    }
    /* EE-side frame polling: POSIX delay (20 ms), not the IOP DelayThread API. */
    usleep(20000);
  }
  padPortClose(0, 0);
  padEnd();
  return 0;
}
