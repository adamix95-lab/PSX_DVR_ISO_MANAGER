#ifndef DESR_PROBE_PNG_H
#define DESR_PROBE_PNG_H

#include <stddef.h>
#include <stdint.h>

/* Single local HTTP/1.0 200 response, not chunked, <=256KiB PNG.
   Returns zero only after framing, CRC and PNG chunk validation.
   Both pointers remain inside the caller-owned response buffer. */
#define PROBE_PNG_MAX (256U * 1024U)
#define PROBE_HTTP_HEADER_MAX 2048U

/* Parse HTTP/1.0 or HTTP/1.1 status line independently of the PNG body.
   Returns 100..599 or -1 for malformed/incomplete status line. */
int probe_http_status(const uint8_t *data, size_t length);

int probe_png_check(const uint8_t *data, size_t length);
int probe_http_extract_png(const uint8_t *data, size_t length,
                           const uint8_t **png, size_t *png_length);
#endif
