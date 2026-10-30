#include "cover_runtime.h"
#include "cover_report.h"
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define COVER_IDX "mass0:/DESR_DVR/COVERS.IDX"
#define COVER_NEW "mass0:/DESR_DVR/COVERS.NEW"
#define COVER_BAK "mass0:/DESR_DVR/COVERS.BAK"
#define COVER_SCAN_LOG "mass0:/DESR_DVR/COVER_SCAN.LOG"
#define COVER_ART "mass0:/DESR_DVR/ART/"
#define SECTOR 2048

/* Bounded static buffers: never keep cover bitmaps or an entire ISO in RAM. */
static unsigned char sector_buf[SECTOR];
static char index_buf[DESR_COVER_INDEX_CAP];
/* Debug report is only written after manual TRIANGLE scan, never at startup. */
static char scan_report[DESR_COVER_INDEX_CAP];
static desr_cover_entry pending[DESR_GAME_CAP];

void desr_covers_clear(desr_cover_entry covers[DESR_GAME_CAP]) {
  memset(covers, 0, sizeof(desr_cover_entry) * DESR_GAME_CAP);
}

static int read_at(int fd, uint32_t at, void *buf, int bytes) {
  int pos, done = 0, r;
  if (at > INT_MAX || bytes <= 0 || bytes > INT_MAX - (int)at) return -1;
  pos = fileXioLseek(fd, (int)at, SEEK_SET);
  if (pos != (int)at) return -1;
  while (done < bytes) {
    r = fileXioRead(fd, (char *)buf + done, bytes - done);
    if (r <= 0) return -1;
    done += r;
  }
  return 0;
}
static int read_sector(int fd, uint32_t sector) {
  if (sector > INT_MAX / SECTOR) return -1;
  return read_at(fd, sector * SECTOR, sector_buf, SECTOR);
}
/* Extract the boot ID from SYSTEM.CNF first. If the CNF cannot be
   recognized, use a single unambiguous root executable ID as fallback.
   We never infer Game IDs from human-readable ISO filenames. */
static int get_id_from_iso(const char *filename,
                           char id[DESR_COVER_ID_CAP],
                           enum desr_cover_scan_status *reason,
                           int *from_exe) {
  char path[256], cnf[8192], candidate[DESR_COVER_ID_CAP] = {0};
  uint32_t root_lba = 0, root_bytes = 0, cnf_lba = 0, cnf_bytes = 0;
  int fd, rc = 0, found_pvd = 0, found_cnf = 0;
  int candidate_found = 0, ambiguous = 0, i, sectors;
  if (!id || !reason || !from_exe) return -1;
  id[0] = '\0';
  *from_exe = 0;
  *reason = DESR_SCAN_NO_PVD;
  if (desr_make_iso_path(filename, path, sizeof(path)) < 0) return -1;
  fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  if (fd < 0) { *reason = DESR_SCAN_READ_ERROR; return -1; }

  /* Conventional ISO9660 2048-byte user-data sectors. */
  for (i = 16; i <= 31; i++) {
    if (read_sector(fd, (uint32_t)i) < 0) {
      rc = -1; *reason = DESR_SCAN_READ_ERROR; goto done;
    }
    if (memcmp(sector_buf + 1, "CD001", 5)) continue;
    if (sector_buf[0] == 255) break;
    if (sector_buf[0] == 1) {
      if (desr_iso_root(sector_buf, SECTOR, &root_lba, &root_bytes) < 0) {
        *reason = DESR_SCAN_BAD_DIRECTORY;
        goto done;
      }
      found_pvd = 1;
      break;
    }
  }
  if (!found_pvd) goto done; /* NO_PVD: possibly non-2048 image. */
  if (root_bytes > 262144U) {
    *reason = DESR_SCAN_BAD_DIRECTORY;
    goto done;
  }

  sectors = (int)((root_bytes + SECTOR - 1) / SECTOR);
  for (i = 0; i < sectors; i++) {
    int r, boot_r;
    char local_id[DESR_COVER_ID_CAP];
    if (root_lba > (uint32_t)(INT_MAX / SECTOR - i)) {
      *reason = DESR_SCAN_BAD_DIRECTORY;
      goto done;
    }
    if (read_sector(fd, root_lba + (uint32_t)i) < 0) {
      rc = -1; *reason = DESR_SCAN_READ_ERROR; goto done;
    }

    r = desr_iso_system_record(sector_buf, SECTOR, &cnf_lba, &cnf_bytes);
    boot_r = desr_iso_root_boot_candidate(sector_buf, SECTOR, local_id);
    if (r < 0 || boot_r < 0) {
      *reason = DESR_SCAN_BAD_DIRECTORY;
      goto done;
    }
    if (r == 1) found_cnf = 1;
    if (boot_r == 2) ambiguous = 1;
    if (boot_r == 1) {
      if (candidate_found && strcmp(candidate, local_id)) ambiguous = 1;
      else {
        memcpy(candidate, local_id, sizeof(candidate));
        candidate_found = 1;
      }
    }
  }

  /* Prefer the launch configuration whenever it gives a valid ID.
     Even if an ISO has multiple executables, valid BOOT2 takes priority. */
  if (found_cnf && cnf_bytes && cnf_bytes <= sizeof(cnf) &&
      cnf_lba <= (uint32_t)(INT_MAX / SECTOR) &&
      read_at(fd, cnf_lba * SECTOR, cnf, (int)cnf_bytes) == 0) {
    if (desr_cnf_game_id(cnf, cnf_bytes, id)) {
      rc = 1;
      goto done;
    }
  }

  if (ambiguous) {
    *reason = DESR_SCAN_AMBIGUOUS_EXE;
  } else if (candidate_found) {
    memcpy(id, candidate, sizeof(candidate));
    *from_exe = 1;
    rc = 1;
  } else {
    *reason = found_cnf ? DESR_SCAN_BAD_BOOT2 : DESR_SCAN_NO_SYSTEM;
  }
done:
  fileXioClose(fd);
  return rc;
}

