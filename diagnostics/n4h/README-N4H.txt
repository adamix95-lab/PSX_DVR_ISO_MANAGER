N4H FastMath TLS1.2 STRICT PEER DIAGNOSTIC DESR-7000
N4G physical RSA X509 on DESR: correct X1->YR PASS,
correct YR->YR1 PASS, corrupt YR1 signature REJECT, RET=0.
N4H changes the N4E SIGTRACE wolfSSL math to that N4G
hardened portable FastMath backend. Certificate verification
stays fully ENABLED. Never treat diagnostic as production HTTPS.
wolfSSL GPLv3, PS2SDK AFL-2.0; compare NULD/ORBIT GPL-3.0.
No user private keys or random bytes are distributed.
Files to put at FAT32 USB root:
- V12_N4H_FASTMATH_TLS_RO.ELF
- N4E.CA (PUBLIC; for controlled comparison use the previous
  02_X1_YR_YR1 CA variant that was used for failing SIGTRACE;
  the bundled Mozilla roots are an alternative baseline)
- N4E.ENT (PRIVATE, generate on PC for exactly ONE execution)
Command on Windows: py generate_n4e_entropy.py E:/N4E.ENT
Replace E: with FAT32 USB drive letter. Remove old N4E.ENT first.
PFS PP.DVRISO12/V12_NET.CNF must remain present/unchanged.
Launch from wLE-R3Z 4.80. No SELECT.
If RET=-649, photograph all VERIFY calls/fail, FIRST/LAST CERT code/depth, SSL err, TCP, IO, RNG.
Only sends Roughtime UDP to time.txryan.com:2002 then TLS1.2
handshakes to raw.githubusercontent.com:443. Sends NO HTTP.
Trust uses user-provisioned CA bundle and pinned TXRyan root.
Certificate CA/domain/date checked twice with two TLS seeds,
at padded signed UTC lower/upper bounds. This is DIAGNOSTIC.
It does not prove persistent anti-replay/monotonic timing and
must NOT be reused as trusted production TLS clock.
No HDD, USB, PFS, RTC, ART, game/XMB writes.
Source: feature/v1.2-native-https N4H source and workflow.
wolfSSL source: wolfSSL/wolfssl commit
decea12e223869c8f8f3ab5a53dc90b69f436eb2
PS2SDK source: ps2dev/ps2sdk commit
d41ea62946e53d9a0c8788bb9eb8c8faf488b559
