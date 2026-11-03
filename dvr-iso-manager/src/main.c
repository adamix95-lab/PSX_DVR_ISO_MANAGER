/* PSX DVR ISO Manager v1: shared UI, runtime-specific app storage.
 * HDD preview is strictly read-only while PP.DVRISO writes are unverified.
 */

#include "iso_utils.h"
#include "hdd_boot_config.h"
#include "cover_runtime.h"
#include "cover_preview.h"
#include "lan_art_sync.h"
#include "art_sync_runtime.h"
#include "art_sync.h"
#include <debug.h>
#include <elf-loader.h>
#include <errno.h>
#include <libcdvd.h>
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

/* All read paths come from one v1 layout header. HDD_PREVIEW never writes. */
#include "runtime_storage.h"
#define USB_TEST_CNF DESR_APP_TEST_CNF
#define USB_BOOT_CNF DESR_APP_BOOT_CNF
#define USB_BACKUP_CNF DESR_APP_BACKUP_CNF
#define USB_LAUNCHER DESR_APP_OSDMENU
#define USB_NEUTRINO DESR_APP_SYSTEM_NEUTRINO
#define USB_NEUTRINO_LANG DESR_APP_ENGLISH_NEUTRINO
#define USB_EECORE_LANG DESR_APP_EECORE_LANG
#define USB_LANG_CFG DESR_APP_LANG_CFG
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
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
IRX(ps2dev9);
IRX(ps2atad);
IRX(ps2hdd);
IRX(ps2fs);
/* These module argument vectors match the proven DESR-7000 DIAG G. */
static const char hdd_arguments[] = "-o\0" "4\0" "-n\0" "20";
static const char pfs_arguments[] = "-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char hdd_args_len_check[(sizeof(hdd_arguments) == 11) ? 1 : -1];
typedef char pfs_args_len_check[(sizeof(pfs_arguments) == 17) ? 1 : -1];
static int hdd_mount_owned;
#ifdef DESR_HDD_TEST
static int hdd_writable;
static int hdd_enable_writes(void);
#endif
#endif

static char games[DESR_GAME_CAP][DESR_FILENAME_CAP];
static int count;
static int selected;
static int force_english; /* 0 = SYSTEM, 1 = ENGLISH */
static desr_cover_entry covers[DESR_GAME_CAP];
#ifdef DESR_HDD_ART_PROBE
static desr_cover_entry probe_all_ids[DESR_GAME_CAP];
#endif
static char status[120] = "Starting...";
static int status_visible;
static int art_prompt; /* X downloads only on the scan confirmation screen. */
static int art_missing;
static int art_retry_cached; /* Recheck USB ART after cancelling a scan. */
static unsigned char padbuf[256] __attribute__((aligned(64)));
static unsigned prev_buttons;
static int dvr_probe_hdd = -999, dvr_probe_root = -999;
static int dvr_notice_game_start = -999;
#ifdef DESR_HDD_TEST
static int hdd_at_boot_system = -999, hdd_at_boot_osd = -999;
static int hdd_at_boot_root = -999;
#endif

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

#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
static int load_irx_args(const char *label, void *irx, unsigned size,
                         const char *args, unsigned arg_size) {
  int rv = -999, id;
  scr_printf("Loading %-18s ", label);
  id = SifExecModuleBuffer(irx, size, arg_size, (void *)args, &rv);
  if (id < 0 || rv < 0) {
    scr_printf("FAIL id=%d rv=%d\n", id, rv);
    return id < 0 ? id : rv;
  }
  scr_printf("OK id=%d rv=%d\n", id, rv);
  return 0;
}
#endif

static int init_drivers(void) {
  int r;
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
  u32 notice_arg = 0;
#endif
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
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
  /* Same APA/PFS driver order as DIAG G. No USB or dev9_ns modules. */
  LOAD(ps2dev9);
  LOAD(ps2atad);
  r = load_irx_args("ps2hdd", ps2hdd_irx, size_ps2hdd_irx,
                    hdd_arguments, sizeof(hdd_arguments));
  if (r < 0) return r;
  r = load_irx_args("ps2fs", ps2fs_irx, size_ps2fs_irx,
                    pfs_arguments, sizeof(pfs_arguments));
  if (r < 0) return r;
  r = fileXioInit();
  scr_printf("HDD fileXioInit=%d\n", r);
  if (r < 0) return r;
#else
  /* Preserve the tested USB v0.8 initialization order. */
  fileXioInit();
#endif
  LOAD(sio2man);
  LOAD(mcman);
  LOAD(mcserv);
  LOAD(freepad);
#if !defined(DESR_HDD_PREVIEW) && !defined(DESR_HDD_TEST)
  /* usbmass_bd requires BDM; FAT filesystem handles mass0: USB access. */
  LOAD(bdm);
  LOAD(bdmfs_fatfs);
  LOAD(usbd_mini);
  LOAD(usbmass_bd_mini);
  LOAD(dev9_ns);
#else
  /* DIAG G initializes CDVD and notice before DVR modules. */
  r = sceCdInit(SCECdINoD);
  scr_printf("HDD sceCdInit=%d\n", r);
  if (r <= 0) return -201;
  dvr_notice_game_start = sceCdNoticeGameStart(0, &notice_arg);
  scr_printf("HDD sceCdNoticeGameStart=%d\n", dvr_notice_game_start);
  if (dvr_notice_game_start < 0) return -202;
  usleep(500000);
#endif
  LOAD(dvrdrv);
  LOAD(dvrfile);
#undef LOAD
#if !defined(DESR_HDD_PREVIEW) && !defined(DESR_HDD_TEST)
  /* wLaunchELF R3Z calls this after its PSX DVR stack is loaded. */
  dvr_notice_game_start = sceCdNoticeGameStart(0, NULL);
  scr_printf("DVR game-start notice: %d\n", dvr_notice_game_start);
  /* Let USB/DVR devices register. */
  sleep(2);
#endif
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
  status_visible = 0;
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
    snprintf(status, sizeof(status), "Cannot open DVR ISO folder (%d)", fd);
    status_visible = 1;
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
    snprintf(status, sizeof(status), "DVR directory read error %d", n);
    status_visible = 1;
  } else {
    snprintf(status, sizeof(status), "Found %d ISO file(s)", count);
  }
  return n < 0 ? n : count;
}

