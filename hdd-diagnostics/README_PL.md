# PSX DVR ISO Manager v1.0 — diagnostyka HDD, etap A

**To nie jest Manager v1.0 ani instalator XMB.** To osobny, niewielki plik ELF do pierwszego, bezpiecznego testu odczytu partycji APA/PFS **`PP.TESTDVR`** i istniejącego `dvr_pfs0:/ISO` na Sony PSX DESR-7000.

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
