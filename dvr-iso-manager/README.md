# PSX DVR ISO Manager v0.5.1 — clearer OPL-style PNG preview (UNTESTED)

## Cover panel alignment — experimental test

The hardware-tested RGB/bilinear renderer now shifts the **Game ID and PNG bitmap upward by 16 pixels**: ID on text row 5 alongside `LANGUAGE: SYSTEM`, PNG at y=48, ending at y=176 before the footer. Dimensions remain **88×128**. This layout change has **not yet been tested on real hardware**; preserve the previously working ELF.

Download the latest `PSX-DVR-ISO-Manager-v0.5.1-ALIGNED-COVER-UNTESTED` artifact. No changes to USB COVERS.IDX/ART, DVR access, BOOT.CNF, Neutrino or Quickboot.

**Hardware report, 2026-10-30:** v0.5.1 initially displayed a God of War II cover, but red/blue were swapped and nearest-neighbor scaling caused jagged pixels. The latest artifact `PSX-DVR-ISO-Manager-v0.5.1-ALIGNED-COVER-UNTESTED` corrects the GS PSMCT32 RGB byte order and applies streamed bilinear downsampling. The corrected build remains **hardware untested**. Keep the validated v0.5 ELF and all existing USB assets unchanged.

**Keep the hardware-tested v0.5 ELF.** This renderer-only build has not yet been verified on a physical DESR-7000.

It replaces the 11×14 glyph mosaic with an **88×128-pixel PNG preview** in the original right column, using the already initialized PS2SDK GS GIF/DMA framebuffer. It does **not** reset GS or initialize gsKit. ISO scanning, `COVERS.IDX`, USB BOOT.CNF, Neutrino, Quickboot and game selection are unchanged.

**Test:** keep a PC backup of the working v0.5 ELF and BOOT.CNF; download `PSX-DVR-ISO-Manager-v0.5.1-ALIGNED-COVER-UNTESTED` and replace only Manager ELF. Use the existing ART/index without rescanning. Verify Cold Fear `SLUS_210.47` has a sharper cover; check paging, footer text, NO COVER fallback and a known-good X launch. Restore v0.5 ELF if the video becomes unstable.

---

## Previous release: PSX DVR ISO Manager v0.5 — cover preview (development)

[Polski / Polish](README_PL.md)

**v0.4 is UNTESTED on physical Sony PSX DESR-7000 hardware.** The v0.3 MONO baseline was previously tested.

## What's new in v0.4

- **LEFT / RIGHT** jumps between 12-game pages, retaining the highlighted row when possible and clamping to the last available game on a short final page.
- Stops at the first/last page; no page wrap. UP / DOWN still selects one game at a time, as before.
- **TRIANGLE** remains a simple ISO directory rescan. Game ID scanning, cover art and USB index files are deferred to v0.5.
- No changes to DVR access, CNF generation, language selection, or Quickboot.

## v0.3 MONO baseline

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
- Press **LEFT** or **RIGHT** to move between pages of 12 games.
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

## v0.4 hardware test plan

Keep the working v0.3 Manager ELF and a PC backup of BOOT.CNF. Download the `PSX-DVR-ISO-Manager-v0.4-PAGING-UNTESTED` Actions artifact and replace **only** the Manager ELF. For 1–12 games LEFT/RIGHT should have no effect; with 13+ games verify page changes, short final pages, UP/DOWN and language selection. Finally check SQUARE and one known-working X Quickboot. **Do not merge until verified on DESR-7000.**
