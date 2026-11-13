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
