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
