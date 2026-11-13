#!/usr/bin/env python3
"""N5F: bracket the precise ssl_in_handshake(ssl,1) decision in SendData.

Use only after N5D and N5E scripts on pinned wolfSSL commit
decea12e223869c8f8f3ab5a53dc90b69f436eb2.
Records control-flow and numeric handshake state, never secrets or plaintext.
No protocol changes, verification bypass, RNG changes or file writes.
"""
from pathlib import Path
import re
import sys

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("wolfssl-n5a")
p = root / "src/internal.c"
text = p.read_text()
handshake_re = r"static int ssl_in_handshake\(WOLFSSL \*ssl, int sending_data\)\s*\{"
senddata_re = (
    r"(?m)^int SendData\(WOLFSSL\* ssl, const void\* data, size_t sz\)\s*\{"
)
h = re.search(handshake_re, text)
d = re.search(senddata_re, text)
assert h and d and h.start() < d.start(), "unexpected pinned handshake layout"
segment = text[h.start():d.start()]
assert segment.count("if (IsSCR(ssl)) {") == 1, "IsSCR site drifted"
assert "if (ssl->options.handShakeState != HANDSHAKE_DONE)" in segment
assert "ssl->options.connectState < SECOND_REPLY_DONE" in segment

# This is read-only tracing of the IsSCR decision; preserves its return value.
is_scr_probe = """
static int n5f_IsSCR_probe(WOLFSSL* ssl)
{
    int value;
    n5d_trace(334);
    value = IsSCR(ssl);
    n5d_trace(335);
    return value;
}

"""
text = text[:h.start()] + is_scr_probe + text[h.start():]
h = re.search(handshake_re, text)
d = re.search(senddata_re, text)
assert h and d
segment = text[h.start():d.start()]
segment = segment.replace(
    "if (IsSCR(ssl)) {",
    "if (n5f_IsSCR_probe(ssl)) {", 1
)
# The original function has no loops; a pre-branch trace tells us its entry.
segment = segment.replace(
    "static int ssl_in_handshake(WOLFSSL *ssl, int sending_data)\n{",
    "static int ssl_in_handshake(WOLFSSL *ssl, int sending_data)\n"
    "{\n    n5d_trace(336);",1
)
# Wrapper captures the return value without changing the value or state.
wrapper = """
extern void n5f_handshake_state(int phase, int result, int handshakeState,
                                 int connectState, int sslError);
static int n5f_handshake_probe(WOLFSSL* ssl, int sending_data)
{
    int result;
    n5d_trace(330);
    n5f_handshake_state(0, -999, ssl->options.handShakeState,
                         ssl->options.connectState, ssl->error);
    result = ssl_in_handshake(ssl, sending_data);
    n5d_trace(331);
    n5f_handshake_state(1, result, ssl->options.handShakeState,
                         ssl->options.connectState, ssl->error);
    return result;
}

"""
text = text[:h.start()] + segment + wrapper + text[d.start():]
d = re.search(senddata_re, text)
assert d
brace_end = re.search(r"(?m)^}\s*$", text[d.end():])
assert brace_end, "SendData end missing"
last = d.end() + brace_end.end()
senddata=text[d.start():last]
assert senddata.count("ssl_in_handshake(ssl, 1)") == 1
senddata=senddata.replace(
    "ssl_in_handshake(ssl, 1)",
    "n5f_handshake_probe(ssl, 1)",1
)
# The next unconditional statement is after the handshake decision.
anchor = "\n    }\n\n#ifdef WOLFSSL_RW_THREADED"
assert senddata.count(anchor) == 1, "post-handshake branch anchor drift"
senddata = senddata.replace(anchor, "\n    }\n    n5d_trace(340);\n\n#ifdef WOLFSSL_RW_THREADED",1)
text = text[:d.start()] + senddata + text[last:]
p.write_text(text)
assert text.count("n5f_handshake_probe(ssl, 1)") == 1
assert text.count("n5d_trace(330)") == 1
assert text.count("n5d_trace(331)") == 1
assert text.count("n5d_trace(340)") == 1
print("PASS N5F: ssl_in_handshake control state/return and IsSCR bracketed")
