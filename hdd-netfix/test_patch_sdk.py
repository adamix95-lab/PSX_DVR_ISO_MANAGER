"""Host regression of the actual patched NETMAN receive functions.

The fake IOP overwrites a bounce slot immediately when it is rearmed.
This catches a copy-after-rearm race, ring wrap errors, and oversized frames.
Hardware DMA/cache behaviour still requires a physical PSX.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import patch_sdk


def function(source, signature):
    start = source.index(signature + "\n{")
    end = source.index("\n}\n", start) + 3
    return source[start:end]


HARNESS = r'''
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
#define NETMAN_RPC_BLOCK_SIZE 64
#define NETMAN_MAX_FRAME_SIZE 1536
#define NETMAN_NETIF_FRAME_SIZE 1514
#define UNCACHED_SEG(p) (p)
#define DI() ((void)0)
#define EI() ((void)0)
struct NetManBD { unsigned short length, offset; void *packet, *payload; u32 unused; };
typedef struct { void *src, *dest; unsigned size, attr; } SifDmaTransfer_t;
static struct NetManBD status[64], iop_status[64];
static struct NetManBD *FrameBufferStatus=status, *RxIOPFrameBufferStatus=iop_status;
static u8 bounce[64*1536], *RxDmaFrames=bounce;
static void *RxPacketPayload[64];
static unsigned short RxBufferRdPtr, RxBufferNextRdPtr;
static unsigned char IsProcessingRx;
static u8 old_payload[1600], replacement[1600], expected[1514];
static int packet, next_packet, rearmed, delivered, freed, test_length;
static jmp_buf done;
static int sceSifSetDma(SifDmaTransfer_t *d, int n) {
    assert(n==1 && d->size==sizeof(struct NetManBD));
    memcpy(d->dest,d->src,d->size);
    /* Simulate a very fast next packet after length=0 is published. */
    memset(((struct NetManBD *)d->src)->payload,0xE7,1536);
    rearmed++;
    return 1;
}
static void SleepThread(void) { assert(freed==1); longjmp(done,1); }
static void *NetManNetProtStackAllocRxPacket(unsigned n, void **payload) {
    assert(n==1514); *payload=replacement; return &next_packet;
}
static void NetManNetProtStackReallocRxPacket(void *p,unsigned n) {
    assert(p==&packet && n==(unsigned)test_length);
}
static void NetManNetProtStackFreeRxPacket(void *p) { assert(p==&packet); freed++; }
static void NetManNetProtStackEnQRxPacket(void *p) {
    assert(p==&packet && rearmed==1);
    assert(!memcmp(old_payload,expected,(size_t)test_length));
    assert(old_payload[test_length]==0xA5); /* exact-length copy */
    delivered++;
    longjmp(done,1);
}
'''

MAIN = r'''
int main(void) {
    static const int lengths[]={1,63,64,65,511,512,513,1514,1515};
    unsigned j,k;
    for(j=0;j<sizeof(lengths)/sizeof(lengths[0]);j++) {
        unsigned slot=(j&1)?63:0;
        memset(status,0,sizeof(status));
        memset(old_payload,0xA5,sizeof(old_payload));
        for(k=0;k<1514;k++) expected[k]=(u8)(k*17+3);
        memcpy(bounce+slot*1536,expected,1514);
        test_length=lengths[j];
        rearmed=delivered=freed=0;
        RxBufferRdPtr=RxBufferNextRdPtr=slot;
        status[slot].packet=&packet;
        status[slot].payload=bounce+slot*1536;
        status[slot].length=(unsigned short)test_length;
        RxPacketPayload[slot]=old_payload;
        if(!setjmp(done)) NETMAN_RxThread(NULL);
        assert(RxBufferRdPtr==(slot+1)%64);
        assert(RxPacketPayload[slot]==replacement);
        assert(iop_status[slot].payload==bounce+slot*1536);
        if(test_length<=1514) assert(delivered==1 && freed==0);
        else {
            assert(delivered==0 && freed==1);
            for(k=0;k<sizeof(old_payload);k++) assert(old_payload[k]==0xA5);
        }
    }
    puts("NETMAN RX: exact copy, rearm race, ring wrap, oversize PASS");
    return 0;
}
'''


class PatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sdk = Path(os.environ["PS2SDK_SOURCE"])

    def test_real_patched_receive(self):
        files = patch_sdk.patched_files(self.sdk)
        source = files[self.sdk / "ee/network/netman/src/rpc_server.c"]
        c = HARNESS + function(source, "static void ClearBufferLen(int index, void *packet, void *payload)")
        c += function(source, "static void NETMAN_RxThread(void *arg)") + MAIN
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"rx.c"
            path.write_text(c)
            exe = str(Path(tmp)/"rx")
            subprocess.run(["cc","-std=c99","-Wall","-Wextra","-Werror",str(path),"-o",exe],check=True)
            subprocess.run([exe],check=True)

    def test_fail_closed_and_optional_pio(self):
        normal = patch_sdk.patched_files(self.sdk)
        pio = patch_sdk.patched_files(self.sdk, True)
        self.assertEqual(len(normal),2)
        self.assertEqual(len(pio),3)
        with self.assertRaises(ValueError):
            patch_sdk.once("already changed", "original anchor", "new")
        opts = normal[self.sdk/"ee/network/tcpip/src/include/lwipopts.h"]
        self.assertRegex(opts,r"#define CHECKSUM_CHECK_TCP\s+1")
        self.assertRegex(opts,r"#define CHECKSUM_CHECK_IP\s+1")


if __name__ == "__main__":
    unittest.main()
