## DIAG Z — zdjęcie z PSX: CHECK=-136 z powodu konkretnej nazwy ISO, bez błędu CNF

Na fizycznej DESR-7000 użytkownik uruchomił Z przez UDPFS/wLE. Wynik zdjęcia:

```text
CHECK=-136 MOUNT=0
CNF=91 ELF=197396 NEUT=75140
SYS=1 BSD=1 DVD=1 USB=0
ISO=3 DVR=3 PFS=3
ZCNF=105 TARGET ELF BYTES=154676
SELECTED ISO OPEN=-2
GAME: dvr_pfs0:/ISO/MGS 2.iso
ZCNF PREFIX EXACT. LAST=00 NUL=1
PRECHECK FAILED; X DISABLED
```

**Ustalenie:** `ZCNF=105` jest zgodnym plikiem, a `TARGET ELF BYTES=154676` oznacza obecność Z_GAME. Jedyną wykazaną przeszkodą jest `SELECTED ISO OPEN=-2` (`ENOENT`) przy tej **konkretnej** ścieżce. `ISO=3 DVR=3` potwierdza, że katalog `dvr_pfs0:/ISO` i sterownik DVR są już dostępne w samym launcherze — **nie** podmieniać ścieżki na `pfs0:/ISO`, bo to inny filesystem. Nie wyłączać bramki `CHECK` bez znajomości rzeczywistej nazwy ISO.

**Poprawka na branchu, tylko launcher Z:** gdy otwarcie wybranego pliku zwraca `-2`, wykonuje `fileXioDopen("dvr_pfs0:/ISO")` i do 256 wywołań `fileXioDread`, zapisując pierwsze osiem nazw i liczbę pozycji. Końcowy ekran pokazuje `DVR DIR OPEN`, `READ`, `SEEN` i `DVR FILE 1..8`. Operacje są tylko do odczytu; `CHECK` pozostaje ujemne, jeśli brak dokładnej nazwy. Nie zmienia się `Z_GAME.ELF`, CNF, aktualny argument `-dvd=dvr_pfs0:/ISO/...`, USB v0.8 ani `BOOT.CNF`. Po odczycie prawdziwej nazwy operator poprawi tylko nazwę pliku w `DIAG_Z.CNF`.

---

## DIAG Z — pierwsza próba uruchomienia prawdziwego ISO z HDD po sprzętowym Y2P PASS

**Potwierdzenie Y2P na fizycznej DESR-7000 (3.11.2026):** `Y2 ACTUAL SELECT idx=3 dvrfile_fhi.irx`, `Y2 87 DVR PRE=1 POST=0 CHOSEN=3`, `Y90 CDVDMAN SETTINGS=1`, `Y2 95 DVR FHI+CDVD READY / SAFE STOP` i `V RESULT=0 LAST_STAGE=95`. Ta faza była uruchomiona przez rzeczywisty OSDMenu z HDD (`CHECK=0 → X` w launcherze UDPFS), ale **nie wykonywała odczytu ISO ani gry**.

**DIAG Z to NOWY ETAP O WIĘKSZYM RYZYKU:** użytkownik poprosił, by po PASS Y2 spróbować uruchomić faktyczne ISO. Z bazowego pinned `pcm720/neutrino:bsd-dvr` budujemy oddzielną, niespakowaną wersję `PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF` z proven `fileXioMount("pfs0:", "hdd0:PP.TESTDVR", FIO_MT_RDONLY)`, `chdir("pfs0:/")`, diagnostyką FHI i bez sztucznego `Y95` STOP. Pozwalamy wykonać **oryginalny kod Neutrino**: `setup_dvd_iso`, odczyt `SYSTEM.CNF`, przełączenie do Emulation Environment i `ExecPS2` (gra). Dodaliśmy markery `Z96`, `Z97`, `Z98`, `Z99` przed kolejnymi operacjami, żeby móc rozpoznać nieudane etapy. Osiągnięcie `Z99` samo nie oznacza jeszcze działania gry — wymagany test obrazu i kontrolerów.

### Bezpieczny wybór ISO

Zawartość nowego osobnego pliku `DIAG_Z.CNF` (szablon):
```ini
boot=PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/NAZWA_GRY.iso
arg=-dev9=NICHDD
#PAD
```

**Użytkownik edytuje WYŁĄCZNIE `NAZWA_GRY.iso`**, zastępując je dokładną nazwą istniejącego pliku ISO z `dvr_pfs0:/ISO/`; np. jeśli w folderze rzeczywiście jest `SLUS_000.00.iso`, wpisuje tę nazwę, bez cudzysłowów. To tylko przykład — nie deklarować, że taki plik jest na dysku. Zachować wszystkie inne znaki, kolejność argumentów i pięć linii. Zapis w UTF-8 **bez BOM**, końce wierszy **LF**, końcowy LF po `#PAD`; nie używać CRLF. Obsługiwane są nazwy prostych plików ASCII o długości 5–95 znaków, zakończone `.iso` lub `.ISO`; ukośniki, dwukropki, `..`, `=` i inne urządzenia są odrzucane. Nie tworzyć nowego katalogu ani nie kopiować gry, jeśli już znajduje się na DVR.

