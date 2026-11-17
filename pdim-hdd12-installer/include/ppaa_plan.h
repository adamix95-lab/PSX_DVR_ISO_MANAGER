#ifndef PDIM12_PPAA_PLAN_H
#define PDIM12_PPAA_PLAN_H
#include <stddef.h>
#include <stdint.h>

#define PDIM12_PPAA_SECTOR_BYTES 512u
/*
 * Plan an XMB/PS2ICON3D header modification in RAM.
 * The physical sector at partition_start+8 has the attribute descriptors.
 * +9 stores system.cnf, +10 stores icon.sys. jkt_*.png and info.sys
 * belong to the partition's PFS res/ directory, NOT to the list.ico slot.
 *
 * Result: 0 = fresh header can be committed, 1 = already matches,
 *        -1 = bad input, -2 = foreign/incomplete header, NO WRITE.
 */
int pdim12_ppaa_plan(
    const uint8_t current[3][PDIM12_PPAA_SECTOR_BYTES],
    const uint8_t *sys, size_t sys_len,
    const uint8_t *icon, size_t icon_len,
    uint8_t proposed[3][PDIM12_PPAA_SECTOR_BYTES]);
#endif
