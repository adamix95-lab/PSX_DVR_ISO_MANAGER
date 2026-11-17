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
