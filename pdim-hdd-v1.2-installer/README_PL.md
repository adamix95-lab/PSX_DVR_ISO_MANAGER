# Instalator PSX DVR ISO MANAGER 1.2 HDD + XMB

**Stan: HARDWARE PASS — Sony PSX DESR-7000 (listopad 2026).** Potwierdzono na konsoli instalację Core/XMB, ponowne uruchomienie instalatora, inspekcję tylko do odczytu i obsługę konfiguracji sieci.

**Status: przygotowania do wydania. Nie tworzyć publicznego Release/taga 1.2 bez wyraźnej zgody.**

[English](README.md) · [README HDD 1.2 / HTTPS](../hdd-v1.2/README_PL.md) · [Instalacja ręczna](../hdd-v1.2/INSTALL_MANUAL_PL.md)

## Cel i bezpieczeństwo

Instalator przygotowuje **`hdd0:PP.DVRISO`** dla wersji **PDIM 1.2 HDD** i rejestruje uruchamianie aplikacji w Sony PSX DESR XMB. Nazwa `PP.DVRISO12` służyła wyłącznie testom i **nie jest celem finalnej instalacji**.

Jeżeli `PP.DVRISO` już istnieje, instalator **nie formatuje, nie usuwa ani nie zmienia jej rozmiaru**. Jeśli partycji nie ma, może utworzyć nową PFS 256 MiB po osobnych potwierdzeniach. Tworzenie partycji zmienia globalne metadane APA całego dysku i błąd może dotknąć także inne partycje. **Przed każdym zapisem należy wykonać i sprawdzić pełną, odtwarzalną kopię sektorową HDD.**

Uruchomienie instalatora, jego kontrola początkowa, SQUARE i SELECT nie zapisują danych. Operacje zapisu wymagają **dwóch potwierdzeń: X, a następnie przytrzymaj L1 + R1 i naciśnij X**. O anuluje.

## Sterowanie instalatorem

| Przycisk | Działanie | Wynik |
| --- | --- | --- |
| **X — Install Core** | Kopiowanie i weryfikacja CRC **79** plików | Manager 1.2 HTTPS, Neutrino FIX95, OSDMenu, moduły, konfiguracja i CA |
| **TRIANGLE — Install XMB** | Kopiowanie i kontrola **5** plików XMB, następnie PPAA | Wpis XMB aktywowany dopiero po sprawdzeniu **84/84** |
| **SQUARE** | Kontrola Core tylko do odczytu | **79** plików manifestu |
| **SELECT** | Inspekcja nagłówka XMB/PPAA tylko do odczytu | Kontrola sektorów docelowej partycji |
| **O** | Wyjście lub anulowanie | Bez zapisu przy anulowaniu |

### Dlaczego instalacja XMB jest osobnym etapem?

Sprawdzony R3 kopiował `EXECUTE.ELF` i `EXECUTE.CNF` podczas X, a trzy pliki `res/` dopiero *po* wstrzyknięciu nagłówka PPAA. W HDD 1.2 kolejność została poprawiona:

1. **X instaluje 79 plików Core** — bez pięciu plików XMB.
2. **TRIANGLE najpierw kopiuje i sprawdza:** `EXECUTE.ELF`, `EXECUTE.CNF`, `res/info.sys`, `res/jkt_001.png`, `res/jkt_002.png`.
3. Każdy plik jest weryfikowany na podstawie rozmiaru/CRC, a przed PPAA ponownie odczytywane są **wszystkie 84 pliki**.
4. Sprawdzony mechanizm sektorowy wywodzący się z wLE-R3Z zapisuje odpowiednie sektory PPAA od **startu partycji +8, +9, +10**, z końcowym zapisem deskryptora/nagłówka. Błąd kopiowania z USB **nie rozpocznie rejestracji PPAA**.

Zgodny, wcześniej zainstalowany nagłówek HDD 1.0 może zostać rozpoznany i zmieniony z `title1=HDD 1.0` na `title1=HDD 1.2`. Obce lub częściowe nagłówki są odrzucane. Sprawdzony podpisany `EXECUTE.ELF` (KELF) z HDD 1.0 pozostaje niezmieniony.

## Aktualizacja HDD 1.0 i ochrona plików użytkownika

- Istniejące `BOOT*.CNF`, `V12_NET.CNF`, `ART/`, indeksy i obrazy ISO **nie są nadpisywane** przez manifest instalacyjny.
- Jeśli poprzedni ELF Managera różni się od nowego, program zapisuje wersję testową jako `.PDIMNEW`, synchronizuje i kontroluje CRC, a stary ELF pozostawia jako `DVR_ISO_MANAGER_HDD.ELF.PDIMBAK`. Kolejne kopie mogą używać `.PDIMBAK2`–`.PDIMBAK9`, bez nadpisywania starszych.
- Niespodziewanie różniące się inne pliki nie są zastępowane automatycznie. Przy nieudanej instalacji należy ręcznie sprawdzić pliki tymczasowe i kopie, **nie formatować ani nie usuwać partycji**.
- **Import `V12_NET.CNF` jest osobny i nie wlicza się do 79 plików Core.** Istniejący plik na HDD zawsze otrzymuje **KEPT**. Gdy go brakuje, instalator sprawdza `mass0:/PDIM_HDD_V12_INSTALLER/V12_NET.CNF`, weryfikuje składnię, kopiuje przez plik tymczasowy i ponownie kontroluje CRC (**COPIED**). Przy brakującym lub błędnym pliku na USB instalacja Core może się zakończyć, ale HTTPS wymaga ręcznego ustawienia sieci.

