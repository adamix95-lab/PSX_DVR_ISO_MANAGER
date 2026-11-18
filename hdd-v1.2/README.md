# PSX DVR ISO MANAGER 1.2 HDD — Native HTTPS Covers

**Hardware status: PASS — Sony PSX DESR-7000 (November 2026).** Manager, HDD/XMB installation, game launch and HTTPS covers have been tested on real hardware.

**Distribution status: PRE-RELEASE.** No public GitHub Release or tag 1.2 has been authorized. The CI ZIPs are test artifacts, not a public release.

[Polski](README_PL.md) · [HDD installer](../pdim-hdd-v1.2-installer/README.md) · [Manual installation](INSTALL_MANUAL.md)

## Project goal

HDD 1.2 combines the hardware-tested **PDIM 1.0 HDD** game launcher (NETFIX, OSDMenu, Neutrino FIX95, SYSTEM/ENGLISH, PFS ART and XMB) with the **native HTTPS downloader from USB 1.2 R4**. The HTTPS option downloads covers directly without requiring a PC cover server.

**The final application target is `hdd0:PP.DVRISO`.** `PP.DVRISO12` was an isolated *development test* partition and is not used for final HDD 1.2 installation or execution.

| Component | Path |
| --- | --- |
| Application partition / PFS mount | `hdd0:PP.DVRISO` / `pfs1:/` |
| Manager ELF | `pfs1:/DVR_ISO_MANAGER_HDD.ELF` |
| HTTPS CA bundle | `pfs1:/N4E.CA` |
| HTTPS network settings | `pfs1:/V12_NET.CNF` |
| Artwork | `pfs1:/ART/` |
| SYSTEM / ENGLISH Neutrino | `pfs1:/neutrino_hdd.elf` / `pfs1:/neutrino_hdd_lang.elf` |
| Original DVR game ISOs | `dvr_pfs0:/ISO` — **read-only access** |

The Manager does not move or write game ISO files. Creating an APA partition can nevertheless change global disk metadata; read **Safety** before any installation.

## Features

- **Native HTTPS:** wolfSSL certificate/hostname verification, signed Roughtime network time, CA bundle and an embedded HTTPS download worker.
- **Automatic ISO/cover scan:** recognize PS2 Game IDs from original DVR ISOs, check existing HDD covers and request missing artwork only.
- **Artwork repositories:** pinned OPL Manager database with CosmicScale/PSBBN fallback; validate and normalize downloaded PNGs.
- **Up to 50 covers per batch** with progress summary, return to the Manager and cover-index refresh.
- **Safe artwork writes:** create `.PART` exclusively, verify written data, rename and synchronize PFS. Existing PNG/JPG files are not overwritten.
- **Game support retained:** HDD NETFIX, OSDMenu/BOOT.CNF handoff, Neutrino FIX95, SYSTEM/ENGLISH and cover previews.
- **Integrated XMB installer:** 79 Core files with X; five XMB files with TRIANGLE; final PPAA header commit happens only after complete file verification.
- **Network configuration:** copy a validated `V12_NET.CNF` from USB only when missing, preserve existing settings.

After scanning via TRIANGLE, select **SERVER (HTTPS, UP TO 50)** to fetch missing covers. The separate **PC APP (LAN HTTP)** choice remains available.

## Development and problem history

| Stage | Problem / solution | Verified outcome |
| --- | --- | --- |
| HDD 1.0 NETFIX | HDD network, ISO launch and covers | Games start; 40 + 4 covers downloaded |
| USB 1.2 R4 | Native TLS, signed time and 50-cover batch | 45/45 HTTPS covers in one session; return and game start |
| HDD 1.2 test on `PP.DVRISO12` | Transfer HTTPS from USB to writable HDD ART | 45 covers; Metal Slug launched in ENGLISH |
| HDD installer R1 | `fileXio` module error `-200` | Fixed by clean IOP reset and driver order |
| HDD installer R2 | XMB header error `-31` | Reworked to wLE-R3Z sector-based PPAA writer |
| HDD installer R3 | XMB hardware PASS, but `EXECUTE.*` copied during Core and `res/` copied after PPAA | Basis retained; install order corrected in HDD 1.2 |
| HDD 1.2 installer | UI wrapping/color, version upgrade, missing `V12_NET.CNF` | Fixed layout, file-first PPAA, preserved user data and optional config copy |

Stable **HDD 1.0** and the proven **R3 installer** remain unchanged as source/reference baselines.

## Hardware test report — target `PP.DVRISO`

The project owner completed these tests on the **Sony PSX DESR-7000**:

| Check | Result |
| --- | --- |
| Installer boots from FAT32 USB under wLE-R3Z 4.80 | **PASS** |
| Target partition detection and XMB inspection | **PASS** |
| X: install/verify 79 Core files | **PASS** |
| TRIANGLE: install five XMB files, verify 84/84 and register PPAA | **PASS** |
| Boot from XMB, start games, SYSTEM / ENGLISH | **PASS** |
| Native HTTPS cover downloads, cover display, return to Manager | **PASS** |
| Re-run X: `COPIED 0 / KEPT 79 / VERIFIED 79` | **PASS** |
| Re-run TRIANGLE: five `KEEP`, `COPY 0`, `VERIFIED 84`, `ALREADY VERIFIED` | **PASS** |
| SQUARE read-only Core verification, SELECT PPAA inspection | **PASS / PASS** |
| Automatic `V12_NET.CNF` handling | **PASS** (confirmed on hardware) |

These are functional hardware results, **not a claim of crash-proof APA transactions or exhaustive fault-injection testing**.

[Successful HDD 1.2 integration CI](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44253182195) builds the Manager, installer and both HDD test packages.

## Installation and future packaging

1. **HDD 1.2 AUTO:** installer ELF, 79 Core files, five XMB PFS files, raw PPAA header assets, supplied `V12_NET.CNF` and docs. [Installation details](../pdim-hdd-v1.2-installer/README.md).
2. **HDD 1.2 MANUAL:** matching files for manual PFS copy and wLE-R3Z XMB registration. [Manual guide](INSTALL_MANUAL.md).
3. **USB 1.2:** separate package planned for Release 1.2, based on hardware-tested USB R4.

**All three are release plans only. Do not publish a GitHub Release or tag until explicitly authorized.**

### Network configuration

The bundled `V12_NET.CNF` includes these contributed LAN values:

~~~ini
IP=192.168.0.240
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
DNS=1.1.1.1
~~~

They work on one local network but **are not universal router defaults**. Edit them as needed; do not reuse an occupied static IP. The installer never overwrites an existing HDD `V12_NET.CNF`. User network settings are intentionally **outside the 84-file immutable CRC manifest**.

## Safety and limitations

- Make and **verify a restorable, full-sector image of the entire HDD** before writing. A backup of APA metadata alone is not a replacement.
- Never format, delete or resize an existing `PP.DVRISO` for an update. The installer can create a missing 256 MiB PFS only after confirmation; creation modifies global APA metadata.
- Preserve `BOOT*.CNF`, existing `V12_NET.CNF`, artwork and indexes. The installer also backs up a previous Manager ELF during an upgrade; interrupted writes may require manual recovery.
- CPU-derived entropy and the TLS integration have been tested in this project but **are not an independently certified general-purpose secure TLS implementation**.
- The software is tailored to the PSX DESR and is not guaranteed to work on arbitrary PS2 hardware or HDD layouts.
- Third-party binary provenance and license notices should be reviewed before any public release.

**Release 1.2 is not yet published.**
