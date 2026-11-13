#!/usr/bin/env python3
"""N4F pinned official Let's Encrypt certificates to EE/host C arrays.

Usage: python3 n4f_embed_certificates.py letsencrypt-website-dir output-dir
No secret material, no network call, deterministic output.
"""
from pathlib import Path
import base64
import re
import sys

SRC = {
    "x1_pem": "static/certs/isrgrootx1.pem",
    "yr_pem": "static/certs/gen-y/root-yr.pem",
    "yr_by_x1_der": "static/certs/gen-y/root-yr-by-x1.pem",
    "yr1_der": "static/certs/gen-y/int-yr1.pem",
}

def decode_pem(blob: bytes) -> bytes:
    match = re.fullmatch(
        rb"\s*-----BEGIN CERTIFICATE-----\s+([a-zA-Z0-9+/\s=]+)"
        rb"-----END CERTIFICATE-----\s*", blob)
    if not match:
        raise ValueError("Invalid cert-only PEM")
    return base64.b64decode(re.sub(rb"\s+", b"", match.group(1)), validate=True)

def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("Usage: n4f_embed_certificates.py LE_DIR OUT_DIR")
    root, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    certs = {}
    for name, path in SRC.items():
        raw = (root / path).read_bytes()
        assert b"-----BEGIN CERTIFICATE-----" in raw
        certs[name] = decode_pem(raw) if name.endswith("_der") else raw
    lines = ["#include <stdint.h>", '#include "n4f_fixtures.h"']
    headers = [
        "#ifndef N4F_FIXTURES_H", "#define N4F_FIXTURES_H",
        "#define N4F_YR1_DER_CAP 4096U"
    ]
    for name, data in certs.items():
        ident = "n4f_" + name
        encoded = ",".join(f"0x{byte:02x}" for byte in data)
        lines.append(f"const unsigned char {ident}[] = {{{encoded}}};")
        lines.append(f"const unsigned {ident}_len = {len(data)}U;")
        headers.append(f"extern const unsigned char {ident}[];")
        headers.append(f"extern const unsigned {ident}_len;")
    headers.append("#endif")
    (out / "n4f_fixtures.h").write_text("\n".join(headers)+"\n")
    (out / "n4f_fixtures.c").write_text("\n".join(lines)+"\n")
    print("N4F public fixtures:", {k: len(v) for k,v in certs.items()})
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
