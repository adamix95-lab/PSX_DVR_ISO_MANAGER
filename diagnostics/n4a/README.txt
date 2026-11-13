N4A READ-ONLY: 96 byte dual-range host CSPRNG file diagnostic.
This ELF does NOT seed wolfSSL, implement TLS, or provide a TLS clock.
1. Copy V12_N4A_ENTROPY_RO.ELF to FAT32 USB.
2. Use Python 3 on PC: py tools/generate_n4a_entropy.py E:/N4A.ENT
   Replace E: with USB letter; file must be EXACTLY 96 raw bytes.
3. Keep PP.DVRISO12/V12_NET.CNF unchanged; launch wLE-R3Z 4.80.
4. RET=0: signed UTC and USB input accepted, HTTPS still DISABLED.
5. ALWAYS regenerate private N4A.ENT before another attempt or reboot.
6. -510..-514 USB; -516 invalid ranges; -520..-523 network;
   -508 strict cryptography; -515 signed UTC policy.
7. No PFS, USB, HDD, RTC, ART, game launch or XMB writes.
