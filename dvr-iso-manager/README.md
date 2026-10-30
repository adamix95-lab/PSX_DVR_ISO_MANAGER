# PSX DVR ISO Manager v0.6 — MONO Refined

**Experimental visual-only update. Not yet tested on physical Sony PSX DESR-7000 hardware.** Preserve the verified v0.5.3 ELF and USB BOOT.CNF before testing.

## What's new

- The PS2SDK MONO browser stays black, white and grayscale; no GS reinitialization or extra sprite library.
- Highlighted game uses a **dark-gray character background** with a white `>` and white title; other entries use slightly dimmer gray lettering. Only the highlighted row is padded to avoid unnecessary DMA transfers.
- Subtle gray separators, padded page indicator (`PAGE 01/04`), clearer controls at the bottom and a small `GAME ID` label.
- **Original preferred cover position is retained**: Game ID text row 7, PNG preview at y=64, 88×128 pixels, existing bilinear scaling and corrected RGB colors.
- Status/footer text is constrained to the left so it cannot draw over the cover.
- Scanner, unique-executable ID fallback, USB index, cover ART files, Quickboot, language modes, BOOT.CNF and all DVR access are unchanged.

## Controls

UP/DOWN: select one game. LEFT/RIGHT: move one page (12 titles).
X: launch through USB OSDMenu/Neutrino. TRIANGLE: manual ISO + cover index scan.
SELECT: SYSTEM / experimental ENGLISH. SQUARE: verify DVR_TEST.CNF. START: exit.

## Storage and safety

ISO images remain read-only at `dvr_pfs0:/ISO/`.
On USB the program uses `mass0:/DESR_DVR/COVERS.IDX`, `ART/<GAMEID>_COV.png` and the optional `COVER_SCAN.LOG` created only on manual rescan. `BOOT.ELF`, `BOOT.CNF` and Neutrino must remain on USB even if Manager itself is loaded through UDPFS.

The v0.5.3 hardware test matched all 40/40 covers using SYSTEM.CNF/BOOT2 or the unique root boot executable fallback. Normal startup uses the index, without rescanning ISO contents.

## DESR-7000 test procedure

1. Keep a copy of the proven v0.5.3 ELF and USB BOOT.CNF.
2. Download `PSX-DVR-ISO-Manager-v0.6-MONO-REFINED-UNTESTED`, replace **only** the Manager ELF.
3. Check the gray selection bar, navigation across all pages, readability of the longest title, and that status/footer never overwrite the artwork.
4. Confirm covers are still cached after restart, TRIANGLE and SELECT work, then perform one known-good X launch (e.g. Black).
5. If there is screen corruption, lag or freezing, restore the old ELF.

**Do not merge this Draft before physical testing.** See [Polish guide](README_PL.md) and [changelog](CHANGELOG.md).
