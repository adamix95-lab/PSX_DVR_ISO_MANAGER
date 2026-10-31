# PSX DVR ISO Manager v0.5.3 — czytelniejsze okładki PNG (TEST)

## Poprawka v0.7.3 — ekran skanowania i ponowne pytanie po O (31.10.2026)

Na DESR-7000 użytkownik potwierdził pobieranie **3/3** brakujących PNG i automatyczne **40/40**. Zgłoszone uwagi:
1. Pytanie **„X: DOWNLOAD / O: SKIP”** powinno pozostać na ekranie **„UPDATING GAME LIST AND COVER INDEX...”**, nie nakładać się na zwykłą listę gier.
2. Po anulowaniu przyciskiem **O** kolejny skan **TRÓJKĄTEM** ma ponownie wykryć braki i wyświetlić pytanie bez restartowania programu.

Poprawka pokazuje odrębny ekran wyniku skanu z liczbą rozpoznanych gier, znalezionych i brakujących okładek oraz przyciskami `X: DOWNLOAD` / `O: SKIP / RETURN`. O nie zmienia ani plików, ani informacji o tym, ile okładek brakuje; ponowny TRÓJKĄT zawsze wykonuje skan i może ponownie zaproponować pobieranie. Wynik skanowania jest dodatkowo porównywany z `COVER_SCAN.LOG`: błędy raportu są sygnalizowane jawnie zamiast fałszywego `MISSING: 0`.

**Do ponownego testu** pobierz artefakt `PSX-DVR-ISO-Manager-v0.7.3-SCAN-PROMPT-CANCEL-FIX-UNTESTED`. Podmień **wyłącznie** `DVR_ISO_MANAGER.ELF`; aplikacja Windows `.exe` v0.7.3 i `NET_PROBE.CNF` pozostają bez zmian.

**Test sprzętowy:** zrób kopię v0.7.3 oraz ART, przygotuj jeden brak (np. Black przez zmianę nazwy `.png` na `.png.SAVED`), wciśnij TRÓJKĄT, zobacz osobny ekran `MISSING: 1`, wciśnij O, ponownie TRÓJKĄT — pytanie powinno wrócić. Dopiero teraz X pobiera, po czym Manager odbudowuje `COVERS.IDX`. Sprawdź Quickboot X poza pytaniem. Nie scalamy tej korekty z `main` przed testem.

## v0.7.3 — ART SYNC w Managerze + serwer Windows .exe (Draft, bez testu na konsoli)

Po ręcznym skanie **TRÓJKĄTEM** program aktualizuje zwykły `COVERS.IDX` i `COVER_SCAN.LOG`, a następnie wylicza **tylko braki z rozpoznanym Game ID** (`NO_ART` / `NO_ART_EXE`). Na ekranie pojawia się liczba brakujących okładek oraz pytanie `X: DOWNLOAD LAN HTTP`. Naciśnięcie **X** w tym oknie włącza Ethernet (NETMAN/SMAP bez drugiego DEV9) i pobiera okładki przez lokalny HTTP. **O (kółko)** anuluje. Gdy okna nie ma, **X nadal uruchamia grę** przez BOOT.ELF / Neutrino.

Po udanym pobraniu PNG do `mass0:/DESR_DVR/ART/<GAMEID>_COV.png` Manager automatycznie ponownie uruchamia sprawdzony skaner Game ID i aktualizuje `COVERS.IDX`; nie trzeba naciskać TRÓJKĄTA drugi raz. Błąd sieci nie powinien blokować normalnego korzystania z aplikacji.

**Odrębna aplikacja PC:** [PSX DVR ART Server v0.7.3 (Python + Windows EXE)](../pc-art-server/README_PL.md). Konsola pyta np. `GET /SLES_538.86_COV.png`, a serwer PC próbuje pobrać grafikę po HTTPS z archiwum OPL Manager, przeskalowuje do 140×200 RGB PNG, zapisuje do cache i odsyła konsoli. Źródło zewnętrzne nie jest gwarantowane i nie wszystkie ID muszą być dostępne.

**Uwaga:** Testy automatyczne/kompilacja nie oznaczają potwierdzenia na fizycznym DESR-7000. Najpierw zrób kopię działającego ELF v0.6, `BOOT.CNF`, `COVERS.IDX` i wybranej okładki. Przy dotychczasowych 40/40 program pokaże 0 braków. W kontrolowanym teście zmień nazwę `ART/SLES_538.86_COV.png` na `.png.SAVED`, naciśnij TRÓJKĄT, potwierdź X przy `MISSING 1`, sprawdź wygląd Black, ponowny start Managera i uruchomienie gry X. Przy błędach wróć do starego ELF i kopii danych.

