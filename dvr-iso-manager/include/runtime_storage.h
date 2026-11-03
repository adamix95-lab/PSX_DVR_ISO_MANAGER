#ifndef DESR_RUNTIME_STORAGE_H
#define DESR_RUNTIME_STORAGE_H

/* Common v1 menu, two packaging targets.
 * HDD_PREVIEW must never write files or start games while PP.DVRISO/PFS
 * driver initialization and write-verification are not hardware-tested.
 *
 * DEV NOTE: This is an APPLICATION partition only. dvr_pfs0:/ISO remains
 * a separate Sony DVR file store and is never writable by the Manager.
 */
#if defined(DESR_HDD_PREVIEW) || defined(DESR_HDD_TEST)
#define DESR_APP_VARIANT "1.0 HDD"
#define DESR_APP_ROOT "pfs0:/"
#define DESR_APP_PARTITION "hdd0:PP.DVRISO"
#ifdef DESR_HDD_TEST
#define DESR_APP_READONLY 0
#else
#define DESR_APP_READONLY 1
#endif
#define DESR_APP_HDD_CNF_ARG "hdd0:PP.DVRISO/BOOT.CNF"
#define DESR_APP_SYSTEM_NEUTRINO "pfs0:/neutrino_hdd.elf"
#define DESR_APP_ENGLISH_NEUTRINO "pfs0:/neutrino_hdd_lang.elf"
#else
#define DESR_APP_VARIANT "1.0 USB"
#define DESR_APP_ROOT "mass0:/DESR_DVR/"
#define DESR_APP_READONLY 0
#define DESR_APP_SYSTEM_NEUTRINO "mass0:/DESR_DVR/neutrino.elf"
#define DESR_APP_ENGLISH_NEUTRINO "mass0:/DESR_DVR/neutrino_lang.elf"
#endif

#define DESR_APP_BOOT_CNF DESR_APP_ROOT "BOOT.CNF"
#define DESR_APP_BOOT_NEXT DESR_APP_ROOT "BOOT.NEXT.CNF"
#define DESR_APP_BOOT_PREV DESR_APP_ROOT "BOOT.PREV.CNF"
#define DESR_APP_TEST_CNF DESR_APP_ROOT "DVR_TEST.CNF"
#define DESR_APP_BACKUP_CNF DESR_APP_ROOT "BOOT_RECOVERY.CNF"
#define DESR_APP_OSDMENU DESR_APP_ROOT "BOOT.ELF"
#define DESR_APP_EECORE_LANG DESR_APP_ROOT "modules/ee_core_lang.elf"
#define DESR_APP_LANG_CFG DESR_APP_ROOT "config/osdlang.toml"
#define DESR_APP_NET_CFG DESR_APP_ROOT "NET_PROBE.CNF"
#define DESR_APP_ART_ROOT DESR_APP_ROOT "ART/"
#define DESR_APP_COVERS_IDX DESR_APP_ROOT "COVERS.IDX"
#define DESR_APP_COVERS_NEW DESR_APP_ROOT "COVERS.NEW"
#define DESR_APP_COVERS_BAK DESR_APP_ROOT "COVERS.BAK"
#define DESR_APP_COVER_SCAN_LOG DESR_APP_ROOT "COVER_SCAN.LOG"

#endif
