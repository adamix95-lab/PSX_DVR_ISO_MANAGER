N5A first HTTPS GET into RAM — PSX DESR-7000 READ-ONLY
Based on physical N4H TLS1.2 PASS with hardened portable FastMath.
This is an EXPERIMENTAL, manually provisioned diagnostic.
wolfSSL GPLv3, PS2SDK AFL-2.0; see NULD/ORBIT license precedent.
Public fixed response is on the test branch at:
https://raw.githubusercontent.com/adamix95-lab/PSX_DVR_ISO_MANAGER/test/n5a-direct-download/diagnostics/n5a/PROBE.TXT
Expected 39 bytes: "PSX-DVR-ISO-MANAGER N5A HTTPS RAM PASS\n".
This file is fetched only inside verified TLS after two strict
Roughtime-backed LOW/HIGH certificate validation checks.
Only HTTP 200 with EXACT payload accepted; no redirects.
Hard limits: 16KiB total HTTP response, 8KiB headers,
256B decoded body, 40 reads, each ≤1024B in RAM.
No USB/HDD/PFS/RTC/ART writes. No game boot or XMB install.
Put in FAT32 USB root:
 - V12_N5A_HTTPS_RAM_RO.ELF
 - N4E.CA (retain previous public variant 02_X1_YR_YR1)
 - N4E.ENT (PRIVATE, EXACT 160 bytes, NEW every run)
Generate N4E.ENT on Windows 11 / Python 3:
   py generate_n5a_entropy.py E:/N4E.ENT
This generator replaces old N4E.ENT atomically with fresh OS
CSPRNG bytes; never upload, display or reuse its contents.
Leave existing PP.DVRISO12/V12_NET.CNF unchanged.
Start ELF from wLE-R3Z 4.80 with Ethernet connected.
Success: ROUGHTIME SIGNED UTC: PASS;
TLS1.2 CHAIN/HOST/DATE: PASS; TWO BOUND CHECKS: PASS;
HTTPS GET 200 RAM: PASS; PAYLOAD 39 BYTES: MATCH; RET=0.
On failure, photograph entire final screen including
HTTP parser/wolfSSL code, TLS bound and NET counters.
NO production HTTPS clock: signed time margin is not yet
formally bounded by monotonic stopwatch/anti-replay on reboot.
Stable 1.0 and XMB untouched; no covers downloaded to HDD.
Source branch feature/v1.2-native-https, N5A workflow.
wolfSSL pinned decea12e223869c8f8f3ab5a53dc90b69f436eb2
PS2SDK pinned d41ea62946e53d9a0c8788bb9eb8c8faf488b559
