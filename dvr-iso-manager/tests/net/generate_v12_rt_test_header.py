#!/usr/bin/env python3
"""Generate PUBLIC test fixture bytes for host/PS2 Ed25519 verifier checks.

Input: pinned Apache-2.0 upstream Cloudflare draft-08 JSON test fixture.
Public root below derives from the fixture's PRIVATE TEST SEED, not a live
Roughtime server key. Never use these keys for live server authentication.
"""
from pathlib import Path
import json
import sys


def arr(name: str, data: bytes) -> str:
    rows = [
        "  " + ", ".join(f"0x{x:02x}" for x in data[i : i + 12]) + ","
        for i in range(0, len(data), 12)
    ]
    return (
        f"static const unsigned char {name}[{len(data)}] = {{\n"
        + "\n".join(rows)
        + "\n};\n"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_v12_rt_test_header.py <output.h>")
    fixture = Path(__file__).resolve().parents[1] / "fixtures" / "roughtime_ietf_draft08_001.json"
    data = json.loads(fixture.read_text())
    assert data["info"] == "draft-ietf-ntp-roughtime-08 1"
    request = bytes.fromhex(data["request"][0])
    reply = bytes.fromhex(data["replies"][0])
    assert request.startswith(b"ROUGHTIM") and len(request) == 1024
    assert reply.startswith(b"ROUGHTIM") and len(reply) <= 2048
    nonce = request[40:72]  # Draft-08 request map: VER [36:40], NONC [40:72]
    assert len(nonce) == 32
    # Derived with Ed25519PublicKey(seed), matching the upstream Go tests.
    root = bytes.fromhex("1ce90cc9d5476809f9088f52018d9a8e5b8449951e2468e13de00d0234d50d14")
    text = (
        "/* GENERATED TEST FIXTURE ONLY — not a trusted production key. */\n"
        "#ifndef V12_RT_TEST_VECTOR_H\n#define V12_RT_TEST_VECTOR_H\n"
        + arr("rt_fixture_reply", reply)
        + arr("rt_fixture_nonce", nonce)
        + arr("rt_fixture_root", root)
        + "#endif\n"
    )
    Path(sys.argv[1]).write_text(text)
    print(f"N3B vector header written: {len(reply)} reply bytes, {len(nonce)} nonce bytes")


if __name__ == "__main__":
    main()
