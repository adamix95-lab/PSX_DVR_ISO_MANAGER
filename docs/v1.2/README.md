# PSX DVR ISO MANAGER 1.2 — experimental development (DESR-7000)

> **PUBLIC DEVELOPMENT BRANCH. NOT A RELEASE.**
>
> The `1.0` release and `main` remain unchanged. **Native HTTPS is NOT implemented yet.**
> The current experimental ELF is **READ-ONLY**: no game launching, no `BOOT.CNF` writes, no ART downloads or RW mounts. This protects 1.0 while the v1.2 Neutrino and TLS paths are verified.

## Partition isolation

| Purpose | APA/PFS partition | Notes |
| --- | --- | --- |
| Stable 1.0 HDD | `hdd0:PP.DVRISO` | **DO NOT MODIFY** |
| Experimental 1.2 HDD | `hdd0:PP.DVRISO12` | Separate application, `ART/`, configuration and Neutrino files |
| Existing DVR game ISOs | `dvr_pfs0:/ISO/` | Always read-only; **not** moved or reformatted |

The experimental build `DVR_ISO_MANAGER_V12_HDD.ELF` passes
`hdd0:PP.DVRISO12/BOOT.CNF` to OSDMenu and mounts only
`hdd0:PP.DVRISO12` as its **application** filesystem (`pfs1:/`).
Cover images, index and network configuration therefore live inside the
experimental partition. It reuses the **read-only** DVR ISO listing.

The Manager **does not create, resize, format or inject APA headers**.
An absent `PP.DVRISO12` must cause a read-only mount failure.

## Creating the test partition on the console — later, manually

1. Make a restorable backup of the APA partition table/header and all critical data.
2. Check free space on the PS2 APA side. In wLaunchELF_R3Z HDDManager, create
   **only** a new PFS application partition named exactly `PP.DVRISO12`
   (suggested size: 256 MB; choose a larger size only if needed).
3. **Do not touch `PP.DVRISO` or Sony's DVR partitions.** Never format or
   expand the entire disk. Never choose a bulk partition operation.
4. For the **read-only v1.2 preview**, only copy test-specific files as
   needed. Do **not** copy the HDD 1.0 Neutrino binaries into this partition:
   their FIX95 wrapper is hard-coded to mount the production `PP.DVRISO`.
   Separate v1.2 SYSTEM/ENGLISH Neutrino builds are provided as experimental
   CI artifacts, not yet approved for live game launching. Never copy a
   production `BOOT.CNF` or inject an XMB header.
5. Compile the experimental v1.2 binary and launch it **manually through
   wLE**, selecting `DVR_ISO_MANAGER_V12_HDD.ELF`. This ELF is **strictly
   read-only** and blocks both Quickboot and all configuration/ART writes.
6. Do **NOT** use the existing `1.0` XMB installer, `__Headers/PP.DVRISO`,
   `EXECUTE.ELF` or `EXECUTE.CNF` for v1.2. The XMB entry continues to
   launch stable 1.0 from `PP.DVRISO`.

**No actual console partition has been created by this GitHub change.**

## Compiling the isolated test ELF

Requires the PS2SDK **patched with the tested HDD NETFIX**, as described in
[`hdd-netfix/`](../../hdd-netfix/README_PL.md).

```sh
cmake -S dvr-iso-manager -B build-v12-hdd -DDESR_BUILD_HDD_NETFIX=ON
cmake --build build-v12-hdd --target DVR_ISO_MANAGER_V12_HDD.ELF -j2
```

Output: `build-v12-hdd/DVR_ISO_MANAGER_V12_HDD.ELF`.

The v1.2 target is `EXCLUDE_FROM_ALL` and uses four compile flags:
`DESR_HDD_TEST=1`, `DESR_HDD_NETFIX=1`, `DESR_HDD_PREVIEW=1`,
`DESR_HDD_V12=1`. The read-only mount is enforced by the preview UI **and**
by a hard failure in `hdd_enable_writes()`.
Production target names and the 1.0 GitHub release assets are unchanged.
Before using any resulting ELF, confirm that it contains
`hdd0:PP.DVRISO12` and that the displayed application variant says
`1.2 HDD`.

## Physical DESR-7000 results (November 8, 2026)

| Test | Physical console result | What it proves |
| --- | --- | --- |
| N1 — DNS/TCP443 | **PASS**, code `0` | Console resolves `raw.githubusercontent.com` and connects on TCP port 443 |
| N2B — NTP UTC | **PASS**, code `0` | With a **depleted RTC battery**, console receives UTC `2026-11-08 22:45:22` from `time.cloudflare.com` over UDP/123; reported **stratum 3** |
| N2A — hardware RTC | **Deferred** | Depleted battery: native RTC need not work to run network time probe |
| N3A — Roughtime UDP/2003 | **PASS**, code `0` | Console received **352 bytes**, parser found **6 tags** and accepted framing; **signature and time are still UNVERIFIED** |
| N3B-OFFLINE — Ed25519 / SHA-512 | **PASS**, code `0` | Standalone `V12_N3B_OFFLINE.ELF` executed on physical DESR-7000: delegation signature, signed response, Merkle/nonce verification passed; wrong root, nonce and altered data rejected |
| N2C/N3B — verified time / wolfSSL HTTPS | **Not yet tested on hardware** | Secure entropy, Ed25519 delegation/signature/nonce, trusted time, CA/hostname verification and TLS handshake remain to be proven |

**NTP stratum 3 is a normal synchronized NTP server level, not an error.**
The N2B time is **provisional and unauthenticated**. It was not written to
PSX RTC or console disk, and it must not be treated as a trusted wolfSSL
certificate-validation clock.

## Stage N2B — Time bootstrap for dead-RTC DESR (PASS ON HARDWARE)

**CI: PASS. Physical DESR-7000: PASS (2026-11-08 22:45:22 UTC, stratum 3, code 0).**

