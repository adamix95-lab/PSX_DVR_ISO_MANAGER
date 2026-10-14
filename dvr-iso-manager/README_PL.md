# DVR ISO Manager v0.2 — OSD Language (Sony PSX DESR-7000)

**EKSPERYMENT: nie scalaj do main do czasu testu na prawdziwym PSX.**
v0.1.5 jest już przetestowany na DESR-7000 (Metal Slug Anthology startuje bezpośrednio X).
Ta gałąź dodaje TYLKO możliwość wyboru języka OSD. Nadal startuje przez ten sam OSDMenu BOOT.ELF.

## Obsługa

- GÓRA/DÓŁ: wybierz grę z __xcontents/ISO.
- SELECT: język gry SYSTEM / ENGLISH [experimental]. Domyślnie SYSTEM.
- KWADRAT: generuj i zweryfikuj USB DVR_TEST.CNF, BOOT.CNF bez zmian.
- X: skopiuj istniejący BOOT.CNF do BOOT_RECOVERY.CNF (tylko pierwsze uruchomienie),
  zapisz/zweryfikuj nowy BOOT.CNF i uruchom sprawdzony USB OSDMenu BOOT.ELF.
- TRÓJKĄT: ponowne skanowanie; START: wyjście.

## Co się zmienia w trybie ENGLISH?

Manager generuje BOOT.CNF z osobnym neutrino_lang.elf i argumentami:

    boot=neutrino_lang.elf
    arg=-bsd=dvr
    arg=-dvd=dvr_pfs0:/ISO/REZ.iso
    arg=-cfg=osdlang
    arg=-osdlang=en

Parametr -cfg=osdlang wskazuje na config/osdlang.toml, w którym eecore.elf
jest ustawiony na ee_core_lang.elf. Dzięki temu gra otrzymuje angielski OSD
przez hooki Neutrino, bez zmiany japońskiego systemu XMB.

## Wymagane nowe pliki z OSOBNEGO artefaktu Neutrino v0.2

    mass0:/DESR_DVR/neutrino_lang.elf
    mass0:/DESR_DVR/modules/ee_core_lang.elf
    mass0:/DESR_DVR/config/osdlang.toml

Stare pliki MUSZĄ pozostać:

    mass0:/DESR_DVR/neutrino.elf       (sprawdzony Neutrino DVR PR228)
    mass0:/DESR_DVR/modules/ee_core.elf
    mass0:/DESR_DVR/config/system.toml
    mass0:/DESR_DVR/BOOT.ELF           (sprawdzony OSDMenu Launcher)
    mass0:/DESR_DVR/BOOT.CNF           (kopia na PC zalecana)

Tryb SYSTEM generuje dokładnie ten sam CNF co v0.1.5 (bez nowych argumentów).
Manager nie zapisuje do DVR ani APA; na USB zapisuje tylko pliki CNF.

## Plan pierwszego testu

1. Zrób kopię działającego BOOT.CNF na komputerze.
2. Dodaj trzy nowe pliki z Neutrino v0.2 i podmień tylko Manager ELF.
3. Najpierw uruchom REZ albo Metal Slug w domyślnym trybie SYSTEM.
4. Wróć do Managera, naciśnij SELECT i potwierdź napis ENGLISH.
5. Wybierz REZ.iso, naciśnij KWADRAT, obejrzyj DVR_TEST.CNF.
6. Naciśnij X; sfotografuj rezultat (menu gry lub komunikat).
7. W razie niepowodzenia uruchom ponownie konsolę i wybierz SYSTEM.

ENGLISH nie jest automatycznym tłumaczeniem gry. Potrzebne są angielskie zasoby ISO.
Nie modyfikuj XMB i nie przeinstalowuj Neutrino.

Więcej: neutrino-v02/README_PL.md w repozytorium.