- `PSX_DVR_HDD_NEUTRINO_Z_LAUNCH.ELF` **uruchamiamy z UDPFS/wLE-R3Z**, jak w Y2P; nadal używa oryginalnego OSDMenu `BOOT.ELF` z HDD. Otwiera i czyta `DIAG_Z.CNF` przez tylko RO PFS, weryfikuje dokładny `boot=`, `arg=-bsd=dvr`, stały prefiks `arg=-dvd=dvr_pfs0:/ISO/`, dozwoloną pojedynczą nazwę pliku ISO i `arg=-dev9=NICHDD`; **nie** dopuszcza dodatkowych argumentów. `#PAD` na końcu musi być zgodne, a wyłącznie jego ostatni bajt dopuszcza LF/NUL/01 (sprzętowo zaobserwowana anomalia). Weryfikuje istnienie **wybranego, rzeczywistego obrazu ISO** przez `fileXioOpen(..., FIO_O_RDONLY)`, zamyka deskryptor, a dopiero `CHECK=0` udostępnia X. W razie błędnej nazwy pliku pokaże ujemny kod `SELECTED ISO OPEN` i zablokuje X.
- `PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF` samodzielnie sprawdza po otrzymaniu przez OSDMenu, że `argv[0]` wskazuje jedynie `hdd0:PP.TESTDVR:pfs:/`, `argv[1]=-bsd=dvr`, a `argv[2]` zaczyna się od `-dvd=dvr_pfs0:/ISO/` i zawiera jedynie dozwoloną prostą nazwę ISO. Nie pozwala używać żadnych innych partycji jako miejsca przygotowania czy zewnętrznych modułów. Montuje tylko testową partycję jako RO, ale **gra zostaje naprawdę uruchomiona i jej własne zachowanie (np. zapis karty pamięci) wykracza poza kontrolę diagnostyki**.
- Jeżeli ISO jest prawidłowe i fork Neutrino wystartuje, Z może przekazać sterowanie `ExecPS2` bez powrotu do testu. Jeśli nastąpi powrót/error, wynik `V RESULT` i ostatni widoczny marker mogą wskazać etap niepowodzenia.

### Procedura

1. Zabezpieczyć ważne dane, zamknąć pozostałe zadania, upewnić się, że na `dvr_pfs0:/ISO/` jest prawidłowy obraz PS2, który użytkownik może legalnie uruchomić.
2. Edytować na PC **tylko nazwę ISO** w `DIAG_Z.CNF`; nie zmieniać `-dev9=NICHDD` ani końcowego `#PAD`. Zapis UTF-8 bez BOM/LF.
3. Na `hdd0:PP.TESTDVR` skopiować nowe `PSX_DVR_HDD_NEUTRINO_Z_GAME.ELF` i dopasowany `DIAG_Z.CNF`. **NIE nadpisywać** `BOOT.ELF`, `BOOT.CNF`, `neutrino.elf`, `config/`, `modules/`, żadnego ISO i nie dotykać systemowych partycji Sony. Nie ruszać USB v0.8.
4. Uruchomić `PSX_DVR_HDD_NEUTRINO_Z_LAUNCH.ELF` z UDPFS w wLE-R3Z; sprawdzić `CHECK=0`, `SELECTED ISO OPEN>=0` i poprawną wyświetloną nazwę. **Dopiero wtedy nacisnąć X**, świadomie zgadzając się na realny start gry przez oryginalny OSDMenu→Neutrino.
5. Zrobić zdjęcie pierwszych markerów `Z95`, `Z96`, `Z97`, `Z98` lub `Z99` jeżeli są widoczne; w razie zawieszenia podać ostatni marker. Jeśli gra uruchomi się, sprawdzić ekran menu gry i sterowanie; nie wnioskować o działaniu po samym `Z99`.
6. Po zakończeniu testu może być konieczny restart konsoli. Przestrzegać wszystkich instrukcji dotyczących kopii danych.

**To nie jest instalacja produkcyjnej v1.0 HDD.** Jest to osobny, eksperymentalny test możliwego uruchomienia gry; do wydania HDD potrzebne będą jeszcze dopracowanie Managera, samodzielny start bez UDPFS, stabilność i regresja. Draft PR #26 nadal niepołączony, stabilne main/USB v0.8 bez zmian.

---

## DIAG Y2P — zabezpieczony wariant CNF po `LAST=01` (3.11.2026)

**Rzeczywisty wynik sprzętowy launchera Y2:** użytkownik uruchomił Y2 przez UDPFS/wLE, na pierwszym ekranie:
```text
PSX DVR NEUTRINO Y2 / RESULTS
CHECK=-136 MOUNT=0
CNF=91 ELF=197396 NEUT=75140
SYS=1 BSD=1 DVD=1 USB=0
ISO=3 DVR=3 PFS=3
Y2CNF=-344 TARGET ELF BYTES=152116
Y2CNF PREFIX EXACT. LAST=01 NUL=0
PRECHECK FAILED; X DISABLED
```
Tak więc **Neutrino Y2 w ogóle nie został uruchomiony**. Launcher wykrył dokładne dopasowanie pierwszych 108 z 109 bajtów `DIAG_Y2.CNF`, lecz ostatni bajt został odczytany jako `0x01` zamiast `0x0A` (LF). W dawnym R2 ostatni LF bywał odczytywany jako `0x00`. Nie ma dowodu, czy wynika to z transferu, odczytu fileXio czy zapisu na dysku; nie akceptujemy żadnego `0x01` w aktywnych argumentach.

**Osobna konfiguracja Y2P** omija tylko niepewny końcowy bajt poprzez **dodatkową linię bez `=`** na samym końcu, bez zmiany właściwych argumentów:
```ini
boot=PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso
arg=-dev9=NICHDD
#PAD
```
W `pcm720/OSDMenu` `launcher/src/handler_quickboot.c` parser pobiera linie przez `fgets`, po czym gdy **nie znajduje `=`**, przechodzi do następnej: `if (!valuePtr) continue;`. Oznacza to, że cała dodatkowa linia `#PAD` jest pomijana, a `-dev9=NICHDD` kończy się normalnym LF jeszcze przed tą linią.

