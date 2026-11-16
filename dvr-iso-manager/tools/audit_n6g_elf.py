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
        "N6G-FREETRACE": build / "V12_N6G_FREETRACE.ELF",
        "N6G FINAL": build / "V12_N6G_FINAL.ELF",
        "N6G SCREEN": build / "V12_N6G_SCREEN.ELF",
        "N6G HEADLESS": build / "V12_N6G_HEADLESS.ELF",
        "N6G ARENA": build / "V12_N6G_ARENA.ELF",
        "N6G BATCH50": build / "V12_N6G_BATCH50.ELF",
    }
    results = {tag: audit(elf, out) for tag, elf in elfs.items()}
    e, f, a, b, trace, free_trace, final, screen, headless, arena, batch = [
        symbols for _, symbols in results.values()
    ]
    buffers = {"scan_row": 2048, "image_rgb": 84000, "png_output": 262144}
    assert all(f[name][1] == b[name][1] == count for name, count in buffers.items())
    assert a["n6g_bss_padding"][1] == sum(buffers.values()) + 1024
    for symbols in (e, a, b, trace, free_trace):
        assert "n6_write_usb_art" not in symbols
    assert "n6_normalize_png" not in a and "png_read_info" not in a
    for symbol in ("n6_normalize_png", "png_read_info", "inflate", "deflate"):
        assert all(symbol in symbols for symbols in (b, trace, free_trace)), symbol
    assert "n6g_free_trace" in free_trace and "n6g_hash_checkpoint" in free_trace
    for symbols in (e, f, a, b, trace, final, screen, headless, arena, batch):
        assert "n6g_free_trace" not in symbols
    for symbols in (final, screen, headless, arena, batch):
        assert "n6_normalize_png_into" in symbols and "n6_write_usb_art" in symbols
        assert not (set(buffers) & symbols.keys()) and "verify_chunk" not in symbols
    for symbols in (screen, headless):
        assert "__wrap_scr_printf" in symbols and "__wrap_scr_clear" in symbols
    assert "__wrap_scr_printf" not in final
    for symbols in (arena, batch):
        assert "n6g_tls_arena_begin" in symbols and "wolfSSL_SetAllocators" in symbols
    assert batch["n6_targets"][1] == 50 * 12 and batch["batch_entropy"][1] == 16000
    assert "n6g_batch_entropy_valid" in batch
    assert "n6g_tls_arena_begin" not in final
    assert b["bdmfs_fatfs_irx"][1] == e["bdmfs_fatfs_irx"][1]
    for symbols in (e, f, a, b, trace, free_trace, final, screen, headless, arena, batch):
        assert symbols["dev9_ns_irx"][1] == 11273 and "ps2dev9_irx" not in symbols
    for tag, marker in (("N6G-A", b"N6G-A STATIC RAM"),
                        ("N6G-B", b"N6G-B PNG LINK ONLY"),
                        ("N6G-TRACE", b"LOW CLEAN 6: return LOW"),
                        ("N6G-FREETRACE", b"19 CTX reference decrement"),
                        ("N6G SCREEN", b"N6G SCREEN TEST: %s"),
                        ("N6G HEADLESS", b"N6G HEADLESS - DISPLAY STARTED AT SUMMARY"),
                        ("N6G ARENA", b"mass0:/N6G.NEWLIB"),
                        ("N6G BATCH50", b"mass0:/N6G50.ENT"),
                        ("N6G FINAL", b"N6G HTTPS USB ART - FINAL RESULT")):
        assert marker in elfs[tag].read_bytes(), tag
    assert b"mass0:/N6G.SCREEN" not in elfs["N6G HEADLESS"].read_bytes()
    assert b"mass0:/N6G.NEWLIB" not in elfs["N6G BATCH50"].read_bytes()
    trace_bytes = elfs["N6G-FREETRACE"].read_bytes()
    for marker in (b"LOW HASH prior:", b"07a hashes empty", b"07f release hash block",
                   b"07g hash block released", b"07h hash cleanup finished"):
        assert marker in trace_bytes, marker

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
