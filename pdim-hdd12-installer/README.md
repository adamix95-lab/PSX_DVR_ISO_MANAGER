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


## R3: automatic XMB jackets and wLE-R3Z compatible PPAA sector writer

The R2 installer was hardware-tested on the DESR-7000: creating the
`PP.DVRISO12` partition and copying the 1.0 HDD payload worked, but XMB
registration returned FAIL -31. Sony XMB booted and the original PDIM 1.0
HDD still worked. The old R2 writer read the PPAA region through a raw
partition file and incorrectly treated PNG jacket data as a list.ico.

R3 uses the **physical sector-based approach from wLE-R3Z** to modify
only PPAA sectors at partition start+8,+9,+10, verifying each write
and leaving unrelated APA/PFS sectors intact. The PPAA text descriptors
point to `system.cnf` and `icon.sys`, not PNG artwork.

**X** creates the test partition if absent and copies the application
core. **SQUARE** checks only the installed core files read-only.
**SELECT** inspects the PPAA header without writing. **TRIANGLE**
registers the PPAA header and then automatically installs
`res/info.sys`, `res/jkt_001.png`, `res/jkt_002.png`, checking their
CRC and finally verifying the whole manifest (84/84 files).
No manual wLE file copy is needed. All console commands and warnings
are **English**. The existing PDIM 1.0 HDD and USB 1.2 remain untouched.

If power is lost after header registration but before the PFS jackets
are in place, the incomplete entry may appear as Corrupted Data until
the TRIANGLE operation is completed again. Do not delete the partition
automatically. **R3 hardware behavior has not yet been verified.**
Keep a restorable full-sector HDD backup before any write.

Source references:
- https://github.com/NathanNeurotic/wLaunchELF_R3Z/blob/master/src/hdd_header_injector.c
- https://github.com/shefben/psx_udpfs_xmb_game_installer/blob/main/src/apa_osd_header.c
- https://github.com/TTRMG3/DESR_XMB_INSTALLER (README-only at inspection)
