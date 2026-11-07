# PSX DVR ISO Manager 1.0 HDD — architecture, fixes and hardware results

> **Update, 2026-11-07:** HDD 1.0 has also launched from the XMB Games entry on a physical DESR-7000 (PDIM icon, wLaunchELF_R3Z APA header injection). After removal of the display-only NETFIX suffix the Manager still worked. The HDD build is now a release candidate; the USB v1.0 build was also confirmed working on DESR-7000 on 2026-11-07 (without attributing the separate v0.8 cover/game tests to v1.0). See [release notes](RELEASE_NOTES_v1.0.0_EN.md) and [release checklist](../release-tools/v1_0/RELEASE_CHECKLIST.md). This does not imply all games/models are supported.

[Polski / Polish](HDD_1_0_TECHNICAL_PL.md) · [Project README](../README.md) · [Manager sources](../dvr-iso-manager/) · [Neutrino integration](../neutrino-hdd-v1/) · [HDD NETFIX](../hdd-netfix/)

> **Status as of 2026-11-06:** HDD 1.0 is **hardware-tested on a Sony PSX DESR-7000**, but **has not yet been published as a stable v1.0 release**. Validated development branch: `feature/v1.0-hdd`, commit `4ef4aba3a6347b3462a117890f992b13af0d38d3`. [Successful build and complete package](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42170404678).

## 1. Hardware verification

| Check | Result | What was actually verified |
| --- | --- | --- |
| Copy the complete package to `PP.DVRISO` and run the HDD Manager | **PASS** | Physical DESR-7000 |
| Download covers into `PP.DVRISO/ART/` over LAN | **PASS** | 44 covers in two batches, **40 + 4** |
| Display the simplified download count on the game list | **PASS** | No BAD/HTTP/CRC counters on the list |
| Launch games from DVR ISO storage | **PASS** | Two games launched in this test |
| Force ENGLISH | **PASS** | One of those two games launched in forced English |
| Earlier DIAG Z / FIX95 tests (including MGS2 and ICO) | **PASS, historical** | Previous boot-chain diagnostics |
| Earlier forced-English Metal Slug test | **PASS, historical** | Previous NETFIX verification |
| Boot compatibility of all 44 titles | **NOT TESTED** | 44/44 refers to **covers**, not game compatibility |
| Automatic XMB installation / launch through an installed XMB entry | **NOT VERIFIED** | Separate work |
| Other DESR models, storage devices or networks | **NOT VERIFIED** | Community testing needed |

A successful real-hardware test is not a blanket guarantee. The separately released **USB v0.8** variant remains available.

## 2. The actual architecture

The USB Manager keeps its resources at `mass0:/DESR_DVR/`. Game ISO images, however, already reside on Sony's DVR file store at `dvr_pfs0:/ISO/`. Simply copying Neutrino or the Manager ELF to HDD did not establish a stable PFS mount, working directory or module/configuration environment across OSDMenu Quickboot.

HDD 1.0 places *application-owned data* on its own APA/PFS partition, `hdd0:PP.DVRISO`, **separate from Sony system partitions and from the DVR game ISO store**:

~~~text
Sony PSX DESR
├── dvr_pfs0:/ISO/               game ISO images (Manager only reads)
└── hdd0:PP.DVRISO               dedicated application APA/PFS partition
    ├── DVR_ISO_MANAGER_HDD.ELF
    ├── BOOT.ELF                  existing OSDMenu Quickboot
    ├── BOOT.CNF                  selected game's launch configuration
    ├── neutrino_hdd.elf         SYSTEM
    ├── neutrino_hdd_lang.elf    forced ENGLISH
    ├── NET_PROBE.CNF
    ├── COVERS.IDX, COVER_SCAN.LOG
    ├── ART/<GAMEID>_COV.png
    ├── config/*.toml
    └── modules/*.irx, ee_core.elf, ee_core_lang.elf
~~~

**Mount hand-off matters:** the Manager mounts the app partition at `pfs1:/`. After OSDMenu has taken over, the modified Neutrino wrapper remounts the same partition **read-only** at `pfs0:/`. These are not the DVR ISO device (`dvr_pfs0:`).

## 3. Why the original Neutrino workflow failed from HDD

This project did **not** prove an inherent upstream Neutrino inability to read DVR ISO files. The DVR-enabled upstream fork already had the necessary FHI backend. The integration issue was the *launch environment* inherited from OSDMenu:

1. Upstream Neutrino opens relative resources such as `config/system.toml`, `config/bsd-dvr.toml` and `modules/…`.
2. Loading the executable from a PFS partition through OSDMenu does not guarantee that `pfs0:` is still mounted or that the current directory points to the application root after Quickboot and IOP transitions.
3. Neutrino must initialize fileXio, remount the app partition, and set the working directory **before** continuing into upstream configuration/module loading.
4. The investigation also encountered a configuration-stage failure (**STAGE 60**). Using the correct matched `config/` and `modules/` files was important. These diagnostics should not all be characterized as bugs in unmodified Neutrino.

The [HDD wrapper](../neutrino-hdd-v1/patch_hdd_runtime.py), in abbreviated form:

~~~c
SifInitRpc(0);
fileXioInit();
fileXioMount("pfs0:", "hdd0:PP.DVRISO", FIO_MT_RDONLY);
chdir("pfs0:/");
rc = neutrino_hdd_upstream_main(argc, argv);
~~~

The actual implementation checks errors and unmounts its own PFS mount if control unexpectedly returns. **We did not replace upstream `setup_dvd_iso()` or implement the DVR FHI backend ourselves.**

## 4. DIAG Z FIX95 — the real ISO-path regression

During our **own earlier diagnostics**, a deliberately non-existent `DIAG_V_NO_GAME.iso` was used to prevent a real game from starting. A leftover test-only equality check later rejected the correct ISO path provided by OSDMenu. **That was our diagnostic guard, not an original upstream Neutrino bug.**

FIX95 removes the dummy ISO comparison and validates the **actual selection** after command-line and TOML processing:

~~~c
/* argv[2] is "-dvd=dvr_pfs0:/ISO/NAME.iso".
   Skip the five-byte "-dvd=" prefix. */
