# Integrated LAN artwork module

This directory contains the HTTP/PNG and Game ID artwork sync code used by **PSX DVR ISO Manager v0.8**.

These files originated from the early standalone `dvr-net-probe` hardware experiments, but are now compiled directly into `DVR_ISO_MANAGER.ELF`:

- `probe_config.c`: strict static IPv4 and `NET_PROBE.CNF` parser
- `probe_png.c`: bounded HTTP response parser and PNG CRC validation
- `art_sync.c`: missing-artwork Game ID extraction and safe request paths
- `art_sync_runtime.c`: LAN HTTP download, USB `.PART` validation and safe publishing of **new** covers

Public headers live in `dvr-iso-manager/include/`. Portable tests live in `dvr-iso-manager/tests/net/` and run in the Manager's GitHub Actions workflow.

**No standalone Probe ELF is distributed or built.** The separate network test project was retired after ART SYNC became part of Manager v0.8.

## Safety and compatibility

Keep the static network initialization, HTTP bounds checks, PNG CRC validation and staged USB writes intact. Do not write to the DVR/ISO partition or overwrite existing artwork. The GitHub v0.8 release binaries were hardware-validated and remain unchanged by this source tree reorganization.
