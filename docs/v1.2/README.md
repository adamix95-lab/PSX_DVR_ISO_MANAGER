# PSX DVR ISO MANAGER 1.2 — experimental development (DESR-7000)

> **PUBLIC DEVELOPMENT BRANCH. NOT A RELEASE.**
>
> The `1.0` release and `main` remain unchanged. **Native HTTPS is NOT implemented yet.**
> The first milestone is safe partition isolation before any internet or TLS work.

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
4. Copy the required *runtime contents* (OSDMenu `BOOT.ELF`, both HDD
   Neutrino executables, `config/`, `modules/` and appropriate network
   configuration) into the new `PP.DVRISO12`. Do **not** copy an old
   `BOOT.CNF` pointing at the production partition.
5. Compile the experimental v1.2 binary and launch it **manually through
   wLE**, selecting `DVR_ISO_MANAGER_V12_HDD.ELF`. Start with a harmless
   read-only inspection before permitting ART or boot-config writes.
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

The v1.2 target is `EXCLUDE_FROM_ALL` and requires three compile flags:
`DESR_HDD_TEST=1`, `DESR_HDD_NETFIX=1`, `DESR_HDD_V12=1`.
Production target names and the 1.0 GitHub release assets are unchanged.
Before using any resulting ELF, confirm that it contains
`hdd0:PP.DVRISO12` and that the displayed application variant says
`1.2 HDD`.

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
