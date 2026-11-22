#include "sg_policy.h"
#include "cover_index.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* FIX95's exact -dvd argument validator imposes a 95-byte filename
 * limit and rejects non-ASCII, equals, parent traversal, /, : and backslash.
 * Enforce it BEFORE any new APA/PFS is created. */
int sg_iso_filename_allowed(const char *filename) {
    size_t n;
    if (!filename || !(n=strlen(filename)) || n>95 || n<=4 ||
        filename[0]=='.' || strstr(filename,".."))return 0;
    if(filename[n-4]!='.' ||
       (filename[n-3]!='i' && filename[n-3]!='I') ||
       (filename[n-2]!='s' && filename[n-2]!='S') ||
       (filename[n-1]!='o' && filename[n-1]!='O'))return 0;
    for(size_t i=0;i<n;i++){
        unsigned char c=(unsigned char)filename[i];
        if(c<0x20 || c>0x7e || c=='=' || c==':' || c=='/' || c=='\\')return 0;
    }
    return 1;
}
int sg_target_partition(const char *filename,const char *game_id,
                        char *partition,size_t cap) {
    char stem[11]={0},suffix[10]={0};
    int j=0,k=0,n;
    if(!partition || cap==0)return 0;
    partition[0]=0;
    if(!sg_iso_filename_allowed(filename)||!desr_valid_game_id(game_id))return 0;
    for(const unsigned char *p=(const unsigned char*)filename;*p && j<10;p++){
        if(*p=='.' && !strcasecmp((const char*)p,".iso"))break;
        if(*p>='a' && *p<='z')stem[j++]=(char)(*p-'a'+'A');
        else if((*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9'))stem[j++]=(char)*p;
    }
    for(const char *p=game_id;*p && k<9;p++){
        if((*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')) suffix[k++]=*p;
    }
    if(j<1 || k!=9)return 0;
    n=snprintf(partition,cap,"PP.%s-%s",stem,suffix);
    if(n<=0 || (size_t)n>=cap || n>=32) {
        partition[0]=0;
        return 0;
    }
    return 1;
}
