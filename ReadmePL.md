<img width="1774" height="887" alt="Metaliczny banner PDIM w pomarańczowych tonach" src="https://github.com/user-attachments/assets/9ffed54c-e8ea-46e2-9af0-65e351b40cb3" />

# PSX DVR ISO MANAGER

[English](README.md)

**Wersja 1.2 — HDD i USB, natywne pobieranie okładek przez HTTPS.**

**PSX DVR ISO MANAGER (PDIM)** umożliwia przeglądanie i uruchamianie gier PlayStation 2 w formacie ISO, zapisanych w systemie plików DVR konsoli **Sony PSX DESR**. Wersja HDD jest głównym wariantem, ale obsługiwane jest również USB. **PDIM 1.2 HDD i instalator XMB przeszły testy funkcjonalne na prawdziwej konsoli DESR-7000.** Testy wersji USB potwierdziły pobieranie okładek HTTPS i uruchamianie gier.

**Oficjalne wydanie 1.2:** [GitHub Release 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/tag/1.2). Wybierz jedną z poniższych paczek:

| Paczka | Przeznaczenie | Zawartość |
| --- | --- | --- |
| [HDD AUTO 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-AUTO-OFFICIAL.zip) | Automatyczna instalacja HDD + XMB | Instalator ELF, Manager, pliki XMB i konfiguracja sieci |
| [HDD MANUAL 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-MANUAL-OFFICIAL.zip) | Instalacja HDD przez wLE-R3Z | Manager, pliki nagłówka XMB i instrukcja ręczna |
| [USB COMPLETE 1.2](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-USB-COMPLETE-OFFICIAL.zip) | Uruchamianie aplikacji z USB | Pełna wersja USB z obsługą HTTPS |

**Archiwalna dokumentacja 1.0:** [English](docs/archive/README_1.0.md) · [Polski](docs/archive/README_PL_1.0.md).

**Dokument techniczny:** [HTTPS 1.2 — TXRyan, wolfSSL i metoda entropii z NULD](docs/HTTPS_1.2_TECHNICAL_EN.md) (angielski).

**Bezpieczeństwo:** Przed modyfikacją dysku **wykonaj pełną odtwarzalną kopię sektorową HDD i sprawdź jej poprawność**. Nie formatuj oryginalnych partycji systemowych ani DVR Sony. Obrazy gier ISO pozostają w oryginalnym obszarze DVR i nie są modyfikowane przez PDIM.

---

## 1. Wymagania

- Konsola **Sony PSX DESR** ze sprawnym wewnętrznym dyskiem DVR (projekt i instalacja sprawdzone na **DESR-7000**).
- **wLaunchELF-R3Z 4.80** lub zgodny program obsługujący DESR — do uruchamiania instalatora, przeglądania PFS/HDD i wykonywania ELF-ów z USB.
- Pendrive sformatowany jako **FAT32** do uruchomienia instalatora, ręcznego kopiowania plików lub korzystania z PDIM USB. Można również użyć UDPFS przy ręcznej instalacji HDD.
- Obrazy ISO gier PS2 zapisane w oryginalnym systemie plików DVR Sony, oddzielnie od plików Managera.
- Dla **HDD 1.2** partycja aplikacji **`hdd0:PP.DVRISO`**. W przypadku jej braku automatyczny instalator może ją utworzyć; można ją również przygotować ręcznie w wLE-R3Z. **Nie używaj `PP.DVRISO12`** — była to partycja testowa.
- Dla **USB 1.2** nie trzeba tworzyć nowej partycji aplikacji: pliki znajdują się w `mass0:/DESR_DVR/`.
- Sieć Ethernet i konfiguracja są potrzebne tylko do pobierania okładek. Do **HTTPS bez komputera** służy `V12_NET.CNF`, a do **Windows PC / LAN HTTP** — `NET_PROBE.CNF`.

**Dostępne są dwie metody instalacji HDD:** automatyczna **HDD/XMB** oraz **ręczna**. Obie pozwalają korzystać z wersji 1.2 HDD uruchamianej z XMB. Nie instaluj obydwu metod jedna na drugiej bez potrzeby.

## 2. Struktura plików HDD

Wersja HDD zapisuje aplikację, Neutrino, ustawienia i okładki na partycji **`PP.DVRISO`**, a nie w miejscu przechowywania gier Sony.

Przy instalacji ręcznej kopiuj **zawartość** folderu `PP.DVRISO-FILES/` bezpośrednio do głównego katalogu partycji. Instalator automatyczny robi to za Ciebie i sprawdza pliki.

