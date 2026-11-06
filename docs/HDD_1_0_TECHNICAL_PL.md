# PSX DVR ISO Manager 1.0 HDD — architektura, poprawki, wyniki testów

[English technical documentation](HDD_1_0_TECHNICAL_EN.md) · [README po polsku](../README_PL.md) · [kod Managera](../dvr-iso-manager/) · [poprawka Neutrino](../neutrino-hdd-v1/) · [HDD NETFIX](../hdd-netfix/)

> **Stan na 2026-11-06:** kandydat HDD **przetestowany na fizycznym Sony PSX DESR-7000**, ale **nieopublikowany jeszcze jako stabilne wydanie v1.0**. Szczegóły weryfikacji poniżej. Obecna gałąź: `feature/v1.0-hdd`; przetestowany commit: `4ef4aba3a6347b3462a117890f992b13af0d38d3`. [Kompilacja zweryfikowanej paczki](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42170404678).

## 1. Potwierdzone testy na sprzęcie

| Test | Wynik | Zakres potwierdzenia |
| --- | --- | --- |
| Pełna paczka skopiowana na `PP.DVRISO`, uruchomienie Managera HDD | **PASS** | Konsola DESR-7000, pakiet HDD 1.0 |
| Pobieranie brakujących okładek przez LAN do `PP.DVRISO/ART/` | **PASS** | 44 obrazy w dwóch sesjach: **40 + 4** |
| Lista gier i uproszczony komunikat po pobieraniu | **PASS** | Wynik na ekranie listy, bez liczników BAD/HTTP/CRC |
| Uruchomienie dwóch gier z obrazów ISO na DVR HDD | **PASS** | Dwie gry uruchomione w bieżącym teście |
| Wymuszenie ENGLISH | **PASS** | Jedna z tych dwóch gier uruchomiona z wymuszonym angielskim |
| Poprzednie testy DIAG Z / FIX95 (m.in. MGS2, ICO) | **PASS — historyczne** | Wcześniejsza diagnostyka ścieżki startowej |
| Wcześniejszy test Metal Slug po angielsku | **PASS — historyczny** | Potwierdzony w poprzednim teście NETFIX |
| Uruchomienie wszystkich 44 tytułów / zgodność wszystkich gier | **NIE TESTOWANO** | 44/44 dotyczy okładek, nie kompatybilności gier |
| Automatyczny instalator / uruchamianie Managera jako wpis XMB | **NIE POTWIERDZONO** | Nie twierdzimy, że jest ukończony |
| Inne modele DESR, inne dyski i sieci | **NIE POTWIERDZONO** | Wymagają testów społeczności |

**PASS na fizycznej konsoli nie oznacza pełnej certyfikacji.** Zachowaj kopie działającej instalacji i konfiguracji. Wersja USB v0.8 pozostaje osobnym, już publicznie wydanym wariantem.

## 2. Dlaczego powstała wersja HDD?

W działającej wersji USB główne pliki aplikacji są na `mass0:/DESR_DVR/`, a Neutrino czyta tam katalogi `config/` i `modules/` oraz `BOOT.CNF`. Gry są na innym systemie plików DVR: `dvr_pfs0:/ISO/`.

Przeniesienie samego pliku ELF na HDD nie wystarczało. Potrzebne było uruchamianie z osobnej **partycji aplikacji APA/PFS** `hdd0:PP.DVRISO`, bez ingerencji w partycje systemowe Sony ani obrazy gier, a także zachowanie poprawnej konfiguracji po przejściu przez OSDMenu i po zmianach środowiska IOP.

### Rozdzielenie systemów plików

