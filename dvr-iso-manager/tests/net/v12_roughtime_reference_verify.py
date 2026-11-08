#!/usr/bin/env python3
"""N3B host-only Roughtime draft-08 signature verification reference.

Implements the public IETF draft-08 wire format independently from the
Cloudflare vector. cryptography is used for Ed25519 signature verification.

DO NOT deploy this Python verifier or the vector's test key as a trusted
Roughtime server in production. Real DESR 1.2 currently has only the N3A
wire/framing diagnostic, not a verified C implementation.

Reference:
 https://github.com/cloudflare/roughtime/blob/master/protocol/protocol.go
 https://github.com/cloudflare/roughtime/blob/master/protocol/testdata/roughtime_ietf_draft08_001.json
 upstream test vector: Apache-2.0, Cloudflare Roughtime authors.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey, Ed25519PrivateKey
from cryptography.hazmat.primitives import serialization


CERT_CONTEXT = b"RoughTime v1 delegation signature--\0"
RESPONSE_CONTEXT = b"RoughTime v1 response signature\0"
FRAMING = b"ROUGHTIM"
DRAFT08 = 0x80000008


class VerifyFailure(ValueError):
    pass


def _fail(message: str):
    raise VerifyFailure(message)


def le32(data: bytes) -> int:
    if len(data) != 4:
        _fail("expected u32")
    return struct.unpack("<I", data)[0]


def le64(data: bytes) -> int:
    if len(data) != 8:
        _fail("expected u64")
    return struct.unpack("<Q", data)[0]


def tag_map(data: bytes) -> dict[bytes, bytes]:
    """Safely decode one Roughtime map, all byte ranges strictly bounded."""
    if len(data) < 4 or len(data) % 4:
        _fail("truncated or unaligned map")
    count = le32(data[:4])
    if count < 1 or count > 32 or count * 8 > len(data):
        _fail("invalid tag count")
    payload = data[count * 8 :]
    offsets = [0]
    for i in range(count - 1):
        offsets.append(le32(data[4 + 4 * i : 8 + 4 * i]))
    offsets.append(len(payload))
    if any(o > len(payload) or o % 4 for o in offsets):
        _fail("out of bounds or unaligned offset")
    if offsets != sorted(offsets):
        _fail("unsorted offsets")

    result: dict[bytes, bytes] = {}
    last_tag_id = -1
    for i in range(count):
        tag = data[4 * count + i * 4 : 4 * count + (i + 1) * 4]
        tag_id = int.from_bytes(tag, "little")
        if tag_id <= last_tag_id:
            _fail("unsorted or duplicate tag")
        last_tag_id = tag_id
        result[tag] = payload[offsets[i] : offsets[i + 1]]
    return result


def framed(packet: bytes) -> dict[bytes, bytes]:
    if len(packet) < 20 or len(packet) > 2048:
        _fail("reply size")
    if packet[:8] != FRAMING or le32(packet[8:12]) != len(packet) - 12:
        _fail("wrong Roughtime framing")
    return tag_map(packet[12:])


def require(tags: dict[bytes, bytes], name: bytes, size: int | None = None) -> bytes:
    if name not in tags:
        _fail("missing tag " + repr(name))
    raw = tags[name]
    if size is not None and len(raw) != size:
        _fail("invalid length of " + repr(name))
    return raw


def check_signature(public_key: bytes, sig: bytes, message: bytes):
    if len(public_key) != 32 or len(sig) != 64:
        _fail("invalid Ed25519 key/signature length")
    try:
        Ed25519PublicKey.from_public_bytes(public_key).verify(sig, message)
    except (ValueError, InvalidSignature) as exc:
        raise VerifyFailure("invalid Ed25519 signature") from exc


def verify_draft08_reply(
    reply_wire: bytes, trusted_root: bytes, original_nonce: bytes
) -> tuple[int, int]:
    """Return (UTC seconds, radius seconds) ONLY after all signatures + proofs.

    The root key must already be securely pinned. The original nonce MUST
    be unpredictable and fresh in any real network usage. An offline vector
    can test algorithms but cannot itself prove nonce freshness or trust.
    """
    if len(trusted_root) != 32 or len(original_nonce) != 32:
        _fail("invalid public key or nonce")
    reply = framed(reply_wire)
    if le32(require(reply, b"VER\0", 4)) != DRAFT08:
        _fail("unexpected protocol version")
    if require(reply, b"NONC", 32) != original_nonce:
        _fail("wrong request nonce")

    cert = tag_map(require(reply, b"CERT"))
    delegation_raw = require(cert, b"DELE")
    check_signature(
        trusted_root,
        require(cert, b"SIG\0", 64),
        CERT_CONTEXT + delegation_raw,
    )
    delegation = tag_map(delegation_raw)
    min_time = le64(require(delegation, b"MINT", 8))
    max_time = le64(require(delegation, b"MAXT", 8))
    online_key = require(delegation, b"PUBK", 32)
    if max_time < min_time:
        _fail("invalid delegation window")

    signed_response_bytes = require(reply, b"SREP")
    check_signature(
        online_key,
        require(reply, b"SIG\0", 64),
        RESPONSE_CONTEXT + signed_response_bytes,
    )
    signed_response = tag_map(signed_response_bytes)
    midpoint = le64(require(signed_response, b"MIDP", 8))
    radius = le32(require(signed_response, b"RADI", 4))
    merkle_root = require(signed_response, b"ROOT", 32)
    if not min_time <= midpoint <= max_time:
        _fail("time outside signed delegation range")
    if radius > 86400:
        _fail("unreasonable uncertainty radius")

    index = le32(require(reply, b"INDX", 4))
    path = require(reply, b"PATH")
    if len(path) % 32 or len(path) > 32 * 24:
        _fail("invalid Merkle path")
    digest = hashlib.sha512(b"\x00" + original_nonce).digest()[:32]
    for offset in range(0, len(path), 32):
        node = path[offset : offset + 32]
        if index & 1:
            digest = hashlib.sha512(b"\x01" + node + digest).digest()[:32]
        else:
            digest = hashlib.sha512(b"\x01" + digest + node).digest()[:32]
        index >>= 1
    if index != 0 or digest != merkle_root:
        _fail("nonce Merkle inclusion check failed")
    return midpoint, radius


def read_vector():
    root = Path(__file__).resolve().parents[1] / "fixtures"
    vector = json.loads((root / "roughtime_ietf_draft08_001.json").read_text())
    assert vector["info"].startswith("draft-ietf-ntp-roughtime-08")
    request = bytes.fromhex(vector["request"][0])
    reply = bytes.fromhex(vector["replies"][0])
    # Upstream vector's root_key/online_key fields are **private key SEEDS**
    # used to create deterministic test responses, NOT public keys.
    # Derive the public key exactly as the upstream Go vector test does.
    seed = bytes.fromhex(vector["root_key"])
    trusted_root = Ed25519PrivateKey.from_private_bytes(seed).public_key().public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw,
    )
    nonce = require(framed(request), b"NONC", 32)  # same framing
    return request, reply, trusted_root, nonce


def selftest():
    request, reply, root, nonce = read_vector()
    assert len(request) == 1024 and len(reply) <= 2048
    midpoint, radius = verify_draft08_reply(reply, root, nonce)
    assert isinstance(midpoint, int) and isinstance(radius, int)
    assert 0 <= radius <= 86400

    def reject(candidate: bytes, root_key=root, one_nonce=nonce):
        try:
            verify_draft08_reply(candidate, root_key, one_nonce)
        except VerifyFailure:
            return
        raise AssertionError("FORGED/INVALID vector unexpectedly verified")

    reject(reply, bytes(32))
    reject(reply, root, bytes(32))
    reject(reply[:-1])
    altered = bytearray(reply)
    altered[12 + 8 * 7 + 3] ^= 1  # signed data/tag region, not important where
    reject(bytes(altered))

    # SIG, signed response, delegation or nonce corruption MUST fail closed.
    for i in (20, 48, 80, 128, 180, len(reply)-10):
        forged = bytearray(reply)
        forged[i] ^= 0x55
        reject(bytes(forged))
    print(
        f"PASS: signed Roughtime draft08 vector, chain+SREP+Merkle; "
        f"midpoint={midpoint} radius={radius}; forged inputs rejected"
    )


if __name__ == "__main__":
    selftest()
