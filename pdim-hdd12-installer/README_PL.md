# PDIM HDD12 INSTALLER — testowy kreator partycji i instalator XMB

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

**R2 wymaga dopiero testu na fizycznym DESR-7000.** Najpierw uruchom
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

## Tryby instalatora

- **X — CREATE IF ABSENT + INSTALL:** sprawdza, czy \`PP.DVRISO12\`
  już istnieje. Jeśli tak, wymaga poprawnego montowania PFS i **NIGDY
  nie formatuje istniejącej partycji**. Jeśli jej nie ma, po osobnym
  potwierdzeniu próbuje utworzyć 256 MiB PFS.
  Bezpieczny zakres LBA28 jest dodatkowym warunkiem. Po utworzeniu
  ponownie sprawdza faktyczne położenie partycji przed jej formatowaniem.
  Instalowane pliki są kopiowane przez tymczasowe \`.PDIMNEW\`, odczytywane
  ponownie i weryfikowane CRC. Istniejące pliki o tej samej zawartości
  są pomijane; o innej zawartości **blokują operację bez nadpisywania**.
- **TRIANGLE — REGISTER XMB ENTRY (OPTIONAL):** dopiero po pozytywnej
  weryfikacji wszystkich plików generuje nagłówek PPAA na wybranej
  partycji testowej, wykorzystując sprawdzoną ikonę PDIM i KELF OSDMenu.
  Obcy/nieznany nagłówek powoduje STOP, bez nadpisywania.
- **SQUARE — READ-ONLY VERIFY FILES:** porównuje rozmiary i CRC32 plików
  na HDD z manifestem z USB; niczego nie modyfikuje.
- **O — EXIT:** kończy instalator.

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

Po wpisaniu nagłówka XMB wyłącz konsolę całkowicie, odłącz i
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
