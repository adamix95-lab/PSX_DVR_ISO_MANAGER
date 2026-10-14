# Neutrino PR #228 — OSD English v0.2 (wersja eksperymentalna)

**Stan: potwierdzono działanie na fizycznym DESR-7000 z grami Metal Slug Anthology i REZ.** Wymuszanie języka nie gwarantuje angielskiego dla każdej gry.
Patch nakładany na dokładnie: aa8d80a1a0b5323cb407c814c0568c3489597c1c (Neutrino PR #228).

## Mechanizm

Nowa opcja uruchamiania Neutrino: -osdlang=en (wymuś angielski), -osdlang=system (domyślnie).
Przechwytywane są dwa wywołania systemowe: GetOsdConfigParam i SetOsdConfigParam,
tak jak w Open PS2 Loader. Gra widzi language=1 oraz japLanguage=1.
Nie zapisujemy niczego do NVRAM, nie zmieniamy języka XMB ani zawartości DVR.
Wersja testowa nie tłumaczy tekstów gry, jeżeli obraz ISO nie ma angielskich zasobów.

## Instalacja bez naruszania dotychczas działającej wersji

Artefakt workflow OSD Language zawiera dokładnie TRZY NOWE PLIKI:

    mass0:/DESR_DVR/neutrino_lang.elf
    mass0:/DESR_DVR/modules/ee_core_lang.elf
    mass0:/DESR_DVR/config/osdlang.toml

Nie nadpisuj starych neutrino.elf, modules/ee_core.elf ani config/system.toml.
Nie zastępuj katalogów modules/ lub config/ całymi katalogami. Dodaj tylko wskazane pliki.
Osobno zainstaluj v0.2 DVR_ISO_MANAGER.ELF z workflow Build DVR ISO Manager v0.2.
Zachowaj kopię aktualnego BOOT.CNF na komputerze.

## Sterowanie v0.2 Manager

- GÓRA/DÓŁ — wybór gry (ISO katalogu DVR).
- SELECT — przełącz SYSTEM / ENGLISH. Domyślnie SYSTEM (sprawdzony).
- KWADRAT — zapis i weryfikacja oddzielnego DVR_TEST.CNF.
- X — backup oryginalnego BOOT.CNF, zapis i weryfikacja nowego BOOT.CNF, start OSDMenu BOOT.ELF.
- TRÓJKĄT — ponowne skanowanie; START — wyjście.

Tryb SYSTEM — niezmienione, działające Neutrino:

    boot=neutrino.elf
    arg=-bsd=dvr
    arg=-dvd=dvr_pfs0:/ISO/REZ.iso

Tryb ENGLISH — osobna binarka i własny EE core:

    boot=neutrino_lang.elf
    arg=-bsd=dvr
    arg=-dvd=dvr_pfs0:/ISO/REZ.iso
    arg=-cfg=osdlang
    arg=-osdlang=en

Opcja -cfg=osdlang ładuje plik config/osdlang.toml, który wskazuje na moduł
modules/ee_core_lang.elf; pozostałe config i modules są zachowane.

## Testy na rzeczywistym DESR-7000 (2026-10-15)

- Metal Slug Anthology: SYSTEM po japońsku, ENGLISH po angielsku.
- REZ: ENGLISH uruchomiony; angielski tekst ostrzeżenia o autosave oraz komunikatu o odczytaniu danych zapisu (potwierdzone zdjęciami).
- Na jednym ekranie REZ przycisk potwierdzenia pozostał po japońsku; poprawka nie tłumaczy zasobów wbudowanych w obraz ISO.
- Przekazanie sterowania w v0.2 nadal realizuje sprawdzony OSDMenu BOOT.ELF na USB. Manager ELF może być uruchamiany przez UDPFS.
- Nie testowano wszystkich gier; przed rozbudową zachować działający neutrino.elf i ee_core.elf.

## Kolejność testów

1. Zapisz działający BOOT.CNF na PC.
2. Zainstaluj trzy nowe pliki oraz testowy manager ELF.
3. Odpal znaną grę z SYSTEM, bez przełączania SELECT.
4. Odpal Manager ponownie, przełącz SELECT na ENGLISH i wybierz REZ.iso.
5. Najpierw naciśnij KWADRAT, sprawdź wygenerowany DVR_TEST.CNF.
6. Naciśnij X. Jeśli zawiesi się gra, restartuj konsolę; SYSTEM korzysta z niezmienionych plików.

Źródła: https://github.com/ps2max32/neutrino/pull/228
        https://github.com/ps2homebrew/Open-PS2-Loader/blob/master/ee_core/src/syshook.c
Pliki źródłowe z Neutrino dostępne na zasadach AFL-3.0 — repo zawiera tylko patch.