Dostarczona konfiguracja:

~~~ini
IP=192.168.0.240
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
DNS=1.1.1.1
~~~

To dane konkretnej sieci, **nie uniwersalne wartości każdego routera**. Jeśli trzeba, zmień plik na USB. Nie przypisuj jednego statycznego IP dwóm urządzeniom. `V12_NET.CNF` pozostaje **poza stałym manifestem 84 plików**.

## Struktura pendrive'a FAT32

Rozpakuj automatyczną paczkę testową, zachowując układ:

~~~text
mass0:/
└── PDIM_HDD_V12_INSTALLER/
    ├── PDIM_HDD_V12_INSTALLER.ELF
    ├── MANIFEST.TXT
    ├── V12_NET.CNF
    ├── V12_NET.CNF.example
    ├── FILES/                (84 pliki: 79 Core + 5 XMB)
    └── XMB/
        ├── system.cnf
        ├── icon.sys
        ├── info.sys
        ├── jkt_001.png
        └── jkt_002.png
~~~

Paczka **MANUAL** zamiast tego zawiera `PP.DVRISO-FILES/`, `__Headers/PP.DVRISO/` i osobny `V12_NET.CNF`. Należy korzystać z [instrukcji ręcznej](../hdd-v1.2/INSTALL_MANUAL_PL.md); nie zapewnia ona automatycznych kopii ELF i kontroli CRC instalatora.

## Testowanie na Sony PSX DESR-7000

1. Zrób pełny, odtwarzalny backup sektorowy HDD. Oddzielnie zabezpiecz ELF Managera, `BOOT.CNF`, `V12_NET.CNF` i okładki.
2. Uruchom `mass0:/PDIM_HDD_V12_INSTALLER/PDIM_HDD_V12_INSTALLER.ELF` w **wLE-R3Z 4.80**. Upewnij się, że cel to **`PP.DVRISO`**. SELECT pozwala obejrzeć PPAA bez zapisu.
3. Wybierz **X**, potwierdź dwukrotnie (X; L1+R1+X). Sprawdź **79 zweryfikowanych plików Core** i osobny status `V12_NET.CNF: COPIED` lub `KEPT`. Istniejąca partycja nie może zostać sformatowana.
4. Przed instalacją wpisu XMB możesz uruchomić `pfs1:/DVR_ISO_MANAGER_HDD.ELF` przez wLE i sprawdzić gry oraz SYSTEM/ENGLISH.
5. Wróć do instalatora, wybierz **TRIANGLE**, potwierdź dwukrotnie. Pięć plików XMB musi zostać zweryfikowanych **przed** zapisem PPAA, po czym oczekujemy **84/84** i PPAA PASS.
6. Całkowicie zrestartuj konsolę. Sprawdź wpis w XMB Games, brak **Corrupted Data**, uruchamianie Managera i gier w obu trybach języka.
7. Przetestuj HTTPS, usuwając tylko wybrane, wcześniej zabezpieczone okładki. Sprawdź pobranie PNG, podgląd, powrót do listy i ponowne skanowanie.
8. Ponownie uruchom **X**, **TRIANGLE**, **SQUARE** i **SELECT**, upewniając się, że konfiguracja, okładki i inne dane nie zostały zmienione.

Przy błędzie partycji/PFS/CRC/PPAA lub przerwanej operacji: **STOP, zapisz dokładny kod i sprawdź stan ręcznie.** Nie usuwaj i nie formatuj dysku automatycznie.

## Potwierdzone testy sprzętowe

| Test | Wynik na DESR-7000 |
| --- | --- |
| Pierwsza instalacja Core i XMB na `PP.DVRISO` | **PASS** |
| Start z XMB, gry, SYSTEM/ENGLISH i HTTPS | **PASS** |
| Ponowne X | **COPIED 0 / KEPT 79 / VERIFIED 79** |
| Ponowne TRIANGLE | **5 KEEP / COPY 0 / VERIFIED 84 / ALREADY VERIFIED** |
| SQUARE i SELECT | **PASS / PASS** |
| Automatyczna obsługa `V12_NET.CNF` | **PASS** |
| Kompilacja, testy hostowe i obie paczki HDD | **PASS** — [CI 44253182195](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44253182195) |

**HARDWARE PASS dotyczy funkcjonalności; nie oznacza gwarancji atomowości zapisu ani automatycznego odzyskiwania uszkodzonej APA.**

## Historia R1 → R3

- **R1:** błąd zależności IOP `fileXio -200` przed zapisem HDD.
- **R2:** naprawione ładowanie modułów; błąd nagłówka XMB `-31`.
- **R3:** zapis sektorowy w stylu wLE-R3Z, sprzętowy PASS instalacji XMB, ale nieoptymalny podział plików Core/XMB.
- **HDD 1.2:** pięć plików XMB przed PPAA, czytelniejszy angielski interfejs i czerwone ostrzeżenie, bezpieczne powtórzenie instalacji, aktualizacja HDD 1.0 oraz opcjonalne kopiowanie `V12_NET.CNF`.

Sprawdzony instalator **R3 i wydanie HDD 1.0** pozostają nienaruszone. Nowy instalator rozwijamy na osobnej gałęzi.

**Release 1.2 nie został jeszcze utworzony ani zatwierdzony do publikacji.**
