#include "v12_https_policy.h"
#include "probe_png.h"
#include <stdio.h>
#include <string.h>

static int valid_game_id(const char *s) {
  size_t i;
  if (!s || strlen(s) != 11 || s[4] != '_' || s[8] != '.') return 0;
  for (i = 0; i < 4; ++i)
    if (s[i] < 'A' || s[i] > 'Z') return 0;
  for (i = 5; i < 11; ++i)
    if (i != 8 && (s[i] < '0' || s[i] > '9')) return 0;
  return 1;
}

int v12_https_target(v12_art_source source, const char *id,
                     char *host, size_t hc, char *path, size_t pc) {
  const char *hostname;
  int n;
  if (!host || !path || !hc || !pc || !valid_game_id(id)) return -1;
  if (source == V12_ART_OPL_ARCHIVE) {
    hostname = "archive.org";
    n = snprintf(path, pc,
      "/download/OPLM_ART_2024_09/OPLM_ART_2024_09.zip/PS2/%s/%s_COV.png",
      id, id);
  } else if (source == V12_ART_COSMICSCALE) {
    hostname = "raw.githubusercontent.com";
    n = snprintf(path, pc,
      "/CosmicScale/psbbn-art-database/main/art/%s.png", id);
  } else {
    return -1;
  }
  if (n < 0 || (size_t)n >= pc || strlen(hostname) + 1 > hc) return -1;
  memcpy(host, hostname, strlen(hostname) + 1);
  return 0;
}

static int ci_equal(unsigned char a, unsigned char b) {
  if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
  if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
  return a == b;
}
static int prefix(const uint8_t *p, size_t n, const char *tag) {
  size_t i, len = strlen(tag);
  if (n < len) return 0;
  for (i = 0; i < len; ++i)
    if (!ci_equal(p[i], (unsigned char)tag[i])) return 0;
  return 1;
}
static int value(const uint8_t *p, size_t n, const char *v) {
  size_t i, len = strlen(v);
  if (n != len) return 0;
  for (i = 0; i < len; ++i)
    if (!ci_equal(p[i], (unsigned char)v[i])) return 0;
  return 1;
}

int v12_https_validate_response(const uint8_t *data, size_t size,
                                v12_https_response *out) {
  size_t i, end = 0, line, body_at, body_size = 0;
  int status = 0, got_len = 0, got_type = 0;
  if (!data || !out || size < 19 ||
      size > V12_HTTPS_MAX_HEADER + V12_HTTPS_MAX_PNG) return -1;
  memset(out, 0, sizeof(*out));
  for (i = 0; i + 3 < size && i < V12_HTTPS_MAX_HEADER; ++i)
    if (!memcmp(data + i, "\r\n\r\n", 4)) { end = i; break; }
  if (end < 13 || end >= V12_HTTPS_MAX_HEADER ||
      memcmp(data, "HTTP/1.", 7) ||
      (data[7] != '0' && data[7] != '1') || data[8] != ' ' ||
      data[9] < '0' || data[9] > '9' ||
      data[10] < '0' || data[10] > '9' ||
      data[11] < '0' || data[11] > '9' ||
      (data[12] != ' ' && data[12] != '\r')) return -1;
  status = (data[9] - '0') * 100 + (data[10] - '0') * 10 +
           (data[11] - '0');
  if (status != 200 && status != 404) return -1; /* No redirects yet. */
  for (line = 0; line + 1 < end; ++line)
    if (data[line] == '\r' && data[line + 1] == '\n') break;
  if (line + 1 >= end) return -1;
  line += 2;
  while (line < end) {
    size_t stop = line, pos;
    while (stop + 1 < end &&
           !(data[stop] == '\r' && data[stop + 1] == '\n')) ++stop;
    if (stop + 1 >= end) stop = end;
    if (stop <= line) return -1;
    if (prefix(data + line, stop - line, "Content-Length:")) {
      if (got_len++) return -1;
      pos = line + 15;
      while (pos < stop && (data[pos] == ' ' || data[pos] == '\t')) ++pos;
      if (pos == stop) return -1;
      for (; pos < stop; ++pos) {
        if (data[pos] < '0' || data[pos] > '9') return -1;
        if (body_size > V12_HTTPS_MAX_PNG / 10U) return -1;
        body_size = body_size * 10U + (unsigned)(data[pos] - '0');
        if (body_size > V12_HTTPS_MAX_PNG) return -1;
      }
    } else if (prefix(data + line, stop - line, "Content-Type:")) {
      if (got_type++) return -1;
      pos = line + 13;
      while (pos < stop && (data[pos] == ' ' || data[pos] == '\t')) ++pos;
      if (status == 200 && !value(data + pos, stop - pos, "image/png"))
        return -1;
    } else if (prefix(data + line, stop - line, "Transfer-Encoding:") ||
               prefix(data + line, stop - line, "Content-Encoding:")) {
      return -1; /* Chunked, compressed or ambiguous body not supported. */
    }
    line = stop + 2;
  }
  body_at = end + 4;
  if (body_at > size) return -1;
  if (got_len && size - body_at != body_size) return -1;
  if (status == 200) {
    if (!got_len || !got_type || body_size < 57 ||
        probe_png_check(data + body_at, body_size) < 0) return -1;
  } else if (size - body_at > V12_HTTPS_MAX_PNG) {
    return -1;
  }
  out->status = status;
  out->body_offset = body_at;
  out->body_size = size - body_at;
  return status == 200 ? 0 : 1;
}
