#ifndef PDIM_HDD12_INSTALLER_H
#define PDIM_HDD12_INSTALLER_H
#include "pdim12_policy.h"
#include <stdint.h>

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
