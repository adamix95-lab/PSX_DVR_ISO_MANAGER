#ifndef DESR_USB12_COVERS_H
#define DESR_USB12_COVERS_H

#include "runtime_storage.h"
#include "n6_usb_art.h"

#ifdef DESR_USB12_ONE_ELF
#ifdef DESR_USB12_CPU_ENTROPY
/* The USB 1.2 production-format package replaces the 1.0 main ELF.
 * Both the embedded worker loader and return loader MUST use this path. */
#define DESR_USB12_MANAGER_ELF DESR_APP_ROOT "DVR_ISO_MANAGER.ELF"
#else
#define DESR_USB12_MANAGER_ELF DESR_APP_ROOT "DVR_ISO_MANAGER_V12_SINGLE.ELF"
#endif
int desr_run_embedded_https(char *language);
#ifdef DESR_USB12_TAIL_PAYLOAD
const char *desr_usb12_loader_error(int result);
#endif
#else
#define DESR_USB12_MANAGER_ELF DESR_APP_ROOT "DVR_ISO_MANAGER_V12.ELF"
#define DESR_USB12_HTTPS_ELF DESR_APP_ROOT "COVER_HTTPS_V12.ELF"
#endif
#define DESR_USB12_CA_FILE DESR_APP_ROOT "N4E.CA"
#define DESR_USB12_NET_CNF DESR_APP_ROOT "V12_NET.CNF"
#define DESR_USB12_ENTROPY_FILE "mass0:/N6G50.ENT"
#define DESR_USB12_ART_RESULT "--usb12-art-result"
#define DESR_USB12_ROUGHTIME_NOT_CHECKED 1

#endif
