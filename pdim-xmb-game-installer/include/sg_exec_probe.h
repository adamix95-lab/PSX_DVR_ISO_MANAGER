#ifndef SG_EXEC_PROBE_H
#define SG_EXEC_PROBE_H
#include <stdint.h>
/* Strict exact-byte comparison used only for reporting. Do NOT use this
 * structure to bypass the existing XMB/APA write preflight. */
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
  int actual_complete;
  uint32_t first_difference;
  uint32_t different_bytes;
  int first_difference_valid;
  uint32_t repeat_size, repeat_crc;
  int repeat_complete;
  int repeat_stable; /* only meaningful when repeat_complete == 1 */
  int repeat_error;  /* the second independent read failed */
} sg_exec_probe;
int sg_probe_execute(const unsigned char *expected, uint32_t size,
                     sg_exec_probe *out);
#endif
