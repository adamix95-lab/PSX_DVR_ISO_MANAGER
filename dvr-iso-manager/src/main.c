/* PSX DVR ISO Manager v0.8 — USB launcher, DVR ISO browser and cover sync.
 * DVR/ISO data is read-only. Launch configuration and cover cache use USB.
 */

#include "iso_utils.h"
#include "cover_runtime.h"
#include "cover_preview.h"
#include "lan_art_sync.h"
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

#define USB_TEST_CNF "mass0:/DESR_DVR/DVR_TEST.CNF"
#define USB_BOOT_CNF "mass0:/DESR_DVR/BOOT.CNF"
#define USB_BACKUP_CNF "mass0:/DESR_DVR/BOOT_RECOVERY.CNF"
#define USB_LAUNCHER "mass0:/DESR_DVR/BOOT.ELF"
#define USB_NEUTRINO "mass0:/DESR_DVR/neutrino.elf"
#define USB_NEUTRINO_LANG "mass0:/DESR_DVR/neutrino_lang.elf"
#define USB_EECORE_LANG "mass0:/DESR_DVR/modules/ee_core_lang.elf"
#define USB_LANG_CFG "mass0:/DESR_DVR/config/osdlang.toml"
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
static int force_english; /* 0 = SYSTEM, 1 = ENGLISH */
static desr_cover_entry covers[DESR_GAME_CAP];
static char status[120] = "Starting...";
static int status_visible;
static int art_prompt; /* X downloads only on the scan confirmation screen. */
static int art_missing;
static int art_retry_cached; /* Recheck USB ART after cancelling a scan. */
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
  scr_printf("v0.8");
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
  scr_printf("PSX DVR ISO MANAGER v0.8");
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
  scr_printf("Only new PNG files are written to USB ART.");
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
  len = desr_make_boot_cnf_lang(games[selected], force_english, cnf, sizeof(cnf));
  if (len <= 0) return -80;
  rc = usb_ready();
  if (rc < 0) return rc;
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

/* X: save+verify BOOT.CNF, then chain to the OSDMenu Launcher with NO
   extra arguments. Its Quickboot uses argv[0] to find BOOT.CNF. */
static int launch_via_quickboot(void) {
  char cnf[320];
  int len, rc;
  if (count <= 0) return -90;
  len = desr_make_boot_cnf_lang(games[selected], force_english, cnf, sizeof(cnf));
  if (len <= 0) return -90;
  rc = usb_ready();
  if (rc < 0) {
    snprintf(status, sizeof(status), "Missing USB runtime (lang %s): %d",
             force_english ? "EN" : "SYSTEM", rc);
    return -91;
  }
  rc = check_usb_file(USB_LAUNCHER);
  if (rc < 0) {
    snprintf(status, sizeof(status), "Missing OSDMenu BOOT.ELF: %d", rc);
    return -92;
  }
  rc = ensure_original_backup();
  if (rc < 0) return rc;
  rc = write_verified_file(USB_BOOT_CNF, cnf, len);
  if (rc < 0) {
    snprintf(status, sizeof(status), "BOOT.CNF write failed: %d. USB backup preserved!", rc);
    return -93;
  }

  scr_clear();
  scr_printf("PSX DVR ISO MANAGER v0.8\n");
  scr_printf("ISO: %s  Language: %s\n", games[selected],
             force_english ? "ENGLISH" : "SYSTEM");
  scr_printf("USB BOOT.CNF verified. Recovery backup preserved.\n");
  scr_printf("Chainloading OSDMenu BOOT.ELF...\n");

  padPortClose(0, 0);
  padEnd();
  /* Keep the established OSDMenu/Neutrino chainload path unchanged. */
  rc = LoadELFFromFile(USB_LAUNCHER, 0, NULL);
  if (!padInit(0) || !padPortOpen(0, 0, padbuf)) return -94;
  prev_buttons = 0xffff;
  snprintf(status, sizeof(status), "BOOT.ELF chainload returned: %d. CNF saved.", rc);
  return -95;
}

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
  scan_dvr();
  desr_covers_load(games, count, covers);
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
            /* Rebuild exactly the same validated USB index as TRIANGLE.
               If a DVR read fails, previous COVERS.IDX is preserved. */
            matched = desr_covers_refresh(games, count, covers, &identified);
            if (matched >= 0)
              snprintf(status,sizeof(status),
                       "ART SYNC: %d saved. INDEX: %d/%d",
                       saved,matched,count);
            else
              snprintf(status,sizeof(status),
                       "ART: %d saved; index failed. Press TRIANGLE.",
                       saved);
          } else {
            snprintf(status,sizeof(status),
                     "ART SYNC: no new PNG. Check PC server / USB.");
          }
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
  return 0;
}
