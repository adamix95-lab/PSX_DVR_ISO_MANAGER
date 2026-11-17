# PSX DVR ISO MANAGER

[Polski / Polish](README_PL.md)

## Version 1.2 USB — HTTPS cover downloading on Sony PSX (development)

> **Complete USB package prepared, not yet published as a GitHub Release.** HTTPS and the standalone `DESR_DVR/V12_NET.CNF` plus `DESR_DVR/N4E.CA` layout were confirmed on a real DESR-7000, as was the `ENGLISH` option. The **unsuffixed primary `DVR_ISO_MANAGER.ELF`** is a subsequent packaging change that still requires a final hardware launch/return test. The current work lives on [`pdim_usb_v1.2`](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/tree/pdim_usb_v1.2), not in a public Release. The stable **1.0 HDD/USB** release and XMB installer are unchanged. The released **1.0 HDD/USB** Manager and XMB installer are unchanged. This section records the design, problems, solutions, contributors and remaining limitations so it can serve as the foundation for the eventual 1.2 release.

**Goal:** allow the Sony PSX DESR-7000 to download missing PlayStation 2 game covers **by itself, directly over HTTPS**, without starting the Windows ART Server. The previous PC-based LAN HTTP downloader remains available as an alternative.

### Origin of the idea

PSX DVR ISO Manager already read ISO games from the DVR partition, extracted their PS2 Game IDs, displayed cover artwork, and launched titles through OSDMenu/Neutrino. Until 1.2, missing covers generally had to be placed on USB manually or requested through a separate Windows PC ART Server. That was functional, but unnecessarily dependent on another machine.

