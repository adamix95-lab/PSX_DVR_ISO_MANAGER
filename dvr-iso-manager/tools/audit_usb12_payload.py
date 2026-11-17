"""Check the ELF layout required by the PS2SDK embedded worker handoff."""

import struct
import sys
from pathlib import Path


def loadable_segment(image):
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", image)
    ident, file_type, machine, _, entry, phoff, _, _, ehsize, phsize, phnum, *_ = header
    assert ident[:6] == b"\x7fELF\x01\x01" and (file_type, machine) == (2, 8)
    assert ehsize == 52 and phsize == 32 and 0 < phnum <= 32
    assert phoff >= ehsize and phoff % 4 == 0 and phoff + phnum * phsize <= len(image)

    headers = [struct.unpack_from("<8I", image, phoff + phsize * i) for i in range(phnum)]
    [(kind, offset, vaddr, _, filesz, memsz, flags, align)] = [h for h in headers if h[0] == 1]
    assert vaddr == 0x00100000 and vaddr + memsz <= 32 * 1024 * 1024
    assert 0 < filesz <= memsz and offset + filesz <= len(image)
    assert vaddr <= entry < vaddr + filesz
    return (entry, vaddr, memsz, flags, align), image[offset:offset + filesz]


def main():
    original, worker, manager = (Path(name).read_bytes() for name in sys.argv[1:])
    assert loadable_segment(original) == loadable_segment(worker), "Worker load image changed"
    (entry, vaddr, memsz, _, _), contents = loadable_segment(manager)
    offset = contents.find(worker)
    assert offset > 0 and (vaddr + offset) % 16 == 0, "Payload is missing or misaligned"
    print(f"Embedded worker at {vaddr + offset:#010x}, {len(worker)} bytes")
    print(f"Manager entry {entry:#010x}, last EE byte {vaddr + memsz - 1:#010x}")


if __name__ == "__main__":
    main()
