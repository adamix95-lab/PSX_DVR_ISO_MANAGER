#!/usr/bin/env python3
"""Write a fresh private N6G50.ENT for one DESR-7000 batch run."""

import os
from pathlib import Path
import secrets
import sys
import tempfile


ATTEMPTS = 50 * 2
STRIDE = 32 + 64 + 64
LENGTH = ATTEMPTS * STRIDE


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: py generate_n6g_batch_entropy.py I:/N6G50.ENT", file=sys.stderr)
        return 2
    target = Path(sys.argv[1])
    if target.name.upper() != "N6G50.ENT" or not target.parent.is_dir():
        print("Choose N6G50.ENT on the test USB drive", file=sys.stderr)
        return 2

    data = secrets.token_bytes(LENGTH)
    nonces = [data[i:i + 32] for i in range(0, LENGTH, STRIDE)]
    seeds = [data[i + j:i + j + 64] for i in range(0, LENGTH, STRIDE) for j in (32, 96)]
    if (len(set(nonces)) != ATTEMPTS or len(set(seeds)) != ATTEMPTS * 2 or
            any(not any(chunk) for chunk in nonces + seeds)):
        print("OS entropy failed validation", file=sys.stderr)
        return 3

    temp = None
    try:
        with tempfile.NamedTemporaryFile(dir=target.parent, prefix="N6G50_", suffix=".TMP",
                                         delete=False) as output:
            temp = Path(output.name)
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temp, target)
    finally:
        if temp is not None and temp.exists():
            temp.unlink()
    print(f"OK: fresh {LENGTH}-byte N6G50.ENT (private, use once)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
