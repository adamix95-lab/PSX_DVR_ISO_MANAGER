# PDIM XMB GAME INSTALLER 0.3 — multi-method DVR ISO scan

**Hardware baseline:** installer **0.1 HARDWARE PASS** on Sony PSX DESR-7000. The user successfully installed and launched **Killzone** from XMB. The earlier manual ICO XMB shortcut also worked.

**Version 0.3 is a separate untested multi-method scanner build.** Test its read-only menu/SQUARE preflight before using its X or TRIANGLE write actions. Never use either write action without a full, verified, recoverable sector-level HDD backup.

**Baseline kept intact:** [v0.1 code](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/tree/feature/xmb-game-shortcuts/pdim-xmb-game-installer). [Official PDIM 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2) is unchanged.

Builds a **separate PlayStation 2 ELF**. It scans `dvr_pfs0:/ISO/` with the already tested DESR drivers, reads game IDs and downloaded PNG covers from the existing `PP.DVRISO/COVERS.IDX` and `PP.DVRISO/ART/`, and proposes a **new** `PP.<TITLE>-<GAMEID>` partition for each recognized title.

### What is reused

- IOP reset, PSDVR module order, BDM/USB/PAD timeout and two-stage confirmation from the tested [PDIM HDD 1.2 installer](../pdim-hdd-v1.2-installer/).
- Existing HDD 1.2 `BOOT.ELF` (OSDMenu) and signed `EXECUTE.ELF` (XMB KELF) embedded into this installer during CI.
- Shared `PP.DVRISO` Neutrino FIX95 and `config/modules` **read-only**.
- `PSXCOV1` cover index and PNG images already downloaded by PDIM 1.2 HTTPS, converted on-console to PNG8 XMB jackets in 140x200 and 74x108 sizes.
- `pdim12_ppaa_plan` for the three 512-byte PPAA attribute sectors. Each selected game's partition must be new and matched by immutable identity before PPAA writes.

### Stage 0.3: Sony DVR ISO multi-method scanner

The game library directory is unchanged: `dvr_pfs0:/ISO/` is the Sony DVR `xcontent/ISO` view. The `pfs1:/` mount is **only** the existing `PP.DVRISO` application partition, never the ISO storage.

**Exact Game ID lookup precedence:**
1. `PP.DVRISO/COVERS.IDX`: verified exact ISO filename to Game ID mapping; then confirm local PNG cover exists.
2. If absent, `COVERS.BAK` using the same PSXCOV1 parser.
3. If still absent, `COVER_SCAN.LOG` using the PDIM PSXCOVSCAN1 parser, only for recognized status labels and valid Game IDs.
4. Only when a selected game lacks all cached IDs, **SQUARE** attempts a live ISO scan to recover the ID. This avoids reading hundreds of large ISOs at menu startup.

**Live ISO methods:** standard ISO9660 (2048-byte sectors), raw 2352-byte sectors with +16 Mode1 or +24 Mode2 user-data offsets; parse `SYSTEM.CNF` and its `BOOT2` field, otherwise accept only one unambiguous root executable ID. All validation uses the existing PDIM `cover_index.c` helpers. A failing `fileXioLseek` triggers a fresh open and bounded sequential reads (maximum 8 MiB), including the case where seek *appears* to succeed but data reads fail.

**Read-only SQUARE diagnostics:**
- `PASS (1)`: existing matching shortcut and an ISO Game ID match;
- `PASS (2)`: existing shortcut verified, ISO ID not detectable, uses PDIM index;
- `PASS (3)`: new target, ISO ID not detectable, PNG validated, uses PDIM index;
- `PASS (4)`: existing shortcut verified, DVR random read unsupported, uses PDIM index;
- `PASS (5)`: new target, DVR random read unsupported, uses PDIM index.
- `FAIL -62`: selected ISO **cannot be opened** or an ISO-only Game ID cannot be rechecked.
- `FAIL -63`: a *positively detected different* ISO ID than the saved mapping.

Never invent a Game ID from a human ISO filename or a random PNG filename. An unsupported DVR seek or sector read is **not equivalent to a missing game file**; original v0.1 launched Killzone and many other games with the cached mapping. A game whose ID is recovered only from a live ISO must remain repeatably identifiable before write operations.

