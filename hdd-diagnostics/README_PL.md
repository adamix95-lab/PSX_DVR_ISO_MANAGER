# PSX DVR ISO Manager v1.0 — diagnostyka HDD, etap A

**To nie jest Manager v1.0 ani instalator XMB.** To osobny, niewielki plik ELF do pierwszego, bezpiecznego testu odczytu partycji APA/PFS **`PP.TESTDVR`** i istniejącego `dvr_pfs0:/ISO` na Sony PSX DESR-7000.

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