if (!desr_iso_arg_allowed(argv[2]) ||
    !sDVDFile ||
    strcmp(sDVDFile, argv[2] + 5) != 0)
    return -95;
~~~

The allowlist restricts paths to `dvr_pfs0:/ISO/`, disallows dangerous extra path components/characters, and compares the ultimately selected ISO with OSDMenu's argument. SYSTEM and ENGLISH follow the same rule. Existing DVR FHI and `setup_dvd_iso()` remain upstream implementations. The earlier DIAG Z FIX95 workflow launched MGS2 and ICO on the DESR-7000.

**Upstream credit:** [Neutrino by ps2max32 and contributors](https://github.com/ps2max32/neutrino), [pcm720's DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228), pinned upstream commit `aa8d80a1a0b5323cb407c814c0568c3489597c1c`. The DVR backend, `dvrfile_fhi.irx`, and core DVD emulation **pre-date this project**.

## 5. OSDMenu, BOOT.CNF and SYSTEM / ENGLISH

When X is pressed on a game, the Manager:

1. Preserves the exact ISO filename case and builds `BOOT.CNF` in its own `PP.DVRISO` partition.
2. Temporarily enables application-partition writes, stages the configuration in `BOOT.NEXT.CNF`, verifies it, and uses a rollback/backup mechanism involving `BOOT_RECOVERY.CNF` and `BOOT.PREV.CNF`.
3. Calls the existing OSDMenu `BOOT.ELF` with the explicit configuration argument `hdd0:PP.DVRISO/BOOT.CNF`.
4. OSDMenu passes `-bsd=dvr` and `-dvd=dvr_pfs0:/ISO/…` to the HDD Neutrino binary. Its wrapper re-establishes `pfs0:/` and the existing DVR FHI launches the image.

Example for SYSTEM:

~~~ini
boot=neutrino_hdd.elf
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/MGS 2.ISO
arg=-dev9=NICHDD
#PAD
~~~

ENGLISH chooses `neutrino_hdd_lang.elf` and adds `arg=-cfg=osdlang` and `arg=-osdlang=en` **before** `arg=-dev9=NICHDD`. It includes the [English language patch](../neutrino-v02/neutrino-osdlang-en.patch), matching `config/osdlang.toml` and `modules/ee_core_lang.elf`. It does **not** permanently change XMB/NVRAM language settings.

OSDMenu consumes `-dev9=NICHDD`; the terminal `#PAD` line is not a Neutrino argument. See [`hdd_boot_config.c`](../dvr-iso-manager/src/hdd_boot_config.c).

## 6. HDD NETFIX — network corruption during cover downloads

The USB variant downloaded valid PNGs. Earlier HDD variants sometimes received an HTTP body whose **Content-Length was correct but whose PNG IDAT CRC was wrong**. This still occurred in a RAM-only diagnostic, making the PFS write path an insufficient explanation.

The HDD startup used a different DEV9 provider and additional ATA/APA/PFS modules. We did **not** isolate a single faulty DEV9 instruction. The successful solution is a set of changes, not a claim that replacing DEV9 alone fixed TCP.

**What the validated build changes:**

- Uses the known `dev9_ns.irx` provider already used by USB, **without loading two DEV9 providers**; retains `ps2atad` / `ps2hdd` / `ps2fs` and the usual `NETMAN` + `SMAP` Ethernet modules.
- Rebuilds both EE network libraries and IOP modules against **one pinned PS2SDK commit**: `d41ea62946e53d9a0c8788bb9eb8c8faf488b559`.
- Reworks NETMAN receive DMA: it uses separate **64-byte-aligned bounce frames**, **64 × 1536 bytes**, reads them via an uncached alias, and copies exactly the received packet length into CPU-owned lwIP pbuf memory **before** rearming the IOP slot.
- Avoids the problematic post-DMA writeback/invalidate operation on cached pbuf payload data.
- Enables IPv4 and TCP checksum validation in lwIP, while **retaining** HTTP Content-Length checking, complete PNG/CRC validation, and .PART write/readback verification on PFS.
- Keeps ordinary **SMAP RX DMA** enabled. The experimental PIO RX workaround is not used; the older **512-byte recv diagnostic** is also not part of the NETFIX release build.

See [NETFIX patch](../hdd-netfix/patch_sdk.py), [build script](../hdd-netfix/build_sdk.sh), and [analysis](../hdd-netfix/README_PL.md).

**Evidence boundary:** 44/44 downloaded covers shows that the *combined repair* works on the tested setup. It does not independently prove which individual cache/DEV9 difference caused every former corruption.

## 7. Artwork workflow and the UI-only follow-up

Triangle performs a manual ISO scan, normally extracting Game IDs from an image's `SYSTEM.CNF` or an unambiguous root executable filename. `COVER_SCAN.LOG` includes recognized games whose artwork is absent; `COVERS.IDX` indexes found covers. The PC-side [ART Server](../pc-art-server/README.md) fetches artwork externally and serves validated PNGs over local HTTP to the PSX.

The downloader intentionally caps a session at **40 candidate downloads** (`ART_SYNC_BATCH_MAX`), which is why 44 covers completed in **40 + 4** sessions. The app writes only new files, using .PART staging, validates them, and rebuilds the index.

The hardware-verified UI follow-up, commit `4ef4aba3`, changes **only the menu rendering function** `draw()` in [`src/main.c`](../dvr-iso-manager/src/main.c). After a batch the game list shows just the count, e.g.:

~~~text
ART SYNC: 40 covers downloaded
~~~

Low-level network/PNG/PFS and game-boot code is untouched by that UI change. Detailed transfer diagnostics remain available in the relevant tools.

## 8. Building and packaging

The development branch includes a pinned-SDK GitHub Actions build and two Neutrino variants (SYSTEM and ENGLISH):

- [Complete HDD 1.0 package workflow](../.github/workflows/build-hdd-test-bundle-v1.yml)
- [HDD Manager / NETFIX workflow](../.github/workflows/build-dvr-iso-manager.yml)
- [Neutrino HDD workflow](../.github/workflows/build-neutrino-hdd-v1.yml)

**The distributed user-facing file is `DVR_ISO_MANAGER_HDD.ELF`**, and the full package includes **no preview ELF**. The internal build target remains named `DVR_ISO_MANAGER_HDD_TEST.ELF` for historical workflow compatibility; packaging renames it without changing its instructions. This is **not an XMB auto-installer**.

Do not blindly replace a working `BOOT.CNF`, `config/`, `modules/`, `ART/` or cover index when upgrading. Back up existing data first. The clean package intentionally omits `BOOT.CNF`; the Manager creates it when a game is selected. The Manager does **not** create/format Sony partitions.

## 9. Attribution, scope and future contributions

| Component | Origin |
| --- | --- |
| Base Neutrino and DVD emulation | Neutrino upstream contributors |
| DVR FHI / DVR PSX support in Neutrino | pcm720's [PR #228](https://github.com/ps2max32/neutrino/pull/228) |
| OSDMenu / Quickboot | existing OSDMenu project; used and integrated here |
| HDD PFS wrapper, FIX95 checks, SYSTEM/ENGLISH configuration | integration changes in this repository |
| HDD Manager, `PP.DVRISO`, verified CNF/ART writes and user interface | PSX DVR ISO Manager |
| NETMAN RX DMA isolation and matching PS2SDK rebuild | [HDD NETFIX](../hdd-netfix/) |
| Windows artwork fetch/cache and LAN HTTP server | [PC ART Server](../pc-art-server/) |

Please preserve the upstream authorship and licenses for Neutrino, OSDMenu, PS2SDK and other dependencies. This document **does not relicense third-party code**.

**Still open:** more game/model compatibility reports, a separate reversible XMB installer, release documentation review and community feedback. The experimental **NATIVE QUIT** branch was **not merged** into this working HDD 1.0 build.

### Source links

- [HDD wrapper and FIX95](../neutrino-hdd-v1/patch_hdd_runtime.py)
- [NETMAN receive DMA / checksums](../hdd-netfix/patch_sdk.py)
- [HDD runtime paths](../dvr-iso-manager/include/runtime_storage.h)
- [BOOT.CNF generator](../dvr-iso-manager/src/hdd_boot_config.c)
- [Hardware-verified full package CI](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42170404678)
