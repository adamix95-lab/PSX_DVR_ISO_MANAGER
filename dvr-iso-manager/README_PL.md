# PSX DVR ISO Manager v0.8 — informacje techniczne

[Instrukcja użytkownika po polsku](../README_PL.md) · [English README](../README.md) · [Serwer okładek Windows](../pc-art-server/README.md)

## Stan wydania

**v0.8 działa na fizycznym Sony PSX DESR-7000.** Użytkownik przetestował 40 ISO na partycji DVR, usunął okładki oraz indeksy i pobrał ponownie 40/40 grafik z aplikacją Windows; wszystkie są widoczne. Sprawdzono też anulowanie i ponowne uruchomienie pobierania bez restartowania Managera oraz uruchomienie REZ w języku angielskim.

**Nie zostało osobno potwierdzone**, czy podczas tego testu zadziałało zapasowe źródło PSBBN (główne archiwum OPL mogło dostarczyć wszystkie pliki).

## Budowanie

Manager wymaga PS2SDK, biblioteki libpng/libz dla PS2 oraz modułów IOP zgodnych z DVR. Projekt CMake: `dvr-iso-manager/CMakeLists.txt`. Przetestowane procedury LAN/HTTP/PNG i ART SYNC są częścią Managera w `src/net/` oraz `include/`; osobny program Probe nie jest już potrzebny.

```sh
cmake -S dvr-iso-manager -B build-dvr-manager
cmake --build build-dvr-manager -j2
```

Plik wynikowy: `build-dvr-manager/DVR_ISO_MANAGER.ELF`. Alternatywnie pobierz automatyczny artefakt **PSX-DVR-ISO-Manager-v0.8** w [GitHub Actions](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions).

## Skanowanie gier i okładek

- ISO są odczytywane z `dvr_pfs0:/ISO/` **bez zapisu na HDD**, APA i do plików gier.
- TRÓJKĄT uruchamia skan listy ISO. Game ID jest odczytywane najpierw z `SYSTEM.CNF` wewnątrz ISO, a w razie potrzeby z jednoznacznego identyfikatora pliku wykonywalnego w katalogu głównym ISO.
- Okładek szukamy w `mass0:/DESR_DVR/ART/<GAMEID>_COV.png` (ewentualnie `.jpg`; podgląd kolorowy jest przeznaczony dla PNG).
- `mass0:/DESR_DVR/COVERS.IDX` przechowuje powiązania **tylko odnalezionych** okładek. `mass0:/DESR_DVR/COVER_SCAN.LOG` przechowuje też wpisy `NO_ART` / `NO_ART_EXE` dla rozpoznanych Game ID bez grafiki.
- Po skanie Manager wyświetla liczbę brakujących okładek. **X** na osobnym ekranie potwierdzenia pobiera przez LAN, **O** wraca do listy. Po O następny TRÓJKĄT ponownie sprawdza braki z logu na USB, bez konieczności ponownego czytania każdego ISO.
- Pobieranie tworzy wyłącznie nowe PNG, zweryfikowane przez HTTP, CRC i odczyt pliku `.PART` z USB. Po udanym zapisie Manager automatycznie odnawia `COVERS.IDX`.

W przypadku niemożliwego do odczytania Game ID skaner nie zgaduje na podstawie samej nazwy ISO. Błędny lub niekompletny plik raportu może wymagać nowego skanowania.

## Nawigacja i Quickboot

Strony zawierają **12 gier**; LEWO/PRAWO przechodzi między stronami, GÓRA/DÓŁ przesuwa zaznaczenie. X poza ekranem pobierania zapisuje i sprawdza konfigurację `BOOT.CNF` na USB, a następnie uruchamia istniejący OSDMenu `BOOT.ELF` i Neutrino. Kopia konfiguracji jest przechowywana w `BOOT_RECOVERY.CNF`.

SELECT przełącza `SYSTEM` / `ENGLISH`. REZ w trybie ENGLISH został uruchomiony pomyślnie na sprzęcie.

## Pliki i bezpieczeństwo

Manager wczytuje jedynie zawartość partycji DVR. Pliki `COVERS.IDX`, `COVER_SCAN.LOG`, `ART/`, `BOOT.CNF`, `DVR_TEST.CNF` i kopia zapasowa są na USB. Połączenie HTTP jest możliwe dopiero po ręcznym potwierdzeniu X. Nie nadpisujemy istniejących grafik i nie używamy bezpośrednio HTTPS na konsoli.

Zachowaj działające sterowniki DVR / Neutrino, pliki startowe i konfigurację. Program nie służy do formatowania HDD ani instalowania ISO.

## Rozwój

Starsze eksperymenty v0.4–v0.7.x pozostają opisane w [CHANGELOG.md](CHANGELOG.md). Historyczne testy Probe zostały włączone do testów Managera (`tests/net/`). Osobna aplikacja diagnostyczna nie jest dostarczana ani budowana.
