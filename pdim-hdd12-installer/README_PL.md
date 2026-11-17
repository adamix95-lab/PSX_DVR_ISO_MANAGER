# PDIM HDD12 INSTALLER R3 — testowy kreator partycji i instalator XMB

**Stan: eksperymentalny, bez potwierdzenia na fizycznym Sony PSX DESR-7000.**
**Cel zapisu: wyłącznie testowa partycja \`hdd0:PP.DVRISO12\`.**
Działająca wersja HDD 1.0 w \`hdd0:PP.DVRISO\` jest oddzielna i nie jest
miejscem instalacji.

## !!!!!!!!!!!!!!!!!!! UWAGA !!!!!!!!!!!!!!!!!!!

**TWORZENIE PARTYCJI MODYFIKUJE GLOBALNĄ TABLICĘ APA CAŁEGO HDD.**

**AWARIA LUB BŁĄD STEROWNIKA MOŻE USZKODZIĆ TAKŻE INNE PARTYCJE
NA DYSKU, A NIE TYLKO \`PP.DVRISO12\`.**

**WYMAGANA JEST SPRAWDZONA, ODTWARZALNA KOPIA SEKTOROWA CAŁEGO HDD
PRZED PIERWSZYM ZAPISEM. KOPIA SAMEJ TABLICY PARTYCJI NIE WYSTARCZA!**

**NIE URUCHAMIAJ OPERACJI ZAPISU BEZ TAKIEJ KOPII.**

Te same komunikaty instalator pokazuje na całym ekranie z czerwonym tłem
przed operacjami modyfikującymi dysk. Pierwsze potwierdzenie to X, a drugie
wymaga **przytrzymania L1+R1 i naciśnięcia X**. O anuluje operację.

## Poprawka R2: błąd fileXio -200 i białe kwadraty

Pierwszy test R1 na DESR-7000 zatrzymał się **bez jakiegokolwiek zapisu
na HDD**: `LOAD fileXio id=-200 rv=0`. Kod -200 oznacza problem
z zależnościami modułów IOP. Poprzednio instalator próbował ponownie
wykorzystać sterowniki załadowane przez wLE-R3Z, rozpoznając je
wyłącznie po nazwach. Mogło to pomylić standardowy moduł Sony
`IO/File_Manager` z wymaganym `iomanX`.

R2 stosuje **sprawdzony w PDIM HDD 1.0 reset IOP i kolejność ładowania
modułów**, w tym pojedynczy `dev9_ns` z HDD NETFIX. Błędy modułów
wciąż zatrzymują instalator przed zapisem.

Usunięto także **białe kwadraciki kursora** przez `scr_setCursor(0)`
zaraz po `init_scr()` — jak w działającym PDIM.

**R2 został przetestowany na fizycznym DESR-7000: tworzenie partycji i kopiowanie PASS, rejestracja XMB FAIL (-31).** Najpierw uruchom
ELF i sprawdź, czy przechodzi `LOAD fileXio` oraz dochodzi do
sprawdzania `PP.DVRISO12`. **Nie naciskaj X ani Trójkąta, dopóki nie
masz sprawdzonej kopii sektorowej całego HDD.**

## Co właściwie instaluje

To **nie jest jeszcze port HTTPS na HDD 1.2**. Podczas prób używa plików
sprawdzonej wersji **PDIM 1.0 HDD**, z dwoma koniecznymi wyjątkami:
\`DVR_ISO_MANAGER_HDD.ELF\` oraz oba \`neutrino_hdd*.elf\` są kompilowane
z tego samego źródła 1.0, ale ich ścieżki \`PP.DVRISO\` są świadomie
przestawiane na \`PP.DVRISO12\`. Bez tej zmiany nowy Manager mógłby
odczytywać lub zapisywać pliki **starej, działającej** partycji 1.0!

Z paczki HDD 1.0 pozostają pliki OSDMenu, konfiguracje, moduły, EE core,
NETFIX oraz inne zasoby. Wykorzystywany jest również sprawdzony XMB
\`EXECUTE.ELF\` (KELF OSDMenu), \`system.cnf\`, \`icon.sys\`, \`info.sys\`,
oryginalne ikony PDIM \`jkt_001.png\` i \`jkt_002.png\`.
Zmieniamy jedynie nazwę testowego wpisu XMB, \`title_id\` i ścieżkę
w \`EXECUTE.CNF\` do \`PP.DVRISO12\`.

Źródłami są publiczne, niezmienione release
[HDD 1.0](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.0)
oraz
[XMB 1.0](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.0).
W paczce znajduje się także \`N4E.CA\` na przyszłe eksperymenty HTTPS;
obecny HDD 1.0 nie pobiera jeszcze okładek przez HTTPS.

## Struktura paczki USB FAT32

\`\`\`text
mass0:/
└── PDIM_HDD12_INSTALLER/
    ├── PDIM_HDD12_INSTALLER.ELF  <- Uruchom ten plik z wLE-R3Z 4.80
    ├── MANIFEST.TXT             <- ścieżki/rozmiary/CRC32
    ├── README_PL.txt
    ├── V12_NET.CNF.example      <- wzór, nie nadpisuje ustawień konsoli
    ├── FILES/
    │   ├── DVR_ISO_MANAGER_HDD.ELF
    │   ├── neutrino_hdd.elf
    │   ├── neutrino_hdd_lang.elf
    │   ├── EXECUTE.ELF
    │   ├── EXECUTE.CNF
    │   ├── BOOT.ELF
    │   ├── N4E.CA
    │   ├── NET_PROBE.CNF
    │   ├── config/
    │   ├── modules/
    │   └── res/
    └── XMB/
        ├── system.cnf
        ├── icon.sys
        ├── info.sys
        ├── jkt_001.png
        └── jkt_002.png
\`\`\`

## Obsługa R3: pełna instalacja bez ręcznego kopiowania zasobów XMB

Używamy **tych samych plików z publicznego wydania HDD 1.0**, w tym
KELF OSDMenu, `EXECUTE.CNF` i oryginalnych okładek XMB. Wykonywalny
Manager i obie wersje Neutrino są kompilowane z odniesieniem wyłącznie
do testowej `PP.DVRISO12`.

- **X — CREATE PARTITION IF MISSING + INSTALL CORE:** sprawdza partycję,
  tworzy tylko brakującą `PP.DVRISO12` jako 256 MiB PFS, instaluje i
  weryfikuje podstawowe pliki aplikacji. **Trzy pliki `res/` celowo
  pozostawia dla etapu rejestracji XMB**.
- **SQUARE — VERIFY CORE FILES (READ ONLY):** sprawdza rozmiary i CRC32
  plików podstawowych; nie wymaga jeszcze zasobów `res/`, nie zapisuje.
- **SELECT — INSPECT XMB HEADER (READ ONLY):** pokazuje sektory i
  deskryptory PPAA bez żadnych zapisów. Gdy brakuje partycji, zwraca błąd.
- **TRIANGLE — REGISTER XMB + COPY ASSETS:** sprawdza pliki aplikacji,
  źródła ikon i ich CRC na USB, odczytuje i aktualizuje nagłówek PPAA
  metodą sektorową wLE-R3Z. Następnie **automatycznie kopiuje i
  weryfikuje** na PFS:
  `res/info.sys`, `res/jkt_001.png` i `res/jkt_002.png`.
  Po operacji pełna weryfikacja manifestu powinna wykazać **84/84**.
  Żadnych plików z `res/` nie trzeba kopiować ręcznie przez wLE.
- **O — EXIT.**

Wszystkie komunikaty na ekranie konsoli — także pełnoekranowe ostrzeżenia
przed zapisem — są **po angielsku**. Ta instrukcja pozostaje po polsku.

### Czym R3 różni się od nieudanego R2?

R2 odczytywał nagłówek przez surowy uchwyt
`hdd0:PP.DVRISO12`, wymagał wyzerowanego obszaru i błędnie
umieszczał obraz PNG w polu `list.ico` struktury PPAA.
Wynik na DESR-7000: `FAIL -31`, po czym w XMB widniał wpis
`Corrupted Data`; użytkownik go usunął, a PDIM 1.0 nadal działał.

R3 opiera się na źródle
[wLE-R3Z hdd_header_injector.c](https://github.com/NathanNeurotic/wLaunchELF_R3Z/blob/master/src/hdd_header_injector.c):
`HDIOC_READSECTOR` i `HDIOC_WRITESECTOR`, odczyt wartości LBA APA
i sektory **start+8 / +9 / +10**. Kod zachowuje pozostałe bajty
sektora PPAA, modyfikuje wyłącznie deskryptory `system.cnf` i
`icon.sys`, zapisuje tekst przed deskryptorem i sprawdza odczyt
po zapisie. W przeciwieństwie do ogólnego mechanizmu wLE zapisuje
tylko trzy wymagane sektory, a nie ponownie wszystkie 4096.

Nagłówek może być obcy lub częściowo zapisany; taki przypadek
kończy się **STOP bez nadpisywania**. Jeśli poprawny nagłówek R3
już istnieje, TRIANGLE może dokończyć brakujące pliki `res/`.
W razie wyłączenia zasilania między zatwierdzeniem nagłówka a
ukończeniem kopiowania zasobów wpis XMB **może tymczasowo wyglądać
jak Corrupted Data**. Nie formatuj wtedy partycji; sprawdź wynik
diagnostyki i ponów jedynie dokończenie instalacji na poprawnym nagłówku.

**R3 ma PASS testów hostowych dopiero po ukończeniu CI; nie ma jeszcze
potwierdzenia XMB na fizycznym DESR-7000.** Przed pierwszym zapisem
konieczna jest sprawdzona kopia sektorowa całego dysku.

## Dlaczego nie ma przycisku FORMAT / DELETE / CLEAN?

Celowo: testowy instalator ma **sztywną białą listę jednej partycji**,
bez operacji usuwania partycji, bez resize i bez ogólnego menu HDD.
Wszystkie błędy zapisów pozostawiają dane i tymczasowe pliki do
ręcznej inspekcji — żaden automatyczny rollback nie usuwa partycji.

Szczególnie jeśli na dysku istnieje już \`PP.DVRISO12\` z dawnych
testów Roughtime/HTTPS, zachowaj jej \`V12_NET.CNF\` i pozostałe pliki.
Kreator nie zastępuje konfiguracyjnych plików użytkownika.
Gdy masz błąd montowania: **STOP, NIE FORMATUJ!**

## Po instalacji

Najpierw uruchom \`pfs1:/DVR_ISO_MANAGER_HDD.ELF\` ręcznie przez wLE.
Sprawdź start gier i język ENGLISH; nie dotykaj partycji
\`PP.DVRISO\`. Dopiero po takim teście wciśnij TRIANGLE w instalatorze
w celu zarejestrowania wpisu XMB. W menu Games powinna się pojawić
odróżnialna pozycja **PDIM HDD12 TEST** z dotychczasową ikoną PDIM.

Po `TRIANGLE` i końcowym `PASS (0)`, obejmującym nagłówek oraz trzy pliki `res/`, wyłącz konsolę całkowicie, odłącz i
podłącz zasilanie, następnie uruchom XMB. Nie usuwaj istniejącej
instalacji PDIM HDD 1.0.

## Ograniczenia

- Kompilacja i testy hostowe nie zastępują prawdziwych testów DESR-7000.
- Ograniczenie adresowania LBA28 jest koniecznym filtrem, lecz
  **nie gwarantuje**, że sterownik APA nie popełni błędu.
- Brak automatycznej procedury odzyskiwania po przerwaniu formatowania.
- Kopia sektorowa dysku jest warunkiem testów zapisu.
- Brak HTTPS HDD w tej paczce. To tylko przygotowanie partycji i instalator.
- Wykorzystujemy mechanizmy APA/PFS ze źródeł PS2SDK i znanych
  instalatorów; IRX \`ps2hdd-sparse-skip\` oraz inne moduły z
  [RepairBox Daniela](https://github.com/DanielFergisz/RepairBox-PSX-XMB-Apps-Installer).
  Przed publikacją konieczna weryfikacja pochodzenia/licencji binariów IRX.
- Same assety XMB i KELF pochodzą z naszego release HDD 1.0;
  ich autorzy i licencje pozostają wymienieni w dokumentacji projektu.

**Ten projekt jest eksperymentalny; w żadnym wypadku nie ingeruj w
oryginalne partycje DVR, systemowe Sony ani działającą \`PP.DVRISO\`.**
