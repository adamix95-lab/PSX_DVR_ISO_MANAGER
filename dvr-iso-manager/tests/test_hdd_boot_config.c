#include "hdd_boot_config.h"
#include "iso_utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char cnf[384];
    char tiny[16];
    const char system[] =
        "boot=neutrino_hdd.elf\n"
        "arg=-bsd=dvr\n"
        "arg=-dvd=dvr_pfs0:/ISO/MGS 2.ISO\n"
        "arg=-dev9=NICHDD\n"
        "#PAD\n";
    const char english[] =
        "boot=neutrino_hdd_lang.elf\n"
        "arg=-bsd=dvr\n"
        "arg=-dvd=dvr_pfs0:/ISO/REZ.iso\n"
        "arg=-cfg=osdlang\n"
        "arg=-osdlang=en\n"
        "arg=-dev9=NICHDD\n"
        "#PAD\n";
    int n;
    n=desr_make_hdd_boot_cnf("MGS 2.ISO",0,cnf,sizeof cnf);
    assert(n == (int)strlen(system));
    assert(!memcmp(cnf,system,(size_t)n));
    n=desr_make_hdd_boot_cnf("REZ.iso",1,cnf,sizeof cnf);
    assert(n == (int)strlen(english));
    assert(!memcmp(cnf,english,(size_t)n));
    assert(n >= 5 && !memcmp(cnf+n-5,"#PAD\n",5));
    assert(strstr(cnf,"arg=-dev9=NICHDD\n#PAD\n") != NULL);
    assert(desr_make_hdd_boot_cnf("../REZ.iso",0,cnf,sizeof cnf) < 0);
    assert(desr_make_hdd_boot_cnf("REZ.iso",0,tiny,sizeof tiny) < 0);
    assert(desr_make_hdd_boot_cnf("REZ.iso",2,cnf,sizeof cnf) < 0);
    assert(desr_make_hdd_boot_cnf("bad\narg=hack.iso",0,cnf,sizeof cnf) < 0);
    assert(desr_make_hdd_boot_cnf("REZ.iso",0,NULL,sizeof cnf) < 0);
    puts("HDD Quickboot CNF: system + English + #PAD + no invalid names — PASS");
    return 0;
}
