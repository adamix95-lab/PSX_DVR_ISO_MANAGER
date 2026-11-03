## DIAG T — rzeczywisty wynik na DESR-7000, DIAG U — następny test RO

**DIAG T zaliczony na fizycznym PSX DESR-7000 (3.11.2026)**, dwa zdjęcia użytkownika:

```text
PSX DVR HDD DIAG T / RESULTS
CHECK=0 MOUNT=0
CNF=91 ELF=197396 NEUT=75140
SYS=1 BSD=1 DVD=1 USB=0
ISO=3 DVR=3 PFS=3
TCNF=107 TARGET ELF BYTES=1550452
TCNF PREFIX EXACT. LAST=00 NUL=1
T PROBES ENTRY CWD. NO GAME.
```

Po naciśnięciu X realny oryginalny OSDMenu uruchomił T_TARGET:

```text
PSX DVR HDD DIAG T / TARGET ENTRY
ARGC=3 ARGS_OK=1
ARGV0: hdd0:PP.TESTDVR:pfs:/PSX_DVR_HDD_DIAG_T_TARGET.ELF
CWD: hdd0:PP.TESTDVR:pfs:
fileXioInit=0 PFS0=-19 HDD0=2
RPC PFS CFG=-19 IRX=-19
RPC REL CFG=-19 IRX=-19
POSIX CFG=-1 ER=22
POSIX IRX=-1 ER=22
DUMMY ISO NOT OPENED. NO GAME.
```

**Wniosek potwierdzony sprzętowo:** łańcuch OSDMenu → ELF → argv działa, urządzenie HDD jest widoczne, ale zamontowany w OSDMenu punkt `pfs0:` **nie jest dostępny** w uruchomionym ELF; RPC i POSIX nie potrafią otworzyć `config/system.toml` ani `modules/dvrfile_fhi.irx` przy bieżącym CWD. Te wyniki nie dowodzą, że pliki nie są fizycznie obecne na testowej partycji.

**Potwierdzenie źródłowe:** oryginalny `pcm720/OSDMenu` w `launcher/src/common.c` `deinitPFS()` wywołuje `fileXioUmount(PFS_MOUNTPOINT)`; `utils/loader/src/loader.c` `shutdownDEV9()` *bezwarunkowo* wywołuje `fileXioUmount("pfs0:")`, a `-dev9=NICHDD` zabezpiecza zasilanie DEV9/HDD, ale nie powoduje automatycznego remountu. Nie stwierdzono jeszcze, czy to jedyna przeszkoda w rozruchu rozszerzonego Neutrino pcm720.

**DIAG U** — kontrolowany test możliwości przywrócenia dostępu PFS po handoffie:
- Nowe pliki testowe `PSX_DVR_HDD_DIAG_U_TARGET.ELF` i `DIAG_U.CNF` (107 bajtów) należy skopiować obok istniejących plików do **wyłącznie** `hdd0:PP.TESTDVR`; `PSX_DVR_HDD_DIAG_U_LAUNCH.ELF` uruchamia się **z UDPFS** przez wLE-R3Z. Żadnych innych plików nie nadpisywać. Używana jest tylko alternatywna konfiguracja U, **nie** `BOOT.CNF`.
- U config jest identyczna układem do T, ale wskazuje wyłącznie U_TARGET i celowo nieistniejący `DIAG_U_NO_GAME.iso`; ostatni argument `-dev9=NICHDD` przechwytuje OSDMenu.
- U LAUNCH przeprowadza fizycznie sprawdzoną preflight R3/T: tylko `FIO_MT_RDONLY`, ścisła kontrola pliku CNF (prefix zgodny byte-for-byte, ostatni LF lub NUL), sprawdzenie targetu, izolowanego HDD, znanej konfiguracji i dostępności katalogu ISO, następnie świadome **X**, lub START anulowanie.
- U TARGET najpierw wykonuje **identyczne sondowanie stanu wejścia jak T przed swoim IOP reset**. Wyłącznie jeśli istnieje `hdd0:` i brak dostępu do `pfs0:`, próbuje `fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)`. Jeśli montowanie uda się, sprawdza `pfs0:/`, otwiera **tylko do odczytu** `pfs0:/config/system.toml`, `pfs0:/modules/dvrfile_fhi.irx`, próbuje `chdir("pfs0:/")` i POSIX `open(O_RDONLY)` na odpowiednich ścieżkach względnych. Otwierane deskryptory są zamykane, własny mount odmontowuje przed swoim IOP reset, który służy wyłącznie do wyświetlenia zachowanych wyników i uruchomienia padu.
- Końcowy ekran `PSX DVR HDD DIAG U / RO REMOUNT` zawiera `MOUNT_RO`, `PFS_AFTER`, `RPC CONFIG`, `DVR IRX`, `CHDIR`, `NEW CWD`, `POSIX CFG/IRX ERRNO`, `UMOUNT OWN`. Jeśli zawiesi się wcześniej, sfotografować ostatni marker `U... BEGIN`.

**Bezpieczniki:** brak otwierania rzeczywistego ISO, brak Neutrino i gry, brak zapisu na HDD w kodzie U, montowanie wyłącznie `PP.TESTDVR` w RO, bez kolizji z istniejącym `pfs0`, brak zmian na USB v0.8, stabilnym `main`, `BOOT.CNF` i systemowych partycjach Sony. Wywołania istniejących obcych sterowników IOP mogą mieć efekty wewnętrzne, dlatego zabezpieczyć ważne dane przed eksperymentami.

---

## DIAG T — dostęp do config/modules w chwili wejścia do ELF z OSDMenu

Po kopiowaniu `config/` i `modules/` na `PP.TESTDVR` i przesunięciu `arg=-dev9=NICHDD` na koniec `BOOT.CNF`, użytkownik nadal widzi czarny ekran i powrót do XMB bez dawnego `-19`. To nie dowodzi uruchomienia Neutrino ani zachowania dostępu do HDD.

**Cel T:** przez prawdziwy OSDMenu uruchomić nieszkodliwy `T_TARGET.ELF`, a **przed jakimkolwiek resetem IOP w tym celu testowym** sprawdzić katalog roboczy oraz dostęp do `config/system.toml` i `modules/dvrfile_fhi.irx` (zarówno poprzez `pfs0:/`, jak i ścieżki względne). Nie uruchamiamy Neutrino ani gry. `-dev9=NICHDD` jest w oddzielnym T CNF na ostatniej pozycji, więc OSDMenu powinien je skonsumować i nie przekazywać do T_TARGET. Skan jest wyłącznie do odczytu.

Przygotowane **trzy nowe pliki** (nie nadpisywać starych):
- `PSX_DVR_HDD_DIAG_T_TARGET.ELF` — skopiować do katalogu głównego `hdd0:PP.TESTDVR` (obok oryginalnego BOOT.ELF);
- `DIAG_T.CNF` — skopiować do tego samego katalogu, dokładne 107 bajtów, treść:
```ini
boot=PSX_DVR_HDD_DIAG_T_TARGET.ELF
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/DIAG_T_NO_GAME.iso
arg=-dev9=NICHDD
```
- `PSX_DVR_HDD_DIAG_T_LAUNCH.ELF` — uruchomić **wyłącznie przez UDPFS w wLE-R3Z**, nie kopiować go na HDD.

T LAUNCH używa sprawdzonego R3 preflight `FIO_MT_RDONLY`: wymaga pliku T CNF o **dokładnej treści**, z wyjątkiem tolerancji tylko ostatniego LF lub NUL (sprzęt R2 tak odczytywał); sprawdza rozmiar i nagłówek T TARGET oraz podstawową konfigurację oryginalnego `BOOT.CNF`, katalog ISO i obecność Neutrino. Na pierwszym ekranie po `CHECK=0` należy **zrobić zdjęcie** i dopiero świadomie nacisnąć **X**. START przerywa test. `CHECK != 0` oznacza brak zgody na start. OSDMenu dostaje **wyłącznie alternatywną `DIAG_T.CNF`**, nie zmienia oryginalnego `BOOT.CNF`.

T TARGET bezpośrednio po przekazaniu sterowania:
1. Wyświetla `T0 TARGET ENTRY BEFORE IOP RESET`, wywołuje `SifInitRpc(0)`, próbuje `getcwd` i `fileXioInit`.
2. Próbuje `fileXioDopen("pfs0:/")` i `fileXioDopen("hdd0:")`, następnie RO open na `pfs0:/config/system.toml`, `pfs0:/modules/dvrfile_fhi.irx`, `config/system.toml`, `modules/dvrfile_fhi.irx` i analogiczne POSIX `open(O_RDONLY)`. Każda operacja ma marker BEGIN, więc w przypadku zawieszenia można zidentyfikować fazę na zdjęciu. `fileXioInit` może zawiesić się, jeśli moduł nie przetrwał handoffu; to obserwacja techniczna, nie automatyczny błąd programu.
3. **Dopiero po zapisaniu wyników do pamięci EE** resetuje IOP w sposób sprawdzony w DIAG R_ARG, ładuje moduły pad i pokazuje końcowy ekran, z START → XMB. Zapisów na HDD nie wykonuje.

**Interpretacja:** `ARGS_OK=1` potwierdza obsługę ostatniego `-dev9=NICHDD` przez OSDMenu. Negatywne wyniki `PFS0`, `RPC REL` i `POSIX` wskazują brak dostępu w danym środowisku, **nie** dowodzą braku skopiowanych plików. Jeśli wejściowe RPC zawiesi się, ostatni marker `T... BEGIN` pomoże zawęzić problem. Sprawdzenie `cwd` może zwrócić `(getcwd failed)` z powodów zależnych od libc.

Ograniczenie: docelowy program sam w końcu resetuje IOP, więc końcowy ekran pokazuje **zachowane wyniki sprzed resetu**, nie stan środowiska po nim. W testach nie wykonujemy żadnego ISO; ścieżka `DIAG_T_NO_GAME.iso` jest fikcyjna. Konfiguracja i moduły po stronie T nie muszą jeszcze działać jak w Neutrino. Reakcje obcych sterowników OSDMenu/IOP pozostają poza kontrolą testu — wykonać kopię ważnych danych. `main`, działający USB v0.8, partycje Sony i rzeczywiste obrazy ISO pozostają nietknięte; zmiany wyłącznie na gałęzi eksperymentalnej, PR #26 Draft.

---

## Stały kontekst projektu — źródła Neutrino, rozszerzenie DVR i powód używania USB

