<img width="1774" height="887" alt="Metaliczny banner PDIM w pomarańczowych tonach" src="https://github.com/user-attachments/assets/9ffed54c-e8ea-46e2-9af0-65e351b40cb3" />

# PSX DVR ISO MANAGER

[Polski / Polish](ReadmePL.md)

**Version 1.2 — HDD + USB, native HTTPS artwork downloading.**

PSX DVR ISO Manager (PDIM) browses and launches PlayStation 2 ISO games stored on the DVR filesystem of a **Sony PSX DESR** console. HDD is the primary version; USB is also supported. **HDD 1.2 and its XMB installer passed functional tests on a real DESR-7000.** The USB HTTPS branch also passed its cover-download/game tests.

**Official Release 1.2:** [Download and release notes](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2). Select one of the three packages below.

| Package | Best for | What it contains |
| --- | --- | --- |
| [HDD AUTO 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-AUTO-OFFICIAL.zip) | Automatic HDD + XMB installation | Installer ELF, runtime, XMB files and network config |
| [HDD MANUAL 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-MANUAL-OFFICIAL.zip) | Manual HDD install via wLE-R3Z | Runtime, XMB header assets and manual instructions |
| [USB COMPLETE 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-USB-COMPLETE-OFFICIAL.zip) | USB execution | Complete USB runtime and HTTPS cover support |

**Previous documentation:** [English README for 1.0](docs/archive/README_1.0.md) · [Polish README for 1.0](docs/archive/README_PL_1.0.md).

**Technical background:** [HTTPS 1.2 — TXRyan, wolfSSL and NULD entropy](docs/HTTPS_1.2_TECHNICAL_EN.md).

**Safety:** Before modifying the HDD, make a **full, verifiably restorable sector-level backup**. Never format an existing Sony system/DVR partition. Game ISOs remain in the Sony DVR area; PDIM does not overwrite them.

---

## 1. Requirements

- **Sony PSX DESR** with a working DVR HDD (development and installation tested on **DESR-7000**).
- **wLaunchELF-R3Z 4.80** (or a compatible DESR-aware build) to launch the installer, browse HDD/PFS and/or launch USB ELF files.
- A **FAT32 USB drive** for installer files, manual copying, or running the USB edition. UDPFS can also help transfer files for manual HDD installation.
- PS2 game ISO files on the native Sony DVR filesystem, separate from application storage.
- For **HDD 1.2**, use **`hdd0:PP.DVRISO`**. This may be created by the automatic installer when absent, or prepared manually using wLE-R3Z. **Do not use `PP.DVRISO12`** — it was only a development test partition.
- For **USB 1.2**, no new application partition is needed: use `mass0:/DESR_DVR/`.
- A working Ethernet connection and a network configuration file are required only for cover downloads. Use `V12_NET.CNF` for **native HTTPS**, and `NET_PROBE.CNF` for the **Windows PC / LAN HTTP** method.

**Two HDD installation choices:** choose the automatic **HDD/XMB Installer** or the **manual HDD** package. Both support running the same 1.2 HDD Manager from XMB after setup; do not install both over each other unnecessarily.

## 2. HDD File Layout

The HDD edition stores the application, Neutrino, configuration and cover art on **`PP.DVRISO`**, not alongside Sony's game ISO data.

Copy the **contents** of `PP.DVRISO-FILES/` to the partition root if installing manually. With the automatic installer, files are copied and verified for you.