**Rozdzielenie ryzyka:** `DIAG_Y2P.CNF` ma 114 bajtów; nowy `PSX_DVR_HDD_NEUTRINO_Y2P_LAUNCH.ELF` wymaga całych 113 pierwszych bajtów **byte-for-byte**, w tym kompletnego boot=, dummy -dvd, `-dev9=NICHDD\n` oraz `#PAD`. **Wyłącznie ostatni bajt nieaktywnej linii** może mieć wartość `0x0A`, `0x00` lub zaobserwowane `0x01`; każda inna wartość blokuje X. Nie luzujemy porównania argumentów. Współdzielony z Y2 `PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF` pozostaje **identyczny** i zachowuje hard stop przed `setup_dvd_iso`: brak ISO i gry.

**Test po udanym CI:** skopiować do `hdd0:PP.TESTDVR` **tylko nowy `DIAG_Y2P.CNF`** obok istniejącego `PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF` (jeżeli Y2 FHI.ELF nadal jest obecny; nie trzeba kopiować ponownie); uruchomić osobny `PSX_DVR_HDD_NEUTRINO_Y2P_LAUNCH.ELF` z UDPFS/wLE. Weryfikacja, czy `CHECK=0` i `Y2PCNF=114`; tylko wtedy X. Zrobić zdjęcie `Y2 PRE`, `Y2 ACTUAL SELECT`, `Y2 95` i końcowego `V RESULT/LAST_STAGE`. `LAST=01` tym razem może być bezpiecznie zaakceptowany wyłącznie w pomijanej linii. Jeśli inna wartość, nie naciskać X i zgłosić wynik. Oryginalne BOOT.CNF/BOOT.ELF, Neutrino, ISO, USB v0.8, partycje Sony i stabilny main bez zmian; PR #26 Draft.

**Plan po sukcesie Y2P:** dopiero gdy sprzętowo zweryfikujemy `Y2 ACTUAL SELECT idx=3 dvrfile_fhi.irx`, `Y2 95` i `V RESULT=0 LAST_STAGE=95`, przygotować osobny bezpiecznie bramkowany build uruchamiający rzeczywiste ISO; nie zmieniać fikcyjnego -dvd w diagnostyce Y2P. Przed uruchomieniem gry użytkownik powinien wskazać rzeczywistą nazwę ISO w `dvr_pfs0:/ISO/` bądź bezpiecznie ją pobrać z jego własnego `BOOT.CNF`.

---

## DIAG Y — rzeczywisty wynik sprzętowy Y88 i wyjaśnienie błędu; DIAG Y2 (3.11.2026)

Użytkownik uruchomił Y normalną ścieżką **wLE-R3Z/UDPFS → Y_LAUNCH.ELF → CHECK=0 → X → oryginalny OSDMenu → Y_FHI.ELF z PP.TESTDVR**. Zdjęcie:

```text
Y85 LE MODULES READY / FHI NEXT
Y86 FHI CONFIG INIT BEGIN
Y87 FHI ACTIVE=1 INIT=0
Y87 DVR FHI MOD=1 SET=0
Y88 ERROR DVR FHI MISSING
V RESULT=-88 LAST_STAGE=88
NO GAME/ISO EXECUTED.
```

**Wyjaśnienie w kodzie pcm720:** `ee/loader/src/fhi_config.c` `fhi_config_init()` iteruje po `fhi_bd.irx`, `mmcefhi.irx`, `udpfs_fhi.irx`, `dvrfile_fhi.irx`, wywołuje `module_get_settings()`, a następnie **`memset(s,0,sizeof(settings))` zeruje cały bufor wybranego modułu, również `MODULE_SETTINGS_MAGIC`**. Wcześniejszy DIAG Y wywołał `module_get_settings(dvrfile_fhi)` **po inicjalizacji** i zobaczył `SET=0` — może to być skutek prawidłowej inicjalizacji właśnie modułu DVR! W takim przypadku Y88 jest **fałszywym alarmem diagnostyki**, a nie uszkodzeniem modułu. Jednak `FHI ACTIVE=1` samo nie wskazuje, który backend wybrano. W `iop/dvrfile/src/dvrfile_fhi.c` fork pcm720 rzeczywiście zawiera `struct fhi_fileid fhi = {MODULE_SETTINGS_MAGIC}`. Nie podmieniamy żadnych modułów na dysku na ślepo.

**DIAG Y2** używa dokładnie tego samego Neutrino w wariancie X→Y, z osobnym `patch_y2_fhi_preinit.py`:
- **PRZED `fhi_config_init`** zapisuje wynik `module_get_settings` każdego z czterech backendów, czy w ogóle znajduje się na liście modułów, rozmiar modułu i to, który **pierwszy** posiada oczekiwany magic.
- W prawdziwym `fhi_config_init()` wstrzykuje `Y2 ACTUAL SELECT idx=... <nazwa>`, więc odnotuje faktyczny wybór (0: `fhi_bd`, 1: `mmcefhi`, 2: `udpfs_fhi`, **3: `dvrfile_fhi`**).
- Po inicjalizacji pokaże `Y2 87 DVR PRE=... POST=... CHOSEN=...`. `POST=0` po poprawnym wyborze `dvrfile_fhi` jest **oczekiwane** — init wyzerował magic. Twardy błąd `Y2 88 WRONG FHI BACKEND` pojawi się tylko, jeśli nie wybrano DVR lub FHI nie jest aktywne.
- Następnie oryginalny kod sprawdzi `cdvdman_emu.irx` (`Y90`), a test zakończy się przy `Y2 95 DVR FHI+CDVD READY / SAFE STOP` z `V RESULT=0 LAST_STAGE=95`.
- Dalej **bezwzględny STOP przed `setup_dvd_iso`**. Żaden obraz ISO nie jest otwierany, nic się nie zapisuje, gry nie startują. Rzeczywiste przełączanie IOP/SCmd/LE pozostaje takie samo jak fizycznie sprawdzony X85; moduły zewnętrzne mogą mimo to mieć własne skutki uboczne.

