# Audyt Native QUIT GAME + IGR — PSX DESR-7000 (tylko gałąź eksperymentalna)

Stan: 2026-11-13. **Audyt statyczny źródeł — NIE JEST potwierdzeniem działania na fizycznej konsoli.**

## Izolacja i źródła

- Gałąź feature/native-quit-igr została założona **z tagu 1.0** (commit fe9b0b77ba51db87d856ca4a0ee95a0ed9db5ff0), nie z eksperymentalnego HTTPS 1.2.
- Wydanie 1.0, main, feature/v1.0-hdd, feature/v1.0-usb, PP.DVRISO, BOOT.CNF, obrazy ISO i XMB nie są zmieniane.
- Pinned Neutrino DVR/FHI: ps2max32/neutrino @ aa8d80a1a0b5323cb407c814c0568c3489597c1c.
- Referencja PS2SDK: ps2dev/ps2sdk @ d41ea62946e53d9a0c8788bb9eb8c8faf488b559.
- OSDMenu: pcm720/OSDMenu @ a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f.
- Porównanie: wcześniejsza eksperymentalna gałąź feature/neutrino-desr-igr (NATIVE QUIT — gra nie wystartowała w próbie USB).
- IGR: danielnuld/nuld, third_party/neutrino-igr/igr.c, dokumentacja docs/phase12-results.md.

## 1. Kto rzeczywiście wysyła sceCdNoticeGameStart i kiedy?

**Manager 1.0 HDD**:
- dvr-iso-manager/src/main.c, linie 126-195: po SifIopReset, przy inicjalizacji sterowników, gałąź DESR_HDD_TEST robi sceCdInit(SCECdINoD) i **sceCdNoticeGameStart(0, &notice_arg)** przed załadowaniem dvrdrv i dvrfile.
- Sprawdza „< 0”, podczas gdy użyta w PS2SDK implementacja zwraca 1 (transport poprawny) lub 0 (błąd transportu). Kod nie wymaga ani return==1, ani status==0. Samo wywołanie nie jest więc potwierdzonym włączeniem QUIT.
- **USB 1.0**: inna gałąź tego samego pliku, linie 189-194, robi sceCdNoticeGameStart(0, NULL) bez kontroli statusu.

**OSDMenu: dwie różne aplikacje / ścieżki, nie mylić ich**:
- OSDMenu common/src/psxinit.c, linie 63-85: switchPSX() resetuje IOP do OSDCNF, zmienia system na PS2, następnie wysyła custom_sceCdNoticeGameStart(1, &stat) i czeka na odpowiedź bez busy. Ten kod jest używany np. w mbr/src/common.c (linia 123).
- **Jednak nasz BOOT.ELF pochodzi z prekompilowanego pakietu OSDMenu Launcher**. Ścieżka launcher/src/main.c → launcher/src/handler_quickboot.c → launcher/src/common.c → launcher/src/handler_pfs.c nie ma wywołania switchPSX. launcher/CMakeLists.txt nie dodaje common/src/psxinit.c do źródeł launchera. W tej ścieżce initModules() wykonuje SifIopReset("") (launcher/src/init.c, linie 193-265).
- **Granica dowodu**: repo przechowuje skrypt pakujący BOOT.ELF z dawnego ZIP, nie sam pełny źródłowy obraz binarny tego BOOT.ELF. Powyższy wniosek dotyczy identyfikowalnej ścieżki źródłowej OSDMenu Launcher, nie audytu bajtów egzemplarza BOOT.ELF z release.

