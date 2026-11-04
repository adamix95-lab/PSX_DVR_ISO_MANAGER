#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SCECdINoD 1
#define SCECdEXIT 5
static int init_ok, commands, exits, busy_count, drop_count, error_status, delays;
static int sceCdInit(int mode)
{
    if (mode == SCECdEXIT) { exits++; return 1; }
    assert(mode == SCECdINoD);
    return init_ok;
}
static void nopdelay(void) { delays++; }
static int sceCdApplySCmd(unsigned char command, const void *input,
                         unsigned short length, void *output)
{
    unsigned char *out = output;
    commands++;
    assert(command == 0x29 && length == 1);
    assert(*(const unsigned char *)input == 1);
    if (drop_count-- > 0) return 0; /* No response was written. */
    out[0] = busy_count-- > 0 ? 0x80 : error_status;
    return 1;
}

#include "../desr_quit.h"

static void reset(void)
{
    init_ok = 1;
    commands = exits = busy_count = drop_count = error_status = delays = 0;
}
int main(void)
{
    reset();
    assert(desr_enable_quit_game("success") == 0);
    assert(commands == 1 && exits == 1 && delays == 0);
    reset(); busy_count = 2; drop_count = 1;
    assert(desr_enable_quit_game("transient busy/transport") == 0);
    assert(commands == 4 && exits == 1);
    reset(); drop_count = 100;
    assert(desr_enable_quit_game("missing response") == -1);
    assert(commands == DESR_QUIT_ATTEMPTS && exits == 1);
    reset(); busy_count = 100;
    assert(desr_enable_quit_game("stuck busy") == -1);
    assert(commands == DESR_QUIT_ATTEMPTS && exits == 1);
    reset(); error_status = 2;
    assert(desr_enable_quit_game("Mechacon rejection") == -1);
    assert(commands == 1 && exits == 1);
    reset(); init_ok = 0;
    assert(desr_enable_quit_game("no CDVD") == -1);
    assert(commands == 0 && exits == 0);
    puts("PASS: mode/size, busy, transport, rejection, bounded retries, init failure");
}
