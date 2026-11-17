"""Keep the DESR-tested CPU R1 worker out of the Manager's loadable segment."""

import argparse
import hashlib
import struct
from pathlib import Path
from zipfile import ZipFile

from audit_usb12_payload import loadable_segment


MANAGER_PATH = "DESR_DVR/DVR_ISO_MANAGER_V12_CPU.ELF"
R1_MANAGER_SHA = "b8cf642fb9a284b86813791f3abb9178f5e21ec94fe2d29bbe5919171a182e21"
R1_WORKER_SHA = "23654c1755c1b79c3d30af32580f823ba451992dc3fcd654ce7bcfc234d7b489"


def require_sha256(data, expected):
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise SystemExit(f"Expected SHA-256 {expected}, got {actual}")


def unpack_r1(reference, destination):
    with ZipFile(reference) as package:
        manager = package.read(MANAGER_PATH)
    require_sha256(manager, R1_MANAGER_SHA)
    metadata, segment = loadable_segment(manager)
    offset = 0x001D4060 - metadata[1]
    worker = segment[offset:offset + 2706688]
    require_sha256(worker, R1_WORKER_SHA)
    destination.write_bytes(worker)


def append_worker(manager_path, worker_path, *, ui_preview=False):
    manager = manager_path.read_bytes()
    worker = worker_path.read_bytes()
    worker_sha = hashlib.sha256(worker).hexdigest()
    if ui_preview:
        if worker_sha == R1_WORKER_SHA:
            raise SystemExit("UI preview unexpectedly contains the OLD hardware R4 worker")
        for marker in (
            b"TRWA POBIERANIE BRAKUJACYCH OKLADEK",
            b"POBIERANIE ZAKONCZONE",
            b"POMINIETO",
        ):
            if marker not in worker:
                raise SystemExit(f"UI worker missing {marker!r}")
    else:
        # Preserve the original R4 reproducible packaging mode untouched.
        require_sha256(worker, R1_WORKER_SHA)
    loadable_segment(worker)
    metadata, segment = loadable_segment(manager)
    if len(segment) >= 2 * 1024 * 1024:
        raise SystemExit("The Manager's loadable segment should exclude the HTTPS worker")
    bytes_on_usb = manager + worker + struct.pack(
        "<8sII", b"N6GCPUR3", len(worker), len(worker) ^ 0xFFFFFFFF
    )
    assert loadable_segment(bytes_on_usb) == (metadata, segment)
    manager_path.write_bytes(bytes_on_usb)
    print(f"Manager loadable segment: {len(segment)} bytes; "
          f"appended {'UI PREVIEW' if ui_preview else 'exact R1'} worker: "
          f"{len(worker)} bytes, sha256={worker_sha}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="action", required=True)
    extract = commands.add_parser("extract")
    extract.add_argument("reference", type=Path)
    extract.add_argument("worker", type=Path)
    pack = commands.add_parser("pack")
    pack.add_argument("manager", type=Path)
    pack.add_argument("worker", type=Path)
    ui = commands.add_parser("pack-ui", help="Append newly built UI worker (never recover old R4)")
    ui.add_argument("manager", type=Path)
    ui.add_argument("worker", type=Path)
    args = parser.parse_args()
    if args.action == "extract":
        unpack_r1(args.reference, args.worker)
    elif args.action == "pack-ui":
        append_worker(args.manager, args.worker, ui_preview=True)
    else:
        append_worker(args.manager, args.worker)


if __name__ == "__main__":
    main()
