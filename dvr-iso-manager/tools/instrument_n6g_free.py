"""Add post-handshake checkpoints to the pinned wolfSSL teardown."""

from pathlib import Path
import sys


def checkpoints(source, start, next_function, before):
    begin = source.index(start)
    end = source.index(next_function, begin + len(start))
    function = source[begin:end]
    for anchor, label in before.items():
        if function.count(anchor) != 1:
            raise SystemExit(f"Unexpected wolfSSL source at {start}: {anchor}")
        function = function.replace(anchor, f'    n6g_free_trace("{label}");\n' + anchor)
    return source[:begin] + function + source[end:]


root = Path(sys.argv[1]) / "src"
declaration = "extern void n6g_free_trace(const char* step);\n\n"
ssl = root / "ssl.c"
source = ssl.read_text()
if declaration in source:
    raise SystemExit("wolfSSL free tracing already installed")
source = checkpoints(
    source, "void wolfSSL_free(WOLFSSL* ssl)\n{", "int wolfSSL_is_server",
    {'    WOLFSSL_ENTER("wolfSSL_free");': "00 wolfSSL_free entry"},
)

internal = root / "internal.c"
body = internal.read_text()
body = checkpoints(
    body, "void FreeSSL(WOLFSSL* ssl, void* heap)\n{",
    "#if !defined(NO_OLD_TLS)",
    {
        "    wolfSSL_ResourceFree(ssl);": "01 ResourceFree",
        "    XFREE(ssl, heap, DYNAMIC_TYPE_SSL);": "17 free SSL object",
        "    if (ctx)": "18 release CTX reference",
        "    (void)heap;": "21 FreeSSL return",
    },
)
body = checkpoints(
    body, "void wolfSSL_ResourceFree(WOLFSSL* ssl)\n{", "void FreeHandshakeResources",
    {
        "    FreeCiphers(ssl);": "02 FreeCiphers",
        "    FreeArrays(ssl, 0);": "03 FreeArrays",
        "    FreeKeyExchange(ssl);": "04 FreeKeyExchange",
        "    if (ssl->options.weOwnRng)": "05 DRBG cleanup",
        "    FreeSuites(ssl);": "06 FreeSuites",
        "    FreeHandshakeHashes(ssl);\n#ifdef HAVE_ECH": "07 FreeHandshakeHashes",
        "    XFREE(ssl->buffers.domainName.buffer,": "08 domain and session keys",
        "    ssl->keepCert = 0;": "09 UnloadCertsKeys",
        "    FreeKey(ssl, DYNAMIC_TYPE_RSA, (void**)&ssl->peerRsaKey);": "10 peer RSA key",
        "    if (ssl->buffers.inputBuffer.dynamicFlag)": "11 TLS buffers",
        "#ifdef HAVE_LIBZ\n    FreeStreams(ssl);": "12 BIO cleanup finished",
        "#if defined(HAVE_ECC) || defined(HAVE_CURVE25519) ||defined(HAVE_CURVE448)": "13 peer public keys",
        "    TLSX_FreeAll(ssl->extensions, ssl->heap);": "14 TLS extensions",
        "    if (ssl->session != NULL)": "15 SSL session",
        "#ifdef WOLFSSL_STATIC_MEMORY": "16 remaining SSL resources",
    },
)
body = checkpoints(
    body, "void FreeSSL_Ctx(WOLFSSL_CTX* ctx)\n{", "void InitCiphers",
    {
        "    wolfSSL_RefWithMutexDec(&ctx->ref, &isZero, &ret);": "19 CTX reference decrement",
        "#ifdef WOLFSSL_REFCNT_ERROR_RETURN": "20 CTX reference decremented",
    },
)

ssl.write_text(declaration + source)
internal.write_text(declaration + body)
