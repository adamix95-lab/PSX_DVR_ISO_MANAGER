#include "cover_runtime.h"
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
#define COVER_ART "mass0:/DESR_DVR/ART/"
#define SECTOR 2048

/* Bounded static buffers: never keep cover bitmaps or an entire ISO in RAM. */
static unsigned char sector_buf[SECTOR];
static char index_buf[DESR_COVER_INDEX_CAP];
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
static int get_id_from_iso(const char *filename,
                           char id[DESR_COVER_ID_CAP]) {
  char path[256], cnf[8192];
  uint32_t root_lba = 0, root_bytes = 0, cnf_lba = 0, cnf_bytes = 0;
  int fd, rc = 0, found = 0, i, sectors;
  if (desr_make_iso_path(filename, path, sizeof(path)) < 0) return -1;
  fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  if (fd < 0) return -1;
  id[0] = '\0';
  /* Descriptor sectors 16..31: support a boot record before the PVD. */
  for (i = 16; i <= 31; i++) {
    if (read_sector(fd, (uint32_t)i) < 0) { rc = -1; goto done; }
    if (memcmp(sector_buf + 1, "CD001", 5)) continue;
    if (sector_buf[0] == 255) break;
    if (sector_buf[0] == 1) {
      if (desr_iso_root(sector_buf, SECTOR, &root_lba, &root_bytes) < 0)
        { rc = 0; goto done; }
      found = 1;
      break;
    }
  }
  if (!found || root_bytes > 262144) goto done;
  sectors = (int)((root_bytes + SECTOR - 1) / SECTOR);
  for (i = 0; i < sectors; i++) {
    int r;
    if (read_sector(fd, root_lba + (uint32_t)i) < 0)
      { rc = -1; goto done; }
    r = desr_iso_system_record(sector_buf, SECTOR, &cnf_lba, &cnf_bytes);
    if (r < 0) goto done; /* Unsupported/corrupt ISO layout. */
    if (r == 1) {
      if (cnf_bytes > sizeof(cnf) || !cnf_bytes ||
          cnf_lba > INT_MAX / SECTOR) goto done;
      if (read_at(fd, cnf_lba * SECTOR, cnf, (int)cnf_bytes) < 0)
        { rc = -1; goto done; }
      rc = desr_cnf_game_id(cnf, cnf_bytes, id);
      goto done;
    }
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
  if (size < 0) size = read_index_file(COVER_BAK);
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

int desr_covers_refresh(char games[][DESR_FILENAME_CAP], int count,
                        desr_cover_entry covers[DESR_GAME_CAP],
                        int *identified) {
  int i, found = 0, ids = 0, size, n;
  char id[DESR_COVER_ID_CAP];
  if (!identified || count < 0 || count > DESR_GAME_CAP) return -1;
  *identified = 0;
  memset(pending, 0, sizeof(pending));
  memcpy(index_buf, DESR_COVER_INDEX_HEADER,
         sizeof(DESR_COVER_INDEX_HEADER) - 1);
  size = sizeof(DESR_COVER_INDEX_HEADER) - 1;
  for (i = 0; i < count; i++) {
    int rc = get_id_from_iso(games[i], id);
    if (rc < 0) return -1; /* Keep old index if DVR read failed. */
    if (!rc) continue;
    ids++;
    if (!cover_exists(id, "png") && !cover_exists(id, "jpg")) continue;
    memcpy(pending[i].game_id, id, DESR_COVER_ID_CAP);
    strcpy(pending[i].ext, cover_exists(id, "png") ? "png" : "jpg");
    pending[i].present = 1;
    n = desr_cover_format(index_buf + size,
                          sizeof(index_buf) - (size_t)size,
                          games[i], &pending[i]);
    if (n < 0) return -1;
    size += n;
    found++;
  }
  if (save_index(index_buf, size) < 0) return -1;
  memcpy(covers, pending, sizeof(pending));
  *identified = ids;
  return found;
}