```text
hdd0:PP.DVRISO/
│
├── DVR_ISO_MANAGER_HDD.ELF       # PDIM 1.2 HDD with embedded HTTPS worker
│
├── EXECUTE.ELF                   # Signed XMB KELF launcher
├── EXECUTE.CNF                   # XMB -> Manager path
├── BOOT.ELF                      # OSDMenu / game launcher
├── BOOT.CNF                      # Generated / updated when launching a game
│
├── neutrino_hdd.elf             # SYSTEM language
├── neutrino_hdd_lang.elf        # Forced ENGLISH
│
├── N4E.CA                       # CA bundle for HTTPS certificate checks
├── V12_NET.CNF                  # Native HTTPS network settings
├── NET_PROBE.CNF                # Windows LAN HTTP / PC ART Server settings
│
├── ART/                         # Saved game covers
│   ├── SLES_XXXXX_COV.png
│   └── SLUS_XXXXX_COV.png
├── COVERS.IDX                   # Generated cover index
├── COVER_SCAN.LOG               # Generated scan report
│
├── config/                      # Neutrino TOML files
│   ├── system.toml
│   ├── bsd-dvr.toml
│   ├── osdlang.toml
│   └── ...                      # Other bundled configuration
│
├── modules/                     # DVR / Neutrino IRX + EE core
│   ├── ee_core.elf
│   ├── ee_core_lang.elf
│   ├── dvrfile_fhi.irx
│   ├── dvrdrv.irx
│   ├── dvrfile.irx
│   └── ...                      # Remaining packaged modules
│
└── res/                         # XMB art and metadata
    ├── info.sys
    ├── jkt_001.png
    └── jkt_002.png
```

*Simplified tree: keep all other files provided in `config/` and `modules/`. Newly created `BOOT.CNF`, `COVERS.IDX`, and `COVER_SCAN.LOG` may be absent immediately after a fresh install.*

**`V12_NET.CNF`** contains `IP`, `NETMASK`, `GATEWAY` and `DNS`. The automatic installer copies the bundled configuration **only when it is missing**, validating and verifying it first. It never overwrites a user's existing configuration. The sample address `192.168.0.240` is **not** a universal default: change it if your router/subnet differs or that IP is already in use.

## 3. Where to store game ISOs

Game images are **not** stored on `PP.DVRISO`.

The Manager uses the original Sony PSX DVR filesystem:

```text
dvr_pfs0:/_xcontent/ISO/
│
├── ICO.iso
├── MGS 2.ISO
├── REZ.iso
├── Metal Slug Anthology.iso
└── ...
```

The `PP.DVRISO` partition is reserved for the application, its configuration and game artwork.

**The Manager reads game ISOs from the DVR partition without modifying their contents.**

---

## 4. Install PDIM

Choose **one** method below. The automatic HDD installer and the manual HDD procedure target `PP.DVRISO`; the USB method does not install an HDD application partition.

<details>
<summary><strong>Install PDIM 1.2 with the HDD/XMB Installer</strong></summary>

**Package:** `PSX-DVR-ISO-Manager-v1.2.0-HDD-AUTO-OFFICIAL.zip`

1. Back up the entire HDD at sector level and check that the backup can be restored. Do not format an existing `PP.DVRISO`.
2. Extract the ZIP to a FAT32 drive, keeping the folder `PDIM_HDD_V12_INSTALLER/` directly under `mass0:/`.
3. In **wLaunchELF-R3Z 4.80**, launch `mass0:/PDIM_HDD_V12_INSTALLER/PDIM_HDD_V12_INSTALLER.ELF`.
4. Confirm the displayed destination: **`PP.DVRISO`**. Use **SELECT** to inspect XMB/PPAA without writing.
<img width="4096" height="3072" alt="IMG_20261118_232754515_HDR" src="https://github.com/user-attachments/assets/af9510f7-4bc1-4821-8c24-42d34074416f" />

5. Press **X — Install Core**. Confirm with **X**, then **hold L1 + R1 and press X**. The installer creates a missing 256 MiB PFS partition or reuses an existing one **without reformatting**. Expect **79/79 Core files** verified.
<img width="4096" height="3072" alt="IMG_20261118_232811531_HDR" src="https://github.com/user-attachments/assets/b2e2176d-2199-4240-99bc-4e6d5d227415" />
<img width="4096" height="3072" alt="IMG_20261118_232850922_HDR" src="https://github.com/user-attachments/assets/b100d05a-9b21-4598-9a33-ed4c111dae2d" />

6. If the HDD does not already contain `V12_NET.CNF`, the installer copies a validated file from USB. Otherwise it displays **KEEP**.
7. Press **TRIANGLE — Install XMB**, with the same two-step confirmation. It copies and CRC-verifies `EXECUTE.ELF`, `EXECUTE.CNF`, `res/info.sys`, `res/jkt_001.png`, `res/jkt_002.png` **before** registering PPAA. Expect **84/84 VERIFIED** and a valid header.
<img width="4096" height="3072" alt="IMG_20261118_232906042_HDR" src="https://github.com/user-attachments/assets/69a360ca-d812-404e-8fa2-18b41d822d69" />
<img width="4096" height="3072" alt="IMG_20261118_232943225_HDR" src="https://github.com/user-attachments/assets/c5b1094d-fdd8-4656-81c5-227f20fe93eb" />

