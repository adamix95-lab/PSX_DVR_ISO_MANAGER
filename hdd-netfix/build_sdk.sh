#!/bin/sh
set -eu
sdk_source=$(cd "${1:?Usage: build_sdk.sh SDK_SOURCE}" && pwd)
sdk_commit=d41ea62946e53d9a0c8788bb9eb8c8faf488b559
test "$(git -C "$sdk_source" rev-parse HEAD)" = "$sdk_commit"
echo '5cdd4539464a2fe92d95d93692239c2bc80b9a99f0d7644d18ae66b756c68201  dvr-iso-manager/modules/dev9_ns.irx' | sha256sum -c -
PS2SDK_SOURCE="$sdk_source" python3 -m unittest discover -s hdd-netfix -p 'test_*.py' -v
RX_PIO=${RX_PIO:-false}
if [ "$RX_PIO" = true ]; then
  python3 hdd-netfix/patch_sdk.py "$sdk_source" --rx-pio
else
  python3 hdd-netfix/patch_sdk.py "$sdk_source"
fi
export PS2SDKSRC="$sdk_source"
make -C "$sdk_source" -j2 install
{
  echo "Manager commit: ${GITHUB_SHA:-$(git rev-parse HEAD)}"
  echo "SDK commit: $sdk_commit"
  echo "SMAP RX PIO fallback: $RX_PIO"
  echo 'NETMAN RX: isolated uncached bounce frames; IP/TCP checksums: enabled'
  echo 'DEV9: exact USB dev9_ns, single loaded provider'
  mips64r5900el-ps2-elf-gcc --version
  sha256sum "$PS2SDK/ee/lib/libps2ip.a" "$PS2SDK/ee/lib/libnetman.a"
  sha256sum "$PS2SDK/iop/irx/netman.irx" "$PS2SDK/iop/irx/smap.irx"
  sha256sum "$PS2SDK/iop/irx/ps2atad.irx" "$PS2SDK/iop/irx/ps2hdd.irx" "$PS2SDK/iop/irx/ps2fs.irx"
} > "$PS2SDK/HDD_NETFIX_BUILD.txt"
