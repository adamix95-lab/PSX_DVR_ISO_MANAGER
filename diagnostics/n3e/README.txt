N3E TXRyan signed UTC diagnostic, strictly READ ONLY; NOT TLS READY.
1. Copy V12_N3E_TXRYAN_UTC_RO.ELF to FAT32 USB.
2. Generate PRIVATE FRESH nonce per attempt on Windows with Python 3:
   py tools/generate_n3d_nonce.py E:/N3E.NONCE
   Replace E: with your USB drive letter; old file must be deleted.
3. Keep existing PP.DVRISO12/V12_NET.CNF, run ELF from wLE-R3Z 4.80.
4. Photograph signed UTC MIDP, radius, lower/upper and RET.
5. RET=0: verified server-signed UTC interval (DIAGNOSTIC only).
6. -508: signature/protocol fail; -515: signed-time policy fail.
7. NEVER reuse N3E.NONCE, including after reboot.
8. No trusted TLS time, TLS entropy, HTTPS, covers, RTC or disk writes.
