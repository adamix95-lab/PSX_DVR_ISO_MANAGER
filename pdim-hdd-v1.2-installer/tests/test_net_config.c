#include "v12_net_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc,char **argv) {
    char data[V12_NET_CONFIG_MAX+2];
    v12_net_config cfg;
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");
    assert(f!=NULL);
    size_t n=fread(data,1,sizeof(data),f);
    assert(ferror(f)==0 && feof(f)!=0 && fclose(f)==0);
    assert(n>0 && n<=V12_NET_CONFIG_MAX);
    assert(v12_parse_net_config(data,n,&cfg)==0);
    assert(cfg.ip[0]==192 && cfg.ip[1]==168 && cfg.ip[2]==0 && cfg.ip[3]==240);
    assert(cfg.gateway[0]==192 && cfg.gateway[1]==168 && cfg.gateway[2]==0 && cfg.gateway[3]==1);
    assert(cfg.dns[0]==1 && cfg.dns[1]==1 && cfg.dns[2]==1 && cfg.dns[3]==1);
    const char *bad="IP=192.168.0.240\nNETMASK=255.255.255.0\nGATEWAY=192.168.0.1\n";
    assert(v12_parse_net_config(bad,strlen(bad),&cfg)!=0);
    const char *dup="IP=192.168.0.240\nIP=192.168.0.1\nNETMASK=255.255.255.0\nGATEWAY=192.168.0.1\nDNS=1.1.1.1\n";
    assert(v12_parse_net_config(dup,strlen(dup),&cfg)!=0);
    const char *invalid="IP=192.168.0.999\nNETMASK=255.255.255.0\nGATEWAY=192.168.0.1\nDNS=1.1.1.1\n";
    assert(v12_parse_net_config(invalid,strlen(invalid),&cfg)!=0);
    puts("PASS: supplied HTTPS network config and invalid/missing key rejection");
    return 0;
}
