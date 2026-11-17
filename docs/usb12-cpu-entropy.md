> **Historical R4 instructions.** The original hardware-tested R4 read `N4E.CA`
> from the USB root. The newer USB 1.2 CA-layout test **requires**
> `mass0:/DESR_DVR/N4E.CA` instead; see
> [the current CA-layout/English UI test instructions](usb12-ui-en-retry-test.md).
> Do not move the CA file when testing the old R4 binary.

# PSX DVR ISO Manager 1.2 USB — eksperymentalny R4

**Wynik sprzętowy z 17 listopada 2026 r. na Sony PSX DESR-7000.**
R4 zapisał **45/45** brakujących okładek w jednej serii bez pliku
`N6G50.ENT`, wyświetlił podsumowanie i wrócił do Managera. Następnie
z listy uruchomiono Killzone. Limit **50** potwierdził test skanera
z biblioteką 60 symulowanych gier.

- Źródła przetestowanego ELF-a: `ec6bc20ccbf2c082cf3ce111b728897d59e4698e`.
- SHA-256 `DVR_ISO_MANAGER_V12_CPU.ELF`: `81a0f045e2597719d58d798499378afcdce7466a6c7ca967546c1499e2355741`.
- [Kompilacja użyta w próbie sprzętowej](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44054143419).

## Instalacja na USB

1. Na nowy pendrive FAT32 skopiuj folder `DESR_DVR/` i plik `N4E.CA`
   z paczki do katalogu głównego. Przy aktualizacji istniejącej instalacji
   USB skopiuj tylko `DESR_DVR/DVR_ISO_MANAGER_V12_CPU.ELF`. Jeżeli brakuje
   `N4E.CA`, skopiuj go do katalogu głównego. Zachowaj własne okładki,
   indeksy i konfigurację.
2. Podłącz konsolę przez Ethernet do sieci z dostępem do internetu. HTTPS
   czyta `V12_NET.CNF` z istniejącej partycji aplikacji
   `hdd0:PP.DVRISO12`. Jeśli plik z prób BATCH50 działa, zostaw go.
   W przeciwnym razie uzupełnij `IP`, `NETMASK`, `GATEWAY` i `DNS`
   w dołączonym `V12_NET.CNF.example`, zapisz jako `V12_NET.CNF` i
   skopiuj na tę partycję. Adresy we wzorze służą do dokumentacji.
3. Przez wLaunchELF-R3Z uruchom
   `mass0:/DESR_DVR/DVR_ISO_MANAGER_V12_CPU.ELF`. Nagłówek to
   `1.2 USB CPU ENTROPY TEST R4`. Oryginalny `DVR_ISO_MANAGER.ELF`
   nadal uruchamia USB 1.0.

## Pobieranie okładek

- **Trójkąt na liście** skanuje bibliotekę. Jeśli brakuje okładek,
  Manager pokazuje pytanie o pobieranie.
- W pytaniu **Trójkąt** pobiera do 50 brakujących okładek bezpośrednio
  przez HTTPS. **X** używa dotychczasowej metody LAN HTTP i programu
  `tools/PSX-DVR-ART-Server-v0.8.exe` na Windows z konfiguracją
  `DESR_DVR/NET_PROBE.CNF`. **Kółko** wraca do listy.
- Po HTTPS podsumowanie pokazuje m.in. `SAVED`, `RET` i ostatnią grę.
  **X** lub **Kółko** wraca do listy. Przy przerwaniu serii, np. po
  `RET=-508`, ponów wyszukiwanie Trójkątem. Skan pominie już zapisane
  okładki. `RET=-735` oznacza brak poprawnych próbek licznika lub
  argumentu przekazywanego z menu.

## HTTPS, pamięć EE i zapis

Pobieracz BATCH50 jest dołączony do jednego ELF-a. Manager czyta go z
USB w blokach 4 KiB. Po pobraniu loader ponownie wczytuje segment menu.
Pobieracz używa dedykowanej puli pamięci EE i kolejności SSL FIRST.
Sprawdza podpisaną odpowiedź Roughtime oraz certyfikat i nazwę hosta
dla granic LOW/HIGH podpisanego przedziału przed zapytaniem HTTPS.

PNG przechodzi sprawdzenie CRC i normalizację do 140 × 200 RGB.
Pobieracz tworzy na USB plik `.PART` z wyłącznością, porównuje ponowny
odczyt z danymi w RAM i dopiero wtedy przemianowuje go na PNG w
`DESR_DVR/ART/`. Istniejące PNG, JPG i PART są chronione przed
nadpisaniem. Katalog DVR ISO i partycja konfiguracji HTTPS są
otwierane tylko do odczytu.

Licznik CPU jest eksperymentalnym źródłem i może dawać przewidywalne
wyniki. BLAKE2b rozdziela wartości dla Roughtime i TLS. Ten wariant
służy do pobierania publicznych okładek. Inspiracja licznikiem COP0
Count pochodzi z projektu NULD Daniela. [Kod sieciowy NULD](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c)

## Zawartość i weryfikacja ZIP-a

Paczka zawiera R4 z wbudowanym pobieraczem, niezmienione pliki
oficjalnej instalacji USB 1.0, ART Server v0.8 dla Windows, wzór
`V12_NET.CNF`, publiczny `N4E.CA`, licencje i sumy SHA-256.
`SOURCE_COMMIT` wskazuje commit paczki, a kompilacja sprawdza
identyczność ELF-a z wersją używaną na konsoli. Plik `SHA256SUMS`
można zweryfikować poleceniem `sha256sum -c SHA256SUMS` po rozpakowaniu.

Plik `N4E.CA` to publiczna paczka certyfikatów Mozilla z 5 listopada
2026 r., w formacie PEM udostępnionym przez curl na MPL-2.0.
[Archiwum CA curl](https://curl.se/docs/caextract.html) · [Wydanie USB 1.0 i ART Server](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.0)
