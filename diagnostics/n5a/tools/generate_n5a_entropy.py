#!/usr/bin/env python3
"""N5A diagnostics: safely replace PRIVATE N4E.ENT with fresh 160 OS CSPRNG bytes.

Windows 11 PowerShell 5.1 / Python 3 compatible. The output file is
created on the same filesystem as a temporary file and atomically replaced
with a fresh nonce+two TLS seed regions. Never print or upload entropy.
Every N5A execution, even after a failure/reboot, requires a fresh file.
"""
import os
import secrets
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: py generate_n5a_entropy.py E:/N4E.ENT", file=sys.stderr)
        return 2
    target = Path(sys.argv[1])
    if target.name.upper() != "N4E.ENT":
        print("Refusing destination other than N4E.ENT", file=sys.stderr)
        return 2
    if not target.parent.is_dir():
        print(f"USB destination not found: {target.parent}", file=sys.stderr)
        return 2

    blob = secrets.token_bytes(160)
    if (len(blob) != 160 or not all((blob[:32], blob[32:96], blob[96:]))
            or blob[32:96] == blob[96:]):
        raise RuntimeError("Host OS CSPRNG did not provide valid input")

    temporary = None
    try:
        fd, name = tempfile.mkstemp(prefix="N5A_", suffix=".TMP", dir=str(target.parent))
        temporary = Path(name)
        with os.fdopen(fd, "wb") as writer:
            writer.write(blob)
            writer.flush()
            os.fsync(writer.fileno())
        os.replace(str(temporary), str(target))
        temporary = None
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()

    if target.stat().st_size != 160:
        raise RuntimeError("N4E.ENT length mismatch after replace")
    print(f"OK: N4E.ENT replaced with 160 NEW private binary bytes at {target}")
    print("One run only. Never reuse, upload or copy this file to GitHub.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
