# PDIM 1.2 HDD Installer — HDD + XMB

**Hardware status: PASS — Sony PSX DESR-7000 (November 2026).** Core installation, XMB/PPAA registration, repeat install, read-only diagnostics and network-config handling were confirmed on hardware.

**Official Release 1.2:** [HDD AUTO Installer](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-AUTO-OFFICIAL.zip). Functionally tested on Sony PSX DESR-7000.

[Polski](README_PL.md) · [HDD 1.2 HTTPS overview](../hdd-v1.2/README.md) · [Manual installer alternative](../hdd-v1.2/INSTALL_MANUAL.md)

## Purpose and safety

This installer prepares **`hdd0:PP.DVRISO`** for **PDIM 1.2 HDD** and registers its entry in the Sony PSX DESR XMB. `PP.DVRISO12` was used only during development and is **not a production target**.

When `PP.DVRISO` already exists, the installer **never formats, deletes or resizes it**. If absent, it can create a 256 MiB PFS partition after explicit confirmation. Partition creation modifies global APA metadata; a failure may affect other partitions. A **verified, restorable full-sector backup of the entire HDD** is required before any write.

The installer's initial preflight and its SQUARE/SELECT diagnostics are read-only. HDD writes require two confirmations: **X**, then **hold L1 + R1 and press X**. O cancels.

## Button map

| Button | Operation | Result |
| --- | --- | --- |
| **X — Install Core** | Copy and CRC-verify **79** application files | Manager 1.2 HTTPS, FIX95 Neutrino, OSDMenu, modules, configs and CA |
| **TRIANGLE — Install XMB** | Copy/verify **5** XMB PFS files and then register PPAA | Boot entry is enabled only after **84/84** file checks |
| **SQUARE** | Read-only verification of Core | **79** manifest files |
| **SELECT** | Read-only PPAA header inspection | Checks target partition's XMB sectors |
| **O** | Exit or cancel | No write on cancellation |

### Why XMB is a separate second stage

The proven R3 installer copied `EXECUTE.ELF` and `EXECUTE.CNF` during Core and copied three `res/` files *after* PPAA registration. HDD 1.2 changes that order:

1. **X installs 79 Core files**, excluding all five XMB-specific PFS files.
2. **TRIANGLE first copies and verifies:** `EXECUTE.ELF`, `EXECUTE.CNF`, `res/info.sys`, `res/jkt_001.png`, `res/jkt_002.png`.
3. Each destination file is checked against the manifest by size/CRC; all **84 files** are re-read before PPAA is updated.
4. The wLE-R3Z-derived sector writer changes the necessary PPAA sectors at **partition start +8, +9, +10**, writing descriptor/header data last. A failed USB copy **does not trigger PPAA registration**.

A matching legacy HDD 1.0 header can be upgraded from `title1=HDD 1.0` to `title1=HDD 1.2`; unknown, partially written or unrelated headers are rejected. The trusted HDD 1.0 signed `EXECUTE.ELF`/KELF is reused unchanged.

## Safe upgrade and user data

- Existing **`BOOT*.CNF`, `V12_NET.CNF`, `ART/`, indexes and game ISOs** are excluded from immutable file copying and preserved.
- A differing old Manager is staged in `.PDIMNEW`, synchronized, checked and swapped while retaining the old executable as `DVR_ISO_MANAGER_HDD.ELF.PDIMBAK`. Subsequent backups use `.PDIMBAK2`–`.PDIMBAK9` without overwriting earlier backups.
- Other unexpected differing destination files are rejected rather than silently overwritten. On failure, check `.PDIMNEW` and the backups manually; **do not format or delete the partition**.
- The new **`V12_NET.CNF` import is optional and separate from the 79 Core files**. The installer keeps any existing HDD configuration (`KEPT`). If missing, it validates the file at `mass0:/PDIM_HDD_V12_INSTALLER/V12_NET.CNF`, stages it, verifies CRC/readback and copies it (`COPIED`). If USB config is missing or invalid, Core installation can finish but HTTPS needs manual network setup.

The supplied configuration is:

~~~ini
IP=192.168.0.240
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
DNS=1.1.1.1
~~~

