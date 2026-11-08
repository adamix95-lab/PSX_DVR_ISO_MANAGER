# v1.0.0 — publication checklist / lista przed publikacją

> **Status:** **PUBLISHED** — tag [1.0](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.0) with HDD, USB, XMB, Windows ART Server and SHA-256 checksums. HDD hardware PASS; XMB launch PASS; **USB v1.0 hardware PASS (2026-11-07)**. The 40/40 covers and REZ-in-English results belong to the prior USB v0.8 test.

## Release files (exact filenames and SHA-256)

| Asset | SHA-256 |
| --- | --- |
| `PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip` | `4ad658817f25b6254b66f0f65f3bd34ca72af70e89d71756eed706d432b7e00f` |
| `PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip` | `d3e2b08cdb22d33da9247e21bf0402e280b085693ccc9a623d29e34018f841c6` |
| `PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip` | `7e2698d199b67ecd90b190203e1b1ff22cb7cdbf79d9bb8eb9b56ffdd49cd33e` |
| `PSX-DVR-ART-Server-v0.8.exe` (compatible Windows cover server) | `aabe7bcce7f594ed15763e44093f87d3e84893d44ea110a527da455872cdd86a` |

HDD Manager ELF SHA-256: `cf2f51a1548c53e5e39086d571c591d782aefc22d5db8af99e9b51b0d7c6f7c0`.
Tested XMB KELF SHA-256: `b335a5010adc94635caf735b33229ce377f146f9c256974cc7fd2ebf38684b49`.
XMB icon: `jkt_002.png`, 76x108 indexed PNG8. No `HDD_TEST` / `PREVIEW` files in the release HDD package.

## Steps before clicking Publish

- [x] Select verified HDD candidate, clean title, retained NETFIX and two Neutrino variants.
- [x] Verify KELF matches hardware-tested wrapper and update XMB installer with current HDD Manager.
- [x] Keep USB 1.0 as a separate download; **hardware operation confirmed on DESR-7000 on 2026-11-07**. Do not confuse this with USB v0.8's detailed test coverage.
- [x] Prepare bilingual release notes and README; document upstream authors and APA risks.
- [x] Build three ZIPs and validate ZIP CRC / ELF header / partition paths / SHA-256.
- [x] Merge release preparation PR #33 into `main` (HDD-first default development history).
- [x] Create **tag `1.0`** targeting commit `fe9b0b77ba51db87d856ca4a0ee95a0ed9db5ff0` and publish the stable release.
- [x] Attach the **three runtime ZIPs**, compatible Windows ART Server and `SHA256SUMS.txt`. The reproducible source builder and third-party license material are available in `release-tools/v1_0/` and within the XMB ZIP.
- [x] Publish [English release notes](../../docs/RELEASE_NOTES_v1.0.0_EN.md) (Polish version available); verify direct asset download URLs.
- [x] Release published. USB 1.0 is hardware-tested; the detailed USB v0.8 40/40 cover and REZ results remain historical.
- [ ] Optional future check: end-to-end XMB → specific game, and other DESR consoles.

## Sources

- HDD build: [GitHub Actions run 42346457001](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42346457001)
- USB 1.0 confirmed working on physical DESR-7000 (user report, 2026-11-07); build from [run 42346249233](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42346249233)
- Approved USB v0.8 packaging [run 42346456930](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42346456930)
- The reproducible asset builder is included in `PSX-DVR-ISO-Manager-v1.0.0-RELEASE-SOURCES.zip` (host-side Python/Pillow). The GitHub branch contains `release-tools/v1_0/verify_release_assets.py` to validate release binaries.
