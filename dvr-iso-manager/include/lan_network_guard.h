#ifndef DESR_LAN_NETWORK_GUARD_H
#define DESR_LAN_NETWORK_GUARD_H

/* PS2 IRXs cannot safely be assumed unloadable after a partial failure.
   Keep the init state across UI scans and cancel/retry events. */
typedef struct {
  int state; /* 0=untouched, -1=attempted/failed, 1=ready */
  int stage; /* for diagnostics only */
} desr_lan_network_guard;

enum {
  DESR_NET_STAGE_NONE = 0,
  DESR_NET_STAGE_NETMAN = 1,
  DESR_NET_STAGE_SMAP = 2,
  DESR_NET_STAGE_PS2IP = 3,
  DESR_NET_STAGE_LINK = 4,
  DESR_NET_STAGE_READY = 5
};

int desr_lan_network_begin(desr_lan_network_guard *guard);
void desr_lan_network_stage(desr_lan_network_guard *guard, int stage);
void desr_lan_network_ready(desr_lan_network_guard *guard);

#endif
