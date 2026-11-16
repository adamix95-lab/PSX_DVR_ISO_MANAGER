# N6G. Regresja TLS na Sony PSX DESR-7000

Na fizycznej konsoli N6E zwrócił `RET=0` po pobraniu i sprawdzeniu okładki. N6F zatrzymał się po komunikacie `LOW: TCP CONNECTED, TLS START`. Ten komunikat poprzedza ustawienie dwóch kontekstów I/O i wywołanie `wolfSSL_connect()`. Normalizator i zapis USB są wywoływane dopiero po weryfikacji HIGH, odbiorze PNG i zwolnieniu obiektów TLS. Ostatni komunikat wyklucza wykonanie dekodera lub zapisu jako bezpośrednią przyczynę tego zatrzymania. Sam komunikat nie mówi, czy klient wysłał ClientHello ani czy odebrał odpowiedź.

## Historyczne pliki z GitHub Actions

| ELF | Plik [B] | .text [B] | .data [B] | .rodata [B] | .bss [B] | Koniec obrazu / początek sterty |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| N6E `04eb9e8` | 4 786 996 | 1 694 160 | 250 848 | 123 112 | 966 480 | `0x003e4fd0` |
| N6F `952f8d0` | 5 992 900 | 1 924 840 | 250 848 | 153 044 | 1 315 664 | `0x00479d50` |
| Różnica | +1 205 904 | +230 680 | 0 | +29 932 | +349 184 | +609 664 B |

Źródła ELF: [N6E build 43748155998](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43748155998), [N6F build 43759324615](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/43759324615).

- `readelf -lW` pokazuje pojedynczy segment LOAD z bazą `0x00100000`. Jego rozmiar w RAM wzrósł z `0x2e4fd0` do `0x379d50`. Rozmiar pliku obejmuje m.in. dane debugowania poza tym segmentem. `size` grupuje sekcje inaczej niż tabela pojedynczych sekcji `readelf -SW`.
- `nm -S` przypisuje wzrost do `png_output` 262 144 B, `image_rgb` 84 000 B, `scan_row` 2 048 B i `verify_chunk` 1 024 B. Razem 349 216 B. Przy zmianie ułożenia i wyrównania sekcji różnica netto `.bss` wynosi 349 184 B.
- `ca_bundle` pozostaje 350 001 B, `n6_response` 270 337 B, bufor puli lwIP 196 623 B, a stos obserwatora 8 192 B. Pula lwIP przesunęła się z `0x003a7e88` na `0x0043cc08`, koniec obrazu przekroczył `0x00400000`. Ten adres służy do porównania układu. W przypiętym skrypcie linkera PS2SDK brak granicy sterty przy 4 MiB.
- `_stack_size=0x20000` (128 KiB), `_stack=-1` i `_heap_size=-1` w obu ELF. Start PS2SDK przekazuje `_end` do `SetupHeap`. Newlib zwiększa stertę od `_end` do wartości zwróconej przez `EndOfHeap()`. Przy nominalnych 32 MiB EE i odjęciu tylko głównego stosu górna granica wolnej przestrzeni przed alokacjami to około 28,0 MiB w N6E i 27,4 MiB w N6F. Stan faktycznej sterty, rezerwacje środowiska startowego i maksymalne użycie stosu nie wynikają z ELF.
- Rekordy MIPS `.pdr` w obu ELF dają te same statyczne ramki: `main` 1 008 B, `tls_one_bound` 224 B, `tls_send` i `tls_recv` po 112 B, `wolfSSL_connect` 32 B, `ProcessPeerCerts` 1 392 B i `fp_exptmod` 3 328 B. Ramki wybranych funkcji są tylko częściową informacją o zagnieżdżeniu. Nie wykazały wzrostu stosu na wejściu w TLS.
- N6E i N6F używają tego samego obrazu CI, tego samego przypiętego PS2SDK i NETFIX oraz tej samej wersji wolfSSL z przenośnym FastMath, bez ASM, i z DRBG zasilanym przez callback. Obydwa historyczne pakiety zawierały identyczny `N4E.CA` (200 116 B, SHA-256 `427c064216761a1e72a8b30f0d954d08cb6e4435a82091fb865afd20c8497285`). Skrypt NETFIX i binarny `dev9_ns.irx` nie zmieniły się między rewizjami. Przeniesienie adresów EE wskutek linkowania jest widoczne. Wpływ na zachowanie DMA na konsoli pozostaje do sprawdzenia.

