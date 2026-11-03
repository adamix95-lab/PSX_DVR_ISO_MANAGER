#!/usr/bin/env python3
"""Diagnostic-only, pin-checked trace for Neutrino HDD STAGE=60.

Find the exact failed TOML file (including recursive "depends" config).
It does not change config selection, module list, disc FHI or success flow.
Apply after patch_hdd_runtime.py and patch_hdd_stages.py, before
patch_hdd_error_screen.py.  Only the HDD feature-branch CI uses this.
"""
from pathlib import Path
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: patch_hdd_config_trace.py main.c config.c")
main_path, config_path = map(Path, sys.argv[1:])
main = main_path.read_text(encoding="utf-8")
config = config_path.read_text(encoding="utf-8")


def unique_replace(source: str, old: str, new: str, label: str) -> str:
    n = source.count(old)
    if n != 1:
        raise SystemExit(f"CONFIG DIAG refused {label}: expected one anchor, found {n}")
    return source.replace(old, new)


config = unique_replace(config,
    '''static const char *g_config_prefix = "config/";''',
    '''static const char *g_config_prefix = "config/";

/* Bounded independent buffers survive TOML free/recursive dependencies. */
char desr_hdd_last_toml[128] = "(none)";
char desr_hdd_failed_toml[128] = "(none)";
char desr_hdd_toml_reason[92] = "(none)";
int desr_hdd_config_error_kind = 0;
/* kind 0=none, 1=TOML open/parse, 2=load_config invalid contents */
''',
    "config trace globals")

config = unique_replace(config,
    '''    res = toml_parse_file_ex(filename);
    if (!res.ok)
        printf("ERROR: failed to load and parse %s: %s\\n", filename, res.errmsg);''',
    '''    snprintf(desr_hdd_last_toml, sizeof(desr_hdd_last_toml),
             "%.120s", filename);
    res = toml_parse_file_ex(filename);
    if (!res.ok) {
        snprintf(desr_hdd_failed_toml, sizeof(desr_hdd_failed_toml),
                 "%.120s", filename);
        snprintf(desr_hdd_toml_reason, sizeof(desr_hdd_toml_reason),
                 "%.90s", res.errmsg);
        desr_hdd_config_error_kind = 1;
        printf("ERROR: failed to load and parse %s: %s\\n", filename, res.errmsg);
    }''',
    "TOML parser result")

config = unique_replace(config,
    '''        int rv = load_config(res.toptab);
        toml_free(res);
        return rv;''',
    '''        int rv = load_config(res.toptab);
        /* Do not mask an inner dependency failure with the outer file. */
        if (rv < 0 && desr_hdd_config_error_kind == 0) {
            snprintf(desr_hdd_failed_toml, sizeof(desr_hdd_failed_toml),
                     "%.40s%.30s%.1s%.30s.toml", g_config_prefix, type,
                     subtype != NULL ? "-" : "",
                     subtype != NULL ? subtype : "");
            snprintf(desr_hdd_toml_reason, sizeof(desr_hdd_toml_reason),
                     "load_config returned %d", rv);
            desr_hdd_config_error_kind = 2;
        }
        toml_free(res);
        return rv;''',
    "recursive load_config failure")

main = unique_replace(main,
    '''#include "config.h"''',
    '''#include "config.h"

/* From config.c, all read-only diagnostics (not part of original Neutrino). */
extern char desr_hdd_last_toml[128];
extern char desr_hdd_failed_toml[128];
extern char desr_hdd_toml_reason[92];
extern int desr_hdd_config_error_kind;''',
    "diagnostic externs")

if "desr_hdd_stage = 60" not in main:
    raise SystemExit("Apply stage markers BEFORE config diagnostics")
if "strcmp(sDVDFile, argv[2] + 5) != 0" not in main:
    raise SystemExit("HDD FIX95 missing")
if "PP.TESTDVR" in main or "PP.DVRISO" not in main:
    raise SystemExit("Unexpected test/application partition")

main_path.write_text(main, encoding="utf-8")
config_path.write_text(config, encoding="utf-8")
print("HDD CONFIG TRACE: last config + nested failure + error reason; no behavior change")
