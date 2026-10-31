# PSX DVR ISO Manager

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

## Complete USB package (recommended)

**Download [PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE.zip](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/v0.8/PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE.zip).** This is the easiest way to install the Manager; you do **not** need to download v0.3 separately or search for Neutrino/OSDMenu modules.

1. Extract the ZIP file on your computer.
2. Open the extracted `PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE` directory and copy its **inner `DESR_DVR` folder** to the **root of your USB drive**. The result must be `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`; do not copy the outer wrapper directory to USB.
3. Launch `DVR_ISO_MANAGER.ELF` via wLE or UDPFS. Your ISO games must already be on the DVR partition at `dvr_pfs0:/ISO/`.
4. To download covers, run the **separate Windows ART Server `.exe`** from the same v0.8 release. Edit the included `DESR_DVR/NET_PROBE.CNF` to match your network as described below.

The user-approved **clean ZIP contains 81 files**, including all required **OSDMenu/Neutrino binaries, modules and TOML configurations from v0.3.0**, the official **v0.8 Manager ELF**, an empty `ART/` directory, a sample `NET_PROBE.CNF` and starter guides. Old test files (`DVR_TEST.CNF`, `info.sys`, `jkt_*.png`), the pre-existing `BOOT_RECOVERY.CNF` and the old checksum manifest are **not included**. The current Manager can create test/recovery files when needed. The v0.3 `BOOT.elf` has the filename **`BOOT.ELF`** in this ZIP, matching the current Manager's launch path; its binary contents are unchanged. Game ISOs, cover images and the Windows program are **not** bundled inside the USB folder.

**Upgrading an existing USB installation?** First make a complete backup of your current `DESR_DVR` folder. In particular, preserve your `BOOT.CNF`, `BOOT_RECOVERY.CNF`, `NET_PROBE.CNF`, `ART/`, `COVERS.IDX` and `COVER_SCAN.LOG` rather than replacing your working personal settings or covers with the clean-install samples.

## USB folder layout

The Manager ELF can also be started through UDPFS, but its boot files and artwork cache remain in `mass0:/DESR_DVR/` on USB.

```text
mass0:/DESR_DVR/
├── DVR_ISO_MANAGER.ELF           # Manager v0.8
├── START_HERE_PL.txt              # First-install guide (Polish)
├── START_HERE_EN.txt              # First-install guide (English)
├── BOOT.ELF                      # OSDMenu launcher
├── BOOT.CNF                      # Game launch configuration
├── neutrino.elf                  # SYSTEM language
├── neutrino_lang.elf             # ENGLISH language
├── NET_PROBE.CNF                 # PC / console LAN settings
├── ART/
│   └── SLES_538.86_COV.png      # Example Game ID cover
├── config/
│   └── osdlang.toml              # Language settings (other configs omitted)
└── modules/
    └── ee_core_lang.elf          # ENGLISH support (other modules omitted)
```

The listing is abbreviated; **the cleaned USB ZIP retains all required v0.3 modules and TOML configuration files**. `COVERS.IDX` and `COVER_SCAN.LOG` are generated on the first Triangle scan; `BOOT_RECOVERY.CNF` and `DVR_TEST.CNF` are created when needed. The game ISO files themselves must be on the DVR HDD under **`dvr_pfs0:/ISO/`** (mapped from the `__xcontents` partition). Manager does **not** write to the DVR partition, APA or ISO images.

## Downloading missing covers with the PC app

The included **PSX DVR ART Server v0.8** runs as a Windows `.exe` (Python source is also provided under [pc-art-server](pc-art-server/README_PL.md)). The PC handles HTTPS downloads and serves validated PNG files to the PSX using simple local HTTP.

1. Download **`PSX-DVR-ART-Server-v0.8.exe`** from the published assets under [v0.8 release](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/v0.8) and open it on your Windows PC.
2. Enter the **PC IPv4 Address**, **Console IPv4 Address** and **HTTP Port** in the Windows application, then click **Start HTTP Server**. The sample addresses below are from a tested DESR-7000 setup; use the addresses of your own network.
3. Make sure `mass0:/DESR_DVR/NET_PROBE.CNF` on the USB drive matches the settings:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

   **How to find your network settings:** Open **Command Prompt** or **PowerShell** on the PC, run `ipconfig /all` (with a **slash**, not `ipconfig -all`), and look at the adapter that connects the PC to the PSX's network.

   | Config setting | Where it comes from |
   | --- | --- |
   | `IP` | **Static IPv4 address manually assigned to the PSX DESR.** Pick an unused address on the same subnet as the PC; it is **not** the PC's IPv4 address and will not be listed by `ipconfig /all`. |
   | `NETMASK` | **Subnet Mask** reported by `ipconfig /all` for that adapter. |
   | `GATEWAY` | **Default Gateway**, typically your router, from `ipconfig /all`. |
   | `SERVER_IP` | **IPv4 Address of the Windows PC** running PSX DVR ART Server, from `ipconfig /all`. |
   | `SERVER_PORT` | **HTTP Port** configured in the Windows app (default `8000`). This value does **not** come from `ipconfig`. |

   **Important:** `IP` and `SERVER_IP` must be different. Avoid a console IP already assigned to another device. If the PC's IPv4 address changes, update `SERVER_IP` and the application's PC IPv4 Address accordingly.

4. In Manager, press **Triangle** and wait for the scan. If covers are missing, select **X: DOWNLOAD** on the scan-result screen. Press **Circle** to skip instead.
5. The PC retrieves the corresponding `<GAMEID>_COV.png` via HTTPS, caches it, and returns a validated, resized PNG over LAN HTTP. The console writes **only new** images to `mass0:/DESR_DVR/ART` and refreshes the index automatically. Existing covers are not replaced.

**Cover Sources:** **Primary — OPL Manager Archive (Archive.org)** provides PS2 box-cover images, usually vertical. **Fallback — CosmicScale PSBBN Art Database (GitHub)** provides game artwork, often square, and is queried **only if the OPL source fails or has no valid PNG**. Square artwork is fitted into the console's 140×200 RGB format with black margins, without stretching. If neither source has artwork for a Game ID, the PC reports it as missing. The PC keeps a local cache in `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`.

**Security:** Use a trusted private LAN, allow the Windows HTTP server only on that network, and do not forward its port on your router. The console reads from DVR and writes new artwork/index files **only to USB**. Back up `BOOT.CNF`, the existing Manager ELF and your art folder before replacing a working installation.

## Screenshot

<img width="4096" height="2452" alt="1000055888" src="https://github.com/user-attachments/assets/dd491fb4-30a7-4120-9857-029a78d36b99" />
<img width="4096" height="2864" alt="1000055889" src="https://github.com/user-attachments/assets/c5a5c06e-5f05-423e-844a-10007cee015c" />
<img width="4096" height="2710" alt="1000055890" src="https://github.com/user-attachments/assets/4e095df8-3283-43eb-ac6a-e98bd06d5f8f" />
<img width="4096" height="3072" alt="1000055891" src="https://github.com/user-attachments/assets/444d9328-27a6-46b2-9c41-ec29764f5023" />
<img width="4096" height="2939" alt="1000055892" src="https://github.com/user-attachments/assets/3fb9f935-9e6d-4acb-b610-2abc85b59300" />
<img width="756" height="563" alt="Zrzut ekranu 2026-11-01 000809" src="https://github.com/user-attachments/assets/ff72eb3a-a41e-42b3-995e-bfb14f32d7a0" />

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
