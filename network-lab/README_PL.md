# PSX DESR LAN A/B/C — odizolowane testy DEV9

Celem jest rozróżnienie problemu **DEV9** od współpracy **ATA/PFS + Ethernet** w PSX DESR-7000. To eksperyment, a nie aktualizacja działającego Managera 1.0 HDD.

| ELF | Ładowane sterowniki DEV9/HDD | Operacje na partycji | Raport |
|---|---|---|---|
| `DVR_NETTEST_A.ELF` | `bdm`, FAT, USB mini + `dev9_ns`, bez ATA/PFS | Żadnych — pobrania do RAM | Na ekranie, 44 pozycje |
| `DVR_NETTEST_B.ELF` | `bdm`, FAT, USB mini + `ps2dev9`, bez ATA/PFS | Żadnych — pobrania do RAM | Na ekranie, 44 pozycje |
| `DVR_NETTEST_C.ELF` | `ps2dev9`, `ps2atad`, `ps2hdd`, `ps2fs`, `dvrdrv`, `dvrfile` | Tylko `PP.NETTEST/ART`; weryfikacja zapisu PNG | Na ekranie + `REPORT_C.TXT` |
| `DVR_NETTEST_CLEAN.ELF` | Stos HDD jak C, **bez inicjalizacji Ethernetu** | Usuwa tylko pliki z 44-elementowej białej listy w `PP.NETTEST/ART`, po potwierdzeniu X | Raport liczby usuniętych / błędów |

W każdym teście A/B/C: to samo 44 Game ID, ten sam kod TCP/HTTP/CRC, ten sam serwer PC i **recv ≤ 512 B**, jedna próba na grę. Wszystkie 44 identyfikatory oraz ustawienia sieciowe są wbudowane w binaria; A/B nie muszą montować dysku po zresetowaniu IOP. Raport: **OK/BAD/LINK/HTTP/MISS/PFS**, liczba poprawnych i odrzuconych obrazów oraz lista wyników na 4 stronach (LEWO/PRAWO). X rozpoczyna test, START wychodzi. Wyniki A/B fotografujemy (nie zapisują plików).

## Dedykowana partycja na HDD

**Przed uruchomieniem** załóż lub przygotuj istniejącą, pustą partycję aplikacji **`PP.NETTEST`** w wolnym miejscu dysku, używając sprawdzonego narzędzia PS2 z obsługą APA/PFS. **Nie twórz jej przez nadpisywanie istniejącej partycji Sony**. Zrób kopię ważnych danych. Żaden ELF nie tworzy ani nie formatuje partycji; jeśli `hdd0:PP.NETTEST` nie istnieje lub nie jest prawidłowym PFS, C i CLEAN pokażą błąd i nie będą niczego modyfikować. A/B nie wymagają partycji do działania i mogą być uruchomione z USB; można też umieścić wszystkie cztery ELF-y w katalogu głównym przygotowanej partycji i uruchamiać je przez wLaunchELF.

`PP.NETTEST/ART` jest **jedynym folderem czyszczonym przez CLEAN**. Program nie usuwa samego folderu ART ani innych plików; nie zna ścieżek `PP.DVRISO`, `PP.TESTDVR` ani partycji systemowych Sony. Program CLEAN odmawia kasowania bez wciśnięcia **X**. Raport `REPORT_C.TXT` jest nadpisywany wyłącznie przez wariant C — przed kolejnym C zachowaj kopię poprzedniego raportu. A/B nie zapisują raportów, ponieważ zapis wymagałby ładowania ATA/PFS, co zaburzyłoby porównanie.

## Kolejność

1. **CLEAN** → X (czyści wyłącznie `PP.NETTEST/ART`), START.
2. **A** → X → zdjęcie sumy OK/BAD/LINK/HTTP/MISS + stron raportu → START.
3. **CLEAN** ponownie (folder ART pozostaje pusty po A), START.
4. **B** → X → zdjęcie raportu → START.
5. **CLEAN** → X, następnie **C** → X → zdjęcie raportu; skopiuj `PP.NETTEST/REPORT_C.TXT` przed kolejnym przebiegiem C.
6. Powtórz każdy test 2–3 razy, zawsze z tym samym serwerem i bez innych transferów w tle.

Uwaga: A/B bez ATA/PFS nie mogą usuwać plików z HDD, bo właśnie brak tych sterowników jest warunkiem eksperymentu. Czyszczenie realizuje **osobny ELF CLEAN** pomiędzy próbami. Czyści tylko wygenerowane PNG z naszej listy, a nie cały folder `ART` na ślepo.

## Oczekiwana interpretacja

- A=44/44, B dużo błędów → podejrzenie różnic `dev9_ns` vs `ps2dev9`.
- A i B=44/44, C ma błędy → wpływ pełnego stosu HDD/DVR lub zapisu PFS.
- Wszystkie warianty mają błędy → szukamy w stosie Ethernet/HTTP/buforowaniu/CRC, a nie tylko w DEV9.

Nie zmieniamy działającego `DVR_ISO_MANAGER_HDD_TEST.ELF`, `neutrino_hdd.elf`, `neutrino_hdd_lang.elf` ani `BOOT.CNF`. Nie podmieniamy modułów w uruchamianym Managerze.

Doprecyzowanie: A/B ładują ten sam zestaw modułów USB/BDM co sprawdzony USB 0.8. Dzięki temu różnica między A a B obejmuje wyłącznie moduł DEV9. Nie oznacza to, że do transferu potrzebny jest pendrive — HTTP nadal odbywa się przez Ethernet.
