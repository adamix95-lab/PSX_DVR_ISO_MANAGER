#include "lan_network_guard.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  desr_lan_network_guard g = {0, 0}, h = {0, 0};
  assert(desr_lan_network_begin(&g) == 0);
  assert(g.state == -1 && g.stage == DESR_NET_STAGE_NETMAN);
  desr_lan_network_stage(&g, DESR_NET_STAGE_SMAP);
  assert(g.stage == DESR_NET_STAGE_SMAP);
  /* IRX initialization failed: later modal retry MUST NOT reload IRXs. */
  assert(desr_lan_network_begin(&g) < 0);
  assert(g.state == -1 && g.stage == DESR_NET_STAGE_SMAP);
  desr_lan_network_stage(&g, DESR_NET_STAGE_LINK);
  desr_lan_network_ready(&g);
  assert(g.state == 1 && g.stage == DESR_NET_STAGE_READY);
  assert(desr_lan_network_begin(&g) < 0);
  /* A separate newly-launched ELF would start with a fresh guard. */
  assert(desr_lan_network_begin(&h) == 0);
  puts("OK: fail-closed network IRX init; no reloading after partial failure");
  return 0;
}
