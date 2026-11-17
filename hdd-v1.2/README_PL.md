# PSX DVR ISO MANAGER 1.2 HDD — przygotowania

**Stan: testy. Nie wolno tworzyć Release 1.2 bez wyraźnej zgody właściciela projektu.**

## Cel

Połączenie sprawdzonego sprzętowo PDIM 1.0 HDD (NETFIX, FIX95, XMB, wybór SYSTEM/ENGLISH) z natywnym pobieraniem okładek HTTPS znanym z USB 1.2 R4.

**Jedyny docelowy obszar aplikacji: `hdd0:PP.DVRISO`.** Tu mają znaleźć się ELF-y, CNF, konfiguracje, okładki `ART/` i pliki uruchamiane przez XMB. `PP.DVRISO12` była tylko partycją testową. Obrazy ISO gier pozostają na oryginalnym dysku DVR (`dvr_pfs0:/ISO`) i są jedynie odczytywane.

## Dotychczasowy przebieg

- PDIM 1.0 HDD — sprawdzony start gier, język angielski, XMB i pobieranie 40+4 okładek przez NETFIX.
- PDIM USB 1.2 R4 — 45/45 okładek HTTPS, powrót do Managera, start gry; limit 50 na przebieg.
- PDIM 1.2 HDD na `PP.DVRISO12` — 45 okładek HTTPS i start Metal Slug w ENGLISH potwierdzone na DESR-7000.
- Installer R1 — problem IOP/fileXio -200. R2 — poprawny reset IOP, lecz XMB -31. R3 — sektorowa metoda PPAA wLE-R3Z i sprzętowy PASS XMB.
- R3 kopiował `EXECUTE.*` za wcześnie i zasoby `res/` za późno, już po aktywacji wpisu XMB.

## Docelowe poprawki

- Wariant kompilacji 1.2 HDD używa `PP.DVRISO` w Managerze, Neutrino, konfiguracji uruchamiania, HTTPS oraz w ścieżce `ART/`.
- X: 79 plików Core, bez `EXECUTE.ELF`, `EXECUTE.CNF` i trzech plików `res/`.
- TRIANGLE: pięć plików XMB, ich CRC/readback, synchronizacja PFS; **zapis PPAA na samym końcu**.
- Rozpoznawanie istniejącego nagłówka HDD 1.0 i bezpieczna aktualizacja wyłącznie jego identyfikatora wersji; obce nagłówki odrzucamy.
- Poprawiony układ tekstu, czerwone ostrzeżenie, potwierdzenia X oraz L1+R1+X.
- Istniejącej `PP.DVRISO` nie formatujemy, nie usuwamy i nie zmieniamy jej rozmiaru. Konfiguracja, ISO i ART pozostają zachowane.

## Plan Release 1.2 (tylko plan)

1. HDD 1.2 z automatycznym instalatorem HDD i XMB.
2. HDD 1.2 z kompletem plików do ręcznego tworzenia partycji, kopiowania i rejestracji XMB.
3. USB 1.2.

Paczki testowe z CI nie są wydaniem. Wydanie powstanie dopiero po osobnym pozwoleniu oraz testach instalacji, XMB, SYSTEM/ENGLISH, HTTPS, pobierania brakujących okładek i ponownej instalacji. Konieczna jest pełna odtwarzalna kopia sektorowa HDD.