**Instrukcja:** na `PP.TESTDVR` skopiować obok poprzednich plików **wyłącznie** `PSX_DVR_HDD_NEUTRINO_Y2_FHI.ELF` i `DIAG_Y2.CNF`; `PSX_DVR_HDD_NEUTRINO_Y2_LAUNCH.ELF` uruchomić poprzez UDPFS i wLE-R3Z. Przy `CHECK=0` sfotografować ekran, nacisnąć X. Zrobić zdjęcie `Y2 PRE`, `Y2 ACTUAL SELECT` i `V RESULT/LAST_STAGE`. Przy zatrzymaniu sfotografować ostatnią linię. Nie nadpisywać oryginalnego `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, `config/`, `modules/`, działającego USB v0.8 ani partycji Sony.

**Decyzja o następnym etapie:** użytkownik chce **po fizycznym PASS Y2** przejść do uruchomienia rzeczywistego ISO. Najpierw musi pojawić się `Y2 ACTUAL SELECT idx=3 dvrfile_fhi.irx`, `Y2 95`, `V RESULT=0 LAST_STAGE=95`. Sama zielona kompilacja lub `CHECK=0` nie wystarczają. W następnym, *osobnym* i kontrolowanym buildzie dopiero użyjemy prawdziwej ścieżki ISO z `BOOT.CNF` i sprawdzimy przejście do gry. Nie dopuścić automatycznego startu gry w diagnostyce Y2.

---

## DIAG Y — backend FHI Neutrino pcm720 po sprzętowym PASS X85 (3.11.2026)

**Potwierdzony tryb testowania X:** Użytkownik uruchomił `PSX_DVR_HDD_NEUTRINO_X_LAUNCH.ELF` z UDPFS przez wLE-R3Z, zobaczył `CHECK=0`, nacisnął X; oryginalny OSDMenu uruchomił plik X_LE.ELF z `hdd0:PP.TESTDVR`. Na fizycznej konsoli widoczne były `X73 OSDCNF RESET SYNC OK`, `X75 DVR SCMD COMPLETED`, `X85 LE MODULES DONE / STOP`, `V RESULT=0 LAST_STAGE=85`. **To jest pełne potwierdzenie oryginalnego łańcucha OSDMenu → Neutrino na HDD do fazy LE**, ale bez ISO i gry.

**Cel DIAG Y:** sprawdzić **kolejny etap oryginalnego kodu** z `pcm720/neutrino:bsd-dvr`: `fhi_config_init(&drv.mod)` (wybór FHI backendu), obecność wczytanego `dvrfile_fhi.irx` i jego struktur ustawień, obecność `cdvdman_emu.irx` z ustawieniami. Y nie otwiera żadnego ISO — `-dvd=` zawiera tak jak poprzednio **fikcyjny** `dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso`.

### Sposób działania DIAG Y

- CI używa **tego samego przypiętego commita pcm720** co X, budując efemeryczną kopię Neutrino z `patch_probe.py` → `patch_w_entry_hold.py` → `patch_x_dvr_le.py` → nowym `patch_y_fhi.py`.
- Zamiast zakończyć na X85, Y przechodzi do oryginalnego `fhi_config_init` i `module_get_settings`, z markerami:
  - `Y85 LE MODULES READY / FHI NEXT`
  - `Y86 FHI CONFIG INIT BEGIN`
  - `Y87 FHI ACTIVE=1 INIT=0` i `Y87 DVR FHI MOD=1 SET=1` (pozytywny stan; kody błędów są raportowane)
  - `Y90 CDVDMAN SETTINGS=1`
  - `Y95 DVR FHI+CDVD READY / SAFE STOP` oraz `V RESULT=0 LAST_STAGE=95` (oczekiwany sukces).
- `patch_y_fhi.py` dodatkowo weryfikuje, że wcześniejsze backendy FHI `fhi_bd`, `mmcefhi`, `udpfs_fhi` nie są aktywne, aby uniknąć fałszywego PASS dla innego sterownika. Błąd pokazuje ostatni marker i kod `V RESULT`.
- **Twardy stop przed pierwszym `setup_dvd_iso`**: brak otwarcia nawet fikcyjnego ISO, brak odczytu `SYSTEM.CNF` z obrazu, brak startu gry. Na końcu skończona pętla CPU z W utrzymuje wynik na ekranie.
- Tak jak X, Y **wykonuje prawdziwą sekwencję PSX DVR LE IOP reset / SCmd 0x29 / start modułów**; to jest etap o większym ryzyku niż W i może wymagać ponownego uruchomienia konsoli.

### Instrukcja (dopiero po pomyślnym CI)

1. Utworzone zostaną trzy osobne pliki: `PSX_DVR_HDD_NEUTRINO_Y_FHI.ELF`, `DIAG_Y.CNF`, `PSX_DVR_HDD_NEUTRINO_Y_LAUNCH.ELF`. Na partycję `hdd0:PP.TESTDVR` skopiować **tylko** Y_FHI.ELF i DIAG_Y.CNF, obok istniejących; niczego nie nadpisywać.
2. Uruchomić Y_LAUNCH.ELF z UDPFS/wLE-R3Z. Na preflight `CHECK=0` sfotografować ekran, potem X uruchomi oryginalny OSDMenu i Y_FHI z HDD.
3. Sfotografować `Y87`, `Y90`, `Y95`, `V RESULT=... LAST_STAGE=...`. Jeśli zatrzyma się na `X...` lub `Y86`, sfotografować ostatni widoczny etap.
4. Bezpieczniki: brak zmian w `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, działającym USB v0.8 i prawdziwym ISO; tylko RO mount `PP.TESTDVR` i odczyty z niej w programie. Używane oryginalne sterowniki IOP DVR mogą wykonać wewnętrzne operacje poza kontrolą testu. Warto mieć kopię danych.
5. `main` oraz stabilne v0.8 są nietknięte. Draft PR #26 pozostaje niepołączony.

