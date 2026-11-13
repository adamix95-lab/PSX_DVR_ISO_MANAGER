#!/usr/bin/env python3
"""N4D host-only 64-byte CSPRNG material: PRIVATE and ONE RUN only.

For experimental wolfCrypt DRBG on physical DESR. This file is not
an output artifact and must NOT be committed or uploaded to GitHub.
The console does not stop reuse of the file after reboot; new seed
required for every execution, including failed executions.
"""
import os
import secrets
import sys
from pathlib import Path

def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: py generate_n4d_seed.py E:/N4D.SEED", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    value = secrets.token_bytes(64)
    if len(value) != 64 or not any(value) or value[:32] == value[32:]:
        raise RuntimeError("OS CSPRNG failed")
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as out:
        out.write(value)
        out.flush()
        os.fsync(out.fileno())
    print(f"N4D: created exactly 64 RAW private bytes at {path}")
    print("One use only. Never share, print, upload or reuse this file.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