~~~text
Sony PSX DVR
├── dvr_pfs0:/ISO/                obrazy gier PS2 — tylko odczyt przez Manager
└── hdd0:PP.DVRISO                osobna partycja aplikacji (APA/PFS)
    ├── DVR_ISO_MANAGER_HDD.ELF
    ├── BOOT.ELF                   OSDMenu / Quickboot
    ├── BOOT.CNF                   konfiguracja wybranej gry
    ├── neutrino_hdd.elf          język SYSTEM
    ├── neutrino_hdd_lang.elf     wymuszony ENGLISH
    ├── NET_PROBE.CNF
    ├── COVERS.IDX
    ├── COVER_SCAN.LOG
    ├── ART/<GAMEID>_COV.png
    ├── config/*.toml
    └── modules/*.irx + ee_core.elf / ee_core_lang.elf
~~~

**Istotna różnica nazw montowania:** podczas pracy Managera `PP.DVRISO` jest pod `pfs1:/`. Po uruchomieniu OSDMenu, zmodyfikowane Neutrino montuje tę samą partycję ponownie **tylko do odczytu** jako `pfs0:/`. Nie wolno mylić tych nazw z `dvr_pfs0:/ISO/`.

## 3. Czemu oryginalne Neutrino nie startowało poprawnie z HDD?

Nie wykazaliśmy ogólnego błędu Neutrino uniemożliwiającego odczyt każdego obrazu ISO z DVR. **Fork Neutrino z obsługą DVR już zawierał mechanizm DVR FHI.** Problem integracji dotyczył przede wszystkim środowiska, w którym OSDMenu uruchamiało Neutrino z naszej partycji aplikacji.

- Neutrino odczytuje m.in. `config/system.toml`, `config/bsd-dvr.toml` i moduły przez ścieżki względne. Samo załadowanie ELF z HDD nie gwarantuje poprawnego katalogu roboczego ani nadal zamontowanego `pfs0:` po Quickboot.
- Po oddaniu sterowania przez OSDMenu Neutrino musiało samodzielnie zainicjalizować `fileXio`, zamontować `hdd0:PP.DVRISO` i ustawić katalog roboczy, **zanim** upstream rozpocznie czytanie konfiguracji.
- W diagnostyce dochodziło również do przerwania na etapie konfiguracji (`STAGE 60`) i wykryliśmy konieczność użycia **właściwego kompletu** `config/` i `modules/`, zgodnego z DVR FHI. Nie traktujemy wszystkich tych problemów jako usterek oryginalnego Neutrino.

### Nasz wrapper inicjalizacji HDD

Fragment odpowiadający faktycznej logice [`patch_hdd_runtime.py`](../neutrino-hdd-v1/patch_hdd_runtime.py):

~~~c
SifInitRpc(0);
fileXioInit();
fileXioMount("pfs0:", "hdd0:PP.DVRISO", FIO_MT_RDONLY);
chdir("pfs0:/");
rc = neutrino_hdd_upstream_main(argc, argv);
~~~

W pełnym kodzie sprawdzamy wartości zwracane oraz odmontowujemy tylko własny mount, gdy upstream niespodziewanie zwróci sterowanie. **Nie zmienialiśmy oryginalnego mechanizmu `setup_dvd_iso()` ani samego odczytu DVR FHI.**

## 4. DIAG Z FIX95: odrzucanie rzeczywistego ISO

W części *naszych własnych, wcześniejszych testów* używaliśmy fikcyjnego obrazu `DIAG_V_NO_GAME.iso` i specjalnego warunku zapobiegającego startowi prawdziwej gry. Po przejściu do DIAG Z ten diagnostyczny warunek przez pewien czas niepotrzebnie odrzucał realny obraz przesyłany przez OSDMenu. **To był błąd naszego kodu diagnostycznego, nie oryginalnego Neutrino.**

FIX95 usunął porównanie z fikcyjną ścieżką i zastąpił je kontrolą **faktycznej ścieżki wybranej przez użytkownika** po parsowaniu argumentów i konfiguracji TOML:

~~~c
/* argv[2] to "-dvd=dvr_pfs0:/ISO/NAZWA.iso".
   argv[2] + 5 pomija prefiks "-dvd=". */
if (!desr_iso_arg_allowed(argv[2]) ||
    !sDVDFile ||
    strcmp(sDVDFile, argv[2] + 5) != 0)
    return -95;
~~~

Kontrola akceptuje tylko wskazaną lokalizację `dvr_pfs0:/ISO/`, bez dodatkowych katalogów i niebezpiecznych znaków, i porównuje ją z ISO ostatecznie wybranym przez Neutrino. Działa w obu wariantach `SYSTEM`/`ENGLISH`. Pozostawiono oryginalne `setup_dvd_iso()` oraz ścieżkę uruchamiania gry w upstream. DIAG Z FIX95 umożliwił wcześniejsze udane uruchomienia MGS2 i ICO na fizycznym DESR-7000.

