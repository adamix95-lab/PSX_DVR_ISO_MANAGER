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
