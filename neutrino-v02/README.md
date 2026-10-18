# Neutrino PR #228 — OSD English v0.2 (experimental)

[Polski / Polish](README_PL.md)

**Status: confirmed working on DESR-7000 with Metal Slug Anthology and REZ.** Forcing the language does not guarantee English in every game.

Patch target: `aa8d80a1a0b5323cb407c814c0568c3489597c1c` (Neutrino PR #228).

## How it works

A new Neutrino launch option is available: `-osdlang=en` (force English) or `-osdlang=system` (default).

Two system calls, `GetOsdConfigParam` and `SetOsdConfigParam`, are hooked in the same way as in Open PS2 Loader. The game sees `language=1` and `japLanguage=1`.

Nothing is written to NVRAM, and neither the XMB language nor the DVR contents are changed.
This test version does not translate in-game text if the ISO does not contain English-language resources.

## Installation without modifying your existing working setup

The OSD Language workflow artifact contains exactly **THREE NEW FILES**:

```text
mass0:/DESR_DVR/neutrino_lang.elf
mass0:/DESR_DVR/modules/ee_core_lang.elf
mass0:/DESR_DVR/config/osdlang.toml
```

Do not overwrite the existing `neutrino.elf`, `modules/ee_core.elf`, or `config/system.toml`.
Do not replace the entire `modules/` or `config/` directories. Add only the files listed above.

Install the v0.2 `DVR_ISO_MANAGER.ELF` separately from the `Build DVR ISO Manager v0.2` workflow.
Keep a backup of your current `BOOT.CNF` on your computer.

## v0.2 Manager controls

- **UP / DOWN** — select a game (ISO from the DVR directory).
- **SELECT** — toggle between SYSTEM and ENGLISH. The verified SYSTEM mode is the default.
- **Square (□)** — write and verify a separate `DVR_TEST.CNF`.
- **X** — back up the original `BOOT.CNF`, write and verify the new `BOOT.CNF`, then launch OSDMenu `BOOT.ELF`.
- **Triangle (△)** — rescan; **START** — exit.

SYSTEM mode — the existing, working Neutrino:

```ini
boot=neutrino.elf
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/REZ.iso
```

ENGLISH mode — a separate executable and its own EE core:

```ini
boot=neutrino_lang.elf
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/REZ.iso
arg=-cfg=osdlang
arg=-osdlang=en
```

The `-cfg=osdlang` option loads `config/osdlang.toml`, which points to the `modules/ee_core_lang.elf` module. All other configuration files and modules remain unchanged.

## Sources and licensing

- Neutrino PR #228: https://github.com/ps2max32/neutrino/pull/228
- Open PS2 Loader system hooks: https://github.com/ps2homebrew/Open-PS2-Loader/blob/master/ee_core/src/syshook.c

Neutrino source files are available under the AFL-3.0 license; this repository contains only the patch.
