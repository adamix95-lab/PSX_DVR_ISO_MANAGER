#!/usr/bin/env python3
"""Trace-only N5D instrumentation of pinned wolfSSL v5.8.2 source.

The patch fails on upstream drift. No changes to protocol bytes, cryptographic
operations, certificate verification, RNG or secrets. Tracing is active only
during the STAGE 10 app-data write.
"""
from pathlib import Path
import re
import sys

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("wolfssl-n5a")
ssl_c = root / "src/ssl.c"
internal_c = root / "src/internal.c"

t = ssl_c.read_text()
header = "static int wolfSSL_write_internal(WOLFSSL* ssl, const void* data, size_t sz)\n{"
call = "    ret = SendData(ssl, data, sz);"
assert t.count(header) == 1, "wolfSSL_write_internal signature changed"
assert t.count(call) == 1, "SendData call changed"
t = t.replace(header, "extern void n5d_trace(unsigned int step);\n\n" +
              header + "\n    n5d_trace(100);", 1)
t = t.replace(call, "    n5d_trace(110);\n" + call +
              "\n    n5d_trace(120);", 1)
ssl_c.write_text(t)

t = internal_c.read_text()
match = re.search(
    r"(?m)^[ \t]*(?:WOLFSSL_LOCAL\s+)?int\s+SendData\s*\("
    r"\s*WOLFSSL\s*\*\s*ssl\s*,\s*const\s+void\s*\*\s*data\s*,"
    r"\s*size_t\s+sz\s*\)\s*\{",
    t, re.S
)
assert match is not None, "SendData function definition not found"
ending = re.search(r"(?m)^}\s*$", t[match.end():])
assert ending is not None, "SendData closing brace not found"
end = match.end() + ending.end()
section = t[match.start():end]
assert "BuildMessage(" in section, "TLS1.2 BuildMessage missing"
assert "SendBuffered(ssl)" in section, "SendBuffered call missing"
section = section.replace("{", "{\n    n5d_trace(200);", 1)
# Boundaries around TLS1.2 record creation.
def around_build(m):
    indent = m.group(1)
    return (indent + "n5d_trace(230);\n" +
            m.group(0) + "\n" +
            indent + "n5d_trace(231);")
section, n = re.subn(
    r"(?m)^([ \t]*)sendSz\s*=\s*BuildMessage\s*\(.*?\);",
    around_build, section, count=1, flags=re.S
)
assert n == 1, "TLS1.2 BuildMessage assignment changed"
section = section.replace("SendBuffered(ssl)", "n5d_SendBuffered(ssl)")
# Wrapper records entry and return without changing actual send behavior.
helper = """
static int n5d_SendBuffered(WOLFSSL* ssl)
{
    int result;
    n5d_trace(250);
    result = SendBuffered(ssl);
    n5d_trace(251);
    return result;
}
"""
t = t[:match.start()] + helper + "\n" + section + t[end:]
# Optional deeper entry point inside the actual TLS record builder.
bm = re.search(
    r"(?m)^[ \t]*(?:WOLFSSL_LOCAL\s+)?int\s+BuildMessage\s*\("
    r"\s*WOLFSSL\s*\*\s*ssl\s*,", t, re.S
)
if bm is not None:
    brace = t.find("{", bm.end())
    semi = t.find(";", bm.end())
    assert brace >= 0 and (semi < 0 or brace < semi), \
        "BuildMessage is not a function definition"
    t = t[:brace+1] + "\n    n5d_trace(240);" + t[brace+1:]
# Make the trace hook visible before any record-builder definition.
t = "extern void n5d_trace(unsigned int step);\n" + t
internal_c.write_text(t)
for marker in (200,230,231,250,251):
    assert t.count(f"n5d_trace({marker})") == 1, marker
print("PASS: N5D wolfSSL write/SendData/BuildMessage/SendBuffered tracing")
