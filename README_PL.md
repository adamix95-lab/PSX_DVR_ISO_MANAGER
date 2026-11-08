<img width="1774" height="887" alt="Metaliczny baner PDIM w pomarańczowych tonach" src="https://github.com/user-attachments/assets/9ffed54c-e8ea-46e2-9af0-65e351b40cb3" />


# PSX DVR ISO MANAGER

[English](README.md)

**PSX DVR ISO Manager** służy do przeglądania i uruchamiania gier PS2 w formacie ISO z partycji DVR konsoli Sony PSX DESR. 

**Główną wersją rozwojową jest HDD; USB pozostaje obsługiwane.** Oba warianty przeglądają i uruchamiają gry PS2 ISO zapisane na partycji DVR konsoli Sony PSX. Wariant HDD został sprawdzony na konsoli, łącznie z uruchamianiem aplikacji z XMB.

| Wariant | Wynik na Sony PSX DESR-7000 | Instalacja |
| --- | --- | --- |
| **1.0 HDD** | **PASS:** 44/44 okładki (40 + 4), dwie uruchomione gry, w tym wymuszony angielski; działa start Managera z XMB i nagłówek `1.0 HDD` | Istniejąca partycja APA/PFS `hdd0:PP.DVRISO`, `DVR_ISO_MANAGER_HDD.ELF` |
| **1.0 USB** | **PASS — przetestowany na Sony PSX DESR-7000 (7.11.2026):** USB 1.0 działa na konsoli. Wcześniejsze testy USB v0.8 oddzielnie potwierdziły 40/40 okładek i REZ po angielsku | Pendrive FAT32 `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` |

**Dokumentacja:** [Opis wydania PL](docs/RELEASE_NOTES_v1.0.0_PL.md) · [English](docs/RELEASE_NOTES_v1.0.0_EN.md) · [Techniczny opis HDD PL](docs/HDD_1_0_TECHNICAL_PL.md) · [English](docs/HDD_1_0_TECHNICAL_EN.md) · [Lista czynności do publikacji](release-tools/v1_0/RELEASE_CHECKLIST.md).

**Bezpieczeństwo:** Manager nie formatuje dysku ani nie zapisuje obrazów ISO. Opcjonalna iniekcja **nagłówka APA dla XMB** modyfikuje metadane partycji — wcześniej wykonaj kopię zapasową, którą w razie potrzeby można przywrócić.

---

## Możliwości programu

- **Limit określony w kodzie:** przeglądanie do **128 plików ISO** (`DESR_GAME_CAP = 128`) z katalogu `dvr_pfs0:/ISO` w trybie tylko do odczytu — po 12 tytułów na stronie, bez zawijania nawigacji między pierwszą a ostatnią stroną. **Na prawdziwym sprzęcie przetestowano dotąd tylko 44 gry w formacie ISO.**
- Wykrywanie **Game ID** (np. `SLES_538.86`) na podstawie danych wewnątrz obrazu ISO i wyszukiwanie okładek w `mass0:/DESR_DVR/ART`.
- Podgląd kolorowych okładek PNG w prawym panelu i zapis indeksu `COVERS.IDX` na USB.
- Ręczne skanowanie TRÓJKĄTEM, raport brakujących grafik `COVER_SCAN.LOG` i pytanie o pobranie ich z lokalnego serwera HTTP.
- Po pobraniu nowych okładek automatyczna odbudowa indeksu i powrót do listy gier.
- Uruchamianie gry przez OSDMenu / Neutrino oraz wybór `SYSTEM` albo `ENGLISH`.

## Sterowanie padem

| Przycisk | Działanie |
| --- | --- |
| **GÓRA / DÓŁ** | Wybór poprzedniej / następnej gry |
| **LEWO / PRAWO** | Przełączanie stron po 12 gier |
| **X** | Uruchomienie wybranej gry ze zwykłego menu |
| **TRÓJKĄT (△)** | Ponowny skan ISO, odczyt Game ID i odświeżenie indeksu okładek |
| **X / KÓŁKO (O)** na ekranie pytania | Pobierz brakujące okładki / pomiń pobieranie |
| **KWADRAT (□)** | Zapis testowej konfiguracji `DVR_TEST.CNF` |
| **SELECT** | Przełączenie języka `SYSTEM` / `ENGLISH` |
| **START** | Wyjście z Managera |

