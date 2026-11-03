# PSX DVR ISO Manager v1.0 — wariant USB i wariant HDD

Status: **decyzja projektowa, 3 listopada 2026**. Dokument roboczy na gałęzi eksperymentalnej. Nie zmienia wersji v0.8 ani gałęzi `main`.

## Zakładane wydanie v1.0

Dwa **oddzielne warianty dystrybucyjne** w ramach jednego wydania, z jasno opisanymi zależnościami:

| | v1.0 USB | v1.0 HDD |
|---|---|---|
| Punkt wyjścia | Fizycznie sprawdzona v0.8 | Osobna ścieżka eksperymentalna |
| Główne pliki Managera/OSDMenu/Neutrino | Na USB (`mass0:`) | Docelowo na HDD, bez USB |
| Obrazy gier | Partycja DVR `dvr_pfs0:/ISO/` | Ta sama partycja DVR |
| Zmiany przed ukończeniem HDD | **Nie ruszać kodu ani działającej konfiguracji v0.8 USB** | Prace tylko na gałęzi HDD / Draft PR #26 |
| Warunek uznania za gotowy | Zachowane działanie v0.8; pakiet odpowiednio nazwany | Uruchomienie całego łańcucha Manager → OSDLauncher/OSDMenu → rozszerzone Neutrino → ISO **bez USB**, potwierdzone fizycznie |

**Nazwy pakietów na etapie końcowego wydania (propozycja):**
`PSX-DVR-ISO-Manager-v1.0-USB.zip` i `PSX-DVR-ISO-Manager-v1.0-HDD.zip`. Nie publikować v1.0 HDD jako działającej, zanim łańcuch zostanie sprawdzony na konsoli. Wariant USB można oprzeć bez zmian funkcjonalnych o sprawdzoną v0.8; nie deklarować osobno testów v1.0 bez faktycznego testu paczki.

## Zależności źródłowe (czytać łącznie)

- Neutrino upstream: https://github.com/ps2max32/neutrino
- Rozszerzenie pcm720 dodające obsługę DVR/moduły: https://github.com/ps2max32/neutrino/pull/228
- Gałąź pcm720: https://github.com/pcm720/neutrino/tree/bsd-dvr

To **nie** brak obsługi DVR w standardowym upstream jest pierwotnym problemem tego projektu. Istniejące rozszerzenie pcm720 dodaje moduły DVR, a OSDLauncher przekazuje m.in. `-bsd=dvr` i `-dvd=dvr_pfs0:/ISO/...`.

**Geneza:** wariant z USB powstał, ponieważ na początku prac kombinacja OSDLauncher + Neutrino + moduły DVR nie dawała się prawidłowo uruchomić bezpośrednio z HDD. USB to sprawdzone obejście, nie architektura docelowa wariantu HDD.

## Co już zweryfikowano na DESR-7000

- DIAG F uruchomił się z HDD; START → XMB jest rozwiązany i nie wymaga dalszych testów.
- DIAG M/N/O/P potwierdziły osobno inicjalizację stosu IOP/PFS, normalizację Quickboot, remount oraz ponowny reset IOP. Nie były testem Neutrino.
- Oryginalny OSDMenu z `DIAG_Q.CNF` uruchomił DIAG F **bez jawnego `-dev9=NICHDD`**.
- Oryginalny OSDMenu z poprawionym launcherem R3 uruchomił benigny `R_ARG.ELF` z `ARGC=3 ARGS_OK=1`, `-bsd=dvr` i `-dvd=...`; celowy fikcyjny ISO nie był otwierany.
- DIAG J przy próbie wykonania Neutrino wrócił do XMB. **Nie wyjaśniono** dlaczego.
- DIAG R2 wykrył jednostkową różnicę końcowego bajtu odczytanego CNF (`0A`→`00`); R3 dopuścił tylko ten wariant w teście. Nie jest to automatyczne wyjaśnienie awarii Neutrino.
- DIAG S zbudowano, ale jego założenia o niezmodyfikowanym upstream oraz sondowaniu TOML w `pfs1:` nie są podstawą do oceny modułów pcm720. Nie jest wymagany do dalszych prac.

## Priorytet: wyłącznie HDD

1. **Porównanie USB/HDD bez modyfikacji USB**: ustalić rzeczywiste ścieżki, katalog roboczy, źródła `config/*.toml`, `modules/*.irx`, argumenty startowe i stan IOP/DEV9 w *działającym* uruchomieniu z USB względem nieudanego z HDD. Porównanie musi uwzględniać build Neutrino z rozszerzeniem pcm720, nie upstream bez DVR.
2. **Znaleźć pierwszy rozbieżny etap**: instrumentować wyłącznie testową kopię Neutrino/OSDLaunchera na gałęzi HDD, z krótkimi markerami przed i po odczycie konfiguracji, dołączeniu nowych modułów DVR, operacjach IOP reset/SCmd, uruchomieniu `dvrfile_fhi` oraz otwarciu obrazu ISO. Pierwszy test bez uruchamiania gry. Gdy konieczny jest zapis logu, rozważyć wyłącznie zewnętrzny kanał i jawny opt-in; nie zapisywać przypadkowo na systemowe partycje Sony.
3. **Usunąć zależność od USB** dopiero po uruchomieniu Neutrino z HDD; następnie przystosować ścieżki Managera i zasobów (okładki, konfiguracje, ewentualny serwer ART) do wariantu HDD bez naruszania USB.
4. **Testy końcowe na fizycznym DESR-7000**: boot Managera bez podłączonego USB, wylistowanie ISO, start gry przez OSDMenu i rozszerzone Neutrino, powrót do XMB, przeglądanie okładek i dostępność funkcji Managera. Dla obu pakietów osobno sprawdzić zawartość release i ścieżki instalacji.

## Zasady zmian

- **Nie modyfikować stabilnego v0.8 USB** ani jego sprawdzonej konfiguracji, dopóki wersja HDD nie będzie działać.
- Draft PR #26 i gałąź `feature/v1.0-hdd-readonly-diagnostics` pozostają eksperymentalne, bez scalania do `main` z automatu.
- Testy i pliki na `PP.TESTDVR` wyłącznie w kontrolowanych etapach, bez zmian na partycjach systemowych Sony. W miarę możliwości odczyt RO, kopia danych przed próbami z zewnętrznymi sterownikami.
- Po stabilizacji HDD rozważyć wspólny kod źródłowy i dwie konfiguracje builda, ale **bez refaktoryzowania USB v0.8 jako warunku wstępnego**.
