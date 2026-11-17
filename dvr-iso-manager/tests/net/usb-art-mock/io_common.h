#pragma once
#define FIO_O_RDONLY 1
#define FIO_O_WRONLY 2
#define FIO_O_CREAT 0x0200
#define FIO_O_EXCL 0x0800
typedef struct { int unused; } iox_stat_t;
typedef struct { char name[256]; } iox_dirent_t;
