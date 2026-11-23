# PDIM XMB GAME INSTALLER 0.3.2 — szczegółowa diagnostyka EXECUTE.ELF

**Punkt odniesienia: v0.1 HARDWARE PASS.** Na Sony PSX DESR-7000 użytkownik utworzył skrót Killzone, a gra uruchomiła się z XMB. Wcześniej ręczny skrót ICO też działał.

**Wersja 0.3 nie była jeszcze testowana na konsoli.** Bazuje na sprawdzonym v0.1, ale ma dodatkowe kontrole bezpieczeństwa. [Kod wersji v0.1](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/tree/feature/xmb-game-shortcuts/pdim-xmb-game-installer).

## Jak działa?

Skanuje ISO z `dvr_pfs0:/ISO/`, odczytuje nazwy i Game ID z zapisanych przez PDIM 1.2 indeksów, a okładki pobiera **lokalnie** z `PP.DVRISO/ART`. **Nie łączy się sam z Internetem**: brakujące okładki pobieramy wcześniej funkcją HTTPS w działającym PDIM 1.2.

Po wybraniu gry przygotowuje nową partycję APA/PFS `PP.<TYTUŁ>-<GAMEID>` (128 MiB), lokalne `BOOT.ELF` i `BOOT.CNF`, XMB `EXECUTE.ELF` i `EXECUTE.CNF`, metadane oraz dwie grafiki PNG8. Neutrino, moduły DVR i konfiguracja pozostają **wyłącznie na PP.DVRISO**; właściwe ISO pozostaje w oryginalnym obszarze DVR Sony. Uruchamianie gry nie otwiera PDIM.

## Sterowanie

- **GÓRA/DÓŁ, LEWO/PRAWO**: wybór gry / zmiana strony
- **SELECT**: SYSTEM / ENGLISH — język utrwalony w osobnym `BOOT.CNF`
- **SQUARE**: kontrola celu i pliku ISO **bez zapisu**
- **X**: nowa partycja i pliki — zapis APA/PFS
- **TRIANGLE**: rejestracja skrótu w XMB — zapis sektorów APA
- **O**: wyjście

X i TRIANGLE mają dwa etapy potwierdzania. Ostatecznie wymagają **przytrzymania L1+R1 i naciśnięcia X**.

## Poprawka 0.3.2 — dokładna diagnostyka podpisanego KELF

**Wynik na sprzęcie:** Skrót Killzone otwiera grę z XMB, a SQUARE w 0.3.1 raportuje `DIAG: EXECUTE.ELF MISSING/DIFFERENT` i `FAIL -61`. Sam wynik porównania bajtów nie oznacza jeszcze, że instalowany wcześniej, podpisany KELF jest niesprawny.

Nowa diagnostyka uruchamiana **wyłącznie przez SQUARE** rozróżnia:

- `EXECUTE.ELF MISSING (DIR VERIFIED)` — katalog odczytano do końca, a pliku nie znaleziono;
- `DIRECTORY READ ERROR`, `FILE OPEN ERROR`, `FILE READ ERROR` — problemy z odczytem, nie udajemy że plik nie istnieje;
- `SIZE MISMATCH` — rozmiar pliku jest inny niż wzorca z oficjalnego wydania 1.2;
- `CRC MISMATCH` — rozmiar jest taki sam, ale zawartość jest inna.

Ekran drukuje `KELF EXPECT: SIZE=... CRC=...` i przy udanym odczycie `KELF ACTUAL: SIZE=... CRC=...`. To pozwoli porównać wersje bez kopiowania pliku na USB, bez formatu i bez modyfikacji APA/PFS.

**Celowo pozostawiono surową kontrolę identyczności `EXECUTE.ELF` w funkcji instalacyjnej `sg_verify_mounted()`.** Inny, potencjalnie działający KELF nadal powoduje `FAIL -61` przy rygorystycznym porównaniu, ale SQUARE jasno opisze go jako możliwy inny wariant KELF, a nie dowiedzione uszkodzenie. Nie oznacza to automatycznego potwierdzenia jego podpisu ani zgodności na innych konsolach.

