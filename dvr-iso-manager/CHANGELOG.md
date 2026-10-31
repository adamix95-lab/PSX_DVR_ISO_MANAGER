# Changelog

## v0.8 — pierwsze pełne wydanie Managera z ART SYNC (31.10.2026)

- MONO Refined: 12 gier na stronę, nawigacja GÓRA/DÓŁ i LEWO/PRAWO, prawy panel PNG.
- Manualny skan TRÓJKĄTEM odczytuje Game ID z ISO i zapisuje `COVERS.IDX` oraz `COVER_SCAN.LOG` na USB.
- Ekran z liczbą brakujących okładek: X pobiera przez lokalny serwer HTTP, O anuluje; kolejny TRÓJKĄT pozwala ponowić pobieranie bez restartu.
- LAN ART SYNC weryfikuje HTTP/PNG/CRC, zapisuje nowe okładki przez plik `.PART` i automatycznie odbudowuje indeks.
- Aplikacja Windows/Python pobiera grafiki przez HTTPS, preferując archiwum OPL Manager, z zapasową PSBBN Art Database CosmicScale; lokalny cache.
- Wsparcie `SYSTEM`/`ENGLISH`, niezmieniony OSDMenu/Neutrino Quickboot oraz diagnostyczny `DVR_TEST.CNF`.
- **Test DESR-7000:** 40 ISO, usunięte indeksy i okładki; 40/40 pobranych, widocznych, REZ uruchomiony po angielsku. Działa anulowanie O, ponowienie i uruchomienie gry po synchronizacji.
- **Uwaga:** nie ma oddzielnego testu rzeczywistego pobrania z zapasowej bazy PSBBN; sukces 40/40 potwierdza cały system, nie wybór konkretnego źródła.
- Ochrona DVR: odczyt ISO bez zapisu na HDD/APA; pliki indeksu, ART i konfiguracja startowa wyłącznie na USB.

## v0.7.3 — integracja sieci i automatyczna synchronizacja

- Połączono skanowanie gier, obsługę X/O i przetestowane pobieranie LAN z Managerem.
- Wersja przejściowa miała błąd po anulowaniu O; poprawiono ponowne sprawdzanie braków z USB bez pełnego powtarzania skanowania ISO.
- Dodano aplikację Windows i bezpieczny fallback PSBBN dla brakujących grafik z OPL.
- Wewnętrzne eksperymentalne etapy nadal można odtworzyć w historycznych PR #15–#19.

## v0.5.3 — awaryjne Game ID z obrazu ISO

- Najpierw `SYSTEM.CNF` / `BOOT2`; w razie potrzeby pojedynczy jednoznaczny ID pliku wykonywalnego w katalogu głównym ISO9660.
- Nie zgadujemy Game ID po nazwie ISO. Diagnostyczny raport z kodami `NO_ART`, `NO_ART_EXE`, `NO_SYSTEM`, `AMBIGUOUS_EXE` i błędami odczytu.

## v0.5.1–v0.5.2 — podgląd PNG

- Prawdziwy kolorowy podgląd 88×128 w buforze GS; korekcja kolejności kanałów RGB i skalowanie bilinearne.
- Weryfikacja USB `COVERS.IDX` i raport z ręcznego skanu.

## v0.4 — nawigacja

- 12 pozycji na stronie, LEWO/PRAWO bez zawijania, GÓRA/DÓŁ po jednej pozycji.

## v0.1–v0.3 — podstawowy Manager

- Odczyt folderu DVR ISO, tworzenie/weryfikacja `BOOT.CNF` na USB i Quickboot przez OSDMenu/Neutrino.
- Wybór języka i minimalistyczny układ MONO.
