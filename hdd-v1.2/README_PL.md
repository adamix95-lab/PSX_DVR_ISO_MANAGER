# PSX DVR ISO MANAGER 1.2 HDD — natywne HTTPS i okładki

**Stan sprzętowy: HARDWARE PASS — Sony PSX DESR-7000 (listopad 2026).** Sprawdzono na konsoli instalator HDD/XMB, Manager, uruchamianie gier oraz pobieranie okładek HTTPS.

**Stan publikacji: OFICJALNE WYDANIE 1.2.** Trzy paczki znajdują się na stronie [GitHub Release 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2). Wcześniejsze artefakty CI pozostają archiwalnymi wersjami testowymi.

[English README](README.md) · [Instalator HDD](../pdim-hdd-v1.2-installer/README_PL.md) · [Instalacja ręczna](INSTALL_MANUAL_PL.md)

## Cel

PDIM 1.2 HDD łączy sprawdzoną wersję **HDD 1.0** (NETFIX, OSDMenu, Neutrino FIX95, SYSTEM/ENGLISH, PFS ART i XMB) z natywnym pobieraniem **HTTPS znanym z USB 1.2 R4**. Pobieranie okładek przez HTTPS nie wymaga serwera na komputerze.

**Docelowa partycja aplikacji i jej plików to wyłącznie `hdd0:PP.DVRISO`.** `PP.DVRISO12` była odizolowaną partycją *testową* i nie jest używana w docelowej instalacji 1.2 HDD.

| Element | Lokalizacja |
| --- | --- |
| Partycja aplikacji / montowanie PFS | `hdd0:PP.DVRISO` / `pfs1:/` |
| Manager | `pfs1:/DVR_ISO_MANAGER_HDD.ELF` |
| Certyfikaty CA HTTPS | `pfs1:/N4E.CA` |
| Konfiguracja sieci HTTPS | `pfs1:/V12_NET.CNF` |
| Okładki | `pfs1:/ART/` |
| Neutrino SYSTEM / ENGLISH | `pfs1:/neutrino_hdd.elf` / `pfs1:/neutrino_hdd_lang.elf` |
| Oryginalne ISO gier DVR | `dvr_pfs0:/ISO` — **tylko odczyt** |

Manager nie przenosi ani nie nadpisuje obrazów ISO. Tworzenie partycji APA może jednak zmienić globalne metadane dysku — przeczytaj sekcję **Bezpieczeństwo**.

## Funkcje PDIM 1.2 HDD

- **Natywne HTTPS:** wolfSSL, weryfikacja certyfikatów i nazwy hosta, podpisany czas Roughtime, zestaw CA, moduł pobierania wbudowany w ELF Managera.
- **Skan ISO i okładek:** rozpoznanie Game ID z gier na DVR, sprawdzanie istniejących okładek i pobieranie wyłącznie brakujących.
- **Źródła grafik:** przypięta wersja bazy OPL Manager oraz zapasowa CosmicScale/PSBBN; kontrola i normalizacja PNG.
- **Maksymalnie 50 okładek w serii**, ekran postępu, powrót do Managera, odświeżenie indeksu na HDD.
- **Bez nadpisywania grafik:** zapis przez `.PART`, weryfikację odczytu, zmianę nazwy i PFS sync. Istniejące PNG/JPG są zachowywane.
- **Zachowane HDD 1.0:** NETFIX, uruchamianie gier przez OSDMenu, Neutrino FIX95, SYSTEM/ENGLISH i podgląd okładek.
- **Instalator Core + XMB:** X kopiuje 79 plików, TRIANGLE pięć plików XMB, a dopiero po ich weryfikacji zapisuje nagłówek PPAA.
- **`V12_NET.CNF`:** instalator automatycznie kopiuje poprawną konfigurację z USB, jeśli nie ma jej na HDD; istniejącego pliku nie nadpisuje.

W PDIM rozpocznij skan okładek przyciskiem TRIANGLE, a na ekranie wyboru wskaż **SERVER (HTTPS, UP TO 50)**. Opcja **PC APP (LAN HTTP)** pozostaje osobna.

## Historia rozwoju i rozwiązane problemy

| Etap | Problem / poprawka | Potwierdzony wynik |
| --- | --- | --- |
| HDD 1.0 NETFIX | Sieć HDD, gry i indeks okładek | Gry startują; pobrano 40 + 4 okładki |
| USB 1.2 R4 | HTTPS/TLS, czas sieciowy, seria 50 i powrót | 45/45 okładek w jednej sesji; start gry |
| HDD 1.2 na `PP.DVRISO12` | Przeniesienie HTTPS na PFS1/ART | Pobrano 45 okładek; Metal Slug uruchomiony w ENGLISH |
| Instalator R1 | Błąd zależności `fileXio -200` | Poprawiony reset IOP i kolejność modułów |
| Instalator R2 | Rejestracja XMB: `-31` | Sektorowa metoda nagłówka PPAA zgodna z wLE-R3Z |
| Instalator R3 | XMB działało, lecz `EXECUTE.*` kopiowano w Core, a `res/` po PPAA | Zachowano stabilny R3, uporządkowano etapy w 1.2 |
| Instalator HDD 1.2 | Zawijanie tekstu, kolor ostrzeżenia, aktualizacja HDD 1.0 i brak `V12_NET.CNF` | Poprawiony UI, PPAA na końcu, ochrona danych, automatyczny plik sieci |

