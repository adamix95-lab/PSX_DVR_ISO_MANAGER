# N3C — PFS dummy journal write/read test (PP.DVRISO12 ONLY)

> **EXPERIMENTAL / NOT A RELEASE.**
>
> This test is distinct from the stable PSX DVR ISO MANAGER 1.0 and the
> read-only 1.2 Manager. No production XMB installer, BOOT.CNF, ART,
> Neutrino or Sony DVR ISO partition is involved.

## What we are checking

The PSX DESR-7000 has already passed N1 (DNS/TCP443), N2B
(untrusted NTP UTC), N3A (Roughtime UDP/2003) and N3B-OFFLINE
(Ed25519 signatures and Merkle proof).

N3C tests the **next missing property**: whether small non-secret
64-byte records on a **dedicated experimental PFS partition** survive
a normal unmount/remount and a normal console restart.

The program creates only:

- `hdd0:PP.DVRISO12` mounted as `pfs1:`
- `pfs1:/PDIM_N3C_TEST_SLOT0.BIN` — 64 bytes, sequence 1, counter 1000
- `pfs1:/PDIM_N3C_TEST_SLOT1.BIN` — 64 bytes, sequence 2, counter 1001

Each record has a fixed **public dummy identifier**, format version,
generation/counter, inverted counter and CRC32. **NO random seed or
private key is created or stored.**

## Safety measures

The ZIP includes **two different ELF binaries**. Neither touches
`PP.DVRISO`, DVR ISO files, `BOOT.CNF` or the XMB header.

| Filename | Operation |
| --- | --- |
| `V12_N3C_PFS_WRITE_ONCE.ELF` | First mounts `PP.DVRISO12` read-only to confirm **both test files do not exist**, remounts it read/write, creates each with `O_CREAT|O_EXCL` (no overwrite/truncate), calls `fileXioSync()` after each, unmounts and remounts read-only to verify |
| `V12_N3C_PFS_VERIFY_RO.ELF` | **Never mounts read/write**, reads the two existing slots and verifies the CRC, identity, monotonic generation and counters |

The *write-once* binary **refuses to write** if either test file already
exists, if the partition is missing, or if a read-only preflight fails.
It never formats partitions and does not delete, rename or clean up
partial files on failure. Any non-success result requires manual review,
**not repeated execution**.

The *read-only* binary cannot create the files; its first run **before
initialization is expected to report FAIL (code -407)**. That baseline
failure simply means the dummy slots are absent.

**IMPORTANT:** A successful `fileXioSync()` and readback, even after
a reboot, **do NOT prove atomic writes on sudden power loss**. PFS
flushes dirty cache in the referenced SDK implementation; power-loss
behavior of the ATA-EXPRESS device, storage bridge and write caches
is still unknown. **Do not intentionally cut power during a write**
as part of this test.

## Download and perform the first physical test

1. Make a restorable APA/HDD backup and confirm that your existing
   `PP.DVRISO12` is the experiment partition, not `PP.DVRISO`.
   It must have some free space. Do **not** create/format any
   additional partitions for this test.
2. Download the diagnostic ZIP from GitHub Actions
   [N3C PFS workflow](../../.github/workflows/v1.2-n3c-pfs-probe.yml),
   artifact `V12-N3C-PFS-TEST-PP.DVRISO12`.
3. Copy **both** test ELFs to FAT32 USB. Do **not** replace the
   existing Manager 1.0 or 1.2.
4. Optionally launch **`V12_N3C_PFS_VERIFY_RO.ELF` first** from
   wLE-R3Z. Before the dummy slots exist it should report code
   **-407** and change nothing. Photograph any unexpected result.
5. When ready for the explicitly authorized, non-secret write test,
   run **`V12_N3C_PFS_WRITE_ONCE.ELF` exactly ONCE** from wLE-R3Z.
   Look for successful creation of both slots, `fileXioSync=0`,
   a successful read-only remount and
   `N3C PFS RESULT: PASS (code 0)`. Photograph the full screen.
   If anything fails, **stop and report the exact code**.
6. After the writer completes normally, restart the console
   normally. Do not force power-off during file activity.
7. Run **`V12_N3C_PFS_VERIFY_RO.ELF`** from wLE-R3Z. It should
   report `JOURNAL VERIFIED: SLOT1 GEN=2 COUNT=1001` and
   `N3C PFS RESULT: PASS (code 0)`. Photograph the result.

Each standalone diagnostic holds its result screen for approximately
**120 seconds**. No game controller action is necessary.

## What results mean

- **PASS after read-only reboot**: records are readable after a
  *normal* reboot, with correct bytes and monotonic counters
- **-407**: two valid slots are not present/readable (expected before
  initialization)
- **-408**: corrupted or inconsistent journal
- **-409**: journal content differs from this dummy test
- **-412**: write-once preflight found files already present or an
  unexpected filesystem error; **do not retry the writer**
- Other negative codes: module, mount, read, write or sync failure;
  send the whole screen for diagnosis

**This test never proves live Roughtime authentication, TLS, hardware
cryptographic RNG or reliable crash-durable nonce reservation.** It
provides one carefully isolated piece of physical evidence.

Source code, versioned journal format, tests and PS2SDK license are
included in the artifact. The installed Manager 1.0 and `main`
branch remain unchanged.
