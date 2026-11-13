#ifndef DESR_N5A_HTTP_RAM_H
#define DESR_N5A_HTTP_RAM_H
#include <stddef.h>
#include <stdint.h>
#define N5A_HTTP_INPUT_CAP 16384U
#define N5A_HTTP_HEADER_CAP 8192U
#define N5A_HTTP_BODY_CAP 256U
#define N5A_HTTP_EXPECTED_BODY "PSX-DVR-ISO-MANAGER N5A HTTPS RAM PASS\n"
#define N5A_HTTP_EXPECTED_BODY_BYTES (sizeof(N5A_HTTP_EXPECTED_BODY)-1U)
/* No disk I/O or network access. This parser accepts exactly HTTP status
 * 200 with a byte-for-byte fixed, public 39-byte RAM probe marker.
 * Content-Length, chunked, or EOF-delimited HTTP/1.1 bodies are supported.
 * Unknown/ambiguous encodings and redirects FAIL CLOSED.
 *
 * Return: 1 = fully validated; 0 = need more bytes; negative = invalid.
 * EOF is 1 only on verified TLS stream close, not a network error.
 * The output is replaced only on full successful validation.
 */
typedef struct {
    int status;
    unsigned header_bytes;
    unsigned body_bytes;
    unsigned chunked;
} n5a_http_result;
int n5a_http_check(const uint8_t *response,size_t len,int eof,
                    n5a_http_result *out);
#endif
