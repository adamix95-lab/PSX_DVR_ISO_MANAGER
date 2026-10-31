PSX DVR ISO Manager v0.8 — complete USB package

NEW INSTALLATION
1. Extract the ZIP. Copy the entire DESR_DVR folder to the root
   directory of your USB drive. Do not scatter files into other folders.
2. Launch mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF via wLE or UDPFS.
3. Put ISO games in dvr_pfs0:/ISO on the DVR partition.
   The Manager never writes to the DVR partition or ISO files.
4. Press TRIANGLE to generate COVERS.IDX and COVER_SCAN.LOG on USB.
   To download missing covers, run the separate PSX DVR ART Server v0.8
   Windows EXE from the same GitHub Release.
5. Edit NET_PROBE.CNF for YOUR private LAN. The default addresses are
   only a working example: console 192.168.0.240,
   Windows PC 192.168.0.122, HTTP port 8000.
   Run ipconfig /all in Windows. NETMASK = Subnet Mask,
   GATEWAY = Default Gateway, SERVER_IP = PC IPv4 Address.
   IP in NET_PROBE.CNF is a free, manually assigned console IP.

UPDATING AN EXISTING USB INSTALLATION
Back up the ENTIRE existing DESR_DVR directory before copying.
Do not unintentionally replace your BOOT.CNF, BOOT_RECOVERY.CNF,
NET_PROBE.CNF, ART/, COVERS.IDX or COVER_SCAN.LOG.
Keep your working OSDMenu/Neutrino configuration.

BUNDLE CONTENTS
All original OSDMenu/Neutrino modules and configs come unchanged
from this repository's published v0.3.0 USB bundle, except that
BOOT.elf is named BOOT.ELF to match the Manager path.
The old Manager ELF is replaced with the official v0.8 release ELF.
Added an empty ART directory, sample NET_PROBE.CNF and this guide.

ISO images and cover art are NOT included. Covers can be fetched via PC.
Full instructions: README.md in the GitHub repository.
