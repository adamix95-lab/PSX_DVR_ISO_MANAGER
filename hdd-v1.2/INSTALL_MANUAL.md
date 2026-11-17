# PDIM 1.2 HDD — manual installation (pre-release)

**No public Release 1.2 is authorized. Full recoverable sector-level HDD backup required before changes.**

Target: **hdd0:PP.DVRISO**. The old PP.DVRISO12 was only a test partition. ISO games stay in Sony's DVR filesystem; do not move or reformat them.

1. Prepare the FAT32 USB stick by extracting the **MANUAL** test ZIP. Keep the `__Headers/PP.DVRISO/` folder exactly as supplied — wLE-R3Z recognizes `__Headers`, not an arbitrary renamed directory.
2. Launch wLE-R3Z 4.80. Inspect the HDD first. If `PP.DVRISO` already exists, **do not create, delete or format it**. If absent, deliberately create a 256 MiB PFS `PP.DVRISO` using the HDDManager after the full backup. Confirm the actual partition name before continuing.
3. Before modifying an existing HDD 1.0 install, back up the original `DVR_ISO_MANAGER_HDD.ELF` and any files you will replace. Keep `BOOT.CNF`, `V12_NET.CNF`, `ART/` and cover indexes intact.
4. Copy the **contents** of `PP.DVRISO-FILES/` to the root of the mounted PFS partition, preserving subdirectories. Do not copy `PP.DVRISO-FILES` as an extra directory. The 84-file manifest covers 79 runtime files and 5 XMB PFS files. Recheck key files by size or hash where available.
5. Prepare `V12_NET.CNF` from `V12_NET.CNF.example` using *your real* IP, NETMASK, GATEWAY and DNS settings; copy it to the PFS root. Never use example documentation addresses as real network configuration.
6. Launch `DVR_ISO_MANAGER_HDD.ELF` manually from the PFS in wLE. Confirm the 1.2 HDD title, list of games, SYSTEM/ENGLISH and at least one game boot.
7. If XMB is not registered yet, with `__Headers/PP.DVRISO` present on USB select **HDDManager → PP.DVRISO → R1 → Inject Header → This partition only** in wLE-R3Z. The five XMB PFS files must already be present. If the existing HDD 1.0 XMB entry already works, do **not** blindly reinject it: reuse the launcher; updating the Manager file is sufficient for functional 1.2.
8. Cold-reboot the console, check Games icon (not Corrupted Data), launch from XMB, test two games and language. Download only deliberately missing backed-up ART over HTTPS, then repeat a scan to check skip behavior.

If any partition/header check fails, stop. Do not delete or format the existing HDD/PFS. The manual path does not perform the installer's automatic CRC verification or protected backups.
