#include "lan_network_guard.h"

int desr_lan_network_begin(desr_lan_network_guard *guard) {
  if (!guard || guard->state != 0) return -1;
  /* Before loading any IRX, permanently mark this ELF session as tried.
     If an IRX fails, do not repeat its load on a later X press. */
  guard->state = -1;
  guard->stage = DESR_NET_STAGE_NETMAN;
  return 0;
}

void desr_lan_network_stage(desr_lan_network_guard *guard, int stage) {
  if (!guard || guard->state != -1) return;
  if (stage >= DESR_NET_STAGE_NETMAN && stage <= DESR_NET_STAGE_LINK)
    guard->stage = stage;
}

void desr_lan_network_ready(desr_lan_network_guard *guard) {
  if (!guard || guard->state != -1) return;
  guard->state = 1;
  guard->stage = DESR_NET_STAGE_READY;
}