# PSX DVR ISO MANAGER 1.0 HDD — uruchamianie i użytkowanie

Wersja **1.0 HDD** umożliwia uruchamianie gier PlayStation 2 zapisanych na partycji DVR konsoli Sony PSX DESR bez konieczności korzystania z pendrive'a podczas pracy Managera.

Aplikacja, jej konfiguracja, moduły Neutrino i okładki gier przechowywane są na osobnej partycji dysku twardego o nazwie **`PP.DVRISO`**.

Wariant został pomyślnie przetestowany na konsoli **Sony PSX DESR-7000**.

## 1. Wymagania

Do uruchomienia potrzebujesz:

- Konsoli Sony PSX DESR z działającym dyskiem twardym.
- Programu **wLaunchELF-R3Z** obsługującego partycje DVR i PFS.
- Istniejącej partycji aplikacji `hdd0:PP.DVRISO`.
- Kompletnej paczki **PSX DVR ISO MANAGER 1.0 HDD**.
- Obrazów gier PS2 w katalogu `dvr_pfs0:/ISO/`.

Pendrive lub UDPFS jest potrzebny do skopiowania plików na dysk i uruchomienia wLaunchELF, ale po przygotowaniu partycji **Manager nie korzysta z USB do przechowywania swoich danych**.

> **Ważne:** Partycja `PP.DVRISO` musi zostać wcześniej utworzona i prawidłowo sformatowana jako PFS. Manager nie tworzy jej automatycznie i nie formatuje żadnych partycji systemowych Sony.

---

## 2. Struktura plików na HDD

Po rozpakowaniu paczki odszukaj folder **`PP.DVRISO-FILES`**.

**Skopiuj jego zawartość bezpośrednio do katalogu głównego partycji `PP.DVRISO`.**

Docelowa struktura wygląda następująco:

```text
hdd0:PP.DVRISO/
│
├── DVR_ISO_MANAGER_HDD.ELF     # Manager 1.0 HDD
│
├── BOOT.ELF                    # OSDMenu Launcher
├── BOOT.CNF                    # Tworzony automatycznie
│
├── neutrino_hdd.elf           # Neutrino - SYSTEM
├── neutrino_hdd_lang.elf      # Neutrino - ENGLISH
│
├── NET_PROBE.CNF              # Konfiguracja sieci LAN
│
├── ART/                       # Okładki gier
│   ├── SLES_XXXXX_COV.png
│   └── SLUS_XXXXX_COV.png
│
├── COVERS.IDX                 # Generowany automatycznie
├── COVER_SCAN.LOG             # Generowany podczas skanowania
│
├── config/                    # Konfiguracja Neutrino
│   ├── system.toml
│   ├── bsd-dvr.toml
│   ├── osdlang.toml
│   └── ...                    # Pozostałe pliki TOML
│
└── modules/                   # Moduły Neutrino / DVR
    ├── ee_core.elf
    ├── ee_core_lang.elf
    ├── dvrfile_fhi.irx
    ├── dvrdrv.irx
    ├── dvrfile.irx
    └── ...                    # Pozostałe moduły z paczki
```

*Struktura jest uproszczona. Należy zachować wszystkie pliki i podfoldery `config/` oraz `modules/` dostarczone w kompletnej paczce.*

### Najważniejsze pliki

| Plik lub folder | Przeznaczenie |
|---|---|
| `DVR_ISO_MANAGER_HDD.ELF` | Główna aplikacja Managera |
| `BOOT.ELF` | Launcher OSDMenu odpowiedzialny za uruchamianie Neutrino |
| `BOOT.CNF` | Konfiguracja wybranej gry, tworzona i aktualizowana automatycznie |
| `neutrino_hdd.elf` | Uruchamianie gier z językiem SYSTEM |
| `neutrino_hdd_lang.elf` | Uruchamianie z wymuszonym językiem ENGLISH |
| `NET_PROBE.CNF` | Adresy IP i port serwera okładek |
| `ART/` | Folder z pobranymi grafikami PNG |
| `COVERS.IDX` | Indeks dostępnych okładek |
| `COVER_SCAN.LOG` | Raport skanowania obrazów ISO |
| `config/` | Pliki konfiguracyjne TOML Neutrino |
| `modules/` | Moduły potrzebne do działania Neutrino i DVR |