/* MONO UI: 12 entries per page, with a PNG preview in the right panel.
 * ISO names are shortened only for display, never for launch paths.
 */
static void draw(void) {
  int i, first, last, title_len;

  /* scr_printf paints both foreground and background: highlight the
     selected row without resetting GS or the cover renderer. */
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xFFFFFF);
  scr_clear();

  scr_setXY(2, 1);
  scr_printf("PSX DVR ISO MANAGER");
  scr_setXY(69, 1);
  scr_printf("%s", DESR_APP_VARIANT);
  scr_setfontcolor(0x787878);
  scr_setXY(2, 3);
  scr_printf("--------------------------------------------------------------------------");
  scr_setfontcolor(0xFFFFFF);
  scr_setXY(2, 5);
  scr_printf("%d GAME%s", count, count == 1 ? "" : "S");
  if (count > DESR_PAGE_SIZE) {
    scr_setXY(24, 5);
    scr_printf("PAGE %02d/%02d", selected / DESR_PAGE_SIZE + 1,
               (count + DESR_PAGE_SIZE - 1) / DESR_PAGE_SIZE);
  }
  scr_setXY(43, 5);
  scr_printf("LANGUAGE: %s", force_english ? "ENGLISH" : "SYSTEM");

  if (count == 0) {
    scr_setXY(4, 10);
    scr_printf("No ISO games found.");
    scr_setXY(4, 11);
    scr_printf("Press TRIANGLE to rescan.");
  } else {
    first = (selected / DESR_PAGE_SIZE) * DESR_PAGE_SIZE;
    last = first + DESR_PAGE_SIZE;
    if (last > count) last = count;
    for (i = first; i < last; i++) {
      /* Drop .iso/.ISO extension in display only; not in launch path. */
      title_len = (int)strlen(games[i]) - 4; /* validated by scanner */
      if (title_len > 49) title_len = 49;
      /* 52 fixed columns: 3 for pointer/spacing + up to 49 for title.
         Even the longest ISO name remains LEFT of the cover at x=434.
         Gray is applied only to the selected line; nonselected rows are
         subtly dimmed to preserve the original MONO appearance. */
      scr_setbgcolor(i == selected ? 0x242424 : 0x000000);
      scr_setfontcolor(i == selected ? 0xFFFFFF : 0xC8C8C8);
      scr_setXY(4, 9 + i - first);
      if (i == selected)
        scr_printf(">  %-49.*s", title_len, games[i]);
      else
        scr_printf("   %.*s", title_len, games[i]);
      /* Only the selected row needs padding: reduces repeated tiny DMA
         transfers when navigating 12-game pages on the DESR. */
      scr_setbgcolor(0x000000);
      scr_setfontcolor(0xFFFFFF);
    }
  }

  /* PNG cover preview: compact color tiles drawn without reinitializing
     the video system. Missing or unsupported art keeps the text fallback. */
  if (count) {
    scr_setfontcolor(0xA8A8A8);
    scr_setXY(61, 6);
    scr_printf("GAME ID");
    scr_setfontcolor(0xFFFFFF);
    if (covers[selected].present) {
      scr_setXY(61, 7);
      scr_printf("%s", covers[selected].game_id);
      if (!desr_cover_preview_png(covers[selected].game_id,
                                  covers[selected].ext)) {
        scr_setXY(62, 11);
        scr_printf("ART FOUND");
        scr_setXY(62, 13);
        scr_printf(covers[selected].ext[0] == 'j' ? "JPG" : "BAD PNG");
      }
    } else {
      scr_setXY(62, 10);
      scr_printf("NO COVER");
    }
  }

  if (status_visible) {
#ifdef DESR_HDD_TEST
    /* End-of-batch error evidence survives console scrollback. */
    if (!strncmp(status,"ART ",4)) {
      char bad_id[ART_SYNC_ID_CAP];
      unsigned long received=0,expected=0;
      int reason=0;
      art_sync_last_bad_detail(bad_id,sizeof bad_id,&received,&expected,&reason);
      if (bad_id[0]) {
        scr_setfontcolor(0xE0C080);
        scr_setXY(2,21);
        scr_printf("BAD ID %.11s BODY%lu EXP%lu TYPE%d",
                   bad_id,received,expected,reason);
        scr_setfontcolor(0xFFFFFF);
      }
    }
#endif
    scr_setXY(2, 22);
    scr_printf("%.54s", status); /* keep status left of the cover */
  }
  /* Footer divider stops BEFORE the cover (x=434), which occupies
     y=64..191. Avoid overwriting bottom pixels of the PNG. */
  scr_setfontcolor(0x707070);
  scr_setXY(2, 23);
  scr_printf("-------------------------------------------------------");
  scr_setfontcolor(0xFFFFFF);
  scr_setXY(2, 24);
  scr_printf("UP/DOWN: GAME   LEFT/RIGHT: PAGE   X: LAUNCH");
  scr_setfontcolor(0xC8C8C8);
  scr_setXY(2, 25);
  scr_printf("SELECT: LANG   TRIANGLE: INDEX   SQUARE: TEST   START: EXIT");
  scr_setfontcolor(0xFFFFFF);
}

