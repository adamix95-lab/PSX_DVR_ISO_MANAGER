#!/usr/bin/env python3
"""Apply the branch-local receive repair to one exact PS2SDK checkout.

No files in an installed SDK or in the known-good USB distribution are edited.
The same patched checkout builds both EE libraries and IOP modules.
"""
import argparse
from pathlib import Path
import re
import subprocess

SDK_COMMIT = "d41ea62946e53d9a0c8788bb9eb8c8faf488b559"
DEV9_SHA256 = "5cdd4539464a2fe92d95d93692239c2bc80b9a99f0d7644d18ae66b756c68201"


def once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"SDK patch anchor changed or already patched: {old[:80]!r}")
    return text.replace(old, new, 1)


def patched_files(root, rx_pio=False):
    """Validate every anchor before the caller writes any file."""
    net_path = root / "ee/network/netman/src/rpc_server.c"
    net = net_path.read_text()
    net = once(net, "static struct NetManBD *RxIOPFrameBufferStatus;", """static struct NetManBD *RxIOPFrameBufferStatus;
/* NETFIX: DMA never targets a cached lwIP pbuf. Each ring slot owns a
   separate 64-byte-aligned bounce frame, read only through its uncached
   alias. pbuf metadata/payload cache lines are entirely CPU-owned. */
static u8 *RxDmaFrames;
static void *RxPacketPayload[NETMAN_RPC_BLOCK_SIZE];""")
    net = once(net, "\tbd->payload = payload;", """\tRxPacketPayload[index] = payload;
\tbd->payload = &RxDmaFrames[index * NETMAN_MAX_FRAME_SIZE];""")
    net = once(net, "\n\t\t\tif(FrameBufferStatus != NULL)\n\t\t\t{\n\t\t\t\tmemset", """\t\t\tif(RxDmaFrames == NULL)
\t\t\t{
\t\t\t\tRxDmaFrames = memalign(64, NETMAN_RPC_BLOCK_SIZE * NETMAN_MAX_FRAME_SIZE);
\t\t\t\tif(RxDmaFrames != NULL)
\t\t\t\t\tsceSifWriteBackDCache(RxDmaFrames, NETMAN_RPC_BLOCK_SIZE * NETMAN_MAX_FRAME_SIZE);
\t\t\t}

\t\t\tif(FrameBufferStatus != NULL && RxDmaFrames != NULL)
\t\t\t{
\t\t\t\tmemset""")
    net = once(net, "\t\tcase NETMAN_EE_RPC_FUNC_DEINIT:\n", """\t\tcase NETMAN_EE_RPC_FUNC_DEINIT:
\t\t\tif(RxDmaFrames != NULL)
\t\t\t{
\t\t\t\tfree(RxDmaFrames);
\t\t\t\tRxDmaFrames = NULL;
\t\t\t}
""")
    net = once(net, "u32 PacketLength, PacketLengthAligned;", "u32 PacketLength;")
    net = once(net, "\t\tpayload = bd->payload;\n\t\tpacket = bd->packet;", """\t\tpayload = RxPacketPayload[RxBufferRdPtr];
\t\tpacket = bd->packet;

\t\t/* Copy BEFORE rearming this slot: IOP can overwrite the bounce
\t\t   frame as soon as ClearBufferLen() publishes length=0. */
\t\tif(PacketLength <= NETMAN_NETIF_FRAME_SIZE)
\t\t\tmemcpy(payload, UNCACHED_SEG(bd->payload), PacketLength);""")
    net = once(net, """\t\t//Now process the received packet.
\t\tPacketLengthAligned = (PacketLength + 63) & ~63;
\t\tsceSifWriteBackDCache(payload, PacketLengthAligned);
\t\tNetManNetProtStackReallocRxPacket(packet, PacketLength);
\t\tNetManNetProtStackEnQRxPacket(packet);""", """\t\t/* No writeback/invalidate after DMA: the lwIP pbuf was filled
\t\t   by CPU memcpy, so its dirty cache lines contain the new data. */
\t\tif(PacketLength <= NETMAN_NETIF_FRAME_SIZE)
\t\t{
\t\t\tNetManNetProtStackReallocRxPacket(packet, PacketLength);
\t\t\tNetManNetProtStackEnQRxPacket(packet);
\t\t}
\t\telse
\t\t\tNetManNetProtStackFreeRxPacket(packet);""")

    opts_path = root / "ee/network/tcpip/src/include/lwipopts.h"
    opts = opts_path.read_text()
    for name in ("CHECKSUM_CHECK_IP", "CHECKSUM_CHECK_TCP"):
        opts, count = re.subn(r"(#define " + name + r"\s+)0\b", r"\g<1>1", opts)
        if count != 1:
            raise ValueError(f"Unexpected checksum definition: {name}")
    opts = once(opts, "//Rely on the Ethernet checksums to reduce the number of checksum compuations. If you require any of these, re-enable them.",
                "// NETFIX: validate IP/TCP after the IOP->EE copy; Ethernet FCS\n// cannot protect transfers inside the console. Keep PNG CRC too.")
    result = {net_path: net, opts_path: opts}
    if rx_pio:
        # Fallback only: do not combine the SMAP DMA hypothesis with the
        # default repair until the normal DMA build has been evaluated.
        xfer_path = root / "iop/network/smap/src/xfer.c"
        xfer = once(xfer_path.read_text(),
                    "result = SmapDmaTransfer(smap_regbase, buffer, length, DMAC_TO_MEM);",
                    "result = 0; /* NETFIX fallback: RX FIFO uses PIO; TX DMA unchanged. */")
        result[xfer_path] = xfer
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sdk", type=Path)
    parser.add_argument("--rx-pio", action="store_true")
    args = parser.parse_args()
    actual = subprocess.check_output(["git", "-C", str(args.sdk), "rev-parse", "HEAD"], text=True).strip()
    if actual != SDK_COMMIT:
        raise SystemExit(f"Wrong SDK commit: {actual}; expected {SDK_COMMIT}")
    files = patched_files(args.sdk, args.rx_pio)
    for path, content in files.items():
        path.write_text(content)
    print(f"Patched SDK {actual}: uncached RX bounce + IP/TCP checksums; RX PIO={args.rx_pio}")


if __name__ == "__main__":
    main()
