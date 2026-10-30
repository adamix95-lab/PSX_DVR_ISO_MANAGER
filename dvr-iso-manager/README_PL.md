# PSX DVR ISO Manager v0.5.1 — czytelniejsze okładki (test)

**UWAGA: wersja nieprzetestowana na DESR-7000.** Zachowaj działający ELF v0.5 i nie scalaj przed testem.

## Co zmienia v0.5.1

- Zamiast starej mozaiki 11×14 wielokolorowych znaków: prawdziwe piksele PNG w miniaturze **88×128** (proporcje zbliżone do OPL 140×200).
- Obraz w tym samym miejscu po prawej stronie. Pozostaje czarne tło, tekst MONO, 12 pozycji na stronę.
- Minimalny moduł przesyła gotowy raster bezpośrednio przez **ten sam kanał GIF/DMA i format PSMCT32**, z których korzysta PS2SDK `scr_putchar`. Nie inicjalizuje gsKit ani GS; nie zmienia trybu wideo.
- PNG jest dekodowany do ograniczonego bufora 44 KiB, dopiero po poprawnym dekodowaniu rysowany. Nie zmieniamy ścieżek ART, indeksu ani jego skanowania.
- Nadal nie obsługujemy podglądu JPG ani przeplotu Adam7 — pozostaje fallback tekstowy.
- Bez zmian w v0.5: `TRIANGLE` indeksuje; `COVERS.IDX` jest odczytywany po restarcie; `LEFT/RIGHT` zmienia strony; `X` uruchamia grę przez ten sam USB OSDMenu/Neutrino.

## Test na DESR-7000

1. Zachowaj ELF v0.5 działający z kolorową okładką oraz kopię `BOOT.CNF` na PC.
2. Pobierz tylko nowy `DVR_ISO_MANAGER.ELF`; nic innego w `DESR_DVR` nie zmieniaj.
3. Uruchom z wLE-R3Z / UDPFS albo USB. **Nie naciskaj trójkąta** na początku: gotowy indeks z v0.5 powinien zadziałać.
4. Sprawdź czy okładka Cold Fear / `SLUS_210.47` jest wyraźniejsza, mieści się po prawej i nie nachodzi na stopkę.
5. Przełącz kilka gier i stron; upewnij się, że brakujące okładki pokazują `NO COVER`, a tekst pozostaje stabilny.
6. Po udanym teście interfejsu sprawdź X na jednej znanej grze (np. Silent Hill 2).

**Jeśli wystąpi czarny ekran / artefakty / zawieszenie: wróć do ELF v0.5 bez zmian plików USB.**

## Wymagane okładki

`mass0:/DESR_DVR/ART/SLUS_210.47_COV.png` i pozostałe nazwy zgodne z OPL. Rekomendowane źródło PNG: 140×200 px, ewentualnie inne PNG do 512×512 px.

## Co zostało sprawdzone w v0.5 (baza)

Na prawdziwym DESR-7000: 40 gier i paginacja, wczytanie `COVERS.IDX` po restarcie, rozpoznanie ID i okładki, kolorowa mozaika PNG oraz próbne uruchomienie Silent Hill 2. Nowy raster v0.5.1 czeka na odrębny test.
