#ifndef N6_NORMALIZE_PNG_H
#define N6_NORMALIZE_PNG_H
#include <stddef.h>
#include <stdint.h>
/* Exactly 140x200 RGB PNG like PSX DVR ART Server 0.8.
 * Decode bounded input <=256KiB, dimensions <=512x512, no interlace.
 * Preserve aspect ratio and letterbox with black, no stretching.
 * Output to private static RAM buffer; no file/network I/O.
 */
int n6_normalize_png(const uint8_t *input,size_t bytes,
                     const uint8_t **output,size_t *output_len);
#endif
