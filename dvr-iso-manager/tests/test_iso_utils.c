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

static void test_lang(const char *filename, const char *expected) {
  char cnf[320];
  int n = desr_make_boot_cnf_lang(filename, 1, cnf, sizeof(cnf));
  assert(n > 0);
  assert((size_t)n == strlen(cnf));
  assert(strcmp(cnf, expected) == 0);
  assert((unsigned char)cnf[0] == 'b');
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
  test_lang("REZ.iso",
            "boot=neutrino_lang.elf\n"
            "arg=-bsd=dvr\n"
            "arg=-dvd=dvr_pfs0:/ISO/REZ.iso\n"
            "arg=-cfg=osdlang\n"
            "arg=-osdlang=en\n");
  test_lang("Metal Slug Anthology.iso",
            "boot=neutrino_lang.elf\n"
            "arg=-bsd=dvr\n"
            "arg=-dvd=dvr_pfs0:/ISO/Metal Slug Anthology.iso\n"
            "arg=-cfg=osdlang\n"
            "arg=-osdlang=en\n");
  {
    char too_short[16];
    assert(desr_make_boot_cnf_lang("REZ.iso", 1, too_short, sizeof(too_short)) < 0);
    assert(desr_make_boot_cnf_lang("REZ.iso", 2, too_short, sizeof(too_short)) < 0);
    assert(desr_make_boot_cnf_lang("../REZ.iso", 1, too_short, sizeof(too_short)) < 0);
  }
  puts("OK: names, traversal/injection, system and English CNF without BOM");
  return 0;
}
