/* DESR native QUIT GAME setup. Uses the ROM CDVDFSV before EE emulation.
 * Reference: pcm720/OSDMenu switchPSX(), notice-game-start mode 1;
 * PS2SDK EE libcdvd sends one byte to Mechacon SCMD 0x29.
 * No pad hook, resident worker, BIOS patches or software reset involved.
 */
#ifndef DESR_QUIT_H
#define DESR_QUIT_H

#define DESR_QUIT_ATTEMPTS 16

static int desr_enable_quit_game(const char *phase)
{
    unsigned char mode = 1;
    unsigned char response[16];
    int attempt, transport = 0;
    unsigned int status = 0xff;

    if (!sceCdInit(SCECdINoD)) {
        printf("DESR QUIT: CDVD init failed (%s)\n", phase);
        return -1;
    }

    for (attempt = 0; attempt < DESR_QUIT_ATTEMPTS; attempt++) {
        /* Never mistake an unchanged output buffer for an accepted command. */
        memset(response, 0xff, sizeof(response));
        transport = sceCdApplySCmd(0x29, &mode, 1, response);
        status = response[0];
        if (transport == 1 && status == 0)
            break;
        /* A non-busy Mechacon error needs diagnosis, not endless retries. */
        if (transport == 1 && !(status & 0x80))
            break;
        nopdelay();
    }
    sceCdInit(SCECdEXIT);

    printf("DESR QUIT: mode=1 bytes=1 rpc=%d status=%02x (%s)\n",
           transport, status, phase);
    return (transport == 1 && status == 0) ? 0 : -1;
}

#endif
