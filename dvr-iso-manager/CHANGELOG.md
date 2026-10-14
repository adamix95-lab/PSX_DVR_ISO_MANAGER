# Changelog

## 0.1 – read-only DVR scanner + USB CNF generator (unverified PS2 build)
- Full DVR driver startup and diagnostic text UI.
- Filters .iso/.ISO (case insensitive), sorts menu entries.
- Creates one BOOT.CNF on USB, without UTF-8 BOM.
- Prevents path traversal and CNF injection via filenames.
- Leaves DVR/APA untouched.