static int cover_exists(const char *id, const char *ext) {
  char path[DESR_COVER_ID_CAP + 55];
  int fd, n;
  if (!desr_valid_game_id(id)) return 0;
  n = snprintf(path, sizeof(path), COVER_ART "%s_COV.%s", id, ext);
  if (n <= 0 || (size_t)n >= sizeof(path)) return 0;
  fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  if (fd < 0) return 0;
  fileXioClose(fd);
  return 1;
}

static int read_index_file(const char *path) {
  int fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  int size = 0, got, closed;
  if (fd < 0) return -1;
  while (size < DESR_COVER_INDEX_CAP - 1) {
    got = fileXioRead(fd, index_buf + size,
                      DESR_COVER_INDEX_CAP - 1 - size);
    if (got < 0) { fileXioClose(fd); return -1; }
    if (!got) break;
    size += got;
  }
  if (size >= DESR_COVER_INDEX_CAP - 1) {
    fileXioClose(fd);
    return -1;
  }
  closed = fileXioClose(fd);
  if (closed < 0) return -1;
  index_buf[size] = '\0';
  return size;
}

int desr_covers_load(char games[][DESR_FILENAME_CAP], int count,
                     desr_cover_entry covers[DESR_GAME_CAP]) {
  int size, parsed = 0, i;
  char *p, *end;
  desr_covers_clear(covers);
  size = read_index_file(COVER_IDX);
  /* Recover an interrupted/invalid index from the retained USB backup. */
  if (size < (int)strlen(DESR_COVER_INDEX_HEADER) ||
      memcmp(index_buf, DESR_COVER_INDEX_HEADER,
             strlen(DESR_COVER_INDEX_HEADER))) {
    size = read_index_file(COVER_BAK);
  }
  if (size < (int)strlen(DESR_COVER_INDEX_HEADER) ||
      memcmp(index_buf, DESR_COVER_INDEX_HEADER,
             strlen(DESR_COVER_INDEX_HEADER))) return 0;
  p = index_buf + strlen(DESR_COVER_INDEX_HEADER);
  end = index_buf + size;
  while (p < end) {
    char *nl = memchr(p, '\n', (size_t)(end - p));
    char filename[DESR_FILENAME_CAP];
    desr_cover_entry entry;
    if (!nl) break;
    *nl = '\0';
    if (desr_cover_parse(p, filename, &entry)) {
      for (i = 0; i < count && i < DESR_GAME_CAP; i++) {
        if (!strcmp(filename, games[i]) &&
            cover_exists(entry.game_id, entry.ext)) {
          covers[i] = entry;
          parsed++;
          break;
        }
      }
    }
    p = nl + 1;
  }
  return parsed;
}

static int verify_new_file(const char *data, int size) {
  int fd = fileXioOpen(COVER_NEW, FIO_O_RDONLY, 0), pos = 0, n;
  char chunk[512];
  if (fd < 0) return -1;
  while (pos < size) {
    int want = size - pos;
    if (want > (int)sizeof(chunk)) want = sizeof(chunk);
    n = fileXioRead(fd, chunk, want);
    if (n <= 0 || memcmp(data + pos, chunk, n)) {
      fileXioClose(fd); return -1;
    }
    pos += n;
  }
  n = fileXioRead(fd, chunk, 1);
  if (fileXioClose(fd) < 0 || n != 0) return -1;
  return 0;
}