Ochrona: maksymalnie 40 prób na uruchomienie, przerwanie po 3 kolejnych błędach pobierania, limit 256 KiB na PNG, CRC, USB staging `.PART` i odczyt zwrotny. Istniejące `.png`, `.jpg`, `.PART` i dane DVR nie są nadpisywane. Stos sieciowy ładuje się **dopiero po zgodzie X**, nigdy przy starcie.

**PR Draft; niczego nie scalać z `main` bez testu sprzętowego.**


**Nowy renderer v0.5.1 jest NIEPRZETESTOWANY na DESR-7000.** Przed testem zachowaj działający ELF v0.5. Nie scalać do `main`.

## v0.5.3 — awaryjne rozpoznawanie Game ID (test, 30.10.2026)

W dostarczonym `COVER_SCAN.LOG` spośród 40 ISO wystąpiło 34 × `FOUND_PNG` oraz sześć × `NO_ID`: **Black, Burnout Dominator, Killzone, Mafia, Shadow of the Colossus, Wipeout Fusion**. Nie wystąpiło `NO_ART`, więc nie mamy powodu na tym etapie zmieniać nazw okładek.

**Zmiana (tylko ręczny TRÓJKĄT):** najpierw próbujemy zwykłego `SYSTEM.CNF`/`BOOT2`, a jeżeli odczyt ID się nie powiedzie, przeglądamy wyłącznie katalog główny obrazu ISO9660 (sektory po 2048 bajty) w poszukiwaniu pliku wykonywalnego o nazwie `SLES_123.45;1`, `SLUS_123.45;1` itp. Jeżeli istnieje **dokładnie jeden różny identyfikator**, wykorzystujemy go jako awaryjny Game ID i szukamy istniejącej okładki OPL. Jeżeli jest wiele ID, **nie zgadujemy**.

Nowy raport `mass0:/DESR_DVR/COVER_SCAN.LOG` rozróżnia:
- `FOUND_PNG_EXE` / `FOUND_JPG_EXE`: okładka po ID odzyskanym z nazwy pojedynczego pliku startowego.
- `NO_ART_EXE`: rozpoznane ID zapasową metodą, ale nie odnaleziono okładki — sprawdź dokładną nazwę na USB.
- `NO_PVD`: nierozpoznany standardowy opis ISO9660 (możliwy niestandardowy układ/rozmiar sektora).
- `NO_SYSTEM`: brak rozpoznanego `SYSTEM.CNF` i brak pojedynczego pliku startowego.
- `BAD_BOOT2`: `SYSTEM.CNF` istnieje, ale jego wpisu `BOOT2` nie rozpoznano i nie znaleziono zapasowego ID.
- `BAD_DIRECTORY`: niepoprawny/nieobsługiwany rekord katalogu.
- `AMBIGUOUS_EXE`: wiele różnych identyfikatorów wykonywalnych, więc pominięto automatyczny wybór.
- `READ_ERROR`: błąd odczytu DVR.
- Dotychczasowe `FOUND_PNG`, `FOUND_JPG`, `NO_ART` pozostają bez zmian.

**Test:** zachowaj stary działający ELF v0.5.1 i `COVERS.IDX` na PC. Pobierz `PSX-DVR-ISO-Manager-v0.5.3-BOOT-ID-FALLBACK-UNTESTED`, uruchom jedynie nowy ELF, naciśnij TRÓJKĄT i przekaż nowy `COVER_SCAN.LOG`. Jeżeli skaner odnajdzie dodatkowe okładki, potwierdź je na ekranie. Gdy wyniki nadal są negatywne, nowy status pokaże, którą część ISO trzeba zbadać. Bez zapisu na DVR, bez zmian w Quickboot i rendererze. **Do testu sprzętowego traktuj tę wersję jako Draft.**

## v0.5.2 — diagnostyka brakujących okładek (Draft, test na konsoli do wykonania)

W v0.5.2 **przywrócono ulubione położenie panelu** z v0.5.1 (Game ID w wierszu 7, okładka od y=64). Nie wprowadzamy wyrównania do LANGUAGE.

Dodano wyłącznie diagnostykę ręcznego skanowania: po naciśnięciu **TRÓJKĄTA** oprócz zwykłego `COVERS.IDX` program próbuje zapisać `mass0:/DESR_DVR/COVER_SCAN.LOG`. Zwykłe uruchomienie nie generuje logu.

Przykład:
```text
PSXCOVSCAN1
Black.iso	NO_ID	-
Burnout Dominator.iso	NO_ART	SLUS_123.45
God of War.iso	FOUND_PNG	SCUS_973.99
```
Przykładowy `SLUS_123.45` nie jest rzeczywistym Game ID Burnout.

