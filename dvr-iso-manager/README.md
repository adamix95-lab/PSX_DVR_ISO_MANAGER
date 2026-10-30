# PSX DVR ISO Manager v0.5.1 — OPL-style clearer preview

**UNTESTED on physical DESR-7000 hardware.** Preserve the proven v0.5 Manager ELF and do not merge before a console test.

## Scope

- Replace the v0.5 11×14 colored character mosaic with a true-color **88×128-pixel PNG bitmap**, fitting the same right-hand MONO column.
- Decode the selected USB PNG with the existing libpng; nearest-neighbor downscale into a bounded 44 KiB buffer; upload through the existing PS2SDK GS GIF/DMA framebuffer (no gsKit video reinitialization).
- Unchanged: Game ID extraction, TRIANGLE manual USB `COVERS.IDX` refresh, cached startup load, no writes to DVR, all Quickboot/Neutrino/BOOT.CNF paths, language switching and 12-game paging.
- JPEG and interlaced PNG preview remain unsupported and show the same fallback text.

## Test

Keep your working v0.5 ELF and a PC backup of USB BOOT.CNF. Download `PSX-DVR-ISO-Manager-v0.5.1-OPL-PREVIEW-UNTESTED` from Actions and launch only the updated Manager ELF using USB or wLE-R3Z/UDPFS. Existing `COVERS.IDX` and `ART` are reused without pressing TRIANGLE. Check Cold Fear's `SLUS_210.47` thumbnail, menu text/footer, switching selection/pages and NO COVER fallback. Finally test one known-good X launch. If the video glitches, restore the previous ELF.

See [Polish documentation](README_PL.md) for installation and hardware-test details.
