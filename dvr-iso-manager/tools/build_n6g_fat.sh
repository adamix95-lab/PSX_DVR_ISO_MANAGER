#!/bin/sh
# Rebuild only the N6G FAT IRX. The installed NETFIX SDK stays unchanged.
set -eu
sdk=$(cd "${1:?Usage: build_n6g_fat.sh SDK_SOURCE OUTPUT_IRX}" && pwd)
output=${2:?Set the output IRX path}
driver="$sdk/iop/fs/bdmfs_fatfs"

python3 - "$driver/src/fs_driver.c" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
source = path.read_text()
original = """    if (flags & O_CREAT)
        f_mode |= FA_OPEN_ALWAYS;
    if (flags & O_TRUNC)
        f_mode |= FA_CREATE_ALWAYS;
    if (flags & O_APPEND)
        f_mode |= FA_OPEN_APPEND;
"""
exclusive = """    if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
        f_mode |= FA_CREATE_NEW;
    } else {
        if (flags & O_CREAT)
            f_mode |= FA_OPEN_ALWAYS;
        if (flags & O_TRUNC)
            f_mode |= FA_CREATE_ALWAYS;
        if (flags & O_APPEND)
            f_mode |= FA_OPEN_APPEND;
    }
"""
if exclusive not in source:
    if source.count(original) != 1:
        raise SystemExit("Unexpected PS2SDK FAT open flags")
    path.write_text(source.replace(original, exclusive))
PY

export PS2SDKSRC="$sdk"
make -C "$driver" -j2
module=$(find "$driver" -name bdmfs_fatfs.irx -type f)
test -f "$module"
cp "$module" "$output"
test -s "$output"
