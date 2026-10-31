# PSX DVR ART Server v0.7.3 — Windows (EXE)

Prosta aplikacja dla komputera Windows, która na żądanie Sony PSX DESR-7000 pobiera brakującą okładkę z internetu przez HTTPS, normalizuje do PNG 140×200 RGB, zapisuje w lokalnym cache i przekazuje konsoli przez HTTP w zaufanej sieci LAN.

**Wersja eksperymentalna.** Kod i budowa `.exe` mogą przejść testy automatyczne, ale nie potwierdzają jeszcze dostępności zewnętrznego archiwum ani zachowania zintegrowanego Managera v0.7.3 na konsoli. Zachowaj działającą v0.6/v0.5.3.

## Skąd okładki?

Źródło: historyczne archiwum OPL Manager `OPLM_ART_2024_09` na Archive.org, wykorzystywane również przez instalator projektu CosmicScale. Przykładowy zdalny adres:

```text
https://archive.org/download/OPLM_ART_2024_09/OPLM_ART_2024_09.zip/PS2/SLES_538.86/SLES_538.86_COV.png
```

**Dostępność plików zależy od Archive.org, regionu wydania oraz kompletności archiwum. Nie wszystkie ID muszą występować.** Serwer nie pobiera metadanych gier, nie korzysta z niezweryfikowanych domen ani nie szuka losowych plików. Gdy archiwum nie odpowiada lub nie ma grafiki, serwer zwraca konsoli 404 — nie generuje fałszywej okładki.

Działanie konsoli: `GET /SLES_538.86_COV.png` → Windows pobiera po HTTPS (jeżeli nie ma w cache) → PNG 140×200 → HTTP 200 z `Content-Length` → konsola zapisuje nowy plik w `ART`.

## Jak uruchomić aplikację .exe

1. Pobierz artefakt **`PSX-DVR-ART-Server-v0.7.3-Windows-EXE`** z workflow **Build PSX DVR ART Server v0.7.3 Windows EXE** w GitHub Actions.
2. Wypakuj ZIP i uruchom **`PSX-DVR-ART-Server-v0.7.3.exe`**. Program nie wymaga ręcznego instalowania Pythona na Windows, ale jest niepodpisanym eksperymentalnym EXE — Windows SmartScreen może pokazać ostrzeżenie.
3. W aplikacji ustaw IP komputera, np. `192.168.0.122`; IP konsoli `192.168.0.240`; port `8000`, tak jak w poprzednich udanych testach. W razie zmiany adresów, zmień je w programie i w `NET_PROBE.CNF` na USB.
4. Kliknij **Start serwera HTTP**. Windows może poprosić o zgodę zapory — zezwalaj **tylko dla sieci prywatnej**. Nie przekierowuj portu na routerze, nie wystawiaj serwera na internet. Program odrzuca żądania od IP innych niż wpisana konsola.
5. W Managerze v0.7.3 naciśnij TRÓJKĄT. Jeśli wykryje brakujące grafiki, zobaczysz liczbę oraz propozycję `X: GET LAN`. Naciśnij X, aby **wyrazić zgodę na pobranie**; O (kółko) oznacza anulowanie. Po powrocie do normalnego menu X ponownie uruchamia grę.
6. Śledź dziennik w aplikacji Windows. Pierwsze pobranie może potrwać, bo komputer pobiera przez HTTPS. Przy kolejnym ID pobranym wcześniej program wyświetli `CACHE` i odczyta plik lokalnie.

Cache w katalogu:
`%LOCALAPPDATA%\\PSX_DVR_ART_SERVER\\cache`

Możesz również ręcznie umieścić tam **poprawne PNG o nazwie `XXXX_123.45_COV.png`**, np. pobrane wcześniej przez OPL Manager. Jest to awaryjna opcja, gdy Archive.org jest niedostępne; nie trzeba jednak tego robić, gdy archiwum działa.

## Bezpieczny test przy obecnych 40/40 okładkach

Na USB masz już komplet 40 okładek. Dlatego TRÓJKĄT prawidłowo pokaże `MISSING: 0` i nie uruchomi pobierania.

Do testu:
1. Zachowaj na komputerze kopię działającego ELF Manager v0.6, `COVERS.IDX`, `BOOT.CNF` oraz `ART/SLES_538.86_COV.png`.
2. Na USB tylko **zmień nazwę** `ART/SLES_538.86_COV.png` na `SLES_538.86_COV.png.SAVED` (niczego nie usuwaj). Zachowaj zapasowy plik.
3. Uruchom zintegrowany Manager v0.7.3 i naciśnij TRÓJKĄT. Spodziewane `MISSING: 1 COVER` i `X: GET LAN`.
4. Uruchom już wcześniej aplikację Windows, następnie naciśnij X na konsoli. Serwer spróbuje pobrać `SLES_538.86_COV.png` z archiwum HTTPS. Jeśli pliku w archiwum nie ma, zajrzyj do dziennika i nie zakładaj, że pobranie się powiodło.
5. Po sukcesie Manager powinien sam odświeżyć `COVERS.IDX` i pokazać okładkę. Uruchom Black przez X **już po zamknięciu okna pobierania**, aby sprawdzić, czy Quickboot nadal działa. Potwierdź zachowanie po restarcie.
6. Gdy pojawi się błąd, wróć do starego działającego ELF i przywróć `.SAVED` do pierwotnej nazwy na USB.

**Uwaga:** pobranie brakujących grafik nie działa w tle. Wymaga ręcznego zatwierdzenia przyciskiem X i uruchomionej aplikacji na PC. Aktualna implementacja przetwarza maksymalnie 40 brakujących ID na uruchomienie i przerwie próby przy 3 kolejnych błędach sieci/źródła. Nie nadpisuje istniejących okładek PNG/JPG ani plików `.PART`. Gdy zapis USB się nie uda, `.PART` może pozostać do ręcznej oceny.

## Uruchomienie ze źródła (opcjonalne)

Dla programisty, bez pliku .exe:

```powershell
py -m pip install -r pc-art-server/requirements.txt
py pc-art-server/psx_dvr_art_server.py
```

Źródło serwera używa Pythona, Pillow, Tkinter i standardowej biblioteki HTTP. Plik .exe buduje GitHub Actions na `windows-latest` z PyInstaller; nie wstrzykuje reklam i nie modyfikuje plików USB ani dysku konsoli.

## Ograniczenia

- Konsola komunikuje się **HTTP tylko wewnątrz LAN**, komputer używa **HTTPS wyłącznie do archiwum Archive.org**.
- Program akceptuje tylko ścisłe nazwy `XXXX_123.45_COV.png`, pobiera maksymalnie 4 MiB danych źródłowych i generuje PNG maksymalnie 256 KiB. Nie wykonuje żądań do dowolnych adresów podanych przez klienta.
- Cache nie nadpisuje istniejących plików. Prawidłowe pliki pozostają na dysku PC, więc późniejsze pobranie jest szybkie.
- Prawa do grafik pozostają u ich właścicieli; używaj ich zgodnie z właściwymi licencjami i warunkami źródła.
- Wersja v0.7.3 ma zostać w **Draft PR** do czasu testów na konsoli i połączenia z rzeczywistym źródłem internetowym.
