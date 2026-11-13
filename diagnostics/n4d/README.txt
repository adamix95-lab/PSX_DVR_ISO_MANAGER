N4D — PSX DVR ISO MANAGER v1.2 experimental wolfCrypt DRBG test
wolfSSL: GPLv3. PS2SDK: Academic Free License 2.0 (AFL-2.0).
NULD (formerly ORBIT) uses statically linked wolfSSL on PS2 and
describes its launcher as GPL-3.0, but this is not a ruling on
license compatibility for every dependency of our build.
This test does not make HTTPS requests or write to console HDD.
1. Copy V12_N4D_WOLFCRYPT_RNG_RO.ELF to FAT32 USB.
2. On PC with Python 3 generate fresh private 64-byte binary:
   py generate_n4d_seed.py E:/N4D.SEED
   Replace E: with your actual USB drive letter.
3. In wLE-R3Z 4.80 launch the ELF. Photograph final RET.
4. NEVER REUSE N4D.SEED, including after failed boot/reboot.
5. No PFS/USB/HDD/RTC writes, no TLS/HTTPS or cover changes.
Original corresponding source revision: feature/v1.2-native-https
wolfSSL source: wolfSSL/wolfssl commit
decea12e223869c8f8f3ab5a53dc90b69f436eb2 (v5.8.2-stable)
PS2SDK source: ps2dev/ps2sdk commit
d41ea62946e53d9a0c8788bb9eb8c8faf488b559
Build commands: .github/workflows/v1.2-n4d-wolfcrypt-rng.yml
