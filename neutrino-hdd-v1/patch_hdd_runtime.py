#!/usr/bin/env python3
"""Neutrino HDD 1.0 runtime patch (source-only, pinned pcm720 PR228).

This is a small extraction of the hardware-proven DIAG Z FIX95 mechanism:
after original OSDMenu Quickboot, remount our isolated app partition RO,
set its PFS cwd and then run upstream Neutrino as-is.

No diagnostic stage stops, sleep loops, ISO rewrites, disk writes or other
changes to pcm720's DVR FHI/game setup. English builds additionally apply
the existing neutrino-v02/neutrino-osdlang-en.patch *before this script*.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1])
s = source.read_text(encoding="utf-8")

def once(old, new):
    global s
    count = s.count(old)
    if count != 1:
        raise SystemExit(f"Neutrino HDD refuses unexpected upstream: {count} occurrences of {old[:70]!r}")
    s = s.replace(old, new)

once("int main(int argc, char *argv[])\n{",
     "static int neutrino_hdd_upstream_main(int argc, char *argv[])\n{")

s += r'''

/* PSX DVR ISO MANAGER 1.0 HDD
 * Separate user application partition; does not touch Sony system partitions.
 * OSDMenu has already passed its exact Quickboot argv and can leave pfs0
 * unmounted. The same remount/chdir fixed actual hardware DIAG W...Z
 * and allowed a real PS2 game to run via dvrfile_fhi on DESR-7000.
 */
static int desr_iso_arg_allowed(const char *arg)
{
    const char prefix[]="-dvd=dvr_pfs0:/ISO/";
    size_t i, len, prefix_len=sizeof(prefix)-1;
    if (!arg || strncmp(arg,prefix,prefix_len)!=0) return 0;
    len=strlen(arg);
    if (len<=prefix_len+4 || len>prefix_len+95) return 0;
    if (strcmp(arg+len-4,".iso") && strcmp(arg+len-4,".ISO"))
        return 0;
    for (i=prefix_len; i<len; ++i) {
        unsigned char c=(unsigned char)arg[i];
        if (c<0x20 || c>0x7e || c=='/' || c=='\\' || c==':' || c=='=')
            return 0;
    }
    if (strstr(arg+prefix_len,"..")) return 0;
    return 1;
}

int main(int argc, char *argv[])
{
    int rc, mount_rc;
    int english;
    const char *system_target =
        "hdd0:PP.DVRISO:pfs:/neutrino_hdd.elf";
    const char *english_target =
        "hdd0:PP.DVRISO:pfs:/neutrino_hdd_lang.elf";
    if (!argv || (argc!=3 && argc!=5) || !argv[0] || !argv[1] ||
        !argv[2] || strcmp(argv[1],"-bsd=dvr") ||
        !desr_iso_arg_allowed(argv[2]))
        return -100;

    english=(argc==5);
    if (english) {
        if (!argv[3] || !argv[4] ||
            strcmp(argv[3],"-cfg=osdlang") ||
            strcmp(argv[4],"-osdlang=en") ||
            strcmp(argv[0],english_target))
            return -101;
    } else if (strcmp(argv[0],system_target)) {
        return -102;
    }

    SifInitRpc(0);
    rc=fileXioInit();
    if (rc<0) return -103;
    mount_rc=fileXioMount("pfs0:", "hdd0:PP.DVRISO", FIO_MT_RDONLY);
    if (mount_rc<0) return -104;
    if (chdir("pfs0:/")<0) {
        fileXioUmount("pfs0:");
        return -105;
    }

    /* Neutrino reads the config and modules directories from pfs0:/.
     * The ISO remains on the completely separate dvr_pfs0:/ISO/ store.
     */
    rc=neutrino_hdd_upstream_main(argc,argv);
    /* Usually ExecPS2 does not return. If it does, undo only our mount. */
    chdir("hdd0:");
    fileXioUmount("pfs0:");
    return rc;
}
'''
if "PP.TESTDVR" in s:
    raise SystemExit("Unexpected test partition in production source")
if 'fileXioMount("pfs0:", "hdd0:PP.DVRISO", FIO_MT_RDONLY)' not in s:
    raise SystemExit("HDD partition RO mount missing")
if 'neutrino_hdd_upstream_main(argc,argv)' not in s:
    raise SystemExit("Upstream Neutrino call missing")
source.write_text(s,encoding="utf-8")
print("HDD 1.0 runtime: pinned Neutrino + verified RO PFS remount/chdir + English argv allowlist")
