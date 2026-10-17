# PSX DVR ISO Manager v0.3 — MONO

**Wersja testowa GUI: nieprzetestowana jeszcze na rzeczywistym Sony PSX DESR-7000.**
Sprawdzona v0.2 i jej wymuszanie angielskiego pozostają w `main`.

## Co zmieniono w v0.3

- Nagłówek `PSX DVR ISO MANAGER` z małym oznaczeniem `v0.3`.
- Monochromatyczny interfejs: czarne tło, biały tekst; zaznaczona gra wyróżniona znakiem `>`.
- Ukryte diagnostyczne linie `DVR: dvr_pfs0:/ISO` i `DVRP notice / hdd0 / pfs0 root`.
- Zwykły wynik skanowania zastąpiono licznikiem `N GAMES`; diagnostyka pojawia się tylko przy błędzie.
- Na ekranie nie wyświetlamy `.iso` ani `.ISO`, ale pełna oryginalna nazwa nadal trafia do BOOT.CNF.
- Ukryte `Selected 1 / 7` i techniczna linia `Backup: ... No DVR writes`.
- Wyłączony blokowy kursor PS2SDK (`scr_setCursor(0)`), który zostawiał białe kwadraty między sekcjami.
- Lista ma maksymalnie 12 pozycji na stronę; numer strony pokazujemy dopiero przy większej liczbie gier.
- Na dole widoczne są tylko sterowanie i, w razie potrzeby, komunikat o teście CNF/błędzie.

## Sterowanie

| Przycisk | Działanie |
|---|---|
| GÓRA / DÓŁ | Wybór gry |
| X | Generowanie/odczyt i weryfikacja BOOT.CNF, a następnie start BOOT.ELF (OSDMenu Launcher) |
| SELECT | Przełącza `SYSTEM` / `ENGLISH` globalnie (domyślnie SYSTEM) |
| KWADRAT | Zapis/odczyt testowego DVR_TEST.CNF, bez uruchamiania gry |
| TRÓJKĄT | Ponowne skanowanie DVR |
| START | Wyjście |

## Bez zmian względem działającej v0.2

- Obrazy ISO nadal odczytywane z `dvr_pfs0:/ISO/` (partycja `__xcontents`).
- Wybór `SYSTEM` używa sprawdzonego `neutrino.elf`, a `ENGLISH` osobnego `neutrino_lang.elf` z `ee_core_lang.elf`.
- Manager tworzy kopię BOOT_RECOVERY.CNF (tylko przy pierwszym X), zapisuje i weryfikuje BOOT.CNF,
  po czym uruchamia ten sam BOOT.ELF na USB, który działa na konsoli.
- Nie ma zapisów na partycjach DVR/APA i nie zmieniamy XMB.
- Przełączenie języka nie tłumaczy zasobów gry, jeśli w ISO nie ma tekstów angielskich.
- Użyte są dokładnie te same generatory CNF i funkcje launchera co w v0.2.

**Ważne:** uruchomienie samego Managera z UDPFS NIE przenosi ścieżki Quickboot na sieć.
Pliki `mass0:/DESR_DVR/BOOT.ELF`, BOOT.CNF i obydwa Neutrino pozostają na USB.
Nie podmieniaj ich podczas testowania zmian w interfejsie.

Dokumentacja Neutrino v0.2: `neutrino-v02/README_PL.md`.
