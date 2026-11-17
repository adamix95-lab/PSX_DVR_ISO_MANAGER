# PSX DVR ISO MANAGER 1.2 HDD HTTPS — R1 hardware-test update

This branch, `pdim_v1.2_hdd`, builds on the physically tested **HDD
1.0 game-launch / PFS / NETFIX / Neutrino** core, using the verified
HDD12 R3 test-partition/XMB channel. The HTTPS cover worker is ported
from the physically working USB 1.2 R4 and compiled for HDD12 PFS1.

**Experimental / not yet physically tested on HDD HTTPS. Do not release.**
Requires an existing test `hdd0:PP.DVRISO12` from R3 and a restorable
full-sector backup before modifying HDD files.

The worker reads network configuration and CA from `pfs1:/`, scans
DVR game ISOs read-only, recognizes existing HDD covers and writes only
new `pfs1:/ART/*_COV.png` files through `.PART` + exclusive creation,
readback, rename and PFS sync. It returns to the Manager on PFS1.

## Manual update for R3 test partition (no reinstallation)

Back up the old `PP.DVRISO12/DVR_ISO_MANAGER_HDD.ELF`.
Copy the package's `COPY_TO_PP.DVRISO12/DVR_ISO_MANAGER_HDD.ELF`
and `COPY_TO_PP.DVRISO12/N4E.CA` to that test partition's root.
Prepare `PP.DVRISO12/V12_NET.CNF` with your actual IP/netmask/gateway/DNS;
the shipped `V12_NET.CNF.example` contains documentation-only addresses.

Do not change `PP.DVRISO`, BOOT/EXECUTE, Neutrino, XMB PPAA metadata,
artwork or any Sony system/DVR partitions. Start the existing XMB app.
Check that `1.2 HDD` shows and games still launch. TRIANGLE indexes,
then TRIANGLE on the scan result starts native HTTPS (up to 50).
Check download, cover preview, rescan and game boot.

The installer R3 will be adjusted only after HDD HTTPS is proven.
