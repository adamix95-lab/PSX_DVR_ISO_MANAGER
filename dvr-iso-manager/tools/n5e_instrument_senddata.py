#!/usr/bin/env python3
"""N5E: localize the first application write within pinned wolfSSL SendData.

Apply after n5d_instrument_wolfssl.py. Only wraps existing expression-level
calls with a side-effect-free, output-only trace callback. Fail closed on
upstream drift and never print secrets, keys, RNG bytes or plaintext.
"""
from pathlib import Path
import re
import sys

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("wolfssl-n5a")
path = root / "src/internal.c"
data = path.read_text()
start = re.search(
    r"\bint\s+SendData\s*\(\s*WOLFSSL\s*\*\s*ssl\s*,"
    r"\s*const\s+void\s*\*\s*data\s*,\s*size_t\s+sz\s*\)\s*\{",
    data, re.S)
if not start:
    raise SystemExit("FAIL: pinned SendData entry drift")
end_match = re.search(r"(?m)^}\s*$", data[start.end():])
if not end_match:
    raise SystemExit("FAIL: pinned SendData end drift")
end = start.end() + end_match.end()
section = data[start.start():end]
assert section.count("n5d_trace(200);") == 1, "N5D patch must run first"

# Trace statements immediately preceding the normal wolfSSL debug macro.
if 'WOLFSSL_ENTER("SendData");' in section:
    section=section.replace('WOLFSSL_ENTER("SendData");',
      'n5d_trace(201);\n    WOLFSSL_ENTER("SendData");\n    n5d_trace(202);',1)
else:
    # This is not fatal: the N5D entry marker and call probes still work.
    print("NOTE: no WOLFSSL_ENTER in SendData; entry=200 still traced")

# Preserve byte-for-byte length in the mask to avoid false matches in
# comments or string literals. Only real function-call expressions are wrapped.
def code_mask(text):
    arr=list(text)
    i=0
    while i<len(text):
        if text.startswith("/*",i):
            j=text.find("*/",i+2)
            if j<0: raise ValueError("unterminated C comment")
            j+=2
            for k in range(i,j):
                if arr[k]!="\n":arr[k]=" "
            i=j
        elif text.startswith("//",i):
            j=text.find("\n",i+2)
            if j<0:j=len(text)
            for k in range(i,j):arr[k]=" "
            i=j
        elif text[i] in ('"',"'"):
            q=text[i]
            j=i+1
            while j<len(text):
                if text[j]=="\\":j+=2;continue
                if text[j]==q:break
                j+=1
            if j>=len(text):raise ValueError("unterminated C literal")
            for k in range(i,j+1):
                if arr[k]!="\n":arr[k]=" "
            i=j+1
        else:i+=1
    return "".join(arr)

fn_ids={
 "RetrySendAlert":301,
 "ssl_in_handshake":302,
 "wolfSSL_negotiate":303,
 "SetKeys":304,
 "n5d_SendBuffered":305,
 "SendBuffered":306,
 "CheckAvailableSize":307,
 "GetOutputBuffer":308,
 "wolfSSL_GetMaxFragSize":309,
 "wolfssl_local_GetMaxPlaintextSize":310,
 "wolfssl_local_GetRecordSize":311,
 "BuildMessage":312,
 "WriteSEQ":313,
 "XMALLOC":314,
 "XMEMCPY":315,
 "XMEMSET":316,
 "FreeAsyncCtx":317,
 "IsAtLeastTLSv1_3":318,
}
mask=code_mask(section)
events=[]
found={}
for name,step in fn_ids.items():
    regex=re.compile(r"\b"+re.escape(name)+r"\s*\(")
    for m in regex.finditer(mask):
        # Filter declaration or function pointer expression if any.
        open_paren=mask.find("(",m.start(),m.end()+1)
        depth=0
        close=-1
        for j in range(open_paren,len(mask)):
            if mask[j]=="(":depth+=1
            elif mask[j]==")":
                depth-=1
                if depth==0:
                    close=j
                    break
        if close<0:
            raise SystemExit("FAIL unbalanced function invocation: "+name)
        events.append((m.start(),f"(n5d_trace({step}), "))
        events.append((close+1,")"))
        found[name]=found.get(name,0)+1

for required in ("RetrySendAlert","CheckAvailableSize","GetOutputBuffer","BuildMessage"):
    if not found.get(required):
        raise SystemExit("FAIL: pinned SendData call missing: "+required)
if not found.get("n5d_SendBuffered"):
    raise SystemExit("FAIL: N5D SendBuffered wrapper missing")

# Apply right to left so original indices remain valid.
for position,insert in sorted(events,key=lambda x:x[0],reverse=True):
    section=section[:position]+insert+section[position:]
data=data[:start.start()]+section+data[end:]
path.write_text(data)
print("PASS N5E trace-only SendData expression probes:",sorted(found.items()))