> **Uwaga:** W nowej paczce nie ma gotowego `BOOT.CNF`. Jest on generowany po pierwszym wybraniu gry przyciskiem **X**. Nie trzeba tworzyć go ręcznie.

**Nie kopiuj całego folderu `PP.DVRISO-FILES` jako podfolderu na HDD** — pliki muszą znajdować się bezpośrednio w katalogu głównym partycji.

---

## 3. Gdzie przechowywać obrazy ISO?

Obrazy gier **nie znajdują się na `PP.DVRISO`**.

Manager korzysta z oryginalnego systemu plików DVR konsoli Sony PSX:

```text
dvr_pfs0:/ISO/
│
├── ICO.iso
├── MGS 2.ISO
├── REZ.iso
├── Metal Slug Anthology.iso
└── ...
```

Partycja `PP.DVRISO` służy wyłącznie do przechowywania aplikacji, konfiguracji i okładek.

**Manager odczytuje obrazy ISO z partycji DVR bez modyfikowania ich zawartości.**

---

## 4. Kopiowanie plików na dysk

1. Rozpakuj kompletną paczkę **HDD 1.0** na komputerze.
2. Skopiuj folder `PP.DVRISO-FILES` na pendrive FAT32 lub udostępnij pliki przez UDPFS.
3. Uruchom **wLaunchELF-R3Z** na konsoli.
4. Otwórz istniejącą partycję `PP.DVRISO`, korzystając z obsługi HDD/PFS w wLaunchELF.
5. Skopiuj **całą zawartość** `PP.DVRISO-FILES` do katalogu głównego tej partycji.
6. Sprawdź, czy plik `DVR_ISO_MANAGER_HDD.ELF` oraz foldery `config/`, `modules/` i `ART/` znajdują się we właściwych lokalizacjach.

> **Aktualizacja istniejącej instalacji:** Przed podmianą plików wykonaj kopię zapasową. Zachowaj swoje okładki w `ART/`, konfigurację sieciową `NET_PROBE.CNF`, plik `BOOT.CNF` oraz istniejące indeksy.

---

## 5. Uruchamianie Managera z HDD

Po skopiowaniu plików:

1. Uruchom **wLaunchELF-R3Z**.
2. Przejdź do menedżera plików HDD/PFS.
3. Otwórz lub zamontuj partycję **`PP.DVRISO`**.
4. W katalogu głównym odnajdź plik:

   **`DVR_ISO_MANAGER_HDD.ELF`**

5. Uruchom ten plik.

Jeśli partycja została zamontowana jako `pfs0:`, plik będzie widoczny pod ścieżką:

```text
pfs0:/DVR_ISO_MANAGER_HDD.ELF
```

Po prawidłowym uruchomieniu pojawi się interfejs:

**PSX DVR ISO MANAGER — 1.0 HDD** Manager wyświetli listę obrazów ISO znajdujących się w katalogu DVR.


---
<details>
<summary><strong> Instalacja PDIM jako aplikacji uruchamianej bezpośrednio z XMB — kliknij, aby rozwinąć</strong></summary>

# Instalacja PSX DVR ISO MANAGER 1.0 HDD w XMB

PSX DVR ISO MANAGER 1.0 HDD może zostać zainstalowany jako aplikacja w menu **XMB konsoli Sony PSX DESR**.

Po zakończeniu instalacji Manager będzie widoczny w sekcji gier pod nazwą **PSX DVR ISO MANAGER**, z własną ikoną PDIM przedstawiającą dysk HDD i płytę DVD.

Instalacja została przetestowana na **Sony PSX DESR-7000** z wykorzystaniem **wLaunchELF-R3Z 4.80**.

## 1. Wymagania

Do przeprowadzenia instalacji potrzebujesz:

- Konsoli Sony PSX DESR z działającym dyskiem HDD.
- Programu **wLaunchELF-R3Z 4.80**.
- Pendrive'a FAT32.
- Kompletnej paczki **PSX DVR ISO MANAGER 1.0 HDD**.
- Paczki **XMB-PDIM-INSTALL** zawierającej nagłówek APA, ikonę i pliki startowe.
- Wolnego miejsca na dysku HDD.