```text
hdd0:PP.DVRISO/
│
├── DVR_ISO_MANAGER_HDD.ELF       # Manager 1.2 HDD z wbudowanym HTTPS
│
├── EXECUTE.ELF                   # Podpisany launcher XMB (KELF)
├── EXECUTE.CNF                   # Konfiguracja XMB -> Manager
├── BOOT.ELF                      # OSDMenu / uruchamianie gier
├── BOOT.CNF                      # Tworzony i zmieniany przy uruchamianiu gry
│
├── neutrino_hdd.elf             # Język SYSTEM
├── neutrino_hdd_lang.elf        # Wymuszony ENGLISH
│
├── N4E.CA                       # Certyfikaty główne HTTPS (CA)
├── V12_NET.CNF                  # Ustawienia sieci HTTPS
├── NET_PROBE.CNF                # Ustawienia serwera Windows LAN HTTP
│
├── ART/                         # Okładki gier
│   ├── SLES_XXXXX_COV.png
│   └── SLUS_XXXXX_COV.png
├── COVERS.IDX                   # Generowany indeks okładek
├── COVER_SCAN.LOG               # Generowany raport skanowania
│
├── config/                      # Pliki konfiguracyjne Neutrino (TOML)
│   ├── system.toml
│   ├── bsd-dvr.toml
│   ├── osdlang.toml
│   └── ...                      # Pozostałe pliki zestawu
│
├── modules/                     # Moduły DVR / Neutrino i rdzeń EE
│   ├── ee_core.elf
│   ├── ee_core_lang.elf
│   ├── dvrfile_fhi.irx
│   ├── dvrdrv.irx
│   ├── dvrfile.irx
│   └── ...                      # Pozostałe moduły
│
└── res/                         # Grafika i metadane XMB
    ├── info.sys
    ├── jkt_001.png
    └── jkt_002.png
```

*To uproszczone drzewko. Zachowaj wszystkie pozostałe pliki w `config/` i `modules/`. Pliki `BOOT.CNF`, `COVERS.IDX` i `COVER_SCAN.LOG` mogą być nieobecne po świeżej instalacji — zostaną utworzone podczas użytkowania.*

**`V12_NET.CNF`** zawiera `IP`, `NETMASK`, `GATEWAY` i `DNS`. Instalator automatycznie kopiuje plik z paczki **tylko jeśli nie istnieje na HDD**, po sprawdzeniu jego zawartości. Istniejące ustawienia są zachowywane. Przykładowy adres `192.168.0.240` **nie jest uniwersalną wartością** — dostosuj go do własnej sieci.

## 3. Gdzie przechowywać obrazy ISO gier

Obrazy gier **nie** znajdują się na `PP.DVRISO`.

Manager korzysta z oryginalnego systemu plików DVR konsoli Sony PSX:

```text
dvr_pfs0:/_xcontent/ISO/
│
├── ICO.iso
├── MGS 2.ISO
├── REZ.iso
├── Metal Slug Anthology.iso
└── ...
```

Partycja `PP.DVRISO` przeznaczona jest wyłącznie na aplikację, konfigurację i okładki.

**Manager odczytuje pliki ISO z partycji DVR, nie modyfikując ich zawartości.**

---

## 4. Instalacja PDIM

Wybierz **jedną** z poniższych metod. Obie wersje HDD korzystają z `PP.DVRISO`; wariant USB nie wymaga nowej partycji aplikacji.

> **WAŻNE OSTRZEŻENIE:** Każda operacja ingerująca w obszar **APA** może wiązać się z ryzykiem uszkodzenia struktury systemowej dysku twardego konsoli Sony PSX. Zachowaj szczególną ostrożność.
>
> Instalator został osobiście przetestowany wielokrotnie, również po celowym usunięciu istniejącej partycji `PP.DVRISO` różnymi sposobami — zarówno z poziomu **XMB**, jak i przez **HDD Manager** w **wLaunchELF-R3Z**. We wszystkich tych testach instalator prawidłowo odtworzył partycję `PP.DVRISO` i zakończył instalację pomyślnie.
>
> Nie stanowi to jednak gwarancji bezpieczeństwa w każdej sytuacji. **Przed wprowadzeniem zmian zdecydowanie zalecamy wykonanie pełnej, możliwej do odtworzenia kopii sektorowej całego dysku HDD.**

<details>
<summary><strong>Instalacja PDIM 1.2 za pomocą instalatora HDD/XMB</strong></summary>