These are contributor-specific values, **not universal LAN defaults**. Edit the USB file for your network; never allow two devices to use the same static IP. `V12_NET.CNF` is deliberately **outside** the 84-entry fixed manifest.

## Files on a FAT32 USB stick

Extract the official HDD AUTO ZIP so the structure includes:

~~~text
mass0:/
└── PDIM_HDD_V12_INSTALLER/
    ├── PDIM_HDD_V12_INSTALLER.ELF
    ├── MANIFEST.TXT
    ├── V12_NET.CNF
    ├── V12_NET.CNF.example
    ├── FILES/                (84 payload files: 79 Core + 5 XMB)
    └── XMB/
        ├── system.cnf
        ├── icon.sys
        ├── info.sys
        ├── jkt_001.png
        └── jkt_002.png
~~~

The **MANUAL** ZIP instead supplies `PP.DVRISO-FILES/`, `__Headers/PP.DVRISO/` and a top-level network config; it must be installed with wLE-R3Z following the [manual guide](../hdd-v1.2/INSTALL_MANUAL.md). Manual file copying does not provide the automatic installer's guarded backups and full CRC checks.

## Installation and test procedure

1. Prepare a restorable full-sector HDD backup. Separately copy your existing Manager ELF, BOOT.CNF, V12_NET.CNF and ART covers.
2. Launch `mass0:/PDIM_HDD_V12_INSTALLER/PDIM_HDD_V12_INSTALLER.ELF` in **wLE-R3Z 4.80**. Verify the displayed target is **`PP.DVRISO`**. SELECT can inspect PPAA read-only.
3. Press **X**, confirm twice (X; L1+R1+X). Expect **79 verified Core files** and a separate network status of **COPIED** or **KEPT**. Existing PFS must not be formatted.
4. Launch `pfs1:/DVR_ISO_MANAGER_HDD.ELF` through wLE. Check the game list and SYSTEM/ENGLISH boot path before XMB registration.
5. Restart the installer and use **TRIANGLE**, with both confirmations. Expect five XMB PFS files verified **before** PPAA, followed by **84/84** and PPAA PASS.
6. Power-cycle the console. Check XMB Games entry, no **Corrupted Data**, successful Manager launch and games in both language modes.
7. Test HTTPS with a few deliberately missing, backed-up artwork files. Confirm downloaded PNGs, cover display, return to Manager and rescan behavior.
8. Re-run **X** and **TRIANGLE**, then **SQUARE** and **SELECT**. Confirm preserved user configuration and covers.

On any unexpected partition identity, missing PFS, CRC/PPAA failure or interrupted copy: **STOP, record the exact error and inspect manually**. Do not use destructive automatic recovery.

## Hardware test results

| Test | Recorded result |
| --- | --- |
| Initial Core + XMB installation on `PP.DVRISO` | **PASS** |
| XMB boot, game launch, SYSTEM/ENGLISH and HTTPS | **PASS** |
| Repeat X | **COPIED 0 / KEPT 79 / VERIFIED 79** |
| Repeat TRIANGLE | **Five KEEP / COPY 0 / VERIFIED 84 / ALREADY VERIFIED** |
| SQUARE and SELECT read-only tests | **PASS / PASS** |
| Automatic `V12_NET.CNF` handling | **PASS**, confirmed on DESR-7000 |
| GitHub Actions build, host tests and both HDD ZIPs | **PASS** — [run 44253182195](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44253182195) |

**Functional hardware PASS does not prove crash-atomic writes or that a corrupted APA layout can be recovered automatically.**

## Historical R1 → R3 fixes

- **R1:** IOP `fileXio -200` dependency failure before any HDD write.
- **R2:** reset IOP and load proven modules; XMB PPAA registration still failed with `-31`.
- **R3:** read/write raw sector method derived from wLE-R3Z; hardware XMB registration PASS, but file-phase ordering was suboptimal.
- **1.2 HDD:** five XMB files moved to TRIANGLE *before* PPAA, cleaner English UI and red warning, safe repeat-install path, existing HDD 1.0 upgrade, and optional `V12_NET.CNF` import.

The **working R3 source and HDD 1.0 release** remain historical reference points. The HDD 1.2 installer is included in [official Release 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2).
