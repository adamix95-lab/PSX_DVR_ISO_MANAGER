#!/usr/bin/env python3
"""G4 controlled PSX boot experiment: send switch-to-PS2 RPC before NoticeGameStart.

Sony / pcm720 OSDMenu PSX sequence:
  SifIopReset("rom0:UDNL rom0:OSDCNF"); sceCdInit();
  custom_sceCdChgSys(2); custom_sceCdNoticeGameStart(1).
Our pinned Neutrino DVR loader only performed the 0x29 part.
Hardware G3A + G2B: mode1 enables physical QUIT->XMB but games black-screen.
Hardware G3B: mode0 allows games but does not enable QUIT.

G4A tests mode0 + added ChangeSys(2) (CONTROL — must boot before G4B).
G4B tests mode1 + exactly the same added ChangeSys(2).
Both use 1-byte NoticeGameStart as PS2SDK does.

This call changes the optical drive mode. TEST ONLY on real DESR with USB
runtime, do not ship as part of stable HDD 1.0. No partition writes or IGR.
Ported from pcm720/OSDMenu common/src/psxinit.c (AFL-3.0):
https://github.com/pcm720/OSDMenu/blob/a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f/common/src/psxinit.c
"""
import argparse
import difflib
from pathlib import Path

# Contains precisely the earlier DVR code. Fail if upstream source changes.
OLD = """            sceCdInit(SCECdINoD);
            uint8_t in[4] = {0};
            uint8_t out[16] = {};
            sceCdApplySCmd(0x29, in, 4, out);
            sceCdInit(SCECdEXIT);"""

# The original OSDMenu change-system call is NOT an ordinary MechaCon SCMD.
# 0x2D is the CDVD SIF RPC function number. Do NOT use sceCdApplySCmd(0x2D).
HELPER = r"""
/*
 * Experimental G4 only: standard Sony/OSDMenu PSX change-system RPC.
 * Derived from pcm720/OSDMenu common/src/psxinit.c (AFL-3.0).
 * Bounded to ONE invocation; no infinite retry loop.
 */
extern SifRpcClientData_t clientSCmd;
extern int sCmdSemaId;
extern uint8_t sCmdRecvBuff[];
extern uint8_t sCmdSendBuff[];
extern int _CdCheckSCmd(int cmd);

static int desr_g4_change_system_ps2(void)
{
    int result = 0;
    if (_CdCheckSCmd(0x2D) == 0)
        return 0;
    *(uint32_t *)sCmdSendBuff = 2;
    if (SifCallRpc(&clientSCmd, 0x2D, 0, sCmdSendBuff, 4,
                   sCmdRecvBuff, 4, NULL, NULL) >= 0)
        result = *(int *)UNCACHED_SEG(sCmdRecvBuff);
    SignalSema(sCmdSemaId);
    return result;
}

"""


def patch(original: str, variant: str):
    if variant not in ("G4A", "G4B"):
        raise ValueError("Unsupported variant")
    if original.count(OLD) != 1 or original.count("void print_usage()") != 1:
        raise ValueError("Unexpected Neutrino source: required exact pinned site")
    # Only difference between A/B is mode0 vs mode1.
    mode = "0" if variant == "G4A" else "1"
    new = (
        "            int cdinit = sceCdInit(SCECdINoD);\n"
        f'            printf("G4: cdinit=%d\\n", cdinit);\n'
        '            if (cdinit <= 0) return -117;\n'
        "            int ps2_mode = desr_g4_change_system_ps2();\n"
        f'            printf("G4: switch=2 result=%d\\n", ps2_mode);\n'
        '            if (ps2_mode != 2) {\n'
        '                sceCdInit(SCECdEXIT);\n'
        '                return -118; // no NoticeGameStart on failed mode transition\n'
        '            }\n'
        f"            uint8_t in[4] = {{{mode}}};\n"
        "            uint8_t out[16] = {};\n"
        "            int scmd = sceCdApplySCmd(0x29, in, 1, out);\n"
        f'            printf("G4: notice_mode={mode} rpc=%d status=%02x\\n", scmd, out[0]);\n'
        "            sceCdInit(SCECdEXIT);"
    )
    changed = original.replace(OLD, new)
    changed = changed.replace("void print_usage()", HELPER+"void print_usage()")
    assert "sceCdApplySCmd(0x29, in, 4, out);" not in changed
    assert changed.count("desr_g4_change_system_ps2();") == 1
    assert changed.count("SifCallRpc(&clientSCmd, 0x2D") == 1
    assert changed.count("sceCdApplySCmd(0x29, in, 1, out);") == 1
    assert "sceCdApplySCmd(0x2D" not in changed
    return changed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("variant", choices=["G4A","G4B"])
    args = ap.parse_args()
    before = args.source.read_text(encoding="utf-8")
    after = patch(before, args.variant)
    args.source.write_text(after, encoding="utf-8")
    print(f"{args.variant}: OSDMenu-style PSX switch mode RPC 0x2D, then one-byte Notice 0x29")
    print("No new IOP reset, hooked syscall, resident thread or disk write.")
    print("".join(difflib.unified_diff(
        before.splitlines(True), after.splitlines(True),
        fromfile="pinned Neutrino 1.0", tofile=args.variant)))


if __name__ == "__main__":
    main()
