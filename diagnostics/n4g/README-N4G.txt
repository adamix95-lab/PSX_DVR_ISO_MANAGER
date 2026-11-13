N4G OFFLINE wolfSSL portable FastMath RSA cert verification.
N4F physical DESR: authentic X1->YR and YR->YR1 signatures
FAIL with ASN_SIG_CONFIRM_E -155; host default wolfSSL PASS.
This build changes ONLY wolfSSL math: USE_FAST_MATH,
SP_MATH_ALL=no, ASM=no, FP_MAX_BITS=8192.
Cryptographic verification remains strict and hardening stays ON.
NO NETWORK. NO TLS HANDSHAKE. NO HDD/USB/RTC/ART WRITES.
Copy V12_N4G_FASTMATH_RSA_RO.ELF to FAT32 USB; launch wLE-R3Z 4.80.
Expect A and B PASS, C tampered signature REJECT, RET=0.
The fixed 2026-11-13 UTC is a PUBLIC OFFLINE test fixture,
NOT a valid live TLS clock.
No N4E.ENT, N4E.CA, Ethernet or HDD write needed.
wolfSSL GPLv3; PS2SDK AFL-2.0; compare NULD GPL-3.0.
Pinned wolfSSL decea12e223869c8f8f3ab5a53dc90b69f436eb2.
Public Let's Encrypt 736bf97e99c0238082e9a5c43d460af79d03873f.
