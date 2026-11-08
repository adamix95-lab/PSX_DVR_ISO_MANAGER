#include "v12_https_client.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t png[] = {
  0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,
  0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
  0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,
  0x08,0x06,0x00,0x00,0x00,0x1f,0x15,0xc4,
  0x89,0x00,0x00,0x00,0x0d,0x49,0x44,0x41,
  0x54,0x78,0x9c,0x63,0x10,0x54,0x32,0xfe,
  0x0f,0x00,0x02,0x14,0x01,0x66,0x04,0xe0,
  0xde,0x7f,0x00,0x00,0x00,0x00,0x49,0x45,
  0x4e,0x44,0xae,0x42,0x60,0x82
};

typedef struct {
  uint8_t in[1024], request[512];
  size_t size, pos, request_size;
  int verified, opened, closed, fail_reads;
  char host[96];
} mock;

static int open_verified(void *ctx, const char *hostname, unsigned port) {
  mock *m = (mock *)ctx;
  ++m->opened;
  assert(port == 443);
  snprintf(m->host, sizeof(m->host), "%s", hostname);
  return m->verified ? 0 : -1;
}
static int write_bytes(void *ctx, const uint8_t *p, size_t n) {
  mock *m = (mock *)ctx;
  if (n > 17) n = 17; /* Force multiple sends. */
  if (!n || m->request_size + n > sizeof(m->request)) return -1;
  memcpy(m->request + m->request_size, p, n);
  m->request_size += n;
  return (int)n;
}
static int read_bytes(void *ctx, uint8_t *dst, size_t n) {
  mock *m = (mock *)ctx;
  if (m->fail_reads) return -1;
  if (m->pos == m->size) return 0;
  if (n > 7) n = 7; /* Force fragmented TLS records / reads. */
  if (n > m->size - m->pos) n = m->size - m->pos;
  memcpy(dst, m->in + m->pos, n);
  m->pos += n;
  return (int)n;
}
static void close_io(void *ctx) {
  mock *m = (mock *)ctx;
  ++m->closed;
}

static v12_https_io transport(mock *m) {
  v12_https_io io;
  io.ctx = m;
  io.open_verified = open_verified;
  io.write_bytes = write_bytes;
  io.read_bytes = read_bytes;
  io.close = close_io;
  return io;
}
static void response(mock *m, const char *header, const uint8_t *body,
                     size_t length) {
  size_t h = strlen(header);
  assert(h + length < sizeof(m->in));
  memcpy(m->in, header, h);
  if (length) memcpy(m->in + h, body, length);
  m->size = h + length;
  m->pos = 0;
}
int main(void) {
  uint8_t buf[1024];
  const uint8_t *image;
  size_t bytes;
  mock m = {0};
  v12_https_io io;
  char head[160];
  int n = snprintf(head, sizeof(head),
    "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: %u\r\n\r\n",
    (unsigned)sizeof(png));
  assert(n > 0 && (size_t)n < sizeof(head));
  response(&m, head, png, sizeof(png));
  m.verified = 1;
  io = transport(&m);
  assert(v12_https_fetch_art(&io, V12_ART_COSMICSCALE, "SLES_538.86",
                            buf, sizeof(buf), &image, &bytes) == 0);
  assert(bytes == sizeof(png) && !memcmp(image, png, bytes));
  assert(m.opened == 1 && m.closed == 1);
  assert(!strcmp(m.host, "raw.githubusercontent.com"));
  assert(m.request_size > 0 && m.request_size < sizeof(m.request));
  m.request[m.request_size] = '\0';
  assert(strstr((const char *)m.request,
    "GET /CosmicScale/psbbn-art-database/main/art/SLES_538.86.png HTTP/1.0\r\n"));
  assert(strstr((const char *)m.request,
    "Host: raw.githubusercontent.com\r\n"));

  /* Rejected TLS/CA/hostname: absolutely no HTTP request is sent. */
  memset(&m, 0, sizeof(m));
  m.verified = 0;
  io = transport(&m);
  assert(v12_https_fetch_art(&io, V12_ART_OPL_ARCHIVE, "SLUS_210.47",
                            buf, sizeof(buf), &image, &bytes) < 0);
  assert(m.opened == 1 && m.request_size == 0 && m.closed == 1);

  /* Incomplete response, CRC failure and buffer overflow never expose PNG. */
  memset(&m, 0, sizeof(m));
  m.verified = 1;
  response(&m, head, png, sizeof(png) - 1);
  io = transport(&m);
  assert(v12_https_fetch_art(&io, V12_ART_COSMICSCALE, "SLES_538.86",
                            buf, sizeof(buf), &image, &bytes) < 0);
  assert(image == NULL && bytes == 0 && m.closed == 1);

  memset(&m, 0, sizeof(m));
  m.verified = 1;
  response(&m, head, png, sizeof(png));
  io = transport(&m);
  assert(v12_https_fetch_art(&io, V12_ART_COSMICSCALE, "SLES_538.86",
                            buf, 32, &image, &bytes) < 0);
  assert(image == NULL && bytes == 0);

  memset(&m, 0, sizeof(m));
  m.verified = 1;
  response(&m, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n",
           NULL, 0);
  io = transport(&m);
  assert(v12_https_fetch_art(&io, V12_ART_OPL_ARCHIVE, "SLUS_210.47",
                            buf, sizeof(buf), &image, &bytes) == 1);
  assert(image == NULL && bytes == 0 && m.closed == 1);

  memset(&m, 0, sizeof(m));
  m.verified = 1;
  io = transport(&m);
  assert(v12_https_fetch_art(&io, V12_ART_OPL_ARCHIVE, "../../escape",
                            buf, sizeof(buf), &image, &bytes) < 0);
  assert(m.opened == 0 && m.request_size == 0);
  puts("PASS: v1.2 verified-TLS-only HTTPS streaming client (mock transport)");
  return 0;
}
