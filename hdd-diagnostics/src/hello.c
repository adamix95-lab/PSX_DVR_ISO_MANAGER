/* Minimal PSX screen test. Does NOT access HDD, USB, RPC or IOP. */
#include <debug.h>
#include <unistd.h>

int main(void)
{
    init_scr();
    scr_setCursor(0);
    scr_setbgcolor(0x000000);
    scr_clear();

    scr_printf("PSX DVR v1.0 - HELLO TEST\n");
    scr_printf("SCREEN INITIALIZED SUCCESSFULLY\n");
    scr_printf("THIS PROGRAM DOES NOT TOUCH THE HDD\n");
    scr_printf("NO RPC / NO FILEXIO / NO IOP RESET\n");
    scr_printf("If you can read this, take a photo.\n");
    scr_printf("If screen stays black, report that.\n");

    for (;;)
        sleep(1);
    return 0;
}