8. Fully power off, then boot Sony XMB → **Games** → **PSX DVR ISO MANAGER**. The Manager should show **1.2 HDD**.
<img width="4096" height="3072" alt="IMG_20261118_075241288_HDR" src="https://github.com/user-attachments/assets/695d98d3-6582-4d6b-a944-168be6e41641" />
<img width="4096" height="3072" alt="IMG_20261118_075338261_HDR" src="https://github.com/user-attachments/assets/254dbb50-5db9-4e3e-927b-83ea644a9951" />


**Repeat installation was hardware-tested:** Core reported `COPIED 0 / KEPT 79 / VERIFIED 79`; the XMB stage reported five `KEEP`, `VERIFIED 84`, and `ALREADY VERIFIED`.

**Do not** delete, resize or format `PP.DVRISO` if an error occurs. Consult the [installer documentation](pdim-hdd-v1.2-installer/README.md).

</details>

<details>
<summary><strong>Install PDIM 1.2 manually on the HDD</strong></summary>

**Package:** `PSX-DVR-ISO-Manager-v1.2.0-HDD-MANUAL-OFFICIAL.zip`

1. Make a complete, restorable sector backup of the HDD. Back up the current Manager ELF, `BOOT.CNF`, `V12_NET.CNF`, `ART/` and other personal files.
2. Extract the manual ZIP to FAT32 USB. Preserve the folders `PP.DVRISO-FILES/` and `__Headers/PP.DVRISO/` as provided.
3. Open **wLaunchELF-R3Z 4.80 → MISC → HDDManager**. If `PP.DVRISO` already exists, **reuse it**. If missing, deliberately create a new **256 MiB PFS partition** named exactly `PP.DVRISO`.
4. Copy the **contents** of `PP.DVRISO-FILES/` to the root of that partition, preserving all paths. Do not copy the outer `PP.DVRISO-FILES` directory onto the HDD. Back up any files before replacing them.
5. Copy the top-level `V12_NET.CNF` only if there is no existing `PP.DVRISO/V12_NET.CNF`; edit the IP values if necessary.
<img width="4096" height="3072" alt="IMG_20261118_075319851_HDR" src="https://github.com/user-attachments/assets/4d164d7d-90d3-486a-b20e-0caa3acf356a" />
6. Optionally test `DVR_ISO_MANAGER_HDD.ELF` from the HDD/PFS file browser first.
7. For a **new XMB entry**, with `__Headers/PP.DVRISO/` in the USB root, select **HDDManager → PP.DVRISO → R1 → Inject Header → This partition only**. All five XMB files must already be present on PFS.
8. Restart and launch the Manager from **XMB → Games**.

If the existing HDD 1.0 XMB entry already works, **do not reinject it blindly**. A working launcher can continue to start the 1.2 Manager after the new runtime files are installed.

More details: [manual installation guide](hdd-v1.2/INSTALL_MANUAL.md).

</details>

<details>
<summary><strong>Run PDIM 1.2 from a USB drive</strong></summary>

**Package:** `PSX-DVR-ISO-Manager-v1.2.0-USB-COMPLETE-OFFICIAL.zip`

1. Format a USB drive as **FAT32**.
2. Extract the ZIP and copy its `DESR_DVR/` folder **directly to the USB root**.
3. Run **wLaunchELF-R3Z** and execute `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`.
4. Leave the USB drive connected while using the Manager; games themselves are still read from the original Sony DVR filesystem.
5. For HTTPS covers, create `mass0:/DESR_DVR/V12_NET.CNF` using the included `V12_NET.CNF.example` and your actual Ethernet settings. For PC LAN downloads, keep using `DESR_DVR/NET_PROBE.CNF`.

```text
mass0:/DESR_DVR/
├── DVR_ISO_MANAGER.ELF         # USB 1.2 Manager + embedded HTTPS
├── BOOT.ELF
├── BOOT.CNF
├── neutrino.elf
├── neutrino_lang.elf
├── N4E.CA
├── V12_NET.CNF                 # User-created HTTPS settings
├── V12_NET.CNF.example
├── NET_PROBE.CNF
├── ART/
├── config/
└── modules/
```

