# PSX DVR ISO Manager v0.8

Pierwsze pełne wydanie z **automatycznym pobieraniem brakujących okładek** z poziomu Sony PSX DESR.

## Aktualizacja pliku EXE i instrukcji (v0.8)

- **Cały interfejs programu Windows jest po angielsku.** Nagłówek **Cover Sources** opisuje źródło główne **OPL Manager Archive (Archive.org)** — pionowe okładki pudełkowe PS2 — oraz zapasowe **CosmicScale PSBBN Art Database (GitHub)** — często kwadratowe grafiki gier używane tylko wtedy, gdy OPL nie zwróci poprawnego PNG.
- W obu README wyjaśniono konfigurację `NET_PROBE.CNF`, w tym **`IP` jako ręcznie nadany statyczny adres konsoli PSX DESR**, oraz odczytywanie `NETMASK`, `GATEWAY` i `SERVER_IP` z polecenia Windows **`ipconfig /all`**. Pole `SERVER_PORT` musi odpowiadać portowi w aplikacji.
- Plik **`PSX-DVR-ART-Server-v0.8.exe`** dołączony do tego wydania został przebudowany i podmieniony po udanych testach na Windows. **`DVR_ISO_MANAGER.ELF` pozostaje bez zmian.**

## Co nowego

- **12 gier na stronie** — LEWO/PRAWO zmienia stronę, GÓRA/DÓŁ wybiera grę.
- **Game ID + PNG ART** — kolorowe okładki i indeks `mass0:/DESR_DVR/COVERS.IDX`.
- **TRÓJKĄT** odczytuje gry z DVR, tworzy indeks i raportuje brakujące okładki.
- **X** na ekranie pytania uruchamia ART SYNC, **O** anuluje; ponowny TRÓJKĄT pozwala spróbować ponownie bez restartu.
- **Windows ART Server v0.8.exe** — PC pobiera okładki przez HTTPS z OPL Manager Archive.org; bazę CosmicScale PSBBN wykorzystuje jako zapasowe źródło. Konsola komunikuje się z PC przez lokalny HTTP.
- Zapis wyłącznie nowych obrazów PNG do `mass0:/DESR_DVR/ART`; weryfikacja CRC i pliku tymczasowego przed publikacją, a potem **automatyczna odbudowa indeksu**.
- Quickboot przez OSDMenu/Neutrino oraz wybór języka SYSTEM/ENGLISH bez zmian.

## Test sprzętowy

**Sony PSX DESR-7000:** 40 obrazów ISO na partycji DVR, usunięte wszystkie okładki oraz pliki indeksu; pobrano **40/40**, wszystkie poprawnie wyświetlone, uruchomiono **REZ po angielsku**. Potwierdzono też anulowanie pobierania O, ponowny TRÓJKĄT i uruchamianie gry po synchronizacji.

Zapasowa baza PSBBN została sprawdzona testami automatycznymi, ale użycie jej w rzeczywistym zestawie 40/40 nie jest potwierdzone osobnym logiem.

## Zawartość wydania

- `DVR_ISO_MANAGER.ELF` — program na konsolę, uruchamiany z USB albo UDPFS; wymaga pozostałych plików OSDMenu/Neutrino na USB.
- `PSX-DVR-ART-Server-v0.8.exe` — aplikacja PC na Windows z interfejsem Start/Stop. Nie wymaga ręcznej instalacji Pythona.
- [README po polsku](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/main/README_PL.md) oraz [English README](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/main/README.md) zawierają instalację, konfigurację sieci i sterowanie.

**Uwaga:** nie jest to zestaw pełnych plików Neutrino ani bibliotek gier. Nie zastępuj działającego `BOOT.ELF`, `BOOT.CNF`, Neutrino ani sterowników DVR bez potrzeby. Zachowaj kopie zapasowe. Manager nie zapisuje na partycji DVR.