- [N2B ELF — GitHub Actions run 42531199387](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42531199387), artifact `V12-ISOLATED-HDD-EXPERIMENTAL-NO-TLS`, containing `DVR_ISO_MANAGER_V12_HDD.ELF`.
- [Strict NTP codec, RTC and partition host tests — run 42531214285](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42531214285) — **PASS**.
- **N2A RTC test is optional/deferred:** the console has a depleted RTC
  battery. N2B demonstrated that network time retrieval works without it.

The v1.2 read-only Manager now supports **SQUARE: NTP UTC**. Pressing it
performs a DNS lookup for `time.cloudflare.com`, sends one NTPv4/SNTP
request on UDP port 123, checks response mode/stratum/leap indicator,
matches the request token with the originate field, rejects truncated
and out-of-era timestamps, and displays the resulting UTC date/time.
It reports `NETWORK TIME RESULT: PASS (code 0)` on success.

There are **no writes** to the RTC, NVRAM, `PP.DVRISO12`, `PP.DVRISO`
or `dvr_pfs0:/ISO`. The existing `V12_NET.CNF` is reused unchanged.
`TRIANGLE` still runs the proven N1 DNS/TCP443 probe, `X` remains
disabled, `CIRCLE` returns to the menu, and `START` exits.

- NTP server: `time.cloudflare.com`, **UDP/123** (not TCP/443).
- Expected output: `NTP UTC: YYYY-MM-DD HH:MM:SS`,
  `NTP STRATUM=...`, `NETWORK TIME RESULT: PASS (code 0)`.
- All times shown are **UTC**, not local Poland time (UTC+1 in winter).
- A router or ISP that blocks outbound UDP/123 can produce a timeout.
  This is a network limitation, not evidence of broken RTC/NETFIX.
- Test with the existing `V12_NET.CNF` and `PP.DVRISO12` from N1.
  Copy the new experimental ELF to the same FAT32 USB path and launch
  through wLaunchELF_R3Z. Do **not** install into the 1.0 XMB.
- Typical errors: `-311` config unavailable, `-313` network init,
  `-314` NTP DNS, `-312` UDP timeout/connect failure, `-316`
  rejected response.

**Security boundary:** plain NTP replies are **not cryptographically
authenticated**. The request timestamp is a reply-matching token, not
cryptographic entropy or proof of time integrity. The Manager treats the
result as a provisional in-RAM UTC estimate **for display and diagnostics
only**. Never disable certificate verification or automatically treat this
NTP reply as a trusted wolfSSL certificate-validation clock. Authentic
time (NTS, signed Roughtime, or another verified bootstrap strategy)
remains an open design requirement for production native HTTPS.

**Development plan:** after N2B succeeds on hardware, start the time
bootstrap automatically as a first step of the future HTTPS download
workflow (not at every Manager startup, to avoid offline startup hangs).
The manual SQUARE action remains useful for diagnostics.

## N3B-ALT: TXRyan read-only strict-Roughtime interoperability probe

The DESR-7000 **N3B MINI Cloudflare test passed the physical
cryptographic forensic check, RET=1**, but Cloudflare omitted the
mandatory IETF draft-08 NONC echo. Strict verification correctly fails;
this reply CANNOT establish a trusted time source.

To avoid weakening the protocol, a second dedicated
`V12_N3B_TXRYAN_RO.ELF` uses `time.txryan.com:2002`, whose
independently published **Ed25519 public key** is pinned (verified against
`cloudflare/roughtime/ecosystem.json` in CI). The server reports support
for old IETF Roughtime drafts including draft-08. This is a compatibility
hypothesis, **not yet validated on the physical DESR-7000**.

Source: `src/v12_n3b_standalone.c` with `V12_N3B_TXRYAN` build flag,
`src/net/v12_roughtime_txryan_sig.c` and
[alternate CI workflow](../../.github/workflows/v1.2-n3b-alternate-server.yml).
The alternative ELF uses the N3C-proven RO PFS/IOP initialization and
NETFIX network path, but **never mounts PFS RW**. No game boot, artwork
downloads, HTTP/TLS, RTC writes or APA operations.

**CI PASS:** [TXRyan alternate probe run 42902009176](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42902009176).
The pinned server root, host wrong-root/tampering tests, source read-only
contract and PS2 cross-build all passed. The isolated ELF is
**2,582,124 bytes**, and the Actions artifact is
`V12-N3B-TXRYAN-READONLY-ALT`.
The original Cloudflare mini regression workflow was also rechecked:
[run 42902009114](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42902009114) **PASS**.

Download artifact `V12-N3B-TXRYAN-READONLY-ALT`,
copy **only** `V12_N3B_TXRYAN_RO.ELF` to FAT32 USB and launch from
wLE-R3Z. Physical DESR test is still **PENDING**.
Existing experimental `PP.DVRISO12/V12_NET.CNF` must be present. The
diagnostic runs automatically (no SELECT) and displays its result for
120 seconds. Report the full screen:

- `RET=0` / `STRICT CRYPTO: PASS (WEAK NONCE)`: strict draft-08
  signature, echo and Merkle validated against TXRyan. **Still not**
  trusted time because the request nonce is predictable and replayable.
- `RET=1` / `NONC MISSING`: only noncompliant forensic crypto PASS.
- Negative `RET`: communication, unsupported version, invalid frame,
  signature or setup failure; record the code, do not weaken checks.

Even if RET=0, the next gate is persistent secure nonce provisioning
(not solved by the N3C synthetic write/read PASS), then an authenticated
UTC policy, strong TLS entropy, CA chain and hostname verification.
RFC 10049 (October 2026) has since been published as Experimental;
this isolated test intentionally uses the earlier IETF draft-08 wire
format and is not an RFC 10049 implementation.

## N3B — GUI boot regression after forensic build (DESR-7000)

