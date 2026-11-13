N4F OFFLINE wolfSSL RSA cert verification for PSX DESR-7000.
NO NETWORK. NO TLS HANDSHAKE. NO HDD/USB/RTC/ART WRITES.
Place only V12_N4F_RSA_OFFLINE_RO.ELF on FAT32 USB.
Run ELF using wLE-R3Z 4.80 and report all three test codes.
Tests:
 X1->YR: ISRG Root X1 verifies cross-signed Root YR.
 YR->YR1: Root YR verifies YR1 intermediate certificate.
 YR1-TAMPER: intentionally corrupted signature MUST fail.
Fixed UTC 2026-11-13 is an OFFLINE test fixture, not TLS clock.
NO N4E.ENT, private seed or internet required.
wolfSSL GPLv3; PS2SDK AFL-2.0; NULD/ORBIT GPL-3.0 precedent.
Source: feature/v1.2-native-https, v1.2-n4f-rsa-offline.yml.
wolfSSL commit decea12e223869c8f8f3ab5a53dc90b69f436eb2.
Let's Encrypt website public certs commit:
736bf97e99c0238082e9a5c43d460af79d03873f.