**Ważne:** Przed rozpoczęciem wykonaj kopię zapasową danych. Tworzenie partycji oraz wstrzykiwanie nagłówków APA modyfikuje strukturę dysku.

Nie formatuj całego dysku konsoli i nie usuwaj oryginalnych partycji systemowych Sony.

---

## 2. Przygotowanie pendrive'a

Rozpakuj paczki HDD i XMB na komputerze.

Na pendrivie przygotuj następującą strukturę:

```text
USB:/
│
├── __Headers/
│   └── PP.DVRISO/
│       ├── system.cnf
│       ├── icon.sys
│       ├── info.sys
│       ├── jkt_001.png
│       └── jkt_002.png
│
├── PFS_COPY_TO_PP.DVRISO/
│   ├── EXECUTE.ELF
│   └── EXECUTE.CNF
│
└── PP.DVRISO-FILES/
    ├── DVR_ISO_MANAGER_HDD.ELF
    ├── BOOT.ELF
    ├── neutrino_hdd.elf
    ├── neutrino_hdd_lang.elf
    ├── NET_PROBE.CNF
    ├── ART/
    ├── config/
    └── modules/
```

Przedstawiona struktura jest uproszczona. Zachowaj wszystkie pozostałe pliki dostarczone w kompletnej paczce HDD.

**Uwaga:** Folder `__Headers` musi znajdować się bezpośrednio w katalogu głównym pendrive'a. wLaunchELF-R3Z odczytuje z niego dane podczas wstrzykiwania nagłówka APA.

### Pliki instalacyjne XMB

| Plik | Przeznaczenie |
|---|---|
| `system.cnf` | Konfiguracja uruchamiania z XMB zapisywana w nagłówku APA |
| `icon.sys` | Metadane ikony |
| `info.sys` | Nazwa i informacje o aplikacji |
| `jkt_001.png` | Grafika 256 × 256 dla PSBBN |
| `jkt_002.png` | Ikona 76 × 108 dla PSX XMB |
| `EXECUTE.ELF` | Kontener KELF odpowiedzialny za uruchomienie launchera |
| `EXECUTE.CNF` | Konfiguracja przekierowująca uruchamianie do PDIM |

---

## 3. Utworzenie partycji PP.DVRISO

Uruchom **wLaunchELF-R3Z 4.80** na konsoli.

1. Przejdź do **MISC → HDDManager**.
2. Otwórz listę partycji dysku `hdd0:`.
3. Sprawdź, czy partycja `PP.DVRISO` już istnieje.
4. Jeżeli jej nie ma, otwórz menu operacji i wybierz **Create partition**.
5. Wprowadź nazwę partycji:

   `PP.DVRISO`

6. Wybierz przykładową pojemność:

   **256 MB**

7. Zatwierdź utworzenie partycji.
8. Sprawdź, czy pojawiła się na liście pod dokładną nazwą `PP.DVRISO` i ma typ **PFS**.

Przykładowy wpis:

```text
hdd0:
└── PP.DVRISO        PFS        256 MB
```

**Dlaczego 256 MB?**

Taka pojemność wystarcza na Managera, Neutrino, konfiguracje i wiele okładek PNG, pozostawiając dodatkowy zapas miejsca.

Można użyć większej partycji, jeśli planujesz przechowywać dużo grafik.

> **Uwaga:** Utwórz partycję o nazwie dokładnie `PP.DVRISO`. Nie używaj `PP.TESTDVR` ani `+DVRISO`. Nasze binaria HDD 1.0 mają skonfigurowaną nazwę `PP.DVRISO`.
>
> Jeżeli partycja już istnieje i zawiera działający Manager, nie twórz jej ponownie i nie formatuj. Jeśli nowej partycji nie można zamontować jako PFS, najpierw sprawdź jej typ i stan — nie używaj opcji formatowania całego HDD.

---

## 4. Wstrzyknięcie nagłówka APA — Inject Header

Po utworzeniu partycji możemy przygotować ją do wyświetlania w XMB.

1. Pozostań w **HDDManager** programu wLaunchELF-R3Z 4.80.
2. Zaznacz utworzoną partycję:

   `PP.DVRISO`

