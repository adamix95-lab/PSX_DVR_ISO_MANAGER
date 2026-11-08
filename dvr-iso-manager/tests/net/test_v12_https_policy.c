#include "v12_https_policy.h"
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

static int check(const char *header, const uint8_t *body, size_t bytes,
                 v12_https_response *result) {
  uint8_t buffer[2048];
  size_t head = strlen(header);
  assert(head + bytes <= sizeof(buffer));
  memcpy(buffer, header, head);
  if (bytes) memcpy(buffer + head, body, bytes);
  return v12_https_validate_response(buffer, head + bytes, result);
}

static void test_targets(void) {
  char host[96], path[192];
  assert(v12_https_target(V12_ART_OPL_ARCHIVE, "SLUS_210.47",
                          host, sizeof(host), path, sizeof(path)) == 0);
  assert(!strcmp(host, "archive.org"));
  assert(!strcmp(path,
    "/download/OPLM_ART_2024_09/OPLM_ART_2024_09.zip/PS2/"
    "SLUS_210.47/SLUS_210.47_COV.png"));
  assert(v12_https_target(V12_ART_COSMICSCALE, "SLES_538.86",
                          host, sizeof(host), path, sizeof(path)) == 0);
  assert(!strcmp(host, "raw.githubusercontent.com"));
  assert(!strcmp(path, "/CosmicScale/psbbn-art-database/main/art/SLES_538.86.png"));
  assert(v12_https_target(V12_ART_COSMICSCALE, "../../etc/passwd",
                          host, sizeof(host), path, sizeof(path)) < 0);
  assert(v12_https_target(V12_ART_COSMICSCALE, "slus_210.47",
                          host, sizeof(host), path, sizeof(path)) < 0);
  assert(v12_https_target(V12_ART_COSMICSCALE, "SLUS_210.47\r\n",
                          host, sizeof(host), path, sizeof(path)) < 0);
  assert(v12_https_target(V12_ART_COSMICSCALE, "SLUS_210.47",
                          host, 8, path, sizeof(path)) < 0);
  assert(v12_https_target(V12_ART_OPL_ARCHIVE, "SLUS_210.47",
                          host, sizeof(host), path, 10) < 0);
  assert(v12_https_target((v12_art_source)99, "SLUS_210.47",
                          host, sizeof(host), path, sizeof(path)) < 0);
}

static void test_http(void) {
  v12_https_response meta;
  char good[256];
  int n = snprintf(good, sizeof(good),
    "HTTP/1.1 200 OK\r\n"
    "content-type: image/png\r\n"
    "Content-Length: %u\r\n"
    "Connection: close\r\n\r\n", (unsigned)sizeof(png));
  assert(n > 0 && (size_t)n < sizeof(good));
  assert(check(good, png, sizeof(png), &meta) == 0);
  assert(meta.status == 200 && meta.body_offset == (size_t)n &&
         meta.body_size == sizeof(png));
  assert(check(good, png, sizeof(png) - 1, &meta) < 0);

  {
    uint8_t bad[sizeof(png)];
    memcpy(bad, png, sizeof(png));
    bad[55] ^= 1;
    assert(check(good, bad, sizeof(bad), &meta) < 0);
  }
  assert(check("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 200 OK\r\nContent-Length: 70\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
               "Content-Length: 70\r\n\r\n", png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n"
               "Content-Length: 70\r\nContent-Length: 70\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n"
               "Content-Length: 70\r\nTransfer-Encoding: chunked\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n"
               "Content-Length: 70\r\nContent-Encoding: gzip\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 302 Found\r\nLocation: https://example.com/a\r\n\r\n",
               NULL, 0, &meta) < 0);
  assert(check("HTTP/1.1 2000 Fake\r\nContent-Length: 70\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n"
               "Content-Length: 9999999999999999\r\n\r\n",
               png, sizeof(png), &meta) < 0);
  assert(check("HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n",
               NULL, 0, &meta) == 1 && meta.status == 404);
  assert(check("HTTP/1.0 404 Not Found\r\n\r\n", NULL, 0, &meta) == 1);
}

int main(void) {
  assert(sizeof(png) == 70);
  test_targets();
  test_http();
  puts("PASS: v1.2 bounded HTTPS art-source and HTTP/PNG policy");
  return 0;
}