### Następna faza

Dopiero po sprzętowym PASS Y95 ma sens test otwierania rzeczywistego obrazu ISO. Do tego będzie potrzebna ścieżka do konkretnego istniejącego ISO w `dvr_pfs0:/ISO/`. Nie zastępujemy fikcyjnej ścieżki prawdziwą w diagnostyce Y, aby nie ryzykować niekontrolowanego uruchomienia gry.

---

## Uzupełnienie potwierdzenia sprzętowego DIAG X — rzeczywisty łańcuch OSDMenu z UDPFS do HDD (3.11.2026)

Użytkownik **jednoznacznie potwierdził sposób uruchomienia zdjęcia X85**: `PSX_DVR_HDD_NEUTRINO_X_LAUNCH.ELF` został uruchomiony **z UDPFS przez wLE-R3Z**, na ekranie był `CHECK=0`, następnie użytkownik nacisnął **X**. Nie uruchamiał bezpośrednio X_LE.ELF z wLE. Tym samym zdjęcie `X85 LE MODULES DONE / STOP`, `V RESULT=0 LAST_STAGE=85` pochodzi **z prawdziwej ścieżki launchera diagnostycznego → oryginalny OSDMenu → X Neutrino na hdd0:PP.TESTDVR**, nie jedynie z bezpośredniego wykonania ELF.

**Potwierdzone na sprzęcie:** oryginalny OSDMenu ładuje Neutrino X z HDD, przekazuje argumenty, Neutrino wykonuje RO remount `pfs0:`, czyta konfigurację i moduły z HDD, przechodzi przez sekwencję przejścia do DVR LE (reset `rom0:OSDCNF`, SCmd `0x29`, drugi reset IOP i start modułów LE) oraz dochodzi do celowego `X85`. `X85` jest stopem przed `fhi_config_init`, obsługą pliku ISO i uruchomieniem gry.

**Granica wyniku:** wejściowy launcher diagnostyczny wciąż był uruchamiany z UDPFS — nie jest to jeszcze test całkowicie samodzielnego Managera bez UDPFS/USB. Nie deklarować uruchomienia ISO/grania. Do następnego etapu nie trzeba powtarzać testu X; celem jest osobne, kontrolowane sprawdzenie FHI i dostępu do obrazu ISO po przełączeniu środowiska LE. Wersja USB v0.8, main, istniejący `BOOT.CNF`, oryginalny `neutrino.elf` i partycje systemowe Sony pozostają bez zmian; PR #26 Draft.

---

## DIAG W — pełny PASS na fizycznej PSX DESR-7000; następny etap DIAG X (3.11.2026)

**Wynik W (zdjęcie z konsoli):**

```text
PSX DVR NEUTRINO W / HDD PROBE
W00 MAIN REACHED: CPU LOOP
W01 ENTRY HOLD COMPLETED
V00 ARGC=3 ARGS_OK=1
V01 fileXioInit=0
V02 MOUNT=0
V03 CHDIR=0
V04 CONFIG FD=3
V05 DVR IRX FD=3
V10 Neutrino core entered
V20 ROMVER OK
V30 LOAD SYSTEM BEGIN
V40 SYSTEM OK
V45 PARSE ARGS
V50 DRIVER CONFIGS NEXT
V60 LOAD EE_CORE + IRX
V70 MODULES LOADED, SAFE STOP
V RESULT=0 LAST_STAGE=70
NO GAME/ISO EXECUTED.
W90 CPU LOOP, PHOTO
```

**Potwierdzone sprzętowo:** oryginalny OSDMenu uruchamia prawdziwe, rozbudowane o DVR Neutrino pcm720 z osobnego `PP.TESTDVR`; argumenty są prawidłowe, wczesny remount PFS w `FIO_MT_RDONLY` i `chdir("pfs0:/")` skutkują dostępem przez POSIX, a rzeczywisty kod Neutrino ukończył konfigurację TOML i załadował moduły do pamięci. Nie jest to potwierdzenie rozruchu gry ani nawet startu LE — `V70` był twardym stopem przed restartem IOP.

