# Skróty gier w Sony PSX XMB — STAGE 1: test PP.ICO

**STAGE 0 PASS:** oba launchery SYSTEM i ENGLISH uruchomiły `ICO.iso` na prawdziwym DESR-7000, bez otwierania PDIM. **STAGE 1 czeka na test z partycji PP.ICO.**

Cel: każdy skrót XMB ma swoją partycję, `BOOT.CNF` i grafikę. Neutrino FIX95, `config/` oraz `modules/` nadal pozostają WYŁĄCZNIE na działającej partycji `hdd0:PP.DVRISO`; obrazy gier nie są kopiowane.

## Sprawdzany łańcuch

```text
mass0:/.../ICOPFS.ELF + ICOPFS.CNF
 -> hdd0:PP.ICO:pfs:/BOOT.ELF (arg=hdd0:PP.ICO/BOOT.CNF)
 -> hdd0:PP.DVRISO:pfs:/neutrino_hdd.elf
 -> dvr_pfs0:/ISO/ICO.iso
```

`BOOT_SYSTEM.CNF` i `BOOT_ENGLISH.CNF` wybierają język gry. Plik `EXECUTE.CNF` przeznaczony jest na przyszły test XMB (KELF do PP.ICO/BOOT.ELF).

## Ważne: ryzyko APA

**Utworzenie nowej partycji może uszkodzić układ APA i dane systemowe Sony PSX.** Przed testem wykonaj i zweryfikuj **pełną, odtwarzalną kopię sektorową całego HDD**. Jeśli nie masz kopii, nie twórz partycji. Nie formatuj, nie kasuj i nie zmieniaj rozmiaru istniejących partycji. Jeśli `PP.ICO` już istnieje, przerwij.

Po kopii sektorowej można ręcznie utworzyć **nową** `PP.ICO` w wolnym miejscu (np. 128 MiB, jeśli wLE-R3Z obsługuje taką wielkość), skopiować zawartość katalogu `PP.ICO-FILES/` z osobnej paczki Stage 1 do głównego katalogu `PP.ICO`, a następnie uruchomić w wLE-R3Z `USB_TEST/ICOPFS.ELF` z pendrive'a. **Nie używaj Inject Header**. To NIE jest instalator do XMB.

Okładka i screenshoty w paczce testowej są grafikami demonstracyjnymi, a metadane `SCES-50760` dotyczą przykładowego wydania PAL ICO (Twoje ISO może być innego regionu).

Po PASS z `PP.ICO` przejdziemy do kolejnego etapu: pojedynczy wpis w XMB, dopiero po oddzielnej weryfikacji bezpieczeństwa.
