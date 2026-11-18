# PSX DVR ISO MANAGER 1.2 — HTTPS technical guide

**Status:** Version 1.2 has been [published](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2) and functionally tested on a **Sony PSX DESR-7000**. This document describes the architecture and its remaining limitations. It does **not** claim certified randomness or formally proven TLS security.

[Project README](../README.md) · [Polish README](../ReadmePL.md) · [HDD installation](../hdd-v1.2/README.md) · [USB installation](usb12-package.md)

## Why HTTPS on the Sony PSX?

The Sony PSX DESR can start PS2 ISO games from its native DVR filesystem. Earlier PDIM versions could fetch missing covers from the **PSX DVR ART Server on Windows**, which downloaded images over HTTPS and passed them to the console over LAN HTTP. Version 1.2 adds a **console-native HTTPS path** so the PC is optional.

The console scans `dvr_pfs0:/ISO/` read-only, extracts Game IDs, checks whether ART already contains valid covers and downloads only missing images from the pinned OPL Manager artwork source, falling back to CosmicScale's PSBBN art database. The downloaded PNG data is checked, normalized and saved using a temporary file, readback verification and a final rename. The maximum batch is **50 missing covers per run**.

| Component | HDD 1.2 | USB 1.2 |
| --- | --- | --- |
| Main Manager | `PP.DVRISO/DVR_ISO_MANAGER_HDD.ELF` | `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` |
| Network settings | `PP.DVRISO/V12_NET.CNF` | `mass0:/DESR_DVR/V12_NET.CNF` |
| Trusted CA store | `PP.DVRISO/N4E.CA` | `mass0:/DESR_DVR/N4E.CA` |
| Cover directory | `PP.DVRISO/ART/` | `mass0:/DESR_DVR/ART/` |
| Game ISO source | `dvr_pfs0:/ISO/` (read-only) | `dvr_pfs0:/ISO/` (read-only) |

`NET_PROBE.CNF` belongs to the separate **Windows PC / LAN HTTP** mode. `V12_NET.CNF` is used by the native HTTPS worker.

## 1. The problem: no dependable hardware clock

The DESR backup battery/RTC may have failed or have an incorrect date. HTTPS certificate validation depends on time: certificate `notBefore` and `notAfter` checks are not meaningful without a plausible time source. Plain NTP was successfully tested during development, but a basic NTP reply is **not authenticated**.

### Why TXRyan Roughtime?

The 1.2 worker requests signed time from **Tanner Ryan's public Roughtime server, `time.txryan.com:2002` over UDP**. It validates the reply against a **hardcoded Ed25519 public root key**, verifying the signature, request nonce and signed interval. The implementation accepts only the supported Roughtime draft-08 format, uses bounded parsing and keeps the time result in memory **without modifying the PSX hardware clock**.

