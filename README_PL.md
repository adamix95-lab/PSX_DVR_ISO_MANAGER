# PSX DVR ISO Manager v0.8

[English](README.md)

**PSX DVR ISO Manager** służy do przeglądania i uruchamiania gier PS2 w formacie ISO z partycji DVR konsoli Sony PSX DESR. Wersja **0.8** zawiera stronicowaną listę gier, podgląd okładek PNG oraz automatyczne uzupełnianie brakujących grafik przez aplikację na komputerze Windows.

**Test na sprzęcie:** Sony PSX DESR-7000, 40 gier na partycji DVR. Potwierdzono pobranie i wyświetlenie **40/40 okładek od zera**, automatyczną odbudowę indeksu i uruchomienie **REZ po angielsku**.

## Możliwości programu

- Lista gier w `dvr_pfs0:/ISO` — do 12 tytułów na stronie, nawigacja bez zawijania od ostatniej strony do pierwszej.
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

Po anulowaniu pobierania KÓŁKIEM można ponownie nacisnąć TRÓJKĄT, aby jeszcze raz wyświetlić pytanie. Nie trzeba restartować Managera. Podczas pytania X oznacza pobieranie; poza nim X nadal uruchamia grę.

## Instalacja na USB

Zachowaj dotychczas działające pliki OSDMenu, Neutrino i sterowniki DVR. Manager ELF możesz uruchomić z USB albo UDPFS, lecz konfiguracja uruchamiania i okładki znajdują się na USB w katalogu **`mass0:/DESR_DVR/`**.

```text
mass0:/DESR_DVR/
├── DVR_ISO_MANAGER.ELF           # Manager v0.8
├── BOOT.ELF                      # OSDMenu Launcher
├── BOOT.CNF                      # Konfiguracja startowa gry
├── BOOT_RECOVERY.CNF             # Kopia zapasowa konfiguracji
├── DVR_TEST.CNF                  # Testowa konfiguracja
├── neutrino.elf                  # Język SYSTEM
├── neutrino_lang.elf             # Język ENGLISH
├── NET_PROBE.CNF                 # Ustawienia sieciowe
├── COVERS.IDX                    # Indeks okładek tworzony przez Managera
├── COVER_SCAN.LOG                # Raport skanowania i brakujące Game ID
├── ART/
│   └── SLES_538.86_COV.png      # Przykład okładki po Game ID
├── config/
│   └── osdlang.toml              # Ustawienia ENGLISH (pozostałe pliki pominięto)
└── modules/
    └── ee_core_lang.elf          # Obsługa ENGLISH (pozostałe moduły pominięto)
```

To uproszczony przykład. **Nie usuwaj pozostałych modułów Neutrino ani działających konfiguracji.** Gry ISO muszą być zapisane na partycji DVR w `dvr_pfs0:/ISO/` (partycja `__xcontents`). Manager **nie zapisuje nic na DVR, APA ani do obrazów ISO**.

## Pobieranie okładek z komputera Windows

Do zestawu dołączamy **PSX DVR ART Server v0.8** — prostą aplikację Windows w formacie `.exe` (dostępny jest też [kod źródłowy Python](pc-art-server/README_PL.md)). Komputer pobiera okładki przez internetowy HTTPS, a konsola łączy się tylko z lokalnym HTTP.

1. Pobierz plik **`PSX-DVR-ART-Server-v0.8.exe`** z artefaktu kompilacji Windows w [GitHub Actions](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions) i uruchom na komputerze.
2. Wprowadź adres komputera, adres konsoli i port. Kliknij **Start serwera HTTP**. Poniżej przykładowa, przetestowana konfiguracja LAN.
3. Na USB utwórz lub popraw `mass0:/DESR_DVR/NET_PROBE.CNF`:

   ```ini
   IP=192.168.0.240
   NETMASK=255.255.255.0
   GATEWAY=192.168.0.1
   SERVER_IP=192.168.0.122
   SERVER_PORT=8000
   ```

4. W Managerze naciśnij **TRÓJKĄT**. Po skanowaniu zobaczysz liczbę brakujących okładek i pytanie, czy pobrać je z lokalnego HTTP.
5. **X: DOWNLOAD** rozpoczyna pobieranie, **O: SKIP** anuluje. Komputer pobierze grafiki według Game ID, zapisze je w swojej pamięci podręcznej i wyśle konsoli. Manager zapisze tylko **nowe** PNG do USB `ART` i automatycznie przebuduje indeks.

Aplikacja na PC sprawdza najpierw archiwum okładek **OPL Manager na Archive.org**, a następnie — jeśli trzeba — bazę **PSBBN Art Database od CosmicScale** na GitHubie. Druga baza może zawierać grafiki kwadratowe zamiast pionowych okładek pudełkowych; program zachowuje proporcje i dodaje czarne marginesy. Nie każda gra musi mieć grafikę w obu źródłach.

Cache komputera: `%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache`. Sposób działania i testy serwera opisano w [instrukcji aplikacji Windows](pc-art-server/README_PL.md).

**Bezpieczeństwo:** uruchamiaj serwer tylko w zaufanej sieci prywatnej, bez przekierowania portów na routerze. Przed aktualizacją zachowaj kopię działającego ELF, `BOOT.CNF` i folderu `ART`. Nowe okładki są sprawdzane przed zapisem, istniejące PNG/JPG nie są nadpisywane.

## Aktualny wygląd Managera

Tutaj można później dodać aktualne zdjęcie interfejsu MONO v0.8.

## Kompilacja

Repozytorium zawiera projekt CMake/PS2SDK, testy skanera i PNG, osobne narzędzia diagnostyczne sieci oraz konfigurację tworzenia Windows EXE przez PyInstaller. Szczegóły: [dvr-iso-manager](dvr-iso-manager/README_PL.md) i [pc-art-server](pc-art-server/README_PL.md).

## Do zrobienia i współpraca

Pozostaje uruchamianie Managera bezpośrednio z XMB konsoli z wewnętrznego HDD (nierozwiązany błąd ścieżki `-19`). Zgłoszenia, pomysły i pull requesty są mile widziane, szczególnie od osób pracujących z Sony PSX DESR i PS2SDK.

## Podziękowania

- [Neutrino](https://github.com/ps2max32/neutrino) i [DVR support PR #228](https://github.com/ps2max32/neutrino/pull/228) — pcm720
- Berion — pomoc merytoryczna
- [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) — zapasowe źródło grafik
- Archiwum okładek OPL Manager — główne źródło
