# G5S — diagnostyka etapów po SYSTEM.CNF (PSX DESR-7000)

**Stan: kompilacja CI / na fizycznej konsoli nieprzetestowane. Tylko USB. Nie podmieniać stabilnego 1.0.**

## Wynik sprzętowy G5S — ostatni podany numer 98 (2026-11-13)

Użytkownik zgłosił: **„dalej to samo, stage 98 stop”**. G5S doszedł przynajmniej do `g5_note(98)`, czyli za odczyt SYSTEM.CNF i inicjalizację kompatybilności/CDVDMAN/Deckard/ATA/MC/Logo. **Nie mamy jednak pełnego tekstu ekranu i nie wolno ogłaszać błędu ani PASS.**

Ważne dla interpretacji: `g5_stop(105, ...)` czyta `last_stage=g5_stage` zanim wywoła `g5_note(105)`. Prawidłowy koniec G5S wyświetla zatem `STAGE:105` **oraz** `LAST PHASE BEFORE STOP:98`. Trzy scenariusze:
- `STAGE:105` / `LAST PHASE BEFORE STOP:98`: oczekiwany **PASS** testu G5S, celowy stop przed `memset` EE.
- `STAGE:298` / `LAST PHASE BEFORE STOP:98`: upstream zwrócił kod błędu po stage98; wartość `VALUE` decyduje o diagnozie.
- `STAGE:98` bez 105/298: ostatni widoczny checkpoint, możliwy hang/fault/GS. Pomiędzy 98 a 105 znajduje się tabela `FAKEMOD`; jeśli aktywna, trzeba odczytać `drv.fake.count`, `set_fakemod` i przyczynę return, ale dopiero po ustaleniu rzeczywistego `STAGE`.

**Następny krok:** potrzebne zdjęcie pełnego ekranu z liniami `STAGE`, `VALUE`, `LAST PHASE BEFORE STOP`. Nie generować kolejnej wersji przed rozróżnieniem powyższych przypadków. Nadal nie testować G5B.

## Punkt wyjścia — wynik G5R

Na konsoli DESR-7000 zgłoszono **„STAGE 91 STOP”**. W źródle G5R jest to checkpoint `g5_note(91, "SYSTEM.CNF read returned", fd_system_cnf)`, a **nie** wywołanie `g5_stop`. Wersja G5R po STAGE 91 nie miała kolejnego widocznego punktu aż do końcowego `STAGE 199`, więc ostatnie 91 **nie jest samo w sobie dowodem awarii odczytu ISO**.

Jeśli na zdjęciu widać jednocześnie `STAGE 298` i `LAST PHASE BEFORE STOP: 91`, oznaczałoby to **normalny powrót z błędem po odczycie SYSTEM.CNF**, a nie zawieszenie w odczycie. Zdjęcie lub dokładny tekst ekranu pomoże rozróżnić te przypadki.

## Co robi G5S?

G5S zachowuje diagnostyczny Neutrino `mode=0, len=1`, działającą wcześniej ścieżkę z G3B/G5R i identyczny odczyt ISO. Zachowuje też:

- pierwszy ekran już przy wejściu do Neutrino (`STAGE 1`);
- wyświetlanie etapów 10–91 i wyników CDVD/MechaCon;
- `STAGE 298` jako przechwycenie zwykłego `return` (pokazuje kod i ostatnią fazę);
- jednorazową inicjalizację GS, bez powtarzania `init_scr()`.

Dodaje **kolejne widoczne punkty 92–98**, bez modyfikowania funkcji Neutrino:

