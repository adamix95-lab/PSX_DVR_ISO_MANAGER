"""Record the EE link layout and check what the N6G hardware tests isolate."""

import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys


def ee(tool, *args):
    command = Path(os.environ["PS2DEV"]) / "ee/bin" / f"mips64r5900el-ps2-elf-{tool}"
    return subprocess.check_output([command, *args], text=True)


def audit(elf, out):
    stem = elf.stem
    sections = ee("readelf", "-h", "-l", "-S", "-W", elf)
    sizes = ee("size", "-A", "-x", elf)
    ordered = ee("nm", "-S", "--size-sort", "--defined-only", elf)
    symbols = {}
    for line in ee("nm", "-P", "-S", "--defined-only", elf).splitlines():
        fields = line.split()
        if len(fields) < 3:
            continue  # unnamed local ELF symbols
        name, _, addr, *size = fields
        symbols[name] = (int(addr, 16), int(size[0], 16) if size else 0)
    (out / f"{stem}.readelf.txt").write_text(sections, encoding="utf-8")
    (out / f"{stem}.size.txt").write_text(sizes, encoding="utf-8")
    (out / f"{stem}.nm.txt").write_text(ordered, encoding="utf-8")
    shutil.copyfile(elf.with_suffix(".map"), out / f"{stem}.map")

    target = elf.parent / "CMakeFiles" / f"{elf.name}.dir"
    stack_usage = []
    for path in sorted(target.rglob("*.su")):
        text = path.read_text(encoding="utf-8")
        if text:
            stack_usage.append(f"{path.relative_to(target)}\n{text}")
    assert stack_usage, elf
    pdr = re.search(r"\.pdr\s+PROGBITS\s+\w+\s+(\w+)\s+(\w+)", sections)
    assert pdr, elf
    offset, length = (int(value, 16) for value in pdr.groups())
    frames = {record[0]: record[5] for record in
              struct.iter_unpack("<8I", elf.read_bytes()[offset:offset + length])}
    selected = ("main", "tls_one_bound", "tls_send", "tls_recv", "verify_two_bound_tls",
                "wolfSSL_connect", "ProcessPeerCerts", "fp_exptmod")
    stack_usage.append("MIPS .pdr (statyczne ramki, bajty)\n" + "\n".join(
        f"{name}\t{frames.get(symbols[name][0], 'brak')}" for name in selected
    ))
    (out / f"{stem}.stack.txt").write_text("\n".join(stack_usage), encoding="utf-8")

    assert "ELF32" in sections and "MIPS" in sections and "EXEC" in sections, elf
    assert symbols["_stack_size"][0] == 128 * 1024, elf
    assert symbols["_heap_size"][0] == 0xFFFFFFFF, elf
    assert symbols["n6_response"][1] == 270337, elf
    assert symbols["ca_bundle"][1] == 350001, elf
    for symbol in ("wolfSSL_connect", "wolfSSL_check_domain_name", "wolfSSL_UseSNI",
                   "wc_SetSeed_Cb", "v12_n3e_txryan_strict"):
        assert symbol in symbols, (elf, symbol)
    sec = dict((name, int(size, 16)) for name, size in
               re.findall(r"^(\.\w+)\s+(0x[\da-f]+)\s+", sizes, re.M))
    return sec, symbols


def main():
    baseline, build, out = map(Path, sys.argv[1:])
    out.mkdir(parents=True, exist_ok=True)
    elfs = {
        "N6E": baseline / "V12_N6E_HTTPS_COVER_RAM_RO.ELF",
        "N6F": baseline / "V12_N6F_HTTPS_USB_ART_ONE.ELF",
        "N6G-A": build / "V12_N6G_A.ELF",
        "N6G-B": build / "V12_N6G_B.ELF",
        "N6G-TRACE": build / "V12_N6G_TRACE.ELF",
        "N6G FINAL": build / "V12_N6G_FINAL.ELF",
    }
    results = {tag: audit(elf, out) for tag, elf in elfs.items()}
    e, f, a, b, trace, final = [symbols for _, symbols in results.values()]
    buffers = {"scan_row": 2048, "image_rgb": 84000, "png_output": 262144}
    assert all(f[name][1] == b[name][1] == count for name, count in buffers.items())
    assert a["n6g_bss_padding"][1] == sum(buffers.values()) + 1024
    for symbols in (e, a, b, trace):
        assert "n6_write_usb_art" not in symbols
    assert "n6_normalize_png" not in a and "png_read_info" not in a
    for symbol in ("n6_normalize_png", "png_read_info", "inflate", "deflate"):
        assert symbol in b and symbol in trace, symbol
    assert "n6_normalize_png_into" in final and "n6_write_usb_art" in final
    assert not (set(buffers) & final.keys()) and "verify_chunk" not in final
    assert b["bdmfs_fatfs_irx"][1] == e["bdmfs_fatfs_irx"][1]
    for symbols in (e, f, a, b, trace, final):
        assert symbols["dev9_ns_irx"][1] == 11273 and "ps2dev9_irx" not in symbols
    for tag, marker in (("N6G-A", b"N6G-A STATIC RAM"),
                        ("N6G-B", b"N6G-B PNG LINK ONLY"),
                        ("N6G-TRACE", b"LOW CLEAN 6: return LOW"),
                        ("N6G FINAL", b"N6G HTTPS USB ART - FINAL RESULT")):
        assert marker in elfs[tag].read_bytes(), tag

    lines = [
        "# Układ pamięci EE z kompilacji GitHub Actions", "",
        "| ELF | Plik [B] | .text [B] | .data [B] | .rodata [B] | .bss [B] | _end |",
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    for tag, (sec, symbols) in results.items():
        parts = " | ".join(str(sec[name]) for name in (".text", ".data", ".rodata", ".bss"))
        lines.append(f"| {tag} | {elfs[tag].stat().st_size} | {parts} | 0x{symbols['_end'][0]:08x} |")
    lines += [
        "", "Każdy ELF deklaruje stos główny 131072 B i automatyczny limit sterty.",
        "Rozruch PS2SDK przekazuje adres `_end` do `SetupHeap`.",
        "Pliki `.stack.txt` pokazują statyczne ramki funkcji aplikacji, bez pomiaru",
        "najgłębszego zagnieżdżenia wolfSSL ani dynamicznego maksimum sterty.", "",
        f"Finalny N6G zmniejsza .bss względem N6F o "
        f"{results['N6F'][0]['.bss'] - results['N6G FINAL'][0]['.bss']} B.",
        "Robocze 86048 B normalizacji są przydzielane dopiero po zwolnieniu TLS.",
        "Rozmiary bibliotek roboczych i modułów NETFIX dokumentują dołączone sumy.", "",
    ]
    (out / "EE-memory.md").write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
