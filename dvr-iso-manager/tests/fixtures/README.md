# Roughtime test-vector provenance

`roughtime_ietf_draft08_001.json` is an **unchanged upstream test
vector** taken from the Apache-2.0-licensed Cloudflare Roughtime project:

- Source: https://github.com/cloudflare/roughtime/blob/master/protocol/testdata/roughtime_ietf_draft08_001.json
- Upstream GitHub blob SHA: `2eee9e2f2c7003bb90612d0f630d20c1b7645e2b`
- Project license: https://github.com/cloudflare/roughtime/blob/master/LICENSE (Apache License, Version 2.0)
- Copyright: The Roughtime Authors and Cloudflare contributors.

The test fixture's public root key is synthetic and must **never**
replace the official production key for the live Roughtime server.

The host-only Python verifier is a separate implementation of the
public protocol and is **not a DESR C implementation**. It does not
prove live server authenticity, secure nonce generation, clock trust,
or wolfSSL TLS support.

If the protocol evolves, pin a known upstream commit and update both
the test vector and the test parser consistently; never silently
change key material or treat arbitrary network responses as trusted.