**DIAG X — następny, ściśle odseparowany eksperyment:**
- Budowa na podstawie **tego samego, przypiętego commita pcm720 Neutrino** co W; `patch_probe.py` i `patch_w_entry_hold.py` są nadal używane. Dodatkowy `patch_x_dvr_le.py` usuwa **tylko** dotychczasowy stop `V70`, sprawdza, że `sys.sBSD == "dvr"` i QuickBoot jest wyłączony, a następnie instrumentuje **prawdziwą sekwencję pcm720**: `SifIopReset("rom0:UDNL rom0:OSDCNF")`, `sceCdApplySCmd(0x29)`, drugi reset IOP i start modułów LE. Widoczne markery `X71`... `X79`, w tym indeks modułu i kod `module_start()`. Po powrocie ze startu modułów bezwzględny stop `X85 LE MODULES DONE / STOP` przed `fhi_config_init`, `setup_dvd_iso`, otwarciem ISO i grą. Pętla CPU z W utrzymuje wynik.
- `PSX_DVR_HDD_NEUTRINO_X_LE.ELF` jest **niespakowanym** testowym Neutrino z tym instrumentowaniem. `DIAG_X.CNF` jest osobny i dozwala tylko docelowy X ELF, dokładnie `-bsd=dvr` i **fikcyjną** ścieżkę `-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso`; `-dev9=NICHDD` pozostaje ostatnią, konsumowaną przez OSDMenu flagą. `PSX_DVR_HDD_NEUTRINO_X_LAUNCH.ELF` wymusza ten konkretny CNF po wcześniejszym preflight RO i po świadomym X uruchamia rzeczywisty OSDMenu.
- **Znacząca różnica ryzyka względem W:** DIAG X naprawdę przełącza PSX DVR w fazę LE, wysyła polecenie SCmd 0x29, resetuje IOP i ładuje moduły IOP. Nie dokonuje zapisów do plików w kodzie programu, lecz wewnętrzne działania oryginalnych modułów/firmware są poza naszą kontrolą. Przed testem wskazana jest kopia ważnych danych, wyłącznie testowa `PP.TESTDVR`, brak ingerencji w Sony partycje systemowe, stabilną USB v0.8 czy prawdziwy `BOOT.CNF`.
- **Po udanym CI:** skopiować na `PP.TESTDVR` tylko `PSX_DVR_HDD_NEUTRINO_X_LE.ELF` i `DIAG_X.CNF`. Uruchomić osobny `PSX_DVR_HDD_NEUTRINO_X_LAUNCH.ELF` z UDPFS w wLE-R3Z. Na pierwszym ekranie wymaga `CHECK=0` i `XCNF` zgodnego z dozwolonym wzorcem; sfotografować, nacisnąć X. W razie zawieszenia sfotografować ostatni marker `X...`. Jeżeli dojdzie do `X85`, należy sfotografować `V RESULT=0 LAST_STAGE=85`. Po testach może być konieczny normalny restart konsoli (środowisko IOP zostało zmienione).
- **NIE TESTOWANE JESZCZE NA SPRZĘCIE.** Dopiero po sprzętowym PASS rozważać osobny test obsługi *realnego* obrazu ISO; DIAG X nie czyta ISO i nie startuje gier.

**Odporność procesu:** PR #26 pozostaje Draft, main/v0.8 USB niezmienione; kopia oryginalnego `neutrino.elf`, `BOOT.CNF`, `BOOT.ELF`, `config/`, `modules/` na `PP.TESTDVR` nie jest nadpisywana przez pliki X.

---

## DIAG W — bezpieczny marker początku `main()` (3.11.2026)

**Wynik sprzętowy V2 (PSX DESR-7000):** Po poprawnym `CHECK=0` i X konsola wróciła do XMB **szybko, zdecydowanie krócej niż 45 sekund**, tak jak przy starej próbie `BOOT.ELF`→Neutrino. W nieudanej spakowanej V również wróciła do XMB. Nie mamy zdjęcia ekranu V00 ani telemetrii z wnętrza Neutrino. Zatem **nie wiemy**, czy Neutrino w ogóle weszło do `main()`, czy tylko nie zdążyło narysować ekranu/od razu wróciło. DIAG U nadal jest prawdziwym, fizycznym dowodem działania RO remountu i POSIX otwierania z PFS.

Weryfikacja plików V2 vs U: **obie mają poprawny nagłówek ELF32 MIPS** i prawidłowy pojedynczy segment `LOAD`; w V2 wpis `0x00106958`, U `0x00101708`. Brak więc prostego błędu formatu ELF. Neutrino pcm720 używa `DISABLE_EXTRA_TIMERS_FUNCTIONS()` i `DISABLE_PATCHED_FUNCTIONS()`; dlatego nie traktujemy `sleep(45)` jako pewnego miernika czasu po starcie. Nowy W nie zależy od zegarów PS2SDK. Nie stwierdzono tego jako przyczyny sprzętowo, to dodatkowa niewiadoma.

**DIAG W**: dokładnie ten sam kod testowy Neutrino V/V2 z `pcm720/neutrino:bsd-dvr`, z przypiętego commita, zawsze **niespakowany**, oraz dodatkowy osobny skrypt `patch_w_entry_hold.py` wstrzykujący:
- bezpośrednio **po** `init_scr()` i `scr_printf("NO ISO OR GAME. READ ONLY.")` komunikat `W00 MAIN REACHED: CPU LOOP` i **skończoną pętlę procesora `w_busy_hold(8)`**, widoczną przez nieokreślony, lecz zauważalny czas przed `SifInitRpc`, `fileXioInit`, PFS i jakąkolwiek logiką Neutrino;
- po zakończeniu oryginalnego testu zamiast `sleep(45)` **`w_busy_hold(16)`** z komunikatem `W90 CPU LOOP, PHOTO`.

