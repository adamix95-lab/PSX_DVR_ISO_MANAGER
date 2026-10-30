#ifndef DESR_COVER_PREVIEW_H
#define DESR_COVER_PREVIEW_H
/* Decodes OPL PNG cover on demand and paints a low-resolution 11x14
   color thumbnail through the existing PS2SDK debug framebuffer.
   Returns 1 when drawn; 0 when missing or unsupported. No GS reinit. */
int desr_cover_preview_png(const char *game_id, const char *ext);
#endif
