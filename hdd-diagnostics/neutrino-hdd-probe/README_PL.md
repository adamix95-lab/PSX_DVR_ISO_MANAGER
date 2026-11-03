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