- `NO_ID` — brak Game ID z `SYSTEM.CNF` w ISO albo nierozpoznany układ ISO9660.
- `NO_ART` — Game ID rozpoznano (widać go w logu), ale nie otwarto pliku `ART/ID_COV.png` ani `.jpg`. Porównaj znak po znaku nazwę z logu i nazwy na USB.
- `FOUND_PNG` — okładka dopasowana i gotowa do podglądu.
- `FOUND_JPG` — JPEG dopasowany, ale renderer v0.5.1 nie dekoduje JPG.
- `READ_ERROR` — błąd odczytu obrazu ISO z DVR; poprzedni indeks jest zachowany.

**Nie trzeba podmieniać innych plików.** Zachowaj ostatni działający ELF. Po skanowaniu otwórz `COVER_SCAN.LOG` na komputerze i prześlij linie dotyczące Black, Burnout Dominator, Killzone, Mafia, Shadow of the Colossus i Wipeout Fusion. Pozwoli to naprawić konkretną przyczynę bez zgadywania Game ID.

Indeks `COVERS.IDX`, uruchamianie gier, Neutrino, BOOT.CNF i odczyt DVR pozostają bez zmian. Nie scalać bez testów sprzętowych.

## Położenie okładki

Powrócono do poprzedniego, preferowanego układu Game ID / okładki: wiersz 7 / y=64. Odrzucono eksperyment z wyrównaniem do LANGUAGE.

## Poprawka po pierwszym teście v0.5.1 (30.10.2026)

Na prawdziwym DESR-7000 wyświetliła się okładka God of War II, ale kolory były zniekształcone, a obraz miał widoczne schodki. Przyczyną kolorów było błędne zapisanie piksela jako `0xRRGGBB` zamiast kolejności GS PSMCT32 (`R | G<<8 | B<<16`). Bieżąca kompilacja poprawia kanały czerwony/niebieski i zastępuje skalowanie nearest-neighbor **bilinearnym filtrowaniem w poziomie i pionie**.

**Użyj artefaktu `PSX-DVR-ISO-Manager-v0.5.2-COVER-SCAN-LOG-UNTESTED`, nie starszego ZIP `OPL-PREVIEW-UNTESTED`.** Nie jest on jeszcze przetestowany na konsoli. Zmienia wyłącznie odczyt/raster okładki; `COVERS.IDX`, Quickboot i inne funkcje są bez zmian.

## Co nowego?

- Podgląd PNG zmienia się z mozaiki **11×14 znaków** na bitmapę **88×128 prawdziwych pikseli** (proporcje zbliżone do OPL 140×200).
- Okładka pozostaje w tej samej prawej kolumnie, bez nachodzenia na stopkę. Tekst MONO, układ 12 gier na stronę i czarne tło bez zmian.
- Nowy transfer obrazu korzysta z już uruchomionego przez PS2SDK framebufferu GS; **nie resetuje GS ani nie inicjalizuje gsKit**.
- Index `COVERS.IDX`, skanowanie `TRIANGLE`, `BOOT.CNF`, `BOOT.ELF`, Neutrino i Quickboot są nietknięte.
- JPG i PNG Adam7 nadal wyświetlają tekst zamiast okładki.

## Pierwszy test na konsoli

1. Zrób kopię działającego v0.5 ELF i BOOT.CNF na PC.
2. Pobierz z Actions artefakt `PSX-DVR-ISO-Manager-v0.5.2-COVER-SCAN-LOG-UNTESTED` i uruchom **tylko** nowy ELF z USB / UDPFS.
3. Korzystaj ze starego folderu `ART` i indeksu `COVERS.IDX`; nie musisz naciskać TRÓJKĄTA.
4. Sprawdź Cold Fear / `SLUS_210.47`: czy grafika jest wyraźniejsza, stabilna, nie uszkadza listy ani stopki.
5. Zmień kilka gier i stron, potwierdź `NO COVER` i na końcu uruchom znany tytuł (np. Silent Hill 2).
6. Jeśli wystąpi problem z obrazem lub zawieszenie, wróć do starego ELF v0.5.

---

## Previous release: PSX DVR ISO Manager v0.5 — okładki (wersja rozwojowa)

**v0.4: NIEPRZETESTOWANA na Sony PSX DESR-7000 — nie scalać przed testami sprzętowymi.**
Wcześniejsza v0.3 MONO była testowana na konsoli.

## Co nowego w v0.4

