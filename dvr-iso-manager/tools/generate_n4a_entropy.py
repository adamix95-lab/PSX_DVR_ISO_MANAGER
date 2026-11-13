#!/usr/bin/env python3
"""N4A diagnostic ONLY: generate 96 independent OS-CSPRNG bytes on the PC.

Roughtime uses first 32B; 64B are reserved for a FUTURE wolfSSL seed hook,
which this ELF DOES NOT IMPLEMENT. Never commit, upload, display or reuse
this file. Use a fresh one for each test, including after any console reboot.
"""
import os
import secrets
import sys
from pathlib import Path

def main() -> int:
    if len(sys.argv)!=2:
        print("Usage: py generate_n4a_entropy.py E:/N4A.ENT",file=sys.stderr)
        return 2
    path=Path(sys.argv[1])
    data=secrets.token_bytes(96)
    if len(data)!=96 or not any(data[:32]) or not any(data[32:]):
        raise RuntimeError("Host CSPRNG failed")
    fd=os.open(path,os.O_WRONLY | os.O_CREAT | os.O_EXCL,0o600)
    with os.fdopen(fd,"wb") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    print(f"Created {path}: 96 RAW random bytes, ONE RUN ONLY.")
    print("Do not reuse, upload or share the file. The ELF does NOT enable TLS.")
    return 0

if __name__=="__main__":
    sys.exit(main())
