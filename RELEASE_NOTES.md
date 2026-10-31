# PSX DVR ISO Manager v0.8

A complete Sony PSX DESR homebrew launcher for PS2 ISO games stored on the DVR partition, featuring a paged game library, cover previews and automatic cover downloads through a Windows PC.

## Features

- **12 games per page**, with Left/Right page navigation and Up/Down game selection.
- **Game ID detection** from ISO contents (including the executable-ID fallback), plus a color PNG cover preview.
- Press **Triangle** to rescan the read-only DVR ISO library and update the USB artwork index and scan report.
- When covers are missing, press **X** on the scan result screen to download them from a local HTTP server, or **Circle (O)** to skip. You can rescan and retry after cancelling, without restarting the Manager.
- The **Windows ART Server** fetches images via HTTPS from the **OPL Manager Archive on Archive.org** (primary source) and **CosmicScale PSBBN Art Database on GitHub** (fallback). The PC caches images and serves normalized PNG covers to the console over a private LAN.
- New PNG covers are validated before being saved to USB and automatically reindexed. Existing covers are not overwritten.
- **Quickboot through OSDMenu / Neutrino** remains available, including SYSTEM/ENGLISH language selection.

## Hardware testing

Validated on a **Sony PSX DESR-7000** with **40 ISO games**: starting without cover images or index files, the Manager and Windows ART Server downloaded **40/40 covers**, displayed every cover and successfully launched **REZ in English**. The cancel → rescan → download workflow and game launch after syncing were also confirmed.

The PSBBN fallback has automated tests, but its use during the 40-game hardware test has not been separately confirmed from the Windows server logs.

## Downloads

### Complete USB package — recommended

**`PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE.zip`** — the author's approved **clean USB package containing 81 files**, including the v0.8 Manager ELF, OSDMenu/Neutrino executables and modules, DVR configuration files, an empty `ART/` directory, a sample `NET_PROBE.CNF` and quick-start instructions.

**Installation:** Extract the ZIP and copy the *inner* `DESR_DVR/` directory from `PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE/DESR_DVR/` to the root of the USB drive. The Manager should be located at:

```text
mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF
```

The clean package excludes obsolete diagnostic/test files and example graphics. ISO games, cover images and the Windows EXE are not included in the USB directory.

### Individual files

- **`DVR_ISO_MANAGER.ELF`** — standalone v0.8 Manager binary for users who already have the necessary OSDMenu/Neutrino USB files.
- **`PSX-DVR-ART-Server-v0.8.exe`** — Windows companion application with an English GUI. It does not require a separate Python installation.

Configure `NET_PROBE.CNF` for your private LAN. On Windows, run **`ipconfig /all`** to find the PC's IPv4 address, subnet mask and default gateway; assign the console an unused static IP on the same subnet. Match the configured HTTP port (default 8000).

**Before upgrading an existing installation**, back up the complete `DESR_DVR/` folder and preserve your personal `BOOT.CNF`, `NET_PROBE.CNF`, `ART/`, `COVERS.IDX` and `COVER_SCAN.LOG`.

## Documentation

- [English installation and controls](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/main/README.md)
- [Polish installation and controls](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/main/README_PL.md)
- [Windows ART Server documentation](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/main/pc-art-server/README.md)

The standalone network probe was an early development tool; its ART SYNC functionality is now integrated into the Manager. No additional network tester executable is required.

**Safety:** The Manager accesses game ISOs on the DVR partition in **read-only mode**. Cover images, index files and launch configuration are stored on USB. Do not expose the Windows HTTP server to the public internet.