- **LEWO / PRAWO** zmienia stronę listy o maksymalnie 12 gier.
- Zaznaczenie pozostaje w tym samym wierszu, jeżeli istnieje on na następnej stronie; na krótszej ostatniej stronie przechodzi na ostatnią dostępną grę.
- Na pierwszej i ostatniej stronie przyciski nie zawijają listy.
- **GÓRA / DÓŁ** nadal wybiera grę po jednej pozycji, tak jak w v0.3 (również między stronami).
- **TRÓJKĄT** nadal ponownie skanuje tylko folder ISO. Indeks okładek, identyfikatory SLES/SLUS i grafiki są planowane dopiero na v0.5.
- Brak zmian w skanowaniu DVR, generowaniu BOOT.CNF, trybach językowych i uruchamianiu gier.

## Zmiany bazowe v0.3 MONO

## Co zmieniono w v0.3

- Nagłówek `PSX DVR ISO MANAGER` z małym oznaczeniem `v0.3`.
- Monochromatyczny interfejs: czarne tło, biały tekst; zaznaczona gra wyróżniona znakiem `>`.
- Ukryte diagnostyczne linie `DVR: dvr_pfs0:/ISO` i `DVRP notice / hdd0 / pfs0 root`.
- Zwykły wynik skanowania zastąpiono licznikiem `N GAMES`; 
- Na ekranie nie wyświetlamy `.iso` ani `.ISO`, ale pełna oryginalna nazwa nadal trafia do BOOT.CNF.
- Ukryte `Selected 1 / 7` i techniczna linia `Backup: ... No DVR writes`.
- Wyłączony blokowy kursor PS2SDK (`scr_setCursor(0)`), który zostawiał białe kwadraty między sekcjami.
- Lista ma maksymalnie 12 pozycji na stronę; numer strony pokazujemy dopiero przy większej liczbie gier.
- Na dole widoczne są tylko sterowanie i, w razie potrzeby, komunikat o teście CNF/błędzie.

## Sterowanie

- Wciśnij **UP** lub **DOWN**, aby wybrać grę z listy
- Wciśnij **LEFT** lub **RIGHT**, aby przejść do poprzedniej/następnej strony (12 pozycji)
- Wciśnij **X** aby uruchomić dany tytuł
- Wciśnij **^** aby ponownie sprawdzić zawartość folderu "ISO"
- Wciśnij **[]** aby zapisać testową konfigurację do *"DVR_TEST.CNF"*
- Wciśnij **SELECT** by zmienić język gry - funkcja dla gier które ustawiają język według menu konsoli (*testowane przy REZ i Metal Slug Anthology*)
- Wciśnij **START** by wyjść z aplikacji

## Bez zmian względem działającej v0.2

- Obrazy ISO nadal odczytywane z `dvr_pfs0:/ISO/` (partycja `__xcontents`).
- Wybór `SYSTEM` używa sprawdzonego `neutrino.elf`, a `ENGLISH` osobnego `neutrino_lang.elf` z `ee_core_lang.elf`.
- Manager tworzy kopię BOOT_RECOVERY.CNF (tylko przy pierwszym X), zapisuje i weryfikuje BOOT.CNF,
  po czym uruchamia OSDLauncher (BOOT.ELF) z odpowiednimi argumentami,
- Nie ma zapisów na partycjach DVR/APA.
- Przełączenie języka nie tłumaczy zasobów gry, jeśli w ISO nie ma tekstów angielskich.
- Użyte są dokładnie te same generatory CNF i funkcje launchera co w v0.2.

**Ważne:** uruchomienie samego Managera z UDPFS NIE przenosi ścieżki Quickboot na sieć.
Pliki `mass0:/DESR_DVR/BOOT.ELF`, BOOT.CNF i obydwa Neutrino pozostają na USB.

## Plan testu v0.4 na DESR-7000

1. Zachowaj kopię działającego pliku ELF v0.3 i dotychczasowego BOOT.CNF na komputerze.
2. Pobierz artefakt `PSX-DVR-ISO-Manager-v0.4-PAGING-UNTESTED` z GitHub Actions i uruchom tylko nowy Manager ELF przez wLE-R3Z / UDPFS lub USB.
3. Przy obecnych 7 grach: LEWO / PRAWO nie powinno zmieniać wyboru ani wyświetlać nowych stron.
4. Przy ponad 12 grach: RIGHT przechodzi na kolejną stronę, LEFT wraca, zaznaczenie jest widoczne; sprawdź ostatnią krótszą stronę.
5. Zweryfikuj GÓRA / DÓŁ, SELECT, TRÓJKĄT oraz KWADRAT; na końcu uruchom jedną sprawdzoną grę przez X.
6. Nie podmieniaj BOOT.ELF ani plików Neutrino podczas testowania samej nawigacji.

**Nie scalać v0.4, zanim powyższe nie zostanie sprawdzone na fizycznym DESR-7000.**