**Neutrino HDD / LANG**:
- Neutrino ee/loader/src/main.c, linie 720-745: w gałęzi „if (sys.bQuickBoot == 0)” oraz „-bsd=dvr” wykonuje reset IOP do OSDCNF, a następnie **sceCdApplySCmd(0x29, in, 4, out)**, gdzie **in to cztery zera**. Brak sprawdzenia transportu i statusu. Potem reset IOP do pustej konfiguracji.
- UWAGA: „Quickboot” w OSDMenu (obsługa BOOT.CNF) **nie jest** tym samym co parametr **-qb** w Neutrino. Nasz generator dvr-iso-manager/src/hdd_boot_config.c (linie 19-34) **nie przekazuje -qb**. Neutrino w ee/loader/src/main.c (linie 196-197) ustawia bQuickBoot dopiero po otrzymaniu -qb. Dlatego na podstawie generowanego BOOT.CNF normalnie wykonuje się wyżej opisana gałąź, a nie jest pomijana.
- HDD FIX95: neutrino-hdd-v1/patch_hdd_runtime.py modyfikuje mount/chdir i sprawdzenie ISO, ale nie dodaje własnej komendy NoticeGameStart ani opcji -qb.
- LANG: neutrino-v02/neutrino-osdlang-en.patch zmienia hooki SetOsdConfigParam/GetOsdConfigParam i przekazuje osdlang_mode do ee_core; **nie dodaje** własnej komendy QUIT, lecz współdzieli z Neutrino jego drogę inicjalizacji CDVD.

**Protokół (PS2SDK)**:
- ee/rpc/cdvd/src/scmd.c, linie 951-969: sceCdNoticeGameStart(arg1, result) wysyła SCMD **0x29 z JEDNYM bajtem parametru** i opcjonalnie zwraca pierwszy bajt statusu; transport przyjmuje wartości 1 lub 0.
- iop/cdvd/cdvdman/src/cdvdman.c, linie 7823-7845, dla CDVD_VARIANT_XOSD potwierdza jednobajtowy argument i status; po poprawnym wyniku czyści wewnętrzny medium_removal_state.
- OSDMenu i przykład PSX od sp193/ps2homebrew używają wartości **1**, aby umożliwić QUIT GAME. Przykładowy kod PS2SDK dla pilota używa **0** przy przygotowaniu odczytu przedniego przycisku. Opublikowane fragmenty nie stanowią pełnej specyfikacji znaczenia każdego trybu — NIE twierdzimy, że 0 zawsze wyłącza QUIT.
- To jest główna rozbieżność: **Manager 0 / Neutrino 0 (4 bajty) / referencyjne PSX enable 1 (1 bajt)**.

## 2. Reset IOP: czy kasuje flagę MechaCon?

- Manager wykonuje SifIopReset na starcie; OSDMenu Launcher wykonuje go przy initModules(); Neutrino bez -qb wykonuje reset do OSDCNF i jeszcze reset „” przed uruchomieniem modułów emulacji.
- W projekcie ps2homebrew/OSD-Initialization-Libraries (osd/main.c, linie 59-77) po pomyślnym sceCdNoticeGameStart(1, &stat) **następuje SifIopReset("")**. To jest mocny przykład oczekiwanej trwałości stanu QUIT w MechaCon podczas resetu IOP; nie jest to pomiar konkretnego DESR-7000 ani dowód zachowania po całej sekwencji Neutrino.
- Reset procesora IOP nie jest tym samym co restart całej konsoli / MechaCon; istnieją inne przejścia stanu (CDVD mode, reset modułów, wejście do gry), które należy osobno sprawdzić na sprzęcie.
- Możliwe hipotezy: ostatni SCMD 0x29 wysłany przez Neutrino z trybem 0 zmienia stan; status jest błędny lub ignorowany; albo flaga zostaje utracona później. Nie rozstrzygamy bez instrumentacji.

## 3. Moduły Sony DVR i emulacja CDVDMAN

