#include "ppaa_plan.h"
#include <stdio.h>
#include <string.h>
#define B PDIM12_PPAA_SECTOR_BYTES
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"FAILED line %d: %s\n",__LINE__,#x);return 1;} } while(0)
static uint8_t current[3][B], proposed[3][B];
static const uint8_t sys[]="BOOT2 = pfs:/EXECUTE.ELF\n";
static const uint8_t icon[]="PS2X\ntitle0=PSX DVR ISO MANAGER\ntitle1=HDD12 TEST\n";
static int plan(void){
    return pdim12_ppaa_plan(current,sys,sizeof(sys)-1,icon,sizeof(icon)-1,proposed);
}
int main(void){
    current[0][0x90]=0x55; current[0][0xFF]=0xAB;
    current[1][200]=1; current[2][230]=2;
    CHECK(plan()==0);
    CHECK(proposed[0][0x90]==0x55 && proposed[0][0xFF]==0xAB);
    CHECK(memcmp(proposed[0],"PS2ICON3D",9)==0);
    CHECK(memcmp(proposed[1],sys,sizeof(sys)-1)==0);
    CHECK(memcmp(proposed[2],icon,sizeof(icon)-1)==0);
    CHECK(proposed[1][200]==0 && proposed[2][230]==0);
    CHECK(proposed[0][0x20]==0 && proposed[0][0x24]==0); /* no fake PNG list.ico */
    memcpy(current,proposed,sizeof(current));
    CHECK(plan()==1);
    current[2][0]^=1; CHECK(plan()==-2);
    memset(current,0,sizeof(current));
    current[0][0x24]=1; CHECK(plan()==-2);
    current[0][0x24]=0; current[0][0]=0xFF; CHECK(plan()==-2);
    current[0][0]=0; CHECK(plan()==0);
    CHECK(pdim12_ppaa_plan(current,sys,B+1,icon,10,proposed)==-1);
    puts("PASS: wLE-style target PPAA planning, preserving bytes, foreign protection, idempotence");
    return 0;
}