/* Separate confirmation after the completed manual TRIANGLE scan.
   Never draw the browser's X: LAUNCH footer underneath the question. */
static void draw_scan_art_question(int missing) {
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xFFFFFF);
  scr_clear();
  scr_setXY(2, 1);
  scr_printf("PSX DVR ISO MANAGER %s", DESR_APP_VARIANT);
  scr_setfontcolor(0x787878);
  scr_setXY(2, 3);
  scr_printf("-------------------------------------------------------");
  scr_setfontcolor(0xFFFFFF);
  scr_setXY(2, 6);
  scr_printf("GAME LIST / COVER INDEX: SCAN COMPLETE");
  scr_setXY(2, 10);
  scr_printf("GAMES: %d", count);
  scr_setXY(2, 12);
  scr_printf("MISSING COVERS: %d", missing);
  scr_setXY(2, 16);
  scr_printf("DOWNLOAD MISSING COVERS FROM LOCAL HTTP?");
  scr_setXY(2, 19);
  scr_printf("X: DOWNLOAD       O: SKIP / RETURN");
  scr_setfontcolor(0xA0A0A0);
  scr_setXY(2, 24);
#ifdef DESR_HDD_TEST
  scr_printf("Missing PNGs are saved to PP.DVRISO/ART.");
#else
  scr_printf("Only new PNG files are written to USB ART.");
#endif
  scr_setfontcolor(0xFFFFFF);
}

/* Native PS2SDK fileXio requires FIO_O_* flags, not POSIX O_* flags. */
static int check_usb_file(const char *path) {
  int fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  int closed;
  if (fd < 0) return fd;
  closed = fileXioClose(fd);
  return closed < 0 ? closed : 0;
}

static int usb_ready(void) {
  int rc;
  /* SYSTEM follows the standard Neutrino DVR boot path. */
  if (!force_english) return check_usb_file(USB_NEUTRINO);

  rc = check_usb_file(USB_NEUTRINO_LANG);
  if (rc < 0) return -101;
  rc = check_usb_file(USB_EECORE_LANG);
  if (rc < 0) return -102;
  rc = check_usb_file(USB_LANG_CFG);
  if (rc < 0) return -103;
  return 0;
}


#ifdef DESR_HDD_TEST
/* Save read-only PFS diagnostics before the network driver is loaded.
 * Error -19 is ENODEV: pfsX open failed due to inaccessible mount/unit.
 * Do not write or auto-remount when the mount has unexpectedly vanished.
 */
static int hdd_probe_dir(const char *path) {
  int fd=fileXioDopen(path);
  if (fd<0) return fd;
  return fileXioDclose(fd);
}
static void hdd_show_launch_diagnostic(const char *stage, int error_code) {
  struct padButtonStatus diagpad;
  int root_rc=hdd_probe_dir(DESR_APP_ROOT);
  int system_rc=check_usb_file(USB_NEUTRINO);
  int english_rc=check_usb_file(USB_NEUTRINO_LANG);
  int osd_rc=check_usb_file(USB_LAUNCHER);
  int apa_rc=hdd_probe_dir("hdd0:");
  int dvr_rc=hdd_probe_dir(DVR_FOLDER);
  int net_active=desr_lan_art_started();
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xFFFFFF);
  scr_clear();
  scr_printf("PSX DVR ISO MANAGER HDD / DIAG PFS\n");
  scr_printf("STAGE: %.38s  FILE ERROR=%d\n",stage,error_code);
  scr_printf("BOOT SNAPSHOT (before LAN):\n");
  scr_printf("  PFS1 ROOT %d; SYSTEM %d; OSD %d\n",
             hdd_at_boot_root,hdd_at_boot_system,hdd_at_boot_osd);
  scr_printf("CURRENT (after LAN or X):\n");
  scr_printf("  PFS1 ROOT %d; SYSTEM %d; ENGLISH %d\n",
             root_rc,system_rc,english_rc);
  scr_printf("  OSD BOOT.ELF %d; hdd0 %d; DVR ISO %d\n",
             osd_rc,apa_rc,dvr_rc);
  scr_printf("  NETMAN=%d; MOUNT_OWNED=%d; RW=%d\n",
             net_active,hdd_mount_owned,hdd_writable);
  scr_printf("  SYSTEM PATH: %s\n",USB_NEUTRINO);
  scr_printf("  PARTITION: %s\n",DESR_APP_PARTITION);
  if (error_code==-19)
    scr_printf("  -19 ENODEV: PFS MOUNT/UNIT UNAVAILABLE\n");
  scr_printf("PHOTO THIS SCREEN. CIRCLE: RETURN TO MENU\n");
  /* Use controller instead of timer: no automatic return to XMB. */
  for (;;) {
    if (padRead(0,0,&diagpad)>0 &&
        ((0xffff ^ diagpad.btns) & PAD_CIRCLE)) break;
    usleep(30000);
  }
  prev_buttons=0xffff;
}
#endif

