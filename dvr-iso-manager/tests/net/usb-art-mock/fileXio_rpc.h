#pragma once
#include "io_common.h"
int fileXioOpen(const char *,int,int);
int fileXioGetStat(const char *,iox_stat_t *);
int fileXioClose(int);
int fileXioRead(int,void *,int);
int fileXioWrite(int,const void *,int);
int fileXioLseek(int,int,int);
int fileXioMkdir(const char *,int);
int fileXioRename(const char *,const char *);
int fileXioDopen(const char *);
int fileXioDread(int,iox_dirent_t *);
int fileXioDclose(int);
