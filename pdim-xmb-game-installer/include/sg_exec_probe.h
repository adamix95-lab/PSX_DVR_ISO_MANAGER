#ifndef SG_EXEC_PROBE_H
#define SG_EXEC_PROBE_H
#include <stdint.h>
/* Diagnostic ONLY: this does not change sg_verify_mounted's strict policy
 * required before XMB APA header writes. */
enum sg_exec_status {
  SG_EXEC_IDENTICAL=0,
  SG_EXEC_MISSING,
  SG_EXEC_DIR_ERROR,
  SG_EXEC_OPEN_ERROR,
  SG_EXEC_READ_ERROR,
  SG_EXEC_SIZE_DIFF,
  SG_EXEC_CRC_DIFF
};
typedef struct {
  enum sg_exec_status status;
  uint32_t expected_size, expected_crc;
  uint32_t actual_size, actual_crc;
  int actual_complete; /* CRC is meaningful only after successful EOF */
} sg_exec_probe;
int sg_probe_execute(const unsigned char *expected, uint32_t size,
                     sg_exec_probe *out);
#endif
