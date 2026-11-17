# Instalator PDIM 1.2 HDD — PP.DVRISO

**WYŁĄCZNIE TESTY. NIE PUBLIKOWAĆ RELEASE 1.2 BEZ ZGODY.**

## UWAGA — zapis na HDD

Operacje tworzenia partycji zmieniają globalne metadane APA. Awaria może wpłynąć na cały dysk DESR, także na inne partycje. Przed zapisem wykonaj i zweryfikuj **pełną, odtwarzalną kopię sektorową HDD**.

**Docelowa partycja: `PP.DVRISO`.** `PP.DVRISO12` służyła tylko testom. Istniejąca partycja `PP.DVRISO` nie jest formatowana, usuwana ani zmieniana. Obrazy gier pozostają na natywnej partycji DVR.

## Obsługa

| Przycisk | Funkcja |
| --- | --- |
| **X — Install Core** | 79 plików Managera 1.2 HDD, Neutrino, modułów i konfiguracji |
| **TRIANGLE — Install XMB** | 5 plików XMB, pełne CRC, potem na samym końcu nagłówek PPAA |
| **SQUARE** | Kontrola 79 plików, tylko odczyt |
| **SELECT** | Diagnostyka nagłówka XMB/PPAA, tylko odczyt |
| **O** | Wyjście / anulowanie |

Przed każdym zapisem pojawiają się dwa potwierdzenia: najpierw X, potem **L1+R1+X**. Ostrzeżenie pozostaje po angielsku.

**TRIANGLE instaluje kolejno:**
`EXECUTE.ELF`, `EXECUTE.CNF`, `res/info.sys`, `res/jkt_001.png`, `res/jkt_002.png`.
Każdy plik zostaje odczytany ponownie i porównany z manifestem. Dopiero po PASS **84/84** instalator zapisuje PPAA (sektory start+8/+9/+10). Jeżeli kopiowanie nie powiedzie się, PPAA pozostaje nienaruszony.

## Aktualizacja z działającej wersji 1.0 HDD

Instalator rozpoznaje istniejącą `PP.DVRISO`. Nie usuwa plików użytkownika ani katalogu `ART/` i nie podmienia `BOOT.CNF` ani `V12_NET.CNF`. Starego Managera zachowuje pod nazwą `DVR_ISO_MANAGER_HDD.ELF.PDIMBAK`, nowy zapisuje przez plik tymczasowy `.PDIMNEW` i kontrolę CRC. Nie nadpisuje wcześniejszego backupu. W razie przerwania instalacji zatrzymaj działania i przeprowadź ręczną kontrolę; nie formatuj dysku.

Wpis XMB z HDD 1.0 zostanie rozpoznany tylko przy zgodności ze znanym nagłówkiem. Instalator pozwoli wtedy zmienić wyłącznie tekst wersji na HDD 1.2. Nie ingeruje w obce nagłówki.

## Procedura testowa

1. Przygotuj pełną kopię sektorową HDD, dodatkowo zachowaj ELF 1.0, `BOOT.CNF`, `V12_NET.CNF` i okładki.
2. Rozpakuj paczkę testową na FAT32 USB, uruchom `mass0:/PDIM_HDD_V12_INSTALLER/PDIM_HDD_V12_INSTALLER.ELF` w wLE-R3Z 4.80.
3. Sprawdź bez zapisu, że wykrywa `PP.DVRISO`; SELECT może pokazać aktualny nagłówek.
4. X + potwierdzenia — oczekuj **79/79 Core PASS**. Bez formatowania istniejącej partycji.
5. Spróbuj uruchomić Managera przez wLE, sprawdź listę ISO, język SYSTEM i ENGLISH oraz start gry.
6. TRIANGLE + potwierdzenia — 5 plików XMB i **84/84 CRC PASS**, później PPAA PASS.
7. Pełny restart konsoli. Sprawdź ikonę i start z XMB (bez `Corrupted Data`).
8. W `PP.DVRISO/V12_NET.CNF` umieść prawdziwą konfigurację sieci; plik `V12_NET.CNF.example` ma tylko przykładowe adresy.
9. Wykonaj próbę HTTPS na **wybranych, wcześniej zabezpieczonych** brakujących okładkach. Sprawdź pobranie, podgląd, limit 50, powrót do listy i ponowny skan bez duplikatów.
10. Ponownie użyj instalatora X i TRIANGLE — zgodne pliki i nagłówek powinny zostać rozpoznane. `ART/`, CNF i gry muszą pozostać bez zmian.

W razie błędu zapisz dokładny kod i ekran, nie podejmuj automatycznego usuwania, formatowania ani naprawy APA.

## Paczki i publikacja

Z CI powstają dwie wersje testowe HDD 1.2: automatyczna i manualna. Wydanie 1.2 ma w przyszłości zawierać również USB 1.2. **Nadal jest to etap przygotowań — bez Release.**