3. Otwórz menu operacji **R1**.
4. Wybierz **Inject Header**.
5. Jako źródło wskaż pendrive USB.
6. Wybierz tryb:

   **This partition only**

7. Potwierdź wstrzyknięcie nagłówka.

Program odczyta pliki znajdujące się w:

```text
USB:/__Headers/PP.DVRISO/
```

Dane `system.cnf` i metadane zostaną zapisane w nagłówku APA partycji. Pliki `info.sys` oraz grafiki XMB zostaną umieszczone przez mechanizm iniekcji w katalogu `res/` na PFS.

**Ważne:** Nie wybieraj opcji `Matching partitions` ani `Matching and create missing partitions`. Chcemy zmodyfikować wyłącznie `PP.DVRISO`.

Po prawidłowym wstrzyknięciu nagłówka przechodzimy do kopiowania plików aplikacji.

---

## 5. Kopiowanie plików Managera na HDD

Wróć do menedżera plików **FileBrowser** wLaunchELF-R3Z.

Otwórz hdd0, następnie otwórz utworzoną partycję `PP.DVRISO`.

### Krok 1 — pliki uruchamiania XMB

Z folderu:

```text
USB:/PFS_COPY_TO_PP.DVRISO/
```

Skopiuj do katalogu głównego partycji `PP.DVRISO`:

```text
EXECUTE.ELF
EXECUTE.CNF
```

Te dwa pliki odpowiadają za uruchamianie PDIM z XMB.

### Krok 2 — pliki Managera HDD

Z folderu:

```text
USB:/PP.DVRISO-FILES/
```

Skopiuj **całą zawartość folderu** do katalogu głównego partycji `PP.DVRISO`.

Nie kopiuj folderu `PP.DVRISO-FILES` jako dodatkowego podfolderu.

### Docelowa struktura partycji

Po zakończeniu kopiowania struktura powinna wyglądać następująco:

```text
hdd0:PP.DVRISO/
│
├── EXECUTE.ELF                # Uruchamianie z XMB (KELF)
├── EXECUTE.CNF                # Konfiguracja launchera XMB
│
├── DVR_ISO_MANAGER_HDD.ELF    # PDIM 1.0 HDD
├── BOOT.ELF                   # OSDMenu Launcher
├── BOOT.CNF                   # Tworzony po wyborze gry
│
├── neutrino_hdd.elf          # Neutrino SYSTEM
├── neutrino_hdd_lang.elf     # Neutrino ENGLISH
│
├── NET_PROBE.CNF             # Ustawienia sieci LAN
│
├── ART/                      # Okładki gier
│
├── config/                   # Konfiguracja Neutrino
│   ├── system.toml
│   ├── bsd-dvr.toml
│   ├── osdlang.toml
│   └── ...
│
├── modules/                  # Moduły Neutrino / DVR
│   ├── ee_core.elf
│   ├── ee_core_lang.elf
│   ├── dvrfile_fhi.irx
│   ├── dvrdrv.irx
│   ├── dvrfile.irx
│   └── ...
│
└── res/                      # Zasoby XMB
    ├── info.sys
    ├── jkt_001.png
    └── jkt_002.png
```

*Struktura jest uproszczona. Nie usuwaj pozostałych plików w `config/` i `modules/`.*

**Uwaga:** `BOOT.CNF` nie musi istnieć bezpośrednio po instalacji — Manager utworzy go automatycznie po wybraniu pierwszej gry.

Jeśli po iniekcji w katalogu `res/` nie ma grafik lub pliku `info.sys`, sprawdź wynik operacji Inject Header, zanim przejdziesz dalej.

---

## 6. Uruchomienie PSX DVR ISO MANAGER z XMB

Po zakończeniu kopiowania:

1. Zamknij wLaunchELF-R3Z.
2. Wyłącz konsolę Sony PSX DESR.
3. Uruchom ponownie konsolę do oryginalnego menu XMB.
4. Przejdź do sekcji gier.

Powinien pojawić się nowy wpis:

**PSX DVR ISO MANAGER**

W XMB wyświetlana będzie ikona **PDIM** z motywem dysku HDD i płyty DVD.
<img width="4096" height="3072" alt="IMG_20261107_231102244_HDR" src="https://github.com/user-attachments/assets/9792cf70-ee0a-46c1-985b-9811784e0549" />
Wybierz aplikację i uruchom ją.

