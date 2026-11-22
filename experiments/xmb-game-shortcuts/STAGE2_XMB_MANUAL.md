# STAGE 2 — manual Sony PSX XMB registration test for ICO

**Stage 1 HARDWARE PASS. Stage 2 XMB UNTESTED.** The game booted from PP.ICO/BOOT.ELF with BOOT.CNF, using the unchanged Neutrino and DVR modules in PP.DVRISO. Stage 2 tests an actual Sony PSX Games-menu shortcut without starting PDIM.

## Critical APA warning

**wLaunchELF-R3Z Inject Header WRITES RAW APA ATTRIBUTE SECTORS** and can damage the console HDD. Only proceed after a full, verified, restorable sector-level backup. Never change Sony system/DVR partitions or PP.DVRISO. Do not use XMB uninstall/delete on this experimental shortcut. Never select Inject All. The test ZIP contains NO automatic APA writer.

## Stage 2 steps

1. Extract the Stage 2 ZIP on a FAT32 stick. Copy the CONTENTS of ICO_XMB_STAGE2_TEST to the USB root, so that mass0:/__Headers/PP.ICO/system.cnf and mass0:/__Headers/PP.ICO/icon.sys exist.
2. Verify that the existing Stage 1 PP.ICO partition is present. If not, STOP; do not create or format a new partition in this test.
3. Copy ONLY the updated PP.ICO-FILES/res/ CONTENTS to PP.ICO/res/ to replace test placeholders. Preserve the hardware-proven BOOT.ELF, BOOT.CNF, EXECUTE.ELF and EXECUTE.CNF.
4. Before writing APA, rerun mass0:/USB_TEST/ICOPFS.ELF to confirm ICO still boots. Abort if it does not.
5. Only after backup and preflight PASS: wLE-R3Z 4.80 -> HDDManager -> PP.ICO -> R1 -> Inject Header -> This partition only. Double-check the source mass0:/__Headers/PP.ICO and target PP.ICO.
6. Full cold boot into Sony XMB -> Games -> ICO. Check artwork, metadata, screenshots and game launch. Report results. Never reinject blindly or format partitions after a failure.

## Important limits

- Exact signed EXECUTE.ELF from official PDIM HDD1.2 is reused unchanged; it has NOT yet been verified from PP.ICO in XMB.
- Shared Neutrino stays on PP.DVRISO, and the game ISO stays at dvr_pfs0:/ISO/ICO.iso. No game ISO or Neutrino is duplicated.
- PAL SCES-50760 cover/metadata are examples. The actual serial in the user's ICO.iso is not verified.
- The real low-resolution ICO box art and gameplay/title images were sourced from [LaunchBox](https://gamesdb.launchbox-app.com/games/images/2128-ico), with sample details from [PSXDatacenter](https://psxdatacenter.com/psx2/games2/SCES-50760.html). Rights belong to their owners.

## Header references

- [Stage 2 system.cnf](stage2-templates/__Headers/PP.ICO/system.cnf) and [icon.sys](stage2-templates/__Headers/PP.ICO/icon.sys)
- [EXECUTE.CNF](stage2-templates/EXECUTE.CNF), [info.sys](stage2-templates/info.sys), [man.xml](stage2-templates/man.xml)
- [wLE-R3Z source](https://github.com/NathanNeurotic/wLaunchELF_R3Z/blob/master/src/hdd_header_injector.c)
- [SvenGDK XMB Manager v4.0](https://github.com/SvenGDK/PSX-XMB-Manager/blob/v4.0/PSX%20XMB%20Manager/Game/GameEditor.xaml.vb)

Main and official PDIM 1.2 release are unchanged.