/* Positive byte count, or a negative error. -ENOENT means absent file.
   A zero-length or oversized existing BOOT.CNF must not be overwritten. */
static int read_small_file(const char *path, char *out, int cap) {
  int fd, got, closed;
  if (!out || cap < 2) return -60;
  fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  if (fd < 0) return fd;
  got = fileXioRead(fd, out, cap);
  closed = fileXioClose(fd);
  if (got <= 0) return got < 0 ? got : -61;
  if (got >= cap) return -62;
  if (closed < 0) return -63;
  return got;
}

/* Checks exact length, bytes and absence of UTF-8 BOM after read-back.
   CAUTION: opening with TRUNC can empty path if later write fails. */
static int write_verified_file(const char *path, const char *src, int len) {
  char verify[640];
  int fd, sent, got, closed;
  if (!src || len <= 0 || len >= (int)sizeof(verify)) return -70;
  fd = fileXioOpen(path, FIO_O_WRONLY | FIO_O_CREAT | FIO_O_TRUNC, 0666);
  if (fd < 0) return fd;
  sent = fileXioWrite(fd, src, len);
  closed = fileXioClose(fd);
  if (sent != len) return -71;
  if (closed < 0) return -72;
  fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  if (fd < 0) return fd;
  got = fileXioRead(fd, verify, len + 1);
  closed = fileXioClose(fd);
  if (got != len || memcmp(verify, src, (size_t)len)) return -73;
  if (closed < 0) return -74;
  if (len >= 3 && (unsigned char)verify[0] == 0xEF &&
      (unsigned char)verify[1] == 0xBB &&
      (unsigned char)verify[2] == 0xBF) return -75;
  return 0;
}

/* SQUARE: writes only a separate test file. */
static int save_test_cnf(void) {
  char cnf[320];
  int len, rc;
  if (count <= 0) return -80;
#ifdef DESR_HDD_TEST
  len = desr_make_hdd_boot_cnf(games[selected], force_english, cnf, sizeof(cnf));
#else
  len = desr_make_boot_cnf_lang(games[selected], force_english, cnf, sizeof(cnf));
#endif
  if (len <= 0) return -80;
  rc = usb_ready();
  if (rc < 0) return rc;
#ifdef DESR_HDD_TEST
  rc = hdd_enable_writes();
  if (rc < 0) return rc;
#endif
  return write_verified_file(USB_TEST_CNF, cnf, len);
}

/* Create BOOT_RECOVERY.CNF once. A valid existing backup is never
   overwritten, even when games are switched repeatedly. */
static int ensure_original_backup(void) {
  char old_cnf[512], backup[512];
  int old_len, backup_len, rc;
  old_len = read_small_file(USB_BOOT_CNF, old_cnf, sizeof(old_cnf));
  if (old_len <= 0) {
    snprintf(status, sizeof(status), "BOOT.CNF missing/empty (%d). Restore it!", old_len);
    return -81;
  }

  backup_len = read_small_file(USB_BACKUP_CNF, backup, sizeof(backup));
  if (backup_len > 0) return 0;
  /* FatFS can report FR_NO_FILE as -4, rather than POSIX -ENOENT. */
  if (backup_len != -ENOENT && backup_len != -4) {
    snprintf(status, sizeof(status), "Existing USB backup invalid (%d)", backup_len);
    return -82;
  }
  rc = write_verified_file(USB_BACKUP_CNF, old_cnf, old_len);
  if (rc < 0) {
    snprintf(status, sizeof(status), "USB backup write/verify failed: %d", rc);
    return -83;
  }
  return 0;
}

#ifdef DESR_HDD_TEST
/* Keep the first BOOT.CNF unchanged as BOOT_RECOVERY.CNF.
 * Every selection is staged, read-back verified and published by rename.
 * Stale NEXT files are preserved, never silently overwritten. */
