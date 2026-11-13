#!/usr/bin/env python3
"""Source-audit invariants only. Never boots a game or touches a PSX HDD."""
from pathlib import Path
import sys


def require(text: str, fragment: str, where: str) -> None:
    if fragment not in text:
        raise AssertionError(f"{where}: missing expected source fragment: {fragment}")


def read(root: Path, path: str) -> str:
    return (root / path).read_text(encoding="utf-8")


def run(manager: Path, neutrino: Path, osdmenu: Path, ps2sdk: Path) -> None:
    m = read(manager, "dvr-iso-manager/src/main.c")
    cnf = read(manager, "dvr-iso-manager/src/hdd_boot_config.c")
    lang = read(manager, "neutrino-v02/neutrino-osdlang-en.patch")
    for expr in (
        "sceCdNoticeGameStart(0, &notice_arg)",
        "sceCdNoticeGameStart(0, NULL)",
    ):
        require(m, expr, "manager 1.0")
    for expr in ("boot=neutrino_hdd.elf", "boot=neutrino_hdd_lang.elf",
                 "arg=-bsd=dvr", "arg=-dvd=%s"):
        require(cnf, expr, "HDD boot config")
    assert "arg=-qb" not in cnf, "OSDMenu quickboot was confused with Neutrino -qb"
    for expr in ("SetOsdConfigParam", "GetOsdConfigParam", "osdlang_mode"):
        require(lang, expr, "LANG")
    assert "sceCdNoticeGameStart" not in lang

    n = read(neutrino, "ee/loader/src/main.c")
    require(n, 'if (sys.bQuickBoot == 0) {', "Neutrino")
    require(n, 'else if (!strncmp(argv[i], "-qb", 3))', "Neutrino")
    require(n, "sceCdApplySCmd(0x29, in, 4, out);", "Neutrino")
    require(n, "uint8_t in[4] = {0};", "Neutrino")
    exports = read(neutrino, "iop/cdvdman_emu/src/exports.tab")
    require(exports, "// sceCdNoticeGameStart", "Neutrino CDVDMAN")
    assert "DECLARE_EXPORT(sceCdNoticeGameStart)" not in exports

    psxinit = read(osdmenu, "common/src/psxinit.c")
    require(psxinit, "custom_sceCdNoticeGameStart(1, &stat)", "OSDMenu mbr")
    launcher = read(osdmenu, "launcher/src/handler_quickboot.c")
    assert "switchPSX()" not in launcher
    require(launcher, "initModules(dtype)", "OSDMenu launcher")
    require(read(osdmenu, "mbr/src/common.c"), "switchPSX();", "OSDMenu mbr")

    scmd = read(ps2sdk, "ee/rpc/cdvd/src/scmd.c")
    require(scmd, "sceCdApplySCmd(0x29u, in, 1, out);", "PS2SDK")
    require(scmd, "in[0] = arg1;", "PS2SDK")
    print("PASS: pinned source facts match the audit; this is NOT a hardware test.")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        raise SystemExit(
            "usage: check_invariants.py MANAGER_ROOT NEUTRINO_ROOT OSDMENU_ROOT PS2SDK_ROOT"
        )
    run(*(Path(p) for p in sys.argv[1:]))