**Paczka:** [HDD AUTO OFFICIAL](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-AUTO-OFFICIAL.zip)

1. Wykonaj odtwarzalną kopię sektorową całego HDD. Jeśli `PP.DVRISO` już istnieje, nie formatuj jej.
2. Rozpakuj ZIP na pendrive FAT32, zachowując folder `PDIM_HDD_V12_INSTALLER/` w katalogu głównym USB (`mass0:/`).
3. Uruchom **wLaunchELF-R3Z 4.80**, a następnie `mass0:/PDIM_HDD_V12_INSTALLER/PDIM_HDD_V12_INSTALLER.ELF`.
4. Sprawdź wyświetlony cel instalacji: **`PP.DVRISO`**. Przycisk **SELECT** pozwala obejrzeć nagłówek XMB/PPAA bez zapisu.

<img width="4096" height="3072" alt="IMG_20261118_232754515_HDR" src="https://github.com/user-attachments/assets/af9510f7-4bc1-4821-8c24-42d34074416f" />

5. Wybierz **X — Install Core**. Potwierdź przyciskiem **X**, następnie **przytrzymaj L1 + R1 i naciśnij X**. Instalator utworzy brakującą partycję PFS 256 MiB albo wykorzysta istniejącą **bez formatowania**. Oczekiwany wynik: **79/79 zweryfikowanych plików Core**.

<img width="4096" height="3072" alt="IMG_20261118_232811531_HDR" src="https://github.com/user-attachments/assets/b2e2176d-2199-4240-99bc-4e6d5d227415" />
<img width="4096" height="3072" alt="IMG_20261118_232850922_HDR" src="https://github.com/user-attachments/assets/b100d05a-9b21-4598-9a33-ed4c111dae2d" />

6. Jeśli na HDD brakuje `V12_NET.CNF`, instalator skopiuje poprawny plik z USB. Jeśli już istnieje, zostanie zachowany (**KEEP**).
7. Wybierz **TRIANGLE — Install XMB** i przejdź przez takie same dwa potwierdzenia. Program kopiuje i sprawdza `EXECUTE.ELF`, `EXECUTE.CNF`, `res/info.sys`, `res/jkt_001.png` oraz `res/jkt_002.png` **przed rejestracją PPAA**. Oczekiwany wynik: **84/84 VERIFIED** i poprawny nagłówek XMB.

<img width="4096" height="3072" alt="IMG_20261118_232906042_HDR" src="https://github.com/user-attachments/assets/69a360ca-d812-404e-8fa2-18b41d822d69" />
<img width="4096" height="3072" alt="IMG_20261118_232943225_HDR" src="https://github.com/user-attachments/assets/c5b1094d-fdd8-4656-81c5-227f20fe93eb" />

8. Całkowicie wyłącz konsolę, uruchom Sony XMB → **Games** → **PSX DVR ISO MANAGER**. Program powinien pokazywać wersję **1.2 HDD**.

<img width="4096" height="3072" alt="IMG_20261118_075241288_HDR" src="https://github.com/user-attachments/assets/695d98d3-6582-4d6b-a944-168be6e41641" />
<img width="4096" height="3072" alt="IMG_20261118_075338261_HDR" src="https://github.com/user-attachments/assets/254dbb50-5db9-4e3e-927b-83ea644a9951" />

**Ponowna instalacja została sprawdzona na konsoli:** Core pokazał `COPIED 0 / KEPT 79 / VERIFIED 79`, a XMB pięć `KEEP`, `VERIFIED 84` i `ALREADY VERIFIED`.

W przypadku błędu **nie formatuj, nie usuwaj i nie zmieniaj rozmiaru `PP.DVRISO`**. Szczegóły: [dokumentacja instalatora](pdim-hdd-v1.2-installer/README_PL.md).

</details>

<details>
<summary><strong>Instalacja PDIM 1.2 manualnie na HDD</strong></summary>

**Paczka:** [HDD MANUAL OFFICIAL](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-HDD-MANUAL-OFFICIAL.zip)