static int publish_hdd_cnf(const char *cnf, int len) {
  char old[512], backup[512];
  int old_len, bak_len, rc, fd, had_live = 0;
  old_len = read_small_file(USB_BOOT_CNF, old, sizeof old);
  if (old_len > 0) {
    had_live = 1;
    bak_len = read_small_file(USB_BACKUP_CNF, backup, sizeof backup);
    if (bak_len != -2 && bak_len != -4 && bak_len <= 0) return -301;
    if (bak_len <= 0) {
      rc = write_verified_file(USB_BACKUP_CNF, old, old_len);
      if (rc < 0) return -302;
    }
  } else if (old_len != -2 && old_len != -4) {
    return -303;
  }
  fd = fileXioOpen(DESR_APP_BOOT_NEXT, FIO_O_RDONLY, 0);
  if (fd >= 0) {
    fileXioClose(fd);
    return -304;
  }
  rc = write_verified_file(DESR_APP_BOOT_NEXT, cnf, len);
  if (rc < 0) return -305;
  if (had_live) {
    fd = fileXioOpen(DESR_APP_BOOT_PREV, FIO_O_RDONLY, 0);
    if (fd >= 0) {
      fileXioClose(fd);
      if (fileXioRemove(DESR_APP_BOOT_PREV) < 0) return -306;
    }
    if (fileXioRename(USB_BOOT_CNF, DESR_APP_BOOT_PREV) < 0)
      return -307;
  }
  rc = fileXioRename(DESR_APP_BOOT_NEXT, USB_BOOT_CNF);
  if (rc < 0) {
    if (had_live) fileXioRename(DESR_APP_BOOT_PREV, USB_BOOT_CNF);
    return -308;
  }
  /* Verify the published PFS file, not only the .NEXT staging file.
     If it differs, do not proceed into OSDMenu/Neutrino. */
  {
    char verify[320];
    int got = read_small_file(USB_BOOT_CNF, verify, sizeof verify);
    if (got != len || memcmp(verify, cnf, (size_t)len) != 0) return -309;
  }
  return 0;
}
#endif

/* X: save+verify BOOT.CNF, then chain to the OSDMenu Launcher with NO
   extra arguments. Its Quickboot uses argv[0] to find BOOT.CNF. */
static int launch_via_quickboot(void) {
  char cnf[320];
  int len, rc;
  if (count <= 0) return -90;
#ifdef DESR_HDD_TEST
  char *hdd_launcher_args[1] = {(char *)DESR_APP_HDD_CNF_ARG};
  len = desr_make_hdd_boot_cnf(games[selected], force_english, cnf, sizeof(cnf));
#else
  len = desr_make_boot_cnf_lang(games[selected], force_english, cnf, sizeof(cnf));
#endif
  if (len <= 0) return -90;
  rc = usb_ready();
  if (rc < 0) {
#ifdef DESR_HDD_TEST
    hdd_show_launch_diagnostic(force_english ? "ENGLISH RUNTIME" : "SYSTEM RUNTIME", rc);
    snprintf(status, sizeof(status), "HDD runtime open error %d (see diagnostics)", rc);
#else
    snprintf(status, sizeof(status), "Missing USB runtime (lang %s): %d",
             force_english ? "EN" : "SYSTEM", rc);
#endif
    return -91;
  }
  rc = check_usb_file(USB_LAUNCHER);
  if (rc < 0) {
#ifdef DESR_HDD_TEST
    hdd_show_launch_diagnostic("OSDMENU BOOT.ELF", rc);
#endif
    snprintf(status, sizeof(status), "Missing OSDMenu BOOT.ELF: %d", rc);
    return -92;
  }
#ifdef DESR_HDD_TEST
  rc = hdd_enable_writes();
  if (rc < 0) {
    snprintf(status, sizeof(status), "PP.DVRISO RW mount failed: %d", rc);
    return -93;
  }
  rc = publish_hdd_cnf(cnf, len);
  if (rc < 0) {
    snprintf(status, sizeof(status), "HDD BOOT.CNF transaction failed: %d", rc);
    return -93;
  }
#else
  rc = ensure_original_backup();
  if (rc < 0) return rc;
  rc = write_verified_file(USB_BOOT_CNF, cnf, len);
  if (rc < 0) {
    snprintf(status, sizeof(status), "BOOT.CNF write failed: %d. USB backup preserved!", rc);
    return -93;
  }
#endif

#ifdef DESR_HDD_TEST
  {
    char iso_path[256];
    int fd;
    if (desr_make_iso_path(games[selected],iso_path,sizeof iso_path)<0)
      return -96;
    fd=fileXioOpen(iso_path,FIO_O_RDONLY,0);
    if (fd<0) {
      snprintf(status,sizeof status,"HDD ISO OPEN FAILED (%d): %.62s",fd,games[selected]);
      return -97;
    }
    if (fileXioClose(fd)<0) return -98;
  }
#endif
  scr_clear();
  scr_printf("PSX DVR ISO MANAGER %s\n", DESR_APP_VARIANT);
  scr_printf("ISO: %s  Language: %s\n", games[selected],
             force_english ? "ENGLISH" : "SYSTEM");
#ifdef DESR_HDD_TEST
  scr_printf("HDD PP.DVRISO BOOT.CNF verified.\n");
  scr_printf("OSDMenu CNF: %s\n", DESR_APP_HDD_CNF_ARG);
#else
  scr_printf("USB BOOT.CNF verified. Recovery backup preserved.\n");
#endif
  scr_printf("Chainloading OSDMenu BOOT.ELF...\n");

  padPortClose(0, 0);
  padEnd();
  /* Keep the established OSDMenu/Neutrino chainload path unchanged. */
#ifdef DESR_HDD_TEST
  /* DIAG Z proven entry: explicit HDD CNF argument, PFS mount kept until
     LoadELFFromFile has read BOOT.ELF into EE RAM. */
  /* Give hardware testers a readable last screen before OSDMenu resets IOP,
     as the successful DIAG Z launcher did. */
  scr_printf("PP.DVRISO on PFS1; OSDMenu owns PFS0.\n");
  sleep(2);
  rc = LoadELFFromFile(USB_LAUNCHER, 1, hdd_launcher_args);
  scr_clear();
  scr_printf("HDD OSDMenu returned=%d. REBOOT; no IOP calls.\n", rc);
  for (;;) sleep(1);
#else
  rc = LoadELFFromFile(USB_LAUNCHER, 0, NULL);
  if (!padInit(0) || !padPortOpen(0, 0, padbuf)) return -94;
  prev_buttons = 0xffff;
  snprintf(status, sizeof(status), "BOOT.ELF chainload returned: %d. CNF saved.", rc);
  return -95;
#endif
}

