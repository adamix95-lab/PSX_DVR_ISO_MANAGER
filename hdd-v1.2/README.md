# PSX DVR ISO MANAGER 1.2 HDD — pre-release integration

**Status: testing only. There is NO authorized 1.2 GitHub Release.**

## Goal

Bring the hardware-confirmed HDD 1.0 (NETFIX, FIX95 Neutrino, SYSTEM/ENGLISH, PFS covers and XMB) together with the USB 1.2 R4 HTTPS cover downloader. The final target is **hdd0:PP.DVRISO** for all application files, CNF, ART and XMB execution. `PP.DVRISO12` was only an earlier isolated test target. Original game ISO files remain on the Sony DVR filesystem (`dvr_pfs0:/ISO`) and are read, never modified, by the Manager.

## Proven baselines and known problems

- HDD 1.0: game launch, 40+4 cover downloads in NETFIX, dual language and XMB were hardware tested.
- USB 1.2 R4: 45/45 HTTPS covers, return to Manager, game launch and 50-cover cap were verified.
- HDD 1.2 on test partition: 45 covers downloaded over HTTPS on DESR-7000; Metal Slug launched in ENGLISH.
- HDD12 R3 installer: XMB automatic install had a hardware PASS, but its X phase contained EXECUTE.* and its TRIANGLE phase copied res/ assets *after* PPAA.
- R1 installer loaded fileXio in an invalid IOP module set (-200); R2 fixed this with clean IOP reset. R2 XMB registration had failed (-31), R3 adopted wLE-R3Z sector-based PPAA writes.
- The early installer warning screen had wrapped lines and an incorrect background color.

## 1.2 production-target improvements (not yet hardware verified)

- Explicit build target `DESR_HDD_V12_PRODUCTION=ON`: points Manager and embedded HTTPS worker at `PP.DVRISO`; does not change USB or HDD12 test build.
- PFS1 contains `DVR_ISO_MANAGER_HDD.ELF`, `N4E.CA`, `V12_NET.CNF`, `ART/`, and existing HDD runtime files.
- Native wolfSSL HTTPS + Roughtime, pinned OPL Manager artwork database and CosmicScale fallback, PNG normalizer and 50-cover batch.
- New dedicated installer in `pdim-hdd-v1.2-installer/`: X installs 79 core files; TRIANGLE installs/verifies 5 XMB files **before** PPAA header commit.
- Distinct hardware tests for installation, XMB registration, game boot, English, HTTPS cover retrieval and idempotent re-run.

## Pre-release deliverables

1. Complete HDD 1.2 automated installer: installer ELF, 84-file manifest, XMB header assets and documentation.
2. Complete HDD 1.2 manual package: same runtime assets and header files to install using wLE-R3Z.
3. USB 1.2 package: prepared separately from the proven USB branch.

**Do not create a tag, GitHub Release, or publish 1.2 until hardware results are reviewed and the owner explicitly approves release.** Test packages are CI artifacts, not public releases.

## Safety

An existing `PP.DVRISO` may contain HDD 1.0. The installer never reformats, deletes or resizes it. It protects BOOT*.CNF, existing `V12_NET.CNF`, `ART/` and indexes. If network configuration is missing, the installer can copy the supplied valid `V12_NET.CNF` from USB once, with validation and CRC readback; it never overwrites existing settings. Only an existing Manager ELF may be replaced, with the old one retained as `.PDIMBAK`. Back up the complete HDD at sector level before any write; an interrupted rename or APA operation is not transactionally atomic. See the installer README for the exact test sequence.