Po prawidłowym starcie zobaczysz:
<img width="4096" height="3072" alt="IMG_20261107_231155465_HDR" src="https://github.com/user-attachments/assets/6b447da9-68c9-43ab-8ea9-3798e632f83b" />

**PSX DVR ISO MANAGER — 1.0 HDD**

Manager odczyta listę gier z:

```text
dvr_pfs0:/ISO/
```

Następnie możesz zaznaczyć grę i uruchomić ją przyciskiem **X**.

---

## 7. Jak działa uruchamianie z XMB?

PDIM wykorzystuje następujący mechanizm:

```text
Sony PSX XMB
    │
    ▼
PP.DVRISO (nagłówek APA)
    │
    ▼
EXECUTE.ELF (KELF)
    │
    ▼
EXECUTE.CNF
    │
    ▼
DVR_ISO_MANAGER_HDD.ELF
    │
    ▼
PSX DVR ISO MANAGER 1.0 HDD
    │
    ▼
BOOT.CNF + BOOT.ELF
    │
    ▼
Neutrino HDD FIX95
    │
    ▼
dvr_pfs0:/ISO/ — gra PS2
```

Dzięki temu nie trzeba uruchamiać Managera ręcznie przez wLaunchELF za każdym razem.

Aplikacja korzysta z wewnętrznego dysku konsoli. Pendrive nie jest wymagany do jej codziennego działania.

---

## 8. Aktualizacja PDIM

Jeżeli aplikacja została już poprawnie zainstalowana w XMB, późniejsze aktualizacje są znacznie prostsze.

Wystarczy podmienić plik:

```text
hdd0:PP.DVRISO/
└── DVR_ISO_MANAGER_HDD.ELF
```

**Nie trzeba ponownie tworzyć partycji ani wykonywać Inject Header.**

Przed aktualizacją wykonaj kopię dotychczasowego ELF-a i zachowaj konfiguracje, moduły oraz pobrane okładki.

---

## 9. Uwagi i bezpieczeństwo

- Instalacja wymaga działającego dysku APA/PFS.
- Nie formatuj całego dysku konsoli Sony PSX.
- Twórz i modyfikuj wyłącznie własną partycję `PP.DVRISO`.
- Używaj **This partition only**.
- Obrazy ISO pozostają w `dvr_pfs0:/ISO/` i nie są przenoszone na partycję aplikacji.
- Pliki `EXECUTE.ELF` oraz `EXECUTE.CNF` są wymagane do uruchamiania z XMB.
- Do działania SYSTEM/ENGLISH zachowaj oba warianty Neutrino i pełne katalogi `config/` oraz `modules/`.
- Samo usunięcie `EXECUTE.ELF` nie przywróci poprzedniego nagłówka APA.
- Integracja XMB została potwierdzona na Sony PSX DESR-7000; inne modele mogą wymagać dodatkowych testów.

**Instalacja zakończona!** PSX DVR ISO MANAGER będzie dostępny bezpośrednio z XMB konsoli Sony PSX DESR.

</details>

<details>
<summary><strong>Uruchamianie PDIM z USB — kliknij, aby rozwinąć</strong></summary>

## Uruchamianie PSX DVR ISO MANAGER
PSX DVR ISO MANAGER umożliwia przeglądanie i uruchamianie gier PS2 z partycji DVR konsoli Sony PSX DESR. Wariant USB nie wymaga tworzenia osobnej partycji aplikacji na dysku twardym.

**Pobierz kompletną paczkę USB:**

[**PSX DVR ISO MANAGER v0.8 USB — POBIERZ**](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/v0.8/PSX-DVR-ISO-Manager-v0.8-USB-COMPLETE.zip)

> **Uwaga:** Link prowadzi do publicznej wersji USB 0.8. Wersja USB 1.0 została również przetestowana na Sony PSX DESR-7000 i zachowuje ten sam sposób instalacji. Po publikacji wydania 1.0 należy pobrać odpowiednią paczkę USB 1.0.

Kompletna paczka zawiera wymagane pliki OSDMenu, Neutrino, moduły oraz konfiguracje. Nie trzeba osobno pobierać starszych wydań.

