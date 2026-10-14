#include "iso_utils.h"
#include <stdio.h>
#include <string.h>

/* Case-insensitive .iso extension, including .ISO. */
int desr_is_iso_filename(const char *name) {
  size_t n;
  if (!name) return 0;
  n = strlen(name);
  if (n <= 4) return 0;
  return name[n - 4] == '.' &&
         (name[n - 3] == 'i' || name[n - 3] == 'I') &&
         (name[n - 2] == 's' || name[n - 2] == 'S') &&
         (name[n - 1] == 'o' || name[n - 1] == 'O');
}

/* Reject any input that could escape ISO/ or inject CNF directives. */
int desr_filename_is_safe(const char *name) {
  size_t i, n;
  if (!name) return 0;
  n = strlen(name);
  if (!n || n >= DESR_FILENAME_CAP || !desr_is_iso_filename(name)) return 0;
  if (name[0] == '.') return 0;
  for (i = 0; i < n; ++i) {
    unsigned char c = (unsigned char)name[i];
    if (c < 0x20 || c == 0x7f || c == '/' || c == '\\' || c == ':') return 0;
  }
  return 1;
}

int desr_make_iso_path(const char *filename, char *out, size_t out_size) {
  int n;
  if (!out || out_size == 0 || !desr_filename_is_safe(filename)) return -1;
  n = snprintf(out, out_size, "%s%s", DESR_ISO_ROOT, filename);
  if (n < 0 || (size_t)n >= out_size || n >= 240) return -1;
  return n;
}

/* Always generates ASCII-compatible CNF preamble, with no UTF-8 BOM.
   Filename bytes are copied unchanged for firmware/driver case matching.
   Note: caller writes exactly the returned byte count, not sizeof(out). */
int desr_make_boot_cnf(const char *filename, char *out, size_t out_size) {
  char iso_path[256];
  int n;
  if (!out || out_size < 1) return -1;
  if (desr_make_iso_path(filename, iso_path, sizeof(iso_path)) < 0) return -1;
  n = snprintf(out, out_size,
               "boot=neutrino.elf\n"
               "arg=-bsd=dvr\n"
               "arg=-dvd=%s\n", iso_path);
  if (n < 0 || (size_t)n >= out_size || n >= 255) return -1;
  return n;
}