1. Zrób pełną kopię sektorową HDD. Zabezpiecz wcześniejszy ELF Managera, `BOOT.CNF`, `V12_NET.CNF`, folder `ART/` i własne dane.
2. Rozpakuj paczkę na pendrive FAT32. Zachowaj dokładne foldery `PP.DVRISO-FILES/` oraz `__Headers/PP.DVRISO/`.
3. W **wLaunchELF-R3Z 4.80 → MISC → HDDManager** sprawdź partycje. Jeśli `PP.DVRISO` istnieje, **nie twórz jej ponownie**. Jeśli jej nie ma, utwórz partycję **PFS 256 MiB** o nazwie `PP.DVRISO`.
4. Skopiuj **zawartość** `PP.DVRISO-FILES/` bezpośrednio do głównego katalogu partycji, zachowując podfoldery. Nie kopiuj folderu `PP.DVRISO-FILES` jako dodatkowego katalogu. Zabezpiecz pliki przed ewentualnym nadpisaniem.
5. Plik `V12_NET.CNF` z głównego katalogu paczki kopiuj tylko wtedy, gdy nie ma go na `PP.DVRISO`. Zmień adresy IP, jeśli Twoja sieć wymaga innych ustawień.

<img width="4096" height="3072" alt="IMG_20261118_075319851_HDR" src="https://github.com/user-attachments/assets/4d164d7d-90d3-486a-b20e-0caa3acf356a" />

6. Opcjonalnie uruchom `DVR_ISO_MANAGER_HDD.ELF` z przeglądarki HDD/PFS i sprawdź działanie.
7. Aby zarejestrować **nowy wpis XMB**, pozostaw `__Headers/PP.DVRISO/` w katalogu głównym USB i wybierz **HDDManager → PP.DVRISO → R1 → Inject Header → This partition only**. Wcześniej wszystkie pięć plików XMB musi być już zapisanych na PFS.
8. Uruchom konsolę ponownie i włącz Managera przez **XMB → Games**.

Jeżeli działający wpis XMB z HDD 1.0 już istnieje, **nie wstrzykuj ponownie nagłówka bez potrzeby** — zgodny launcher może uruchamiać nowy Manager 1.2.

[Pełna instrukcja ręczna](hdd-v1.2/INSTALL_MANUAL_PL.md).

</details>

<details>
<summary><strong>Uruchamianie PDIM 1.2 z pamięci USB</strong></summary>

**Paczka:** [USB COMPLETE OFFICIAL](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ISO-Manager-v1.2.0-USB-COMPLETE-OFFICIAL.zip)

1. Sformatuj pendrive jako **FAT32**.
2. Rozpakuj ZIP i skopiuj folder `DESR_DVR/` **bezpośrednio do katalogu głównego USB**.
3. Uruchom **wLaunchELF-R3Z** i wybierz `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF`.
4. Podczas korzystania z PDIM pendrive powinien pozostać podłączony. Gry nadal są odczytywane z oryginalnego systemu DVR Sony.
5. Aby korzystać z HTTPS, utwórz `mass0:/DESR_DVR/V12_NET.CNF` na podstawie dołączonego `V12_NET.CNF.example`, wpisując ustawienia swojej sieci. Do pobierania przez komputer nadal służy `DESR_DVR/NET_PROBE.CNF`.

```text
mass0:/DESR_DVR/
├── DVR_ISO_MANAGER.ELF         # Manager USB 1.2 z HTTPS
├── BOOT.ELF
├── BOOT.CNF
├── neutrino.elf
├── neutrino_lang.elf
├── N4E.CA
├── V12_NET.CNF                 # Konfiguracja użytkownika
├── V12_NET.CNF.example
├── NET_PROBE.CNF
├── ART/
├── config/
└── modules/
```

Zachowaj wszystkie pliki z folderu `DESR_DVR/`, w tym moduły i konfiguracje. Szczegóły: [instrukcja USB 1.2 po polsku](docs/usb12-package-pl.md).

</details>

## 5. Jak korzystać z PDIM

Manager może przeglądać do **128 obrazów ISO** (limit w kodzie), wyświetlając **12 gier na stronie**. Liczba wykrytych ISO nie oznacza, że każda gra została osobno przetestowana pod kątem uruchamiania.

| Przycisk | Funkcja |
| --- | --- |
| **GÓRA / DÓŁ** | Poprzednia / następna gra |
| **LEWO / PRAWO** | Zmiana strony (12 gier) |
| **X** (lista gier) | Uruchomienie wybranej gry przez OSDMenu / Neutrino |
| **TRIANGLE** (lista gier) | Skanowanie ISO, rozpoznawanie ID i odświeżanie okładek |
| **X** (pytanie o pobranie) | Pobieranie przez Windows **PC ART Server** (LAN HTTP) |
| **TRIANGLE** (pytanie o pobranie) | Pobieranie bezpośrednio z konsoli przez **HTTPS** (do 50 na serię) |
| **CIRCLE (O)** (pytanie o pobranie) | Anulowanie / pominięcie |
| **X / CIRCLE (O)** (wynik HTTPS) | Powrót do listy gier |
| **SQUARE** (lista gier) | Zapisanie konfiguracji wybranej gry do `DVR_TEST.CNF` |
| **SELECT** | Przełączanie `SYSTEM` / `ENGLISH` |
| **START** | Wyjście z PDIM |

