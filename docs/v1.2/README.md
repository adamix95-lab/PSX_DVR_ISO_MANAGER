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