**Źródła, do których mamy dostęp w GitHub (zawsze sprawdzać oba przed wnioskami o DVR):**
- Główny projekt: https://github.com/ps2max32/neutrino
- Rozszerzenie pcm720, wprowadzenie dodatkowych modułów i obsługi PSX DVR (otwarty PR #228): https://github.com/ps2max32/neutrino/pull/228
- Gałąź z kodem autora PR: https://github.com/pcm720/neutrino/tree/bsd-dvr

**Ważne doprecyzowanie użytkownika (3.11.2026):** opisujemy pracę pcm720 przede wszystkim jako **dodanie nowych modułów i backendu obsługi DVR do Neutrino**, a nie jako ogólnikowe „patchowanie uruchamiania” lub naprawienie Neutrino. Podczas startu przez OSDLauncher/OSDMenu przekazywany jest kontekst/argumenty wskazujące obsługę DVR oraz obraz ISO, np. `-bsd=dvr` i `-dvd=dvr_pfs0:/ISO/...`. **Działanie z USB było oryginalnym obejściem**: już na początku projektu PSX DVR ISO MANAGER nie udało się uruchomić połączenia OSDLauncher + Neutrino + moduły DVR wyłącznie z HDD. Dlatego został opracowany wariant działający z pamięci masowej USB. Naszym celem jest **usunąć tę zależność od USB**, zachowując wersję stabilną v0.8.

Fizyczny DIAG Q (bez `-dev9=NICHDD`) wykonał benigny DIAG F z HDD przez rzeczywisty OSDMenu, a R3 uruchomił benigny `R_ARG.ELF` i przekazał `ARGC=3 ARGS_OK=1`. Te dowody **nie** potwierdzają, że pełne Neutrino z dodatkowymi modułami DVR może zostać poprawnie zainicjalizowane z HDD; w szczególności należy sprawdzić dostęp do modułów, konfigurację i ścieżki po handoffie, przejście IOP/DEV9 i fazę uruchamiania obrazu ISO. DIAG S nie jest obowiązkowym następnym krokiem; brak kandydata TOML na `pfs1:` sam nie przesądza o błędzie.

**Dalsze prace:** porównywać dokładnie działającą sekwencję z USB i niedziałającą z HDD w OSDLauncher oraz forku pcm720. Nie traktować problemu jako początku od zera: mamy potwierdzone oddzielne segmenty łańcucha (loader i argv), ale nie kompletny start Neutrino. Nie modyfikować `main`, v0.8, Sony systemowych partycji i działających `BOOT.CNF` bez kontrolowanego eksperymentu. PR #26 pozostaje Draft.

---

## AKTUALIZACJA 2026-11-03 — DIAG S nie jest wymagany, punktem odniesienia jest patch pcm720 (Neutrino PR #228)

Użytkownik potwierdził, że używa **forka Neutrino z PR [ps2max32/neutrino#228](https://github.com/ps2max32/neutrino/pull/228)** autorstwa **pcm720**, a **nie** obecnego niezmodyfikowanego upstream. PR #228 jest otwarty i nie został zmergowany. Poprzednie rozumowanie „Neutrino nie obsługuje DVR” nie dotyczy naszej wersji i nie może być podstawą do diagnozy. **Nie wymagamy uruchomienia DIAG S**; pozostaje opcjonalnym narzędziem do read-only obserwacji (np. PVD ISO), lecz jego sondowanie układu plików `config/*.toml` względem `pfs1:/` nie dowodzi braków konfiguracji, bo rzeczywisty katalog roboczy Neutrino po OSDLauncher jest nieznany.

Zweryfikowane w źródłach PR #228 (head `pcm720/neutrino:bsd-dvr`): `ee/loader/config/bsd-dvr.toml` ładuje `iomanX.irx`, `fileXio.irx`, `dvrdrv.irx` (LE+EE), `dvrfile.irx` (LE), `dvrfile_fhi.irx` (EE), zależność `i_dev9_ns`. `ee/loader/src/config.c` mapuje prefiks `dvr_pfs` na BSD `dvr`. `ee/loader/src/main.c` przed uruchomieniem środowiska LE dla `bsd=dvr` używa `SifIopReset("rom0:UDNL rom0:OSDCNF", 0)`, `sceCdInit(SCECdINoD)`, `sceCdApplySCmd(0x29,...)`, `sceCdInit(SCECdEXIT)`, a następnie kolejnego resetu IOP i ładowania modułów. PR opisuje realne testy Ridge Racer V oraz Gran Turismo 4 z `dvr_pfs0:`.

R3 udowodnił łańcuch prawdziwy OSDMenu → `R_ARG` z `ARGC=3 ARGS_OK=1`; **nie** udowodnił działania patcha w Neutrino. Następny rozsądny test to instrumentacja **tej konkretnej gałęzi pcm720** (lub jej testowa kopia) z etapowymi markerami startu: odczyt TOML i modułów, zmiana trybu DVR/IOP, `dvrfile_fhi`, otwarcie/PVD ISO, aż do ładowania docelowego ELF. Osobno należy ustalić rzeczywisty cwd po OSDLauncher. **Bez wprowadzania zmian na Sony partycjach systemowych i bez uruchamiania gry w pierwszej próbie.** `main` i v0.8 pozostają bez zmian.

---

## DIAG S — sprawdzenie warunków startu Neutrino bez jego uruchamiania

**Stan sprzętowy:** DIAG R3 przeszło na fizycznym DESR-7000: prawdziwy OSDMenu uruchomił `PSX_DVR_HDD_DIAG_R_ARG.ELF` z `PP.TESTDVR` z `ARGC=3 ARGS_OK=1`, `ARGV1=-bsd=dvr` oraz `ARGV2=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso`. DIAG J wcześniej zwracał do XMB podczas próby uruchomienia `neutrino.elf` ze wskazanym rzeczywistym ISO. **S jest nowym testem; nie był jeszcze sprawdzony na konsoli.**

### Dlaczego taki zakres

Obecny kod źródłowy upstream `ps2max32/neutrino`, `ee/loader/src/main.c`, sprawdza `rom0:ROMVER` (wymaga przynajmniej 15 bajtów), otwiera `config/system.toml` lub posługuje się alternatywnym układem płaskim, czyta konfigurację `compat.toml`, a przy `-bsd=dvr` próbuje wczytać konfigurację `bsd-dvr.toml`. Kod `ee/loader/src/config.c` tworzy nazwy `config/bsd-<nazwa>.toml` i `config/emu-dvd-file.toml`; `modlist.c` czyta moduły spod `modules/`. Standardowa lista driverów upstream **nie wymienia `dvr`**, ale binarny `neutrino.elf` na `PP.TESTDVR` może pochodzić z innej wersji lub forka. Nie znamy też rzeczywistego katalogu roboczego Neutrino po załadowaniu z OSDMenu. **Nie twierdzimy, że brak któregoś pliku na testowej partycji dowodzi błędu Neutrino** — to tylko diagnostyka kandydatów.

### Co robi DIAG S

`PSX_DVR_HDD_DIAG_S.ELF` jest jednym programem uruchamianym **bezpośrednio przez UDPFS z wLE-R3Z**, bez kopiowania niczego nowego na `PP.TESTDVR`. Używa fizycznie sprawdzonego z DIAG R stosu APA/PFS/DVR i montuje **wyłącznie własną partycję** `hdd0:PP.TESTDVR` jako `pfs1:` tylko do odczytu. Działa wyłącznie diagnostycznie:

1. Odczytuje istniejący `BOOT.CNF`, sprawdza nagłówek `neutrino.elf` (ELF32, little-endian, MIPS) i `rom0:ROMVER` (liczbę odczytanych bajtów).
2. Bez tworzenia plików sprawdza **potencjalne** pliki konfiguracyjne w `pfs1:/config/` i w wariancie płaskim: `system.toml`, `bsd-dvr.toml`, `compat.toml`, `emu-dvd-file.toml`, oraz katalog `modules/`. Wartości ujemne wskazują brak dostępu w **tym katalogu**, a nie automatycznie przyczynę awarii Neutrino.
3. Inicjalizuje moduły DVR jak poprzednie testy, wybiera rzeczywisty ISO z dotychczasowego `BOOT.CNF` (nie wpisuje nowego ani nie używa fikcyjnego `DIAG_R_NO_GAME.iso`), sprawdza odczyt **tylko 6 bajtów** przy sektorze 16 ISO9660: typ deskriptora 01 i `CD001`. Zapisuje `ISO FD`, `ISO SEEK`, `ISO READ` i `PVD` (1: nagłówek pasuje, 0: inny, ujemne: błąd odczytu lub brak seeku). Nie czyta danych gry dalej niż wymagane 6 bajtów.
4. Na ekranie `PSX DVR HDD DIAG S / RESULTS` pokazuje `CHECK/MOUNT`, `CNF/OSD/NEUT`, `ROMVER/MIPS`, `SYS/BSD DVR/COMPAT/DVD CFG`, `MODULES DIR`, `ISO FD/PVD/SEEK/READ`. **`CHECK=0` oznacza, że wstępna diagnostyka zakończyła się, nie że Neutrino działa!** Nawet przy błędach konfiguracji i PVD program nadal pokazuje wyniki.
5. Po zdjęciu wystarczy START do powrotu do XMB, tak jak w dotychczasowych diagnostykach. **Nie ma przycisku X ani funkcji loadera**, więc S nie uruchamia OSDMenu, Neutrino ani żadnej gry.

### Instrukcja sprzętowa i granice

1. Wcześniejsze pliki pozostają nienaruszone: `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, `DIAG_Q.CNF`, `DIAG_R.CNF`, R_ARG, ISO. Niczego nie trzeba kopiować na HDD.
2. Po udanym CI pobierz `PSX_DVR_HDD_DIAG_S.ELF` i uruchom **z UDPFS** w wLE-R3Z. Zrób zdjęcie końcowego ekranu, szczególnie wartości `ROMVER`, `SYS CFG`, `BSD DVR CFG`, `MODULES DIR`, `ISO PVD`. Jeśli zatrzyma się na `S... BEFORE`, sfotografuj ostatnią linijkę.
3. Nie próbuj instalować nowej konfiguracji Neutrino ani nowych sterowników na podstawie domysłów. Po uzyskaniu wyników ustalimy, czy problemem jest niewidoczna konfiguracja/drivery, nagłówek ISO lub następny etap działania samego Neutrino.
4. Kod S nie wykonuje zapisów ani nie montuje żadnych partycji RW. **Nie możemy gwarantować braku wewnętrznych operacji zewnętrznych sterowników IOP**, więc zalecana jest kopia ważnych danych. Nie dotykamy partycji systemowych Sony i nie zmieniamy `main`, v0.8 ani XMB. PR #26 pozostaje Draft.

---

## DIAG R3 — pełny PASS z ekranu docelowego na fizycznym DESR-7000 (3.11.2026)

Użytkownik uruchomił `PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF` i po naciśnięciu X przesłał zdjęcie **końcowego ekranu R_ARG**. Nie wykonał zdjęcia ekranu preflight przed X, więc wartości `CHECK`, `RCNF` i `LAST` z tego ekranu nie są odrębnie potwierdzone fotograficznie. Widoczny wynik:

```text
PSX DVR HDD DIAG R / ARG RESULTS
ARGC=3 ARGS_OK=1
ARGV0: hdd0:PP.TESTDVR:pfs:/PSX_DVR_HDD_DIAG_R_ARG.ELF
ARGV1: -bsd=dvr
ARGV2: -dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso
DUMMY ISO PATH - NOT OPENED.
START: EXIT TO XMB/OSD
PHOTO RESULTS.
```

**Wniosek:** realny łańcuch OSDMenu Quickboot → handlePFS → loader wykonał na Sony PSX DESR-7000 benigny `R_ARG.ELF` z `PP.TESTDVR` i przekazał **oba argumenty poprawnie** (`ARGC=3 ARGS_OK=1`), bez jawnej flagi `-dev9=NICHDD`. Wcześniejszy problem preflight R/R2 (`RCNF=-344` przez końcowe `00` zamiast `0A`) został ominięty precyzyjnym R3; nie wiemy, skąd bierze się ten bajt. Potwierdzenie nie dowodzi startu Neutrino ani poprawnego dostępu do DVR/ISO: ścieżka ISO w R_ARG była celowo fikcyjna i **nie została otwarta**. Wcześniejszy DIAG J nadal zwracał do XMB. Następny etap powinien rozdzielać inicjalizację Neutrino od jego argumentów i odczytu rzeczywistego ISO, bez nadpisywania dotychczasowych konfiguracji.

Zmiany wyłącznie na gałęzi eksperymentalnej; `main`, v0.8 i partycje systemowe Sony pozostają bez zmian. PR #26 pozostaje Draft.

---

## DIAG R2 — sprzętowa diagnoza, wynik 3.11.2026

Zdjęcie użytkownika: `CHECK=-136 MOUNT=0 RCNF=-344`; `SIZE=87 EXPECT=87 READ=87`; `DIFF OFFSET=86 E=0A A=00`; `CR=0 LF=2 NUL=1`; sumy `7111/7101`, ostatnie bajty `E: 69 73 6F 0A`, `A: 69 73 6F 00`. Zawartość jest identyczna poza ostatnim bajtem, który przy odczycie jest NUL zamiast LF. Nie znamy jeszcze źródła zamiany.

### DIAG R3 — poprawiony launcher bez kolejnych kopii na HDD

Po zielonym CI uruchom **tylko `PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF` przez UDPFS**. Nie kopiuj żadnego pliku na HDD. Nowy launcher porównuje dokładnie 86 pierwszych bajtów `DIAG_R.CNF` i wymaga rozmiaru/odczytu 87 bajtów. Ostatni bajt może być **tylko 0A (LF) albo 00 (NUL)**. Nie dopuszcza innych zmian treści ani nazw programu/argumentów.

Jeśli pierwszy ekran pokaże `CHECK=0`, `RCNF=87`, `LAST=00 NUL=1`, zrób zdjęcie i świadomie naciśnij X. Oryginalny `BOOT.ELF` powinien uruchomić jedynie nieszkodliwy `PSX_DVR_HDD_DIAG_R_ARG.ELF`, a on pokazać `ARGC=3 ARGS_OK=1` oraz `-bsd=dvr`, `-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso`. Fikcyjny obraz ISO nie jest otwierany przez program docelowy. Przy błędzie nie naciskaj X. Daj zdjęcie końcowego ekranu albo opisz powrót do XMB.

`BOOT.CNF`, `BOOT.ELF`, `DIAG_Q.CNF`, `DIAG_R.CNF`, `R_ARG`, Neutrino, gry, Sony partycje systemowe, `main` i v0.8 pozostają bez zmian. Test nie rozstrzyga pochodzenia końcowego NUL, a oryginalny OSDMenu i zewnętrzne sterowniki mogą wykonywać własne operacje; zalecana kopia ważnych danych. PR #26 pozostaje Draft.

---

## DIAG R2 — powtarzalny błąd `RCNF=-344` znaleziony na ostatnim bajcie (zdjęcie 3.11.2026)

Użytkownik wykonał R2 przez UDPFS i przesłał zdjęcie:

```text
PSX DVR DIAG R2 / CNF BYTES
CHECK=-136 MOUNT=0 RCNF=-344
SIZE=87 EXPECT=87 READ=87
DIFF OFFSET=86 E=0A A=00
CR=0 LF=2 NUL=1
SUM EXPECT=7111 READ=7101
WINDOW AT 83 (UP TO 8 BYTES):
E: 69 73 6F 0A
A: 69 73 6F 00
```

**Potwierdzenie:** odczytane 87 bajtów zgadza się ze wzorcem aż do indeksu 85 włącznie. **Jedyna różnica to ostatni bajt 86 (0x0A LF zmienione w 0x00 NUL)**. Nie wiemy, czy powstaje podczas zapisu, kopiowania czy odczytu przez stos PS2. Ponowne kopiowanie oryginału nie pomogło.

## DIAG R3 — ograniczona poprawka, bez zmian plików HDD

Zamiast edytować `DIAG_R.CNF`, przygotowano **tylko nowy `PSX_DVR_HDD_DIAG_R3_LAUNCH.ELF`**, do uruchomienia wyłącznie przez UDPFS w wLE-R3Z. Działające `BOOT.CNF`, `BOOT.ELF`, `DIAG_Q.CNF`, `DIAG_R.CNF`, `PSX_DVR_HDD_DIAG_R_ARG.ELF`, `neutrino.elf` i ISO pozostają nienaruszone. Po inicjalizacji IOP i weryfikacji wyłącznie do odczytu odblokowuje X tylko wtedy, gdy:

- plik `pfs1:/DIAG_R.CNF` ma **dokładnie 87 bajtów** i wszystkie zostały odczytane,
- pierwsze **86 bajtów są identyczne** z bezpiecznym wzorcem (`boot=PSX_DVR_HDD_DIAG_R_ARG.ELF`, `arg=-bsd=dvr`, fikcyjny `arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso`),
- ostatni bajt to **wyłącznie 0x0A albo 0x00**, a nie dowolna inna wartość,
- pozostałe preflight APA/DVR/ELF nadal przechodzą.

Na ekranie pierwszym oczekujemy `RCNF=87 CHECK=0` i `LAST=00 NUL=1`. Zrób zdjęcie. Przy `CHECK=0` naciśnij **X** świadomie, aby przekazać sterowanie oryginalnemu `pfs1:/BOOT.ELF` z argumentem `hdd0:PP.TESTDVR/DIAG_R.CNF`. OSDMenu powinien uruchomić wyłącznie dotychczasowy, niewykonujący HDD IO `PSX_DVR_HDD_DIAG_R_ARG.ELF`. Docelowy ekran powinien pokazać `PSX DVR HDD DIAG R / ARG RESULTS`, `ARGC=3 ARGS_OK=1`, `ARGV1=-bsd=dvr`, `ARGV2=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso`.

**Ograniczenie:** samo dopuszczenie końcowego NUL nie dowodzi jego pochodzenia; OSDMenu powinien parsować ostatnią linię także bez końcowego LF, ale dopiero test na konsoli to zweryfikuje. Sam R3 w kodzie aplikacji nie zapisuje na dysk, lecz jego oryginalny OSDMenu i sterowniki mogą mieć inne operacje wewnętrzne. Zachowaj kopię ważnych danych. START to zwykłe wyjście, bez osobnego testowania. `main`, v0.8 i systemowe partycje Sony są nietknięte, PR #26 zostaje Draft.

---

## DIAG R2 — rozpoznanie powtarzalnego RCNF=-344 (3 listopada 2026)

Użytkownik zgłosił **ten sam błąd po ponownym skopiowaniu oryginalnego DIAG_R.CNF**. Na pierwszym zdjęciu: `CHECK=-136 MOUNT=0`, `RCNF=-344`, `ARG ELF BYTES=1525188`; pozostałe preflight APA/DVR/PFS są OK. Kod wskazuje, że `-344` powstaje po udanym odczytaniu 87 bajtów `DIAG_R.CNF`, gdy `memcmp` wykrywa różnicę od wzorca. Nie należy ponawiać ślepo kopiowania ani uznawać problemu za błąd Neutrino.

**Nowy test DIAG R2:** tylko `PSX_DVR_HDD_DIAG_R2_CNF.ELF` uruchamiany z **UDPFS** w wLE-R3Z. **Nic nowego nie kopiować na HDD**. Istniejący `DIAG_R.CNF`, `R_ARG.ELF`, `BOOT.CNF`, `BOOT.ELF`, `DIAG_Q.CNF` pozostają bez zmian. Program uruchamia znaną inicjalizację, montuje wyłącznie własną partycję `PP.TESTDVR` jako `pfs1:` RO, odczytuje `DIAG_R.CNF` i wyświetla:

- `SIZE` (rozmiar z HDD), `EXPECT` (87), `READ` (faktycznie odczytane bajty),
- `DIFF OFFSET` (pierwsza różnica bajtowa, `-1` oznacza zgodność), `E` (hex oczekiwany), `A` (hex faktyczny),
- `CR`, `LF`, `NUL` z zawartości, sumy kontrolne (proste sumy bajtów), osiem bajtów HEX wokół różnicy (`E:` i `A:`),
- `RCNF`, `CHECK`. Jeśli rozmiar i odczyt są OK, ale `DIFF OFFSET` wskazuje niezgodność, porównamy dokładny fragment. Zero-jedynkowy test `memcmp` zastąpiono jawną pętlą porównania bajtów.

**Brak możliwości uruchomienia docelowego ELF:** kod R2 nie zawiera wywołania `LoadELFFromFile` ani innego loadera i zawsze blokuje X, nawet przy `CHECK=0`; START tylko wychodzi do XMB. W razie błędu użytkownik ma wykonać zdjęcie ekranu, nie edytować plików i nie testować osobno START. Nadal nie potwierdzono przekazania argumentów z OSDMenu, bo R nie wyszedł poza precheck.

Nawet gdy kod aplikacyjny używa tylko `FIO_MT_RDONLY`, sterowniki IOP mogą mieć wewnętrzne zachowania; zachować kopię danych. Żadnych zmian w `main`, stabilnym v0.8, systemowych partycjach, ani w pierwotnym DIAG R. PR pozostaje Draft.

---

## DIAG R — rzeczywisty OSDMenu i przekazanie argumentów, BEZ Neutrino/gry

**Warunek wejściowy:** użytkownik potwierdził, że oryginalny `PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF` z `hdd0:PP.TESTDVR/DIAG_Q.CNF` uruchomił DIAG F. `DIAG_Q.CNF` ma dokładnie 28 bajtów i nie zawiera `-dev9=NICHDD`. W DIAG J uruchamianie Neutrino z `BOOT.CNF` doprowadziło do powrotu do XMB. Nie wiemy jeszcze, czy problem zależy od argumentów, trybu wyłączenia DEV9 czy samego Neutrino.

**DIAG R ma rozdzielić te kwestie bez wykonywania Neutrino.** Oryginalny `BOOT.ELF` OSDMenu zostaje nienaruszony; jego prawdziwy Quickboot → handlePFS → embedded loader wczytuje nowy, nieszkodliwy `PSX_DVR_HDD_DIAG_R_ARG.ELF` z własnej `PP.TESTDVR`. R_ARG wykorzystuje stos z fizycznie sprawdzonego DIAG F, **nie ładuje żadnych modułów HDD/DVR**, nie otwiera obrazu ani partycji, wyświetla tylko `argc/argv[0..2]`, `ARGS_OK` i pozwala wrócić przez START.

### Pliki DIAG R (nie mylić z DIAG_Q.CNF)

1. `PSX_DVR_HDD_DIAG_R_ARG.ELF` — **skopiować do głównego katalogu `PP.TESTDVR`**. Jest to program docelowy (nie launcher UDPFS).
2. `DIAG_R.CNF` — **również skopiować do głównego katalogu `PP.TESTDVR`**. To nowa, osobna konfiguracja.
3. `PSX_DVR_HDD_DIAG_R_LAUNCH.ELF` — **uruchomić przez UDPFS** w wLE-R3Z (lub USB), nie kopiować na HDD.

Zawartość testowego pliku `DIAG_R.CNF` dokładnie 87 bajtów, LF:
```text
boot=PSX_DVR_HDD_DIAG_R_ARG.ELF
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso
```

**`DIAG_R_NO_GAME.iso` jest celowo fikcyjną nazwą.** Nie jest i nie musi być obecny na HDD. OSDMenu w tym kroku tylko przekazuje argument `-dvd` do R_ARG, który wyświetla go i **nigdy nie otwiera**. `-bsd=dvr` również jest jedynie tekstem przekazanym do naszego programu. Tym testem badamy transport argumentów, a nie funkcje DVR Neutrino ani faktyczny odczyt ISO. Podobnie jak w pierwotnym Q **nie ustawiamy `-dev9=NICHDD`**.

### Plan na konsoli

1. Zachowaj kopię danych i nie edytuj `BOOT.CNF`, `BOOT.ELF`, `DIAG_Q.CNF`, `neutrino.elf`, `PSX_DVR_HDD_DIAG_F.ELF`, gier, partycji systemowych Sony ani XMB. PR #26 pozostaje eksperymentalny, `main` i v0.8 nietknięte.
2. Po **zielonym GitHub Actions** pobierz paczkę `PSX-DVR-HDD-DIAG-R-OSDMENU-ARGV-ONLY`. Skopiuj na **własną** `PP.TESTDVR` tylko R_ARG.ELF oraz `DIAG_R.CNF`.
3. Uruchom przez UDPFS `PSX_DVR_HDD_DIAG_R_LAUNCH.ELF`. Tak jak oryginalny Q: montuje `PP.TESTDVR` jako `pfs1:` **tylko do odczytu**, sprawdza istniejące `BOOT.CNF` (bez zmian), pliki startowe, wybrane ISO, R_ARG oraz **dokładną zawartość `DIAG_R.CNF`**. Dopiero przy `CHECK=0` możesz świadomie nacisnąć X. Przy błędzie X będzie wyłączony; zrób zdjęcie.
4. Po X realny OSDMenu powinien załadować R_ARG z HDD. Na ekranie `PSX DVR HDD DIAG R / ARG RESULTS` oczekujemy `ARGC=3 ARGS_OK=1`, `ARGV1=-bsd=dvr` oraz `ARGV2=-dvd=dvr_pfs0:/ISO/DIAG_R_NO_GAME.iso`. Zrób zdjęcie. START to zwykłe wyjście do XMB (nie testujemy już osobno działania START).
5. W przypadku powrotu do XMB bez ekranu R_ARG, błędu OSDMenu lub zawieszenia, zapisz ostatni ekran/komunikat. W sytuacji zablokowania IOP może być konieczny restart konsoli.

**Ograniczenia:** to pierwszy eksperyment z prawdziwym loaderem przekazującym dwa argumenty do nowego ELF, ale R_ARG nie jest Neutrino i nie otwiera ISO. Nawet pełny PASS nie wykaże, czy same moduły Neutrino i jego DVR `-dvd` startują poprawnie na DESR. Oryginalny OSDMenu może wykonać własną sekwencję DEV9 / HDD (nie gwarantujemy braku wewnętrznych zapisów sterowników), choć nasz kod diagnostyczny nie zawiera zapisu. Jeżeli R przejdzie, następny etap to diagnostyka inicjalizacji Neutrino bez launchu gry lub kontrolowany, świadomy start przy realnym ISO.

---

## DIAG Q — korekta identyfikacji wariantu, fizyczny PASS na DESR-7000 (3.11.2026)

**Użytkownik wyjaśnił:** wykonał próbę z pierwszym wariantem **`PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF` + `DIAG_Q.CNF`**, a **DIAG F rzeczywiście wystartował**. NIE używał dodatkowego wariantu `PSX_DVR_HDD_DIAG_Q.ELF` + `BOOTQ.CNF`. Poprzedni wpis w PR i README, który przypisał ten wynik konfiguracji `BOOTQ.CNF` z `-dev9=NICHDD`, był błędny i zostaje jednoznacznie sprostowany.

Zawartość sprawdzonego w repo `hdd-diagnostics/diag-q/DIAG_Q.CNF` (28 bajtów):
```text
boot=PSX_DVR_HDD_DIAG_F.ELF
```

**Nie ma flagi `-dev9=NICHDD`.** Pierwotny launcher Q wskazuje `hdd0:PP.TESTDVR/DIAG_Q.CNF` i uruchamia istniejący oryginalny `pfs1:/BOOT.ELF`. Zgłoszony start DIAG F potwierdza, według relacji użytkownika, że oryginalny OSDMenu i jego loader potrafią wykonać samodzielny ELF z `PP.TESTDVR` **także bez jawnego `-dev9=NICHDD`**. Nie dostarczono zdjęcia tego konkretnego uruchomienia, wynik oparty na relacji użytkownika. To nie dowodzi uruchamiania Neutrino ani gry.

**Konsekwencje:** plan DIAG R polegający tylko na usunięciu `-dev9=NICHDD` jest zbędny: pierwotny DIAG Q wykonał już test bez tej flagi. Powrót DIAG J do XMB nadal wymaga wyjaśnienia; bardziej uzasadnione jest teraz zbadanie różnic między uruchamianiem sprawdzonego F a Neutrino (argumenty `-bsd=dvr`, `-dvd`, inicjalizacja DVR, ścieżki i zachowanie samego Neutrino), etapowo i bez nadpisywania działających plików.

**Odrębny wariant** `BOOTQ.CNF` z `-dev9=NICHDD` istnieje w repo, ale **nie jest testem fizycznie potwierdzonym**. `main`, v0.8 i systemowe partycje Sony pozostają bez zmian; PR #26 pozostaje Draft.

---

# DIAG Q — prawdziwy OSDMenu → prawdziwy loader → bezpieczny DIAG F

**Stan:** fizyczny DESR-7000: DIAG P pełny PASS (`RESULT=0 STAGE=64`, `RESET2=1 MOD2=1 PAD2=1`, `UM1=0 REMOUNT=0 TRYFD=3 UM2=0 PATH=1`). **Oryginalny BOOT.ELF uruchamiany przez DIAG J po naciśnięciu X wrócił do XMB, bez zaobserwowanego Neutrino.** DIAG Q jest **nieprzetestowany sprzętowo**.

## Dlaczego tylko jeden nowy ELF?

Wcześniejsze testy M–P odtwarzały fragmenty OSDMenu, ale nie jego własny, osadzony loader. Kod upstream `pcm720/OSDMenu` (`launcher/src/handler_quickboot.c`, `launcher/src/handler_pfs.c`, `common/src/loader.c` i `utils/loader/src/loader.c`) pokazuje, że `handleQuickboot()` odczytuje podaną konfigurację, `handlePFS()` przygotowuje ścieżkę `:pfs:`, a następnie rzeczywisty loader montuje partycję i próbuje wykonać docelowy ELF.

DIAG Q używa **istniejącego na PP.TESTDVR oryginalnego `BOOT.ELF`** z DIAG H–J; nie tworzymy jego kopii ani zmienionej wersji. Zamieniamy wyłącznie argument CNF i cel testowy: nie uruchamiamy Neutrino, tylko sprawdzony samodzielny DIAG F, który w DIAG K już wykonał się z HDD.

**Dwa nowe pliki w paczce:** `PSX_DVR_HDD_DIAG_Q.ELF` (launcher uruchamiany przez UDPFS) i **`BOOTQ.CNF`** (tekstowa konfiguracja, do skopiowania na własną `PP.TESTDVR`). DIAG F oraz BOOT.ELF już są na dysku. Żadnych nowych plików ELF na HDD tym razem nie trzeba kopiować.

Zawartość `BOOTQ.CNF` (dokładnie 45 bajtów, LF):
```text
boot=PSX_DVR_HDD_DIAG_F.ELF
arg=-dev9=NICHDD
```

Flaga `-dev9=NICHDD` poleca upstreamowemu loaderowi **nie wyłączać zasilania HDD/DEV9** przy przekazaniu sterowania. To kontrolowana różnica względem dotychczasowego `BOOT.CNF`, dlatego nawet udany Q nie udowodni, że domyślne zachowanie zasilania jest poprawne w J. Używamy jej jako środka ostrożności podczas pierwszego prawdziwego testu loadera.

## Co robi Q?

1. Uruchamia się z UDPFS przez wLE-R3Z. Korzysta z potwierdzonej sekwencji read-only inicjalizacji APA/PFS/DVR jak DIAG J i montuje wyłącznie `hdd0:PP.TESTDVR` jako `pfs1:` z `FIO_MT_RDONLY`.
2. **Weryfikuje bajt w bajt** `pfs1:/BOOTQ.CNF`: dokładnie jedna linia `boot=PSX_DVR_HDD_DIAG_F.ELF` i `arg=-dev9=NICHDD`. Brak pliku albo różnica choćby jednego bajtu = ujemny `CHECK` i wyłączone X.
3. Otwiera tylko do odczytu `pfs1:/BOOT.ELF` (rzeczywisty OSDMenu) i `pfs1:/PSX_DVR_HDD_DIAG_F.ELF` (zweryfikowany wcześniej F), sprawdza dodatni rozmiar i nagłówek ELF. Kontroluje współistnienie `dvr_pfs0:/ISO` i PS2 PFS.
4. Wyświetla `CHECK`, `MOUNT`, `BOOTQ`, `OSD ELF`, `TARGET F`, `DVR`, `PFS`. START = anulowanie i normalne wyjście do XMB. **Dopiero świadome X przy CHECK=0** wczytuje oryginalne `pfs1:/BOOT.ELF` przez sprawdzoną w DIAG K funkcję EE `LoadELFFromFile`.
5. Jako jawny argument `argv[1]` OSDMenu dostaje `hdd0:PP.TESTDVR/BOOTQ.CNF`. Jeśli prawdziwy Quickboot, `handlePFS` i osadzony loader działają, powinien wyświetlić się ekran **`PSX DVR HDD DIAG F / EXIT TEST`**, a nie Neutrino/gra.

## Instrukcja testu i bezpieczeństwo

1. Zalecana kopia ważnych danych przed eksperymentem z obcymi sterownikami IOP. Nie zmieniaj danych DVR, partycji Sony ani wpisów XMB.
2. Z **zielonego CI** pobierz paczkę `PSX-DVR-HDD-DIAG-Q-ORIGINAL-OSDMENU-TO-F`. Sprawdź, że zawiera pliki `PSX_DVR_HDD_DIAG_Q.ELF` i `BOOTQ.CNF`.
3. Skopiuj na własną `hdd0:/PP.TESTDVR/` przez wLE-R3Z **tylko `BOOTQ.CNF`**. Nie nadpisuj `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf` ani `PSX_DVR_HDD_DIAG_F.ELF` (F już był na HDD od DIAG K).
4. Uruchom `PSX_DVR_HDD_DIAG_Q.ELF` z UDPFS, nie kopiuj go na HDD.
5. Na ekranie `PSX DVR HDD DIAG Q / RESULTS` oczekuj `CHECK=0`, `MOUNT=0`, `BOOTQ=45`, dodatnie `OSD ELF` i `TARGET F`; zrób zdjęcie. Przy `CHECK` ujemnym **nie naciskaj X**, przekaż zdjęcie. Q sam blokuje X.
6. Przy `CHECK=0` świadomie naciśnij **X**. Wyświetli się komunikat `Q BEFORE REAL OSDMENU`. Po nim spodziewamy się **ekranu DIAG F**. Zrób zdjęcie finalnego ekranu; opisz, czy nastąpił powrót do XMB, czarny ekran, czy była wiadomość błędu z OSDMenu.
7. Jeśli przejdzie do DIAG F, START to zwykłe wyjście (nie powtarzamy osobnego testu START). Jeżeli po X wystąpi zawieszenie, może być wymagany restart konsoli. **Nie resetuj dysku w trakcie możliwego dostępu bez potrzeby.**

**Granice:** Q jest prawdziwym uruchomieniem oryginalnego `BOOT.ELF` i jego dalszego loadera, a nie tylko odczytem — mimo że sam kod Q nie zapisuje HDD i preflight montuje RO, nie możemy gwarantować operacji wewnętrznych OSDMenu. W przeciwieństwie do DIAG P, po X możliwa jest zmiana stanu IOP/DEV9. Testowy CNF wskazuje tylko DIAG F, więc przy poprawnej obsłudze OSDMenu nie powinien uruchamiać żadnej gry. `main`, wydanie v0.8 i poprzednie diagnostyki pozostają bez zmian; PR #26 zostaje Draft.

---

# DIAG Q — rzeczywisty OSDMenu Loader z bezpiecznym celem DIAG F

**Stan wejściowy:** na fizycznym Sony PSX DESR-7000 DIAG P zakończył się `RESULT=0 STAGE=64`, `RESET2=1 MOD2=1 PAD2=1`, `UM1=0 REMOUNT=0 TRYFD=3 UM2=0 PATH=1`. Rozwiązaliśmy inicjalizację, mount i konstrukcję `hdd0:PP.TESTDVR:pfs:/neutrino.elf`, ale w DIAG J oryginalny OSDMenu po naciśnięciu X wracał do XMB i nie uruchomił Neutrino. **DIAG Q nie był jeszcze testowany na fizycznej konsoli.**

## Dlaczego DIAG Q nie ma drugiego ELF do umieszczenia na HDD?

Zamiast kolejnej symulacji, wykorzystujemy **istniejący na PP.TESTDVR oryginalny `BOOT.ELF` (OSDMenu)** oraz już przetestowany w K **`PSX_DVR_HDD_DIAG_F.ELF`**. Komponentem testowym Q jest mały plik tekstowy **`DIAG_Q.CNF`** zawierający dokładnie jedną linię:

```text
boot=PSX_DVR_HDD_DIAG_F.ELF
```

Plik ma być w katalogu głównym **`PP.TESTDVR`**, obok, ale **ZAMIAST NADPISYWANIA** oryginalnego `BOOT.CNF`. Nie zmieniaj `BOOT.CNF`, `BOOT.ELF` ani `neutrino.elf` i nie przenoś ich z partycji.

Źródło `pcm720/OSDMenu` (konkretnie `launcher/src/main.c`, `launcher/src/handler_quickboot.c`, `launcher/src/handler_pfs.c`, `common/src/loader.c` oraz `utils/loader/src/loader.c`) pokazuje, że drugi argument `hdd0:PP.TESTDVR/DIAG_Q.CNF` wybiera oryginalne `handleQuickboot`. Następnie `boot=PSX_DVR_HDD_DIAG_F.ELF` ma przejść przez oryginalne `launchPath` i `handlePFS`, zbudować ścieżkę **`hdd0:PP.TESTDVR:pfs:/PSX_DVR_HDD_DIAG_F.ELF`** i uruchomić prawdziwy wbudowany w OSDMenu loader. Dzięki temu **nie próbujemy uruchamiać Neutrino ani gry**. W razie niezgodności aktualnego binarnego OSDMenu z kodem upstream albo nieprzewidzianego zachowania powrót do XMB/zawieszenie jest możliwy; nie da się całkowicie wykluczyć wykonania innego kodu przez obcy BOOT.ELF.

## Co robi nowy launcher `PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF`?

1. Uruchomiony z UDPFS przez wLE-R3Z montuje wyłącznie własne `hdd0:PP.TESTDVR` jako `pfs1:` z `FIO_MT_RDONLY`.
2. Powtarza sprawdzony w J/K read-only preflight, w tym sprawdza istniejący OSDMenu `pfs1:/BOOT.ELF` i obecne pliki F i Neutrino. **Nie modyfikuje** istniejącego `BOOT.CNF`, `BOOT.ELF`, danych gier czy sektorów systemowych.
3. Otwiera **`pfs1:/DIAG_Q.CNF` tylko do odczytu** i sprawdza zawartość **bajt po bajcie**, dokładnie `boot=PSX_DVR_HDD_DIAG_F.ELF\n`. Przy pustym, innym lub uszkodzonym `DIAG_Q.CNF` blokuje X. Sprawdza nagłówek ELF F.
4. Przy `CHECK=0`, poprawnym QCNF i F, wyświetla **`X: RUN OSDMENU -> DIAG F`** oraz **`START: CANCEL TO XMB/OSD`**. Tylko nowe naciśnięcie X, nie przytrzymany wcześniej przycisk, rozpoczyna test.
5. Po naciśnięciu X ładuje **istniejący `pfs1:/BOOT.ELF`**, przekazując argument **`hdd0:PP.TESTDVR/DIAG_Q.CNF`**. Na ekranie zobaczysz komunikat **`Q BEFORE LoadELFFromFile`**.
6. **SUKCES:** wyświetli się znany ekran `PSX DVR HDD DIAG F / EXIT TEST`. To dowiedzie, że istniejący OSDMenu wykonał jego rzeczywisty łańcuch loadera do prostego ELF z HDD — wciąż nie będzie to dowód uruchomienia Neutrino.
7. **NIEPOWODZENIE:** zobaczysz powrót do XMB, czarny ekran, ewentualnie kod `Q CHAINLOAD RETURNED=...`. Zapisz co widać i po jakim czasie (OSDMenu może sam wrócić po obsłudze błędu). To pomoże ustalić, że problem leży PRZED Neutrino, a nie w samej grze.

## Instrukcja instalacji do własnej partycji (bez dotykania Sony)

1. Wykonaj kopię ważnych danych. Korzystaj wyłącznie z własnej `PP.TESTDVR`, nigdy partycji Sony `__*`.
2. Po zielonym CI pobierz archiwum **`PSX-DVR-HDD-DIAG-Q-REAL-OSDMENU-TO-F`**. W paczce znajdziesz **ELF launchera i `DIAG_Q.CNF`**, nie drugie ELF.
3. Skopiuj **wyłącznie `DIAG_Q.CNF`** do katalogu głównego `PP.TESTDVR` (ten sam, gdzie `BOOT.ELF` i `PSX_DVR_HDD_DIAG_F.ELF`). Jeśli pliku F już tam nie ma, skopiuj wcześniej sprawdzony `PSX_DVR_HDD_DIAG_F.ELF` z zielonego artefaktu DIAG K. **Nie nadpisuj `BOOT.CNF`.**
4. **`PSX_DVR_HDD_DIAG_Q_LAUNCH.ELF` uruchom z UDPFS** przez wLE-R3Z, nie z HDD.
5. Na pierwszym ekranie oczekujemy `CHECK=0 MOUNT=0`, `QCNF=...` dodatnie, `F BYTES=...` dodatnie. Zrób zdjęcie. Przy błędzie X jest zablokowany.
6. Przy `CHECK=0` naciśnij X. Zapisz, czy pojawił się ekran F, ekran OSDMenu z błędem, powrót do XMB lub zawieszenie. Nie musisz już testować działania START jako osobnej funkcji.

**Bezpieczeństwo:** kod Q nie zapisuje na dysk, ale po naciśnięciu X wykonuje **obcy** OSDMenu i jego własny loader; nie gwarantujemy braku jego wewnętrznych operacji dyskowych. Jeśli pojawi się inny niż DIAG F program, nie kontynuuj działania i opisz wynik. Nie ma automatycznego uruchamiania bez zgody użytkownika. `main`, Manager v0.8, wcześniejsze ELF, partycje systemowe i wpisy XMB pozostają bez zmian. Draft PR #26 pozostaje niescalony.

---

## DIAG P — fizyczny PASS na Sony PSX DESR-7000 (3 listopada 2026)

Użytkownik przesłał dwa zdjęcia z prawidłowego testu **X** (wcześniejsze przypadkowe naciśnięcie START wycofał, więc nie zaliczamy go jako testu uruchomieniowego).

**Launcher DIAG P:** `CHECK=0 MOUNT=0`, `CNF=74 ELF=197396 NEUT=75140`, `SYS=1 BSD=1 DVD=1 USB=0`, `ISO=3 DVR=3 PFS=3`, `TARGET P BYTES=1757308`.

**Target uruchomiony z PP.TESTDVR:** `RESULT=0 STAGE=64`, `ARG=1 OSDMOD=0`, `HDD=2 MOUNT=0 PFS=3`, `CNF=74 ELF=197396 NEUT=75140`, `QB=75140 STDIO=0 BOOT=1`, `RESET2=1 MOD2=1 PAD2=1`, `UM1=0 REMOUNT=0`, `TRYFD=3 UM2=0 PATH=1`. Zbudowana ścieżka: `hdd0:PP.TESTDVR:pfs:/neutrino.elf`. Ekran potwierdził `NO TARGET EXECUTION`.

**Wniosek:** potwierdzony drugi reset IOP po Quickboot oraz powtórne uruchomienie przygotowanego zestawu modułów OSDMenu-like, ponowny montaż tylko do odczytu i przygotowanie ścieżki loadera. **Nie** oznacza to wykonania rzeczywistego oryginalnego OSDMenu, jego wszystkich kroków (m.in. CLOSEALL/SYNC) ani uruchomienia Neutrino/gry. Powrót do XMB po uruchomieniu OSDMenu w DIAG J pozostaje niewyjaśniony. Proponowany następny etap: izolacja rzeczywistego przekazania sterowania do nieszkodliwego testowego ELF przy użyciu tego samego mechanizmu ścieżki `:pfs:`, zanim zostanie wywołane Neutrino. Brak zmian na `main`, v0.8, partycjach systemowych i XMB. START nie wymaga osobnych testów.

---

# DIAG P — drugi reset IOP po Quickboot, bez uruchamiania Neutrino

**Dotychczas potwierdzone na fizycznym DESR-7000:** DIAG O po wystartowaniu z PP.TESTDVR zakończył się `RESULT=0 STAGE=38`, `UM1=0 REMOUNT=0 TRYFD=3 UM2=0 PATH=1` i wyświetlił `hdd0:PP.TESTDVR:pfs:/neutrino.elf`. W DIAG O nie było jednak drugiego resetu IOP wykonywanego przez oryginalny OSDMenu w `handlePFS`. DIAG P pozostaje nowym eksperymentem — **jeszcze nie sprawdzonym sprzętowo**.

## Co robi DIAG P

`PSX_DVR_HDD_DIAG_P_LAUNCH.ELF` uruchamiany przez UDPFS wykorzystuje potwierdzony wcześniej stos sterowników DVR/APA, czyta tylko `PP.TESTDVR`, sprawdza trzy pliki startowe, docelowy P oraz wskazany ISO. Dopiero po `CHECK=0` i nowym naciśnięciu **X** uruchamia **wyłącznie** `PSX_DVR_HDD_DIAG_P.ELF` z `pfs1:`, przekazując argument `hdd0:PP.TESTDVR/BOOT.CNF`. START pozostaje zwykłym anulowaniem; nie testujemy go osobno.

Program docelowy P (startujący naprawdę z HDD):
1. Uruchamia sprawdzony w M/N/O zestaw sterowników OSDMenu-like, montuje wyłącznie `hdd0:PP.TESTDVR` jako `pfs0:` **tylko do odczytu** i czyta `BOOT.CNF` przez `fopen()` oraz Neutrino z `fileXioOpen`.
2. Kończy Quickboot na **P32** odmontowaniem własnego `pfs0:`. Dopiero gdy odmontowanie się uda, przechodzi do obsługi `handlePFS`.
3. **P40:** zamyka stary kontroler, wykonuje drugi `SifIopReset`; **P41:** `SifIopSync`; **P42:** inicjalizacja RPC/patches; **P43–P57:** powtórne załadowanie `iomanX`, `fileXio`, `sio2man`, `mcman`, `mcserv`, diagnostycznego `freepad`, `ps2dev9`, `bdm`, `bdmfs_fatfs`, `ata_bd`, `ps2hdd-osd`, `ps2fs`, `secrsif` oraz ponowna inicjalizacja pada.
4. **P58–P60:** nowe `fileXioInit`, dostęp `hdd0:` i kontrola, czy `pfs0:` jest wolne.
5. **P61:** nowe montowanie `PP.TESTDVR` RO; **P62:** POSIX `open(..., O_RDONLY)` dla `neutrino.elf`; **P63:** odmontowanie własnego `pfs0:`; **P64:** budowa ścieżki `hdd0:PP.TESTDVR:pfs:/neutrino.elf` do przyszłego loadera. **Nie wykonuje jej.**

Na ekranie wyników spodziewamy się `RESULT=0 STAGE=64`, `ARG=1 OSDMOD=0`, `RESET2=1 MOD2=1 PAD2=1`, `UM1=0 REMOUNT=0 TRYFD>=0 UM2=0 PATH=1`. Jeśli wynik jest ujemny, prześlij `RESULT` i `STAGE`. Jeśli nastąpi zawieszenie, sfotografuj ostatni komunikat `P... BEFORE`.

## Bezpieczna instrukcja testu

1. Miej aktualną kopię ważnych danych — nie wszystkie wewnętrzne operacje obcych sterowników IOP są audytowane.
2. Pobierz **dwa ELF z zielonego artefaktu GitHub Actions** `PSX-DVR-HDD-DIAG-P-SECOND-IOP-RESET-READONLY`.
3. Skopiuj **tylko `PSX_DVR_HDD_DIAG_P.ELF`** na własną, już utworzoną `hdd0:/PP.TESTDVR/` w wLE-R3Z. Niczego innego na tej partycji nie nadpisuj. Nie dotykaj partycji Sony `__*`, DVR ISO, XMB ani wydania v0.8.
4. **`PSX_DVR_HDD_DIAG_P_LAUNCH.ELF` uruchom z UDPFS** (lub USB), a nie z HDD. Pierwszy ekran powinien pokazać `CHECK=0 MOUNT=0` i dodatnie `TARGET P BYTES`; zrób zdjęcie.
5. Naciśnij X dopiero przy `CHECK=0`. Drugi ekran to `PSX DVR HDD DIAG P / RESULTS`; zrób zdjęcie.
6. Po zakończeniu START wraca do XMB. Jeśli po drugim resecie IOP program się zatrzyma, START może nie działać; sfotografuj ostatni etap i zrestartuj konsolę.

**Granice testu:** DIAG P nie uruchamia Neutrino, OSDMenu ani gry, nie wywołuje loadera docelowego ELF i nie wykonuje `PDIOC_CLOSEALL` lub `fileXioSync` z oryginalnego `deinitPFS`. Używa modułów zbudowanych w naszym środowisku PS2SDK, niekoniecznie bitowo identycznych z oryginalnym BOOT.ELF. Test ma izolować drugi reset sterowników; nawet PASS nie jest jeszcze pełnym uruchomieniem Managera bez USB. Żadnych nowych wpisów w XMB ani zmian w stabilnym v0.8, `main` i wcześniejszych diagnostykach. Draft PR #26 pozostaje niescalony.

---

## DIAG O — fizyczny PASS na DESR-7000 (2.11.2026)

Dwa zdjęcia użytkownika potwierdzają zakończoną diagnostykę DIAG O. Launcher: `CHECK=0 MOUNT=0`, `CNF=74 ELF=197396 NEUT=75140`, `SYS=1 BSD=1 DVD=1 USB=0`, `ISO=3 DVR=3 PFS=3`, `TARGET O BYTES=1755660`. Target z HDD: `RESULT=0 STAGE=38`, `ARG=1 OSDMOD=0`, `HDD=2 MOUNT=0 PFS=3`, `QB=75140 STDIO=0 BOOT=1`, `UM1=0 REMOUNT=0 TRYFD=3 UM2=0 PATH=1`. Poprawna ścieżka loadera: `hdd0:PP.TESTDVR:pfs:/neutrino.elf`.

**Potwierdzono:** odmontowanie własnego `pfs0:`, ponowne montowanie `PP.TESTDVR` tylko do odczytu, POSIX `open()` na `neutrino.elf`, ponowne odmontowanie i konstrukcję ścieżki `:pfs:`. **Nie potwierdzono:** ponownego resetu IOP w prawdziwym `handlePFS`, operacji `PDIOC_CLOSEALL` i `fileXioSync`, uruchomienia Neutrino/gry ani startu z XMB bez nośnika. DIAG J nadal wracał do XMB. Nic nie zmieniamy w stabilnym v0.8, `main` i partycjach Sony.

---

# DIAG O — test handlePFS: odmontowanie, ponowny odczyt ELF i budowanie ścieżki :pfs:

**Wynik wcześniejszy:** DIAG N pełny PASS na fizycznym Sony PSX DESR-7000: `RESULT=0 STAGE=32`, `ARG=1 OSDMOD=0`, `STDIO=0 ERRNO=0 BOOT=1`, `QB ELF=75140 ARGS=2 BSD=1 DVD=1`. Udało się otworzyć `BOOT.CNF` przez `fopen()` po starcie z HDD. Oryginalny OSDMenu w DIAG J nadal wcześniej wracał do XMB; przyczyna nieustalona. DIAG O **nie był jeszcze uruchomiony na sprzęcie**.

## Co bada O?

W upstreamowym `pcm720/OSDMenu` (`launcher/src/handler_quickboot.c`, `launcher/src/handler_pfs.c`, `launcher/src/common.c`) po odczycie BOOT.CNF funkcja Quickboot odmontowuje swoje `pfs0:`, buduje `hdd0:PP.TESTDVR/neutrino.elf`, wywołuje `launchPath()`, a obsługa `handlePFS()` ponownie inicjalizuje sterowniki APA/PFS, montuje HDD, sprawdza plik przez POSIX `open()`, odmontowuje i buduje ścieżkę `hdd0:PP.TESTDVR:pfs:/neutrino.elf`, która następnie trafia do loadera.

**DIAG O testuje tylko bezpieczny wycinek tego łańcucha, bez uruchomienia Neutrino ani gry:**

1. Z UDPFS przez wLE-R3Z uruchomiany jest osobny `PSX_DVR_HDD_DIAG_O_LAUNCH.ELF` z kontrolą `CHECK=0`; przy świadomym X ładuje **tylko** `PSX_DVR_HDD_DIAG_O.ELF` z naszej `PP.TESTDVR` i przekazuje `hdd0:PP.TESTDVR/BOOT.CNF` jako argument.
2. Docelowy O sam ponownie resetuje IOP, ładuje zestaw modułów OSDMenu przetestowany już przez M/N, montuje wyłącznie `PP.TESTDVR` w trybie `FIO_MT_RDONLY` jako `pfs0:` i odczytuje CNF/ELF.
3. Powtarza sprawdzony w N fragment Quickboot (normalizacja i `fopen/fgets`). Przy poprawnej konfiguracji wykonuje **etap O32**: odmontowuje wyłącznie własne `pfs0:`.
4. Symuluje dalsze `handlePFS`: **O33** waliduje `hdd0:PP.TESTDVR/neutrino.elf`, **O34** sprawdza, czy `pfs0:` jest wolne, **O35** ponownie montuje `PP.TESTDVR` tylko do odczytu, **O36** sprawdza plik przez POSIX `open(..., O_RDONLY)`, **O37** odmontowuje własne `pfs0:`, **O38** buduje ścieżkę `hdd0:PP.TESTDVR:pfs:/neutrino.elf`.
5. Wyświetla wyniki `RESULT`, `STAGE`, `ARG`, `OSDMOD`, `UM1`, `REMOUNT`, `TRYFD`, `UM2`, `PATH` i ewentualną ścieżkę `LOADER`. **NIE uruchamia ładowarki docelowego Neutrino**, nie wywołuje `LoadELFFromFile` z docelowego ELF O.

**Rozmyślne ograniczenia testu:** O nie wykonuje ponownego resetu IOP między Quickboot i handlePFS (upstream `handlePFS` robi to poprzez `initModules`). Nie używa też upstreamowych `fileXioDevctl(PDIOC_CLOSEALL)` ani `fileXioSync`, by nie ryzykować dodatkowych operacji dyskowych. Testuje sam cykl `unmount → remount RO → POSIX open → unmount → path`, a nie pełny runtime OSDMenu. Nawet `RESULT=0` nie oznacza, że cały oryginalny OSDMenu lub Neutrino będzie działać.

## Instrukcja

1. Zachowaj kopię ważnych danych. Nie używaj HDD Manager/Format/Inject Header, nie dotykaj partycji Sony ani obszaru DVR.
2. Z zielonego CI pobierz artefakt `PSX-DVR-HDD-DIAG-O-HANDLEPFS-READONLY`. Znajdziesz w nim **dwa ELF**.
3. Skopiuj do katalogu głównego **`PP.TESTDVR` tylko `PSX_DVR_HDD_DIAG_O.ELF`** przez wLE-R3Z. Pozostałe pliki `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, F–N pozostają bez zmian.
4. **`PSX_DVR_HDD_DIAG_O_LAUNCH.ELF` uruchom z UDPFS**, nie z HDD. Na pierwszym ekranie spodziewane `CHECK=0 MOUNT=0` i dodatnie `TARGET O BYTES`; zrób zdjęcie.
5. Dopiero przy `CHECK=0` naciśnij X, aby uruchomić docelowy O z HDD. Zrób zdjęcie końcowego `PSX DVR HDD DIAG O / RESULTS`. Oczekujemy `RESULT=0`, `STAGE=38`, `ARG=1`, `OSDMOD=0`, `UM1=0`, `REMOUNT=0`, `TRYFD>=0`, `UM2=0`, `PATH=1`.
6. Gdy test zatrzyma się na `O... BEFORE` lub pojawi się błąd, prześlij ostatni ekran z kodem. START jest zwykłym powrotem do XMB, nie musisz osobno testować jego działania.

Program diagnostyczny wykonuje wyłącznie otwarcia i montowania tylko do odczytu, a obcy kod sterowników IOP może wykonywać inne wewnętrzne operacje. W przypadku zablokowania RPC może być potrzebny restart. `main`, v0.8, poprzednie diagnostyki i systemowe partycje Sony pozostają nienaruszone; PR #26 wciąż Draft.

---

## DIAG N — pełny PASS na fizycznym DESR-7000 (2.11.2026)

Użytkownik przesłał dwa zdjęcia. Launcher: `CHECK=0 MOUNT=0`, `CNF=74 ELF=197396 NEUT=75140`, `SYS=1 BSD=1 DVD=1 USB=0`, `ISO=3 DVR=3 PFS=3`, `TARGET N BYTES=1747216`. Właściwy ELF z `PP.TESTDVR`: `RESULT=0 STAGE=32`, `ARG=1 OSDMOD=0`, `HDD=3 MOUNT=0 PFS=3`, `CNF=74 ELF=197396 NEUT=75140`, `STDIO=0 ERRNO=0 BOOT=1`, `QB ELF=75140 ARGS=2 BSD=1 DVD=1`.

Potwierdzono symulację wybranej części Quickboot: normalizację do `pfs0:/BOOT.CNF`, standardowe `fopen()`, `fgets` oraz przygotowanie/odczyt `neutrino.elf` bez jego wykonania. To **nie jest** potwierdzenie działania oryginalnego łańcucha `handleQuickboot → launchPath → handlePFS → LoadELFFromFile` ani uruchomienia gry. W upstream OSDMenu po odczycie BOOT.CNF mechanizm `handlePFS` montuje partycję, tworzy ścieżkę `:pfs:`, wywołuje loader i może wykonywać przejście DEV9. Proponowany następny DIAG O: diagnostyka tych dalszych etapów bez uruchomienia Neutrino. Zachowujemy brak zmian w `main`, v0.8, systemowych partycjach Sony; PR Draft. START pozostaje zwykłym wyjściem, bez osobnego testowania.

---

# DIAG N — szczegółowy test Quickboot OSDMenu (bez uruchamiania gry)

**Stan wejściowy:** fizyczny Sony PSX DESR-7000 potwierdził `DIAG M / LAUNCH: CHECK=0` oraz `DIAG M / RESULTS: RESULT=0, ARG=1, OSDMOD=0, MOUNT=0`. Test M wykazał możliwość uruchomienia ELF z HDD i inicjalizacji podobnego do OSDMenu stosu modułów APA/PFS, jednak wcześniejszy rzeczywisty `BOOT.ELF` w DIAG J wrócił do XMB po X. Powód pozostaje nieznany.

## Co konkretnie testujemy

N odtwarza **wycinek logiki** upstream `pcm720/OSDMenu` `launcher/src/handler_quickboot.c` oraz `launcher/src/common.c` bez wywołania `launchPath`, `LoadELFFromFile` do Neutrino, ani wykonania OSDMenu z programu docelowego. Na podstawie przekazanego argumentu `hdd0:PP.TESTDVR/BOOT.CNF` program:

1. Inicjalizuje sprawdzony w DIAG M zestaw modułów `iomanX/fileXio/sio2man/mcman/mcserv/ps2dev9/bdm/bdmfs_fatfs/ata_bd/ps2hdd-osd/ps2fs/secrsif` i dodatkowy `freepad`. **Nie ładuje docelowo modułów DVR**, nie uruchamia gier.
2. Montuje jedynie własną partycję `hdd0:PP.TESTDVR` jako `pfs0:` z `FIO_MT_RDONLY`, sprawdza odczyt `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf` przez `fileXio`.
3. Sprawdza rozbicie ścieżki startowej na partycję `hdd0:PP.TESTDVR` oraz ścieżkę wewnętrzną `/BOOT.CNF`, normalizując ją do **`pfs0:/BOOT.CNF`** jak upstream.
4. Otwiera `BOOT.CNF` przez zwykłe **`fopen(..., "r")`**, inaczej niż wcześniejsze DIAG I/M, które używały `fileXioOpen`. To może ujawnić rozbieżność między stdio a fileXio na DESR-7000.
5. Czyta wiersze przez `fgets`, rozpoznaje klucze `boot=`, `path=`, `arg=` na zasadzie upstream Quickboot; buduje docelową ścieżkę `hdd0:PP.TESTDVR/neutrino.elf`, następnie sprawdza przez `fileXio`, że wskazany ELF można otworzyć (read-only, nagłówek ELF).
6. Na ekranie `PSX DVR HDD DIAG N / RESULTS` wyświetla `RESULT`, `STAGE`, `ARG`, `OSDMOD`, `HDD/MOUNT/PFS`, `STDIO/ERRNO/BOOT`, `QB ELF/ARGS/BSD/DVD`. Zamknie wszystkie otwarte pliki i odmontuje wyłącznie własne `pfs0:`. START kończy program jak we wcześniejszych diagnostykach (nie testujemy osobno START).

**Granice:** To kontrolowana symulacja etapów Quickboot z podanym plikiem, **nie uruchomienie oryginalnego `BOOT.ELF` ani pełnej implementacji `handleQuickboot()` i `handlePFS()`**. Może wykryć problem w normalizacji, fopen lub odczycie CNF. Nawet `RESULT=0` nie dowiedzie poprawności całego OSDMenu ani Neutrino. ISO wskazane w `arg=-dvd` jest w N tylko parsowane; nie próbujemy go otwierać z docelowego stosu OSDMenu, który nie ma inicjalizacji DVR.

## Kopiowanie i test na konsoli

1. Przed eksperymentami na HDD zachowaj kopię ważnych danych. Nie używaj HDD Manager/Format/Inject i nie dotykaj partycji systemowych Sony.
2. Po **zielonym GitHub Actions** pobierz artefakt `PSX-DVR-HDD-DIAG-N-QUICKBOOT-READONLY` z dwoma plikami ELF.
3. Skopiuj do katalogu głównego **wyłącznie** `PSX_DVR_HDD_DIAG_N.ELF` na własną `PP.TESTDVR`. Nie nadpisuj istniejących `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, F, L ani M.
4. Uruchom **`PSX_DVR_HDD_DIAG_N_LAUNCH.ELF` przez UDPFS** w wLaunchELF R3Z; nie kopiuj launchera na HDD.
5. Na pierwszym ekranie oczekujemy `CHECK=0` i dodatniego `TARGET N BYTES`. Zrób zdjęcie, a następnie naciśnij **X**, aby uruchomić wyłącznie test N z HDD. Jeśli `CHECK` ujemny, X jest wyłączony — prześlij zdjęcie.
6. Zrób zdjęcie drugiego ekranu `PSX DVR HDD DIAG N / RESULTS`. Oczekujemy `RESULT=0`, `ARG=1`, `OSDMOD=0`, `MOUNT=0`, `STDIO=0`, `BOOT=1`, `QB ELF>0`, `BSD=1`, `DVD=1`. Gdy `RESULT` ujemny, `STAGE` oraz `STDIO/ERRNO` pokażą źródło problemu. Jeśli zawiesi się na `N... BEFORE`, sfotografuj ostatni etap.

N nie zawiera operacji zapisu, tworzenia/formatowania partycji ani wykonania gry; montowanie odbywa się tylko do odczytu. Nie da się jednak wykluczyć wewnętrznych zapisów sterowników IOP. Możliwe zablokowanie synchronicznego RPC (wtedy START może nie pomóc). `main`, Manager v0.8, wcześniejsze testy i XMB zostają bez zmian. PR #26 pozostaje Draft i nie jest scalany.

---

## DIAG M — fizyczny PASS na DESR-7000 (2 listopada 2026)

Dwa zdjęcia użytkownika potwierdzają: `PSX DVR HDD DIAG M / LAUNCH` => `CHECK=0 MOUNT=0`, `CNF=74 ELF=197396 NEUT=75140`, `SYS=1 BSD=1 DVD=1 USB=0`, `ISO=3 DVR=3 PFS=3`, `TARGET M BYTES=1676004`. Docelowy ELF z `PP.TESTDVR` wykonał `PSX DVR HDD DIAG M / RESULTS`: `RESULT=0 STAGE=27`, `ARG=1 OSDMOD=0`, `HDD=3 MOUNT=0 PFS=3`, `CNF=74 ELF=197396 NEUT=75140`.

Wynik potwierdza działanie stosu podobnego do OSDMenu (m.in. `ata_bd`, `bdm`, `bdmfs_fatfs`, `ps2hdd-osd`, `ps2fs`, `secrsif`) po uruchomieniu z HDD, przekazanie argumentu `hdd0:PP.TESTDVR/BOOT.CNF`, odczyt plików i odmontowanie własnego `pfs0:`. **Nie potwierdza** przejścia przez prawdziwy kod `handleQuickboot`, `launchPath` ani uruchomienia Neutrino/grania na DVR. DIAG J wcześniej wracał do XMB. Nie wykonano żadnych zmian w `main` i wydaniu v0.8; PR pozostaje Draft. Oddzielnych testów START nie powtarzamy.

---

# DIAG M — diagnostyka modułów APA/OSDMenu po uruchomieniu ELF z HDD

Stan wejściowy: fizyczny DESR-7000 przeszedł **DIAG L / LAUNCH** i **DIAG L / POST-BOOT** (`RESULT=0 MOUNT=0`). Wcześniej DIAG J przechodził kontrolę `CHECK=0`, ale przy X wracał do XMB zamiast uruchomić grę. DIAG M jest **nowym, nieprzetestowanym sprzętowo** testem, nie naprawą OSDMenu.

## Dlaczego test M jest inny?

W kodzie źródłowym `pcm720/OSDMenu`, `launcher/src/init.c`, kolejność modułów dla `Device_APA` to m.in. `iomanX`, `fileXio`, `sio2man`, `mcman`, `mcserv`, `ps2dev9`, `bdm`, `bdmfs_fatfs`, `ata_bd`, `ps2hdd-osd`, `ps2fs`, `secrsif`. Upstream dodaje około 1 s przerwy po `ata_bd` i używa parametrów `-o 4 -n 20` dla `ps2hdd-osd`, `-o 10 -n 40` dla `ps2fs`. DIAG M powtarza tę kolejność, lecz dodatkowo uruchamia `freepad` po podstawowych modułach dla możliwości START. Nie ładuje DVR IRX w docelowym programie — najpierw izolujemy zgodność stosu OSDMenu z DESR-7000.

UWAGA: **nasz kod nie zapisuje na dysk i montuje wyłącznie testową partycję `PP.TESTDVR` w trybie `FIO_MT_RDONLY`, ale nie możemy zagwarantować, że wszystkie zewnętrzne sterowniki IOP są całkowicie pozbawione własnych operacji zapisu**. Wykonaj kopię ważnych danych przed testem. Nie korzystaj z partycji systemowych Sony. Nie używaj tego ELF do instalacji XMB lub formatowania.

## Pakiet: dwa pliki ELF

1. Po **zielonym CI** pobierz artefakt `PSX-DVR-HDD-DIAG-M-OSDMENU-MODULE-PROBE`.
2. Skopiuj wyłącznie **`PSX_DVR_HDD_DIAG_M.ELF`** do katalogu głównego własnej partycji `PP.TESTDVR` przez wLE-R3Z. Nie ruszaj pozostałych plików `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, F i L. W DIAG M jest to `pfs1:/PSX_DVR_HDD_DIAG_M.ELF` w momencie startu.
3. **`PSX_DVR_HDD_DIAG_M_LAUNCH.ELF` uruchom przez UDPFS** (lub USB), a nie z HDD. Na ekranie launchera `CHECK=0` i dodatnie `TARGET M BYTES` potwierdzają gotowość. Zrób zdjęcie.
4. Naciśnij X dopiero przy `CHECK=0`. Program docelowy powinien pokazać `M0 ENTRY FROM HDD`, `argc`, `argv0` i `argv1`, a następnie osobno inicjalizować moduły z komunikatami `M<nr> ... BEFORE`. Docelowo wyświetli `PSX DVR HDD DIAG M / RESULTS`.
5. Zrób zdjęcie wyników. `RESULT=0`, `ARG=1`, `OSDMOD=0`, `MOUNT=0`, oraz dodatnie rozmiary CNF/ELF/NEUT oznaczają, że ta konkretna część stosu OSDMenu działa po uruchomieniu z HDD. Jeśli program się zawiesi, sfotografuj **ostatni numer M... BEFORE**, a przy błędzie podaj `RESULT`, `STAGE`, `OSDMOD`, `MOUNT`. X w launcherze nie uruchamia OSDMenu ani gry.

**Różnica montowania:** launcher M, podobnie jak K/L, montuje testowo `pfs1:` wyłącznie do odczytu. Docelowy M uruchamia się po resetach IOP i używa `pfs0:`, podobnie jak OSDMenu. Samodzielnie sprawdza kolizję, nie odmontowuje cudzych mountów. Przekazujemy przez loader argument `hdd0:PP.TESTDVR/BOOT.CNF`, którego poprawność diagnoza sprawdza w `argv1` bez wykonywania pliku. Wariant M nie uruchamia Neutrino ani nie wykonuje gry. Nie ładuje modułów DVR w docelowym teście, więc nie wyklucza późniejszych problemów po przejściu do Neutrino.

START nadal umożliwia normalne wyjście do XMB, ale nie wymagamy już odrębnego testowania tej funkcji. Po zablokowaniu wywołania RPC/IOP możliwy jest restart konsoli.

**Niezmienione:** `main`, wydanie v0.8, starsze diagnostyki, systemowe partycje Sony, pliki gier i wpisy w XMB. Draft PR #26 nie jest scalany.

---

## DIAG L — fizyczny PASS na DESR-7000 (2 listopada 2026)

Użytkownik przesłał dwa zdjęcia. `PSX DVR HDD DIAG L / LAUNCH`: `CHECK=0 MOUNT=0`, `CNF=74 ELF=197396 NEUT=75140`, `SYS=1 BSD=1 DVD=1 USB=0`, `ISO=3 DVR=3 PFS=3`, `TARGET L BYTES=1769984`. Po X uruchomił się `PSX DVR HDD DIAG L / POST-BOOT`: `RESULT=0 MOUNT=0`, `CNF bytes=74 ELF bytes=197396 NEUTRINO bytes=75140`, `CNF SYS=1 BSD=1 DVD=1 USB=0`, `ISO FILE=3`, `DVR ISO=3 PS2 PFS=3`, komunikat `TARGET EXECUTED. NO GAME LAUNCH.`

**Potwierdzone:** niezależny ELF uruchomiony z `PP.TESTDVR` potrafi od nowa zresetować IOP, załadować nasz sprawdzony stos APA/PFS i DVR, odczytać `BOOT.CNF`, dwa ELF i wybrany obraz ISO na DVR. **Niepotwierdzone:** zgodność innego stosu sterowników upstream OSDMenu (`ata_bd/ps2hdd_osd/secrsif`), prawidłowe przetworzenie parametrów `argv` oraz start Neutrino lub gry. Bez zmian na `main`, w v0.8 i w partycjach systemowych Sony. Oddzielne testy START uznajemy za zakończone.

---

# DIAG L — IOP / APA / DVR po uruchomieniu ELF bezpośrednio z HDD

**Stan potwierdzony na DESR-7000:** DIAG K uruchomił samodzielny DIAG F z `PP.TESTDVR`. DIAG J przechodził kontrolę plików (`CHECK=0`), lecz po X konsola wracała do XMB zamiast uruchomić Neutrino. DIAG L jest odrębnym eksperymentem; **nie został jeszcze przetestowany na fizycznej konsoli**.

## Dwa ELF — zwróć uwagę, który gdzie skopiować

1. Po zielonym buildzie pobierz artefakt **`PSX-DVR-HDD-DIAG-L-POSTBOOT-IOP`**, który zawiera dwa ELF (oraz instrukcję).
2. Zrób kopię ważnych danych. Przez wLE R3Z skopiuj wyłącznie **`PSX_DVR_HDD_DIAG_L.ELF`** do katalogu głównego istniejącej własnej partycji **`hdd0:/PP.TESTDVR/`**. Zostaw nietknięte `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, DIAG F, ISO i partycje systemowe. Nie formatuj niczego. W diagnostyce jest to `pfs1:/PSX_DVR_HDD_DIAG_L.ELF`.
3. **`PSX_DVR_HDD_DIAG_L_LAUNCH.ELF` uruchom przez UDPFS** z wLE R3Z (może być też USB), ale **nie kopiuj go na HDD**.
4. Na ekranie launchera zobaczysz `PSX DVR HDD DIAG L / LAUNCH`, `CHECK=...`, `MOUNT=...`, `TARGET L BYTES=...`. Jeśli `CHECK` jest ujemny, **X jest zablokowany**. Zrób zdjęcie i naciśnij START. Przy `CHECK=0` zrób zdjęcie i naciśnij X, aby uruchomić z HDD **tylko test L**, bez OSDMenu, Neutrino i bez gry.

## Co zobaczysz po X

Docelowy ELF L powinien pokazać natychmiast `PSX DVR HDD DIAG L / HDD TARGET` i `L0 ENTRY: ELF EXECUTING`, z informacją o `argv[0]`. To potwierdzi **samodzielne wykonanie drugiego ELF** po załadowaniu go z HDD.

Następnie target L (jako już działający program) wykonuje od nowa IOP reset, synchronizację, patchowanie RPC, ładowanie **sprawdzonego** zestawu `iomanX/fileXio/ps2dev9/ps2atad/ps2hdd/ps2fs`, montuje **tylko** `PP.TESTDVR` jako `pfs1:` w trybie `FIO_MT_RDONLY`, sprawdza nagłówki plików OSDMenu i Neutrino, odczytuje **wyłącznie** `BOOT.CNF`, inicjalizuje DVR i sprawdza wskazane ISO na `dvr_pfs0:/ISO`. Na koniec odmontowuje tylko własne `pfs1:` i wyświetla `PSX DVR HDD DIAG L / POST-BOOT` oraz `RESULT=...`, `MOUNT=...`, `ISO FILE=...`. Zrzut końcowego ekranu jest najważniejszy.

- `RESULT=0 MOUNT=0`, `CNF SYS=1 BSD=1 DVD=1 USB=0`, `ISO FILE>=0`, `DVR ISO>=0`, `PS2 PFS>=0` oznaczają sukces diagnostyki **po** wykonaniu z HDD.
- Jeśli zawiesi się na `L... BEFORE`, zrób zdjęcie ostatniego etapu. Na przykład `L1 IOP RESET BEFORE` wskazuje problem podczas resetu po chainload.
- `START` pozostaje zwykłym wyjściem do XMB **bez odrębnego testowania przycisku**. Gdy target lub IOP RPC zawiśnie, może być potrzebny restart.

**Granice wnioskowania:** L używa naszego wcześniej sprawdzonego stosu PS2SDK, a nie dokładnie `ata_bd`, `ps2hdd_osd` i `secrsif` występujących w upstreamowym OSDMenu. Sukces L potwierdzi restart sterowników po wykonaniu ELF z HDD, ale **nie rozstrzygnie jeszcze**, czy konkretne sterowniki OSDMenu działają na DESR. L nie próbuje uruchamiać OSDMenu ani Neutrino, nie zapisuje na HDD i nie instaluje XMB. `main`, Manager v0.8, poprzednie diagnostyki i istniejący Draft PR pozostają nienaruszone.

---

## DIAG K — fizyczny DESR-7000: URUCHOMIENIE ELF Z PP.TESTDVR POTWIERDZONE

**2 listopada 2026:** użytkownik zgłasza, że po uruchomieniu DIAG K **DIAG F wystartował bezpośrednio z HDD**. To potwierdza poprawne załadowanie i wykonanie niezależnego pliku ELF z naszej własnej partycji PS2 APA/PFS `PP.TESTDVR`. W tym eksperymencie UDPFS/USB służyło do wystartowania DIAG K, natomiast DIAG F było odczytywane z HDD.

**Nie oznacza** jeszcze działania uruchamiania OSDMenu/Neutrino ani startu Managera z XMB bez USB. DIAG J wcześniej wrócił do XMB po X; po sukcesie K podejrzenie pada na obsługę argumentów, inicjalizację APA/PFS, DVR lub awaryjne wyjście OSDMenu — nie wskazuje to na ogólny brak możliwości wykonywania ELF z HDD. Następny test powinien izolować te komponenty. Osobnych testów START nie powtarzamy.

---

# DIAG J — wynik fizyczny i następny DIAG K (Sony PSX DESR-7000)

**Wynik J, 2 listopada 2026:** użytkownik uruchomił DIAG J przez wLE R3Z/UDPFS. Zdjęcie: `CHECK=0 MOUNT=0`, `CNF=74 ELF=197396 NEUT=75140`, `SYS=1 BSD=1 DVD=1 USB=0`, `ISO=3 DVR=3 PFS=3`. Po **naciśnięciu X konsola wróciła do XMB**, nie odnotowano startu gry. **J preflight PASS; sukces uruchomienia OSDMenu/Neutrino z HDD NIEPOTWIERDZONY**. W kodzie OSDMenu (`launcher/src/common.c`) funkcja błędu po około 10 sekundach wywołuje `ExecOSD`, co *mogłoby* wyjaśnić powrót; to tylko hipoteza, nie ustalenie przyczyny.

## DIAG K — bezpieczniejsze rozdzielenie problemu

DIAG K spróbuje uruchomić **sprawdzony samodzielny DIAG F z partycji `PP.TESTDVR`**, zamiast OSDMenu lub Neutrino. K i F nie uruchamiają gier i nie zapisują na HDD. Dzięki temu odróżnimy awarię odczytu/przekazania sterowania ELF z HDD od ewentualnego błędu OSDMenu w ponownej inicjalizacji APA/PFS lub odczycie CNF.

**Jednorazowa ręczna kopia pliku tylko na własną PP.TESTDVR:**

1. Wykonaj kopię ważnych danych HDD przed kolejnymi eksperymentami. Nie formatuj żadnej partycji.
2. Z zielonego builda GitHub Actions pobierz artefakt `PSX-DVR-HDD-DIAG-K-RAW-ELF-TEST`. ZIP zawiera **dwa pliki ELF: K i F**.
3. Przez wLE R3Z skopiuj **tylko `PSX_DVR_HDD_DIAG_F.ELF` do katalogu głównego `PP.TESTDVR`** (do tej samej partycji, na której leżą już BOOT.CNF, BOOT.ELF, neutrino.elf). Nie nadpisuj tamtych trzech plików. W przyszłej diagnostyce ścieżka to `pfs1:/PSX_DVR_HDD_DIAG_F.ELF`; w wLE wybierz własną partycję `hdd0:/PP.TESTDVR/`. Nie kopiuj niczego do partycji systemowych Sony ani do DVR.
4. **`PSX_DVR_HDD_DIAG_K.ELF` uruchom przez UDPFS** (albo USB) z wLE R3Z; nie kopiuj K na HDD.
5. Na ekranie wyników K oczekuj `CHECK=0`, `MOUNT=0`, `TARGET F BYTES` większe od 0, a także prawidłowych kodów istniejących plików i katalogów DVR/PFS. Jeśli `CHECK` jest ujemny, X nie działa; zrób zdjęcie i wróć przez START.
6. Przy `CHECK=0` zrób zdjęcie, potem naciśnij **X**. Jedynym celem uruchamiania jest teraz znany test `PSX_DVR_HDD_DIAG_F.ELF`. **Prawidłowy wynik to ekran** `PSX DVR HDD DIAG F / EXIT TEST` z instrukcją START. Nie traktuj tego jako ponownego testowania samego START — potrzebny jest dowód, że ekran DIAG F uruchomił się Z HDD.
7. Jeśli nie zobaczysz DIAG F, sfotografuj ostatni ekran. Jeśli `K CHAINLOAD RETURNED=<kod>`, zapisz dokładną wartość. Po uruchomieniu docelowego ELF/IOP może być konieczny restart.

**Ważne:** Samo skompilowanie K nie oznacza, że działa na fizycznym DESR-7000. START w K służy do rezygnacji bez uruchomienia. K używa własnego montowania `PP.TESTDVR` tylko do odczytu i uruchamia dopiero po świadomym X. Podobnie jak w J, loader może zresetować IOP i wtedy przycisk START w K nie odzyska sterowania. Nie zmieniamy `main`, wydania v0.8 ani kodu działających testów A–J.

---

# DIAG J — pierwszy świadomy start OSDMenu z PP.TESTDVR (test sprzętowy)

**Ważne: TO NIE JEST TEST TYLKO DO ODCZYTU W CAŁYM ŁAŃCUCHU.** Sam DIAG J tylko czyta HDD i DVR; po naciśnięciu **X** przekazuje sterowanie obcemu programowi OSDMenu/Neutrino, który może uruchomić grę i zachowywać się niezależnie. Nie wiemy jeszcze, czy OSDMenu i Neutrino potrafią niezawodnie ponownie zainicjalizować sterowniki APA/DVR na DESR-7000. Wymagane zdjęcia wyników. Przy zawieszeniu uruchom ponownie konsolę i wróć do znanego działającego Managera v0.8 na USB.

## Założenia i pliki na HDD

DIAG I został fizycznie potwierdzony na DESR-7000: `RESULT=0 MOUNT=0`, `CNF SYS=1 BSD=1 DVD=1 USB=0`, `ISO FILE=3`, `DVR ISO=3 PS2 PFS=3`. Na `PP.TESTDVR` znajdują się już trzy wcześniej przetestowane pliki: `BOOT.ELF` (OSDMenu Launcher), `BOOT.CNF` i `neutrino.elf`. **Nie kopiuj teraz żadnych innych plików i nie zmieniaj BOOT.CNF.** Tylko samodzielnie utworzona partycja PP.TESTDVR; nie dotykaj partycji systemowych, danych DVR ani wpisów XMB. Zalecana wcześniejsza kopia ważnych danych.

## Konstrukcja DIAG J

- Jest to **osobny** program uruchamiany z USB przez wLaunchELF R3Z, oparty na sekwencji IOP, APA, DVR i padu potwierdzonej w DIAG I.
- Najpierw samodzielnie montuje `hdd0:PP.TESTDVR` → `pfs1:` z `FIO_MT_RDONLY`, czyta `BOOT.CNF`, weryfikuje nagłówki obu plików ELF i sprawdza konfigurację SYSTEM, `-bsd=dvr` i rzeczywisty plik ISO.
- Przy błędzie wyświetla `CHECK` ujemne i wyłącza X. Zawsze można zakończyć przez **START**. Nie powtarzamy odrębnego testu samego START.
- Tylko przy `CHECK=0`: wyświetla **X: LAUNCH HDD OSDMENU/GAME** oraz **START: CANCEL TO XMB/OSD**. Wciśnięcie X z nowego naciśnięcia (nie trzymanego podczas startu) powoduje przekazanie sterowania.
- DIAG J ładuje plik `pfs1:/BOOT.ELF`, gdy jego własny punkt montowania jest jeszcze aktywny, przez `LoadELFFromFile(…, 1, …)`. Do `argv[1]` OSDMenu przekazuje **jawnie** `hdd0:PP.TESTDVR/BOOT.CNF`, co wywołuje upstreamowy config handler. Nie polegamy na `argv[0]=pfs1:/BOOT.ELF` do automatycznego Quickboot; nazwa wewnętrzna `pfs1:` nie jest trwała.
- Zwykłe START wykonuje własne `fileXioUmount("pfs1:")`, zamyka pad i `return 0`. Przy X otwarte pliki są już zamknięte, ale **nie odmontowujemy** `pfs1:` przed wczytaniem `BOOT.ELF` przez loader. Loader czyta ELF przed resetem IOP, który unieważnia stare montowanie.
- Jeśli loader nie przekaże sterowania i zwróci błąd, DIAG J wyświetli `J CHAINLOAD RETURNED=<kod>` i nie będzie ponownie wywoływał IOP RPC (stan po próbie może być nieokreślony). Zrób zdjęcie i zrestartuj konsolę.

## Uruchomienie

1. Pobierz artefakt `PSX-DVR-HDD-DIAG-J-X-ARMED-OSDMENU-LAUNCH` **wyłącznie z zielonego GitHub Actions**. Rozpakuj `PSX_DVR_HDD_DIAG_J.ELF` na USB.
2. Przez wLaunchELF R3Z uruchom DIAG J z USB, z padem podłączonym do portu 1.
3. Zobacz wyniki preflight na ekranie `PSX DVR HDD DIAG J / RESULTS`. Jeśli `CHECK` nie jest zerem, **nie próbuj uruchamiać OSDMenu**. Prześlij zdjęcie i naciśnij START.
4. Gdy `CHECK=0`, zrób zdjęcie ekranu, a potem **naciśnij X tylko jeżeli chcesz uruchomić OSDMenu/Neutrino i ewentualnie grę wskazaną przez BOOT.CNF**. Test może uruchomić grę bez dalszego pytania.
5. Opisz dokładnie, co się wydarzyło po X (ekran OSDMenu, Neutrino, gra, czarny ekran, komunikat błędu). W razie zawieszenia zapisz ostatni ekran/etap. Żadna zmiana w XMB ani `main` nie jest wykonywana przez DIAG J.

**Zastrzeżenie:** fakt udanej kompilacji i DIAG I nie potwierdza jeszcze uruchomienia z HDD. OSDMenu samo może resetować IOP i ładować inne moduły. Użytkownik nie musi ponownie udowadniać, że START wraca do XMB w zwykłych diagnostykach. Stabilna wersja v0.8 i wszystkie poprzednie diagnostyki pozostają nietknięte.

---

## Wynik DIAG I na DESR-7000 — PASS (2 listopada 2026)

Zdjęcie od użytkownika potwierdza: `RESULT=0 MOUNT=0`, `CNF bytes=74`, `ELF bytes=197396`, `NEUTRINO bytes=75140`, `CNF SYS=1 BSD=1 DVD=1 USB=0`, `ISO FILE=3`, `DVR ISO=3 PS2 PFS=3`. Ekran wyświetlał ścieżkę referencyjną `hdd0:PP.TESTDVR/BOOT.ELF` i napis `NOT LAUNCHED. RO ONLY.`

Potwierdzono konfigurację SYSTEM odczytaną z PP.TESTDVR i nieujemny wynik sprawdzania ISO na DVR. Wykrywanie USB dotyczy zawartości `BOOT.CNF`; **nie wyklucza** ukrytych zależności launchera/Neutrino od USB. Nie uruchomiono jeszcze programu z HDD ani nie utworzono wpisu w XMB. Następny etap musi mieć osobne, świadome wyzwolenie startu programu, nie uruchamiać go automatycznie po włączeniu diagnostyki. `main`, wydanie v0.8 i starsze binaria diagnostyczne pozostają bez zmian.

---

# DIAG I — weryfikacja ścieżki startu OSDMenu z HDD, bez uruchamiania

**Wynik poprzedni:** DIAG H: fizyczny DESR-7000, RESULT=0 MOUNT=0, CNF=74 B, OSDMenu BOOT.ELF=197396 B, neutrino.elf=75140 B, DVR ISO=3, PS2 PFS=3. Użytkownik potwierdził START → XMB w H. Nie powtarzamy osobnych testów START.

**Na tym etapie niczego więcej NIE kopiuj ani NIE nadpisuj na PP.TESTDVR.** Pozostaw trzy już sprawdzone pliki `BOOT.ELF`, `BOOT.CNF` i `neutrino.elf` tak, jak po DIAG H.

## Dlaczego I nie uruchamia jeszcze BOOT.ELF

Kod OSDMenu (pcm720/OSDMenu, `launcher/src/handler_quickboot.c` i `launcher/src/common.c`) rozpoznaje ścieżkę APA rozpoczynającą się od `hdd` i sam resetuje IOP, ładuje swój sterownik APA, montuje PFS oraz konwertuje ścieżkę do wewnętrznego punktu montowania. Kandydat do wywołania przez loader:

```text
hdd0:PP.TESTDVR/BOOT.ELF
```

Alternatywny format wspierany przez upstream: `hdd0:PP.TESTDVR:pfs:/BOOT.ELF`. Natomiast `pfs1:/BOOT.ELF` jest **tymczasowym punktem montowania testów D–I**, a nie kanoniczną ścieżką startową OSDMenu.

Launcher w trybie Quickboot wyprowadza nazwę `BOOT.CNF` z `argv[0]`. Po przekazaniu sterowania może sam zresetować IOP, więc równoczesny dostęp do obu systemów plików **w naszym DIAG H nie gwarantuje** uruchomienia launchera i Neutrino. Ponadto `BOOT.CNF` skopiowany z v0.8 może być skonfigurowany na tryb SYSTEM albo ENGLISH i inny obraz ISO.

## Co dokładnie robi DIAG I

- Montuje tylko `hdd0:PP.TESTDVR` jako własne `pfs1:` z flagą `FIO_MT_RDONLY`; sprawdza rozmiary ELF i ich nagłówki jak H.
- Otwiera `BOOT.CNF` **tylko do odczytu**, wymaga pliku tekstowego o rozmiarze poniżej 512 bajtów (bez BOM i bajtów sterujących).
- Odczytuje wiersze `boot=neutrino.elf`, `arg=-bsd=dvr` i `arg=-dvd=dvr_pfs0:/ISO/...`. Wykrywa odwołania do USB, `ata0:` i konfigurację ENGLISH (brak jeszcze skopiowanego `neutrino_lang.elf` i innych zależności).
- Po uruchomieniu sterowników DVR sprawdza istnienie dokładnego obrazu ISO wskazanego w `arg=-dvd` przez otwarcie i zamknięcie **tylko do odczytu**.
- Zamknie otwarte uchwyty, odmontuje własne `pfs1:` i wyświetli `RESULT`, `CNF SYS/BSD/DVD/USB`, `ISO FILE` oraz referencyjną ścieżkę `hdd0:PP.TESTDVR/BOOT.ELF`. Potem START jak w H.

`RESULT=0` oznacza spójny **SYSTEM** `BOOT.CNF`, otwieralny wskazany obraz ISO i zgodne pliki startowe. `RESULT=-136` oznacza, że któryś warunek nie jest spełniony (np. konfiguracja ENGLISH lub brak odwołania do dostępnego ISO), a nie awarię partycji. Gdy wystąpi inne ujemne `RESULT`, wyślij jego kod i zdjęcie.

**Instrukcja testu:** po zielonym buildzie w Actions pobierz artefakt `PSX-DVR-HDD-DIAG-I-BOOT-PATH-READONLY`, skopiuj `PSX_DVR_HDD_DIAG_I.ELF` na USB i uruchom przez wLE-R3Z. Zrób zdjęcie ekranu wyników. Przycisk START służy tylko do wyjścia; nie robimy już osobnego testu jego działania.

**Bezpieczeństwo:** brak zapisu HDD/DVR, brak formatowania, instalacji XMB, wykonywania OSDMenu lub Neutrino. Żaden plik z HDD nie zostanie uruchomiony przez I. Ciągła blokada w IOP RPC może uniemożliwić użycie START. Przed nowymi eksperymentami wykonaj kopię ważnych danych. CI potwierdzi jedynie kompilację i testy źródłowe, nie zachowanie na realnej konsoli.

---

# DIAG H — odczyt 3 plików OSDMenu / Neutrino z PP.TESTDVR

**Stan:** DIAG G i DIAG H zostały przetestowane na fizycznym Sony PSX DESR-7000 (2 listopada 2026). H: RESULT=0, MOUNT=0, CNF bytes=74, ELF bytes=197396, NEUTRINO bytes=75140, DVR ISO=3, PS2 PFS=3. Użytkownik potwierdził powrót do XMB przyciskiem START również w H. Nadal nie testowano uruchamiania OSDMenu/Neutrino z HDD. Odrębnych powtarzanych testów START nie wymagamy; przyszłe diagnostyki zachowują obsługę START do XMB.

**Najpierw sprawdź, czy nowy GitHub Actions build jest zielony.** Nie pobieraj ELF z nieudanego builda. Zrób kopię danych przed ręcznym zapisem na dysk. Cały eksperyment dotyczy wyłącznie wcześniej utworzonej przez Ciebie partycji PP.TESTDVR.

## Co skopiować ręcznie w wLaunchELF R3Z (tylko 3 pliki)

Ze sprawdzonej paczki USB v0.8 (masz już folder mass0:/DESR_DVR/), skopiuj dokładnie:

1. `mass0:/DESR_DVR/BOOT.ELF` → `hdd0:/PP.TESTDVR/BOOT.ELF` (OSDMenu Launcher)
2. `mass0:/DESR_DVR/BOOT.CNF` → `hdd0:/PP.TESTDVR/BOOT.CNF` (zachowaj zawartość; **może nadal wskazywać USB**, DIAG H nie uruchamia konfiguracji)
3. `mass0:/DESR_DVR/neutrino.elf` → `hdd0:/PP.TESTDVR/neutrino.elf`

To ścieżki interfejsu plików wLE. W kodzie DIAG H docelowa partycja będzie montowana jako `pfs1:`, a pliki znajdą się odpowiednio w `pfs1:/BOOT.ELF`, `pfs1:/BOOT.CNF`, `pfs1:/neutrino.elf`. Sprawdź, że kopiujesz do **PP.TESTDVR**, nie do katalogu DVR, partycji systemowej ani USB. Nie korzystaj z HDD Manager/Inject Header/Format. Jeśli kopiowanie w wLE zgłasza błąd, przerwij etap i podaj dokładny komunikat.

**Na razie nie kopiuj** pozostałych modułów, konfiguracji, ISO ani okładek. Nie zastępuj plików z działającej instalacji USB. Na tym etapie **nie uruchamiaj** HDD `BOOT.ELF` ani Neutrino — do prawdziwego startu potrzebna będzie dalsza diagnostyka ścieżek i zależności.

## Test DIAG H (tylko odczyt)

1. Pobierz z GitHub Actions artefakt `PSX-DVR-HDD-DIAG-H-STAGED-FILES-READONLY`, rozpakuj `PSX_DVR_HDD_DIAG_H.ELF` na USB.
2. Uruchom DIAG H przez wLE R3Z **z USB**. Program sam montuje jedynie `hdd0:PP.TESTDVR` → `pfs1:` z `FIO_MT_RDONLY`. Sprawdza otwarcie, rozmiar i pierwsze bajty trzech plików. Dla `BOOT.ELF` i `neutrino.elf` oczekuje nagłówka `0x7F ELF`.
3. Następnie DIAG H sprawdza jednocześnie `dvr_pfs0:/ISO` i PS2 `pfs1:/`, zamyka pliki, odmontowuje własne `pfs1:`, i wyświetla wynik oraz START: EXIT TO XMB/OSD.
4. Zrób zdjęcie finalnego ekranu wyników i naciśnij START. Potwierdź powrót do XMB. Przy zawieszeniu sfotografuj ostatni napis `H... BEFORE`; przy błędzie skopiuj jego wartość.

**Oczekiwany wynik:** `RESULT=0`, `MOUNT=0`, dodatnie rozmiary (`CNF bytes`, `ELF bytes`, `NEUTRINO bytes`), `DVR ISO>=0` i `PS2 PFS>=0`. `-2` teraz oznacza brak pliku, więc nie jest już oczekiwanym wynikiem.

Uwaga: `RESULT=0` potwierdza jedynie dostępność, wielkość i nagłówek odczytanych plików; to nie jest weryfikacja pełnych sum kontrolnych ani uruchomienia z HDD/XMB. DIAG H **nie ma wywołań zapisujących** ani funkcji startu gier. Jeśli synchronizacja RPC zawiśnie, START może nie zadziałać. `main`, v0.8 i DIAG E/F/G pozostają bez zmian.

---
## DIAG G — test łączony (PASS na konsoli DESR-7000)

Po potwierdzeniu przez użytkownika DIAG F (START → XMB) przygotowano osobny ELF PSX_DVR_HDD_DIAG_G.ELF. DIAG E i F pozostają niezmienione; v0.8 i main bez zmian.

**Pozostaw PP.TESTDVR pustą.** DIAG G montuje hdd0:PP.TESTDVR wyłącznie read-only jako pfs1:, sprawdza pliki BOOT.CNF, BOOT.ELF oraz neutrino.elf tylko do odczytu (na pustej partycji -2 jest oczekiwane), następnie uruchamia sterowniki DVR i sprawdza katalog dvr_pfs0:/ISO oraz ponownie pfs1:/. Odpina tylko własny mount przed ekranem wyników i czeka na START, aby wrócić do XMB/OSD.

**Test:** pobierz artefakt PSX-DVR-HDD-DIAG-G-RO-PAD-EXIT z GitHub Actions, skopiuj PSX_DVR_HDD_DIAG_G.ELF na USB, uruchom przez wLE R3Z i zrób zdjęcie wyników. Naciśnij START i potwierdź powrót do XMB. Jeśli program zastygnie, sfotografuj ostatni G... BEFORE. Blokujące wywołanie RPC może uniemożliwić reakcję na START.

**DIAG G PASS na DESR-7000 (2.11.2026).** Zrzut z testu: RESULT=0, MOUNT=0, trzy pliki=-2, DVR ISO=3, PS2 PFS=3, START wrócił do XMB. Przed DIAG H na PP.TESTDVR można skopiować wyłącznie trzy pliki wymienione w najnowszej instrukcji wyżej. DIAG G nie uruchamiał gier.

---
# PSX DVR ISO Manager v1.0 — diagnostyka HDD, etap A

**To nie jest Manager v1.0 ani instalator XMB.** To osobny, niewielki plik ELF do pierwszego, bezpiecznego testu odczytu partycji APA/PFS **`PP.TESTDVR`** i istniejącego `dvr_pfs0:/ISO` na Sony PSX DESR-7000.

## Wynik DIAG E na DESR-7000: oba systemy plików dostępne jednocześnie — PASS

Użytkownik potwierdził zdjęciem z prawdziwej konsoli:

```text
PSX DVR DIAG E / BOTH FILESYSTEMS
E13 MOUNT=0; E16 NOTICE=1
E17/E18 DVR MODULES STARTED
E19 DVR HDD DOPEN=3
E20 DVR PFS DOPEN=3
E21 DVR ISO DOPEN=3
E22 PS2 PFS1 DOPEN=3
E24 UMOUNT=0
E FINAL: result=0
PS2 MOUNT=0 AFTER=3
DVR HDD=3 PFS=3 ISO=3
```

**Wniosek:** `hdd0:PP.TESTDVR` (PS2 APA/PFS) oraz `dvr_pfs0:/ISO` (DVR) były równocześnie dostępne do odczytu w jednej sesji IOP. Nie jest to jeszcze test uruchomienia OSDMenu/Neutrino z HDD ani startu przez XMB.

### Następny test: DIAG F — przycisk START i wyjście do XMB/OSD

Użytkownik potwierdził, że Manager v0.8 po naciśnięciu START wraca do menu XMB/OSD. Aby nie zwiększać ryzyka i nie mieszać wyników IOP z przyciskami, zbudowaliśmy **oddzielny minimalny test `PSX_DVR_HDD_DIAG_F.ELF`**, który:

- wykonuje znaną inicjalizację IOP, `iomanX` i `fileXio` (jak w DIAG C);
- ładuje moduły kontrolera `sio2man`, `mcman`, `mcserv`, `freepad`, jak Manager v0.8;
- pokazuje komunikat **`PRESS START: EXIT TO XMB/OSD`**, a po świeżym naciśnięciu START zamyka pad i kończy działanie przez `return 0`;
- **nie otwiera, nie czyta, nie montuje ani nie zapisuje żadnego HDD/USB**; nie uruchamia DVR, OSDMenu ani gier.

**Przebieg:** Pobierz artefakt **`PSX-DVR-HDD-DIAG-F-PAD-EXIT`** z GitHub Actions, skopiuj ELF na USB, uruchom przez wLE R3Z, a po pojawieniu się komunikatu START naciśnij START. Potwierdź, czy konsola wróciła do XMB/OSD. Jeżeli zatrzyma się na `F... BEFORE`, zrób zdjęcie etapu. Jeśli pokazuje błąd uruchomienia kontrolera, powrót może nie być dostępny i nie należy zakładać, że test przeszedł.

**Ważne:** powrót do XMB działa na sprzęcie w Managerze v0.8, ale zachowanie osobnego ELF DIAG F wymaga dopiero sprawdzenia. Tak samo nie możemy zagwarantować działania przycisku, gdy program utknie wewnątrz blokującego RPC. **DIAG E pozostaje bez zmian.** Nie zmieniono `main`, oficjalnego wydania v0.8 ani zawartości partycji.

## Wynik DIAG D na DESR-7000: APA/PFS działa — PASS

Zdjęcie z rzeczywistej konsoli potwierdza:

```text
D11 hdd0: DOPEN=2 / DCLOSE=2
D12 pfs1 DOPEN=-19                  (jeszcze niezamontowane)
D13 MOUNT=0                         (PP.TESTDVR / FIO_MT_RDONLY)
D14 PFS1 ROOT DOPEN=3 / DCLOSE=3
D15 BOOT.CNF OPEN=-2                (plik nie istnieje na pustej partycji)
D16 UMOUNT=0
PS2 APA/PFS RO CHECK COMPLETE
```

**Wniosek:** `hdd0:` oraz montowanie `hdd0:PP.TESTDVR` na `pfs1:` w trybie tylko do odczytu działają na DESR-7000. `-19` przed montowaniem nie jest niepowodzeniem, a `-2` dla `BOOT.CNF` to prawidłowa informacja o braku pliku. Ten test **nie uruchamiał jeszcze modułów DVR**.

### Następny test: DIAG E — równoczesny PS2 APA/PFS i PSX DVR

Pobierz z GitHub Actions na tej gałęzi artefakt **`PSX-DVR-HDD-DIAG-E-APA-DVR-READONLY`**, rozpakuj i skopiuj **`PSX_DVR_HDD_DIAG_E.ELF`** na USB.

1. Uruchom ELF przez **wLaunchELF R3Z**, tak jak DIAG D. Nie trzeba kopiować żadnego pliku na `PP.TESTDVR`.
2. Program wykorzysta sprawdzoną sekwencję resetu IOP i sterowniki `iomanX`, `fileXio`, `ps2dev9`, `ps2atad`, `ps2hdd`, `ps2fs`.
3. Następnie wyświetli odczytowe sprawdzenie `hdd0:` (**E11**), kontrolę zajętości `pfs1:` (**E12**), montowanie `PP.TESTDVR` tylko do odczytu (**E13**) i odczyt katalogu `pfs1:/` (**E14**).
4. **E15–E18** uruchomią procedury `sceCdInit`, `sceCdNoticeGameStart`, `dvrdrv`, `dvrfile` (kolejność inspirowana RepairBox). Program nie ładuje jednocześnie drugiej wersji DEV9.
5. **E19–E22** sprawdzą odczytowo `dvr_hdd0:`, `dvr_pfs0:/`, **`dvr_pfs0:/ISO`** oraz ponownie `pfs1:/` po inicjalizacji DVR.
6. Program odmontuje wyłącznie własne `pfs1:` (**E24**), a na ekranie pozostawi wyniki obu systemów plików.

**Jeśli program się zawiesi**, zrób zdjęcie ostatniego `E... BEFORE`; są przerwy przed operacjami. Jeśli zakończy się, zrób zdjęcie ekranu z wartościami `DVR HDD`, `PFS`, `ISO` i `PS2 MOUNT`.

**Bezpieczeństwo:** E nie otwiera plików do zapisu i nie tworzy, nie formatuje ani nie naprawia żadnej partycji. Jedyny mount to **`hdd0:PP.TESTDVR` → `pfs1:`, `FIO_MT_RDONLY`**. Katalog gier DVR jest tylko odczytywany. Program zmienia działające sterowniki IOP w pamięci, więc po teście **zrestartuj konsolę**. Sama inicjalizacja dyskowych sterowników zawsze niesie pewne ryzyko na nietypowym sprzęcie; przed testami nowych wersji warto mieć kopię danych.

Po pozytywnym E przejdziemy do diagnostyki plików `BOOT.ELF`, `BOOT.CNF` i Neutrino na `PP.TESTDVR` i samego błędu OSDMenu Launchera. Na razie **nie wykonujemy żadnego uruchamiania gry ani instalacji kanału XMB**.

## Wynik DIAG C na prawdziwym DESR-7000: SUKCES

Zdjęcie z konsoli potwierdza:

```text
STEP C1 IOP RESET: SENT
STEP C2 IOP SYNC: DONE
STEP C3 RPC+PATCHES: DONE
STEP C4 iomanX: id=25 result=0
STEP C5 fileXio: id=26 result=0
STEP C6 fileXioInit=0
STOP C: result=0
```

Wniosek: uruchomienie nowego, własnego stosu IOP (reset, `iomanX`, `fileXio`) rozwiązuje zawieszenie testów A/B podczas `fileXioInit`. **Ten test nie sprawdzał jeszcze HDD, PFS ani DVR.**

### Następny krok: DIAG D — APA/PFS w trybie tylko do odczytu

Pobierz nowy artefakt GitHub Actions **`PSX-DVR-HDD-DIAG-D-APA-PFS-READONLY`**, rozpakuj i skopiuj **`PSX_DVR_HDD_DIAG_D.ELF`** na USB.

1. Upewnij się, że samodzielnie utworzyłeś na obszarze PS2/APA partycję `PP.TESTDVR` za pomocą wLaunchELF R3Z. **Nie trzeba na nią jeszcze kopiować żadnych plików**. Przed jakimikolwiek dalszymi zmianami APA należy mieć kopię zapasową danych.
2. Uruchom diagnostyczny ELF D przez **wLaunchELF R3Z**. Po resecie IOP program ładuje moduły `iomanX`, `fileXio`, `ps2dev9`, `ps2atad`, `ps2hdd`, `ps2fs` (parametry z RepairBox), a następnie wywołuje `fileXioInit()`.
3. Na kolejnym ekranie zobaczysz **`DRIVERS LOADED / IOP RPC READY`** oraz etapy:
   - **D11** — odczyt katalogu partycji PS2 `hdd0:`;
   - **D12** — sprawdzenie, czy `pfs1:` jest już zamontowane;
   - **D13** — montowanie **wyłącznie `hdd0:PP.TESTDVR`** do `pfs1:` z **`FIO_MT_RDONLY`**;
   - **D14** — odczyt głównego katalogu `pfs1:/`;
   - **D15** — próba odczytowego otwarcia `BOOT.CNF` (na pustej partycji brak pliku jest normalny);
   - **D16** — odmontowanie `pfs1:`, ale tylko jeśli program sam je zamontował.
4. Zrób zdjęcie ekranu. Jeśli nastąpi zawieszenie, **ostatni napis `BEFORE` określi etap**. Jeżeli wynik montowania będzie ujemny, przekaż dokładną wartość (np. `D13 MOUNT=-19`).

**Bezpieczeństwo:** DIAG D nie ma funkcji zapisu, tworzenia ani formatowania partycji; nie uruchamia gry, OSDMenu, Neutrino ani instalatora XMB. Resetuje tylko IOP (pamięć RAM), dlatego po teście najlepiej zrestartować konsolę. **D celowo nie ładuje jeszcze sterowników DVR i nie dotyka `dvr_pfs0:/ISO`**. Jeżeli montowanie PS2 APA/PFS zadziała, następny niezależny etap zbada jednoczesne korzystanie z partycji DVR i PS2. Nie zmieniamy wydania v0.8.

## Wynik testu DESR-7000 (DIAG B): blokada fileXioInit

Na rzeczywistej konsoli DESR-7000 diagnostyka B zatrzymała się w tym miejscu:

```text
STEP 1 SifInitRpc: BEFORE
STEP 1: DONE
STEP 2 fileXioInit: BEFORE
```

Nie pojawił się komunikat `STEP 2: RESULT=...`. To wskazuje na zablokowanie inicjalizacji klienta `fileXio`, **zanim** uruchomiono jakikolwiek test `hdd0:`, DVR lub `PP.TESTDVR`.

### Następny test: PSX_DVR_HDD_DIAG_C.ELF

Pobierz nowy artefakt **`PSX-DVR-HDD-DIAG-C-FILEXIO`** z GitHub Actions na tej gałęzi. Zawiera `PSX_DVR_HDD_DIAG_C.ELF` i niniejszą instrukcję.

1. Uruchom `PSX_DVR_HDD_DIAG_C.ELF` z USB przez **wLaunchELF R3Z**.
2. Ta wersja **resetuje wyłącznie procesor IOP i jego moduły w RAM** (tak jak działający Manager v0.8), inicjalizuje RPC i ładuje wbudowane `iomanX.irx` oraz `fileXio.irx` przed `fileXioInit()`. To celowa różnica względem poprzednich testów, które zakładały zachowany stos IOP po wLE.
3. Oczekiwane etapy: `STEP C1 IOP RESET`, `STEP C2 IOP SYNC`, `STEP C3 RPC+PATCHES`, `STEP C4 iomanX`, `STEP C5 fileXio`, `STEP C6 fileXioInit`.
4. Zrób zdjęcie, na którym widać ostatni etap oraz kody `id`, `result` i `fileXioInit`. Szczególnie ważne, czy `STEP C6 fileXioInit=...` w ogóle się pojawi.

**C nie montuje żadnej partycji, nie odczytuje i nie zapisuje na HDD, nie uruchamia gier i nie zmienia XMB.** Nie próbuje jeszcze sprawdzać `PP.TESTDVR` ani `dvr_pfs0:/ISO`. Reset IOP zmienia jedynie uruchomione sterowniki w pamięci — po teście uruchom ponownie konsolę lub wLE, żeby przywrócić normalny stan IOP. Nie jest to test bez resetowania IOP jak A i B.

Jeśli C poprawnie wykona `fileXioInit`, będziemy mogli zbudować kolejny test wspólnej inicjalizacji PS2 HDD oraz DVR i dopiero potem wrócić do montowania `PP.TESTDVR` oraz błędu OSDMenu `-19`.

## Gdy diagnostyka A nie pokazuje wartości

Zbudowano dwa dodatkowe, niezależne pliki ELF, żeby **odróżnić problem z wyświetlaniem od zablokowania wywołań IOP/HDD**. Są w artefakcie GitHub Actions **`PSX-DVR-HDD-DIAG-B-STAGED`**.

1. **Najpierw uruchom `PSX_DVR_HDD_HELLO.ELF` z wLaunchELF R3Z.** Powinien pokazać napis `SCREEN INITIALIZED SUCCESSFULLY`. Nie wywołuje sieci, fileXio ani sterowników HDD. Jeśli także pozostaje czarny ekran, problem występuje jeszcze przed diagnostyką montowania PFS. Zrób zdjęcie.
2. Jeśli HELLO pokazuje tekst, uruchom **`PSX_DVR_HDD_DIAG_B.ELF`**. Program wyświetla kolejne etapy `STEP 1` do `STEP 8`; przed potencjalnie blokującym wywołaniem wyświetla `BEFORE` i robi krótką przerwę, aby napis pozostał widoczny także przy zawieszeniu.
3. **Zanotuj ostatni widoczny `STEP ... BEFORE`**, jeśli program zastygnie; jeżeli pokaże `MOUNT RESULT=-19`, zrób zdjęcie pełnego ekranu. Nie trzeba zgadywać, czy błąd dotyczy APA, PFS, czy wcześniejszej inicjalizacji.
4. Oba pliki są **wyłącznie do odczytu**; B sprawdza tylko `hdd0:`, `dvr_pfs0:/ISO` i ręcznie przygotowaną `hdd0:PP.TESTDVR` jako `pfs1:`. Programy nie resetują IOP ani nie modyfikują XMB.

Jeśli pierwszy ELF wyświetlał nagłówek, ale nie liczby, również uruchom B: pokaże, w którym dokładnie kroku zatrzymał się odczyt.

## Zasady bezpieczeństwa

- **`PP.TESTDVR` tworzysz samodzielnie w wLaunchELF R3Z**, na obszarze PS2/APA dysku; diagnostyka nigdy jej nie tworzy ani nie formatuje.
- Program ma tylko wywołania odczytowe. `fileXioMount("pfs1:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)` montuje wskazaną partycję **tylko do odczytu**.
- **Nie dotyka `dvr_pfs0:/ISO` w trybie zapisu**, nie zmienia APA ani systemowych partycji `__*`, nie tworzy wpisu w XMB, nie zmienia `BOOT.CNF`, nie modyfikuje USB.
- Nie resetuje IOP, nie doładowuje IRX ani nie zastępuje sterowników działającego wLaunchELF R3Z. Jeżeli uruchomisz go bez odpowiedniego stosu IOP, wyniki będą błędami — i to jest ważna informacja diagnostyczna.
- Korzysta z punktu `pfs1:`, aby nie przejmować `pfs0:` używanego przez wLE. Jeśli `pfs1:` już jest zajęty, **przerywa test tej partycji**, nie odmontowuje cudzego systemu plików.
- Jedynym odmontowywanym systemem plików jest `pfs1:`, jeśli diagnostyka samodzielnie pomyślnie go zamontowała.

## Pierwszy test

1. Przygotuj działającą kopię zapasową konfiguracji i, najlepiej, pełną kopię HDD przed eksperymentami z APA. Nie korzystaj z opcji formatowania całego dysku.
2. W **wLaunchELF R3Z** ręcznie załóż partycję **dokładnie `PP.TESTDVR`** w obszarze PS2, typu **PFS**. Na razie pozostaw ją **pustą**. Upewnij się, że inne ważne partycje są bez zmian.
3. Pobierz artefakt **`PSX-DVR-HDD-DIAG-A-READONLY`** z GitHub Actions z tej gałęzi, rozpakuj ZIP i skopiuj sam **`PSX_DVR_HDD_DIAG_A.ELF`** na USB.
4. Uruchom ELF **z wLaunchELF R3Z**, nie z XMB. Przy uruchamianiu nie odłączaj USB; urządzenie nie jest używane do danych, ale jest źródłem startowego ELF.
5. Zrób **zdjęcie całego ekranu wyników**, w szczególności:
   - `Attach fileXio` — dostęp do istniejącego sterownika RPC;
   - `PS2 APA hdd0:` — lista partycji PS2;
   - `DVR ISO` — sprawdzenie folderu ISO **tylko do odczytu**;
   - `Mount hdd0:PP.TESTDVR -> pfs1:` — rzeczywisty kod montowania, **bez zamiany błędów na -19**;
   - `PP.TESTDVR root` — czy zawartość testowej partycji jest widoczna.
6. Podeślij zdjęcie z diagnostyki. Ekran pozostaje wyświetlony do wyłączenia/restartu; aplikacja nie uruchamia gry.

**To normalne**, że przy pustej partycji próby otwarcia `BOOT.ELF`, `BOOT.CNF`, `neutrino.elf` i `DVR_ISO_MANAGER.ELF` pokażą kody ujemne. Na tym etapie liczy się przede wszystkim poprawne **montowanie read-only** i jednoczesny dostęp do `hdd0:` i `dvr_pfs0:/ISO`.

## Co dalej

Po potwierdzeniu montowania zaczniemy etap B: diagnostykę przekazywania poprawnej ścieżki `BOOT.CNF` do OSDMenu Launchera, bez utraty kodu błędu PFS. Dopiero później przygotujemy wersję Managera, w której wszystkie CNF, moduły, okładki i ELF są na `PP.TESTDVR`. Instalator pozycji XMB zbadamy osobno. **Wydanie v0.8 pozostaje nietknięte.**
