#!/usr/bin/env python3
"""Create a one-run N6C.ENT for Sony PSX DESR-7000 experimental N6.

32 bytes Roughtime nonce + 1 * (4 * 64 bytes) independent TLS DRBG seeds.
Only a USB file target is accepted. Overwrites the prior local ENT using an
atomic replace, NEVER uploads or prints its cryptographic content.
"""
import os
from pathlib import Path
import secrets
import sys
import tempfile

LENGTH = 32 + 1 * 4 * 64  # 288 bytes

def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: py generate_n6c_entropy.py I:/N6C.ENT", file=sys.stderr)
        return 2
    target = Path(sys.argv[1])
    if target.name.upper() != "N6C.ENT":
        print("Output basename must be N6C.ENT", file=sys.stderr)
        return 2
    if not target.parent.is_dir():
        print("Target drive/path does not exist", file=sys.stderr)
        return 2
    data = secrets.token_bytes(LENGTH)
    # Reject any degenerate OS CSPRNG output; no deterministic fallback.
    chunks = [data[32 + 64*i:32 + 64*(i+1)] for i in range(4)]
    if not any(data[:32]) or any(not any(x) for x in chunks) or len(set(chunks)) != 4:
        print("OS entropy failed validation", file=sys.stderr)
        return 3
    temp = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=target.parent, prefix="N6CENT_", suffix=".TMP", delete=False
        ) as f:
            temp = Path(f.name)
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        os.replace(temp, target)
        assert target.stat().st_size == LENGTH
    finally:
        if temp is not None and temp.exists():
            temp.unlink()
    print("OK: fresh 288-byte N6C.ENT created (private; DO NOT reuse or upload)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