**Zdjęcia z konsoli Sony PSX DESR-7000:**

<img width="4096" height="3072" alt="PDIM application entry in the Sony PSX XMB" src="https://github.com/user-attachments/assets/3ed2f6ea-2c6d-4011-bd5c-3612ea6c18ce" />

<img width="4096" height="3072" alt="IMG_20261118_231052225_HDR" src="https://github.com/user-attachments/assets/6a092292-7000-4da6-9a16-70da55ba7a7f" />

<details>
<summary><strong>Pobieranie okładek przez Windows PC ART Server (LAN HTTP)</strong></summary>

Program **[PSX DVR ART Server v0.8](pc-art-server/README.md)** działa w Windows. Komputer pobiera obrazy przez HTTPS, a następnie przesyła je do PDIM przez lokalną sieć **HTTP**.

1. Pobierz [PSX-DVR-ART-Server-v0.8.exe](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/releases/download/1.2/PSX-DVR-ART-Server-v0.8.exe) i uruchom go na komputerze Windows.
2. Wpisz **PC IPv4 Address**, **Console IPv4 Address** i **HTTP Port**; kliknij **Start HTTP Server**.
3. Ustaw `NET_PROBE.CNF` na konsoli (dla HDD w `PP.DVRISO`, dla USB w `DESR_DVR/`). Przykład:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

   **IP** to wolny adres statyczny konsoli PSX. **SERVER_IP** to adres komputera, **nie** konsoli. Sprawdź adresy poleceniem `ipconfig /all` w Windows.

4. W menu gier naciśnij **TRIANGLE**, aby przeskanować bibliotekę, a następnie **X — PC APP (LAN HTTP)**, aby rozpocząć pobieranie.

<img width="4096" height="2710" alt="PSX DVR ISO Manager game list" src="https://github.com/user-attachments/assets/4e095df8-3283-43eb-ac6a-e98bd06d5f8f" />

5. Serwer Windows pobierze brakujące okładki, sprawdzi je i zapisze w katalogu `ART/` aplikacji, a Manager odświeży indeks. Istniejące grafiki są zachowywane.

Nie przekierowuj portu serwera LAN HTTP do Internetu. Folder pamięci podręcznej PC: `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`.

**Źródła grafik:** archiwum OPL Manager (główne) i CosmicScale PSBBN Art Database (zapasowe).

<img width="756" height="563" alt="Windows ART Server interface" src="https://github.com/user-attachments/assets/ff72eb3a-a41e-42b3-995e-bfb14f32d7a0" />

</details>

<details>
<summary><strong>Pobieranie okładek bezpośrednio przez HTTPS na konsoli</strong></summary>

**Nowość w PDIM 1.2:** konsola Sony PSX DESR może pobierać brakujące okładki bezpośrednio z Internetu, **bez uruchamiania Windows ART Server**.

1. Podłącz konsolę do Ethernetu i ustaw `V12_NET.CNF`:
   - **HDD:** `PP.DVRISO/V12_NET.CNF` (instalator kopiuje plik tylko gdy go brakuje).
   - **USB:** `mass0:/DESR_DVR/V12_NET.CNF` (utwórz na podstawie `.example`).
2. Upewnij się, że obok Managera znajduje się plik certyfikatów `N4E.CA`.
3. Włącz PDIM, naciśnij **TRIANGLE** (skan gier), a potem **TRIANGLE — SERVER (HTTPS, UP TO 50)**.

<img width="4096" height="3072" alt="IMG_20261118_075348419_HDR" src="https://github.com/user-attachments/assets/393df748-2ac8-44fb-bcc7-04bb245cd819" />

4. Moduł pobierania uzyskuje **kryptograficznie podpisany czas** z usługi **TXRyan Roughtime**, a następnie **wolfSSL** sprawdza certyfikat serwera HTTPS, jego nazwę oraz okres ważności.
5. Brakujące okładki są pobierane z archiwum OPL Manager (z zapasowym źródłem CosmicScale/PSBBN), sprawdzane, normalizowane i zapisywane bez nadpisywania istniejących plików.