Testy komputerowe weryfikują wszystkie przypadki, także częściowe odczyty. **0.3.2 nie została jeszcze przetestowana na konsoli. Testuj na Killzone tylko SQUARE.**

## Poprawka 0.3.1 — diagnostyka istniejących skrótów (Killzone -61)

Wersja 0.1 i wersja 0.3 mają identyczny generator BOOT.CNF oraz identyczny rygorystyczny mechanizm sprawdzania SHORTCUT.ID/CRC. W związku z tym wynik FAIL -61 po prawidłowym uruchomieniu Killzone z XMB nie dowodzi uszkodzenia gry.

1. **SQUARE** sprawdza partycję tylko do odczytu, najpierw z aktualnie wybranym językiem.
2. Gdy kontrola daje -61, próbuje również **drugiego języka** bez zmieniania plików. **PASS (6)** = skrót zainstalowany SYSTEM, **PASS (7)** = skrót zainstalowany ENGLISH. Przy potrzebie dostosowania ustawień należy użyć SELECT, nie reinstalować gry.
3. Jeżeli oba języki nie przechodzą, ekran SQUARE wyświetla osobne komunikaty DIAG dla BOOT.ELF, EXECUTE.ELF, BOOT.CNF, EXECUTE.CNF, res/info.sys, SHORTCUT.ID i okładek PNG8.
4. Kontrola uwzględnia sumy CRC i rozmiary z SHORTCUT.ID, zapisany Game ID/nazwę ISO oraz tryb języka.
5. Wykorzystuje **wyłącznie montowanie PFS READ ONLY**. Działający Killzone i wydanie PDIM HDD 1.2 pozostają nietknięte.

Ta wersja ma jeszcze status **TEST BUILD**: pierwszą próbę wykonaj wyłącznie przyciskiem SQUARE. Nie używaj X ani TRIANGLE na konsoli przed analizą wyniku diagnostyki i zabezpieczeniem kopii sektorowej HDD.

## NOWOŚĆ 0.3: ID gier rozpoznawane kilkoma sposobami

**Katalog gier jest ten sam:** `xcontent/ISO` na Sony DVR odpowiada naszemu `dvr_pfs0:/ISO/`. `pfs1:/` wskazuje na **PP.DVRISO**, gdzie są PDIM, ART i Neutrino. Nie tworzymy dodatkowego katalogu ISO i nie przenosimy obrazów.

**Kolejność rozpoznawania Game ID:**

1. Dokładna nazwa ISO w `PP.DVRISO/COVERS.IDX` (sprawdzone na konsoli).
2. `PP.DVRISO/COVERS.BAK`, jeżeli główny indeks nie zawiera gry.
3. `PP.DVRISO/COVER_SCAN.LOG`, jeżeli nie ma jej w obu indeksach.
4. Jeżeli nie mamy ID, **SQUARE** próbuje odczytać je z aktualnego ISO.

Skaner ISO obsługuje **2048-bajtowe ISO9660**, **2352-bajtowe sektory Mode1/Mode2**, szuka `SYSTEM.CNF` i `BOOT2` albo jednoznacznego identyfikatora w nazwie pliku wykonywalnego. Wszystko wykorzystuje sprawdzone funkcje rozpoznawania ID z PDIM. Jeśli odczyt losowego sektora nie działa, próbuje ograniczonego **odczytu sekwencyjnego** bez `fileXioLseek`.

**Wyniki SQUARE:**

- `PASS (1)` — istniejący skrót i ID potwierdzone w ISO.
- `PASS (2)` — istniejący skrót, ID z indeksu, brak ID wewnątrz ISO.
- `PASS (3)` — nowa partycja może być przygotowana, ID z indeksu.
- `PASS (4)` — istniejący skrót, odczyt sektorów DVR niedostępny, zaufano indeksowi.
- `PASS (5)` — nowy skrót, odczyt sektorów DVR niedostępny, zaufano indeksowi.
- `FAIL -62` — obraz ISO nie daje się nawet otworzyć albo ID znalezione tylko w ISO nie może być ponownie potwierdzone.
- `FAIL -63` — wykryto inne Game ID niż w indeksie.

