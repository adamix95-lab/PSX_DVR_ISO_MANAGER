#include "iso_utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_build(const char *filename, const char *expected) {
  char cnf[320];
  int n = desr_make_boot_cnf(filename, cnf, sizeof(cnf));
  assert(n > 0);
  assert((size_t)n == strlen(cnf));
  assert(strcmp(cnf, expected) == 0);
  assert((unsigned char)cnf[0] == 'b');  /* No BOM (EF BB BF) */
}

int main(void) {
  assert(desr_is_iso_filename("HALF-LIFE.ISO"));
  assert(desr_is_iso_filename("REZ.iso"));
  assert(desr_is_iso_filename("Wipeout Fusion.ISO"));
  assert(!desr_is_iso_filename("README.TXT"));
  assert(!desr_filename_is_safe("../REZ.ISO"));
  assert(!desr_filename_is_safe("/REZ.ISO"));
  assert(!desr_filename_is_safe("REZ.ISO\npath=bad"));
  assert(!desr_filename_is_safe("X:REZ.ISO"));
  assert(!desr_filename_is_safe("REZ\\X.ISO"));
  assert(desr_filename_is_safe("REZ.ISO"));
  test_build("REZ.ISO", "boot=neutrino.elf\narg=-bsd=dvr\narg=-dvd=dvr_pfs0:/ISO/REZ.ISO\n");
  test_build("HALF-LIFE.ISO", "boot=neutrino.elf\narg=-bsd=dvr\narg=-dvd=dvr_pfs0:/ISO/HALF-LIFE.ISO\n");
  test_build("Wipeout Fusion.ISO", "boot=neutrino.elf\narg=-bsd=dvr\narg=-dvd=dvr_pfs0:/ISO/Wipeout Fusion.ISO\n");
  puts("OK: names, traversal/injection, CNF with no BOM");
  return 0;
}