Keep the complete contents of `DESR_DVR/` — do not remove bundled modules or configuration files. See [USB documentation](docs/usb12-package.md).

</details>

## 5. How to use PDIM

Browse up to **128** game ISO entries (source-code limit), displaying **12 games per page**. The number of games scanned is not the number of individually boot-tested titles.

| Controller | Function |
| --- | --- |
| **UP / DOWN** | Select previous / next game |
| **LEFT / RIGHT** | Move between pages of 12 games |
| **X** (game list) | Launch the highlighted game through OSDMenu / Neutrino |
| **TRIANGLE** (game list) | Scan ISO games, detect IDs and refresh the cover index |
| **X** (cover download prompt) | Download covers with the Windows **PC ART Server** (LAN HTTP) |
| **TRIANGLE** (cover download prompt) | Download missing covers **directly over HTTPS** on the console (up to 50 per run) |
| **CIRCLE (O)** (download prompt) | Cancel / skip downloading |
| **X / CIRCLE (O)** (completed HTTPS result) | Dismiss the summary and return to the game list |
| **SQUARE** (game list) | Write the selected game's test configuration to `DVR_TEST.CNF` |
| **SELECT** | Toggle `SYSTEM` / `ENGLISH` |
| **START** | Exit PDIM |

**Screenshots (real DESR-7000 XMB examples):**

<img width="4096" height="3072" alt="PDIM application entry in the Sony PSX XMB" src="https://github.com/user-attachments/assets/3ed2f6ea-2c6d-4011-bd5c-3612ea6c18ce" />

<img width="4096" height="3072" alt="IMG_20261118_231052225_HDR" src="https://github.com/user-attachments/assets/6a092292-7000-4da6-9a16-70da55ba7a7f" />


<details>
<summary><strong>Download covers using Windows PC ART Server (LAN HTTP)</strong></summary>

The companion **[PSX DVR ART Server v0.8](pc-art-server/README.md)** runs on Windows. It downloads cover images over HTTPS on the PC, then serves them to PDIM over local **HTTP**.

1. Download `PSX-DVR-ART-Server-v0.8.exe` from the [existing release](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2) and run it on Windows.
2. Enter the **PC IPv4 Address**, **Console IPv4 Address** and **HTTP Port**; choose **Start HTTP Server**.
3. Configure `NET_PROBE.CNF` on the console's storage (`PP.DVRISO` for HDD, `DESR_DVR/` for USB). Example:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

   **IP** is the PSX's unused static address. **SERVER_IP** is the PC's address, **not** the PSX address. Find your network values with `ipconfig /all` on Windows.
4. In the PDIM game list, press **TRIANGLE** to scan and select **X — PC APP (LAN HTTP)** when download options appear.
<img width="4096" height="2710" alt="PSX DVR ISO Manager game list" src="https://github.com/user-attachments/assets/4e095df8-3283-43eb-ac6a-e98bd06d5f8f" />

5. Missing covers are retrieved by the Windows server, checked, saved to the application ART directory, and indexed. Previously present artwork is preserved.

Do not forward the local HTTP service's port to the internet. The PC server's cache location is `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`.

**Sources:** OPL Manager archive (preferred) and the CosmicScale PSBBN Art Database (fallback).

<img width="756" height="563" alt="Windows ART Server interface" src="https://github.com/user-attachments/assets/ff72eb3a-a41e-42b3-995e-bfb14f32d7a0" />

</details>

<details>
<summary><strong>Download covers directly via HTTPS on the console</strong></summary>

**New in PDIM 1.2:** the PSX DESR downloads missing covers directly, without a PC ART server.

1. Connect the console to Ethernet and configure `V12_NET.CNF`:
   - **HDD:** `PP.DVRISO/V12_NET.CNF` (automatic installer only copies if missing).
   - **USB:** `mass0:/DESR_DVR/V12_NET.CNF` (create from the included `.example`).
