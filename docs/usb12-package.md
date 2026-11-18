# PSX DVR ISO MANAGER v1.2 USB — Complete package

**Official Release 1.2:** [download USB COMPLETE OFFICIAL](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-USB-COMPLETE-OFFICIAL.zip). This package was built from the `pdim_usb_v1.2` branch.

The Sony PSX DESR-7000 hardware tests confirmed direct HTTPS artwork downloads,
return to the game list, the English-language Neutrino option (Metal Slug),
and continued support for the previous LAN HTTP/Windows ART Server path.
The **renamed main ELF** in this package must be checked once on the console,
because the worker's return path now targets the unsuffixed filename.

## Contents

```text
DESR_DVR/
  DVR_ISO_MANAGER.ELF         <- PRIMARY application: PDIM 1.2 USB
  BOOT.ELF                    <- OSDMenu launcher
  BOOT.CNF                    <- initial/stock configuration (back it up)
  neutrino.elf                <- SYSTEM language
  neutrino_lang.elf           <- ENGLISH language
  N4E.CA                      <- Mozilla CA bundle for HTTPS
  V12_NET.CNF.example         <- edit/copy as V12_NET.CNF
  NET_PROBE.CNF               <- older PC LAN HTTP method
  ART/                        <- game cover artwork
  config/                     <- Neutrino/OSD settings
  modules/                    <- required PS2/PSX driver modules
README.md                     <- this guide
README_PL.md                  <- Polish guide
tools/PSX-DVR-ART-Server-v0.8.exe
licenses/
SOURCE_COMMIT
SHA256SUMS
```

The HTTPS downloader **is embedded in the primary Manager ELF**.
Do not expect or install a separate `COVER_HTTPS_V12.ELF` or
`DVR_ISO_MANAGER_V12_CPU.ELF` with this package.

## Installation

1. Use a FAT32 USB drive. **Back up** your current `DESR_DVR` directory,
   especially `BOOT.CNF`, `NET_PROBE.CNF`, `V12_NET.CNF`, `ART/`,
   `COVERS.IDX` and personal settings.
2. **New installation:** copy the `DESR_DVR` folder from this archive
   to the USB root, keeping all its `modules/`, `config/` and Neutrino
   files. Do not format or modify Sony DVR partitions.
3. **Upgrade from a working installation:** replace the existing
   `DESR_DVR/DVR_ISO_MANAGER.ELF` with the file from this archive.
   Copy `DESR_DVR/N4E.CA` if absent or outdated. Preserve the already
   configured files, artwork, BOOT files and modules unless you have a
   specific reason to replace them.
4. HTTPS needs a **valid** `mass0:/DESR_DVR/V12_NET.CNF` with exactly
   `IP`, `NETMASK`, `GATEWAY` and `DNS`. Preserve an existing working
   file, or copy it once from the previous `hdd0:PP.DVRISO12/V12_NET.CNF`.
   Otherwise edit `V12_NET.CNF.example` with your REAL LAN values,
   then save as `V12_NET.CNF` in `DESR_DVR`.
   The example addresses are placeholders; **this archive deliberately
   does NOT contain a ready-to-use `V12_NET.CNF`.**
5. Start `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` via wLaunchELF-R3Z 4.80.

## Controls and download modes

At the game list, **Triangle** scans for missing covers.
In the confirmation dialog: **Triangle** starts direct HTTPS from the
console, **X** starts the legacy LAN HTTP method with the Windows ART
Server, **O** cancels. HTTPS downloads up to **50 covers per batch**
from the OPL Manager artwork archive with CosmicScale fallback.
Validated PNG files are normalized, saved safely via `.PART` and USB
readback, and indexed on the next Manager load. After a download,
**X or O** dismisses the result summary.

Switch between `SYSTEM` and `ENGLISH` with **SELECT** on the game list.
The game-launching function continues to use OSDMenu and Neutrino.

## Important notes

- The certificate bundle is at `DESR_DVR/N4E.CA`, **not** the USB root.
- HTTPS uses `DESR_DVR/V12_NET.CNF`, **not** the HDD network file.
- The old `NET_PROBE.CNF` remains the separate configuration for LAN HTTP.
- The experimental CPU-counter entropy method was **directly borrowed
  from Daniel's [NULD](https://github.com/danielnuld/nuld)** and adapted
  for this application. Its cryptographic entropy quality is not proven.
  Hashing or repeated sampling alone cannot create additional entropy.
- The earlier sporadic green-screen problem at ELF return is not
  definitively root-caused. Keep a backup of a known-good ELF.
- This package is **part of official Release 1.2**. The original source notices and licensing obligations for wolfSSL, NULD-derived techniques and other components continue to apply. Attribution does not replace legal compliance.

## Provenance, credits and verification

The package is built in GitHub Actions from the `pdim_usb_v1.2` branch.
`SOURCE_COMMIT` identifies the exact source revision;
`SHA256SUMS` lists the SHA-256 hashes of all package files.

Thanks and upstream credits: **Daniel / NULD** (borrowed COP0 Count
entropy approach, GPL-3.0), **Tanner Ryan** (TXRyan signed Roughtime
server), **wolfSSL**, **Monocypher**, **PS2SDK**, **Neutrino/ps2max32**
and **pcm720** (DESR support), **OPL Manager/Luden02** and
**CosmicScale** (cover archives), **Mozilla/curl** (CA bundle), and
**Berion** (technical assistance).
See the project's [HTTPS 1.2 technical guide](../docs/HTTPS_1.2_TECHNICAL_EN.md)
for detailed attribution, security limitations, and license notes.
