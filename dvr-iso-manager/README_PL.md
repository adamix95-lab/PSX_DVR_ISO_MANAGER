# PSX DVR ISO Manager v0.5.1 — czytelniejsze okładki PNG (TEST)

**Nowy renderer v0.5.1 jest NIEPRZETESTOWANY na DESR-7000.** Przed testem zachowaj działający ELF v0.5. Nie scalać do `main`.

## Co nowego?

- Podgląd PNG zmienia się z mozaiki **11×14 znaków** na bitmapę **88×128 prawdziwych pikseli** (proporcje zbliżone do OPL 140×200).
- Okładka pozostaje w tej samej prawej kolumnie, bez nachodzenia na stopkę. Tekst MONO, układ 12 gier na stronę i czarne tło bez zmian.
- Nowy transfer obrazu korzysta z już uruchomionego przez PS2SDK framebufferu GS; **nie resetuje GS ani nie inicjalizuje gsKit**.
- Index `COVERS.IDX`, skanowanie `TRIANGLE`, `BOOT.CNF`, `BOOT.ELF`, Neutrino i Quickboot są nietknięte.
- JPG i PNG Adam7 nadal wyświetlają tekst zamiast okładki.

## Pierwszy test na konsoli

1. Zrób kopię działającego v0.5 ELF i BOOT.CNF na PC.
2. Pobierz z Actions artefakt `PSX-DVR-ISO-Manager-v0.5.1-OPL-PREVIEW-UNTESTED` i uruchom **tylko** nowy ELF z USB / UDPFS.
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
