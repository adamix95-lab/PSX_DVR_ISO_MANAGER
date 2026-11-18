# PDIM 1.2 HDD Installer — PP.DVRISO (pre-release)

**EXPERIMENTAL TEST. DO NOT PUBLISH 1.2 RELEASE. Hardware verification pending.**

## Installation partition and file split

All application files go to `hdd0:PP.DVRISO`. The physical game ISO store on the DVR filesystem is read-only to PDIM. A pre-existing PP.DVRISO PFS is NEVER reformatted, removed or resized. If absent, the installer can create an isolated 256 MiB PFS, which alters global APA metadata and requires a recoverable complete sector-level HDD backup.

| Button | Operation | Files |
| --- | --- | --- |
| X — Install Core | Install and CRC-check 79 runtime files | Manager 1.2 HTTPS, FIX95 Neutrino SYSTEM/ENGLISH, modules, BOOT.ELF, CA and configs |
| TRIANGLE — Install XMB | Copy, sync and CRC-check all five files before PPAA | EXECUTE.ELF, EXECUTE.CNF, res/info.sys, res/jkt_001.png, res/jkt_002.png |
| SQUARE | Read-only verification of Core | 79 files |
| SELECT | Read-only PPAA inspect | 3 target sectors |
| O | Exit | No writes |

**PPAA raw-sector write is last**: after the five PFS files and all 84 CRC checks, update only partition start+8/+9/+10 using the verified R3 method. If the USB copy fails, do not change PPAA.

The code recognizes a matching HDD 1.0 XMB header and safely changes only the known `title1=HDD 1.0` text to `title1=HDD 1.2`. A foreign/partial header is rejected. The signed EXECUTE.ELF from HDD 1.0 is reused unchanged.

## Existing HDD 1.0 installs

The existing Manager is the only executable which may differ from the 84-file release manifest. A replacement is first staged in `.PDIMNEW`, synchronized and CRC-checked. The old ELF is preserved as `DVR_ISO_MANAGER_HDD.ELF.PDIMBAK`, then the new ELF is moved into place and checked. Backups are never overwritten automatically; subsequent upgrades use .PDIMBAK2 through .PDIMBAK9 when necessary. If an interrupted installation leaves a `.PDIMNEW` or backup with a missing main ELF, stop and recover manually from backup, not via formatting.

The installer does not touch user `BOOT*.CNF`, `V12_NET.CNF`, `ART/`, cover indexes, game ISOs or unrelated APA partitions. The automatic package includes the user-provided `V12_NET.CNF` alongside its optional `.example`. During **X — Install Core**, the installer checks the existing `PP.DVRISO/V12_NET.CNF` first: when present it displays **NET CONFIG: KEEP** and never overwrites it. When missing, it validates the USB file using the same strict network parser as HTTPS, stages `V12_NET.CNF.PDIMNEW`, syncs and verifies CRC/readback, then installs it as `V12_NET.CNF` and displays **NET CONFIG: COPIED**. A missing/invalid USB config prints a warning and leaves the 79-file Core installation intact. A filesystem write/readback error stops the operation. `V12_NET.CNF` is deliberately NOT part of the immutable 84-file CRC manifest, because LAN settings are user-managed.

The included example values are IP `192.168.0.240`, NETMASK `255.255.255.0`, GATEWAY `192.168.0.1`, DNS `1.1.1.1`; they are valid for the contributor's network, **not universal defaults**. Edit the source file on USB if your router/subnet differs or the IP is already in use.

## Test on Sony PSX DESR-7000

1. Create/verify a **restorable full-sector HDD backup**. Also separately back up the current `PP.DVRISO` manager ELF, `V12_NET.CNF`, `BOOT.CNF` and `ART/`.
2. Extract the **CI test** zip to the root of a FAT32 USB flash drive. Launch `mass0:/PDIM_HDD_V12_INSTALLER/PDIM_HDD_V12_INSTALLER.ELF` in wLE-R3Z 4.80.
3. Inspect status without writes. Use SELECT read-only; verify exact target `PP.DVRISO`.
4. Press X, acknowledge warning, then hold L1+R1 and press X. Expect **79/79 core files** verified plus **V12_NET.CNF: COPIED** on a fresh install, or **KEPT** on a repeat. Existing PFS must not be formatted.
5. Before enabling XMB registration, launch `pfs1:/DVR_ISO_MANAGER_HDD.ELF` in wLE. Check list/launch and SYSTEM/ENGLISH.
6. Re-enter installer; press TRIANGLE, acknowledge twice. Expect five PFS files with CRC PASS **before** the PPAA write, then full **84/84** verification, then PPAA PASS.
7. Cold reboot DESR. Confirm that Games shows PSX DVR ISO MANAGER (no Corrupted Data), XMB starts the 1.2 HDD Manager, and two games start including Metal Slug in ENGLISH.
8. Confirm that the preserved or newly copied `PP.DVRISO/V12_NET.CNF` uses valid static network settings. Remove **only selected backed-up covers** from `PP.DVRISO/ART`; then run HTTPS download. Confirm batch limit 50, expected game IDs, PNG display and return to Manager. Confirm a second pass skips present artwork.
9. Re-run installer X and TRIANGLE: matching files should be kept, and PPAA recognized as already installed. Confirm user configs and covers remain unchanged.

**If a warning, unexpected partition identity, CRC, PPAA or I/O error appears, stop, record the exact code/screen and do not format or delete anything.** A full HDD backup is still essential; this is not a transactional filesystem upgrade.

## Release policy

The automatic installer zip and corresponding manual HDD zip are **CI test artifacts only**. No public Release or tag may be created without explicit permission after hardware tests.
