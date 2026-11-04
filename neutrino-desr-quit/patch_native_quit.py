#!/usr/bin/env python3
"""Apply only native DESR QUIT setup to the pinned PR228 loader.

Run after the optional OSD English patch and HDD FIX95 wrapper. Refuse
unrecognized source. The preceding pad-based IGR prototype is not included.
"""
from pathlib import Path
import sys


def patch(text):
    def once(old, new):
        nonlocal text
        if text.count(old) != 1:
            raise ValueError(f"unexpected source: {text.count(old)} copies of {old[:70]!r}")
        text = text.replace(old, new)

    once('#include "tomlc17.h"', '#include "tomlc17.h"\n#include "desr_quit.h"')
    once('''            sceCdInit(SCECdINoD);
            uint8_t in[4] = {0};
            uint8_t out[16] = {};
            sceCdApplySCmd(0x29, in, 4, out);
            sceCdInit(SCECdEXIT);''', '''            if (desr_enable_quit_game("ROM before LE") < 0)
                return -97;''')
    # Outside the !QuickBoot branch: HDD QuickBoot previously skipped the SCMD.
    # Reassert after the standard path's second IOP reset, before EE emulation.
    once('''    // Load EE_CORE settings
''', '''    if (!strncmp(sys.sBSD, "dvr", 3) &&
        desr_enable_quit_game(sys.bQuickBoot ? "QuickBoot before EE" : "LE before EE") < 0)
        return -97;

    // Load EE_CORE settings
''')
    return text


if __name__ == "__main__":
    source = Path(sys.argv[1])
    result = patch(source.read_text())
    source.with_name("desr_quit.h").write_text(Path(__file__).with_name("desr_quit.h").read_text())
    source.write_text(result)
    print("Native DESR QUIT: mode 1 / one byte / checked status / normal + QuickBoot")
