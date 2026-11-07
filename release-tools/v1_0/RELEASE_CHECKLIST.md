# v1.0.0 — publication checklist / lista przed publikacją

> **Status:** packages prepared and checksummed; nothing has been published as GitHub Release v1.0.0 yet. HDD hardware PASS; XMB launch PASS; USB v1.0 binary built but not separately hardware-tested.

## Release files (exact filenames and SHA-256)

| Asset | SHA-256 |
| --- | --- |
| `PSX-DVR-ISO-Manager-v1.0.0-HDD-COMPLETE.zip` | `4ad658817f25b6254b66f0f65f3bd34ca72af70e89d71756eed706d432b7e00f` |
| `PSX-DVR-ISO-Manager-v1.0.0-USB-COMPLETE.zip` | `18d4176d2d4e09277864e577735286fa71ae9e9d4561a93dd7f33d4c56493ba7` |
| `PSX-DVR-ISO-Manager-v1.0.0-XMB-PDIM-INSTALL.zip` | `9eb31f374c9e71bbad4a94793bb7e20f363d43cb1ffe2183fdbd82271cc7e834` |

HDD Manager ELF SHA-256: `cf2f51a1548c53e5e39086d571c591d782aefc22d5db8af99e9b51b0d7c6f7c0`.
Tested XMB KELF SHA-256: `b335a5010adc94635caf735b33229ce377f146f9c256974cc7fd2ebf38684b49`.
XMB icon: `jkt_002.png`, 76x108 indexed PNG8. No `HDD_TEST` / `PREVIEW` files in the release HDD package.

## Steps before clicking Publish

- [x] Select verified HDD candidate, clean title, retained NETFIX and two Neutrino variants.
- [x] Verify KELF matches hardware-tested wrapper and update XMB installer with current HDD Manager.
- [x] Keep USB 1.0 as a separate download and identify its testing limitation.
- [x] Prepare bilingual release notes and README; document upstream authors and APA risks.
- [x] Build three ZIPs and validate ZIP CRC / ELF header / partition paths / SHA-256.
- [ ] Review and merge release preparation PR into `main` (becomes HDD-first default development history); keep `feature/v1.0-usb` for USB-specific work.
- [ ] On GitHub, create a **draft release** at tag **`v1.0.0`** targeting the merged `main` commit.
- [ ] Attach the three ZIPs plus `SHA256SUMS.txt`; optionally attach `RELEASE_MANIFEST.json`.
- [ ] Paste [English release notes](../../docs/RELEASE_NOTES_v1.0.0_EN.md) (Polish version available); check final filenames and links.
- [ ] Publish the draft when ready. Do not accidentally mark the USB 1.0 binary as independently hardware-tested.
- [ ] Optional future check: end-to-end XMB → specific game, and other DESR consoles.

## Sources

- HDD build: [GitHub Actions run 42346457001](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42346457001)
- USB 1.0 baseline built in [run 42346249233](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42346249233)
- Approved USB v0.8 packaging [run 42346456930](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42346456930)
- Packaging tool: `release-tools/v1_0/build_release_assets.py` (host-side Python/Pillow)
