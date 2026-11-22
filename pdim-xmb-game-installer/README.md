# PDIM XMB GAME INSTALLER 0.2 — standalone ELF

**Hardware baseline:** installer **0.1 HARDWARE PASS** on Sony PSX DESR-7000. The user successfully installed and launched **Killzone** from XMB. The earlier manual ICO XMB shortcut also worked.

**Version 0.2 is a separate untested security-hardening build.** Test its read-only menu/SQUARE preflight before using its X or TRIANGLE write actions. Never use either write action without a full, verified, recoverable sector-level HDD backup.

**Baseline kept intact:** [v0.1 code](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/tree/feature/xmb-game-shortcuts/pdim-xmb-game-installer). [Official PDIM 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2) is unchanged.

Builds a **separate PlayStation 2 ELF**. It scans `dvr_pfs0:/ISO/` with the already tested DESR drivers, reads game IDs and downloaded PNG covers from the existing `PP.DVRISO/COVERS.IDX` and `PP.DVRISO/ART/`, and proposes a **new** `PP.<TITLE>-<GAMEID>` partition for each recognized title.

### What is reused

- IOP reset, PSDVR module order, BDM/USB/PAD timeout and two-stage confirmation from the tested [PDIM HDD 1.2 installer](../pdim-hdd-v1.2-installer/).
- Existing HDD 1.2 `BOOT.ELF` (OSDMenu) and signed `EXECUTE.ELF` (XMB KELF) embedded into this installer during CI.
- Shared `PP.DVRISO` Neutrino FIX95 and `config/modules` **read-only**.
- `PSXCOV1` cover index and PNG images already downloaded by PDIM 1.2 HTTPS, converted on-console to PNG8 XMB jackets in 140x200 and 74x108 sizes.
- `pdim12_ppaa_plan` for the three 512-byte PPAA attribute sectors. Each selected game's partition must be new and matched by immutable identity before PPAA writes.

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
