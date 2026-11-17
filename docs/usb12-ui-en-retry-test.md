# PSX DVR ISO Manager 1.2 USB — UI EN + Roughtime Retry (TEST)

**Experimental CA-layout candidate; its new CA location is not yet hardware-tested.** The prior English UI + Roughtime retry variant passed 45/45 and 32/32 HTTPS hardware tests and 15/15 LAN HTTP; the intermittent green-screen issue is not root-caused.
Base commit: `735188867a27cd1d8663fa6fdde8e9125c334d14`.
Do not overwrite your proven R4 backup or the stable HDD/USB 1.0 installer.

## What's new

- English HTTPS progress: `DOWNLOADING MISSING COVERS`,
  `PLEASE WAIT...`, `N/N OK`, `SKIPPED`, `DOWNLOADING...`.
- Pressing Triangle to launch HTTPS displays `PLEASE WAIT...` while
  the embedded downloader is loaded from the single Manager ELF.
- On return to Manager, successful driver-load debug text is hidden
  behind `PLEASE WAIT...`. Driver failures remain visible.
- English Manager result: `DOWNLOAD COMPLETE` or
  `DOWNLOAD INTERRUPTED`, with counts and X/O to return to game list.
- Roughtime: up to three requests if the response is absent or invalid.
  Each attempt uses a different BLAKE2b-derived nonce. All normal
  signature, nonce and signed-time checks remain mandatory; failures
  never bypass TLS verification. Retries are bounded.
- No change to the R4 worker->Manager loader, its arguments, or the
  existing 8-second return delay.

## Test on physical Sony PSX DESR-7000

1. Back up R4 and the previous UI TEST ELF.
2. From the archive, replace **only**
   `DESR_DVR/DVR_ISO_MANAGER_V12_CPU.ELF` on FAT32 USB.
   Place `DESR_DVR/N4E.CA` from the new ZIP **next to the ELF**.
   If you already have a known-good `N4E.CA` at the USB root,
   you may move that same file into `DESR_DVR/` (do not regenerate it).
   The new worker **does not use the old USB-root CA path**.
   Keep `V12_NET.CNF`, `ART/`, HDD 1.0, and XMB unchanged.
3. Start from wLE-R3Z 4.80. Press Triangle to scan, then Triangle to
   run HTTPS. Test 5-10 missing covers first, then 20, then up to 50.
4. Confirm English screens, saved files, X/O return, and game launch.
5. If return hangs on green, note the last text/stage and check after
   reboot whether all images are on USB. Do not repeatedly reset
   during an ongoing write.

## Important limitations

- The 45/45 green-screen return hang is intermittent and **not yet
  root-caused**. This build changes only presentation and Roughtime
  retry behavior, not speculative TLS/IOP shutdown sequencing.
- Experimental CPU-counter entropy is not equivalent to a guaranteed
  cryptographic RNG; hashing/retry derivation does not add entropy.
- Verified JPEG/PNG fallback, BATCH50, download resume, `.PART`,
  write/readback/rename, and the verified R4 loader contract remain.
- Both stable USB/HDD 1.0 and the tested R4 remain untouched.
- Planned HDD 1.2: `N4E.CA` will live in the `hdd0:PP.DVRISO`
  application partition (mounted as `pfs1:/N4E.CA`), not on USB.
  This HDD requirement is documented only; no HDD build is changed.

If this regresses, restore your saved working R4 ELF.