### 1. Przygotowanie pendrive'a

1. Przygotuj pendrive sformatowany jako **FAT32**.
2. Pobierz i rozpakuj kompletną paczkę USB na komputerze.
3. W rozpakowanym folderze znajdź wewnętrzny katalog **`DESR_DVR`**.
4. Skopiuj cały folder `DESR_DVR` do katalogu głównego pendrive'a.
5. Podłącz pendrive do konsoli Sony PSX DESR.

**Prawidłowa ścieżka aplikacji:**

`mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`

Nie kopiuj na USB zewnętrznego katalogu archiwum ZIP.

### 2. Struktura katalogów USB

Po skopiowaniu plików struktura powinna wyglądać następująco:

```text
mass0:/DESR_DVR/
│
├── DVR_ISO_MANAGER.ELF       # Manager USB 0.8 / 1.0
│
├── BOOT.ELF                  # OSDMenu Launcher
├── BOOT.CNF                  # Konfiguracja uruchamianej gry
│
├── neutrino.elf              # Neutrino - SYSTEM
├── neutrino_lang.elf         # Neutrino - ENGLISH
│
├── NET_PROBE.CNF             # Konfiguracja sieci LAN
│
├── ART/                      # Okładki gier
│   └── SLES_538.86_COV.png   # Przykładowa okładka
│
├── COVERS.IDX                # Indeks okładek
├── COVER_SCAN.LOG            # Raport skanowania ISO
│
├── config/                   # Konfiguracja Neutrino
│   ├── system.toml
│   ├── osdlang.toml
│   └── ...                   # Pozostałe konfiguracje
│
└── modules/                  # Moduły Neutrino i DVR
    ├── ee_core.elf
    ├── ee_core_lang.elf
    ├── dvrfile_fhi.irx
    └── ...                   # Pozostałe moduły
```

*Przedstawiona struktura jest uproszczona. Nie usuwaj żadnych plików z katalogów `config/` i `modules/`.*

Pliki `COVERS.IDX` i `COVER_SCAN.LOG` są tworzone automatycznie podczas skanowania gier. Nie muszą znajdować się w świeżo rozpakowanej paczce.

### 3. Uruchamianie Managera

1. Uruchom **wLaunchELF-R3Z** na konsoli.
2. Otwórz menedżera plików.
3. Przejdź do urządzenia USB (`mass0:`).
4. Otwórz katalog `DESR_DVR`.
5. Uruchom plik **`DVR_ISO_MANAGER.ELF`**.

Możesz również uruchomić Managera przez UDPFS, ale katalog `DESR_DVR` z konfiguracjami i okładkami musi nadal znajdować się na USB.

Po uruchomieniu aplikacja wyświetli listę obrazów ISO zapisanych na partycji DVR.

</details>

## Pobieranie okładek z komputera Windows

Do zestawu dołączono **PSX DVR ART Server v0.8** — prostą aplikację Windows w formacie `.exe` (dostępny jest też [kod źródłowy Python](pc-art-server/README.md)). Komputer pobiera okładki przez internetowy HTTPS, a konsola łączy się tylko z lokalnym HTTP.