**Pochodzenie rozwiązania DVR:** [Neutrino ps2max32](https://github.com/ps2max32/neutrino), [poprawka DVR pcm720 — PR #228](https://github.com/ps2max32/neutrino/pull/228); przypięty upstream commit: `aa8d80a1a0b5323cb407c814c0568c3489597c1c`. **DVR FHI, `dvrfile_fhi.irx` i podstawowa emulacja DVD nie zostały napisane w tym projekcie.**

## 5. OSDMenu, BOOT.CNF i wybór języka

Przy wyborze gry Manager:

1. Odczytuje nazwę pliku z `dvr_pfs0:/ISO/`, zachowując wielkość liter, i generuje odpowiedni `BOOT.CNF` na `PP.DVRISO`.
2. Podnosi mount aplikacji do zapisu tylko na czas wymaganych operacji. Nową konfigurację zapisuje etapowo przez `BOOT.NEXT.CNF`, weryfikuje, zachowuje mechanizm kopii `BOOT_RECOVERY.CNF` / `BOOT.PREV.CNF` i dopiero potem podmienia `BOOT.CNF`.
3. Wywołuje istniejące `BOOT.ELF` (OSDMenu), przekazując jawny argument `hdd0:PP.DVRISO/BOOT.CNF`.
4. OSDMenu przekazuje Neutrino `-bsd=dvr` oraz `-dvd=dvr_pfs0:/ISO/…`. Po ponownym montowaniu przez wrapper Neutrino używa upstream DVR FHI i uruchamia grę.

Uproszczony przykład `BOOT.CNF` dla SYSTEM:

~~~ini
boot=neutrino_hdd.elf
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/MGS 2.ISO
arg=-dev9=NICHDD
#PAD
~~~

Dla ENGLISH Manager wybiera `neutrino_hdd_lang.elf` i dopisuje `arg=-cfg=osdlang` oraz `arg=-osdlang=en` **przed** `arg=-dev9=NICHDD`. Ten wariant używa [naszego istniejącego patcha językowego](../neutrino-v02/neutrino-osdlang-en.patch), `config/osdlang.toml` i pasującego `modules/ee_core_lang.elf`. Nie zmienia języka XMB ani NVRAM konsoli.

**`-dev9=NICHDD` konsumuje OSDMenu**, a `#PAD` jest końcową linią ochronną, nie argumentem Neutrino. Generator: [`hdd_boot_config.c`](../dvr-iso-manager/src/hdd_boot_config.c).

## 6. Dlaczego HDD początkowo uszkadzał okładki pobierane po TCP?

Manager USB pobierał okładki poprawnie, a wariant HDD otrzymywał czasami **poprawny Content-Length, lecz błędne CRC bloku IDAT PNG**. W testach RAM, nawet bez zapisu na PFS, błąd nadal występował. To zawęziło problem do ścieżki odbioru danych, nie do samych oryginalnych okładek.

Istotne różnice między wersjami obejmowały `dev9_ns` z USB vs `ps2dev9` oraz dodatkowe sterowniki HDD/APA/PFS. Nie udowodniliśmy jednak, że sam sterownik DEV9 był jedyną przyczyną. Przywrócenie znanego modułu USB było tylko częścią naprawy.

### HDD NETFIX — najważniejsze zmiany

- Manager HDD ładuje **jednego dostawcę DEV9**: znany `modules/dev9_ns.irx`; nadal ładuje `ps2atad`, `ps2hdd` i `ps2fs` dla HDD, a do Ethernetu `NETMAN` + `SMAP`.
- Zmieniliśmy kod odbioru `NETMAN` w **przypiętej wersji PS2SDK**. Zamiast kierować DMA bezpośrednio do buforów `lwIP pbuf`, używa on osobnych **64-bajtowo wyrównanych** buforów pośrednich: `64 × 1536 bajtów`. Dane są odczytywane przez alias niecache'owany, kopiowane do bufora pbuf **przed** ponownym udostępnieniem slotu odbiorczego IOP.
- Po DMA nie wykonujemy niebezpiecznej dla tej ścieżki sekwencji writeback/invalidate na buforze pbuf. Zmniejsza to ryzyko konfliktu między cache CPU a obszarem zapisanym przez DMA.
- Włączona jest kontrola sum **IPv4 i TCP** w lwIP. Po HTTP Manager nadal sprawdza format obrazu, `Content-Length`, CRC PNG oraz zapis/odczyt w PFS przez tymczasowy plik `.PART`. Istniejących okładek nie nadpisuje.
- Sprawdzony build używa **normalnego SMAP RX DMA**, a nie eksperymentalnego obejścia PIO. Diagnostyczny limit pojedynczego `recv` do 512 B **nie jest częścią NETFIX**.
- Zgodne biblioteki EE i moduły IOP budujemy z przypiętego PS2SDK `d41ea62946e53d9a0c8788bb9eb8c8faf488b559`. Build używa [`build_sdk.sh`](../hdd-netfix/build_sdk.sh) i [`patch_sdk.py`](../hdd-netfix/patch_sdk.py).

**Granica dowodu:** wynik 44/44 okładek potwierdza działanie całego zestawu poprawek w sprawdzonej konfiguracji. Nie jest laboratoryjnym dowodem, że wyłącznie jeden błąd cache był przyczyną wszystkich poprzednich problemów.

## 7. Pobieranie okładek i wyświetlanie wyników

TRÓJKĄT skanuje ISO i wykrywa Game ID (najpierw `SYSTEM.CNF`, a gdy potrzeba — jednoznaczny plik wykonywalny w katalogu głównym obrazu). `COVER_SCAN.LOG` obejmuje również gry bez okładek; `COVERS.IDX` indeksuje odnalezione grafiki. Komputer z [ART Server](../pc-art-server/README.md) pobiera obraz z zewnętrznego źródła, a PSX odbiera PNG po lokalnym HTTP.

Downloader ma limit **40 prób pobrania na jedną sesję** (`ART_SYNC_BATCH_MAX`), dlatego 44 okładki zostały prawidłowo pobrane w dwóch seriach: **40 + 4**. Po każdym pobraniu lista gier jest odświeżana. Komunikat po zakończeniu w menu pokazuje tylko **liczbę zapisanych okładek**; szczegółowa diagnostyka pozostaje dostępna podczas transferu i w osobnych narzędziach.

Przykład interfejsu po pierwszej sesji:

~~~text
ART SYNC: 40 covers downloaded
~~~

Uproszczenie dotyczy wyłącznie [`draw()` w `src/main.c`](../dvr-iso-manager/src/main.c), commit `4ef4aba3`. Nie zmieniło kodu TCP/HTTP, sprawdzania CRC, zapisu ani uruchamiania gier. Właśnie ten build został ponownie potwierdzony na DESR-7000.

## 8. Jak zbudować wariant HDD

Publiczny kod źródłowy jest w gałęzi `feature/v1.0-hdd`. Automatyczne wydania GitHub Actions budują menedżer na zmodyfikowanym PS2SDK i przygotowują osobne binaria Neutrino SYSTEM/ENGLISH.

- [Workflow — kompletny HDD 1.0 ZIP](../.github/workflows/build-hdd-test-bundle-v1.yml)
- [Workflow — kompilacja Managera i NETFIX](../.github/workflows/build-dvr-iso-manager.yml)
- [Workflow — Neutrino HDD](../.github/workflows/build-neutrino-hdd-v1.yml)

Docelowy plik dla użytkownika na partycji to **`DVR_ISO_MANAGER_HDD.ELF`**. Historyczna wewnętrzna nazwa celu CMake `DVR_ISO_MANAGER_HDD_TEST.ELF` pozostaje na potrzeby procesu kompilacji; bundler dostarcza nową nazwę **bez** `HDD_PREVIEW`. Nie jest to samoinstalująca paczka XMB.

Przy aktualizacji działającej instalacji **nie nadpisuj automatycznie** własnych `BOOT.CNF`, `config/`, `modules/`, `ART/` i indeksów. Zrób kopię. Archiwum dla nowej instalacji celowo nie zawiera `BOOT.CNF` — powstaje on dopiero po wyborze gry. Manager nie tworzy, nie formatuje i nie usuwa partycji Sony.

## 9. Podział prac, autorstwo i dalsze możliwości

| Element | Pochodzenie |
| --- | --- |
| Podstawowe Neutrino, jego emulacja DVD | projekt Neutrino i autorzy upstream |
| DVR FHI / obsługa `dvr_pfs0:` w Neutrino | prace pcm720, [PR #228](https://github.com/ps2max32/neutrino/pull/228) |
| OSDMenu / Quickboot | istniejący projekt OSDMenu; integracja z jego interfejsem po stronie tego repo |
| Wrapper PFS dla HDD, FIX95, konfiguracja SYSTEM/ENGLISH | poprawki integracyjne PSX DVR ISO Manager |
| Manager 1.0 HDD, `PP.DVRISO`, bezpieczny zapis CNF/ART, interfejs | PSX DVR ISO Manager |
| Naprawa buforów RX NETMAN, spójny build SDK, testy | [`hdd-netfix/`](../hdd-netfix/) w tym repo |
| Pobieranie okładek z Windows po HTTPS i serwowanie HTTP w LAN | [`pc-art-server/`](../pc-art-server/) |

Przy wykorzystywaniu zmian w innych projektach należy uwzględnić licencje i informacje o autorach oryginalnego Neutrino, OSDMenu, PS2SDK oraz innych zależności. Ten dokument **nie nadaje nowej licencji** cudzym projektom.

**Zadania nadal otwarte:** weryfikacja większej liczby gier i modeli DESR, opcjonalny bezpieczny instalator XMB, końcowy przegląd dokumentacji wydania i zgłoszenia społeczności. Eksperymentalny **NATIVE QUIT** nie został scalony z tym działającym HDD 1.0.

### Najważniejsze odnośniki

- [HDD wrapper i FIX95](../neutrino-hdd-v1/patch_hdd_runtime.py)
- [NETMAN DMA bounce i sumy TCP/IP](../hdd-netfix/patch_sdk.py)
- [Układ plików HDD](../dvr-iso-manager/include/runtime_storage.h)
- [Generator konfiguracji bootowania](../dvr-iso-manager/src/hdd_boot_config.c)
- [HDD NETFIX — historia eksperymentów](../hdd-netfix/README_PL.md)
- [Historia diagnostyki Neutrino — etap STAGES/60](../neutrino-hdd-v1/README_PL.md)
- [Zweryfikowany build w GitHub Actions](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/42170404678)