#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
/* Operates only on the owner-created application partition. No disk writes. */
static void hdd_unmount_owned(void) {
  int rc;
  if (!hdd_mount_owned) return;
  rc = fileXioUmount("pfs1:");
  scr_printf("1.0 HDD: OWN PFS1 UMOUNT=%d\n", rc);
  if (rc >= 0) {
    hdd_mount_owned = 0;
#ifdef DESR_HDD_TEST
    hdd_writable = 0;
#endif
  }
}
static int hdd_mount_readonly(void) {
  int fd, rc;
  fd = fileXioDopen("hdd0:");
  scr_printf("1.0 HDD: APA ROOT=%d\n", fd);
  if (fd < 0) return fd;
  fileXioDclose(fd);

  fd = fileXioDopen("pfs1:/");
  if (fd >= 0) {
    fileXioDclose(fd);
    scr_printf("1.0 HDD: PFS1 ALREADY OCCUPIED; ABORT\n");
    return -210;
  }
  rc = fileXioMount("pfs1:", DESR_APP_PARTITION, FIO_MT_RDONLY);
  scr_printf("1.0 HDD: PP.DVRISO RO MOUNT=%d\n", rc);
  if (rc < 0) return rc;
  hdd_mount_owned = 1;
  fd = fileXioDopen(DESR_APP_ROOT);
  scr_printf("1.0 HDD: PFS ROOT=%d\n", fd);
  if (fd < 0) {
    hdd_unmount_owned();
    return fd;
  }
  fileXioDclose(fd);
  fd = fileXioDopen(DVR_FOLDER);
  scr_printf("1.0 HDD: DVR ISO=%d\n", fd);
  if (fd < 0) {
    hdd_unmount_owned();
    return fd;
  }
  fileXioDclose(fd);
  return 0;
}
#ifdef DESR_HDD_TEST
/* Explicit user action required for writes; app PP.DVRISO only. */
static int hdd_enable_writes(void) {
  int rc, fd;
  if (hdd_writable) return 0;
  if (!hdd_mount_owned) return -220;
  rc = fileXioUmount("pfs1:");
  if (rc < 0) return -221;
  hdd_mount_owned = 0;
  rc = fileXioMount("pfs1:", DESR_APP_PARTITION, FIO_MT_RDWR);
  if (rc < 0) {
    if (fileXioMount("pfs1:", DESR_APP_PARTITION, FIO_MT_RDONLY) >= 0)
      hdd_mount_owned = 1;
    return rc;
  }
  hdd_mount_owned = 1;
  fd = fileXioDopen(DESR_APP_ROOT);
  if (fd < 0) {
    hdd_unmount_owned();
    return fd;
  }
  fileXioDclose(fd);
  hdd_writable = 1;
  scr_printf("1.0 HDD: PP.DVRISO READ/WRITE OPT-IN OK\n");
  return 0;
}
#endif
#endif

int main(int argc, char *argv[]) {
  int rc, bits, new_press;
  struct padButtonStatus btn;
  (void)argc; (void)argv;
  init_scr();
  /* Disable the default PS2SDK block cursor. */
  scr_setCursor(0);
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xFFFFFF);
  scr_printf("PSX DVR ISO Manager - reading DVR games\n");
  rc = init_drivers();
  if (rc < 0) {
    scr_printf("\nDriver initialization error %d\n", rc);
    scr_printf("No HDD modifications were requested. Reboot console.\n");
    sleep(60);
    return 1;
  }
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
  /* Read-only preflight, before loading ISO list or artwork. */
  rc = hdd_mount_readonly();
  if (rc < 0) {
    scr_printf("1.0 HDD: READ-ONLY PREFLIGHT FAILED %d\n", rc);
    scr_printf("PP.DVRISO must exist; no auto-create or writes.\n");
    sleep(60);
    return 3;
  }
  scr_printf("1.0 HDD: BOTH FILESYSTEMS READABLE (RO)\n");