Kod `tls_one_bound` w N6F zachowuje wywołania N6E. Obserwator startuje dopiero przy zapisie HTTP po HIGH. Przegląd ścieżki do handshaku nie wskazał nowego użycia niezainicjalizowanej zmiennej. Obsługa `setjmp`/libpng i jej alokacje mają miejsce po handshake. Same statyczne pomiary nie potwierdzają wyczerpania sterty, uszkodzenia stosu ani awarii DMA.

## Warianty i weryfikacja

- A dodaje do N6E 349 216 B w `.bss`. Jeśli systematycznie zawiesza się z poprawnie działającym N6E jako kontrolą, zmiana samych danych i adresów może wystarczyć do odtworzenia problemu.
- B dołącza oryginalny normalizator, libpng i zlib do N6E. Jeśli A działa, a B się zawiesza, zbadaj różnice kodu i danych dołączonych przez linkowanie. Sam dekoder w B pozostaje niewywołany. Jeśli oba działają, osobna próba z dołączonym kodem USB ART może rozdzielić pozostałe różnice N6F.
- Finalny N6G przydziela 86 048 B roboczych po zwolnieniu TLS. Po zakończeniu odczytu PNG normalizator używa bufora odpowiedzi HTTP jako wyjścia. Wynik 140×200 RGB przechodzi walidację PNG/CRC, a następnie porównanie z odczytem `.PART`.
- Pinned PS2SDK mapował `O_CREAT` na otwarcie lub utworzenie i ignorował `O_EXCL`. Finalny N6G dołącza osobną kompilację samego modułu FAT z mapowaniem `O_CREAT|O_EXCL` na `FA_CREATE_NEW`. FatFs odmawia też przemianowania, gdy ścieżka docelowa istnieje. Wspólne moduły sieciowe NETFIX i jedyny DEV9 zostają te same.

GitHub Actions mierzy ELFy historyczne i warianty N6G tym samym toolchainem. Do paczki trafiają mapy, dane `readelf`, wyniki `size` i `nm` oraz statyczne ramki stosu. Aktualne liczby dla N6G są w `evidence/EE-memory.md`. [Instrukcja testu sprzętowego](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/test/n6g-codex-png-ee-fix/docs/n6g-desr7000-test.md).

## Pierwsza próba N6G na DESR-7000

A zakończył próbę z `RET=0`. B i FINAL doszły do `LOW: VERIFIED PEER/HOST PASS`, po czym konsola się zatrzymała. W tych dwóch próbach LOW zdążył potwierdzić certyfikat, nazwę hosta i wykorzystanie zalążka DRBG. Kod po tym komunikacie zwalnia obiekt SSL, zamyka gniazdo TCP, zwalnia kontekst SSL, czyści zalążek i usuwa callback zegara. Dopiero po powrocie z tej ścieżki pojawia się `STAGE 9`.

Sama dodatkowa `.bss` z wariantu A nie odtworzyła problemu. B nie wywołuje normalizatora i nie zawiera kodu zapisu, więc zatrzymanie w B może wystąpić przed tymi operacjami i bez ich udziału. TRACE powtarza B z komunikatami przed kolejnymi operacjami zamykania LOW. Zmiana układu ELF i czasu działania w wariancie z komunikatami jest częścią warunków tej próby. Jeżeli TRACE przejdzie, powtórz B i TRACE na tym samym zestawie danych wejściowych, ze świeżymi plikami entropii.