| STAGE | Co sprawdza |
|---|---|
| 91 | Funkcja odczytu `SYSTEM.CNF` zwróciła wynik nieujemny |
| 92 | Rozpoczyna się wyszukiwanie `cdrom0:` i nazwy BOOT2 w `SYSTEM.CNF` |
| 93 | Ścieżka boot ELF znaleziona i zakończona |
| 94 | Zakończone ustawienia kompatybilności gry z pliku CNF |
| 95 | Ustawienia typu płyty i emulacji CDVDMAN przygotowane |
| 96 | Ustawienia Deckard zakończone |
| 97 | Ustawienia ATA / MC zakończone |
| 98 | Ustawienia PS2 Logo zakończone |
| **105** | **Celowe zatrzymanie PRZED czyszczeniem pamięci EE i instalacją modułów** |
| 292 | Nie znaleziono poprawnej nazwy BOOT2 w `SYSTEM.CNF` |
| 298 | Neutrino zwróciło kod — na ekranie `VALUE` i `LAST PHASE BEFORE STOP` |

**W G5S nie oczekujemy etapu 199.** Zgodnie z założeniem zatrzymujemy się wcześniej, na `STAGE 105`, aby sprawdzić, czy G5R zatrzymywał się podczas czyszczenia obszaru pamięci EE lub późniejszego przygotowania modułów.

G5S **nigdy nie uruchamia gry**. Nie potrzebujemy teraz testować fizycznego QUIT GAME.

## Jak przetestować

1. W GitHub Actions pobierz artefakt `DESR-QUIT-G5S-POST-CNF-USB-UNTESTED` z workflow `Native QUIT G5S - post-SYSTEM.CNF phase trace before EE clear USB`. W środku znajduje się `DESR_QUIT_G5S_POST_CNF_USB.zip`.
2. Rozpakuj **tylko folder `DESR_G5S/`** do katalogu głównego pendrive'a FAT32; nie nadpisuj działającego `DESR_DVR/`, `DESR_G5R/` ani plików HDD.
3. Przez wLE-R3Z 4.80 uruchom `mass0:/DESR_G5S/DVR_ISO_MANAGER.ELF`. Nagłówek ma zawierać `1.0 USB G5S`.
4. Wybierz **tę samą grę** co w G5R, w trybie **SYSTEM**, naciśnij X. Nie oczekuj startu gry, oczekuj informacji diagnostycznej.
5. Zrób zdjęcie końcowego ekranu. Szczególnie ważne: **STAGE, VALUE, LAST PHASE BEFORE STOP**, ewentualnie to, czy przed zatrzymaniem widać choć raz napis G5S.

## Wynik i dalsza interpretacja

- **STAGE 105, POST-CNF PASS** — etap parsowania `SYSTEM.CNF`, konfiguracji gry oraz ustawień CDVDMAN nie blokuje loadera; podejrzenie przenosimy na czyszczenie pamięci EE / IRX / EE core / instrumentację po tym miejscu.
- **STAGE 292** — prawdopodobnie brak poprawnie rozpoznanej ścieżki BOOT2 w odczytanym `SYSTEM.CNF`. Zapisz `VALUE`.
- **STAGE 298** — normalny powrót Neutrino; konieczny kod `VALUE` i `LAST PHASE`.
- **Ostatni STAGE 91–98 bez 105/298** — loader przestał aktualizować obraz lub zawiesił się pomiędzy ostatnim widocznym punktem a kolejnym. Wniosek zależy od zdjęcia.
- **Powrót do XMB bez punktów** — konieczna osobna weryfikacja przekazania sterowania z OSDMenu do Neutrino.

## Bezpieczeństwo

Zmienione są tylko testowe `DVR_ISO_MANAGER.ELF` i `neutrino.elf` we własnym katalogu USB; pozostałe pliki pochodzą bajtowo ze zweryfikowanego publicznego USB 1.0. Manager używa osobnego `BOOT.CNF` w `DESR_G5S`. Moduły DVR/FHI, `ee_core`, język EN, `BOOT.ELF`, HDD, partycja `PP.DVRISO` i XMB pozostają nienaruszone. Zatrzymanie jest celowe, więc po zdjęciu wyjdź zwykłym wyłączeniem konsoli, gdy nic nie zapisuje.
