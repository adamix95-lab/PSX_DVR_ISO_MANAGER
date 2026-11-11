PSX DVR ISO MANAGER 1.2 N3D TXRyan USB nonce probe
READ-ONLY DIAGNOSTIC, NOT A RELEASE; NO TRUSTED UTC OR HTTPS
- Download V12_N3D_TXRYAN_USB_RO.ELF directly from this branch.
- Place ELF on FAT32 USB; keep existing PP.DVRISO12/V12_NET.CNF.
- On Windows with Python 3: py generate_n3d_nonce.py E:/N3D.NONCE
  Replace E: with FAT32 pendrive's correct drive letter.
- N3D.NONCE must be exactly 32 raw random bytes; keep it private.
- Boot ELF through wLE-R3Z 4.80, report the on-screen RET code.
- New PC-generated nonce REQUIRED for EVERY attempt; never reuse.
- ELF does not write to HDD, RTC, PFS, USB, APA, or XMB.
- No XMB installation, game boot, TLS or artwork downloading.
- RET=0 checks strict signatures and Merkle with USB random nonce;
  NOT trusted UTC: this one-use discipline is NOT enforced on reboot.
