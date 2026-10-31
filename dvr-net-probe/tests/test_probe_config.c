#include "probe_config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char config[] =
  "# NET PROBE ONLY\n"
  "IP=192.168.1.220\r\n"
  "NETMASK=255.255.255.0\n"
  "GATEWAY=192.168.1.1\n"
  "SERVER_IP=192.168.1.100\n"
  "SERVER_PORT=8000\n";
int main(void) {
  probe_config c;
  char bad[512];
  assert(probe_parse_config(config, strlen(config), &c) == 0);
  /* Regression: original shipped example had a 64-char comment and
     was rejected even though all five IPv4/port fields were valid. */
  {
    const char *actual_user_config =
      "# Copy as mass0:/DESR_DVR/NET_PROBE.CNF and edit for YOUR LAN.\n"
      "# Choose a free static IP outside your router DHCP pool.\n"
      "# Only this standalone test reads this file. No writes anywhere.\n"
      "IP=192.168.0.240\n"
      "NETMASK=255.255.255.0\n"
      "GATEWAY=192.168.0.1\n"
      "SERVER_IP=192.168.0.122\n"
      "SERVER_PORT=8000\n";
    assert(strlen("# Only this standalone test reads this file. No writes anywhere.") == 64);
    assert(probe_parse_config(actual_user_config, strlen(actual_user_config), &c) == 0);
    assert(c.ip[3] == 240 && c.server[3] == 122 && c.port == 8000);
  }
  /* Restore the baseline fixture before checking its field values. */
  assert(probe_parse_config(config, strlen(config), &c) == 0);
  assert(c.ip[0] == 192 && c.ip[3] == 220);
  assert(c.server[3] == 100 && c.port == 8000);
  strcpy(bad, config);
  assert(probe_parse_config(bad, strlen(bad) - 10, &c) < 0);
  strcpy(bad, config);
  {
    char *p = strstr(bad, "192.168.1.220");
    assert(p);
    memcpy(p, "999.168.1.220", 13);
  }
  assert(probe_parse_config(bad, strlen(bad), &c) < 0);
  strcpy(bad, config);
  strcat(bad, "IP=192.168.1.200\n");
  assert(probe_parse_config(bad, strlen(bad), &c) < 0);
  strcpy(bad, config);
  {
    char *p = strstr(bad, "SERVER_PORT=8000");
    assert(p);
    memcpy(p, "SERVER_PORT=0000", 16);
  }
  assert(probe_parse_config(bad, strlen(bad), &c) < 0);
  assert(probe_parse_config(config, 513, &c) < 0);
  puts("OK: strict bounded IPv4/static local HTTP probe configuration");
  return 0;
}
