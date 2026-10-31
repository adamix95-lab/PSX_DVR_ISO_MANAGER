# DESR Local PNG Fetch v0.7.1 — test pobrania JEDNEJ okładki z komputera

**EKSPERYMENT — NIEPRZETESTOWANY na konsoli.** Poprzedni v0.7 zakończył się sukcesem: użytkownik wykazał na Sony PSX DESR-7000 równoczesne działanie sterowników DVR/NETMAN/SMAP, aktywny Ethernet, `TCP: CONNECTED` i `HTTP/1.0 200 OK` z lokalnego komputera.

Ta wersja jest **oddzielnym** plikiem `DVR_COVER_FETCH_TEST.ELF`, NIE `DVR_ISO_MANAGER.ELF` ani `DVR_NET_PROBE.ELF`.

## Co zrobi — ograniczony test

- Włączy działające sterowniki USB i DVR oraz NETMAN + SMAP bez drugiego DEV9.
- Ponownie sprawdzi katalog `dvr_pfs0:/ISO` tylko do odczytu.
- Użyje tej samej konfiguracji `mass0:/DESR_DVR/NET_PROBE.CNF`.
- Wykona tylko **jedno** żądanie do lokalnego komputera: `GET /NET_TEST_COV.png HTTP/1.0`.
- Odbierze nie więcej niż **256 KiB PNG + 2048 bajtów nagłówków HTTP**. Odrzuci brak statusu 200, brak prawidłowego `Content-Length` lub `Content-Type: image/png`, niekompletne pliki i PNG z uszkodzonymi sumami CRC.
- Dopiero gdy kompletny PNG przejdzie kontrolę struktury oraz CRC, zapisze na USB **wyłącznie** `mass0:/DESR_DVR/NET_DOWNLOAD_TEST.PNG`.
- Jeśli plik docelowy już istnieje, **przerwie test bez nadpisywania go**.

Ten tester nie czyta ani nie zapisuje `COVERS.IDX`, nie dotyka `ART`, `BOOT.CNF`, ISO, APA ani partycji DVR. Zapis wyniku USB jest jedynym celowym działaniem zapisu. Podczas nagłego odłączenia USB możliwy jest niepełny plik testowy; nie używaj go jako okładki bez sprawdzenia na PC.

## Instrukcja — Windows (Twoja dotychczasowa sieć)

1. Zachowaj działający Manager v0.5.3/v0.6 i pliki Neutrino bez zmian. Podmień **wyłącznie oddzielny tester**, uruchamiając `DVR_COVER_FETCH_TEST.ELF` z wLE.
2. Na komputerze (`192.168.0.122`) utwórz pusty folder, np. `C:\\DESR_HTTP_TEST`.
3. Skopiuj do tego folderu **jedną ze swoich okładek PNG** z `DESR_DVR/ART`, nadając kopii dokładnie nazwę **`NET_TEST_COV.png`**. Polecam zwykły, nieprzeplatany PNG RGB lub RGBA (8 bitów), poniżej 256 KiB. Nie trzeba zmieniać oryginalnej okładki.
4. Uruchom PowerShell w tym folderze i wpisz:

   ```powershell
   python3 -m http.server 8000 --bind 192.168.0.122
   ```

   Nie otwieraj portu na internet; zapora Windows — sieć prywatna.
5. Pozostaw na USB `NET_PROBE.CNF` z poprzedniego testu:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

   Adres `192.168.0.240` musi wciąż być wolny/rezerwowany dla Sony PSX.
6. **Przed testem** sprawdź, czy na USB **NIE MA** pliku `mass0:/DESR_DVR/NET_DOWNLOAD_TEST.PNG`. Jeśli jest z poprzedniej próby, skopiuj go na PC, sprawdź, a następnie usuń ręcznie przed nowym testem.
7. Uruchom `DVR_COVER_FETCH_TEST.ELF`. Powinien pokazać `GET /NET_TEST_COV.png`, `PNG: VALID CRC` i `USB: NET_DOWNLOAD_TEST.PNG SAVED`, a następnie `PASS: DVR + LAN PNG + USB test output.`
8. Sprawdź na komputerze, czy plik `NET_DOWNLOAD_TEST.PNG` na USB wygląda identycznie jak źródłowy `NET_TEST_COV.png`; możesz porównać hashe SHA-256 poleceniem `Get-FileHash` w PowerShell. Prześlij zdjęcie wyniku z konsoli.

## Co oznacza błąd

- `STOP: NET_DOWNLOAD_TEST.PNG already exists` — tester chroni istniejący wynik przed nadpisaniem.
- `HTTP read timeout` — brak odpowiedzi serwera, sprawdź firewall i IP.
- `REJECT: not complete valid HTTP 200 PNG` — plik nie istnieje (404), ma nieobsługiwany format, przekracza limit lub jest uszkodzony. Sprawdź nazwę `NET_TEST_COV.png` i rozmiar.
- `USB test output open failed` — plik nie może zostać utworzony na USB.
- `USB write failed` — błąd pendrive'a; usuń częściowy plik testowy po sprawdzeniu na PC.

W tej fazie **nie pobieramy okładek bezpośrednio z Archive.org ani nie indeksujemy ich w Managerze**. Najpierw potwierdzamy bezpieczny zapis PNG po lokalnym HTTP, potem osobny etap automatycznego pobierania brakujących grafik. Nie scalać tej wersji przed fizycznym testem.
