# Sony PSX DESR-7000 — ART SYNC v0.7.2 (osobny ELF, Draft)

**Eksperymentalna synchronizacja brakujących okładek przez lokalną sieć HTTP.** Nie scalać do `main` bez testów sprzętowych.

Wcześniejsze testy na DESR-7000 potwierdziły współpracę NETMAN/SMAP z DVR, TCP/HTTP 200 oraz pobranie, weryfikację CRC i zapis jednego PNG na USB. To **nie** oznacza jeszcze, że ta nowa wersja z automatycznym wykrywaniem braków jest sprawdzona.

## Dokładnie co robi v0.7.2

Uruchamiasz **osobny `DVR_ART_SYNC_TEST.ELF`**, nie podmieniasz `DVR_ISO_MANAGER.ELF` ani `BOOT.ELF`.

1. Ładuje działające sterowniki USB/DVR, otwiera tylko do odczytu katalog `dvr_pfs0:/ISO`.
2. Odczytuje konfigurację sieci `mass0:/DESR_DVR/NET_PROBE.CNF`.
3. Czyta **`mass0:/DESR_DVR/COVER_SCAN.LOG` z ostatniego ręcznego skanu TRÓJKĄTEM w Managerze**. Przyjmuje tylko poprawne wpisy `NO_ART` i `NO_ART_EXE` zawierające Game ID `XXXX_123.45`. Nie zgaduje ID na podstawie nazwy gry.
4. Dla każdego kandydata sprawdza istniejące `ART/ID_COV.png` oraz `.jpg`. Jeśli którykolwiek istnieje — **nie dotyka go**.
5. Najwyżej **3** brakujące pozycje na uruchomienie próbuje pobrać z lokalnego PC jako `http://SERVER_IP:SERVER_PORT/<GAMEID>_COV.png`, np. `GET /SLES_538.86_COV.png`.
6. Sprawdza pełną odpowiedź HTTP 200, `Content-Length`, `Content-Type: image/png`, PNG i sumy CRC. Limit jednego PNG to 256 KiB.
7. Zapisuje wyłącznie nowy plik `mass0:/DESR_DVR/ART/<GAMEID>_COV.png.PART`, odczytuje go ponownie i porównuje bajt po bajcie z pobranymi danymi, a dopiero potem **zmienia nazwę** na `<GAMEID>_COV.png`. Nie usuwa istniejących `.PART`; jeśli taki pozostał po błędzie, wyświetla `SKIP` i należy rozwiązać sytuację ręcznie.

**Ważne:** po udanym pobraniu ręcznie naciśnij **TRÓJKĄT** w Managerze, aby przebudować `COVERS.IDX` i zobaczyć nową grafikę. V0.7.2 **celowo nie nadpisuje ani automatycznie nie regeneruje `COVERS.IDX`** — tę automatyzację dodamy dopiero po potwierdzeniu bezpiecznych zapisów w `ART`. Nie zapisuje nic na DVR, APA, ISO ani do `BOOT.CNF`, `COVER_SCAN.LOG` czy innych istniejących plików.

## Dlaczego przy obecnych 40/40 okładkach nic się nie pobierze?

Na Twoim USB jest obecnie **40/40 okładek**. Poprawny wynik v0.7.2 z niezmodyfikowanym `COVER_SCAN.LOG` to `NO MISSING ART`. To nie jest błąd.

Żeby wykonać **kontrolowany test jednej brakującej okładki**:

1. Skopiuj na komputer kopie zapasowe `DESR_DVR/ART/SLES_538.86_COV.png` (Black), `COVERS.IDX` oraz działający `DVR_ISO_MANAGER.ELF`. Zostaw `BOOT.CNF` i Neutrino bez zmian.
2. W folderze USB `DESR_DVR/ART` **zmień nazwę** `SLES_538.86_COV.png` na `SLES_538.86_COV.png.SAVED` (nie usuwaj danych). Sprawdź, że nie istnieje inny `SLES_538.86_COV.jpg` ani pozostawiony `SLES_538.86_COV.png.PART`.
3. Uruchom sprawdzony Manager v0.5.3 albo v0.6. Naciśnij **TRÓJKĄT** raz. Nowy `COVER_SCAN.LOG` musi zawierać `Black.iso\tNO_ART_EXE\tSLES_538.86`. Inne wpisy powinny pozostać poprawne.
4. Na komputerze `192.168.0.122` w osobnym katalogu umieść kopię okładki, ale pod **oryginalną nazwą** `SLES_538.86_COV.png`. Uruchom tam:

   ```powershell
   python3 -m http.server 8000 --bind 192.168.0.122
   ```

   Zapora Windows: dostęp tylko z zaufanej sieci prywatnej, bez przekierowywania portów.
5. Na USB pozostaw działający `NET_PROBE.CNF`:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

6. Uruchom **`DVR_ART_SYNC_TEST.ELF`** przez wLE, **nie** podmieniaj nim Managera. Oczekujemy komunikatów `GET /SLES_538.86_COV.png`, `PNG: CRC OK`, `SAVED: SLES_538.86_COV.png` i `ART SYNC: 1 saved of 1 attempted`.
7. Wejdź na USB: nowy `ART/SLES_538.86_COV.png` powinien otwierać się na PC, a `.SAVED` ma pozostać nietknięty. Możesz porównać `Get-FileHash` obu kopii.
8. Ponownie uruchom Manager, naciśnij TRÓJKĄT i upewnij się, że Black ma okładkę i gra nadal startuje. Dopiero potem zdecyduj, czy usunąć ręcznie zapasowy `.SAVED`.

Jeżeli po restarcie wolisz powrócić do stanu sprzed testu, na PC usuń **wyłącznie nowo pobrany** `SLES_538.86_COV.png`, przywróć `.SAVED` do oryginalnej nazwy, przywróć kopię `COVERS.IDX` albo skanuj TRÓJKĄTEM.

## Inne wyniki

- `COVER_SCAN.LOG missing` — wykonaj TRÓJKĄT w działającym Managerze.
- `NO MISSING ART` — wszystko zidentyfikowane lub okładki są już na USB.
- `HTTP missing or invalid PNG` — pliku o ID brak w serwerze PC (404), plik nie spełnia ograniczeń lub sieć przerwała transfer.
- `SKIP: stale .PART` — poprzedni test przerwał zapis; **nie** nadpisujemy nieznanego pliku tymczasowego.
- `USB staging/verify/rename` — błąd zapisu lub weryfikacji; .PART pozostawiono do ręcznej diagnozy.
- `ART SYNC: 0 saved` — nic nowego nie zapisano.

**Nie jest to jeszcze samodzielne pobieranie z internetu.** HTTPS i wybór wiarygodnego źródła OPL Manager / Archive.org pozostają osobnym krokiem. Lokalny serwer PC udostępnia pliki pod oczekiwanymi nazwami Game ID.

### Zasada wdrożeniowa

Wersja v0.7.2 pozostaje jako **Draft PR**, bez scalania. Dopiero po wyniku na sprzęcie rozważamy więcej niż 3 pobrania, integrację z menu Managera i automatyczną odbudowę indeksu.
