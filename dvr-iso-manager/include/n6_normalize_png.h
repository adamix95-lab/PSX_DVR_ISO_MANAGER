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
#ifdef N6G_PNG_WORKSPACE
/* The caller owns the output buffer. It may overlap input: encoding starts
 * after the reader is destroyed. Scratch is allocated only for this call. */
int n6_normalize_png_into(const uint8_t *input,size_t bytes,
                          uint8_t *output,size_t capacity,size_t *output_len);
#endif
#endif
