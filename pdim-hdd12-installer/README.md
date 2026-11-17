# PDIM HDD12 TEST Installer

**EXPERIMENTAL — NOT HARDWARE VERIFIED. NO PUBLIC RELEASE.**

**WARNING: CREATING AN APA PARTITION MODIFIES THE WHOLE HDD'S PARTITION
METADATA. DAMAGE CAN AFFECT ANY PARTITION, NOT JUST PP.DVRISO12. A FULL,
RECOVERABLE SECTOR-LEVEL HDD BACKUP IS REQUIRED BEFORE ANY WRITE.**

The installer is a separate MIPS ELF launched from FAT32 USB via
wLaunchELF-R3Z 4.80 on a Sony PSX DESR-7000. It creates (only if absent)
an exact \`PP.DVRISO12\` 256 MiB PFS test partition, then copies and
verifies files from the PDIM HDD 1.0 release. The Manager and Neutrino
are **rebuilt for PP.DVRISO12** so they cannot accidentally mount the
existing \`PP.DVRISO\`. This is not the HDD HTTPS 1.2 port yet.

Large uppercase warnings must be acknowledged by X followed by
L1+R1+X before any write. No automatic deletion, resize, or reformat
of existing partitions is implemented. XMB setup is a separate optional
action and uses the already-tested 1.0 KELF and PDIM artwork assets.
Read \`README_PL.md\` for complete details.

## Hardware R1 -> R2: IOP module loading and screen cursor

The first DESR-7000 run of the installer **stopped before any HDD write**:
`LOAD fileXio id=-200 rv=0`. In PS2SDK this indicates an IOP module
dependency error. The initial implementation tried to reuse modules
from wLaunchELF-R3Z using only their names; that can mistake Sony's
original file manager for the PS2SDK `iomanX` export library required
by `fileXio`.

**R2** uses the same clean IOP-reset/module-loading approach as the
tested PDIM HDD 1.0: iomanX, fileXio, a single trusted `dev9_ns` module,
ATA, HDD/PFS, USB and pad. Any failure still stops before writes.
R2 also calls `scr_setCursor(0)` after `init_scr()` to remove the
PS2SDK's white block cursor, as in the working PDIM interface.

**Hardware status:** R2 has not yet been verified on the DESR-7000.
First run it and report whether the startup passes `fileXio` and
reaches `PP.DVRISO12` preflight. **Do not use X/Triangle to modify
the disk before a verified full sector-level HDD backup.**

