#ifndef DESR_V12_HTTPS_CLIENT_H
#define DESR_V12_HTTPS_CLIENT_H
#include "v12_https_policy.h"
#include <stddef.h>
#include <stdint.h>

/* Transport is intentionally abstract: a wolfSSL backend must verify the
 * certificate chain, hostname, certificate time and strong RNG before open.
 * It must enforce finite timeouts. This header does not implement TLS.
 */
typedef struct {
  void *ctx;
  int (*open_verified)(void *ctx, const char *hostname, unsigned port);
  int (*write_bytes)(void *ctx, const uint8_t *data, size_t length);
  int (*read_bytes)(void *ctx, uint8_t *data, size_t capacity);
  void (*close)(void *ctx);
} v12_https_io;

/* 0 = verified PNG; 1 = 404 missing art; -1 = failure.
 * Response is buffered only in caller-owned RAM; NO filesystem writes.
 * buffer must hold complete HTTP headers + body. Returned PNG points into it.
 */
int v12_https_fetch_art(v12_https_io *io, v12_art_source source,
                        const char *game_id, uint8_t *buffer, size_t capacity,
                        const uint8_t **png, size_t *png_size);

#endif
