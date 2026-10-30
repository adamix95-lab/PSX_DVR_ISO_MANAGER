# Changelog

## 0.5.3 — conservative ISO9660 executable-ID fallback (hardware untested)
- After 34/40 covers found, six missing titles diagnosed as `NO_ID`.
- Manual rescans now prefer SYSTEM.CNF/BOOT2 then fall back to the *only*
  matching root PS2 executable ID. Multiple candidates are rejected.
- Distinguish NO_PVD, NO_SYSTEM, BAD_BOOT2, BAD_DIRECTORY, AMBIGUOUS_EXE,
  FOUND_PNG_EXE / FOUND_JPG_EXE and NO_ART_EXE in USB COVER_SCAN.LOG.
- No changes to COVERS.IDX schema, cached startup, renderer, layout,
  USB BOOT.CNF, Neutrino or read-only DVR access.


## 0.5.2 – optional manual cover-scan diagnostics (hardware untested)
- TRIANGLE generates `mass0:/DESR_DVR/COVER_SCAN.LOG` with reason per game:
  NO_ID, NO_ART, FOUND_PNG, FOUND_JPG or READ_ERROR.
- Report formatting is bounded and covered by portable host-side tests.
- The indexed cover database, PNG renderer, Quickboot and DVR access are unchanged.
- Game ID/cover panel restored to its original preferred placement.


## 0.5.1 – safer OPL-style bitmap preview (hardware untested)
- Reverted rejected 16-pixel cover panel shift; preserve preferred row 7 / y=64 layout.
- Hardware report: bitmap renders, but incorrect `0xRRGGBB` pixel packing
  inverted red/blue and nearest-neighbor sampling produced jagged edges.
- Fix GS PSMCT32 byte order (R low, G middle, B high); use streamed bilinear
  resampling and add host tests for primaries, transparency and interpolation.
- Corrected ELF requires a new physical DESR test; keep v0.5 available.

- Replace 11x14 glyph-based mosaic with 88x128 real PNG pixels.
- Transfer one bounded PSMCT32 rectangle to PS2SDK's existing debug framebuffer
  using the same GIF/DMA channel and transfer packet layout as scr_putchar.
- No GS/gsKit reset, no cover index changes, no Quickboot or DVR writes.
- Keep v0.5 ELF as fallback pending physical DESR-7000 test.


## 0.5 – experimental manual ART index + tiny PNG preview
- Read-only ISO9660 SYSTEM.CNF Game ID reader and OPL cover discovery.
- USB COVERS.IDX with staging/verification and backup, manual TRIANGLE scan.
- 11×14 color-tile PNG preview on the existing debug framebuffer; JPEG indexed only.
- Keep all existing v0.4 navigation and Quickboot behavior. Hardware untested.


## 0.4 – MONO page navigation (hardware untested)
- LEFT/RIGHT changes 12-game pages without wrapping and keeps the highlighted row where possible.
- Selected ISO filename and all launch/CNF paths remain unchanged.
- Portable page helper + host-side edge-case tests; update build artifact name.
- TRIANGLE still only rescans the DVR ISO directory; artwork/Game IDs are not part of v0.4.

## 0.1 – read-only DVR scanner + USB CNF generator (unverified PS2 build)
- Full DVR driver startup and diagnostic text UI.
- Filters .iso/.ISO (case insensitive), sorts menu entries.
- Creates one BOOT.CNF on USB, without UTF-8 BOM.
- Prevents path traversal and CNF injection via filenames.
- Leaves DVR/APA untouched.
