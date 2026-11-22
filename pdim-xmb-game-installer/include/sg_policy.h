#ifndef SG_POLICY_H
#define SG_POLICY_H
#include <stddef.h>
/* Pure host-testable policy. Reject ISO names not accepted by FIX95.
 * The existing 1.2 Manager's scanning rules are broader, intentionally:
 * this XMB installer must never create an entry Neutrino will reject. */
int sg_iso_filename_allowed(const char *filename);
int sg_target_partition(const char *filename,const char *game_id,
                        char *partition,size_t capacity);
#endif