- Manager wgrywa dvrdrv.irx i dvrfile.irx (src/main.c, linie 186-187). Dostarcza je ze swoim zestawem modułów.
- Neutrino bazuje na DVR/FHI z pcm720 PR #228 oraz używa dvrfile_fhi.irx, a nasze HDD FIX95 dokłada jedynie poprawkę dostępu do własnego PP.DVRISO i weryfikację ISO (bez zastępowania emulatora CD/DVD).
- Zestaw „SYSTEM” oraz „ENGLISH” bazuje na tym samym pinie Neutrino; LANG używa własnego ee_core_lang.elf i nie zapisuje na stałe języka systemu.
- **Po wejściu w emulację CDVDMAN** nie wolno zakładać, że gry mogą wywołać oryginalny Sony sceCdNoticeGameStart. W Neutrino iop/cdvdman_emu/src/exports.tab nazwa jest tylko w komentarzu w miejscu odpowiadającym wysokiemu eksportowi; brak rzeczywistego DECLARE_EXPORT(sceCdNoticeGameStart). Z tego względu potencjalną flagę trzeba ustawić **przed** przejęciem CDVD przez emulację.
- Sam sukces uruchomienia ISO z dvr_pfs0:/ISO/ nie dowodzi zachowania mechanizmu QUIT GAME.

## 4. Czy można przenieść NULD IGR?

- **TAK, jako oddzielny fallback na pad**, a nie automatyczny sterownik natywnego QUIT.
- NULD third_party/neutrino-igr/igr.c wykrywa kombinację pada przez libpad i VBLANK, resetuje IOP, zatrzymuje SPU2 (resetspu.irx), przywraca syscall hooks i ładuje rom0:OSDSYS przez LoadExecPS2.
- NULD docs/phase12-results.md: reboot/power-off potwierdzone na zwykłej SCPH-75001; NIE na PSX DESR.
- **KRYTYCZNE**: rom0:OSDSYS w PSX DESR nie musi przywracać pełnego XMB DVR. Nie zakładać, że reset NULD prowadzi do oryginalnego XMB. Wykorzystanie wymaga ustalenia bezpiecznej sekwencji startu DVR po powrocie.
- IGR ingeruje w ee_core (pamięć, syscalle, IOP, cache, hooki); musi być przenoszony na nasz pinned PR228, zachowując ee_core_lang i DVR FHI, a nie jako gotowy plik z NULD. W NULD kod Neutrino IGR jest oznaczony AFL-3.0.
- Dotychczasowa feature/neutrino-desr-igr miała test USB kończący się czarnym ekranem **przed** uruchomieniem gry, więc ten wynik nie świadczy, że QUIT jest sprzętowo niemożliwy. Nie scalamy jej bez analizy regresji.

## Kolejność testów / bramki bezpieczeństwa

### G0 — bezpieczny, samodzielny test USB, zero zapisów HDD
Z tej gałęzi można zbudować eksperymentalny QUIT_MODE1_PROBE.ELF (native-quit-igr/probe). Jest **niezależny** od Managera i Neutrino, nie montuje ani nie tworzy partycji, nie zapisuje BOOT.CNF, plików ani pamięci karty. Uruchomić z USB przez wLE, odczytać trzy liczby: sceCdInit, transport, status. Jeśli (init>0, transport=1, status=0), fizyczny QUIT można sprawdzić **tylko gdy nie odbywa się zapis żadnych danych**. Program próbuje włączyć tryb gry MechaCon, więc naciśnięcie QUIT może zresetować konsolę. Jeśli usługa CDVD jest niedostępna w bieżącym IOP, wynik G0 jest nierozstrzygający.
Nie zastępować BOOT.ELF, Neutrino, Managera, modułów i nie wgrywać nic na HDD.

### G1 — porównanie z działającym Managerem 1.0
W normalnej wersji 1.0 sprawdzić, czy fizyczny QUIT działa w grze (bez eksperymentalnych plików). Zanotować SYSTEM/ENGLISH i konkretną grę. Jeśli G0 działa, a G1 nie, podejrzenie skupia się na łańcuchu OSDMenu/Neutrino.

