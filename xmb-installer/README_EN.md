# PSX DVR ISO MANAGER — experimental HDD XMB installer (first test)

[Instrukcja po polsku](README_PL.md)

**Not yet hardware-tested.** Isolated development branch `feature/hdd-xmb-installer-test`. The existing PSX DESR-7000 HDD 1.0 build remains unchanged and hardware-validated (44/44 cover downloads, two games, forced English).

This test adds an XMB Games entry named **PSX DVR ISO MANAGER** to the **already existing** `hdd0:PP.DVRISO` APA/PFS partition, using wLaunchELF_R3Z's **This partition only** header injector. It never creates or formats partitions. **Back up the partition's APA header and data before injection**.

The ZIP has two parts: (1) `__Headers/PP.DVRISO/` contains `system.cnf`, `icon.sys`, `info.sys`, and indexed PNG assets, injected with wLaunchELF_R3Z; (2) `PFS_COPY_TO_PP.DVRISO/` contains `EXECUTE.ELF` (an HDD-signed KELF envelope of the original OSDMenu Launcher) and `EXECUTE.CNF` (a Quickboot path to the existing HDD Manager). The original `BOOT.ELF`, `BOOT.CNF`, Neutrino, NETFIX, game images, modules, and configuration files are never modified. A byte-identical Manager is provided **only if missing**.

The XMB icon must be **76×108 8-bit indexed palette PNG with RGBA transparency data**, named `jkt_002.png` (not 74×108). The alternate PSBBN cover is `jkt_001.png` (256×256).

**Unverified dependency:** XOSD must load a signed KELF stored under `EXECUTE.ELF` as indicated by `BOOT2 = pfs:/EXECUTE.ELF`, and provide an argv[0] usable by the unchanged OSDMenu Launcher's Quickboot parser. Failure must be investigated in this new launch path, not worked around by modifying the proven HDD Manager.

Sources: [RepairBox v1.3](https://github.com/DanielFergisz/RepairBox-PSX-HDD-Setup), [wLaunchELF_R3Z](https://github.com/NathanNeurotic/wLaunchELF_R3Z), [OSDMenu Launcher](https://github.com/pcm720/OSDMenu/tree/main/launcher), [hdl-dump GPLv2](https://github.com/DanielFergisz/RepairBox-PSX-HDD-Setup/tree/main/third_party/hdl-dump), [PFS BatchKit](https://github.com/GDX-X/PFS-BatchKit-Manager).

For detailed procedures and rollback safety, see the Polish README.
