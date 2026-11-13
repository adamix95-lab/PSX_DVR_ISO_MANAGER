#!/usr/bin/env python3
"""N5G: trace completion of IsAtLeastTLSv1_3() in pinned wolfSSL SendData.

Run AFTER N5D, N5E and N5F instruments. Wraps the result with a pure
identity function that emits one diagnostic step. It does not change
negotiated TLS version, verification, ciphering, RNG or network data.
Fail closed if the instrumented expression cannot be found exactly.
"""
from pathlib import Path
import re
import sys

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("wolfssl-n5a")
path = root / "src/internal.c"
text = path.read_text()
m = re.search(r"(?m)^int SendData\(WOLFSSL\* ssl, const void\* data, size_t sz\)\s*\{",text)
assert m, "pinned SendData definition not found"
end = re.search(r"(?m)^}\s*$",text[m.end():])
assert end, "pinned SendData end not found"
stop = m.end()+end.end()
part = text[m.start():stop]
pattern = r"\(n5d_trace\(318\),\s*(IsAtLeastTLSv1_3\(ssl->version\))\)"
part, n = re.subn(pattern, r"(n5d_trace(318), n5g_tls13_result(\1))",part)
assert n >= 1 and n <= 4, f"unexpected TLSv1.3 probes count={n}"
helper = """
/* Identity wrapper: observe only that the version predicate returned. */
static int n5g_tls13_result(int original)
{
    n5d_trace(319);
    return original;
}

"""
text = text[:m.start()] + helper + part + text[stop:]
assert text.count("n5d_trace(319)") == 1
assert text.count("n5g_tls13_result(IsAtLeastTLSv1_3(ssl->version))") == n
path.write_text(text)
print(f"PASS N5G: post-IsAtLeastTLSv1_3 instrumentation at {n} SendData site(s)")
