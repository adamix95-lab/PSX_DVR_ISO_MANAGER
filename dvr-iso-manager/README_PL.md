# PSX DVR ISO Manager v0.6 — MONO Refined

**Wersja eksperymentalna v0.6: wygląd interfejsu wymaga testu na Sony PSX DESR-7000.** Wersja v0.5.3 to sprawdzony punkt odniesienia: 40/40 okładek oraz działający Quickboot. Zachowaj kopię starego ELF i USB `BOOT.CNF`.

## Co zmieniono w v0.6

- Czarne tło, biały tekst i dyskretne odcienie szarości — bez dodatkowego graficznego systemu menu.
- Zaznaczona gra ma **ciemnoszare tło** na szerokości listy i biały tekst `>`. Pozostałe tytuły są lekko przygaszone.
- Cieńszy wizualnie separator (szary tekst), nagłówek `v0.6`, licznik `PAGE 01/04`, uporządkowane dwuwierszowe sterowanie.
- Mały szary napis `GAME ID` nad istniejącym ID; **Game ID nadal w wierszu 7, kolorowa okładka PNG nadal zaczyna się przy y=64 i ma 88×128 px**. Odrzucone wyrównanie okładki do `LANGUAGE` **nie wraca**.
- Status ma ograniczoną szerokość, żeby nie zasłaniać dolnej części okładki. Separator stopki również kończy się przed okładką.
- Zmiany w `draw()` i etykiecie wersji, **bez zmiany skanowania ISO/Game ID, plików indeksu, kompilacji Neutrino ani Quickboot**.

Wyświetlanie bazuje na znanym PS2SDK `scr_printf()`: `scr_setbgcolor()` ustawia tło pojedynczego wiersza znaków. Nie resetujemy GS ani nie instalujemy nowych zależności.

## Sterowanie

- **GÓRA / DÓŁ** — wybór gry, jedna pozycja na naciśnięcie.
- **LEWO / PRAWO** — zmiana strony listy (12 gier).
- **X** — uruchomienie wybranej gry przez istniejący BOOT.ELF / Neutrino.
- **TRÓJKĄT** — ręczne skanowanie listy ISO, Game ID, okładek i aktualizacja indeksu.
- **SELECT** — tryb SYSTEM / eksperymentalny ENGLISH.
- **KWADRAT** — zapis i weryfikacja `DVR_TEST.CNF`.
- **START** — wyjście z Managera.

## Ścieżki i bezpieczeństwo

```text
DVR (odczyt): dvr_pfs0:/ISO/*.iso

mass0:/DESR_DVR/
├── BOOT.ELF                   # OSDMenu Launcher
├── BOOT.CNF                   # Konfiguracja uruchomienia
├── BOOT_RECOVERY.CNF          # Kopia oryginalnej konfiguracji
├── neutrino.elf               # SYSTEM
├── neutrino_lang.elf          # ENGLISH — eksperymentalny
├── DVR_ISO_MANAGER.ELF        # Manager (przy testach v0.6: tylko ten plik)
├── COVERS.IDX                 # Cache na USB
├── COVER_SCAN.LOG             # Opcjonalny raport z TRÓJKĄTA
├── ART/
│   └── SLES_538.86_COV.png    # Przykład okładki zgodnej z OPL
├── config/                    # Konfiguracje Neutrino
└── modules/                   # Wymagane moduły
```

Normalny start czyta `COVERS.IDX` i nie analizuje ponownie wszystkich ISO. TRÓJKĄT w razie potrzeby sprawdza `SYSTEM.CNF`/`BOOT2`, a jeśli ID brakuje, unikalną nazwę pliku wykonywalnego w katalogu głównym ISO. W teście v0.5.3 uzyskano 40/40 dopasowanych okładek.

DVR i obrazy ISO pozostają **wyłącznie do odczytu**. Zmiany indeksu, raportu i BOOT.CNF dotyczą tylko USB. Quickboot nadal wymaga `mass0:/DESR_DVR/BOOT.ELF` i Neutrino na USB, nawet jeśli Manager uruchomiono przez UDPFS.

## Plan testowy v0.6 na DESR-7000

1. Zapisz na komputerze działający ELF **v0.5.3** oraz kopię USB `BOOT.CNF`. **Nie podmieniaj** innych plików.
2. Uruchom artefakt `PSX-DVR-ISO-Manager-v0.6-MONO-REFINED-UNTESTED` jako nowy `DVR_ISO_MANAGER.ELF`.
3. Zweryfikuj ciemnoszare zaznaczenie, czytelność białego tekstu, brak artefaktów oraz wolniejszego przewijania niż w v0.5.3.
4. Sprawdź pierwszą i ostatnią stronę listy, długi tytuł oraz brak nachodzenia stopki lub statusu na okładkę.
5. Sprawdź kolorowe okładki po restarcie, SELECT, TRÓJKĄT i KWADRAT. Na końcu uruchom znaną grę przez X (np. Black).
6. Jeśli coś jest nieczytelne lub konsola się zawiesi, przywróć poprzedni ELF v0.5.3.

**PR pozostaje Draft, dopóki v0.6 nie przejdzie testu na konsoli.**

Historia wcześniejszych wydań: [CHANGELOG.md](CHANGELOG.md).
