# Sony PSX DESR-7000 — próba sieciowa v0.7 (ODDZIELNY ELF)

**Wersja DRAFT — NIEPRZETESTOWANA na fizycznym DESR.** Ma ustalić, czy istniejące sterowniki `dev9_ns` / `dvrdrv` / `dvrfile` mogą współpracować z PS2SDK `NETMAN`, `SMAP` i stosem TCP/IP EE. To warunek wstępny późniejszego automatycznego pobierania brakujących okładek.

**To NIE jest nowy Manager!** Uruchamia się jako osobny `DVR_NET_PROBE.ELF`. Niczego nie dodajemy do v0.5.3 / v0.6, nie włączamy sieci podczas uruchamiania gier i nie zmieniamy Quickboot.

## Zakres i bezpieczeństwo

- Diagnostyka uruchamia sprawdzoną sekwencję modułów USB/DVR, odczytuje katalog `dvr_pfs0:/ISO` (nie otwiera ISO do zapisu) i odczytuje mały plik konfiguracyjny z USB.
- Następnie próbuje załadować `netman.irx` i `smap.irx` **bez ładowania drugiego `ps2dev9.irx`**. Konflikt sterowników jest możliwy — właśnie to sprawdzamy.
- Przy aktywnym Ethernet skonfigurowanym statycznym IPv4 łączy się wyłącznie z wpisanym serwerem LAN przez TCP. Wysyła pojedynczy `GET / HTTP/1.0` i odczytuje najwyżej 95 bajtów odpowiedzi, po czym zamyka połączenie.
- Nie pobiera PNG ani danych z internetu, nie zapisuje nic na USB/DVR/APA, nie modyfikuje `BOOT.CNF` i nie wywołuje OSDMenu/Neutrino. Brak dyskowych działań zapisu.
- Po zakończeniu pozostawia wynik na ekranie i czeka na ręczne ponowne uruchomienie konsoli. Nie gwarantuje braku zawieszenia przy niedopasowanych modułach SMAP/DVR, dlatego zaczynamy wyłącznie od osobnego ELF.

## Przygotowanie

1. Zachowaj działający `DVR_ISO_MANAGER.ELF` v0.5.3 (i ewentualnie v0.6) oraz `BOOT.CNF`; nie zastępuj żadnego z nich.
2. Podłącz Sony PSX **kablem Ethernet** do tej samej sieci lokalnej co komputer.
3. Na PC sprawdź jego lokalny adres IPv4 (np. w Windows: `ipconfig`; Linux: `ip -4 addr`).
4. Skopiuj `NET_PROBE.EXAMPLE.CNF` na USB pod nazwą `mass0:/DESR_DVR/NET_PROBE.CNF` i **zmień przykładowe adresy**:
   - `IP` — wolny adres dla Sony PSX, zgodny z Twoją podsiecią; wyłączony z zakresu przydziałów DHCP.
   - `NETMASK` — zwykle `255.255.255.0`.
   - `GATEWAY` — lokalny adres routera.
   - `SERVER_IP` — rzeczywisty IPv4 PC (nie `127.0.0.1`).
   - `SERVER_PORT` — `8000` lub port Twojego lokalnego serwera.
   Żadnych spacji przy `=`. Każde pole podaj raz. Plik ma najwyżej 512 bajtów. **Nie używaj dosłownie przykładów, jeśli Twoja sieć ma inne adresy.**
5. Uruchom na komputerze w dowolnym pustym folderze:
   ```bash
   python3 -m http.server 8000 --bind 0.0.0.0
   ```
   W Windows możesz też użyć `py -m http.server 8000 --bind 0.0.0.0`. Zezwól w zaporze wyłącznie dla **sieci prywatnej / LAN**, nie wystawiaj portu na internet ani nie konfiguruj przekierowania portów.
6. Uruchom z wLaunchELF/wLE-R3Z wyłącznie **osobny** plik `DVR_NET_PROBE.ELF` (także przez USB lub UDPFS). Nie uruchamiaj z jego miejsca `DVR_ISO_MANAGER.ELF`.

## Co sfotografować

Przykładowe etapy na ekranie:
```text
DVR ISO directory: READ-ONLY OK
NET_PROBE.CNF: accepted (PSX IP ...)
netman id=... result=...
smap id=... result=...
PS2IP: STARTED
Ethernet link state: 1
TCP: CONNECTED
HTTP: HTTP/1.0 200 OK
PASS: DVR + Ethernet + local HTTP response.
```

Jeżeli program zatrzyma się na `STOP: SMAP incompatible with DVR dev9_ns`, mamy jasny powód, by poszukać innego sterownika Ethernet; **nie próbuj ładować ps2dev9 równolegle ani zmieniać DVR/APA**. Przy `no Ethernet link` sprawdź kabel i ustawienia. Przy `HTTP failed` sprawdź adres PC, port i firewall.

**Prześlij zdjęcie ekranu i informację, na której ostatniej linii stanął test.** W kolejnym etapie dopiero rozważymy bezpieczne pobieranie PNG na USB. Nie scalać PR z `main` przed wynikiem sprzętowym.

PS2SDK: https://github.com/ps2dev/ps2sdk — EE PS2IP/NETMAN/SMAP; kompatybilność na sprzęcie DVR wymaga osobnego potwierdzenia.