To **naprawa ogólna, nie wyjątek dla Killzone lub BLACK**. Istniejące gry, okładki i skróty pozostają nietknięte. W przypadku braku ID nadal nie zgadujemy go z nazwy pliku.

**Wersja 0.3 ma na razie tylko testy komputerowe. Przetestuj SQUARE na Killzone, BLACK i kilku innych grach przed korzystaniem z zapisu X lub TRIANGLE.**

## Poprawka 0.2.1 — Killzone i zapasowe rozpoznawanie ID

Instalator **nadal pobiera ID przede wszystkim z `PP.DVRISO/COVERS.IDX`**, tak jak działająca wersja 0.1. Okładka ma nazwę `PP.DVRISO/ART/<Game_ID>_COV.png`. Dodatkowe rozpoznawanie ID wewnątrz ISO działa jako kontrola, opartą na tych samych parserach `SYSTEM.CNF` i plików wykonywalnych, z których korzysta PDIM.

- **PASS (1)** — istniejący skrót został zweryfikowany, a Game ID jest zgodne z ISO.
- **PASS (2)** — istniejący skrót jest poprawny, ale ISO nie ujawniło ID: użyto `COVERS.IDX`.
- **PASS (3)** — można utworzyć nowy skrót, ISO nie ujawniło ID: użyto `COVERS.IDX`, okładka przeszła kontrolę.
- **FAIL -63** — wewnątrz ISO wykryto **inne** Game ID niż w indeksie.
- **FAIL -62** — rzeczywisty problem z odczytem obrazu ISO. Takiej sytuacji nie traktujemy jak nierozpoznanego ID.

Poprzedni FAIL -62 dla Killzone mógł oznaczać brak możliwości rozpoznania ID, a nie nieprawidłową grę. W 0.2.1 te sytuacje są rozdzielone i dodano testy regresji.

**0.2.1 ma PASS kompilacji i testów hostowych, ale nie została jeszcze sprawdzona na DESR-7000.** Zacznij od SQUARE dla istniejącego Killzone, bez używania opcji zapisu X lub TRIANGLE.

## Co poprawiono względem v0.1?

1. Instalator próbuje dodatkowo odczytać **Game ID z wnętrza ISO**. Jeśli wykryje inne ID niż w indeksie, blokuje działanie. Gdy ID nie da się rozpoznać, wykorzystuje sprawdzony wpis z `COVERS.IDX`.
2. Odrzuca niebezpieczne nazwy ISO, których Neutrino FIX95 nie obsługuje, oraz kolizje nazw partycji dwóch gier.
3. Przed utworzeniem nowej partycji w pełni dekoduje obraz PNG i sprawdza, czy jest to poprawna okładka 140×200.
4. Istniejące partycje z inną zawartością nadal **nigdy nie są automatycznie nadpisywane ani formatowane**. Nagłówek XMB jest zapisywany na końcu, po weryfikacji plików.

## Bezpieczeństwo

**Operacje APA mogą uszkodzić system i dane na HDD konsoli Sony PSX.** Wymagana jest pełna, sprawdzona i możliwa do odtworzenia **kopia sektorowa całego dysku**. Nowy ELF nie został przetestowany na fizycznej konsoli. Zalecany pierwszy test to uruchomienie i użycie tylko SQUARE, bez X i TRIANGLE. Nie twórz, nie kasuj i nie formatuj obcych partycji. Nie ma automatycznej naprawy nieudanej instalacji.

Wersja 0.2 nie zawiera jeszcze pobierania metadanych, opisów i screenshotów z PSXDatacenter. Pliki `info.sys` tworzy z nazwy ISO i Game ID, a grafiki PNG8 generuje z istniejącej okładki PDIM. Tę funkcję rozszerzymy osobno.

**Oficjalne wydanie PDIM 1.2 oraz istniejące skróty ICO/Killzone pozostają bez zmian.**