### G2 — przyszła minimalna zmiana tylko na gałęzi
Dopiero po wynikach G0/G1 przygotować osobny **USB** Manager/launcher wywołujący sceCdNoticeGameStart(1, &status) bezpośrednio przed przekazaniem do OSDMenu, przy pozostawieniu oryginalnego Neutrino. Dopiero gdy gra uruchomi się poprawnie, sprawdzić QUIT. Ten etap nie jest tu implementowany, by nie powtórzyć czarnego ekranu z poprzedniej gałęzi.

### G3 — dopiero potem Neutrino/NULD
Jeśli G2 nie działa, dodać ostrożną instrumentację ostatniego 0x29 w Neutrino. Każda zmiana: osobny artefakt, czytelny log, PASS gry przed testem QUIT; nie kopiować paczki ze starej gałęzi.
Jeśli natywny QUIT jest niemożliwy, rozważyć IGR NULD dla kombinacji pada, ale powrót XMB musi być przetestowany odrębnie na DESR.

**Czego nie robimy**: nowa partycja, instalacja XMB, zmiana PP.DVRISO, podmiana v1.0, merge, wymuszony LoadExecPS2("rom0:OSDSYS") jako rzekomo sprawdzony powrót DVR XMB.

## Wynik z konsoli / stan po teście (2026-11-13)

G0 (samodzielny USB MODE1 PROBE) wykonany na prawdziwym **DESR-7000**:
- `sceCdInit(SCECdINoD): 1`
- `sceCdNoticeGameStart(1): 1`
- `MechaCon status: 0x00000000`
- Wciśnięcie fizycznego **QUIT GAME nie wywołało żadnej reakcji**; konsola została wyłączona ręcznie.

Interpretacja: sukces transportu SCMD 0x29 nie wystarcza do dowodu działania wyjścia podczas samodzielnego ELF. **G0 nie uruchamiał gry**. Dlatego G2 porównuje realny start tej samej gry z testowych kopii USB (A: dokładnie Neutrino z oficjalnego 1.0; B: jedynie 0->1 i 4->1 bajt w istniejącym SCMD).

G2 przygotowane w tej samej gałęzi: [szczegółowa instrukcja testu A/B](g2/README_TEST_PL.md). Test obejmuje osobne foldery USB `DESR_G2A` i `DESR_G2B`, oryginalny `BOOT.ELF`, oryginalne moduły z USB 1.0 i izolowane `BOOT.CNF`. Ani wydanie 1.0, ani HDD/XMB nie są modyfikowane. **Wynik sprzętowy G2 nieznany, dopóki użytkownik nie przetestuje artefaktu.**

## G2B — wynik fizycznej DESR-7000: **native QUIT wraca do XMB (2026-11-13)**

**Informacja od użytkownika po teście sprzętowym G2B:** gra **nie wystartowała** (czarny ekran). Po naciśnięciu fizycznego **QUIT GAME** po kilku sekundach konsola **wróciła do oryginalnego XMB Sony**.

Rozróżnienie bramek:
- **G2B — boot gry: FAIL / czarny ekran.**
- **G2B — QUIT na czarnym ekranie: PASS / powrót do XMB Sony.**
- **QUIT podczas normalnie działającej gry: nadal NIEPOTWIERDZONY.**
- G2A: na podstawie tej wypowiedzi użytkownika nie wpisujemy nowego wyniku, dopóki nie zostanie zgłoszony.

Wniosek: najważniejsza funkcja sprzętowa **jest osiągalna** po zmianie starego 0x29 `mode=0, bytes=4` na `mode=1, bytes=1`, ale ta modyfikacja lub środowisko testowe powoduje regresję bootowania gry. **Nie wolno przedstawiać jej jako gotowej poprawki.**

Dodatkowy fakt z CI `G2C`: oryginalna, niezmieniona kompilacja Neutrino SYSTEM w identycznym toolchainie dała ELF bajt-po-bajcie równy oficjalnej 1.0. Poprzednie `package-g2c` przerwało się na zbyt ostrym asercie `neutrino != baseline`, czyli dowód tożsamości został błędnie uznany za błąd paczki. Ta kontrola eliminuje różnice wersji kompilatora jako wyjaśnienie.

