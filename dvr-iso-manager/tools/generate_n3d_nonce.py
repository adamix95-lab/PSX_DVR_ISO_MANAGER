#!/usr/bin/env python3
"""Produce one 32-byte nonce on the user's PC, NOT on GitHub Actions.

NEVER reuse N3D.NONCE. The console cannot detect reuse after reboot.
The created file is private and must not be uploaded to a public repo.
"""
import os
import secrets
import sys
from pathlib import Path

def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: py generate_n3d_nonce.py E:/N3D.NONCE", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    random_bytes = secrets.token_bytes(32)
    if len(random_bytes) != 32 or not any(random_bytes):
        raise RuntimeError("Invalid CSPRNG result")
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as file:
        file.write(random_bytes)
        file.flush()
        os.fsync(file.fileno())
    print(f"Created {path}, exactly 32 raw bytes, for ONE use only.")
    print("Do not share or reuse it. Remove it before generating a new one.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
