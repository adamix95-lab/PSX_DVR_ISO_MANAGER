# PDIM XMB Game Shortcuts — Stage 1: isolated PP.ICO PFS experiment

**Status (2026-11-22):** Stage 0 on a real Sony PSX DESR-7000 **PASS**: both `ICOSYS.ELF` and `ICOEN.ELF` started `dvr_pfs0:/ISO/ICO.iso`, without executing the PDIM manager. The new **Stage 1 remains a proposed console test**, not a hardware PASS.

## Goal

Each game will eventually have its own APA/PFS partition and independent XMB art/metadata and `BOOT.CNF`. The signed XMB entry and local OSDMenu launcher should load a shared Neutrino HDD FIX95 from `hdd0:PP.DVRISO`. **Never** duplicate game ISO files, Neutrino or DVR IOP modules across game partitions.

```text
XMB -> PP.ICO/EXECUTE.ELF -> PP.ICO/EXECUTE.CNF
    -> PP.ICO/BOOT.ELF + PP.ICO/BOOT.CNF
    -> PP.DVRISO/neutrino_hdd.elf
    -> dvr_pfs0:/ISO/ICO.iso
```

Stage 1 tests **only the second half**, with a USB OSDMenu launcher replacing the untested XMB handoff:

```text
USB_TEST/ICOPFS.ELF + ICOPFS.CNF
    -> hdd0:PP.ICO:pfs:/BOOT.ELF (argv[1]=hdd0:PP.ICO/BOOT.CNF)
    -> PP.DVRISO/neutrino_hdd.elf
    -> ICO.iso in original DVR filesystem
```

The `PP.ICO/BOOT.CNF` uses an **absolute `path=`** pointing to the existing `PP.DVRISO` Neutrino. OSDMenu's quickboot handler supports this and accepts an explicit absolute HDD CNF argument; this is **supported by source analysis, not yet hardware-tested in this two-stage chain**. Both `BOOT.ELF` and `ICOPFS.ELF` are bit-for-bit copies of the already used OSDMenu ELF (SHA-256 `64e45abcd92b1096dc490d38fc0a20ac9959004a4286c4cb661f00019423b3ae`). `EXECUTE.ELF` is the unchanged signed KELF from PDIM HDD 1.2, but is **not yet verified to launch from a different partition**.

## Files and CNF templates

- [BOOT_SYSTEM.CNF](BOOT_SYSTEM.CNF) — default shared Neutrino
- [BOOT_ENGLISH.CNF](BOOT_ENGLISH.CNF) — forced English
- [EXECUTE.CNF](EXECUTE.CNF) — future XMB KELF -> local OSDMenu and explicitly passed BOOT.CNF
- [ICOPFS.CNF](ICOPFS.CNF) — Stage 1 USB test -> local OSDMenu + BOOT.CNF
- [Polish test guide](README_PL.md)

The user's tested source filename is exactly `ICO.iso`, but the game's **serial/region has not been verified**. The Stage 1 ZIP contains only original **test placeholder** PNG8 art and PAL ICO `SCES-50760` example metadata, not actual copyrighted game screenshots.

## Safety policy

**Creating a new PFS partition modifies the Sony PSX HDD APA layout and can corrupt the DVR/system HDD if something goes wrong.** Only the hardware owner can choose to create `PP.ICO`, manually with wLaunchELF-R3Z, **after a full, verified, restorable sector-level backup**. Never delete, repartition, resize or format `PP.DVRISO` or any existing Sony system/DVR partition. If `PP.ICO` already exists, fail closed and stop.

There is **no APA sector writer, no XMB header injection, no automatic partition creation and no changes to release 1.2** in this stage.

After a confirmed Stage 1 boot from `PP.ICO`, Stage 2 can introduce reviewed APA/XMB registration, metadata, image validation and safe rollback for each game. Never assume success on USB alone establishes XMB compatibility.

## References

- [OSDMenu quickboot source](https://github.com/pcm720/OSDMenu/blob/main/launcher/src/handler_quickboot.c)
- [OSDMenu PFS loader](https://github.com/pcm720/OSDMenu/blob/main/launcher/src/handler_pfs.c)
- [PSX XMB Manager v4.0 metadata/image generation](https://github.com/SvenGDK/PSX-XMB-Manager/blob/v4.0/PSX%20XMB%20Manager/Game/GameEditor.xaml.vb)
- [ICO PAL (sample metadata only)](https://psxdatacenter.com/psx2/games2/SCES-50760.html)
- [Official PDIM 1.2 HDD release](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2)