Sprawdzona **wersja HDD 1.0 w repozytorium** i instalator **R3** pozostają nienaruszonymi punktami odniesienia.

## Testy sprzętowe — docelowa `PP.DVRISO`

Użytkownik potwierdził działanie na **Sony PSX DESR-7000**:

| Test | Wynik |
| --- | --- |
| Start instalatora z USB FAT32 przez wLE-R3Z 4.80 | **PASS** |
| Wykrywanie `PP.DVRISO` i inspekcja XMB | **PASS** |
| X: instalacja i weryfikacja 79 plików Core | **PASS** |
| TRIANGLE: pięć plików XMB, 84/84 i PPAA | **PASS** |
| Restart, uruchamianie z XMB, gry, SYSTEM / ENGLISH | **PASS** |
| Pobieranie HTTPS, wyświetlanie okładek, powrót do Managera | **PASS** |
| Ponowne X: `COPIED 0 / KEPT 79 / VERIFIED 79` | **PASS** |
| Ponowne TRIANGLE: pięć `KEEP`, `COPY 0`, `VERIFIED 84`, `ALREADY VERIFIED` | **PASS** |
| SQUARE — kontrola Core tylko do odczytu; SELECT — inspekcja PPAA | **PASS / PASS** |
| Automatyczna obsługa `V12_NET.CNF` | **PASS**, potwierdzone na konsoli |

To **testy funkcjonalne**, a nie gwarancja odporności APA na przerwanie zapisu lub zanik zasilania.

[GitHub Actions — PASS: HDD 1.2, instalator i dwie paczki testowe](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions/runs/44253182195).

## Sposoby instalacji i dostępne paczki

1. **HDD 1.2 AUTO** — ELF instalatora, 79 plików Core, pięć plików XMB, nagłówki PPAA, CA, `V12_NET.CNF` i dokumentacja. [Opis instalatora](../pdim-hdd-v1.2-installer/README_PL.md).
2. **HDD 1.2 MANUAL** — te same pliki do ręcznego kopiowania i rejestracji XMB w wLE-R3Z. [Instrukcja](INSTALL_MANUAL_PL.md).
3. **USB 1.2** — oficjalna kompletna paczka USB z natywnym HTTPS, oparta na testach USB R4. [Pobierz USB 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-USB-COMPLETE-OFFICIAL.zip).

**Wydanie 1.2 zostało opublikowane:** [GitHub Release 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2).

### Konfiguracja sieci

W paczce znajduje się `V12_NET.CNF`:

~~~ini
IP=192.168.0.240
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
DNS=1.1.1.1
~~~

To wartości działające w konkretnej sieci, **nie uniwersalna konfiguracja każdego routera**. Zmień je przed instalacją, gdy Twoja sieć wymaga innych adresów. Plik sieciowy to dane użytkownika: nie jest częścią manifestu 84 plików, a istniejący na HDD zostaje zachowany.

## Bezpieczeństwo i ograniczenia

- **Przed zapisem wykonaj i sprawdź pełną, odtwarzalną kopię sektorową HDD.** Sama kopia APA nie jest jej zamiennikiem.
- Nie usuwaj, nie formatuj ani nie zmieniaj rozmiaru istniejącej `PP.DVRISO`. Gdy partycja nie istnieje, instalator może utworzyć 256 MiB PFS po potwierdzeniach, co zmienia globalne metadane APA.
- Zachowaj `BOOT*.CNF`, sieć, `ART/` i indeksy. Poprzedni Manager ELF jest zabezpieczany przy aktualizacji, ale przerwany zapis może wymagać ręcznej kontroli.
- Entropia oparta o CPU i zintegrowany TLS to rozwiązania przetestowane eksperymentalnie na DESR, **nie niezależnie certyfikowany klient TLS**.
- Projekt jest przeznaczony do PSX DESR; nie gwarantuje zgodności z każdym PS2 i dowolnym układem HDD.
- Podczas dystrybucji obowiązują odpowiednie licencje i informacje o pochodzeniu zależności; same podziękowania nie zastępują obowiązków licencyjnych.

**Wydanie 1.2 jest dostępne:** [trzy paczki do pobrania](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2). [Opis techniczny HTTPS (EN)](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/main/docs/HTTPS_1.2_TECHNICAL_EN.md).