TXRyan is a **network time service**, not an ELF, library or binary shipped inside PDIM. [TXRyan Roughtime source and documentation](https://github.com/tannerryan/roughtime).

The server may intentionally send malformed responses as a protocol robustness test. Consequently PDIM retries rejected/missing replies **up to three times**, deriving a distinct request nonce on each retry. Failure to verify the reply is never treated as a successful time check. This authenticates the server's signed time statement; it does not solve weak client randomness or establish an independent trusted-clock system.

Source references: [TXRyan strict verifier](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/pdim_usb_v1.2/dvr-iso-manager/src/net/v12_n3e_txryan.c), [Roughtime packet parser](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/pdim_usb_v1.2/dvr-iso-manager/src/net/v12_roughtime_wire.c), [retry policy](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/pdim_usb_v1.2/dvr-iso-manager/src/net/n6g_roughtime_retry.c).

## 2. The TLS implementation: wolfSSL

**[wolfSSL / wolfCrypt](https://github.com/wolfSSL/wolfssl)** handles TLS sessions, cryptographic primitives for HTTPS, CA-chain validation, SNI/hostname checking and certificate date checks using signed Roughtime time bounds. The bundled `N4E.CA` is a public Mozilla root certificate bundle obtained through curl's CA extract, **not a custom certificate invented by PDIM**.

The PS2 has unusually limited memory and a nonstandard runtime. The worker uses a bounded TLS memory arena, explicitly cleans up sessions and sockets and handles network I/O conservatively. Earlier network/TLS diagnostic stages exposed stalls, time-bound and transport failures; the working build does **not** solve them by switching off certificate validation.

The upstream wolfSSL revision is pinned in the build workflow to `decea12e223869c8f8f3ab5a53dc90b69f436eb2`. This revision is distributed under **GPLv3-or-later or a commercial license**, according to [upstream LICENSING](https://github.com/wolfSSL/wolfssl/blob/decea12e223869c8f8f3ab5a53dc90b69f436eb2/LICENSING). Retain the relevant notices and corresponding-source information when redistributing the binary.

## 3. The entropy problem and what we borrowed from NULD

On conventional systems, cryptographic libraries typically obtain random bytes from an operating-system source such as `/dev/urandom`. The PS2 environment does not provide an equivalent trustworthy service out of the box. Early PDIM prototypes therefore needed a **PC-generated entropy file**. To remove that dependency we studied **Daniel's NULD (formerly ORBIT)**, which implements a PS2 HTTPS workflow and samples jitter from the MIPS CPU **COP0 Count** register using `mfc0`.

**Credit:** We directly borrowed the *COP0 Count jitter sampling idea* from [NULD](https://github.com/danielnuld/nuld), especially its [`net.c` implementation at commit `2951dc9`](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c). PDIM reimplements the sampling and mixing for its own worker; it does **not** copy NULD's exact `/dev/urandom` wrapper verbatim.

PDIM's implementation:
1. Samples an initial CPU Count value and **96 subsequent Count readings**; rejects a frozen sequence.
2. Uses **Monocypher BLAKE2b** to derive distinct, labeled inputs for each of up to **50** games and each of **two artwork sources**.
3. Supplies an individual Roughtime nonce and separate low/high certificate-bound TLS seed material for each request.
4. Derives distinct retry nonces and wipes temporary key material.

Source: [`n6g_cpu_entropy.c`](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/pdim_usb_v1.2/dvr-iso-manager/src/net/n6g_cpu_entropy.c) · [header](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/pdim_usb_v1.2/dvr-iso-manager/include/n6g_cpu_entropy.h).

**Security limitation:** hashing unpredictable-looking CPU values does **not** create entropy if the original values are predictable. COP0 Count samples may be weak, especially under repeatable boot conditions. The current method is **experimental**, not a certified secure RNG, and must not be described as appropriate for passwords, financial information or other sensitive TLS traffic.

**Licensing/provenance:** NULD is published under **GPL-3.0**, and the adopted technique is expressly attributed to Daniel. wolfSSL uses GPLv3-or-later/commercial dual licensing, while PS2SDK, Neutrino and other components have their own notices. Attribution is necessary but not a substitute for satisfying applicable distribution terms.

## 4. Application behavior and observed results

The native HTTPS worker is embedded in the Manager ELF as a separately loadable payload, temporarily executes to fetch art, and returns to the Manager with a result summary. The user presses **TRIANGLE** on the game list, then **TRIANGLE** on the download prompt (or **X** for the Windows LAN HTTP method). The final screen reports downloaded and skipped covers; **X/O** returns to the game list.

Hardware results:
- **USB 1.2 R4:** 45/45 missing covers downloaded in one session; return and game start confirmed.
- **USB 1.2 English UI / Roughtime retry:** 45/45 and 32/32 in separate hardware tests; successful return.
- **HDD 1.2:** HTTPS cover retrieval, game launch, SYSTEM/ENGLISH, XMB startup and installer idempotence confirmed on DESR-7000, using **`PP.DVRISO`** as application storage.
- A **sporadic green-screen hang** was seen in an older experimental USB return path. Later tests passed, but the earlier root cause was not conclusively established.
- Tests demonstrate functionality under the tested conditions, **not a guarantee of security, crash durability, exhaustive compatibility or future server uptime**.

## 5. Credits and relevant upstream sources

| Project | Contribution |
| --- | --- |
| [Tanner Ryan / Roughtime](https://github.com/tannerryan/roughtime) | Public signed Roughtime service used for time-bound certificate checks |
| [wolfSSL / wolfCrypt](https://github.com/wolfSSL/wolfssl) | HTTPS TLS implementation and certificate checking |
| [Daniel's NULD](https://github.com/danielnuld/nuld) | Original inspiration and direct source for the CPU COP0 Count jitter entropy approach |
| [Monocypher](https://github.com/LoupVaillant/Monocypher) | Ed25519 and BLAKE2b primitives for signed replies and seed derivation |
| [PS2SDK](https://github.com/ps2dev/ps2sdk) | PS2 networking, EE/IOP modules and toolchain |
| [ps2max32 / pcm720 Neutrino](https://github.com/ps2max32/neutrino) | PS2 game launch, DVR FHI modules and English-language configuration |
| [Luden02 OPL art archive](https://github.com/Luden02/psx-ps2-opl-art-database) | Primary cover source |
| [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) | Fallback cover source |
| [curl CA extract](https://curl.se/docs/caextract.html) | Mozilla certificate authority bundle |

**Version 1.2 is published. This technical guide is maintained separately from the main installation instructions.**
