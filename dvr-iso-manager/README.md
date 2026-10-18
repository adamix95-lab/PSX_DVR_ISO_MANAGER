# PSX DVR ISO Manager v0.3 — MONO

[Polski / Polish](README_PL.md)

**GUI test version: tested on Sony PSX DESR-7000.**

## What's new in v0.3

- `PSX DVR ISO MANAGER` heading with a small `v0.3` label.
- Monochrome interface: black background and white text; the selected game is marked with `>`.
- Diagnostic lines `DVR: dvr_pfs0:/ISO` and `DVRP notice / hdd0 / pfs0 root` are hidden.
- The standard scan result is replaced with an `N GAMES` counter.
- The `.iso` or `.ISO` extension is hidden on screen, but the full original filename is still written to BOOT.CNF.
- `Selected 1 / 7` and the technical `Backup: ... No DVR writes` line are hidden.
- The PS2SDK block cursor (`scr_setCursor(0)`) is disabled to prevent white squares from appearing between sections.
- Up to 12 games are displayed per page; page numbers appear only when there are more games.
- Only the controls and, when needed, CNF test or error messages are shown at the bottom.

## Controls

- Press **UP** or **DOWN** to select a game.
- Press **X** to launch the selected game.
- Press **Triangle (△)** to rescan the "ISO" folder.
- Press **Square (□)** to save a test configuration to `DVR_TEST.CNF`.
- Press **SELECT** to change the game language — for games that use the console menu language (*tested with REZ and Metal Slug Anthology*).
- Press **START** to exit.

## Unchanged from the working v0.2 release

- ISO images are still read from `dvr_pfs0:/ISO/` (the `__xcontents` partition).
- `SYSTEM` uses the verified `neutrino.elf`; `ENGLISH` uses the separate `neutrino_lang.elf` with `ee_core_lang.elf`.
- On the first press of X, the manager creates a `BOOT_RECOVERY.CNF` backup, writes and verifies `BOOT.CNF`, then launches OSDLauncher (`BOOT.ELF`) with the appropriate arguments.
- Nothing is written to the DVR/APA partitions.
- Switching languages does not translate game content if the ISO lacks English-language resources.
- The same CNF generators and launcher functions as in v0.2 are used.

**Important:** Launching the manager itself from UDPFS does **not** move the Quickboot path to the network.
`mass0:/DESR_DVR/BOOT.ELF`, `BOOT.CNF`, and both Neutrino executables must remain on USB.
