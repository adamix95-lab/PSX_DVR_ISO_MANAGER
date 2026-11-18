#ifndef PDIM_HDD12_INSTALLER_H
#define PDIM_HDD12_INSTALLER_H
#include "pdim12_policy.h"
#include <stdint.h>

/* Network config is optional and NOT counted among the 79 Core files. */
#define PDIM12_NET_NOT_CHECKED 0
#define PDIM12_NET_COPIED 1
#define PDIM12_NET_KEPT 2
#define PDIM12_NET_NEEDS_SETUP 3

typedef struct {
    int exists;
    int pfs_ok;
    int created_this_run;
    uint32_t start;
    uint32_t sectors;
    uint32_t last_end;
    int files_total;
    int files_copied;
    int files_existing;
    int files_verified;
    int net_config_status;
} pdim12_state;

int pdim12_probe(pdim12_state *state);
int pdim12_mount(int readonly);
void pdim12_unmount(void);
int pdim12_prepare_partition(pdim12_state *state);
int pdim12_install_files(pdim12_state *state);
int pdim12_verify_files(pdim12_state *state);
int pdim12_verify_core_files(pdim12_state *state);
int pdim12_preflight_xmb_assets(pdim12_state *state);
int pdim12_install_xmb_assets(pdim12_state *state);
int pdim12_inspect_xmb(pdim12_state *state);
int pdim12_install_xmb(pdim12_state *state);
const char *pdim12_error(int rc);

#endif
