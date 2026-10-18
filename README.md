## PSX_DVR_ISO_MANAGER

[Polski / Polish](README_PL.md)

A simple tool for launching games in .ISO format from the DVR partition of a Sony PSX DESR console.
This is a prototype, with a basic interface and a limited set of features.

## Installation
In the current version, all program configuration and supporting files (patches, OSDLauncher files, Neutrino, etc.) **MUST** be placed in the `DESR_DVR` directory on a USB storage device. `DVR_ISO_MANAGER.ELF` can be launched either from USB or from a UDPFS server.

```text
mass0:/DESR_DVR/
│
├── BOOT.ELF                 # OSDMenu Launcher
├── BOOT.CNF                 # Configuration for the game to launch
├── BOOT_RECOVERY.CNF        # Backup configuration
├── DVR_TEST.CNF             # Test configuration
│
├── neutrino.elf             # Neutrino DVR - SYSTEM
├── neutrino_lang.elf        # Neutrino DVR - ENGLISH
├── DVR_ISO_MANAGER.ELF      # Manager v0.3 - updates BOOT.CNF and launches neutrino.elf or neutrino_lang.elf
│
├── config/
│   ├── system.toml          # Basic configuration
│   ├── bsd-dvr.toml         # DVR support
│   └── osdlang.toml         # ENGLISH language configuration
│
└── modules/
    ├── ee_core.elf          # Standard Neutrino core
    ├── ee_core_lang.elf     # Modified ENGLISH core
    ├── dvrdrv.irx           # DVR driver
    ├── dvrfile.irx          # DVR file support
    └── dvrfile_fhi.irx      # ISO access during gameplay
```

**Note:** This list includes only the most important project files; it does not cover all Neutrino modules.

ISO images must be stored on the DVR partition under `dvr_hdd0:/__xcontents/ISO/*`.

## Using the application
<img width="640" height="480" alt="PSX DVR ISO Manager interface" src="https://github.com/user-attachments/assets/344403f5-11b3-4a1d-925e-0c676ad2fbb2" />

- Press **UP** or **DOWN** to select a game from the list.
- Press **X** to launch the selected game.
- Press **Triangle (△)** to scan the "ISO" folder again.
- Press **Square (□)** to save a test configuration to `DVR_TEST.CNF`.
- Press **SELECT** to change the game's language. This is intended for games that use the console menu language (*tested with REZ and Metal Slug Anthology*).
- Press **START** to exit the application.

## To do
- Launch the entire application from the HDD through the XMB. The path-related error `-19` has not yet been resolved.

## Contributions welcome!

One of the remaining goals is to launch PSX DVR ISO Manager directly from the PSX XMB using the internal HDD, without requiring a USB drive. Previous attempts encountered error `-19` when resolving the application path. The root cause has not yet been identified.

Contributions, technical insights, and pull requests are welcome — especially from developers familiar with Sony PSX DESR, PS2SDK, and XMB application launching.

## Sources and credits

- Neutrino: https://github.com/ps2max32/neutrino
- PR #228 (pcm720): https://github.com/ps2max32/neutrino/pull/228
- pcm720 - for creating a patch to add DVR support for Neutrino
- Berion — technical assistance

This repository contains the project's build configuration and documentation; the upstream Neutrino sources are downloaded during compilation.