static int save_index(const char *data, int size) {
  int fd, done = 0, n, closed, existed;
  fd = fileXioOpen(COVER_NEW, FIO_O_WRONLY | FIO_O_CREAT | FIO_O_TRUNC, 0666);
  if (fd < 0) return -1;
  while (done < size) {
    int chunk = size - done;
    if (chunk > 1024) chunk = 1024;
    n = fileXioWrite(fd, data + done, chunk);
    if (n <= 0) { fileXioClose(fd); return -1; }
    done += n;
  }
  closed = fileXioClose(fd);
  if (closed < 0 || verify_new_file(data, size) < 0) return -1;

  /* Never truncate the live index. Rotate the verified previous copy to
     .BAK, then publish .NEW; on failed promotion restore the backup. */
  fd = fileXioOpen(COVER_IDX, FIO_O_RDONLY, 0);
  existed = fd >= 0;
  if (existed) {
    fileXioClose(fd);
    fileXioRemove(COVER_BAK);
    if (fileXioRename(COVER_IDX, COVER_BAK) < 0) return -1;
  }
  if (fileXioRename(COVER_NEW, COVER_IDX) < 0) {
    if (existed) fileXioRename(COVER_BAK, COVER_IDX);
    return -1;
  }
  return 0;
}

/* Diagnostic is optional: write failures never affect the existing cache,
   the game list, or Quickboot. Only the USB report path is writable. */
static void write_scan_report(size_t size) {
  int fd, done = 0, n;
  if (!size || size >= sizeof(scan_report)) return;
  fd = fileXioOpen(COVER_SCAN_LOG,
                   FIO_O_WRONLY | FIO_O_CREAT | FIO_O_TRUNC, 0666);
  if (fd < 0) return;
  while (done < (int)size) {
    int want = (int)size - done;
    if (want > 1024) want = 1024;
    n = fileXioWrite(fd, scan_report + done, want);
    if (n <= 0) break;
    done += n;
  }
  fileXioClose(fd);
}

int desr_covers_refresh(char games[][DESR_FILENAME_CAP], int count,
                        desr_cover_entry covers[DESR_GAME_CAP],
                        int *identified) {
  int i, found = 0, ids = 0, size, n, report_ok = 1;
  size_t report_size = 0;
  char id[DESR_COVER_ID_CAP];
  if (!identified || count < 0 || count > DESR_GAME_CAP) return -1;
  *identified = 0;
  memset(pending, 0, sizeof(pending));
  memcpy(index_buf, DESR_COVER_INDEX_HEADER,
         sizeof(DESR_COVER_INDEX_HEADER) - 1);
  size = sizeof(DESR_COVER_INDEX_HEADER) - 1;
  if (desr_cover_report_init(scan_report, sizeof(scan_report),
                             &report_size) < 0) report_ok = 0;
  for (i = 0; i < count; i++) {
    enum desr_cover_scan_status reason = DESR_SCAN_NO_ID;
    int from_exe = 0;
    int rc = get_id_from_iso(games[i], id, &reason, &from_exe);
    int png_found, jpg_found;
    if (rc < 0) {
      if (report_ok && desr_cover_report_add(
             scan_report, sizeof(scan_report), &report_size,
             games[i], DESR_SCAN_READ_ERROR, NULL) < 0) report_ok = 0;
      if (report_ok) write_scan_report(report_size);
      return -1; /* Old index survives failed DVR read, as in v0.5. */
    }
    if (!rc) {
      if (report_ok && desr_cover_report_add(
             scan_report, sizeof(scan_report), &report_size,
             games[i], reason, NULL) < 0) report_ok = 0;
      continue;
    }
    ids++;
    png_found = cover_exists(id, "png");
    jpg_found = png_found ? 0 : cover_exists(id, "jpg");
    if (!png_found && !jpg_found) {
      if (report_ok && desr_cover_report_add(
             scan_report, sizeof(scan_report), &report_size,
             games[i], from_exe ? DESR_SCAN_NO_ART_EXE : DESR_SCAN_NO_ART,
             id) < 0) report_ok = 0;
      continue;
    }
    memcpy(pending[i].game_id, id, DESR_COVER_ID_CAP);
    strcpy(pending[i].ext, png_found ? "png" : "jpg");
    pending[i].present = 1;
    n = desr_cover_format(index_buf + size,
                          sizeof(index_buf) - (size_t)size,
                          games[i], &pending[i]);
    if (n < 0) return -1;
    size += n;
    found++;
    if (report_ok && desr_cover_report_add(
           scan_report, sizeof(scan_report), &report_size,
           games[i],
           from_exe
             ? (png_found ? DESR_SCAN_FOUND_PNG_EXE : DESR_SCAN_FOUND_JPG_EXE)
             : (png_found ? DESR_SCAN_FOUND_PNG : DESR_SCAN_FOUND_JPG),
           id) < 0) report_ok = 0;
  }
  if (save_index(index_buf, size) < 0) return -1;
  memcpy(covers, pending, sizeof(pending));
  *identified = ids;
  if (report_ok) write_scan_report(report_size);
  return found;
}
