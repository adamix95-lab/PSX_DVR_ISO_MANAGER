#!/usr/bin/env python3
"""Generate PUBLIC test fixture bytes for host/PS2 Ed25519 verifier checks.

Input: pinned Apache-2.0 upstream Cloudflare draft-08 JSON test fixture.
Public root below derives from the fixture's PRIVATE TEST SEED, not a live
Roughtime server key. Never use these keys for live server authentication.
"""
from pathlib import Path
import json
import sys
import struct


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



def omit_nonc_echo(reply: bytes) -> bytes:
    """Create a signed 6-tag diagnostic vector, 40 bytes shorter.

    This intentionally removes the unsigned top-level NONC echo ONLY;
    delegation, signed SREP and Merkle root remain untouched. This vector
    is INVALID draft08 and must only exercise forensic compatibility.
    """
    assert reply[:8] == b"ROUGHTIM"
    body=reply[12:]
    count,=struct.unpack_from("<I",body)
    offsets=[0]+[struct.unpack_from("<I",body,4+4*i)[0]
                 for i in range(count-1)]
    values_start=8*count
    offsets.append(len(body)-values_start)
    keys=[body[count*4+4*i:count*4+4*(i+1)]
          for i in range(count)]
    values=[body[values_start+offsets[i]:values_start+offsets[i+1]]
            for i in range(count)]
    assert keys.count(b"NONC")==1
    index=keys.index(b"NONC")
    assert len(values[index])==32
    del keys[index]
    del values[index]
    assert len(keys)==6
    new_offsets=[]
    start=0
    for value in values[:-1]:
        start+=len(value)
        new_offsets.append(start)
    new_body=(struct.pack("<I",len(keys))
        +b"".join(struct.pack("<I",x) for x in new_offsets)
        +b"".join(keys)+b"".join(values))
    converted=b"ROUGHTIM"+struct.pack("<I",len(new_body))+new_body
    assert len(reply)-len(converted)==40
    assert len(converted)==352
    return converted

def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: generate_v12_rt_test_header.py <output.h>")
    fixture = Path(__file__).resolve().parents[1] / "fixtures" / "roughtime_ietf_draft08_001.json"
    data = json.loads(fixture.read_text())
    assert data["info"] == "draft-ietf-ntp-roughtime-08 1"
    request = bytes.fromhex(data["request"][0])
    reply = bytes.fromhex(data["replies"][0])
    no_echo_reply=omit_nonc_echo(reply)
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
        + arr("rt_fixture_missing_nonc_reply", no_echo_reply)
        + arr("rt_fixture_nonce", nonce)
        + arr("rt_fixture_root", root)
        + "#endif\n"
    )
    Path(sys.argv[1]).write_text(text)
    print(f"N3B vector header written: {len(reply)} reply bytes, {len(nonce)} nonce bytes")


if __name__ == "__main__":
    main()
