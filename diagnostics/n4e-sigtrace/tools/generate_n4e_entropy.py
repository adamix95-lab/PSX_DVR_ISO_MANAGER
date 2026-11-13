#!/usr/bin/env python3
"""N4E laboratory TLS diagnostic: fresh host OS CSPRNG material.

Bytes 0..31: STRICT TXRyan request nonce.
Bytes 32..95: first TLS handshake seed.
Bytes 96..159: second TLS handshake seed.
Private file; NEVER reuse after any boot or failure. No on-device
replay protection. No secrets are committed or transmitted to GitHub.
"""
import os
import secrets
import sys
from pathlib import Path

def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: py generate_n4e_entropy.py E:/N4E.ENT", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    data = secrets.token_bytes(160)
    a, b, c = data[:32], data[32:96], data[96:]
    if len(data) != 160 or not all(map(any, (a, b, c))) or b == c:
        raise RuntimeError("CSPRNG failure")
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
    print(f"Created {path}: 160 private binary bytes, ONE execution only.")
    print("NEVER reuse, share, print, upload or commit N4E.ENT.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
