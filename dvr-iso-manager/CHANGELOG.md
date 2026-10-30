# Changelog

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
