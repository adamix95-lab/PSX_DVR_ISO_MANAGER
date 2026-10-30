#include "cover_index.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put32(unsigned char *p, unsigned n) {
  p[0] = (unsigned char)n;
  p[1] = (unsigned char)(n >> 8);
  p[2] = (unsigned char)(n >> 16);
  p[3] = (unsigned char)(n >> 24);
}
static void test_iso_records(void) {
  unsigned char pvd[2048] = {0}, block[2048] = {0};
  unsigned extent = 0, bytes = 0;
  const char *name = "SYSTEM.CNF;1";
  size_t len = strlen(name);
  pvd[0] = 1;
  memcpy(pvd + 1, "CD001", 5);
  pvd[6] = 1;
  pvd[156] = 34;
  pvd[156+32] = 1; /* special root name */
  put32(pvd + 156 + 2, 1234);
  put32(pvd + 156 + 10, 4096);
  assert(desr_iso_root(pvd, sizeof(pvd), &extent, &bytes) == 0);
  assert(extent == 1234 && bytes == 4096);
  pvd[1] = 'X';
  assert(desr_iso_root(pvd, sizeof(pvd), &extent, &bytes) < 0);

  block[0] = (unsigned char)(33 + len + 1);
  block[32] = (unsigned char)len;
  memcpy(block + 33, name, len);
  put32(block + 2, 2048);
  put32(block + 10, 96);
  assert(desr_iso_system_record(block, sizeof(block), &extent, &bytes) == 1);
  assert(extent == 2048 && bytes == 96);
  block[32] = 252;
  assert(desr_iso_system_record(block, sizeof(block), &extent, &bytes) < 0);
  memset(block, 0, sizeof(block));
  assert(desr_iso_system_record(block, sizeof(block), &extent, &bytes) == 0);
}
static void iso_dir_entry(unsigned char *block, size_t offset,
                          const char *name, unsigned lba) {
  size_t name_len = strlen(name), rec_len = 33 + name_len;
  if (rec_len & 1U) rec_len++;
  assert(offset + rec_len <= 2048 && rec_len <= 255);
  block[offset] = (unsigned char)rec_len;
  block[offset + 32] = (unsigned char)name_len;
  memcpy(block + offset + 33, name, name_len);
  put32(block + offset + 2, lba);
  put32(block + offset + 10, 1024);
}

static void test_root_exe_fallback(void) {
  unsigned char block[2048] = {0};
  char id[DESR_COVER_ID_CAP] = {0};
  size_t offset;
  int result;
  /* A root PS2 executable can recover a game whose BOOT2 is inaccessible. */
  iso_dir_entry(block, 0, "SLUS_210.47;1", 333);
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) == 1);
  assert(!strcmp(id, "SLUS_210.47"));
  offset = block[0];
  iso_dir_entry(block, offset, "SYSTEM.CNF;1", 99);
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) == 1);
  assert(!strcmp(id, "SLUS_210.47"));

  /* Two different PS2 executables are ambiguous; never guess. */
  offset += block[offset];
  iso_dir_entry(block, offset, "SCES_533.26;1", 456);
  result = desr_iso_root_boot_candidate(block, sizeof(block), id);
  assert(result == 2 && id[0] == '\0');

  /* No false positives for partial IDs, extra suffix or directories. */
  memset(block, 0, sizeof(block));
  iso_dir_entry(block, 0, "SLUS_210.4;1", 12);
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) == 0);
  memset(block, 0, sizeof(block));
  iso_dir_entry(block, 0, "SLUS_210.47.ELF;1", 12);
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) == 0);
  memset(block, 0, sizeof(block));
  iso_dir_entry(block, 0, "sles_538.86;1", 12);
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) == 1);
  assert(!strcmp(id, "SLES_538.86"));
  block[25] = 2; /* directory rather than file */
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) == 0);
  block[0] = 2; /* malformed record */
  assert(desr_iso_root_boot_candidate(block, sizeof(block), id) < 0);
}

static void test_cnf_ids(void) {
  char id[DESR_COVER_ID_CAP];
  const char cnf[] = "BOOT2 = cdrom0:\\SLES_544.39;1\r\nVER = 1.0\r\n";
  assert(desr_cnf_game_id(cnf, strlen(cnf), id) == 1);
  assert(!strcmp(id, "SLES_544.39"));
  assert(desr_cnf_game_id("boot2=cdrom0:\\slus_210.59;1\n", 29, id) == 1);
  assert(!strcmp(id, "SLUS_210.59"));
  assert(desr_cnf_game_id("# BOOT2=SLUS_210.59\n", 20, id) == 0);
  assert(desr_cnf_game_id("BOOT = SLUS_210.59\n", 19, id) == 0);
  assert(!desr_valid_game_id("SLES_544.3X"));
  assert(!desr_valid_game_id("../x"));
}
static void test_index(void) {
  desr_cover_entry a = {{0}, {0}, 1}, b;
  char line[256], filename[DESR_FILENAME_CAP];
  int n;
  strcpy(a.game_id, "SLES_544.39");
  strcpy(a.ext, "png");
  n = desr_cover_format(line, sizeof(line), "Okami.iso", &a);
  assert(n > 0);
  assert(!strcmp(line, "Okami.iso\tSLES_544.39\tpng\n"));
  line[n-1] = '\0';
  assert(desr_cover_parse(line, filename, &b));
  assert(!strcmp(filename, "Okami.iso"));
  assert(b.present && !strcmp(b.game_id, "SLES_544.39"));
  assert(!strcmp(b.ext, "png"));
  assert(desr_cover_format(line, sizeof(line), "../bad.iso", &a) < 0);
  assert(desr_cover_format(line, 5, "Okami.iso", &a) < 0);
  strcpy(line, "Okami.iso\tSLES_544.39\tgif");
  assert(!desr_cover_parse(line, filename, &b));
  strcpy(line, "Okami.iso\tSLES_544.39\tpng\tmore");
  assert(!desr_cover_parse(line, filename, &b));
}
int main(void) {
  test_iso_records();
  test_root_exe_fallback();
  test_cnf_ids();
  test_index();
  puts("OK: bounded ISO9660/PVD, SYSTEM.CNF Game ID, and safe OPL cover index");
  return 0;
}
