# Osobne rozwinięcie: HDD NETFIX

Gałąź: `fix/hdd-cover-network-integrity`, baza HDD:
`d9595b41ad07f2ab0334177ef5c02b4b15e57f2c`.

To próba naprawy pobierania okładek, jeszcze bez potwierdzenia na DESR.
Nie zastępuje wydania USB ani dotychczasowej gałęzi HDD. Nie zmienia
OSDLaunchera, Neutrino, wymuszania języka ani formatu CNF. Nie używa ELF-ów A/B/C.

## Ustalenia i kolejność prac

1. W USB jest wbudowany `modules/dev9_ns.irx`, a HDD używało `ps2dev9.irx`
   z obrazu kompilatora. Identyczne zewnętrzne foldery `modules/` nie
   gwarantowały identycznego środowiska Managera.
2. Źródła DEV9 z Neutrino `aa8d80a1a0b5323cb407c814c0568c3489597c1c`
   oraz PS2SDK `d41ea62946e53d9a0c8788bb9eb8c8faf488b559` mają tę samą
   zasadniczą implementację transferu DMA, pod zmienionymi nazwami eksportów.
   `DEV9_NO_SHUTDOWN=1` wyłącza wyłączanie DEV9, nie ATA/DVR DMA.
   To nie jest dowód, że samo DEV9 powodowało błędne CRC.
3. NETFIX ładuje tylko znany moduł `dev9_ns` (kontrola SHA-256 w CI),
   zachowując ATAD/HDD/PFS. Pozostałe moduły IOP i biblioteki EE powstają
   z jednego przypiętego SDK. Nie ładuje drugiego DEV9 po starcie Ethernetu.
4. W NETMAN odbiornik bezpośrednio udostępniał IOP bufory lwIP, a po DMA
   wywoływał `sceSifWriteBackDCache`. Ta funkcja wykonuje zapis ORAZ
   unieważnienie cache. Brudna linia cache może więc nadpisać dane DMA;
   zależy to od wcześniejszego użycia i wyrównania bufora. Źródła NETMAN
   nie przygotowują payloadu przed przekazaniem go IOP. To konkretny
   mechanizm zgodny z objawem, nadal wymagający weryfikacji sprzętowej.
5. Zamiast ryzykownej zmiany cache współdzielonych pbufów NETFIX używa
   oddzielnych, wyrównanych do 64 B buforów DMA (64 x 1536 B = 96 KiB).
   Przed pierwszym użyciem usuwa ich stare linie cache, a później czyta
   je tylko przez alias bez cache. Kopiuje dokładną długość pakietu do
   zwykłego pbufa PRZED ponownym udostępnieniem slotu IOP. Bufory lwIP
   są obsługiwane wyłącznie przez CPU. Zbyt duże ramki są odrzucane.
6. W przebudowanej bibliotece lwIP włączone są kontrole sum IP/TCP;
   CRC PNG i weryfikacja zapisu pozostają obowiązkowe. Ograniczenie
   `recv` do 512 B nie jest używane w NETFIX. Komentarz o ps2ips RPC
   był nietrafny: Manager korzysta z TCP/IP po stronie EE.

Domyślnie SMAP zachowuje DMA. Obejście RX przez PIO jest dopiero następnym
krokiem, jeśli normalny NETFIX nadal odrzuca PNG. Można je zbudować ręcznie
w tym samym workflow, zaznaczając `rx_pio`; wtedy tylko odbiór SMAP
pomija DMA, transmisja TX i dysk pozostają bez zmiany. Nie budujemy
kilku porównawczych ELF-ów automatycznie ani nie zwiększamy liczby ponowień.

## Plik dla konsoli

Workflow **Build isolated HDD cover network repair** tworzy artefakt
`PSX-DVR-HDD-NETFIX-DMA` zawierający:

- `DVR_ISO_MANAGER_HDD_NETFIX.ELF` — osobny Manager z nagłówkiem HDD NETFIX;
- `BUILD_INFO.txt` — commit SDK, tryb RX i skróty bibliotek/modułów;
- `SDK_CHANGES.patch`, mapę linkera i sumę ELF-a.

Wgraj ELF pod jego osobną nazwą obok dotychczasowego Managera na
`PP.DVRISO`. Uruchom go z wLE. Nie podmieniaj istniejącego Managera
ani pliku uruchamianego przez XMB. NETFIX korzysta z istniejącej
konfiguracji, indeksu i `ART/` na tej samej partycji.

Pobieranie zaczyna się zwykłym TRIANGLE i potwierdzeniem X. Zapisuje
wyłącznie brakujące okładki, poprzez `.PART`, kontrolę CRC oraz odczyt
porównawczy. Istniejących okładek nie usuwa. Stare `.PART` są raportowane
i pomijane, tak jak wcześniej. X w przeglądarce nadal uruchamia grę
i zapisuje CNF według dotychczasowej logiki HDD.

Zanotuj liczbę brakujących okładek przed pobieraniem oraz wyniki
`saved`, `BAD_PNG`, `LINK`, `STATUS`, `PFS_WRITE`, `STALE_PART`.
Poprawna kompilacja i test hosta nie potwierdzają jeszcze naprawy na PSX.
Dla pełnego potwierdzenia potrzebne są poprawne CRC wszystkich dostępnych
PNG oraz poprawny zapis/odczyt na PFS; HTTP 404 należy liczyć osobno.

## Sprawdzenie bez konsoli

Na czystym checkout przypiętego SDK:

```sh
PS2SDK_SOURCE=/path/to/ps2sdk python3 -m unittest discover -s hdd-netfix -p 'test_*.py' -v
sh hdd-netfix/check_host.sh
```

Test kompiluje rzeczywiste zmodyfikowane funkcje odbioru NETMAN z modelem
IOP, który nadpisuje slot natychmiast po jego udostępnieniu. Sprawdza
kopiowanie przed rearm, dokładne długości, przejście slotu 63 -> 0
i odrzucenie ramki przekraczającej limit. Nie emuluje sprzętowego cache.

Obraz narzędzi kompilacji nadal ma tag `:main`; wersja SDK jest przypięta,
a wersja kompilatora i skróty wyników są zapisywane. Nie deklarujemy
bitowej odtwarzalności całego środowiska ani tożsamości źródłowej starego
pliku DEV9; jego tożsamość bajtowa względem pliku USB jest sprawdzana.
