# DIAG H — odczyt 3 plików OSDMenu / Neutrino z PP.TESTDVR

**Stan:** DIAG G potwierdzony na fizycznym Sony PSX DESR-7000 (2 listopada 2026): RESULT=0, MOUNT=0, BOOT.CNF/ELF/NEUTRINO=-2 na pustej partycji, DVR ISO=3, PS2 PFS=3. START wrócił do XMB. DIAG H wymaga osobnego testu na sprzęcie.

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