Wszystkie pozostałe zabezpieczenia V pozostają: ścisłe dozwolone `hdd0:PP.TESTDVR:pfs:`, `-bsd=dvr`, `-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso`, montowanie tylko `PP.TESTDVR` jako `pfs0:` `FIO_MT_RDONLY`, `chdir("pfs0:/")`, wyłącznie odczyt dwóch plików, **bezwzględny stop w V70** przed zmianą trybu IOP, odczytem ISO i uruchomieniem gry. `DIAG_W.CNF` jest osobnym, dokładnie kontrolowanym plikiem z końcowym `arg=-dev9=NICHDD`. Nie nadpisywać `BOOT.CNF`, `BOOT.ELF`, `neutrino.elf`, `config/`, `modules/` ani wersji USB v0.8.

**Po udanym CI:** skopiować wyłącznie `PSX_DVR_HDD_NEUTRINO_W_ENTRY.ELF` i `DIAG_W.CNF` do głównego `hdd0:PP.TESTDVR`. `PSX_DVR_HDD_NEUTRINO_W_LAUNCH.ELF` uruchomić z UDPFS/wLE. Zdjęcie `CHECK=0`, potem X. Oczekiwane `W00 MAIN REACHED` przez zauważalny czas (pętla nie jest precyzyjnym zegarem), następnie wyniki `V00...V70` lub `V RESULT LAST_STAGE` oraz `W90...` przez kolejny zauważalny czas. **Jeśli natychmiast wróci do XMB, zrób od razu próbę bezpośredniego uruchomienia W ELF przez UDPFS/wLE (bez argumentów)** jako oddzielny test: W powinien przynajmniej dojść do `W00` zanim odrzuci błędny `argc`; bez resetu HDD, bez ISO. Wynik pozwoli rozdzielić awarię binarnego Neutrino przed main od oddziaływania OSDMenu.

Uwaga: Pętla CPU nie potrzebuje timerów ani IOP; jej faktyczny czas zależy od optymalizacji i sprzętu, dlatego nie wyciągać wniosków z dokładnego czasu w sekundach. `CHECK=0` zawsze oznacza tylko poprawny preflight launchera. Przy ewentualnym zawieszeniu sfotografować ostatni marker. `main`, stabilne USB v0.8 i systemowe partycje Sony pozostają bez zmian.

---

## DIAG V2 — identyczny Neutrino V, ale *bez* ps2-packer (3.11.2026)

Na fizycznym DESR-7000 użytkownik otrzymał `CHECK=0` przy uruchomieniu `PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF` i po X wrócił do XMB, bez zgłoszonego zdjęcia ekranu `V ...`. Nie znamy dokładnego czasu trwania ani tego, czy V zdążyło wejść w `main()`; nie można utożsamiać tego z błędem dostępu do `config/`/modułów. Wcześniejsze U potwierdziło fizycznie ich otwieranie po remoncie PFS.

Analiza artefaktów CI: `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF` jest **pakowany przez `ps2-packer`** (73,172 B, punkt wejścia `0x01d0001c`). Sprawdzony T/U używał niespakowanego ELF. Podejrzewamy problem z wejściem do spakowanego Neutrino przez oryginalny OSDMenu, ale nie zakładamy przyczyny bez pomiaru.

**V2 to kontrola tylko jednej zmiennej:** ten sam przypięty commit `pcm720/neutrino:bsd-dvr`, ten sam `patch_probe.py` (bez własnego startu gry, bez otwierania ISO, stop po konfiguracjach i `modlist_load` na `V70`), te same argumenty, HDD/DEV9, partcja `PP.TESTDVR` montowana tylko RO i `chdir("pfs0:/")`. Zamiast kompresowanego `neutrino.elf`, CI kopiuje **`neutrino_unpacked.elf`** jako `PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF`, do osobnej konfiguracji:
```ini
boot=PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso
arg=-dev9=NICHDD
```

**Test:**
1. Skopiować tylko `PSX_DVR_HDD_NEUTRINO_V2_UNPACKED.ELF` i `DIAG_V2.CNF` obok starych plików w **`hdd0:PP.TESTDVR`**, bez nadpisywania `neutrino.elf`, `BOOT.ELF`, `BOOT.CNF` czy V.
2. W wLE-R3Z uruchomić `PSX_DVR_HDD_NEUTRINO_V2_LAUNCH.ELF` z UDPFS. Pierwszy ekran powinien pokazać `CHECK=0` i poprawny `V2CNF` (tylko ta konfiguracja jest dozwolona); X uruchomi oryginalny OSDMenu→ **niespakowany** Neutrino V2. START anuluje.
3. Sfotografować pierwszy ekran oraz jakikolwiek ekran/marker `V00 ... V70`/`V RESULT LAST_STAGE`. Jeśli znów wróci do XMB **bez komunikatów V**, zanotować orientacyjny czas od X — pozwala odróżnić celowy automatyczny powrót po 45 sekundach od awarii przed wejściem do programu.
4. Brak jakichkolwiek zmian w prawdziwych ISO, systemowych partycjach Sony, stabilnym USB v0.8 i `main`. Obie wersje V/V2 są diagnostyką bez gry. Draft PR #26 jest eksperymentalny.

**Interpretacja:** jeśli V2 pokaże fazy/osiągnie `V70`, a spakowana V ich nie pokazywała, przyczyną może być wykonywanie spakowanego ELF przez OSDMenu; wniosek potwierdzimy oddzielnym kontrolnym testem, nie automatycznie. Jeśli V2 też wróci do XMB, następną hipotezą jest wczesna inicjalizacja/nielogowany powrót Neutrino, której nie wystarczy wyjaśnić samym pakowaniem.

---

# DIAG V — Neutrino pcm720 z kontrolowanym montowaniem HDD (test bez gry)

