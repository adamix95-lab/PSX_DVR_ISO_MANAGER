#include "probe_config.h"
#include <string.h>
#include <stdint.h>

/* Reject malformed, overflowing or ambiguous IPv4 strings. */
static int parse_ipv4(const char *s, uint8_t out[4]) {
  int i;
  if (!s || !*s) return -1;
  for (i = 0; i < 4; i++) {
    unsigned part = 0, digits = 0;
    while (*s >= '0' && *s <= '9') {
      if (++digits > 3) return -1;
      part = 10U * part + (unsigned)(*s++ - '0');
      if (part > 255U) return -1;
    }
    if (!digits) return -1;
    out[i] = (uint8_t)part;
    if (i < 3) {
      if (*s++ != '.') return -1;
    }
  }
  return *s == '\0' ? 0 : -1;
}
static int parse_port(const char *s, unsigned short *out) {
  unsigned n = 0;
  int seen = 0;
  if (!s || !*s) return -1;
  for (; *s; s++) {
    if (*s < '0' || *s > '9') return -1;
    n = n * 10U + (unsigned)(*s - '0');
    if (n > 65535U) return -1;
    seen = 1;
  }
  if (!seen || !n) return -1;
  *out = (unsigned short)n;
  return 0;
}
int probe_parse_config(const char *source, size_t size, probe_config *out) {
  size_t at = 0;
  unsigned seen = 0;
  probe_config temp = {{0}, {0}, {0}, {0}, 0};
  if (!source || !out || !size || size > 512) return -1;
  while (at < size) {
    char line[64], *key, *val;
    size_t len = 0;
    int id, rc;
    while (at < size && source[at] != '\r' && source[at] != '\n') {
      if (len + 1 >= sizeof(line)) return -1;
      line[len++] = source[at++];
    }
    while (at < size && (source[at] == '\r' || source[at] == '\n')) at++;
    line[len] = '\0';
    if (!len || line[0] == '#') continue;
    key = line;
    val = strchr(line, '=');
    if (!val) return -1;
    *val++ = '\0';
    if (!strcmp(key, "IP")) id = 0;
    else if (!strcmp(key, "NETMASK")) id = 1;
    else if (!strcmp(key, "GATEWAY")) id = 2;
    else if (!strcmp(key, "SERVER_IP")) id = 3;
    else if (!strcmp(key, "SERVER_PORT")) id = 4;
    else return -1;
    if (seen & (1U << id)) return -1;
    if (id == 0) rc = parse_ipv4(val, temp.ip);
    else if (id == 1) rc = parse_ipv4(val, temp.netmask);
    else if (id == 2) rc = parse_ipv4(val, temp.gateway);
    else if (id == 3) rc = parse_ipv4(val, temp.server);
    else rc = parse_port(val, &temp.port);
    if (rc) return -1;
    seen |= 1U << id;
  }
  if (seen != 31U) return -1;
  if (temp.ip[0] == 0 || temp.ip[0] >= 224 ||
      temp.server[0] == 0 || temp.server[0] >= 224 ||
      (temp.netmask[0] | temp.netmask[1] |
       temp.netmask[2] | temp.netmask[3]) == 0) return -1;
  *out = temp;
  return 0;
}
