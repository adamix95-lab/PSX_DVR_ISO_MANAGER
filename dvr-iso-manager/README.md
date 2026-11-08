# PSX DVR ISO Manager 1.0

PSX DVR ISO Manager is a homebrew game launcher for **Sony PSX DESR** consoles. It lists and launches PlayStation 2 ISO games stored on the console's DVR partition.

## Version 1.0

Two variants are available:

- **HDD 1.0 (recommended):** runs from the `hdd0:PP.DVRISO` APA/PFS application partition and supports launching the Manager from XMB. Includes the tested HDD network fix (NETFIX).
- **USB 1.0:** runs from `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` on a FAT32 USB drive.

Both variants feature:

- Up to **128 ISO entries** in the browser, with **12 games per page**.
- Game ID detection and color PNG cover previews.
- Missing-cover scanning and downloads through the companion Windows ART Server.
- OSDMenu / Neutrino game launching with **SYSTEM / ENGLISH** selection.
- Read-only access to game ISOs in `dvr_pfs0:/ISO/`. Covers, configuration and indexes are stored in the application location.

**Hardware status:** HDD 1.0 and USB 1.0 were confirmed working on a Sony PSX DESR-7000. The HDD tests included 44 downloaded covers and successful game launches. This does not imply every ISO was individually boot-tested.

## Download and installation

**[Download PSX DVR ISO Manager 1.0 (HDD, USB and optional XMB package)](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.0)**

For installation steps, see the [main English README](../README.md) or the [Polish guide](../README_PL.md). For HDD implementation details, see the [technical documentation](../docs/HDD_1_0_TECHNICAL_EN.md).

**XMB integration is a manual procedure**, not a one-click installer. Back up the APA partition header before modifying it.

## Build

The project uses **PS2SDK**, CMake and PS2 PNG dependencies (`libpng` / `libz`).

```sh
cmake -S dvr-iso-manager -B build-dvr-manager
cmake --build build-dvr-manager -j2
```

The default target outputs `build-dvr-manager/DVR_ISO_MANAGER.ELF` (USB). Building the interactive HDD version requires the patched PS2SDK and the `DESR_BUILD_HDD_NETFIX=ON` option; see [HDD build workflow](../.github/workflows/build-hdd-test-bundle-v1.yml).

## Credits

Based on [Neutrino](https://github.com/ps2max32/neutrino), with DVR support from [pcm720's PR #228](https://github.com/ps2max32/neutrino/pull/228). Thanks to upstream PS2 homebrew contributors and Berion for technical assistance.