#endif
#ifdef DESR_HDD_TEST
  hdd_at_boot_root=hdd_probe_dir(DESR_APP_ROOT);
  hdd_at_boot_system=check_usb_file(USB_NEUTRINO);
  hdd_at_boot_osd=check_usb_file(USB_LAUNCHER);
  scr_printf("HDD EARLY: ROOT=%d SYS=%d OSD=%d\n",
             hdd_at_boot_root,hdd_at_boot_system,hdd_at_boot_osd);
#endif
  scan_dvr();
  desr_covers_load(games, count, covers);
  if (!padInit(0) || !padPortOpen(0, 0, padbuf)) {
    scr_printf("\nUnable to initialize gamepad\n");
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
    hdd_unmount_owned();
#endif
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

#ifdef DESR_HDD_PREVIEW
#ifdef DESR_HDD_ART_PROBE
      /* Dedicated RO LAN benchmark: TRIANGLE fetches every indexed Game ID
         into RAM and validates it. Never mount RW, scan/write INDEX or
         allow Quickboot/CNF writes. START cleanly unmounts our PFS1. */
      if (new_press & PAD_TRIANGLE) {
        int ok;
        scr_clear();
        scr_printf("PSX DVR HDD LAN RAM PROBE - READ ONLY\n");
        scr_printf("PP.DVRISO remains PFS1 READ-ONLY.\n");
        scr_printf("All indexed games are fetched even if ART exists.\n");
        {
          int ids=desr_covers_resolve_ids(games,count,probe_all_ids);
          scr_printf("PROBE: resolved %d/%d IDs from INDEX + SCAN.LOG\n",
                     ids,count);
          if (ids<0) ok=ids;
          else ok=desr_lan_art_probe(probe_all_ids,count);
        }
        {
          int done=0,total=0,bad=0,link=0,http=0,miss=0;
          int recovered=0,bad_attempts=0,first_ok=0;
          art_sync_probe_stats(&done,&total,&bad,&link,&http,&miss);
          art_sync_probe_retry_stats(&recovered,&bad_attempts,&first_ok);
          if (ok<0)
            snprintf(status,sizeof status,
                     "RAM PROBE ERROR %d; no PFS writes performed",ok);
          else
            snprintf(status,sizeof status,
                     "RAM %d/%d OK BAD%d REC%d LINK%d H%d MISS%d; RO",
                     done,total,bad,recovered,link,http,miss);
        }
        status_visible=1;
        draw();
        continue;
      }
#else
      /* Preview only: permit menu navigation and read-only rescans.
         DO NOT call write_verified_file(), desr_covers_refresh() or
         desr_lan_art_download() until dedicated HDD write/rename tests. */
      if (new_press & PAD_TRIANGLE) {
        rc = scan_dvr();
        if (rc >= 0) desr_covers_load(games, count, covers);
        snprintf(status, sizeof(status),
                 "HDD preview: read-only ISO rescan (%d)", rc);
        status_visible = 1;
        draw();
        continue;
      }
#endif /* not DESR_HDD_ART_PROBE */
      if (new_press & (PAD_CROSS | PAD_SQUARE)) {
        snprintf(status, sizeof(status),
#ifdef DESR_HDD_ART_PROBE
                 "HDD LAN PROBE READ ONLY: X/SQUARE launch & writes disabled"
#else
                 "HDD preview: CNF / ART writes and launch disabled"
#endif
                 );
        status_visible = 1;
        draw();
        continue;
      }
#endif

      /* X downloads only during the scan confirmation; otherwise Quickboot. */
      if (art_prompt) {
        if (new_press & PAD_CIRCLE) {
          art_prompt = 0;
          /* A cancelled scan can be retried using the USB report. */
          art_retry_cached = art_missing > 0;
          snprintf(status, sizeof(status),
                   "ART SKIPPED. TRIANGLE: RETRY LAST COVER SCAN");
          status_visible = 1;
          draw();
        } else if (new_press & PAD_CROSS) {
          int saved,identified=0,matched;
          art_prompt = 0;
          art_retry_cached = 0;
          scr_setbgcolor(0x000000);
          scr_setfontcolor(0xFFFFFF);
          scr_clear();
          scr_setXY(2, 1);
          scr_printf("LAN HTTP: downloading missing PNG covers...\n");
          scr_printf("DVR/ISO remain READ-ONLY.\n");
          saved = desr_lan_art_download();
          if (saved > 0) {
            /* Rebuild exactly the same validated index as TRIANGLE.
               If a DVR read fails, previous COVERS.IDX is preserved. */
            matched = desr_covers_refresh(games, count, covers, &identified);
            if (matched >= 0) {
#ifdef DESR_HDD_TEST
              int candidates=0,tried=0,not_found=0,http_failed=0,write_failed=0;
              art_sync_last_stats(&candidates,&tried,&not_found,&http_failed,&write_failed);
              int bad=0,link=0,http_status=0;
              art_sync_last_net_details(&bad,&link,&http_status);
              snprintf(status,sizeof(status),
                       "ART %d/%d IDX%d MISS%d NET%d BAD%d LINK%d H%d PFS%d",
                       saved,candidates,matched,not_found,http_failed,
                       bad,link,http_status,write_failed);
#else
              snprintf(status,sizeof(status),
                       "ART SYNC: %d saved. INDEX: %d/%d",
                       saved,matched,count);
#endif
            } else
              snprintf(status,sizeof(status),
                       "ART: %d saved; index failed. Press TRIANGLE.",
                       saved);
          } else {
#ifdef DESR_HDD_TEST
            {
              int candidates=0,tried=0,not_found=0,http_failed=0,write_failed=0;
              art_sync_last_stats(&candidates,&tried,&not_found,&http_failed,&write_failed);
              int bad=0,link=0,http_status=0;
              art_sync_last_net_details(&bad,&link,&http_status);
              snprintf(status,sizeof(status),
                       "ART 0/%d TRY%d MISS%d NET%d BAD%d LINK%d H%d PFS%d",
                       candidates,tried,not_found,http_failed,
                       bad,link,http_status,write_failed);
            }
#else
            snprintf(status,sizeof(status),
                     "ART SYNC: no new PNG. Check PC server / USB.");
#endif
          }
#ifdef DESR_HDD_TEST
          {
            int sys_after=check_usb_file(USB_NEUTRINO);
            if (sys_after<0) {
              snprintf(status,sizeof status,
                       "ART finished; HDD runtime error %d. Restart, test X first",
                       sys_after);
            }
          }
#endif
          status_visible = 1;
          draw();
        }
        /* Consume the whole key event even if another pad button is held. */
        continue;
      }

      if (new_press & PAD_TRIANGLE) {
        int identified = 0, matched;
        if (art_retry_cached) {
          int missing;
          art_retry_cached = 0;
          /* Refresh the missing-art count from USB without reading all ISOs. */
          missing = desr_lan_art_missing();
          if (missing > 0) {
            art_missing = missing;
            art_prompt = 1;
            draw_scan_art_question(art_missing);
            continue;
          }
          /* Rebuild the report when the cached one is no longer usable. */
        }
#ifdef DESR_HDD_TEST
        rc = hdd_enable_writes();
        if (rc < 0) {
          snprintf(status, sizeof(status), "PP.DVRISO RW mount failed: %d", rc);
          status_visible = 1;
          draw();
          continue;
        }
#endif
        rc = scan_dvr();
        if (rc >= 0) {
          desr_covers_load(games, count, covers);
          scr_clear();
          scr_setXY(2, 5);
          scr_printf("Reading ISO Game IDs; updating USB index...");
          matched = desr_covers_refresh(games, count, covers, &identified);
          art_prompt = 0;
          art_missing = 0;
          art_retry_cached = 0;
          if (matched >= 0) {
            art_missing = desr_lan_art_missing();
            if (art_missing > 0) {
              art_prompt = 1;
              draw_scan_art_question(art_missing);
              /* Keep X/O on the scan screen until one is selected. */
              continue;
            } else if (art_missing == 0) {
              snprintf(status,sizeof(status),
                       "INDEX: %d IDs / %d COVERS. MISSING: 0",
                       identified,matched);
            } else {
              snprintf(status,sizeof(status),
                       "INDEX OK (%d); cannot read COVER_SCAN.LOG.",
                       matched);
            }
          } else {
            snprintf(status,sizeof(status),
                     "Index update failed; previous USB index preserved.");
          }
          status_visible = 1;
        }
        draw();
      }
      if (new_press & PAD_SELECT) {
        force_english = !force_english;
        status_visible = 0;
        draw();
      }
      if (count && (new_press & PAD_UP)) {
        selected = (selected + count - 1) % count;
        status_visible = 0;
        draw();
      }
      if (count && (new_press & PAD_DOWN)) {
        selected = (selected + 1) % count;
        status_visible = 0;
        draw();
      }
      /* Page jump keeps the selected row whenever the destination has it.
         UP/DOWN remain single-game navigation; no wrap across pages here. */
      if (count && (new_press & PAD_LEFT) && !(new_press & PAD_RIGHT)) {
        int next = desr_page_move(selected, count, -1);
        if (next != selected) {
          selected = next;
          status_visible = 0;
          draw();
        }
      } else if (count && (new_press & PAD_RIGHT) && !(new_press & PAD_LEFT)) {
        int next = desr_page_move(selected, count, 1);
        if (next != selected) {
          selected = next;
          status_visible = 0;
          draw();
        }
      }
      if (count && (new_press & PAD_CROSS)) {
        rc = launch_via_quickboot();
        if (rc != -81 && rc != -82 && rc != -83 && rc != -91 &&
            rc != -92 && rc != -93 && rc != -95)
          snprintf(status, sizeof(status), "Quickboot setup failed: %d", rc);
        status_visible = 1;
        draw();
      }
      if (count && (new_press & PAD_SQUARE)) {
        rc = save_test_cnf();
        if (rc == 0)
          snprintf(status, sizeof(status), "Verified DVR_TEST.CNF: %.60s", games[selected]);
        else
          snprintf(status, sizeof(status), "DVR_TEST.CNF failed: %d", rc);
        status_visible = 1;
        draw();
      }
    }
    /* EE-side frame polling: POSIX delay (20 ms), not the IOP DelayThread API. */
    usleep(20000);
  }
  padPortClose(0, 0);
  padEnd();
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
  hdd_unmount_owned();
#endif
  return 0;
}
