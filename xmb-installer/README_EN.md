# PSX DVR ISO MANAGER v1.0.0 — XMB launch

**Hardware PASS:** on a Sony PSX DESR-7000 the **PSX DVR ISO MANAGER** Games entry launches the HDD Manager, including the selected PDIM HDD+DVD icon. The final Manager also works with the clean `1.0 HDD` heading. HDD ISO game launch, cover downloads and forced English were tested separately.

**This is manual APA header injection using wLaunchELF_R3Z, not a one-click or disk-formatting installer.** It requires an existing `hdd0:PP.DVRISO` APA/PFS partition with the complete HDD 1.0 package installed.

## Launch path

```text
XMB Games → PP.DVRISO APA: BOOT2=pfs:/EXECUTE.ELF
  → EXECUTE.ELF (KELF containing the unchanged OSDMenu Launcher)
  → EXECUTE.CNF
  → hdd0:PP.DVRISO:pfs:/DVR_ISO_MANAGER_HDD.ELF
  → BOOT.CNF / BOOT.ELF → Neutrino HDD FIX95 → DVR game
```

The optional XMB ZIP includes `__Headers/PP.DVRISO/` with `system.cnf`, `icon.sys`, `info.sys`, and two indexed PNG8 icons: `jkt_002.png` **76×108** for XMB, `jkt_001.png` **256×256** for PSBBN. It also includes the two PFS files `EXECUTE.ELF` and `EXECUTE.CNF`, the up-to-date Manager as an optional file, and licensing/source notes.

## Safe manual installation

1. **Back up the APP partition's APA header and important data; verify you can restore it.** Injecting a header modifies disk metadata.
2. Extract the ZIP to FAT32 USB with `__Headers/PP.DVRISO/` directly under the USB root.
3. Via wLE, copy only `EXECUTE.ELF` and `EXECUTE.CNF` from `PFS_COPY_TO_PP.DVRISO/` to the root of the existing `PP.DVRISO` partition. Do not replace working `BOOT.ELF`, `BOOT.CNF`, Neutrino, `config/`, `modules/` or `ART/`.
4. In wLaunchELF_R3Z, select **only PP.DVRISO → R1 → Inject Header → This partition only** from USB. Never format the HDD, create/expand partitions, or inject all partitions.
5. Restart into XMB Games and select **PSX DVR ISO MANAGER**.

**For future upgrades:** when XMB already launches the Manager, replace only `DVR_ISO_MANAGER_HDD.ELF`; do **not** repeat APA header injection. Deleting EXECUTE files alone cannot undo the metadata change — restore the backed-up APA header to revert it.

Credits: [wLaunchELF_R3Z](https://github.com/NathanNeurotic/wLaunchELF_R3Z), [RepairBox](https://github.com/DanielFergisz/RepairBox-PSX-HDD-Setup), [OSDMenu](https://github.com/pcm720/OSDMenu), [hdl-dump KELF GPLv2](https://github.com/DanielFergisz/RepairBox-PSX-HDD-Setup/tree/main/third_party/hdl-dump). The XMB package includes the hdl-dump GPLv2 license and relevant KELF template source.

[Polski](README_PL.md) · [Release notes](../docs/RELEASE_NOTES_v1.0.0_EN.md)
