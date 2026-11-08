#ifndef DESR_V12_HTTPS_POLICY_H
#define DESR_V12_HTTPS_POLICY_H

#include <stddef.h>
#include <stdint.h>

/* Pure host-testable policy; NO sockets, DNS, TLS, file I/O or HDD writes.
 * This is not a replacement for verified wolfSSL HTTPS transport.
 */
#define V12_HTTPS_MAX_HEADER 4096U
#define V12_HTTPS_MAX_PNG (256U * 1024U)

typedef enum {
  V12_ART_OPL_ARCHIVE = 1,
  V12_ART_COSMICSCALE = 2
} v12_art_source;

typedef struct {
  int status;                /* 200 or 404 only */
  size_t body_offset;
  size_t body_size;
} v12_https_response;

/* Build allowlisted HTTPS host and path from strictly validated PS2 Game ID.
 * Return 0 on success. No arbitrary URLs, hosts or query strings accepted.
 */
int v12_https_target(v12_art_source source, const char *game_id,
                     char *host, size_t host_cap, char *path, size_t path_cap);

/* Parse a COMPLETE decrypted HTTP response. Returns 0 for a verified PNG,
 * 1 for a 404 (missing cover), or -1 for unsupported/bad responses.
 * Rejects oversized/incomplete bodies, redirects, Transfer-Encoding,
 * Content-Encoding, duplicate headers and PNG with invalid CRC.
 * Only call AFTER TLS peer hostname and CA chain have been verified.
 */
int v12_https_validate_response(const uint8_t *bytes, size_t size,
                                v12_https_response *out);

#endif
