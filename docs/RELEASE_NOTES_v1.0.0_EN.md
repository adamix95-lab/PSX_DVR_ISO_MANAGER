# PSX DVR ISO MANAGER v1.0.0 — GitHub Release notes

**HDD is now the main development branch. USB remains supported.** On Sony PSX DESR-7000, the HDD application successfully downloaded 44/44 artwork files in two batches (40 + 4), launched two games including one in forced English, and launched from the XMB Games entry with the PDIM HDD/DVD icon. The clean title displays `1.0 HDD` with no NETFIX suffix; the underlying network fix remains active.

## Release attachments

- `PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip` — the **byte-for-byte hardware-tested** Manager and its Neutrino SYSTEM/ENGLISH, OSDMenu, modules and configuration on `PP.DVRISO`.
- `PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip` — the supported USB variant with a `1.0 USB` header, built from the HDD-branch Manager's shared source code and the approved USB v0.8 package.
- `PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip` — **optional** XMB integration with wLaunchELF_R3Z and the indexed 76 × 108 PDIM artwork.
- `SHA256SUMS.txt` — checksums for the archives.

The Windows [ART Server v0.8](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/v0.8) is a separate download and remains the companion server. Do not copy game ISOs into the application directory.

## Installation

**HDD:** Back up the disk and the application partition; copy the contents of `PP.DVRISO-FILES/` to the root of the already-existing `hdd0:PP.DVRISO` partition. Do not overwrite personal `ART/`, `BOOT.CNF`, `NET_PROBE.CNF` or indexes without a backup. Launch `DVR_ISO_MANAGER_HDD.ELF` through wLE; the game ISO images remain in the read-only DVR store `dvr_pfs0:/ISO/`.

**XMB (optional):** Back up the **APA partition header**, copy `EXECUTE.ELF` and `EXECUTE.CNF` from the XMB ZIP to the existing `PP.DVRISO`, then in wLaunchELF_R3Z select **PP.DVRISO only → R1 → Inject Header → This partition only** with `__Headers/PP.DVRISO/` on FAT32 USB. **Never format, create/expand partitions or inject headers to all partitions.** An already-working XMB installation does *not* need reinjection for a Manager update.

**USB:** Copy the inner `DESR_DVR/` folder from the USB ZIP to the root of a FAT32 stick and run `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` through wLE. Preserve your settings during upgrades.

## Hardware test coverage and limitations

HDD 1.0 / XMB → Manager: **PASS on DESR-7000**. USB **v0.8** baseline: **PASS** for 40/40 covers and REZ in English; the **new USB 1.0 binary has not been independently hardware-tested**. Not all 44 game ISOs, all DESR models, and not a one-click XMB installer have been verified. Native QUIT is an unmerged experiment.

## Development and credits

See [HDD architecture and FIX95/NETFIX technical documentation](docs/HDD_1_0_TECHNICAL_EN.md) ([Polski](docs/HDD_1_0_TECHNICAL_PL.md)). Original Neutrino and DVD emulation by upstream contributors; DVR FHI originated in pcm720's [PR #228](https://github.com/ps2max32/neutrino/pull/228). Existing OSDMenu, PS2SDK, wLaunchELF_R3Z and hdl-dump remain credited to their respective developers. GPLv2 template source and license are included with the XMB ZIP. These components are not newly authored by this project.
