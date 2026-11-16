#include "n6g_screen_probe.h"
#include <stdarg.h>
#include <debug.h>

static int screen_muted;

void n6g_mute_screen(int muted) {
    screen_muted = muted;
}

int n6g_screen_muted(void) {
    return screen_muted;
}

/* The EE debug console performs blocking GIF DMA on every character.
 * Link wrappers keep the QUIET and VERBOSE modes in the same ELF. */
void __wrap_scr_printf(const char *format, ...) {
    va_list args;
    if (screen_muted)
        return;
    va_start(args, format);
    scr_vprintf(format, args);
    va_end(args);
}

void __real_scr_clear(void);

void __wrap_scr_clear(void) {
    if (!screen_muted)
        __real_scr_clear();
}