<img width="4096" height="3072" alt="IMG_20261118_075435773_HDR" src="https://github.com/user-attachments/assets/877033ea-138f-4762-b5a9-87b0e8a2878a" />

6. Po zakończeniu odczytaj podsumowanie **DOWNLOADED / SKIPPED**, naciśnij **X lub O** i wróć do listy. Jeśli masz więcej niż 50 brakujących okładek, przeskanuj gry ponownie.

<img width="4096" height="3072" alt="IMG_20261118_075543062_HDR" src="https://github.com/user-attachments/assets/59a2a2f4-17e0-4c61-abf3-caaa4cc0e06f" />
<img width="4096" height="3072" alt="IMG_20261118_075600051_HDR" src="https://github.com/user-attachments/assets/21a337b4-c230-487b-997a-1bc7a29ec83b" />

**Dlaczego czas sieciowy?** Zegar RTC w PSX DESR może być rozładowany lub pokazywać błędną datę. TXRyan dostarcza podpisany czas potrzebny do sprawdzenia ważności certyfikatów HTTPS. To usługa internetowa, a nie program instalowany na konsoli.

**Skąd entropia?** Metoda eksperymentalna korzysta z licznika **COP0 Count** procesora PS2, na podstawie pomysłu zaczerpniętego z projektu **[NULD (dawniej ORBIT) Daniela](https://github.com/danielnuld/nuld)**. Monocypher/BLAKE2b służy do wyprowadzenia różnych wartości na kolejne żądania. Takie przetwarzanie **nie gwarantuje dobrej losowości i nie tworzy brakującej entropii**. To nie jest certyfikowany generator liczb losowych.

Więcej szczegółów: [HTTPS 1.2 — opis TXRyan, wolfSSL, NULD i ograniczeń](docs/HTTPS_1.2_TECHNICAL_EN.md) (angielski).

</details>

## Kompilacja (Building)

Repozytorium zawiera projekt CMake/PS2SDK, zintegrowane skanowanie ISO, testy sieci i PNG oraz konfigurację PyInstaller dla aplikacji Windows. Dawny osobny Probe został wycofany. Więcej informacji w [dvr-iso-manager](dvr-iso-manager/README.md) i [pc-art-server](pc-art-server/README.md).

## Podziękowania (Credits)

- [Neutrino / ps2max32](https://github.com/ps2max32/neutrino) i [DVR support PR #228 autorstwa pcm720](https://github.com/ps2max32/neutrino/pull/228) — uruchamianie gier PS2 i obsługa modułów DVR.
- **Berion** — pomoc techniczna.
- [OPL Manager Art Database](https://github.com/Luden02/psx-ps2-opl-art-database) — główne źródło okładek; [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) — źródło zapasowe.
- [Daniel / NULD (ORBIT)](https://github.com/danielnuld/nuld) — pomysł eksperymentalnego pozyskiwania entropii przez **COP0 Count**; [oryginalne `net.c` z NULD](https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c).
- [wolfSSL](https://github.com/wolfSSL/wolfssl) — połączenia TLS/HTTPS i weryfikacja certyfikatów serwerów.
- [TXRyan Roughtime](https://github.com/tannerryan/roughtime) — publiczna usługa **podpisanego czasu** używana do sprawdzania ważności certyfikatów HTTPS.
- [Monocypher](https://monocypher.org/) — funkcje kryptograficzne Ed25519/BLAKE2b wykorzystywane do weryfikacji czasu i obliczeń danych wejściowych.
- [PS2SDK](https://github.com/ps2dev/ps2sdk), [wLaunchELF-R3Z](https://github.com/NathanNeurotic/wLaunchELF_R3Z) i [RepairBox PSX XMB Installer](https://github.com/DanielFergisz/RepairBox-PSX-XMB-Apps-Installer) — narzędzia, PFS/APA i rozwiązania pomocne przy instalatorze.

**Licencje i dystrybucja:** Poszczególne projekty zachowują własne warunki licencyjne. Użyta wersja wolfSSL jest dostępna na GPLv3 lub późniejszej albo na licencji komercyjnej. Pozostałe komponenty mogą mieć inne licencje. Odnośniki do źródeł i informacje o pochodzeniu kodu nie zastępują obowiązków wynikających z licencji.

**Testy HDD 1.2:** działanie potwierdzono na **Sony PSX DESR-7000**. Inne modele i konfiguracje dysków wymagają dodatkowych testów.