Następny test: **G3 A/B** z dwoma wariantami ortogonalnymi: `mode=1, len=4` i `mode=0, len=1`. Nie dodawać nowego SCMD, resetu IOP, hooków ani podmiany modułów DVR. Testować boot, a fizyczny QUIT tylko gdy stan jest stabilny i nic nie jest zapisywane.

## G3 — wyniki fizycznego QUIT / startu gry, 2026-11-13

- **G3A**, `sceCdApplySCmd(0x29)` z `mode=1, len=4`: po wybraniu gry X czarny ekran; fizyczny QUIT wywołał po dość długim oczekiwaniu **powrót do oryginalnego XMB Sony**. **Gra FAIL / QUIT→XMB PASS**.
- **G3B**, `mode=0, len=1`: **gra uruchamia się**, natomiast fizyczny QUIT **nie reaguje**. **Gra PASS / QUIT FAIL**.
- Porównanie z **G2B** (`mode=1,len=1`): czarny ekran, lecz fizyczny QUIT po kilku sekundach wraca do XMB.
- Wniosek dla dostępnych testów: wyłącznie zmiana **trybu na 1** wiąże się z aktywnym QUIT, ale też utratą możliwości startu gry. Zmiana długości **4→1** nie wyjaśnia ani jednego, ani drugiego; mechanizm utraty startu gry pozostaje nierozpoznany.
- Współzależność `mode=1` i niemożności uruchomienia gry nie dowodzi, że powodem jest awaria `dvr_pfs0`; nie mamy logów z ostatniej fazy bootowania po SCMD.
- Referencyjny OSDMenu `switchPSX()` wywołuje `custom_sceCdChgSys(2)` **przed** `custom_sceCdNoticeGameStart(1)`, a Neutrino DVR w naszym pinie nie wywołuje w tej sekwencji `sceCdChgSys`. To **hipoteza do oceny**, nie automatyczna poprawka — przełączenie napędu może wpłynąć na dostęp do danych DVR i nie wolno go bez pomiaru włączać w stabilnym Neutrino.
- Dalsze testy powinny diagnozować działanie sterowników DVR / FHI po aktywacji flagi i dokładny etap czarnego ekranu oraz sprawdzić wariant wierny inicjalizacji PSX na oddzielnej gałęzi/USB. Dopiero jeśli natywny QUIT uniemożliwia stały odczyt DVR ISO, rozważyć IGR NULD jako niezależny fallback.
- **Nie scalamy** G2B/G3A ani żadnej testowej binarki z 1.0, dopóki uruchomienie gry i wyjście do XMB nie przejdą obu bramek.

Szczegóły: [wyniki G3 i instrukcja](g3/README_TEST_PL.md).

## G4 — gotowy odrębny test sekwencji Sony (CI PASS, hardware UNTESTED)

- [Instrukcja G4A/G4B](g4/README_TEST_PL.md) i [workflow CI](../.github/workflows/native-quit-g4-sequence.yml).
- G4A: `custom_sceCdChgSys(2)`-style SIF RPC `0x2D`, potem oryginalny `NoticeGameStart(0)` (jednobajtowo). To kontrola działania FHI po przełączeniu trybu napędu.
- G4B: identyczne `ChangeSys(2)`, potem `NoticeGameStart(1)`. Test natywnego QUIT przy sekwencji analogicznej do Sony.
- **Warunek:** test G4B wykonywać dopiero jeżeli G4A uruchomi tę samą grę. Gdy G4A nie startuje: STOP, najpierw diagnoza dostępu DVR po ChangeSys.
- GitHub Actions [43415798131](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43415798131) **PASS — kompilacja i weryfikacja paczki**, nie wynik na sprzęcie.
- Nic nie zmieniano w stabilnym wydaniu 1.0 ani na HDD/XMB.

## G4 — wynik hardware: G4A PASS gry, G4B FAIL gry i QUIT (2026-11-13)

