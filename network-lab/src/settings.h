#ifndef DESR_NETWORK_LAB_SETTINGS_H
#define DESR_NETWORK_LAB_SETTINGS_H

/* IDENTICAL, reproducible 44-game manifest for A/B/C (from the supplied
   DESR_DVR/COVER_SCAN.LOG). No disk access is needed by variant A or B. */
#define LAB_GAME_COUNT 44
static const char *const lab_ids[LAB_GAME_COUNT] = {
  "SLES_502.82",
  "SLES_538.86",
  "SLES_525.84",
  "SLES_546.27",
  "SLES_535.07",
  "SLUS_210.47",
  "SLES_529.63",
  "SLUS_206.49",
  "SLUS_210.42",
  "SLES_503.58",
  "SCUS_974.81",
  "SCUS_973.99",
  "SCUS_973.28",
  "SLES_525.41",
  "SLUS_205.52",
  "SLES_505.04",
  "SLES_528.77",
  "SCUS_971.13",
  "SCES_503.61",
  "SCUS_974.02",
  "SCES_509.67",
  "SLES_522.78",
  "SLES_820.42",
  "SLUS_215.50",
  "SLES_503.83",
  "SLES_501.34",
  "SLUS_201.38",
  "SLES_512.22",
  "SLES_524.78",
  "SLUS_203.44",
  "SLUS_211.11",
  "SCUS_974.72",
  "SLES_511.56",
  "SLES_514.34",
  "SLES_524.45",
  "SLES_555.69",
  "SLES_518.20",
  "SCUS_974.74",
  "SCUS_972.75",
  "SCUS_971.34",
  "SLES_525.45",
  "SLUS_210.59",
  "SLES_539.91",
  "SCES_500.05"
};

/* LAN settings recorded in the user's provided NET_PROBE.CNF. These are
   private-LAN IPv4 addresses, not remote web addresses. To use a different
   network, edit and rebuild ALL three programs with identical settings. */
#define LAB_IP_A 192
#define LAB_IP_B 168
#define LAB_IP_C 0
#define LAB_IP_D 240
#define LAB_MASK_A 255
#define LAB_MASK_B 255
#define LAB_MASK_C 255
#define LAB_MASK_D 0
#define LAB_GW_A 192
#define LAB_GW_B 168
#define LAB_GW_C 0
#define LAB_GW_D 1
#define LAB_PC_A 192
#define LAB_PC_B 168
#define LAB_PC_C 0
#define LAB_PC_D 122
#define LAB_PC_PORT 8000

/* EXACT, dedicated sandbox partition. NEVER PP.DVRISO / PP.TESTDVR.
   This project NEVER creates or formats partitions automatically. */
#define LAB_PARTITION "hdd0:PP.NETTEST"
#define LAB_MOUNT "pfs1:"
#define LAB_ART "pfs1:/ART/"
#define LAB_REPORT "pfs1:/REPORT_C.TXT"
#define LAB_RECV_LIMIT 512U
#endif
