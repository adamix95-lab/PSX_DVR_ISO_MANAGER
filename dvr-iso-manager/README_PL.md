# DVR ISO Manager v0.2 — OSD Language (Sony PSX DESR-7000)

**Stan: potwierdzony start i przełączanie języka na fizycznym Sony PSX DESR-7000.** Obsługa angielskiego zależy od wersji i zasobów konkretnej gry.
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

## Wyniki testów sprzętowych v0.2 (2026-10-15)

- Metal Slug Anthology: tryb SYSTEM = japoński; ENGLISH = angielski. Potwierdzony start gry przez Quickboot na DESR-7000.
- REZ: uruchomiony w trybie ENGLISH; angielskie komunikaty dotyczące autosave i wczytania zapisu potwierdzone zdjęciami.
- Jeden z przycisków OK na zdjęciu REZ nadal wyświetla japoński napis — część zasobów gry może być niezależna od ustawień OSD.
- Manager ELF można uruchamiać z serwera UDPFS przez wLE-R3Z; zapis BOOT.CNF i start OSDMenu BOOT.ELF pozostają na USB (mass0:/DESR_DVR/).
- Zmienione jest wyłącznie to, co gra otrzymuje z konfiguracji OSD; japońskiego menu systemowego XMB nie zmieniano.
- Pozostałe tytuły i pełne przejście gameplayu nie zostały zweryfikowane.

## Instrukcja odtworzenia testu

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