- **G4A:** ChangeSys(2) + NoticeGameStart(mode 0): **gra uruchomiła się z DVR ISO; fizyczny QUIT bez reakcji**. Boot PASS / QUIT FAIL.
- **G4B:** ChangeSys(2) + NoticeGameStart(mode 1): **czarny ekran zamiast gry; fizyczny QUIT bez reakcji**. Po pełnym wyłączeniu i ponownym uruchomieniu konsoli wynik pozostał taki sam. Boot FAIL / QUIT FAIL.
- To nie jest działająca poprawka. Dodatkowa operacja ChangeSys(2) nie rozwiązuje problemu mode 1, a względem G3A (bez ChangeSys(2)) zaobserwowano również brak powrotu do XMB.
- Wcześniejsze wyniki pozostają: G3A boot FAIL / QUIT→XMB PASS; G3B boot PASS / QUIT FAIL; G2B boot FAIL / QUIT→XMB PASS.
- Następne działania: etapowa diagnostyka startu gry i statusów SIF RPC/MechaCon, bez eksperymentowania z kolejnymi wartościami SCMD. Zachować oddzielną gałąź i wyłącznie USB; nie zmieniać v1.0, HDD ani XMB.

Szczegóły w [G4 README](g4/README_TEST_PL.md).

## G5 — etapowa diagnostyka USB przygotowana (CI PASS, hardware UNTESTED)

