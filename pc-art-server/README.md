# PSX DVR ART Server v0.8 — Windows application

[PSX DVR ISO Manager](../README.md) · [Download v0.8](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/v0.8)

**PSX DVR ART Server** is the Windows companion application for **PSX DVR ISO Manager v0.8** running on Sony PSX DESR consoles. It provides a local HTTP server and fetches missing PS2 artwork by **Game ID** over HTTPS. The console receives a validated PNG over your private LAN and saves it to USB itself.

The app is available as a standalone **`.exe`**, built with Python, Tkinter, Pillow and PyInstaller. You do not need to install Python to use the Windows executable.

## Download and start

1. Download **[PSX-DVR-ART-Server-v0.8.exe](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/v0.8/PSX-DVR-ART-Server-v0.8.exe)**.
2. Open the EXE on Windows. It is an unsigned application, so Windows SmartScreen may request confirmation.
3. Enter your **PC IPv4 Address**, **Console IPv4 Address** and **HTTP Port**. For example, the tested setup used `192.168.0.122`, `192.168.0.240` and port `8000`. Use addresses appropriate for **your network**.
4. Click **Start HTTP Server**. Allow inbound connections in Windows Firewall only for your **private network**, never the public internet.
5. On the PSX, open Manager v0.8 and press **Triangle** to scan the ISO games. If artwork is missing, choose **X: DOWNLOAD** to request it from the PC. Press **Circle (O)** to skip; you can retry by scanning again.

The Windows app shows a request log with downloaded Game IDs and source information. It does not directly access the console's DVR partition or USB drive.

## Configure the console network

Create or edit `mass0:/DESR_DVR/NET_PROBE.CNF` on the USB drive:

```ini
IP=192.168.0.240
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
SERVER_IP=192.168.0.122
SERVER_PORT=8000
```

To determine your PC's network settings, open **Command Prompt** or **PowerShell** and run `ipconfig /all` (with a **slash**, not `ipconfig -all`). Look for the network adapter used to connect to your PSX.

| Setting | Meaning |
| --- | --- |
| `IP` | An unused, **manually assigned static IPv4 address for the PSX DESR**. It must be on the PC's subnet and must **not** equal the PC address. |
| `NETMASK` | **Subnet Mask** from the active Windows network adapter shown by `ipconfig /all`. |
| `GATEWAY` | **Default Gateway**, typically the router's address, shown by `ipconfig /all`. |
| `SERVER_IP` | The **IPv4 Address of the Windows PC** running the ART Server, shown by `ipconfig /all`. |
| `SERVER_PORT` | The port configured in the Windows application (**HTTP Port**), usually `8000`. It is not supplied by `ipconfig`. |

Do not enable internet/router port forwarding for this service. If the PC's IP address changes, update both `SERVER_IP` and the PC IPv4 Address in the application.

## Cover Sources

The application checks sources in this order:

| Priority | Source | Artwork |
| --- | --- | --- |
| 1 | **Local PNG cache** | Previously downloaded and validated covers |
| 2 | **Primary — OPL Manager Archive (Archive.org)** | Preferred PS2 box-cover artwork, usually portrait-shaped |
| 3 | **Fallback — CosmicScale PSBBN Art Database (GitHub)** | Alternative game artwork, often square; used only if the primary source fails or does not provide a valid PNG |

**Primary:** The OPL Manager collection `OPLM_ART_2024_09` on Archive.org, also referenced by the CosmicScale installer.

```text
https://archive.org/download/OPLM_ART_2024_09/OPLM_ART_2024_09.zip/PS2/SLES_538.86/SLES_538.86_COV.png
```

**Fallback:** The separate [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database), which stores game artwork indexed by PS2 Game ID.

```text
https://raw.githubusercontent.com/CosmicScale/psbbn-art-database/main/art/SLES_538.86.png
```

Square fallback artwork is proportionally fitted to a **140 × 200 RGB PNG**, using black borders instead of stretching. This means a fallback image may not resemble the original portrait box cover. If neither source has an image for a Game ID, the PC responds with HTTP 404 and the console does not save an invalid file.

The Windows log shows messages such as `HTTPS OPL`, `HTTPS PSBBN FALLBACK` and `SOURCE <GameID>: OPL` or `SOURCE <GameID>: PSBBN`.

**Hardware verification:** A Sony PSX DESR-7000 successfully downloaded and displayed **40/40 covers** using the v0.8 PC application, and REZ launched in English. That test did not separately establish whether the fallback database was used; fallback behavior is also covered by automated tests.

## Cache, format and safety

The local PNG cache is located at:

```text
%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache
```

The server accepts HTTP requests only from the configured console IPv4 address, accepts only strict Game ID filenames, and restricts upstream requests to the configured HTTPS artwork sources. Remote image downloads are limited to **4 MiB** and normalized output PNG files to **256 KiB** at **140 × 200 RGB**.

The console's Manager validates the received HTTP/PNG data, saves only **new** artwork under `mass0:/DESR_DVR/ART/`, then rebuilds `COVERS.IDX`. Existing USB covers are not overwritten. If an incomplete `.PART` file remains, inspect it manually rather than overwriting an unknown file.

## Run from Python (optional)

From the repository root:

```powershell
py -m pip install -r pc-art-server/requirements.txt
py pc-art-server/psx_dvr_art_server.py
```

Python 3.10+ and Pillow are required. The standalone EXE is built and tested by [GitHub Actions](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions).

## Troubleshooting

- **No console connection:** check the server status, both IPv4 addresses, `NET_PROBE.CNF`, the Ethernet link and Windows Firewall's private-network rule.
- **No artwork found:** check the server log for upstream errors or `NOT FOUND`. A fallback fetch is attempted only after the preferred OPL source fails.
- **A cover already exists:** the Manager intentionally does not overwrite existing PNG/JPG files. The Windows cache also preserves existing files.
- **The application works but covers do not appear:** run Triangle again and inspect the USB index; check for failed or partial downloads.

Artwork rights belong to their respective copyright owners. Respect the original sources' licensing and terms.
