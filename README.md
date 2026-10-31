# PSX DVR ISO Manager v0.8

[Polski / Polish](README_PL.md)

**PSX DVR ISO Manager** lists and launches PS2 ISO games from the DVR partition of a Sony PSX DESR console. Version **0.8** adds a 12-game paged menu, cover previews and on-demand cover downloads via a companion Windows PC application.

**Hardware status:** tested on a Sony PSX DESR-7000 with 40 ISO games. A full 40/40 artwork download, automatic index refresh and launch of **REZ in English** were confirmed. Individual games and external artwork sources may still behave differently.

## Features

- Browse up to 128 ISO filenames from the **read-only** `dvr_pfs0:/ISO` directory; 12 titles per page.
- Display Game IDs and USB-hosted PNG covers, using OPL-style names such as `SLES_538.86_COV.png`.
- Manually rescan the ISO library with **Triangle**, generate/update `COVERS.IDX` and `COVER_SCAN.LOG`, and count missing covers.
- After a scan, optionally press **X** to download missing artwork from a PC on your local network; press **Circle (O)** to skip. Successfully downloaded covers are automatically reindexed.
- Launch games through the configured OSDMenu/Neutrino Quickboot path, with optional `SYSTEM` or `ENGLISH` language selection.

## Controller

| Button | Action |
| --- | --- |
| **Up / Down** | Select previous / next game |
| **Left / Right** | Switch between pages of 12 games |
| **X** | Launch selected game from the normal menu |
| **Triangle** | Rescan the DVR ISO library, identify Game IDs and refresh the USB cover index |
| **X / Circle** on the missing-artwork screen | Download missing covers / cancel and return |
| **Square** | Write a test configuration to `DVR_TEST.CNF` |
| **SELECT** | Switch between `SYSTEM` and `ENGLISH` |
| **START** | Exit Manager |

Page navigation does not wrap from the first page to the last. After cancelling a cover download, **Triangle** can offer the missing artwork again without restarting Manager.

## Installation

Copy the existing OSDMenu / Neutrino DVR support files and the Manager to **`mass0:/DESR_DVR/`** on USB. The Manager ELF can also be started through UDPFS, but its boot files and artwork cache remain on USB.

```text
mass0:/DESR_DVR/
├── DVR_ISO_MANAGER.ELF           # Manager v0.8
├── BOOT.ELF                      # OSDMenu launcher
├── BOOT.CNF                      # Game launch configuration
├── BOOT_RECOVERY.CNF             # Existing configuration backup
├── DVR_TEST.CNF                  # Optional test configuration
├── neutrino.elf                  # SYSTEM language
├── neutrino_lang.elf             # ENGLISH language
├── NET_PROBE.CNF                 # PC / console LAN settings
├── COVERS.IDX                    # Cover index, generated on Triangle
├── COVER_SCAN.LOG                # Scan report with missing Game IDs
├── ART/
│   └── SLES_538.86_COV.png      # Example Game ID cover
├── config/
│   └── osdlang.toml              # Language settings (other configs omitted)
└── modules/
    └── ee_core_lang.elf          # ENGLISH support (other modules omitted)
```

The listing is illustrative; keep all the other working DVR/Neutrino modules and config files. The game ISO files themselves must be on the DVR HDD under **`dvr_pfs0:/ISO/`** (mapped from the `__xcontents` partition). Manager does **not** write to the DVR partition, APA or ISO images.

## Downloading missing covers with the PC app

The included **PSX DVR ART Server v0.8** runs as a Windows `.exe` (Python source is also provided under [pc-art-server](pc-art-server/README_PL.md)). The PC handles HTTPS downloads and serves validated PNG files to the PSX using simple local HTTP.

1. Download **`PSX-DVR-ART-Server-v0.8.exe`** from the Windows build artifact under [GitHub Actions](../../actions) and open it on your Windows PC.
2. Enter the PC IP, console IP and HTTP port, then click **Start serwera HTTP**. The example below uses the LAN settings tested with DESR-7000.
3. Make sure the USB `NET_PROBE.CNF` matches those addresses:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

4. In Manager, press **Triangle** and wait for the scan. If covers are missing, select **X: DOWNLOAD** on the scan-result screen. Press **Circle** to skip instead.
5. The PC retrieves the corresponding `<GAMEID>_COV.png` via HTTPS, caches it, and returns a validated, resized PNG over LAN HTTP. The console writes **only new** images to `mass0:/DESR_DVR/ART` and refreshes the index automatically. Existing covers are not replaced.

The PC searches the **OPL Manager archive on Archive.org first** and uses **CosmicScale PSBBN Art Database on GitHub** as a fallback. PSBBN artwork may be square game art rather than a vertical box cover; it is proportionally fitted with black margins. Not every Game ID will have artwork in both sources. The PC keeps a local cache in `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`.

**Security:** Use a trusted private LAN, allow the Windows HTTP server only on that network, and do not forward its port on your router. The console reads from DVR and writes new artwork/index files **only to USB**. Back up `BOOT.CNF`, the existing Manager ELF and your art folder before replacing a working installation.

## Screenshot

An updated screenshot of the v0.8 MONO interface can be added here.

## Build and development

The repository includes PS2SDK/CMake build configuration for the Manager, standalone network diagnostics, automated tests and a Windows PyInstaller workflow for the PC application. See [dvr-iso-manager](dvr-iso-manager/README_PL.md) and [pc-art-server](pc-art-server/README_PL.md) for details.

## Known limitation and contributions

Launching the Manager directly from the console XMB using the internal HDD remains unresolved (previous path-resolution error `-19`). Running the ELF from USB or UDPFS with the required files on USB is the supported route.

Contributions, issue reports and pull requests are welcome, particularly from developers working with Sony PSX DESR, PS2SDK and DVR hardware.

## Credits

- [Neutrino](https://github.com/ps2max32/neutrino) and [pcm720's DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228)
- pcm720 — DVR support for Neutrino
- Berion — technical assistance
- [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) — fallback artwork source
- OPL Manager artwork archive — primary artwork source