**Physical test on DESR-7000: FAIL (2026-11-10).** The complete
`DVR_ISO_MANAGER_V12_N3B_LIVE.ELF` containing the new forensic
missing-NONC path displayed only **`PS` in the upper-left corner**
and did not enter the Manager. This is a NEW **pre-menu boot failure**;
it is not the previous on-network `-327` signature rejection.
The initial Manager screen begins with `PSX DVR ISO Manager...`,
so the observed `PS` suggests interruption during very early startup.
The root cause remains **UNCONFIRMED**; possibilities include
load-time memory layout/size or the debug screen code.

Do **NOT** retest the faulty full N3B GUI build or repeat N3C
WRITE_ONCE. To isolate the boot regression, a new separate
`V12_N3B_MINI_RO.ELF` was added. It excludes the Manager UI,
cover/ISO services and unrelated embedded IRX. It follows the
known-good N3C PFS driver initialization sequence and the prior N3A
network probe. Its early screen prints `N3B MINI BOOT - STAGE 0`
before any IOP reset, then reports stage 1 (IOP reset), stage 2
(HDD/PFS drivers), stage 3 (read-only config), stage 4
(NETMAN/SMAP), stage 5 (UDP), stage 6 (Ed25519/SHA512). The result
screen remains visible for 120 seconds. Error codes: -502 IOP/module,
-503 RO config, -504 network, -505 DNS, -506 UDP,
-507 frame, -508 signatures, -509 RO unmount. Positive
`RET=1` means **missing NONC, forensic crypto only**, never trusted
time. `RET=0` is strict signed-response verification with the
still-insecure diagnostic nonce, also NOT a trusted clock.

**CI PASS:** [N3B minimal boot workflow — run 42769359257](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42769359257),
artifact `V12-N3B-MINI-READONLY-BOOTFIX`, containing
`V12_N3B_MINI_RO.ELF`. The build compiles against the pinned
hardware-tested NETFIX SDK, checks read-only source isolation, and
compares ELF layouts: **minimal 2,582,008 bytes** versus
**full N3B Manager 4,235,296 bytes** (39% reduction). This is
strong evidence that the previous binary carried substantially more
code and embedded modules, **not proof of the freeze cause**.
**Physical DESR-7000 result: PASS (2026-11-10).** The MINI ELF booted
and completed all stages. The user observed the exact final screen:

```text
NONC MISSING: FORENSIC CRYPTO PASS
INVALID IETF REPLY: NOT A TRUSTED CLOCK
NO TLS / COVER / PFS WRITES / RTC
RET=1
```

This proves the real Cloudflare response passed pinned Ed25519 root
and delegation/response signatures, plus nonce-bound Merkle inclusion
via the *forensic* verifier on physical EE hardware. The response is
**noncompliant** because the required NONC echo is absent. This is
**NOT strict Roughtime verification, trusted UTC, strong nonce, secure
TLS, or HTTPS cover downloading**. The full Manager boot regression
remains unproven in cause; the MINI diagnostic is a working workaround.
Do **not** rerun N3C WRITE_ONCE. The diagnostic did not write to PFS.
Launch the dedicated ELF from FAT32 USB via wLE-R3Z.
It will run without pressing SELECT and reuse
`PP.DVRISO12/V12_NET.CNF` in read-only mode. No RTC changes,
game launch, TLS/HTTP, ART, HDD/APA/PFS writes.

## N3B-LIVE-SIG — physical FAIL -327 diagnosed; protocol compatibility probe

