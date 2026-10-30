#include "cover_index.h"
#include <stdio.h>
#include <string.h>

static uint32_t le32(const unsigned char *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int up(int c) {
  return c >= 'a' && c <= 'z' ? c - 32 : c;
}
static int match(const unsigned char *s, size_t n, const char *wanted) {
  size_t i, len = strlen(wanted);
  if (n != len) return 0;
  for (i = 0; i < n; i++)
    if (up(s[i]) != (unsigned char)wanted[i]) return 0;
  return 1;
}

/* ISO9660 PVD: sector 16+ (2048-byte Mode-1 user sectors).
   Root directory record starts at byte 156. */
int desr_iso_root(const unsigned char *pvd, size_t n,
                  uint32_t *extent, uint32_t *bytes) {
  const unsigned char *r;
  if (!pvd || n != 2048 || !extent || !bytes) return -1;
  if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) || pvd[6] != 1)
    return -1;
  r = pvd + 156;
  if (r[0] < 34 || r[32] != 1) return -1;
  *extent = le32(r + 2);
  *bytes = le32(r + 10);
  if (!*extent || !*bytes) return -1;
  return 0;
}

/* Parse one complete 2048-byte root directory sector. 0: absent;
   1: SYSTEM.CNF found; -1: malformed sector. */
int desr_iso_system_record(const unsigned char *block, size_t n,
                           uint32_t *extent, uint32_t *bytes) {
  size_t at = 0;
  if (!block || n != 2048 || !extent || !bytes) return -1;
  while (at < n) {
    size_t len = block[at], namelen;
    const unsigned char *r;
    if (!len) return 0; /* padding to next sector */
    if (len < 34 || at + len > n) return -1;
    r = block + at;
    namelen = r[32];
    if (namelen == 0 || 33 + namelen > len) return -1;
    if (!(r[25] & 2) &&
        (match(r + 33, namelen, "SYSTEM.CNF;1") ||
         match(r + 33, namelen, "SYSTEM.CNF"))) {
      *extent = le32(r + 2);
      *bytes = le32(r + 10);
      return *extent && *bytes ? 1 : -1;
    }
    at += len;
  }
  return 0;
}

/*
 * Look for a unique executable with a conventional PS2 boot ID in the root
 * ISO9660 directory. This is a read-only fallback if SYSTEM.CNF/BOOT2 is
 * unavailable. Never guess between two different boot candidates.
 */
int desr_iso_root_boot_candidate(const unsigned char *block, size_t n,
                                 char out[DESR_COVER_ID_CAP]) {
  size_t at = 0;
  int found = 0;
  if (!block || n != 2048 || !out) return -1;
  out[0] = '\0';
  while (at < n) {
    const unsigned char *r;
    size_t len = block[at], namelen, k;
    char candidate[DESR_COVER_ID_CAP];
    int valid_suffix = 1;
    if (!len) break; /* Zero padding at sector tail */
    if (len < 34 || at + len > n) return -1;
    r = block + at;
    namelen = r[32];
    if (namelen == 0 || 33 + namelen > len) return -1;
    if (!(r[25] & 2) && namelen >= 11) {
      for (k = 0; k < 11; k++)
        candidate[k] = (char)up(r[33 + k]);
      candidate[11] = '\0';
      if (namelen > 11) {
        if (r[33 + 11] != ';' || namelen == 12) valid_suffix = 0;
        for (k = 12; k < namelen; k++) {
          if (r[33 + k] < '0' || r[33 + k] > '9')
            valid_suffix = 0;
        }
      }
      if (valid_suffix && desr_valid_game_id(candidate)) {
        if (found && strcmp(out, candidate)) {
          out[0] = '\0';
          return 2;
        }
        memcpy(out, candidate, sizeof(candidate));
        found = 1;
      }
    }
    at += len;
  }
  return found;
}

int desr_valid_game_id(const char *s) {
  int i;
  if (!s || strlen(s) != 11) return 0;
  for (i = 0; i < 4; i++) if (s[i] < 'A' || s[i] > 'Z') return 0;
  if (s[4] != '_' || s[8] != '.') return 0;
  for (i = 5; i <= 10; i++) {
    if (i == 8) continue;
    if (s[i] < '0' || s[i] > '9') return 0;
  }
  return 1;
}

/* Parse BOOT2=cdrom0:\\SLES_123.45;1, case-insensitively.
   Match identifiers only on BOOT2 lines (not VER/VMODE/comments). */
int desr_cnf_game_id(const char *cnf, size_t n,
                     char out[DESR_COVER_ID_CAP]) {
  size_t at = 0;
  if (!cnf || !out || n > 8192) return 0;
  out[0] = '\0';
  while (at < n) {
    size_t end = at, p, i;
    while (end < n && cnf[end] != '\n' && cnf[end] != '\r') end++;
    p = at;
    while (p < end && (cnf[p] == ' ' || cnf[p] == '\t')) p++;
    if (end - p >= 5 && up(cnf[p]) == 'B' &&
        up(cnf[p+1]) == 'O' && up(cnf[p+2]) == 'O' &&
        up(cnf[p+3]) == 'T' && cnf[p+4] == '2') {
      p += 5;
      while (p < end && (cnf[p] == ' ' || cnf[p] == '\t')) p++;
      if (p < end && cnf[p] == '=') {
        p++;
        for (i = p; i + 11 <= end; i++) {
          char id[DESR_COVER_ID_CAP];
          size_t j;
          for (j = 0; j < 11; j++) id[j] = (char)up((unsigned char)cnf[i+j]);
          id[11] = '\0';
          if (desr_valid_game_id(id)) {
            memcpy(out, id, sizeof(id));
            return 1;
          }
        }
      }
    }
    at = end;
    while (at < n && (cnf[at] == '\r' || cnf[at] == '\n')) at++;
  }
  return 0;
}

static int valid_ext(const char *ext) {
  return ext && (!strcmp(ext, "png") || !strcmp(ext, "jpg"));
}

int desr_cover_format(char *out, size_t cap, const char *filename,
                      const desr_cover_entry *cover) {
  int n;
  if (!out || !cap || !cover || !cover->present ||
      !desr_filename_is_safe(filename) ||
      !desr_valid_game_id(cover->game_id) || !valid_ext(cover->ext)) return -1;
  n = snprintf(out, cap, "%s\t%s\t%s\n",
               filename, cover->game_id, cover->ext);
  if (n < 0 || (size_t)n >= cap) return -1;
  return n;
}

/* line is mutable, without trailing LF; reject untrusted records. */
int desr_cover_parse(char *line, char filename[DESR_FILENAME_CAP],
                     desr_cover_entry *cover) {
  char *id, *ext;
  if (!line || !filename || !cover) return 0;
  id = strchr(line, '\t');
  if (!id) return 0;
  *id++ = '\0';
  ext = strchr(id, '\t');
  if (!ext) return 0;
  *ext++ = '\0';
  if (strchr(ext, '\t') || !desr_filename_is_safe(line) ||
      !desr_valid_game_id(id) || !valid_ext(ext)) return 0;
  memcpy(filename, line, strlen(line) + 1);
  memset(cover, 0, sizeof(*cover));
  memcpy(cover->game_id, id, 12);
  memcpy(cover->ext, ext, strlen(ext) + 1);
  cover->present = 1;
  return 1;
}
