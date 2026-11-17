# PSX DVR ISO MANAGER 1.2 HDD HTTPS — TEST R1

**Wersja testowa dla Sony PSX DESR-7000. Bez publicznego wydania.**
Gałąź: `pdim_v1.2_hdd`.

## Pochodzenie kodu

Podstawą działania gry jest **sprzętowo sprawdzony PDIM 1.0 HDD** (NETFIX,
FIX95, OSDMenu, CNF, PFS1 i podgląd okładek). Punkt wyjścia dla tej
gałęzi to sprzętowo sprawdzony **HDD12 R3** — jego skompilowany Manager
to HDD 1.0 skierowany wyłącznie na `PP.DVRISO12`.
Z branch `pdim_usb_v1.2` przeniesiono mechanizm HTTPS R4:
wolfSSL, Roughtime, CA, 50 okładek/serię, fallback OPL/PSBBN,
normalizację PNG i ekran postępu.

**UWAGA: sam port HTTPS na HDD nie był jeszcze testowany na DESR-7000.**

## Wersja R1 — przeznaczenie

To **aktualizacja plików** istniejącej testowej partycji
`hdd0:PP.DVRISO12`, utworzonej instalatorem R3.
Nie wykonuje tworzenia partycji, formatowania PFS, zapisu nagłówka APA,
rejestracji XMB ani operacji na `PP.DVRISO` stabilnej wersji 1.0.

HTTPS worker uruchamia się z tego samego pliku co Manager. Po restarcie
IOP sam montuje `PP.DVRISO12` jako `pfs1:` RW, czyta
`pfs1:/V12_NET.CNF` i `pfs1:/N4E.CA`. Gry są nadal wykrywane w
`dvr_pfs0:/ISO` **tylko do odczytu**.

Istniejące okładki są sprawdzane w `pfs1:/ART`, a wyłącznie
brakujące zapisywane w tej samej lokalizacji jako
`GAME_ID_COV.png`. Każdy zapis nowej okładki wykorzystuje
`.PART`, wyłączność tworzenia, kontrolę odczytu, rename, PFS sync;
istniejących PNG/JPG/PART nie nadpisuje. Maksymalnie 50 na przebieg.

Po pobieraniu worker wraca do
`pfs1:/DVR_ISO_MANAGER_HDD.ELF`; Manager pokazuje wynik,
odświeża indeks na HDD i pozwala dalej uruchamiać gry.

## Instrukcja dla posiadacza działającej partycji HDD12 R3

**Przed zmianami wymagany jest sprawdzony, odtwarzalny backup
sektorowy całego HDD.** Nie ruszaj `PP.DVRISO`.

1. Uruchom wLE-R3Z, przejdź do `PP.DVRISO12`, skopiuj
   oryginalny `DVR_ISO_MANAGER_HDD.ELF` na komputer jako backup.
2. Rozpakuj paczkę. Skopiuj do katalogu głównego **tej partycji**:
   `COPY_TO_PP.DVRISO12/DVR_ISO_MANAGER_HDD.ELF` i
   `COPY_TO_PP.DVRISO12/N4E.CA`.
3. W katalogu `PP.DVRISO12` utwórz `V12_NET.CNF` z wartościami
   IP, NETMASK, GATEWAY, DNS z działającej konfiguracji.
   Dostarczony `V12_NET.CNF.example` zawiera **wyłącznie adresy przykładowe**.
   Jeśli sieć się zmieniła, ustaw bieżące parametry.
4. **Nie podmieniaj** `BOOT.ELF`, `BOOT.CNF`, `EXECUTE.ELF`,
   `EXECUTE.CNF`, plików Neutrino, ikon, folderu `res/`,
   okładek znajdujących się w `ART/`.
   **Nie wstrzykuj ponownie nagłówka XMB**.
5. Uruchom z oryginalnego XMB `PDIM HDD12 TEST`.
   Sprawdź napis `1.2 HDD` oraz uruchomienie jednej gry.
6. TRIANGLE uruchamia skan DVR i indeks okładek. Na ekranie
   potwierdzenia wybierz **TRIANGLE: SERVER (HTTPS, UP TO 50)**.
   **X: PC APP (LAN HTTP)** zachowuje lokalny serwer PC.
7. Sprawdź liczbę pobranych okładek, powrót do Managera, obraz PNG
   przy wybranej grze i jej uruchomienie. Ponowny skan powinien
   pomijać okładki obecne w `PP.DVRISO12/ART`.

### Raport po teście

Podaj: ile ISO wykryto, ile brakuje okładek, ile pobrał HTTPS, ile
SKIPPED, czy Manager powrócił, czy widać nową okładkę z HDD ART
i czy gra nadal startuje. Przy błędzie: dokładny kod i ostatni ekran.

**Nie kasuj ani nie formatuj partycji przy błędzie HTTPS.** Jeśli
Manager nie wystartuje, odtwórz tylko jego poprzedni ELF z własnego
backupu na tej samej partycji testowej. Nie naruszaj instalacji 1.0.

## Ograniczenia

- HDD HTTPS to R1, wymaga testów sprzętowych, nie publiczny Release.
- CPU-counter entropy R4 pozostaje rozwiązaniem eksperymentalnym; nie
  należy przedstawiać go jako certyfikowanego klienta TLS.
- Zapis nie kasuje starego `.PART`, nie nadpisuje istniejących okładek.
- Dopiero po sprawnym HTTPS wrócimy do dopracowania instalatora R3,
  układu tekstu i przeniesienia `EXECUTE.*` do etapu rejestracji XMB.
