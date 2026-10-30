#ifndef DESR_PROBE_CONFIG_H
#define DESR_PROBE_CONFIG_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t ip[4], netmask[4], gateway[4], server[4];
  unsigned short port;
} probe_config;

/* Parse a small, complete PC-authored file. No writes are performed.
   Return 0 when all five fields are present exactly once. */
int probe_parse_config(const char *source, size_t size, probe_config *out);

#endif
