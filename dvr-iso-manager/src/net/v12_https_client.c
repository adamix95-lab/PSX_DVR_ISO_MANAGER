#include "v12_https_client.h"
#include <stdio.h>
#include <string.h>

int v12_https_fetch_art(v12_https_io *io, v12_art_source source,
                        const char *game_id, uint8_t *buffer, size_t capacity,
                        const uint8_t **png, size_t *png_size) {
  char host[96], path[192], request[384];
  int req_len, n, result = -1;
  size_t sent = 0, received = 0;
  v12_https_response meta;
  if (png) *png = NULL;
  if (png_size) *png_size = 0;
  if (!io || !io->open_verified || !io->write_bytes || !io->read_bytes ||
      !io->close || !buffer || !capacity || !png || !png_size ||
      capacity > V12_HTTPS_MAX_HEADER + V12_HTTPS_MAX_PNG ||
      v12_https_target(source, game_id, host, sizeof(host),
                       path, sizeof(path)) < 0) return -1;
  req_len = snprintf(request, sizeof(request),
    "GET %s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "Accept: image/png\r\n"
    "User-Agent: PSX-DVR-ISO-Manager/1.2\r\n"
    "Connection: close\r\n\r\n", path, host);
  if (req_len < 0 || (size_t)req_len >= sizeof(request)) return -1;

  /* If the backend cannot establish VERIFIED TLS, no request may be sent. */
  if (io->open_verified(io->ctx, host, 443U) != 0) goto cleanup;
  while (sent < (size_t)req_len) {
    n = io->write_bytes(io->ctx, (const uint8_t *)request + sent,
                        (size_t)req_len - sent);
    if (n <= 0 || (size_t)n > (size_t)req_len - sent) goto cleanup;
    sent += (size_t)n;
  }
  for (;;) {
    uint8_t extra;
    if (received == capacity) {
      n = io->read_bytes(io->ctx, &extra, 1);
      if (n != 0) goto cleanup; /* Overflow or transport error. */
      break;
    }
    n = io->read_bytes(io->ctx, buffer + received, capacity - received);
    if (n < 0 || (size_t)n > capacity - received) goto cleanup;
    if (n == 0) break;
    received += (size_t)n;
  }
  result = v12_https_validate_response(buffer, received, &meta);
  if (result == 0) {
    *png = buffer + meta.body_offset;
    *png_size = meta.body_size;
  }
cleanup:
  /* close must also clean up sockets after a failed TLS handshake. */
  io->close(io->ctx);
  return result;
}