2. Ensure the certificate bundle `N4E.CA` is present alongside the Manager.
3. Start PDIM, press **TRIANGLE** to scan ISO games and select **TRIANGLE — SERVER (HTTPS, UP TO 50)**.
<img width="4096" height="3072" alt="IMG_20261118_075348419_HDR" src="https://github.com/user-attachments/assets/393df748-2ac8-44fb-bcc7-04bb245cd819" />


4. The worker obtains **cryptographically signed time** using **[TXRyan Roughtime](https://time.txryan.com/)**, then **[wolfSSL](https://www.wolfssl.com/)** verifies the HTTPS server's certificate, hostname and time validity.
5. Missing images are downloaded from the pinned OPL Manager source (with CosmicScale/PSBBN fallback), checked and normalized, and written without overwriting existing covers.
<img width="4096" height="3072" alt="IMG_20261118_075435773_HDR" src="https://github.com/user-attachments/assets/877033ea-138f-4762-b5a9-87b0e8a2878a" />
6. At completion, read **DOWNLOADED / SKIPPED**, press **X or O**, and continue in the game list. Rescan if your library contains more than 50 missing covers.
<img width="4096" height="3072" alt="IMG_20261118_075543062_HDR" src="https://github.com/user-attachments/assets/59a2a2f4-17e0-4c61-abf3-caaa4cc0e06f" />
<img width="4096" height="3072" alt="IMG_20261118_075600051_HDR" src="https://github.com/user-attachments/assets/21a337b4-c230-487b-997a-1bc7a29ec83b" />



For the protocol, pinned key, entropy limitations and implementation details, see the [HTTPS 1.2 technical guide](docs/HTTPS_1.2_TECHNICAL_EN.md).

**Why network time?** The PSX DESR's real-time clock may be flat or inaccurate; TXRyan supplies signed time data used for certificate-validity checks. TXRyan is a public **time service**, not a bundled binary.

**Entropy origin:** our experimental method samples the PS2's **COP0 Count** register, following the approach used by **[Daniel's NULD (formerly ORBIT)](https://github.com/danielnuld/nuld)**, then uses Monocypher/BLAKE2b to derive request-specific values. Derivation creates distinct values but does **not prove or increase the original entropy**. It is not a certified secure RNG.

</details>

## Building

This repository includes the CMake/PS2SDK build, integrated ISO scanning, LAN and PNG tests, and the PyInstaller configuration for the Windows executable. The former standalone Probe has been retired. See [dvr-iso-manager](dvr-iso-manager/README.md) and [pc-art-server](pc-art-server/README.md) for more information.

## Credits

- [Neutrino / ps2max32](https://github.com/ps2max32/neutrino) and [pcm720's DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228) — PS2 game-launch integration and DESR modules.
- **Berion** — technical assistance.
- [OPL Manager art database](https://github.com/Luden02/psx-ps2-opl-art-database) — primary artwork source; [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) — fallback source.
- [Daniel's NULD / ORBIT](https://github.com/danielnuld/nuld) — experimental **COP0 Count CPU timing/entropy technique**, adapted for PDIM 1.2; [NULD's original `net.c`](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c).
- [wolfSSL](https://github.com/wolfSSL/wolfssl) — TLS/HTTPS client, pinned CA/hostname/certificate validity checks.
- [TXRyan / time.txryan.com](https://time.txryan.com/) — public **signed Roughtime** source for HTTPS certificate date verification.
- [Monocypher](https://monocypher.org/) — cryptographic primitives (Ed25519/BLAKE2b) used in signed-time and entropy derivation.
- [PS2SDK](https://github.com/ps2dev/ps2sdk), [wLaunchELF-R3Z](https://github.com/NathanNeurotic/wLaunchELF_R3Z) and [RepairBox PSX XMB installer](https://github.com/DanielFergisz/RepairBox-PSX-XMB-Apps-Installer) — development tools, PFS/APA and installation references.

**Licenses and distribution:** Third-party components retain their own license terms. In particular the pinned wolfSSL build uses GPLv3-or-later / commercial dual licensing; other components use different licenses. Licensing notices and source/provenance information are supplied with the release packages. The build-specific integration and rights for pinned binaries must be reviewed independently; attribution alone does not replace license compliance.

**HDD 1.2 hardware status:** tested on **Sony PSX DESR-7000**. Other PSX variants and HDD layouts require independent testing.
