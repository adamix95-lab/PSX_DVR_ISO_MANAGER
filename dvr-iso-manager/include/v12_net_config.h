#ifndef DESR_V12_NET_CONFIG_H
#define DESR_V12_NET_CONFIG_H
#include <stddef.h>
#include <stdint.h>

/* The 1.2-only V12_NET.CNF has NO PC HTTP server settings and never writes
 * to the disk. It is separate from the stable NET_PROBE.CNF format.
 */
#define V12_NET_CONFIG_MAX 512U
typedef struct {
  uint8_t ip[4], netmask[4], gateway[4], dns[4];
} v12_net_config;

/* Require exactly IP/NETMASK/GATEWAY/DNS and reject duplicates or unknown
 * options. Return 0 on success; leave out untouched on error. */
int v12_parse_net_config(const char *data, size_t length,
                         v12_net_config *out);
#endif
