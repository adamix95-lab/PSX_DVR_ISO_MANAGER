# DESR7000-DVR

Osobny projekt eksperymentalny dla Sony PSX DESR-7000 korzystający z obsługi partycji DVR w Neutrino.

## Cel

Budować i testować [Neutrino PR #228](https://github.com/ps2max32/neutrino/pull/228) autorstwa pcm720 na konsoli PSX DESR-7000 z DVRPwned. Ten projekt **nie** jest kontynuacją `DESR7000-PFS-ISO` i **nie** implementuje odczytu obrazów ISO z partycji APA `+ISO1`, `+ISO2` ani `__common`.

## Co udostępnia PR #228

- `dvr_pfs0:` -> `dvr_hdd0:__xcontents`
- `dvr_pfs1:` -> `dvr_hdd0:__xdata`
- backend Neutrino `-bsd=dvr`

Autor PR raportuje uruchomienie ISO skopiowanych do `__xcontents` na PSX DESR. Funkcja jest eksperymentalna, a zgodność z naszym DESR-7000 nie została jeszcze zweryfikowana.

## Budowanie

1. Na GitHubie otwórz **Actions**.
2. Wybierz **Build Neutrino DVR (PR 228)**.
3. Kliknij **Run workflow** na gałęzi `main`.
4. Po zielonym statusie pobierz artefakt `DESR7000-DVR-Neutrino-PR228`.
5. Zachowaj razem `neutrino.elf`, `modules/` oraz `config/` — sam ELF nie wystarczy.

Workflow pobiera oficjalny PR, a więc aktualny kod PR może się zmieniać. Plik `PR228_COMMIT.txt` w każdym artefakcie zawiera dokładny commit użyty do kompilacji. Ten workflow **nie przesyła danych na konsolę i nie zmienia partycji**.

## Przykładowe wywołanie

```text
neutrino.elf -bsd=dvr -dvd=dvr_pfs0:/GAME.iso
```

To przykład z PR. Uruchomienie z parametrami wymaga launchera/środowiska przekazującego argumenty; zwykłe kliknięcie `neutrino.elf` w wLE nie wybierze automatycznie gry.

## Bezpieczeństwo testu

Obszar DVR został zmniejszony przez `psxrepart`. Nie formatuj ani nie powiększaj partycji DVR w celu sprawdzenia PR. Przed skopiowaniem testowego ISO zweryfikuj wolne miejsce i zachowaj oryginalną strukturę dysku. Nie zakładaj, że działający build gwarantuje uruchomienie gry.

## Źródła i autorzy

- Neutrino: https://github.com/ps2max32/neutrino
- PR #228 (pcm720): https://github.com/ps2max32/neutrino/pull/228

Ten repozytorium zawiera tylko konfigurację budowania i dokumentację projektu; właściwe źródła Neutrino są pobierane podczas kompilacji.