**Stan: eksperymentalny, nieprzetestowany na sprzęcie.** Źródło: pinned publiczny `pcm720/neutrino` z `bsd-dvr`, commit `aa8d80a1a0b5323cb407c814c0568c3489597c1c`; upstream PS2 `ps2max32/neutrino` pozostaje nietknięty. OSDMenu `BOOT.ELF` i wariant USB v0.8 pozostają bez zmian.

## Dlaczego

Na prawdziwym PSX DESR-7000 **DIAG U zakończył się pełnym PASS** po uruchomieniu rzeczywistego OSDMenu z `PP.TESTDVR` i argumentami: `ARGC=3 ARGS_OK=1`; `PFS_PRE=-19` / `HDD=2`, po ponownym montowaniu `MOUNT_RO=0`, `PFS_AFTER=3`, `RPC CONFIG=3`, `DVR IRX=3`, `CHDIR=0`, `NEW CWD=pfs0:/`, `POSIX CFG=3 ERRNO=0`, `POSIX IRX=3 ERRNO=0`, `UMOUNT OWN=0`. To sprzętowo potwierdza możliwość *przywrócenia* odczytu `config/` i `modules/` po tym, jak OSDMenu odmontował `pfs0:`.

Dotychczasowy `BOOT.CNF` uruchamiający `neutrino.elf` (spatchowana gałąź pcm720 z nowymi modułami DVR) wracał do XMB, nawet gdy na testowym HDD pojawiły się `config/`, `modules/` i końcowe `-dev9=NICHDD`. DIAG V sprawdza **kolejny etap: oryginalne wczytywanie konfiguracji i modułów Neutrino z HDD**.

## Zawartość

- `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF` — osobna, zbudowana z `pcm720/neutrino:bsd-dvr` diagnostyczna wersja prawdziwego Neutrino. Nie jest produkcyjnym `neutrino.elf`. Na wejściu dopuszcza wyłącznie `hdd0:PP.TESTDVR:pfs:/...`, dokładnie `-bsd=dvr` i **fikcyjną** ścieżkę `-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso`.
- `DIAG_V.CNF` — ścisła alternatywna konfiguracja z poprawną końcową flagą OSDMenu `-dev9=NICHDD`, adresująca wyłącznie `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF`.
- `PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF` — odpalany z UDPFS w wLE-R3Z, używa realnego oryginalnego OSDMenu `BOOT.ELF` i sprawdza alternatywny `DIAG_V.CNF` byte-for-byte (dopuszczalny wyłącznie końcowy LF lub NUL, jak w DIAG R3/T/U).

## Dokładnie co robi Neutrino V

1. Uruchamia debugowy ekran i potwierdza argumenty; nie kontynuuje, jeśli argumenty/partycja różnią się od dozwolonych.
2. Korzysta z `SifInitRpc(0)`, `fileXioInit()`; próbuje zamontować **tylko** `hdd0:PP.TESTDVR` pod `pfs0:` z `FIO_MT_RDONLY`, potem `chdir("pfs0:/")`.
3. Otwiera *tylko do odczytu* `config/system.toml` i `modules/dvrfile_fhi.irx`; drukuje wyniki.
4. Wywołuje prawdziwy inicjalizator Neutrino z forka pcm720 — etap ROMVER, `system.toml`, argumenty `-bsd=dvr`, zależności `bsd-dvr.toml`, inne konfiguracje, `ee_core.elf` i binaria modułów `*.irx` wczytywane do pamięci. Widoczne są markery faz `V10`, `V20`, `V30`, `V40`, `V45`, `V50`, `V60`.
5. **Bezwzględny stop po `modlist_load`** na `V70 MODULES LOADED, SAFE STOP`. W testowym buildzie *nie dochodzi* do przełączania IOP w tryb DVR, `sceCdApplySCmd`, tworzenia `dvrfile_fhi`, otwierania obrazu gry ani `ExecPS2`.
6. Raport `V RESULT=... LAST_STAGE=...` utrzymuje się około 45 sekund, następnie kod odmontowuje wyłącznie własny PFS i wraca. Gdy przedwczesny błąd wystąpi w fazie źródłowego Neutrino, wrapper również pokazuje `LAST_STAGE` i końcowy wynik.

`V70` z `V RESULT=0` oznacza tylko poprawne **wczytanie konfiguracji i modułów**, nie uruchomienie gry ani potwierdzenie wszystkich operacji DVR. `LAST_STAGE` poniżej 70 pomaga wskazać nieudany etap.

## Instrukcja po pomyślnym GitHub CI

1. **Nie nadpisywać** oryginalnych `BOOT.ELF`, `BOOT.CNF`, `neutrino.elf`, `config/`, `modules/`, ISO, wariantu USB v0.8 ani partycji Sony.
2. Na testową `PP.TESTDVR` skopiować **tylko** `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF` i `DIAG_V.CNF` obok oryginalnych plików.
3. `PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF` uruchomić z UDPFS przez wLE-R3Z. Przy `CHECK=0` zrobić zdjęcie pierwszego ekranu, nacisnąć X. Przy `CHECK !=0` X jest nieaktywny.
4. Zrobić zdjęcie końcowego ekranu z `V RESULT` i `LAST_STAGE` (oraz ostatniego markera, jeśli nastąpi zawieszenie).

**Bezpieczeństwo:** kod V nie tworzy ani nie zapisuje plików. Montowanie jest wyłącznie RO, dopuszczalna jest tylko odseparowana partycja testowa. Inicjalizowanie istniejącego zewnętrznego IOP i modułów może mieć niezależne skutki; zalecana jest kopia ważnych danych. Przed etapem rzeczywistego uruchamiania gry wymagany jest osobny, świadomy test. PR #26 pozostaje Draft, `main` i v0.8 USB nietknięte.