- [Instrukcja G5](g5/README_TEST_PL.md), [repo workflow](../.github/workflows/native-quit-g5-stages.yml), [CI #43422949377](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43422949377) — zakończony **PASS 5/5**.
- **G5A**: NoticeGameStart(mode 0, 1 bajt), znana działająca kontrola z G3B. **G5B**: NoticeGameStart(mode 1, 1 bajt), tryb powodujący czarny ekran w G2B. Oba Neutrino mają identyczne punkty kontrolne GUI/GS.
- Na ekranie etap 20/21 pokazuje CDVD init / transport SCMD i status MechaCon; etap 30/31 drugi IOP reset, 40/42 moduły LE, 50/51 FHI, 61–69 otwieranie i walidacja ISO, 90/91 SYSTEM.CNF.
- Przy powodzeniu oba programy **celowo zatrzymują się na STAGE=199 przed uruchomieniem gry**. Nie interpretować tego jako błędu bootowania gry.
- Porównanie A/B identyfikuje najwcześniejszy nieukończony etap; jeśli G5B dochodzi do 199, dalszy problem leży w etapie EE handoff / następnym IOP/EE core lub zostaje zamaskowany przez instrumentację.
- Użyto dokładnego bazowego USB 1.0 (SHA-256 zweryfikowany), a moduły, OSDMenu, język EN, EE core i konfiguracje pozostają bajtowo zgodne z wydaniem. Osobne katalogi USB `DESR_G5A` i `DESR_G5B`. Żadnych zapisów HDD ani zmian stabilnego wydania.
- **Rzeczywisty wynik sprzętowy G5 jeszcze nieznany.**

## Wynik sprzętowy G5A — 2026-11-13

Użytkownik zgłosił: **G5A po próbie uruchomienia wraca do XMB**, zamiast wyświetlić końcowy ekran diagnostyczny `STAGE 199`. Nie zgłoszono, czy którykolwiek STAGE był widoczny pośrednio, dlatego nie przypisujemy awarii konkretnemu etapowi ani nie wnioskujemy, że ISO lub MechaCon zawiodły. **G5A nie przeszedł bramki diagnostycznej. G5B nie powinien być testowany** do czasu potwierdzenia kontroli. Analiza źródła wskazuje, że pierwszy ekran G5A pojawia się dopiero po wczytaniu konfiguracji i modułów; błędy/return przed tym punktem powodują nieinstrumentowany powrót. Dodatkowo `init_scr()` jest wywoływany dla każdego etapu, co zwiększa ingerencję w GS. Potrzebny jest test kontrolny, który wyświetli pierwszy STAGE przy wejściu do `main`, a także przechwyci wszelkie wczesne kody return.

## G5R — kontrola po nieoczekiwanym powrocie G5A do XMB

- G5A na prawdziwym DESR-7000 wrócił do XMB zamiast `STAGE 199`. Nie ma informacji o ostatnim widocznym STAGE, dlatego źródło błędu pozostało nieznane, a G5B nie został dopuszczony do próby.
- Audyt pierwszego G5A: widoczny punkt `STAGE 10` był dopiero po odczycie ROMVER/config i wczytaniu modułów. Wczesny `return -1` mógł skończyć się XMB bez zdjęcia kodu.
- G5R dodaje **`STAGE 1` przy wejściu Neutrino**, lekkie wczesne fazy 2–9, przechwytuje zakończenie `main` przez `STAGE 298` (kod return + ostatnia faza) i inicjalizuje GS tylko raz. Wszystkie pozostałe G5A checkpointy, SCMD `mode0,len1` i celowy STOP przed EE handoff `STAGE 199` zostają zachowane. Brak HDD/USB log writes i nowych resetów IOP.
- [Instrukcja G5R](g5r/README_TEST_PL.md); [GitHub Actions #43426651495](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43426651495) zakończył się **PASS 3/3**. Paczka: `DESR-QUIT-G5R-EARLY-RETURN-USB-UNTESTED`.
- Kompilacja PASS nie jest dowodem działania na konsoli; należy najpierw uruchomić `mass0:/DESR_G5R/DVR_ISO_MANAGER.ELF` i dostarczyć zdjęcie pierwszego/ostatniego widocznego etapu. G5B nadal bez testów.

## G5R — fizyczny test dotarł do STAGE 91 (2026-11-13)

- Na prawdziwym DESR-7000 użytkownik zgłosił **„STAGE 91 STOP”** w G5R. W kodzie G5R `g5_note(91)` występuje po nieujemnym wyniku `read_system_cnf(sDVDFile,...)`; nie jest samo w sobie celowym stopem. Nie znamy pełnej treści ekranu i czy widniało `STAGE: 298 / LAST PHASE BEFORE STOP: 91`. Nie przypisywać wyniku do awarii MechaCon ani I/O ISO bez tej informacji.
- Luki poprzedniej instrumentacji: od `STAGE 91` do `STAGE 199` nie było żadnego dalszego widocznego checkpointu. W tym odcinku znajdują się parsowanie BOOT2 z `SYSTEM.CNF`, per-game compat, konfiguracja CDVDMAN, Deckard, ATA/MC, PS2 Logo, fake table, czyszczenie pamięci EE `memset(0x82000..0x100000)`, budowa IOPRP, kopia EE core i przekazanie sterowania.
- Przygotowano **G5S** w odrębnym `mass0:/DESR_G5S/`, wyłącznie `mode=0,len=1`. Zachowuje wcześniejsze `STAGE 1` oraz pułapkę `STAGE 298`, dodaje widoczne etapy 92–98, celowe `STAGE 105` **przed czyszczeniem EE RAM**. To kontrolny stop: G5S nigdy nie uruchamia gry.
- [Instrukcja G5S](g5s/README_TEST_PL.md), [CI G5S](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/workflows/native-quit-g5s-post-cnf.yml). Jest to wciąż eksperyment, bez działania na fizycznej konsoli. Nie modyfikować HDD, XMB ani wydania 1.0 i nie testować G5B bez dalszej diagnozy.

## Wynik sprzętowy G5S — ostatni podany numer 98 (2026-11-13)

Użytkownik zgłosił: **„dalej to samo, stage 98 stop”**. G5S doszedł przynajmniej do `g5_note(98)`, czyli za odczyt SYSTEM.CNF i inicjalizację kompatybilności/CDVDMAN/Deckard/ATA/MC/Logo. **Nie mamy jednak pełnego tekstu ekranu i nie wolno ogłaszać błędu ani PASS.**

Ważne dla interpretacji: `g5_stop(105, ...)` czyta `last_stage=g5_stage` zanim wywoła `g5_note(105)`. Prawidłowy koniec G5S wyświetla zatem `STAGE:105` **oraz** `LAST PHASE BEFORE STOP:98`. Trzy scenariusze:
- `STAGE:105` / `LAST PHASE BEFORE STOP:98`: oczekiwany **PASS** testu G5S, celowy stop przed `memset` EE.
- `STAGE:298` / `LAST PHASE BEFORE STOP:98`: upstream zwrócił kod błędu po stage98; wartość `VALUE` decyduje o diagnozie.
- `STAGE:98` bez 105/298: ostatni widoczny checkpoint, możliwy hang/fault/GS. Pomiędzy 98 a 105 znajduje się tabela `FAKEMOD`; jeśli aktywna, trzeba odczytać `drv.fake.count`, `set_fakemod` i przyczynę return, ale dopiero po ustaleniu rzeczywistego `STAGE`.

**Następny krok:** potrzebne zdjęcie pełnego ekranu z liniami `STAGE`, `VALUE`, `LAST PHASE BEFORE STOP`. Nie generować kolejnej wersji przed rozróżnieniem powyższych przypadków. Nadal nie testować G5B.

## Bezpośrednie dowody (permalinki)

- [Manager 1.0, main.c](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/1.0/dvr-iso-manager/src/main.c#L126-L195)
- [Manager 1.0, CNF SYSTEM/ENGLISH](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/1.0/dvr-iso-manager/src/hdd_boot_config.c#L19-L34)
- [Manager 1.0, LANG patch](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/1.0/neutrino-v02/neutrino-osdlang-en.patch)
- [HDD Neutrino build pin](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/1.0/.github/workflows/build-neutrino-hdd-v1.yml)
- [Neutrino SCMD 0x29 / !QuickBoot](https://github.com/ps2max32/neutrino/blob/aa8d80a1a0b5323cb407c814c0568c3489597c1c/ee/loader/src/main.c#L720-L745)
- [Neutrino emulator CDVDMAN exports](https://github.com/ps2max32/neutrino/blob/aa8d80a1a0b5323cb407c814c0568c3489597c1c/iop/cdvdman_emu/src/exports.tab#L162-L171)
- [OSDMenu switchPSX()](https://github.com/pcm720/OSDMenu/blob/a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f/common/src/psxinit.c#L63-L85)
- [OSDMenu Launcher Quickboot](https://github.com/pcm720/OSDMenu/blob/a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f/launcher/src/handler_quickboot.c#L11-L42)
- [OSDMenu Launcher: źródła kompilacji](https://github.com/pcm720/OSDMenu/blob/a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f/launcher/CMakeLists.txt#L16-L32)
- [PS2SDK EE wysyła 1 bajt 0x29](https://github.com/ps2dev/ps2sdk/blob/d41ea62946e53d9a0c8788bb9eb8c8faf488b559/ee/rpc/cdvd/src/scmd.c#L951-L969)
- [PS2SDK XOSD CDVDMAN](https://github.com/ps2dev/ps2sdk/blob/d41ea62946e53d9a0c8788bb9eb8c8faf488b559/iop/cdvd/cdvdman/src/cdvdman.c#L7823-L7845)
- [PSX przykładowo resetuje IOP po sygnale game start](https://github.com/ps2homebrew/OSD-Initialization-Libraries/blob/main/osd/main.c#L59-L77)
- [NULD IGR źródło](https://github.com/danielnuld/nuld/blob/master/third_party/neutrino-igr/igr.c)
- [NULD wyniki na SCPH-75001](https://github.com/danielnuld/nuld/blob/master/docs/phase12-results.md)
- [Stare nieudane QUIT TEST (nie kopiować)](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/feature/neutrino-desr-igr/neutrino-desr-quit/README_PL.md)