1. Pobierz plik **`PSX-DVR-ART-Server-v0.8.exe`** z artefaktu kompilacji Windows w [wydaniu v0.8](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/v0.8) i uruchom na komputerze.
2. W aplikacji Windows wpisz **PC IPv4 Address** (adres komputera), **Console IPv4 Address** (adres konsoli) oraz **HTTP Port**. Kliknij **Start HTTP Server**. Poniższe wartości pochodzą z przetestowanej sieci DESR-7000; w swojej sieci wpisz własne adresy.
3. Na USB utwórz lub popraw plik `mass0:/DESR_DVR/NET_PROBE.CNF`, tak aby odpowiadał ustawieniom:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

   **Skąd wziąć adresy?** Na komputerze Windows uruchom **Wiersz polecenia (CMD)** lub **PowerShell** i wpisz `ipconfig /all` (poprawna składnia to **ukośnik /**, a nie `ipconfig -all`). Sprawdź kartę sieciową, przez którą komputer komunikuje się z konsolą.

   | Parametr | Co wpisać |
   | --- | --- |
   | `IP` | **Ręcznie przydzielony statyczny adres IPv4 konsoli PSX DESR**. Wybierz wolny adres z tej samej podsieci co komputer. **To nie jest adres komputera** i nie pojawi się w wyniku `ipconfig /all`. |
   | `NETMASK` | **Maska podsieci (Subnet Mask)** odczytana w `ipconfig /all`. |
   | `GATEWAY` | **Brama domyślna (Default Gateway)** odczytana w `ipconfig /all`, zazwyczaj adres routera. |
   | `SERVER_IP` | **Adres IPv4 komputera (IPv4 Address)**, na którym działa PSX DVR ART Server — odczytany w `ipconfig /all`. |
   | `SERVER_PORT` | **HTTP Port** z aplikacji na PC, domyślnie `8000`; tej wartości nie podaje `ipconfig`. |

   **Ważne:** `IP` i `SERVER_IP` muszą być różne. Nie używaj adresu zajętego przez inne urządzenie. Jeśli zmieni się adres komputera, popraw `SERVER_IP` i pole **PC IPv4 Address** w aplikacji.

4. W Managerze naciśnij **TRÓJKĄT**. Po skanowaniu zobaczysz liczbę brakujących okładek i pytanie, czy pobrać je z lokalnego HTTP.
5. **X: DOWNLOAD** rozpoczyna pobieranie, **O: SKIP** anuluje. Komputer pobierze grafiki według Game ID, zapisze je w swojej pamięci podręcznej i wyśle konsoli. Manager zapisze tylko **nowe** PNG do USB `ART` i automatycznie przebuduje indeks.

**Cover Sources (źródła okładek):** **Primary — OPL Manager Archive (Archive.org)** to główne źródło, zawierające zwykle pionowe okładki pudełkowe PS2. **Fallback — CosmicScale PSBBN Art Database (GitHub)** to oddzielna baza grafik gier, często kwadratowych; aplikacja sięga po nią **dopiero gdy OPL nie zwróci poprawnej okładki PNG**. Kwadratowy obraz dopasowywany jest do 140×200 pikseli z czarnymi marginesami, bez rozciągania. Jeżeli nie ma grafiki w żadnym źródle, serwer zgłasza brak okładki.

Cache komputera: `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`. Sposób działania i testy serwera opisano w [instrukcji aplikacji Windows](pc-art-server/README.md).

**Bezpieczeństwo:** uruchamiaj serwer tylko w zaufanej sieci prywatnej, bez przekierowania portów na routerze. Przed aktualizacją zachowaj kopię działającego ELF, `BOOT.CNF` i folderu `ART`. Nowe okładki są sprawdzane przed zapisem, istniejące PNG/JPG nie są nadpisywane.

## Zdjęcia i zrzuty ekranu

Poniżej znajdują się te same zdjęcia i zrzuty ekranu aktualnego interfejsu Managera v1.0, które zamieszczono w angielskim README.

<img width="4096" height="3072" alt="IMG_20261107_231102244_HDR" src="https://github.com/user-attachments/assets/3ed2f6ea-2c6d-4011-bd5c-3612ea6c18ce" />
<img width="4096" height="3072" alt="IMG_20261107_231155465_HDR" src="https://github.com/user-attachments/assets/452a6dfe-00df-40a4-a4c7-4738eb14d242" />
<img width="4096" height="2710" alt="1000055890" src="https://github.com/user-attachments/assets/4e095df8-3283-43eb-ac6a-e98bd06d5f8f" />
<img width="756" height="563" alt="Zrzut ekranu 2026-11-01 000809" src="https://github.com/user-attachments/assets/ff72eb3a-a41e-42b3-995e-bfb14f32d7a0" />

## Kompilacja

Repozytorium zawiera projekt CMake/PS2SDK, testy skanera, sieci i PNG zintegrowane z Managerem oraz konfigurację tworzenia Windows EXE przez PyInstaller. Dawny samodzielny Probe został wycofany. Szczegóły: [dvr-iso-manager](dvr-iso-manager/README.md) i [pc-art-server](pc-art-server/README.md).

## Podziękowania

- [Neutrino](https://github.com/ps2max32/neutrino) i [DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228) — pcm720
- Berion — pomoc merytoryczna
- [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) — zapasowe źródło grafik
- Archiwum okładek OPL Manager — główne źródło
