# Changelog

## [1.0.0] — prepared for release

**Primary development direction: HDD** (USB remains supported).

- Hardware-tested HDD Manager on `PP.DVRISO`, with 44/44 cover downloads (40 + 4), two launched games and selectable SYSTEM/ENGLISH.
- XMB Games → Manager launch using the PDIM HDD/DVD icon and wLaunchELF_R3Z APA-header injection.
- Neutrino HDD PFS environment wrapper, dynamic ISO path validation (FIX95).
- HDD TCP/HTTP integrity repair (NETMAN RX DMA / patched SDK), without debug counters or NETFIX in the Manager title.
- Separate 1.0 USB ELF and complete USB package; follows the v0.8 USB architecture. USB v1.0 binary not yet independently tested on the console.
- Bilingual technical and release documentation, with upstream attribution and safe install instructions.

**Excluded:** experimental Native QUIT, automatic disk formatting or partition creation, one-click XMB installer.

## [0.8] — 2026-10-31

USB game browser (12 per page), artwork fetching/indexing, PC ART server; validated 40/40 covers and REZ in English.

## [0.3.0] — 2026-10-18

First public version.