**Physical DESR-7000 on November 10, 2026: FAIL (-327).** Previous N3A
transport PASS: 352 bytes, 6 top-level tags. The bundled *synthetic* signed
IETF draft08 fixture is 392 bytes/7 tags. This exactly matches the 40-byte
absence of a 32-byte NONC echo plus 8 bytes tag/offset overhead.
Independent upstream issue [cloudflare/roughtime #72](https://github.com/cloudflare/roughtime/issues/72)
opened April 24, 2026 reports official Go IETF client rejection
`protocol: response is missing NONC tag` on the same Cloudflare host.
This is the **probable cause**, not proven until the next physical test.

The **strict** `v12_rt_verify_draft08()` still REJECTS a missing
mandatory NONC, as required by the IETF draft. An explicitly separated
`v12_rt_diagnostic_missing_nonce_echo()` mode independently checks
the pinned Ed25519 delegation signature, SREP signature, and request
nonce's signed Merkle inclusion **without accepting the malformed
response as valid IETF**. It returns **no time value**. Host tests build
a 352-byte synthetic *NONC-absent* vector from the original 392-byte
signed fixture, require strict rejection and forensic crypto success,
and reject bad roots/nonces/tampering.

When the new ELF is built and tested, SELECT may report
`RESULT: NONC MISSING (code 1)`,
`Ed25519 and Merkle: FORENSIC PASS`,
`IETF PROTOCOL: NONCOMPLIANT`.
This is diagnostic progress, **not a trusted server time**, not a
certificate-validating clock and not a success condition for TLS.
A FAIL -327 still means verification failure requiring further diagnosis.
The current request nonce is also weak and replayable.

**Updated N3B forensic CI: PASS.** [Run 42767679238](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42767679238)
built the new `DVR_ISO_MANAGER_V12_N3B_LIVE.ELF` with pinned NETFIX
and published artifact `V12-N3B-LIVE-SIG-READONLY`. Its host tests
confirm that a generated **352-byte, six-tag** signed fixture lacking
NONC remains rejected by the strict IETF verifier, while the
isolated forensic path verifies Ed25519 and Merkle; tampering and
wrong keys/nonces are rejected. Physical DESR test PENDING.

To check: copy only the new ELF from this run to FAT32 USB,
run it with wLE-R3Z, retain `PP.DVRISO12/V12_NET.CNF`, press SELECT.
The **expected investigative outcome** (not guaranteed) is:

```text
RESULT: NONC MISSING (code 1)
Ed25519 and Merkle: FORENSIC PASS
IETF PROTOCOL: NONCOMPLIANT
NO TRUSTED TIME: weak/replayable nonce.
```

Any FAIL must be reported with its code. No TLS clock trust or
cover download is enabled. `CIRCLE` returns to menu.


## N3B-LIVE-SIG — signature of a real Cloudflare Roughtime reply

An **independent, opt-in, read-only** experimental Manager executable
`DVR_ISO_MANAGER_V12_N3B_LIVE.ELF` combines the N3A UDP/2003 transport
already tested on DESR-7000 with the Monocypher Ed25519/SHA-512 backend
which passed hardware verification in N3B-OFFLINE. It pins the **public**
Cloudflare-Roughtime-2 root key from the upstream `ecosystem.json`,
verifies root-signed delegation, online response signature, MIDP range,
nonce equality and SHA-512/32 Merkle inclusion, and rejects a wrong root
or mismatched nonce.

**CRITICAL: signature PASS is NOT trusted time.** The N3A request nonce
is derived from predictable local state; it has not been replaced with a
secure, crash-safe provisioned nonce. An adversary could potentially
replay a previously signed response. Therefore the live diagnostic
intentionally does **not** expose its timestamp to TLS, RTC or other
application logic. This test performs **no HTTPS, PNG transfer, disk
write or game launch**.

**CI PASS:** [GitHub Actions run 42726532384](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42726532384)
completed successfully: production root-key pin, host Ed25519
fixture/negative tests and independent PS2 NETFIX cross-build.

Download its `V12-N3B-LIVE-SIG-READONLY` artifact from the Actions
page (NOT the ordinary 1.2 Manager).
Place only `DVR_ISO_MANAGER_V12_N3B_LIVE.ELF` on FAT32 USB, launch
through wLE-R3Z, reuse existing `PP.DVRISO12/V12_NET.CNF`, and
press **SELECT**. Expected diagnostic:

```text
PSX DVR ISO MANAGER 1.2 - N3B LIVE SIG
LIVE SIGNATURE RESULT: PASS (crypto only) (0)
NO TRUSTED TIME: weak/replayable nonce.
NO TLS, ART, PFS OR RTC WRITES.
```

Photograph the full screen. Use CIRCLE to return to the list. This
probe remains **UNTESTED on physical DESR hardware** until actual
feedback is received. If any error occurs, report the full result
code (`-327` indicates signature rejection; `-326` invalid frame).
Do not interpret `PASS` as sufficient for native HTTPS.

## Stage N3B-OFFLINE — cryptographic self-test (PASS ON HARDWARE)

**Host runtime: PASS. PS2 EE build: PASS. Physical DESR-7000: PASS (code 0).**

Hardware photograph confirms exactly: `ED25519 DELEGATION: PASS`,
`ED25519 SIGNED RESPONSE: PASS`, `NONCE / MERKLE PROOF: PASS`,
`WRONG ROOT/NONCE/TAMPER: REJECTED`, `N3B OFFLINE RESULT: PASS (code 0)`.
This verifies the isolated crypto code actually runs on the EE with the
synthetic public test vector. **It does NOT verify a live server.**

- [N3B-OFFLINE GitHub Actions run 42570084969](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42570084969) — **PASS**.
- In its **Artifacts** section download
  `V12-N3B-OFFLINE-PERMISSIVE-DIAGNOSTIC`. Inside are
  `V12_N3B_OFFLINE.ELF`, `README-N3B-OFFLINE.txt`, and
  `n3b-source/` containing the C code, public synthetic test fixture,
  Monocypher source, licenses and build workflow.
- This is a **standalone ELF**, **NOT** a replacement for
  `DVR_ISO_MANAGER_V12_HDD.ELF` and **NOT** the 1.0 XMB program.
  It uses Monocypher's BSD-2-Clause/CC0 Ed25519+SHA512 instead of GPL
  wolfSSL; the test fixture comes from Apache-2.0 Cloudflare Roughtime.
- It makes **no network calls, no RTC/NVRAM writes, no PFS/APA mounts
  or writes**, and contains no Quickboot. No `PP.DVRISO12` or
  `V12_NET.CNF` is required.

### Physical DESR-7000 test

1. Copy **only** `V12_N3B_OFFLINE.ELF` to FAT32 USB. Do not
   overwrite either Manager 1.0 or Manager 1.2.
2. Use **wLaunchELF-R3Z** to execute the standalone ELF from USB.
3. The GS debug screen will show a signed synthetic test-vector
   verification result. Expect:
   ```text
   PSX DVR ISO MANAGER 1.2 - N3B OFFLINE TEST
   ED25519 DELEGATION: PASS
   ED25519 SIGNED RESPONSE: PASS
   NONCE / MERKLE PROOF: PASS
   WRONG ROOT/NONCE/TAMPER: REJECTED
   N3B OFFLINE RESULT: PASS (code 0)
   NO TRUSTED LIVE TIME. NO HTTPS. NO DISK WRITES.
   ```
4. Take a photo of the complete screen. The program holds this
   screen for approximately 90 seconds, then exits; no controller
   buttons are required in this standalone diagnostic.
5. On any `FAIL (code ...)`, report the exact number instead of
   assuming TLS or hardware crypto works.

**Security scope:** these are public artificial test signatures and
keys. Passing only confirms that Ed25519/SHA512 and the Roughtime
delegation/Merkle parser executed on the real EE. It does **NOT**
authenticate a live server or replace strong nonce/seed provisioning,
certified time, CA verification or real HTTPS.

The baseline wolfSSL option remains under development. Because the
installed PS2SDK contains AFL-2.0 components and GPL-3.0 wolfSSL
linking has licensing implications, the isolated hardware diagnostic
uses a permissive crypto library. This is not a final legal opinion;
license compatibility and attributions must be reviewed for a public
1.2 Manager release.

## Stage N3B — Roughtime cryptographic verification (HOST REFERENCE PASS)

**Live N3B (authenticated server time) is NOT ready for hardware testing.** The separate **N3B-OFFLINE** cryptographic self-test above is now hardware-confirmed PASS.

- [N3B host tests: PASS — GitHub Actions run 42534048696](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42534048696).
### N3B — independent C implementation now tested

- [Host **C** verifier and OpenSSL test run 42567182252](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42567182252) — **PASS**.
  Added `dvr-iso-manager/src/net/v12_roughtime_verify.c` with strict
  map parsing, pinned-key callback, delegation and SREP Ed25519 checks,
  nonce binding and SHA512/32 Merkle verification. The callbacks are
  injected by a cryptographic backend; the core has no sockets, RTC,
  disk writes or TLS clock changes. Host tests accept the signed
  Cloudflare Apache-2.0 test fixture and reject modified/wrong-key/
  wrong-nonce responses.
- [PS2 EE cross-compiled **full C verifier + wolfCrypt Ed25519/SHA512**
  run 42567328578](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42567328578) — **PASS**.
  The isolated N3B compile includes `v12_roughtime_wolfcrypt.c`
  and a generated *public* signed test vector. The binary remains
  **CI-only** and is not executed on hardware or included in the
  Manager. No changes to the pinned HDD NETFIX SDK.
- **Not yet solved:** unpredictable request nonces and secure TLS
  session entropy, plus pinned production Roughtime root / rotation
  policy. The N3A xorshift-based probe nonce is explicitly unsuitable
  for authenticated time. No live signed-time trust or HTTPS client
  is enabled, even though the offline known-answer verifier passes.

- `dvr-iso-manager/tests/net/v12_roughtime_reference_verify.py` uses
  host Python `cryptography` Ed25519 to verify a public Cloudflare
  IETF draft-08 test vector: the root-signed delegation certificate,
  signed response, delegated key, version, NONC, signed time bounds,
  radius, and nonce Merkle inclusion proof. A deliberately modified
  response, wrong root or wrong nonce must fail. The fixture is
  Apache-2.0 from the upstream Cloudflare Roughtime repository.
- This is an **offline known-answer test** with synthetic test keys,
  not a production server identity, not a verified PS2 C runtime.
  The synthetic vector stores **private key seeds** for repeatability;
  those are not keys to embed in the console.
- The default wolfSSL PS2SDK port **does not include Ed25519
  verification**. A completely separate PS2 wolfSSL build with
  `WOLFSSL_ED25519=yes` and `WOLFSSL_SHA512=yes` has now **PASSED
  cross-compilation + static linking**:
  [N3B Ed25519+SHA512 CI run 42534544451](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42534544451).
  The linked audit contains RFC 8032 verification test 1 and a SHA512 test;
  CI checks the ELF is valid but **does not execute it on hardware**.
  No binary is distributed. The build uses the same PS2SDK
  `XINET_PTON` workaround as ps2sdk-ports and does not alter the pinned
  NETFIX SDK or stable Manager.
- Roughtime cryptographic verification still requires a **fresh,
  unpredictable nonce** for replay resistance, an explicitly pinned
  current server public key, and a careful server key-rotation policy.
  These remain open implementation and security tasks.

No **live authenticated-time N3B** ELF is distributed. The N3A output of
`352 bytes` and `6 tags` is **still not authenticated**.

### N3C — durable nonce reservation policy (host tests PASS)

A new **offline-only** state machine in
`dvr-iso-manager/src/net/v12_rt_reservation.c` consumes a freshly
provisioned 256-bit secret, a 16-byte public key identifier and a
persistently reserved 64-bit counter. It requires a storage adapter to
perform compare-and-swap with a **crash-durable** write before exposing
a nonce. It then **re-reads** the state and only derives the nonce when
the stored key ID and counter match the newly reserved value.

[Host GitHub Actions run 42573388346](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42573388346):
**PASS**. Tests simulate failed loads and commits, stale readbacks,
counter overflow, HMAC failure, and rollback of a partition snapshot.
The rollback test deliberately demonstrates that restoring the **same**
secret and counter would repeat a nonce; recovery therefore **must
rotate to a new CSPRNG secret and identifier**.

**This does not yet establish durable writes on DESR PFS.**
The persistence callbacks are currently exercised against an in-memory
mock. No seed or counter is provisioned on the console and this module
is not linked into any shipping Manager or downloadable ELF.
Before use, a dedicated hardware validation must establish actual PFS
write, flush, readback and power-failure behavior; the seed must never
be committed to the public repository or logged in diagnostics.
There is no claim of authenticated live time or TLS yet.

### N3C PFS physical write/verify probe — PASS ON DESR-7000

**Static safety tests: PASS. PS2 EE WRITE_ONCE + VERIFY_RO builds: PASS.
Physical console: PASS (normal-reboot readback; NOT power-loss durability).**

Physical hardware results, reported by the DESR-7000 tester (2026-11-10):
- **TEST A**, `V12_N3C_PFS_VERIFY_RO.ELF` before first write:
  `N3C PFS RESULT: FAIL (code -407)`. This is the EXPECTED
  missing-record outcome and is therefore a **PASS for preflight**.
- **TEST B**, `V12_N3C_PFS_WRITE_ONCE.ELF`, run once:
  `fileXioSync=0` and
  `JOURNAL VERIFIED: SLOT1 GEN=2 COUNT=1001`.
  The final `N3C PFS RESULT` line was NOT visible/reported,
  so the writer's final unmount status is **not independently proven**.
  Dummy-file write, sync and immediate RO readback are confirmed.
- **TEST C**, after normal console restart, `V12_N3C_PFS_VERIFY_RO.ELF`:
  `N3C PFS RESULT: PASS (code 0)`.
  This verifies both synthetic journal slots could be decoded and checked
  via an isolated **read-only mount after a normal restart**.

No seed/secret was created, and neither the test nor its results imply
crash-atomic persistence, rollback protection, a secure nonce, trusted
Roughtime time or HTTPS. **Do not rerun WRITE_ONCE** with these files
present.

[GitHub Actions run 42575103152](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42575103152)
contains artifact `V12-N3C-PFS-TEST-PP.DVRISO12`. It bundles two
standalone ELFs, source, attribution and
[complete safety instructions](./N3C_PFS_TEST.md).

The first ELF can write **exactly two new, non-secret 64-byte files**
to **`hdd0:PP.DVRISO12` ONLY**. It refuses overwrites, unexpected
stat failures and missing partitions. It calls `fileXioSync()` after
each record, unmounts RW and remounts RO for verification. The
second ELF **only reads** these test records after a normal reboot.
Neither is part of Manager 1.0 or Manager 1.2; the stable program
remains read-only in this experimental branch.

**Do not run the writer twice or cut power during its operation.**
Back up APA critical data first. Even passing both tests proves only
write/read visibility across a normal reboot — **not** atomic crash
durability, strong entropy, trusted Roughtime, or HTTPS.

### N3C — two-slot journal record and corruption policy (host PASS)

A **64-byte, versioned counter record** is now available in
`dvr-iso-manager/src/net/v12_rt_journal.c` with an 8-byte magic,
version, 16-byte public seed ID, 64-bit generation number, last
reserved nonce counter, inverted counter, CRC32 and end marker.

- The record contains **NO private seed bytes**. CRC32 only detects
  accidental damage; it cannot prevent malicious tampering or rollback.
- Two alternating slots are considered valid only if their generations
  and key IDs are unambiguous. The parser **fails closed** on any
  nonblank corrupted slot instead of silently falling back to an older
  counter. One blank slot is accepted only at initial provisioning
  (generation 1), not later.
- A newer reservation must be durably written and flushed, then
  re-read and confirmed **before** its nonce can be used in a request.
  File-write flush/rename durability remains untested on real PFS.
- [GitHub Actions N3C host test run 42573756836](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42573756836)
  — **PASS**, including 512 single-bit corruptions, contradictory
  slots, invalid versions and fail-closed behavior.

The journal encoding/selection library is pure in-memory and is
exercised by the standalone N3C PFS diagnostic; the **synthetic 64-byte
records have now passed hardware readback after a normal reboot**.
The journal and nonce-reservation state machine are **not integrated
into the Manager or any live Roughtime/TLS client**. Actual power-loss
atomicity and restoration of older HDD snapshots remain unproven.
Next steps: audit a fail-closed PFS persistence adapter, design
one-time CSPRNG seed provisioning and test the authenticated
Roughtime/TLS bootstrap without relaxing certificate verification.

### Strong nonce design prototype — not yet deployed

A new **host-tested** module,
`dvr-iso-manager/src/net/v12_roughtime_nonce.c`, can derive 32-byte
Roughtime nonces using domain-separated HMAC-SHA256 from a provisioned
256-bit secret and a 64-bit counter. The independent OpenSSL known-answer
test checks that distinct counters produce distinct output and invalid
inputs fail without modifying the output buffer:
[host CI](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42567762336) — **PASS**.

This is **not** a built-in random number generator, and is **not ready
for use on DESR**. To be secure it requires (a) a unique seed generated
once with a trusted operating-system CSPRNG, (b) confidential storage of
that seed on the experimental application partition, and (c) a
crash-safe, persistently reserved counter that is never reused across
power failures, console resets, backups or restores. A restored disk
image can roll the counter back; when this occurs the seed MUST be
rotated. Merely mixing a timer, MAC, xorshift or packet jitter is NOT
a replacement for a high-entropy seed. The Manager is still strictly
read-only and does not create seed/counter files.

A future standalone provisioning utility may create such a seed once
on a user's trusted PC, but this would be a **one-time setup**, not a
requirement to run a PC while later downloading covers. No secret key
bytes should ever be uploaded to GitHub or included in diagnostics.
A separate, domain-separated design is still needed for wolfSSL's
cryptographic DRBG. Until these pieces and N3B live validation pass,
**verified online time and HTTPS remain disabled**.

## Stage N3A — Roughtime wire/network preflight (PASS ON HARDWARE)

**Host tests: PASS.** [GitHub Actions run 42532764928](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42532764928).
**PS2 build: PASS.** [GitHub Actions run 42532764843](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42532764843).
Download artifact `V12-ISOLATED-HDD-EXPERIMENTAL-NO-TLS`,
containing `DVR_ISO_MANAGER_V12_HDD.ELF` (~1.6 MB archive). Replace only
the experimental ELF on the FAT32 USB, launch from wLE-R3Z as before.
Keep the same **PP.DVRISO12/V12_NET.CNF** as N1/N2B.

**Physical DESR-7000: PASS.** SELECT produced:
`UDP RESPONSE: 352 bytes`,
`ROUGHTIME FRAME: PASS, tags=6 (UNVERIFIED)`,
`N3A UDP/2003 RESULT: PASS (transport only) (code 0)`
and `N3A TRANSPORT RESULT: PASS (0)`.

This establishes **wire-format and network transport only**. No signatures,
server authentication, fresh unpredictable nonce, trusted UTC or TLS
certificate checking have yet succeeded.


Cloudflare operates the public `roughtime.cloudflare.com:2003` service,
which signs time responses with Ed25519 using a published root public key.
The reference [Cloudflare Roughtime protocol implementation](https://github.com/cloudflare/roughtime)
is Apache-2.0 licensed, but its README warns against production use. Our
probe is independently implemented from the public draft-08 message layout.

The new v1.2 read-only build supports **SELECT**:

- N1 remains on **TRIANGLE**: DNS + outbound TCP/443.
- N2B remains on **SQUARE**: provisional NTP UTC over UDP/123.
- N3A uses **SELECT**: resolve `roughtime.cloudflare.com`, send one
  1024-byte IETF Roughtime draft-08 UDP request to **port 2003**, receive
  a bounded response, check structural framing and whether the response
  includes `SIG`, `SREP` and `CERT` tags.
- **CIRCLE** returns to the browser; **START** exits.
- **X and Quickboot remain disabled**, and the ELF writes no ART,
  configuration, RTC or disk data.

Expected success screen:

```text
PSX DVR ISO MANAGER 1.2 - N3A ROUGHTIME
Cloudflare UDP/2003 (IETF draft-08 transport)
...
UDP RESPONSE: ... bytes
ROUGHTIME FRAME: PASS, tags=... (UNVERIFIED)
NO Ed25519 check. NO trusted time.
N3A TRANSPORT RESULT: PASS (0)
DO NOT USE THIS REPLY FOR TLS CERTIFICATES.
```

**Passing N3A proves only that the server answered and the packet has a
recognizable shape. It does NOT authenticate the server or the time.**
The request nonce in this transport-only test is generated from public,
non-cryptographic state and MUST NOT be used in any signature-verification
or TLS security claim. The test deliberately extracts no timestamp and
never calls wolfSSL.

If UDP/2003 is blocked, N1/N2B may still work. Timeouts or malformed
frames are diagnostic failures, not evidence that PP.DVRISO12 is damaged.

Next potential stage **N3B**: pin Cloudflare's documented root Ed25519
public key, verify its delegated online certificate, the signature over
SREP, Merkle inclusion of a **fresh, unpredictable** nonce, midpoint,
radius and version. Reject unknown/rotated keys and unsigned data.
**No trusted time is available until ALL these checks pass.** Even then,
verify the Roughtime implementation and key-rotation policy before using
it to validate production HTTPS.

## Stage N2C — TLS prerequisites and time callback (IN DEVELOPMENT)

The time API used by wolfSSL is `wc_SetTimeCb()`. The experimental
wolfSSL **CI-only link audit** now checks that this callback API can be
compiled and linked for the PS2 EE target. The audit provides a deliberate
epoch-zero *failure clock*: **not a usable time source**, but a way to ensure
that a missing trusted time source does not silently become trusted.

The documented plain-NTP result is not a cryptographic trust anchor.
A secure bootstrap needs a *separately authenticated* time source or an
equivalent robust trust mechanism. Certificate verification **must remain
enabled** and cannot rely solely on the spoofable NTP result.

WolfSSL also requires a cryptographically strong entropy source to seed its
hash-based DRBG. No suitable hardware RNG has yet been proven on the
DESR-7000. Random values derived only from `clock()`, addresses, COP0
counters or deterministic jitter are **not accepted** as strong entropy.
The real verified TLS handshake is blocked pending a security-reviewed
solution. This is not a console test yet, and no TLS-enabled artifact
has been published.

Research: [wolfSSL porting guide: random seed](https://www.wolfssl.com/documentation/manuals/wolfssl-porting-guide/section02.html)
and [wolfSSL custom time callback API](https://www.wolfssl.com/documentation/wolfSSL-Manual.pdf).

## Stage N2A — optional RTC diagnostic (deferred: battery depleted)

**Built successfully:** [RTC-capable 1.2 HDD ELF / Actions run 42530395826](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42530395826).
The `V12-ISOLATED-HDD-EXPERIMENTAL-NO-TLS` artifact contains
`DVR_ISO_MANAGER_V12_HDD.ELF` with the original N1 and a **new SQUARE action**.

Using the **same** test `PP.DVRISO12`, `V12_NET.CNF`, USB and wLE setup
already confirmed by N1:

1. Replace only the experimental ELF on USB with the N2A build.
2. Launch from wLE, verify the header reads `1.2 HDD`.
3. Press **SQUARE**: reads `sceCdReadClock` and shows
   `sceCdReadClock rc=...`, `status=...` and `RTC DATE` or
   `RTC RAW BCD` plus `RTC PLAUSIBLE: PASS/FAIL`.
4. Take a photo of the complete screen, then use **CIRCLE** to return or
   **START** to exit. TRIANGLE still runs the previously passed N1 Internet
   test, X/Quickboot is still disabled.

**No console time is changed**, no HTTP/TLS handshake is started, and
no app/DVR files are written by this build. The check validates only date
format and plausible calendar range; even `PASS` is not authenticated
network time and cannot replace hostname/CA/certificate validation.

### wolfSSL integration evidence

[wolfSSL EE API compile + static link audit run 42530578757](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42530578757): **PASS**.
`V12_WOLFSSL_LINK_AUDIT.ELF` is built *inside CI only* using
`WOLFSSL_VERIFY_PEER`, a hostname-check API and custom I/O callbacks.
The ELF is **not released or uploaded** while downstream GPL/OSS licensing
and cryptographic entropy remain under review. It does not make a TLS
connection or authenticate a certificate.

Next blocker is provisioning genuinely strong randomness for wolfSSL
without reusing a deterministic seed; after that come CA trust and
certificate-time checks, followed by the first verified TLS handshake.
Never disable peer verification as a shortcut.

## Stage N1 — Physical DNS + TCP/443 test (READ-ONLY)

**No TLS handshake, HTTP request or PNG download happens in this test.**
The test checks network configuration, NETMAN/SMAP + NETFIX, DNS and a
short-lived TCP connection to port 443 of `raw.githubusercontent.com`.
It **never** mounts the app partition read/write. `X` and `SQUARE` are
disabled and it never launches Neutrino or writes BOOT/ART/CNF.

**N1 STATUS: PASS ON DESR-7000 (NOVEMBER 8, 2026).**

- [Verified N1 ELF — GitHub Actions run 42528063946](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42528063946) — **PASS**, artifact `V12-ISOLATED-HDD-EXPERIMENTAL-NO-TLS` containing `DVR_ISO_MANAGER_V12_HDD.ELF`.
- [Host parser and safety contract — run 42528083914](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42528083914) — **PASS**.
- This is **not** a complete HTTPS client. No TLS handshakes or cover transfers are performed by N1.

The ELF is built by [`v1.2-hdd-build.yml`](../../.github/workflows/v1.2-hdd-build.yml) from the test branch, **not** from a public release.

### What to prepare on your DESR-7000

1. Back up important APA headers/data. **Never format `PP.DVRISO`.**
   If you have not created `PP.DVRISO12`, create that separate PFS
   application partition manually only when prepared to test; 256 MB
   is sufficient for N1. Do **not** inject any XMB header.
2. Put **only** the following config file at
   `hdd0:PP.DVRISO12/V12_NET.CNF` using wLaunchELF_R3Z. Replace
   *every example address* with your own free LAN IP/router/subnet/DNS.
   Do not copy your working `PP.DVRISO/BOOT.CNF`.

   ```ini
   # EXAMPLE ONLY — adjust to your home network.
   IP=192.168.1.90
   NETMASK=255.255.255.0
   GATEWAY=192.168.1.1
   DNS=192.168.1.1
   ```

3. Copy the **experimental** `DVR_ISO_MANAGER_V12_HDD.ELF` to
   FAT32 USB and launch it with wLE (not from production XMB).
   It must show `1.2 HDD`, not `1.0 HDD`.
4. Press **TRIANGLE** exactly once. It should print the IP, DNS,
   resolved IPv4 address for `raw.githubusercontent.com` and
   `TCP/443 CONNECTED`. It then displays a result code and holds
   the screen for a photograph. **CIRCLE** returns to the menu;
   **START** exits the Manager.
5. Photograph the result screen, including error codes. The program
   does not download images; a successful N1 only proves DNS and TCP
   reachability, **not native HTTPS/TLS**.

Codes: `-301` missing configuration, `-302` invalid configuration,
`-303` NETMAN/SMAP/network failure, `-304` invalid fixed host,
`-305` DNS failure, `-306` TCP/443 connect failure. `0` is
DNS/TCP success.

The first N1 probe may wait for DNS timeouts on a broken network.
Use a working wired router and a unique unused console IP address.
No PS2 system/DVR ISO partition write is performed.

## Progress: verified-in-RAM HTTPS request scaffolding

The experimental-only modules are now in the tree:

- `dvr-iso-manager/src/net/v12_https_policy.c` builds **allowlisted** OPL Archive
  and CosmicScale requests from verified Game IDs. It rejects redirects,
  unsupported transfer encodings, oversized bodies, invalid HTTP and PNG CRC.
- `dvr-iso-manager/src/net/v12_https_client.c` builds an HTTP/1.0 GET and
  reads a bounded response into **RAM only**, requiring a transport whose
  `open_verified` has already validated TLS certificate chain, hostname,
  certificate time and sufficient cryptographic entropy. It cannot write HDD
  files and never accepts an unverified TLS handshake.
- `dvr-iso-manager/tests/net/test_v12_https_policy.c` and
  `test_v12_https_client.c` test known-good PNG, missing covers, truncated
  responses, malicious Game IDs, overflow, partial I/O, redirects and
  rejected TLS using a **mock** transport.
- GitHub Actions runs these host tests and the dedicated
  `DVR_ISO_MANAGER_V12_HDD.ELF` build target compiles the new C modules
  without using them in stable 1.0.

### Build and audit evidence

- [Host tests: partition isolation + HTTPS policy/client + Neutrino retarget](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42527003801) — **PASS**.
- [Separate Neutrino v1.2 SYSTEM and ENGLISH builds](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42527061168) — **PASS**.
- [Experimental HDD ELF with NETFIX, read-only preview and v1.2 partition](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42527208156) — **PASS**. It intentionally contains no usable Quickboot or live HTTPS.
- [PS2 wolfSSL headers/toolchain audit](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42527310888) — **PASS** for the cross-compiler using `WOLFSSL_USER_IO`, `WOLFSSL_NO_SOCK` and `NO_WRITEV`. The library exists in PS2SDK ports; certificate validation, secure entropy, DNS and live TLS are **not yet validated**.

**Still not implemented:** the live wolfSSL TLS transport with
cryptographically verified certificates and hostname, an authenticated
time source for certificate validation, strong RNG entropy, CA trust
provisioning, remote image normalization and HDD artwork writes.
The N1 connectivity and N2B **unauthenticated** network UTC tests have
both been confirmed on the physical DESR-7000.

**Neutrino isolation:** the stable HDD Neutrino FIX95 wrapper also hardcoded
`hdd0:PP.DVRISO` for both targets and its `pfs0` mount. The new
`neutrino-hdd-v1/patch_hdd_runtime_v12.py` retargets exactly three
references to `hdd0:PP.DVRISO12` and fails on an unexpected upstream layout.
Dedicated GitHub Actions build separate SYSTEM / ENGLISH Neutrino variants;
these remain **untested on physical DESR**. Quickboot remains blocked until the
complete v1.2 OSDMenu -> Neutrino -> ISO chain is verified.
**Do not claim that native HTTPS works on DESR yet.**

### Upstream research (not copied)

The reference launcher is now called
[NULD (formerly ORBIT)](https://github.com/danielnuld/nuld) and is GPL-3.0.
Its `net.c` uses wolfSSL but disables certificate verification
(`WOLFSSL_VERIFY_NONE`) and substitutes jitter-based entropy for PS2
`/dev/urandom`. Neither of these security shortcuts is acceptable for the
verified-TLS-only v1.2 transport; the adapter must fail closed until an
adequate solution exists. **No NULD source has been copied into this repo.**
Before public binary release, audit licensing of wolfSSL and all linked
components; comply with their respective source/attribution requirements.

## Planned native HTTPS cover downloads

1. Review ORBIT's actual license and attribution obligations and evaluate
   wolfSSL/libcurl compatibility with bundled PS2 components. Do not copy
   ORBIT source code until the licensing review is complete.
2. Build a **read-only** TLS/DNS probe against GitHub's image endpoint.
   Verify certificates and system time; do not disable TLS verification.
3. Download one bounded PNG into RAM, verify content/size/CRC and avoid writes.
4. Add OPL Archive HTTPS support (including legitimate redirects and image
   normalization); use CosmicScale as a fallback.
5. Only after hardware tests, integrate opt-in saving through `.PART` into
   `PP.DVRISO12/ART/`. Never write to `PP.DVRISO` or DVR ISO files.
6. Keep the known-good local Windows ART Server path as an optional fallback.

No public 1.2 tag, release or merge into `main` until tested on DESR-7000.