Próba TRACE na fizycznym DESR-7000 zatrzymała się na `LOW CLEAN 1: wolfSSL_free`. Brak `LOW CLEAN 2: lwip_close` wskazuje, że wywołanie zwalniające SSL nie wróciło, zanim program dotarł do zamykania TCP. wolfSSL zwalnia kolejno zasoby sesji, pamięć obiektu SSL oraz referencję kontekstu TLS. FREETRACE znakuje te operacje wewnątrz przypiętego wolfSSL. GitHub Actions sprawdza zgodność konfiguracji kompilacji biblioteki i dołącza diagnostyczną kopię wyłącznie do FREETRACE. Komunikaty wewnętrzne są aktywne tylko podczas zwalniania poprawnie zweryfikowanej sesji LOW. Wynik pozwoli ustalić, którą operację zbadać dalej. [Kod FreeSSL w przypiętym wolfSSL](https://github.com/wolfSSL/wolfssl/blob/decea12e223869c8f8f3ab5a53dc90b69f436eb2/src/internal.c#L9113).

FREETRACE z rewizji `5646267` zatrzymał się na `FREE 07 FreeHandshakeHashes`. W ELF z tej próby funkcja odczytuje `ssl->hsHashes`, a przy niepustym wskaźniku zeruje trzy stany SHA (256, 384, 512), zwalnia blok przez `wolfSSL_Free` i ustawia pole na NULL. Nowe znaczniki rozdzielają zerowanie stanów, zwolnienie bloku i powrót z funkcji. Podpis `LOW HASH prior` podaje, czy funkcja wcześniej weszła i wróciła wewnątrz `wolfSSL_connect()` dla tej sesji. Liczniki są zbierane w RAM, a druk następuje po udanej weryfikacji LOW. [Kod FreeHandshakeHashes](https://github.com/wolfSSL/wolfssl/blob/decea12e223869c8f8f3ab5a53dc90b69f436eb2/src/internal.c#L7290).

FREETRACE `5fd9384` zatrzymał się wcześniej, na `FREE 04 FreeKeyExchange`. `LOW HASH prior: in=1 nonnull=1 out=1` potwierdził zakończenie wcześniejszego sprzątania niepustego bloku wewnątrz `wolfSSL_connect()`. Zatrzymanie przy różnych wywołaniach po zmianie diagnostyki nie pozwala przypisać awarii do jednego destruktora. `scr_printf` w przypiętym PS2SDK czeka na zakończenie GIF DMA przy rysowaniu znaków, także przy czyszczeniu linii po wypisanym komunikacie. Ostatnia widoczna linia może więc pozostać na ekranie, choć samo jej wypisywanie jeszcze się nie skończyło. [Kod ekranu debug PS2SDK](https://github.com/ps2dev/ps2sdk/blob/d41ea62946e53d9a0c8788bb9eb8c8faf488b559/ee/debug/src/scr_printf.c).

Wariant SCREEN porównuje dwa tryby tego samego ELF. QUIET omija druk i czyszczenie ekranu od startu sieci do podsumowania. Wygasza też obserwatora zapisu HTTP, który korzysta z GIF DMA. VERBOSE zachowuje dotychczasowe logi. Oba tryby wykonują taki sam TLS LOW/HIGH, normalizację i zweryfikowany zapis okładki. Wynik QUIET i próba kontrolna VERBOSE na tym samym ID pozwolą sprawdzić hipotezę o wpływie wyświetlania statusu na zawieszenie.

W próbie SCREEN z pustym USB `ART` nie pojawił się napis `SCREEN TEST: QUIET` ani nowa okładka. Ostatni widoczny komunikat dotyczył prawdopodobnie trybu tylko do odczytu DVR. Napis QUIET jest po odczycie dysku i wyborze ID, więc ta próba nie potwierdza, że program dotarł do wyciszenia. HEADLESS odracza również inicjalizację ekranu do podsumowania, tak aby preflight, obie sesje TLS i zapis nie korzystały z GIF DMA ekranu debugowego.