### Fix 0.2.1: PDIM COVERS.IDX fallback (Killzone regression)

- Source of each Game ID remains the hardware-proven **`PP.DVRISO/COVERS.IDX`** record matched to the **exact ISO filename**. Cover files are located at `PP.DVRISO/ART/<Game_ID>_COV.png`.
- The additional live ISO scan uses the same `cover_index.c` validators as PDIM: parse `SYSTEM.CNF/BOOT2` when available, otherwise accept a unique root executable ID. It is a **best-effort cross-check**, not a compulsory source of IDs.
- **SQUARE preflight returns PASS (2)** for an existing verified shortcut if the ISO is readable but its boot ID is not detectable. **PASS (3)** for a new, free target with a fully validated source PNG. Both explicitly show `COVERS.IDX FALLBACK`.
- **A positively detected different Game ID remains FAIL (-63)**, and physical I/O failures remain FAIL (-62). No APA write occurs on either failure.
- The previous 0.2 error `-62` for unidentifiable but otherwise readable games such as Killzone was an unintended regression; original 0.1 is untouched and was tested on physical hardware. Version 0.2.1 still requires physical verification before any writes.

### New checks in version 0.2

- Enforce Neutrino FIX95-compatible ISO filenames: maximum 95 ASCII bytes, no traversal (`..`), control characters or `=` which would corrupt the boot argument.
- **Re-read the actual Game ID from the ISO**, preferring its `SYSTEM.CNF/BOOT2` (with the same unique-executable fallback as the 1.2 Manager). Reject if `COVERS.IDX` is stale or mismatched.
- Refuse **ambiguous duplicate partition names** when two different ISO names normalize to the same ten-character stem and Game ID.
- Decode and validate the entire source PNG (140 × 200) **before creating a partition**. Bad artwork never triggers a new APA allocation.
- The exact 1.2 `PP.DVRISO` is mounted **read-only**; existing target partitions are never formatted or overwritten. PPAA writes are checked against the selected target, the original 3 sectors and the immutable per-game marker.

### Stage 0.1 hardware validation

On the physical Sony PSX DESR-7000 **test READ-ONLY scan / SQUARE first** and report the exact displayed game ID and suggested target. The v0.1 create/XMB path was confirmed by the Killzone hardware test; the stricter v0.2 path still needs its own hardware regression test. They require a complete, verified, restorable sector-level HDD backup. XMB Stage 2 on `PP.ICO` was already proven with manual wLE injection; it does NOT guarantee this generic installer.

### Controls

- UP/DOWN, LEFT/RIGHT: select game / page
- SELECT: SYSTEM / ENGLISH mode to bake into a fixed per-game `BOOT.CNF`
- SQUARE: inspect target, no writes
- X: create **new** 128 MiB PFS partition + copy/verify independent OSDMenu, XMB assets, generated CNFs
- TRIANGLE: verify the exact matching partition and assets, then register XMB (three PPAA sectors)
- CIRCLE: exit

Both **X and TRIANGLE** require a two-stage confirmation; hold L1+R1 and press X on the second screen. An existing target partition, any foreign marker or anything other than an exact `PP.<title>_<ID>` name will prevent installation. The original Sony game ISO, `PP.DVRISO` Neutrino, and the 1.2 release remain untouched.

**Cover prerequisite:** current 0.1 uses already downloaded PNGs. If absent, first use PDIM 1.2 TRIANGLE -> HTTPS to download art. Native HTTPS and PSXDatacenter descriptions/screenshot scraping are **NOT yet integrated in this ELF**.

### Known limits

- The small `res/info.sys` metadata comes from the selected filename and Game ID. Rich PSXDatacenter descriptions/screenshots require a separate network metadata module.
- The 3-3-2 indexed palette conversion may look less faithful than the manually prepared ICO Stage 2 icons.
- No automatic deletion, partition resizing or updating existing shortcuts.
- A failed new partition format/write leaves the created partition intact for manual inspection; no automatic cleanup.
- Each shortcut consumes a separate 128 MiB partition, while ISO remains in the Sony DVR filesystem.
