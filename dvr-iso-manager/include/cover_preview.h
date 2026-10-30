#ifndef DESR_COVER_PREVIEW_H
#define DESR_COVER_PREVIEW_H
/* Decode the selected OPL PNG to an 88x128 RGB bitmap and transfer it to
   PS2SDK's already initialized 640x224 debug framebuffer through GIF DMA.
   No gsKit screen init. 1 = drawn, 0 = missing / invalid / unsupported. */
int desr_cover_preview_png(const char *game_id, const char *ext);
#endif
