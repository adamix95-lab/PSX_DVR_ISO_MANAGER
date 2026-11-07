#include "hdd_boot_config.h"
#include "iso_utils.h"
#include <stdio.h>
#include <string.h>

int desr_make_hdd_boot_cnf(const char *iso_filename,
                           int force_english, char *out, size_t out_cap)
{
    char path[256];
    int len;
    if (!out || out_cap == 0 || (force_english != 0 && force_english != 1))
        return -1;
    if (desr_make_iso_path(iso_filename, path, sizeof(path)) < 0)
        return -1;

    /* OSDMenu consumes -dev9=NICHDD, leaving exactly the Neutrino args.
     * The arbitrary final byte belongs to #PAD, NEVER an actual arg line.
     */
    if (force_english)
        len = snprintf(out, out_cap,
             "boot=neutrino_hdd_lang.elf\n"
             "arg=-bsd=dvr\n"
             "arg=-dvd=%s\n"
             "arg=-cfg=osdlang\n"
             "arg=-osdlang=en\n"
             "arg=-dev9=NICHDD\n"
             "#PAD\n", path);
    else
        len = snprintf(out, out_cap,
             "boot=neutrino_hdd.elf\n"
             "arg=-bsd=dvr\n"
             "arg=-dvd=%s\n"
             "arg=-dev9=NICHDD\n"
             "#PAD\n", path);

    if (len <= 0 || (size_t)len >= out_cap || len > 255)
        return -1;
    return len;
}