The next objective was to keep **the entire artwork workflow on the console**: detect a missing cover, retrieve it from a public database, validate it, save it, refresh the index, and return to the game list. [NULD (formerly ORBIT), developed by Daniel (`danielnuld`)](https://github.com/danielnuld/nuld) also demonstrated PS2-side cover downloads over HTTPS. Its solution to sourcing CPU-counter jitter for wolfSSL was **directly borrowed** for our own DESR implementation (explained in detail below). We should not portray that technique as a new invention of this project.

### How 1.2 works

```text
PSX DVR ISO Manager 1.2 USB
  |
  +-- Triangle: scan ISO files and identify missing artwork
  |     X        = old Windows PC / LAN HTTP download
  |     Triangle = console-native HTTPS download
  |     O        = cancel
  |
  +-- PLEASE WAIT... -> launch embedded HTTPS worker ELF
  |     |
  |     +-- Signed time: time.txryan.com:2002 (UDP Roughtime)
  |     |     pinned Ed25519 public key, checked request nonce
  |     |     strict verification; up to 3 attempts for invalid replies
  |     |
  |     +-- HTTPS: raw.githubusercontent.com:443 (wolfSSL)
  |     |     CA verification + SNI/hostname + certificate validity
  |     |     primary artwork: OPL Manager GameArt archive (Luden02)
  |     |     fallback: CosmicScale PSBBN Art Database
  |     |
  |     +-- Validate PNG -> normalize to 140 x 200 RGB
  |     +-- Write USB ART/*.PART -> readback -> final PNG rename
  |
  +-- Reload Manager ELF -> DOWNLOAD COMPLETE / INTERRUPTED
        X / O = return to game list
```

The prepared v1.2 USB complete package contains **one primary Manager file**: `DESR_DVR/DVR_ISO_MANAGER.ELF` (the old 1.0 main executable is replaced). Its HTTPS worker is appended to the Manager ELF and loaded separately into EE memory. The worker then returns to the Manager via `LoadELFFromFile()`, passing the download counts and status as arguments. It is not simply a TLS function running inside the ordinary game-selection UI.

For the experimental USB build:
- Game ISOs are read from Sony's `dvr_pfs0:/ISO` **without modification**.
- **USB 1.2 is independent of any HDD application partition for network setup:** HTTPS reads `mass0:/DESR_DVR/V12_NET.CNF` and the CA bundle from `mass0:/DESR_DVR/N4E.CA`. The legacy `NET_PROBE.CNF` remains solely for the PC ART Server.
- Newly verified artwork goes into `mass0:/DESR_DVR/ART/`.
- **Planned HDD 1.2:** store the same `N4E.CA` inside the existing `hdd0:PP.DVRISO` application partition (accessible as `pfs1:/N4E.CA` once mounted). This is a design target, **not an implemented HDD 1.2 build**.
- Each run processes at most **50** missing covers (`BATCH50`); previously saved artwork is detected and not downloaded again.

### Obstacles and what we fixed

| Challenge | Diagnosis and engineering response |
| --- | --- |
| **No trustworthy RTC time** | The DESR's clock can be wrong when its backup battery is depleted. NTP connectivity alone does not authenticate time. We implemented a **signed Roughtime** verification step before checking TLS certificate validity. |
| **wolfSSL needs entropy** | PS2 lacks the conventional `/dev/urandom` service. Early approaches needed an entropy file generated on PC (`N6G50.ENT`). We **borrowed NULD's COP0 Count jitter method** and adapted it to generate per-request nonce/seed material on the DESR. It is experimental, not proven cryptographically strong. |
| **TLS handshake stalls and failures** | Incremental N3/N4/N5/N6 tests isolated DNS, UDP, TCP, TLS initialization, read/write callbacks, clock bounds and memory use. Earlier `-649` TLS and `-523` network-link errors were not bypassed; the working HTTPS path uses pinned wolfSSL configuration and bounded operations. |
| **Roughtime can reject a reply mid-batch** | A prototype returned `RET=-508` after a failed signed-response verification, interrupting an otherwise useful batch. The English-UI test retries missing or rejected Roughtime replies with distinct nonces, at most **three** attempts, and still fails if none verifies. |
| **Long download batches and EE memory pressure** | We built BATCH50 (maximum 50 covers per run), bounded response buffers, an **8 MiB TLS arena**, explicit per-session wolfSSL cleanup and narrow TLS allocation checks. |
| **Bad/partial files and possible data corruption** | PNG is checked and normalized in RAM before writing. `.PART` creation, USB readback verification and final rename protect completed covers. ISO contents and already existing artwork are preserved. |
| **Too much debugging output** | The initial diagnostic build's driver/network/TLS messages were replaced with an English progress UI (`PLEASE WAIT...`, per-cover result and a final Manager summary). Errors are still signaled; debug removal is not treated as a reliability fix. |
| **Intermittent green screen on return** | One 45-cover UI test completed the writes but hung during the ELF-to-Manager return. Inspection of the pinned PS2SDK loader found a diagnostic **green background set before `SifLoadElf()`**. This helps locate the stage but does **not** prove the underlying cause is fixed. Later hardware tests returned successfully. |

Development deliberately proceeded in small stages: network access and signed-time checks first, two-bound TLS, HTTPS GET into RAM, decoding and normalization, verified USB writes, then multi-cover batches and a usable interface. A successful TLS handshake was never assumed to imply that subsequent filesystem writes or application returns would work.

### Why use Tanner Ryan's Roughtime server?

**Problem:** HTTPS certificate validation requires a reasonable and trustworthy notion of time, which an aging Sony PSX does not necessarily possess. Merely contacting an NTP server does not authenticate the returned time.

We selected **[Tanner Ryan's Roughtime implementation and public service](https://github.com/tannerryan/roughtime)** at `time.txryan.com:2002`. The Manager sends a request with a nonce, then verifies a signed time interval against a **hardcoded/pinned Ed25519 public root key**. The strict verifier is limited to the supported draft-08 response and checks the signature, nonce and time interval. Invalid signatures and incorrect nonces cannot be silently accepted; this time is used only for certificate checks, not to change the hardware RTC.

The server documentation says that it intentionally **"greases" approximately 1% of replies by default**: some responses are deliberately malformed to exercise client validation. Our original worker could correctly **reject** such a response but then stopped the whole series. This is a plausible explanation for previously observed `-508` interruptions, **not proof** that every such interruption came from greasing.

The latest experimental branch uses a bounded retry policy:
1. Submit the original per-source nonce and verify the reply strictly.
2. For a missing, malformed or cryptographically invalid reply, derive another distinct nonce and request again.
3. Stop after **three total attempts**; never accept a failed signature, reuse the rejected response, disable peer validation or ignore an exhausted error.

Relevant code: [strict TXRyan verifier](dvr-iso-manager/src/net/v12_n3e_txryan.c), [wire parsing](dvr-iso-manager/src/net/v12_roughtime_wire.c), [retry policy](dvr-iso-manager/src/net/n6g_roughtime_retry.c).

**Security boundary:** Roughtime authenticates a signed statement of time from the pinned server; it is not a cure for weak local entropy or a complete production-grade trusted-clock policy. Current certificate-time bounds include a diagnostic margin. These caveats remain important before any final security claims.

### CPU entropy for TLS — directly borrowed from NULD

**We directly borrowed the CPU-entropy approach from Daniel's [NULD / former ORBIT](https://github.com/danielnuld/nuld).** Daniel's implementation showed how to work around missing PS2 `/dev/urandom` by using jitter from the MIPS CPU's **COP0 Count** register (`mfc0`). The [NULD `net.c` implementation we consulted](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c) wraps `open/read` and supplies the wolfSSL random input using repeated Count samples and variable timing. This idea was the decisive starting point for enabling an **on-console, no-PC entropy workflow** in PDIM 1.2.

We **adapted and reimplemented** the technique for the Sony DESR's worker rather than reproducing NULD's `/dev/urandom` wrapper verbatim:

- Read an initial launch Count and **96 COP0 Count samples**, rejecting a frozen sample sequence.
- Run the collected input through Monocypher **BLAKE2b** to derive separate inputs for up to 50 games and two artwork sources: one **32-byte Roughtime nonce** and **two 64-byte TLS seed inputs** for the low and high certificate-time-bound sessions.
- Derive distinct, domain-separated nonces for Roughtime retries without repeating the prior request.
- Wipe temporary seed material after use. The worker no longer needs the host-generated `N6G50.ENT` file.

See [`n6g_cpu_entropy.c`](dvr-iso-manager/src/net/n6g_cpu_entropy.c) and [`n6g_cpu_entropy.h`](dvr-iso-manager/include/n6g_cpu_entropy.h).

**Important distinction:** this solved the *practical integration issue* of feeding wolfSSL without a PC-generated file. It does **not** establish the quality of the entropy. CPU-counter samples can be predictable, and BLAKE2b cannot create missing randomness. The implementation is **experimental**, appropriate here only for retrieving *public cover images*; it must not be advertised as a certified RNG or reused for passwords, financial information or other sensitive TLS traffic without a trustworthy entropy source.

NULD's repository is [GPL-3.0 licensed](https://github.com/danielnuld/nuld/blob/main/LICENSE). The technical method and its source must be acknowledged. Before a public binary release, we should additionally **review whether any adapted/copied material triggers GPL license obligations**; credit alone is not a license compliance review.

### HTTPS trust, cover integrity and software architecture

The worker uses **wolfSSL/wolfCrypt** for TLS and the public **Mozilla CA bundle** (`N4E.CA`, obtained via curl's CA extraction). It checks certificate chains, hostname/SNI and certificate validity against both bounds of the signed Roughtime interval. It does **not** disable certificate verification to force a connection. The two HTTPS sessions use independent seed inputs and an 8 MiB EE allocation arena; wolfSSL contexts, sessions and sockets are released along their normal cleanup paths. Their cleanup is implemented, but a pair of successful long batches cannot establish the complete absence of leaks.

Only fixed, commit-pinned paths are requested from `raw.githubusercontent.com`. The primary [Luden02 archive of OPL Manager GameArt](https://github.com/Luden02/psx-ps2-opl-art-database) is queried first; [CosmicScale's PSBBN artwork database](https://github.com/CosmicScale/psbbn-art-database) is the fallback. The downloader validates the 11-character PS2 Game ID, rejects unsupported HTTP responses and redirects, and keeps the response in bounded RAM until PNG validation has passed. The image is normalized to **140 × 200 RGB**.

USB writing uses exclusive temporary `.PART` files, readback checking and renaming to the final `*_COV.png` name only after success. A failed or unavailable image cannot silently overwrite an existing cover, and no DLC, ISO or Sony DVR filesystem content is changed.

### Hardware results — Sony PSX DESR-7000 (17 November 2026)

| Test | Result | Return to Manager |
| --- | --- | --- |
| Baseline HTTPS **R4** | **45/45** missing covers in one run; Killzone later launched | **PASS** |
| First simplified Polish-UI test | Successful 45-cover run; another **45/45** download | An **intermittent green-screen hang** occurred on one return |
| Polish-UI regression with fewer covers | **20** covers downloaded | **PASS** |
| Latest **English UI + TXRyan retry** | **45/45**, followed by **32/32** in a separate test | **PASS both times**, no green screen |
| Legacy Windows ART Server / LAN HTTP | **15/15** covers | **PASS** |
| Host-side BATCH50 test | Limit of **50** verified against 60 simulated candidate games | Not a console run |

The earlier experimental build completed **77/77 HTTPS cover downloads across two hardware sessions with the previous HDD-hosted V12_NET.CNF layout** and kept the earlier **15/15 LAN HTTP** workflow working. This is strong practical evidence for that build, but **not proof** that the sporadic green screen has been permanently eliminated or that the entropy source is cryptographically secure.

Sources and test history: [R4 test details](docs/usb12-cpu-entropy.md), [older two-ELF prototype](docs/usb12-https-prototype.md), [English UI/retry test instructions](docs/usb12-ui-en-retry-test.md), [CI build and artifacts](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44082362681). The previous hardware-tested experimental Manager ELF was built with SHA-256 `26c4989e77de9b9f3feff6086807497f5f511c0c5bb634efc019e7b228010275` (this is a **test build hash**, not a promise about the eventual 1.2 release artifact).

### Using the experimental build

Back up your working main ELF, settings and artwork first. In the v1.2 USB complete package the primary executable is `DESR_DVR/DVR_ISO_MANAGER.ELF` without a suffix. For clean installation or careful upgrades use the [complete package instructions](docs/usb12-package.md). Place the current CA bundle **inside** `DESR_DVR/N4E.CA` (alongside the ELF), not at the USB root; this is a **new, not-yet-hardware-verified CA layout**. Copy your **existing working** `V12_NET.CNF` from `hdd0:PP.DVRISO12/` into `DESR_DVR/` on USB **once** (or fill in `DESR_DVR/V12_NET.CNF.example`, save it as `V12_NET.CNF`). Keep the originals and ART and the 1.0 HDD/XMB installation unchanged. The downloader will no longer mount PP.DVRISO12 for network configuration. Launch from **wLaunchELF-R3Z 4.80**. Press **Triangle** to scan missing covers, then **Triangle** to download via HTTPS (or **X** for the existing PC ART Server). The Manager summary stays visible until **X/O** is pressed.

The experimental CPU entropy build does **not** require `N6G50.ENT`. Do not assume that an unfinished or interrupted series has been saved completely: after restarting, rescan and check the remaining missing covers. Never perform live write/partition experiments on the Sony system partitions without a recoverable sector-level HDD backup.

### Credits, upstream work and licensing for 1.2

The following projects **made this work possible**. Their contributions are not original PDIM inventions:

| Project / contributor | Actual role in our implementation | Upstream / license |
| --- | --- | --- |
| **Daniel / NULD (formerly ORBIT)** | **Direct source of the CPU COP0 Count jitter entropy technique** used to get wolfSSL running without a host-generated entropy file. Adapted into DESR-specific sampling/seed derivation. | [NULD](https://github.com/danielnuld/nuld) · [`net.c`](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c) · GPL-3.0 |
| **Tanner Ryan / TXRyan** | Operates the public **Roughtime service** used for signed time; server's deliberate malformed replies motivated strict bounded retry handling. The client verifier is part of this project, not a claim to authorship of Tanner's server. | [tannerryan/roughtime](https://github.com/tannerryan/roughtime) · BSD-2-Clause |
| **wolfSSL Inc.** | **wolfSSL/wolfCrypt**, the core TLS and certificate-validation implementation. | [wolfSSL](https://github.com/wolfSSL/wolfssl) · GPLv3-or-later **or commercial license**, per upstream licensing terms |
| **Loup Vaillant and Monocypher contributors** | Monocypher **BLAKE2b and Ed25519** primitives used in entropy expansion and signed-response verification. | [Monocypher](https://github.com/LoupVaillant/Monocypher) · BSD-2-Clause / CC0 (refer to individual source notices) |
| **PS2DEV / PS2SDK contributors** | PS2 EE/IOP libraries, drivers, network stack, toolchain support and ELF-loading foundation. | [ps2sdk](https://github.com/ps2dev/ps2sdk) · AFL-2.0 and relevant component notices |
| **Mozilla / curl CA extraction** | Public **CA root certificate bundle** used for TLS validation, not custom certificates created by PDIM. | [curl CA extract](https://curl.se/docs/caextract.html) · [Mozilla MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/) |
| **OPL Manager and Luden02** | Primary **GameArt collection** and public archival dump used as a cover lookup source. Ownership of individual images remains with their respective rights holders. | [OPL Manager](https://oplmanager.com/) · [Luden02 dump](https://github.com/Luden02/psx-ps2-opl-art-database) |
| **CosmicScale** | Public **PSBBN Art Database** used as the second source when an OPL Manager archive cover is unavailable. | [CosmicScale/psbbn-art-database](https://github.com/CosmicScale/psbbn-art-database) |
| **ps2max32, pcm720 and Neutrino contributors** | **Neutrino game launching and DVR module support**, on which PDIM's earlier game-starting functionality builds. | [Neutrino](https://github.com/ps2max32/neutrino) · [pcm720 DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228) · AFL-3.0 |
| **wLaunchELF-R3Z and its contributors** | Practical launcher and DVR-capable filesystem tooling used in DESR hardware tests. | See corresponding upstream distribution and licensing notices |

**Additional thanks:** **Berion** for technical assistance; all upstream PS2/PSX developers and contributors; and the hardware testers who reported actual DESR-7000 behavior, including failed stages as well as successful downloads.

**Pre-release licensing checklist:** retain all applicable third-party notices in the release ZIP; review the GPL-3.0 provenance of any NULD-derived source and the GPL/commercial licensing of the linked wolfSSL binary; keep Monocypher, PS2SDK, Neutrino and CA-bundle notices as required; and verify artwork redistribution rights separately from the executable's open-source licenses. **Attribution is necessary, but is not by itself a substitute for license compliance.** Do not declare public 1.2 release-ready until those items and outstanding hardware-return reliability are reviewed.

---

## v1.0.0 — HDD + USB

**PSX DVR ISO Manager** lets you browse and launch PlayStation 2 ISO games from the DVR partition of a Sony PSX DESR console.

**HDD is the primary development variant, while USB remains supported.** Both versions launch PS2 ISO games stored on the Sony PSX DVR partition. The HDD version has been hardware-tested, including launching the Manager directly from XMB.

| Version | Sony PSX DESR-7000 testing | Installation |
| --- | --- | --- |
| **1.0 HDD** | **PASS:** 44/44 covers downloaded (40 + 4), two games launched including forced English; Manager launches from XMB and displays `1.0 HDD` | Existing APA/PFS partition `hdd0:PP.DVRISO`; `DVR_ISO_MANAGER_HDD.ELF` |
| **1.0 USB** | **PASS — tested on a Sony PSX DESR-7000 (November 7, 2026):** USB 1.0 runs on hardware. Earlier USB v0.8 tests separately confirmed 40/40 covers and REZ in English | FAT32 USB at `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` |

**Documentation:** [English release notes](docs/RELEASE_NOTES_v1.0.0_EN.md) · [Polski](docs/RELEASE_NOTES_v1.0.0_PL.md) · [HDD technical explanation](docs/HDD_1_0_TECHNICAL_EN.md) · [Polski](docs/HDD_1_0_TECHNICAL_PL.md) · [Release checklist](release-tools/v1_0/RELEASE_CHECKLIST.md).

**Safety:** The Manager never formats the drive or modifies game ISO files. The optional **APA header injection used for XMB integration** changes partition metadata; first make a backup that you can restore.

---

## Features

- **Source-code limit:** browse up to **128 ISO files** (`DESR_GAME_CAP = 128`) from the read-only `dvr_pfs0:/ISO` directory, 12 titles per page, without page wrapping. **A library of 44 ISO entries was used on real hardware; this is not a claim that all 44 games were individually boot-tested.**
- Detect **Game IDs** (for example `SLES_538.86`) from information inside ISO images and find artwork in the configured application `ART/` directory (on USB: `mass0:/DESR_DVR/ART`; on HDD: `PP.DVRISO/ART`).
- Show color PNG cover previews and maintain `COVERS.IDX`.
- Manually scan with **Triangle**, write `COVER_SCAN.LOG`, identify missing covers and optionally download them from a LAN HTTP server.
- Automatically rebuild the artwork index and return to the game list after downloading new images.
- Boot games via OSDMenu / Neutrino with `SYSTEM` or `ENGLISH` language selection.

## Controller

| Button | Function |
| --- | --- |
| **UP / DOWN** | Select previous / next game |
| **LEFT / RIGHT** | Switch between pages of 12 games |
| **X** | Launch the highlighted game from the normal list |
| **TRIANGLE (△)** | Rescan ISO files, detect Game IDs and refresh the cover index |
| **X / CIRCLE (O)** on the prompt | Download missing covers / skip downloading |
| **SQUARE (□)** | Write the test configuration to `DVR_TEST.CNF` |
| **SELECT** | Switch between `SYSTEM` and `ENGLISH` |
| **START** | Exit the Manager |

# PSX DVR ISO MANAGER 1.0 HDD — launch and user guide

The **1.0 HDD** version launches PlayStation 2 games stored on the Sony PSX DESR DVR partition without needing a USB drive while the Manager is running.

The application itself, Neutrino configuration/modules and game artwork are stored on a separate HDD application partition named **`PP.DVRISO`**.

This version has been successfully tested on a physical **Sony PSX DESR-7000**.

## 1. Requirements

You need:

- A Sony PSX DESR console with a functioning internal HDD.
- **wLaunchELF-R3Z**, supporting DVR and PFS partitions.
- An existing application partition `hdd0:PP.DVRISO`.
- The complete **PSX DVR ISO MANAGER 1.0 HDD** package.
- PS2 ISO images already stored in `dvr_pfs0:/ISO/`.

You need a USB drive or UDPFS to transfer the files and start wLaunchELF, but once the partition is prepared, **the HDD Manager does not use USB to store its own data**.

> **Important:** `PP.DVRISO` must already exist and be correctly formatted as PFS. The Manager does not create it automatically and never formats Sony system partitions.

---

## 2. HDD file layout

After extracting the HDD package, locate the **`PP.DVRISO-FILES`** folder.

**Copy its contents directly to the root of the `PP.DVRISO` partition.**

The resulting structure is approximately:

```text
hdd0:PP.DVRISO/
│
├── DVR_ISO_MANAGER_HDD.ELF     # Manager 1.0 HDD
│
├── BOOT.ELF                    # OSDMenu Launcher
├── BOOT.CNF                    # Generated automatically
│
├── neutrino_hdd.elf           # Neutrino — SYSTEM
├── neutrino_hdd_lang.elf      # Neutrino — ENGLISH
│
├── NET_PROBE.CNF              # LAN configuration
│
├── ART/                       # Game covers
│   ├── SLES_XXXXX_COV.png
│   └── SLUS_XXXXX_COV.png
│
├── COVERS.IDX                 # Generated automatically
├── COVER_SCAN.LOG             # Generated by scanning
│
├── config/                    # Neutrino settings
│   ├── system.toml
│   ├── bsd-dvr.toml
│   ├── osdlang.toml
│   └── ...                    # Other TOML files
│
└── modules/                   # Neutrino / DVR modules
    ├── ee_core.elf
    ├── ee_core_lang.elf
    ├── dvrfile_fhi.irx
    ├── dvrdrv.irx
    ├── dvrfile.irx
    └── ...                    # Remaining bundled modules
```

*This is a simplified layout. Keep every file and subdirectory shipped inside `config/` and `modules/`.*

### Important files

| File or folder | Purpose |
| --- | --- |
| `DVR_ISO_MANAGER_HDD.ELF` | Main Manager application |
| `BOOT.ELF` | OSDMenu Launcher used to start Neutrino |
| `BOOT.CNF` | Selected-game configuration, created and updated automatically |
| `neutrino_hdd.elf` | Launch games with SYSTEM language |
| `neutrino_hdd_lang.elf` | Launch games with forced ENGLISH |
| `NET_PROBE.CNF` | PC IP, console IP and artwork-server port |
| `ART/` | Downloaded PNG game covers |
| `COVERS.IDX` | Artwork index |
| `COVER_SCAN.LOG` | ISO scanning report |
| `config/` | Neutrino TOML settings |
| `modules/` | Neutrino / DVR dependencies |

> **Note:** A new installation does not include `BOOT.CNF`. The Manager creates it when you launch your first game with **X**; there is no need to write it manually.

**Do not copy `PP.DVRISO-FILES` itself as an additional folder on HDD.** Its contents must be at the partition root.

---

## 3. Where to store game ISOs

Game images are **not** stored on `PP.DVRISO`.

The Manager uses the original Sony PSX DVR filesystem:

```text
dvr_pfs0:/ISO/
│
├── ICO.iso
├── MGS 2.ISO
├── REZ.iso
├── Metal Slug Anthology.iso
└── ...
```

The `PP.DVRISO` partition is reserved for the application, its configuration and game artwork.

**The Manager reads game ISOs from the DVR partition without modifying their contents.**

---

## 4. Copying files to HDD

1. Extract the complete **HDD 1.0** package on your computer.
2. Copy `PP.DVRISO-FILES` to a FAT32 USB drive or make the files available via UDPFS.
3. Launch **wLaunchELF-R3Z** on the console.
4. Open the existing `PP.DVRISO` partition using wLaunchELF's HDD/PFS support.
5. Copy **all the contents** of `PP.DVRISO-FILES` to the root of this partition.
6. Verify that `DVR_ISO_MANAGER_HDD.ELF` and the `config/`, `modules/` and `ART/` folders are in the correct locations.

> **Upgrading:** Make a backup before replacing existing files. Preserve your `ART/` covers, `NET_PROBE.CNF`, `BOOT.CNF` and cover indexes.

---

## 5. Starting the Manager from HDD

After copying the files:

1. Launch **wLaunchELF-R3Z**.
2. Go to its HDD/PFS file browser.
3. Open or mount **`PP.DVRISO`**.
4. Find **`DVR_ISO_MANAGER_HDD.ELF`** at the partition root.
5. Execute that file.

If the partition is mounted at `pfs0:`, the path looks like:

```text
pfs0:/DVR_ISO_MANAGER_HDD.ELF
```

After successful startup you will see:

**PSX DVR ISO MANAGER — 1.0 HDD** and the list of ISO games found in the DVR directory.

---

<details>
<summary><strong>Install PDIM as an application directly in XMB — click to expand</strong></summary>

# Install PSX DVR ISO MANAGER 1.0 HDD in XMB

PDIM 1.0 HDD can also appear as an application in the **Games** section of the original Sony PSX DESR XMB.

It uses its own PDIM icon representing an HDD and a DVD disc and appears under the name **PSX DVR ISO MANAGER**.

This procedure has been tested on a **Sony PSX DESR-7000** using **wLaunchELF-R3Z 4.80**.

## 1. Requirements

You need:

- A Sony PSX DESR console with a working HDD.
- **wLaunchELF-R3Z 4.80**.
- A FAT32 USB drive.
- The complete **PSX DVR ISO MANAGER 1.0 HDD** package.
- The **XMB-PDIM-INSTALL** package containing the APA header files, icon and launcher files.
- Free space on the HDD.

**Warning:** Create a recoverable backup before you begin. Creating a partition and injecting its APA header modify on-disk structures.

Never format the entire Sony PSX HDD or delete its original system partitions.

---

## 2. Preparing USB

Extract the HDD and XMB packages on your computer.

Prepare your USB drive with this layout:

```text
USB:/
│
├── __Headers/
│   └── PP.DVRISO/
│       ├── system.cnf
│       ├── icon.sys
│       ├── info.sys
│       ├── jkt_001.png
│       └── jkt_002.png
│
├── PFS_COPY_TO_PP.DVRISO/
│   ├── EXECUTE.ELF
│   └── EXECUTE.CNF
│
└── PP.DVRISO-FILES/
    ├── DVR_ISO_MANAGER_HDD.ELF
    ├── BOOT.ELF
    ├── neutrino_hdd.elf
    ├── neutrino_hdd_lang.elf
    ├── NET_PROBE.CNF
    ├── ART/
    ├── config/
    └── modules/
```

This is a simplified example; retain all other files included in the HDD package.

**Important:** `__Headers` must be directly in the USB drive's root. wLaunchELF-R3Z reads the header files from there.

### XMB installation files

| File | Purpose |
| --- | --- |
| `system.cnf` | XMB boot configuration written into the APA header |
| `icon.sys` | Icon metadata |
| `info.sys` | Application name and metadata |
| `jkt_001.png` | 256 × 256 PSBBN artwork |
| `jkt_002.png` | 76 × 108 PSX XMB icon |
| `EXECUTE.ELF` | KELF container to start the launcher |
| `EXECUTE.CNF` | Configuration forwarding startup to PDIM |

---

## 3. Creating the PP.DVRISO partition

Start **wLaunchELF-R3Z 4.80**.

1. Open **MISC → HDDManager**.
2. Display the partitions on `hdd0:`.
3. Check whether `PP.DVRISO` already exists.
4. If missing, open the operations menu and choose **Create partition**.
5. Enter the exact name: **`PP.DVRISO`**.
6. Choose a sample capacity of **256 MB**.
7. Confirm creation.
8. Ensure it appears in the partition list with type **PFS**.

Example:

```text
hdd0:
└── PP.DVRISO        PFS        256 MB
```

**Why 256 MB?** It is enough for the Manager, Neutrino, settings and many PNG covers, with extra free space. You may choose a larger partition for a bigger artwork library.

> **Note:** Use exactly `PP.DVRISO`, not `PP.TESTDVR` or `+DVRISO`, because our HDD 1.0 binaries reference that specific partition.
>
> If the partition already contains a working installation, **do not recreate or format it**. If a newly created partition cannot be mounted as PFS, inspect its type and state instead of formatting the entire drive.

---

## 4. Injecting the APA header

After creating the application partition, register it for XMB display:

1. Stay in **HDDManager** in wLaunchELF-R3Z 4.80.
2. Highlight **`PP.DVRISO`**.
3. Open the **R1** operations menu.
4. Select **Inject Header**.
5. Choose the USB drive as the source.
6. Select **This partition only**.
7. Confirm the injection.

The header injector reads:

```text
USB:/__Headers/PP.DVRISO/
```

`system.cnf` and the relevant metadata are written into the APA header. `info.sys` and the XMB artwork are copied into the application's PFS `res/` directory.

**Do not select** `Matching partitions` or `Matching and create missing partitions`. Only `PP.DVRISO` should be changed.

---

## 5. Copying the Manager files to HDD

Return to wLaunchELF-R3Z **FileBrowser**.

Open `hdd0` and then the newly created `PP.DVRISO` partition.

### Step 1 — XMB boot files

Copy the contents of:

```text
USB:/PFS_COPY_TO_PP.DVRISO/
```

to the root of `PP.DVRISO`:

```text
EXECUTE.ELF
EXECUTE.CNF
```

These are required to launch PDIM from XMB.

### Step 2 — HDD Manager files

Copy **the contents** of:

```text
USB:/PP.DVRISO-FILES/
```

directly into the root of `PP.DVRISO`. Do not create an additional `PP.DVRISO-FILES` folder on the partition.

### Final directory layout

```text
hdd0:PP.DVRISO/
│
├── EXECUTE.ELF                # XMB boot (KELF)
├── EXECUTE.CNF                # XMB launcher configuration
│
├── DVR_ISO_MANAGER_HDD.ELF    # PDIM 1.0 HDD
├── BOOT.ELF                   # OSDMenu Launcher
├── BOOT.CNF                   # Created when a game is selected
│
├── neutrino_hdd.elf          # Neutrino SYSTEM
├── neutrino_hdd_lang.elf     # Neutrino ENGLISH
│
├── NET_PROBE.CNF             # LAN configuration
│
├── ART/                      # Game covers
│
├── config/                   # Neutrino configuration
│   ├── system.toml
│   ├── bsd-dvr.toml
│   ├── osdlang.toml
│   └── ...
│
├── modules/                  # Neutrino / DVR modules
│   ├── ee_core.elf
│   ├── ee_core_lang.elf
│   ├── dvrfile_fhi.irx
│   ├── dvrdrv.irx
│   ├── dvrfile.irx
│   └── ...
│
└── res/                      # XMB artwork / metadata
    ├── info.sys
    ├── jkt_001.png
    └── jkt_002.png
```

*The folder tree is abbreviated. Keep all other files in `config/` and `modules/`.*

`BOOT.CNF` does not need to exist immediately after installation; the Manager generates it when you select a game for the first time.

If `res/` is missing its icons or `info.sys`, check the header injection outcome before continuing.

---

## 6. Launching PDIM from XMB

1. Exit wLaunchELF-R3Z.
2. Power off the Sony PSX DESR.
3. Boot back into the original XMB.
4. Open the Games section.

You should see **PSX DVR ISO MANAGER**, with the PDIM HDD-and-DVD icon.

<img width="4096" height="3072" alt="PDIM application in Sony PSX XMB" src="https://github.com/user-attachments/assets/9792cf70-ee0a-46c1-985b-9811784e0549" />

Select it to start the application.

<img width="4096" height="3072" alt="PSX DVR ISO MANAGER 1.0 HDD launched from XMB" src="https://github.com/user-attachments/assets/6b447da9-68c9-43ab-8ea9-3798e632f83b" />

The Manager should display **PSX DVR ISO MANAGER — 1.0 HDD** and scan:

```text
dvr_pfs0:/ISO/
```

Highlight a game and press **X** to launch it.

---

## 7. How XMB launching works

```text
Sony PSX XMB
    │
    ▼
PP.DVRISO (APA header)
    │
    ▼
EXECUTE.ELF (KELF)
    │
    ▼
EXECUTE.CNF
    │
    ▼
DVR_ISO_MANAGER_HDD.ELF
    │
    ▼
PSX DVR ISO MANAGER 1.0 HDD
    │
    ▼
BOOT.CNF + BOOT.ELF
    │
    ▼
Neutrino HDD FIX95
    │
    ▼
dvr_pfs0:/ISO/ — PS2 game
```

This allows the Manager to be started directly from XMB without opening wLaunchELF every time. No USB drive is needed for normal operation after installation.

---

## 8. Updating PDIM

Once the XMB entry works, future updates are simpler. Replace only:

```text
hdd0:PP.DVRISO/
└── DVR_ISO_MANAGER_HDD.ELF
```

**Do not recreate the partition or repeat Inject Header.**

Make a backup of the previous ELF, settings, modules and downloaded covers before upgrading.

---

## 9. Safety notes

- A functioning APA/PFS disk is required.
- Do not format the entire Sony PSX drive.
- Create or modify only your own `PP.DVRISO` partition.
- Choose **This partition only**.
- Game ISOs stay in `dvr_pfs0:/ISO/` and are not moved to the application partition.
- `EXECUTE.ELF` and `EXECUTE.CNF` are necessary for XMB booting.
- Keep both Neutrino versions and the complete `config/` / `modules/` directories.
- Removing `EXECUTE.ELF` alone does not restore the previous APA header.
- XMB integration has been verified on a Sony PSX DESR-7000; other models may require additional testing.

**Installation complete!** PSX DVR ISO MANAGER is now available directly from XMB.

</details>

<details>
<summary><strong>Install and use PDIM from USB — click to expand</strong></summary>

## Launching PSX DVR ISO MANAGER

PDIM also launches PS2 games from the DVR partition using a USB drive, without creating a separate HDD application partition.

**Download the complete USB package:**

[**PSX DVR ISO MANAGER v0.8 USB — DOWNLOAD**](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/v0.8/PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE.zip)

> **Note:** This link refers to the previously published USB v0.8. USB 1.0 has also been tested on the DESR-7000 and uses the same installation layout. For 1.0, download the USB package from the **1.0 release**.

The package already contains the OSDMenu/Neutrino executables, required modules and configuration files. You do not need to install earlier versions separately.

### 1. Preparing USB

1. Format a USB drive as **FAT32**.
2. Download and extract the complete USB package on your computer.
3. Locate the inner **`DESR_DVR`** folder.
4. Copy the whole `DESR_DVR` folder to the root of your USB drive.
5. Connect the USB drive to your Sony PSX DESR.

Correct application path:

`mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`

Do not copy the outer ZIP directory to USB.

### 2. USB directory layout

```text
mass0:/DESR_DVR/
│
├── DVR_ISO_MANAGER.ELF       # USB Manager 0.8 / 1.0
│
├── BOOT.ELF                  # OSDMenu Launcher
├── BOOT.CNF                  # Game launch settings
│
├── neutrino.elf              # Neutrino SYSTEM
├── neutrino_lang.elf         # Neutrino ENGLISH
│
├── NET_PROBE.CNF             # LAN configuration
│
├── ART/                      # Game artwork
│   └── SLES_538.86_COV.png   # Example Game ID cover
│
├── COVERS.IDX                # Cover index
├── COVER_SCAN.LOG            # ISO scan report
│
├── config/                   # Neutrino settings
│   ├── system.toml
│   ├── osdlang.toml
│   └── ...                   # Other configurations
│
└── modules/                  # Neutrino / DVR modules
    ├── ee_core.elf
    ├── ee_core_lang.elf
    ├── dvrfile_fhi.irx
    └── ...                   # Remaining modules
```

*This is a simplified list. Do not remove any other files from `config/` or `modules/`.*

`COVERS.IDX` and `COVER_SCAN.LOG` are created automatically on the first scan and may not exist in a fresh archive.

### 3. Starting the Manager

1. Start **wLaunchELF-R3Z**.
2. Open the file browser.
3. Navigate to USB (`mass0:`).
4. Open `DESR_DVR`.
5. Execute **`DVR_ISO_MANAGER.ELF`**.

You may also start the ELF through UDPFS, but `DESR_DVR` and its configuration/artwork must still reside on USB.

The Manager will display the ISO game list from the DVR partition.

</details>

## Downloading covers from a Windows PC

The companion **PSX DVR ART Server v0.8** is a Windows `.exe` application (Python sources are also available at [pc-art-server](pc-art-server/README.md)). The PC retrieves cover images using HTTPS, while the PSX communicates with it over local HTTP.

1. Download **`PSX-DVR-ART-Server-v0.8.exe`** from the [v0.8 release](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/v0.8) and run it on your Windows PC.
2. Enter **PC IPv4 Address**, **Console IPv4 Address** and **HTTP Port**. Select **Start HTTP Server**. The example addresses below are from a tested DESR-7000 network; use your own settings.
3. On USB, create or edit `mass0:/DESR_DVR/NET_PROBE.CNF`. For HDD, the matching file resides in the `PP.DVRISO` root:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

   **How to find the settings:** Run `ipconfig /all` in Windows CMD or PowerShell, and check the interface connected to your PSX network.

   | Setting | Value |
   | --- | --- |
   | `IP` | **The PSX console's static IPv4 address**; choose an unused address on the PC's subnet. It is **not** the PC address. |
   | `NETMASK` | The interface's **Subnet Mask**. |
   | `GATEWAY` | The network's **Default Gateway**, usually the router. |
   | `SERVER_IP` | The **IPv4 address of the Windows PC** running the ART Server. |
   | `SERVER_PORT` | The port configured in the Windows program (default `8000`). |

   **Important:** `IP` and `SERVER_IP` must be different, unused addresses. If the computer's IP changes, update the settings.

4. In the Manager press **TRIANGLE** to scan. If artwork is missing, the prompt will offer a download.
5. **X: DOWNLOAD** downloads the missing images; **O: SKIP** cancels. The PC caches the images and sends them to the console, where the Manager writes only new PNGs and rebuilds the index.

**Artwork sources:** Primary — **OPL Manager Archive (Archive.org)**, generally providing vertical PS2 cover art. Fallback — **CosmicScale PSBBN Art Database (GitHub)**, which may provide square images. The fallback is queried only when the primary returns no valid PNG. Square artwork is fitted into a 140×200 layout with black padding, without distortion. If no valid cover exists, the PC reports it as missing.

PC cache: `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`. See the [Windows ART Server documentation](pc-art-server/README.md) for further details and tests.

**Security:** Use the server on a trusted private LAN without forwarding its port. Back up a working Manager ELF, `BOOT.CNF` and `ART/` before updates. New artwork is validated before saving; existing artwork is not overwritten.

## Photos and screenshots

The following screenshots show the current v1.0 interface and the XMB entry:

<img width="4096" height="3072" alt="PDIM application in Sony PSX XMB" src="https://github.com/user-attachments/assets/3ed2f6ea-2c6d-4011-bd5c-3612ea6c18ce" />
<img width="4096" height="3072" alt="PSX DVR ISO MANAGER 1.0 HDD on DESR-7000" src="https://github.com/user-attachments/assets/452a6dfe-00df-40a4-a4c7-4738eb14d242" />
<img width="4096" height="2710" alt="Manager game list" src="https://github.com/user-attachments/assets/4e095df8-3283-43eb-ac6a-e98bd06d5f8f" />
<img width="756" height="563" alt="Windows ART Server and cover synchronization" src="https://github.com/user-attachments/assets/ff72eb3a-a41e-42b3-995e-bfb14f32d7a0" />

## Building

This repository includes the CMake/PS2SDK build, integrated ISO scanning, LAN and PNG tests, and the PyInstaller configuration for the Windows executable. The former standalone Probe has been retired. See [dvr-iso-manager](dvr-iso-manager/README_PL.md) and [pc-art-server](pc-art-server/README.md) for more information.

## Credits

- **[Daniel / NULD (formerly ORBIT)](https://github.com/danielnuld/nuld)** — direct source of the COP0 Count CPU-entropy technique adapted for the HTTPS 1.2 worker (see the detailed [1.2 credits and licensing section](#credits-upstream-work-and-licensing-for-12)).
- **[Tanner Ryan / TXRyan Roughtime](https://github.com/tannerryan/roughtime)** — public signed-time service.
- **[wolfSSL](https://github.com/wolfSSL/wolfssl), [Monocypher](https://github.com/LoupVaillant/Monocypher), [PS2SDK](https://github.com/ps2dev/ps2sdk)** — TLS, cryptography, EE/IOP infrastructure.
- **[Neutrino](https://github.com/ps2max32/neutrino)** and **[pcm720's DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228)** — PS2 game loader and DVR integration.
- **Berion** — technical assistance.
- **[CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database)** — fallback artwork; **[OPL Manager](https://oplmanager.com/) / [Luden02 archive](https://github.com/Luden02/psx-ps2-opl-art-database)** — primary artwork.
- **[Mozilla/curl CA bundle](https://curl.se/docs/caextract.html)** and the PS2 homebrew/toolchain community.

See the [technical 1.2 credits](#credits-upstream-work-and-licensing-for-12) for precisely what was borrowed, attribution and pre-release license obligations.
