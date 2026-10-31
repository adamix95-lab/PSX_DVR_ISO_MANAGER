# PSX DVR ART Server v0.8 — aplikacja Windows

[Manager — instrukcja po polsku](../README_PL.md) · [English](../README.md)

Aplikacja **PSX DVR ART Server** działa na komputerze Windows jako lokalny serwer HTTP. Konsola Sony PSX DESR wysyła żądanie z **Game ID**, a komputer pobiera grafikę z internetu przez HTTPS, dopasowuje ją do obsługiwanego PNG i wysyła do konsoli przez LAN. Program jest napisany w Pythonie i dostępny jako pojedynczy plik **`.exe`**.

## Pobieranie i uruchomienie

1. Pobierz artefakt **PSX-DVR-ART-Server-v0.8-Windows-EXE** z [GitHub Actions](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/actions).
2. Rozpakuj ZIP i uruchom **`PSX-DVR-ART-Server-v0.8.exe`**. Na Windows nie trzeba osobno instalować Pythona. Plik nie jest podpisany certyfikatem, więc SmartScreen może wyświetlić ostrzeżenie.
3. Wpisz **IP komputera**, **IP konsoli** i **port HTTP**. Dla przetestowanej konfiguracji: `192.168.0.122`, `192.168.0.240`, `8000`.
4. Kliknij **Start serwera HTTP**. Program będzie pokazywał dziennik pobrań. W Windows Firewall zezwól na połączenia **tylko w sieci prywatnej**.
5. Na konsoli uruchom Manager v0.8, naciśnij **TRÓJKĄT**. Przy brakujących okładkach pojawi się ekran `X: DOWNLOAD` / `O: SKIP`. Wybierz X.

Konsola potrzebuje konfiguracji USB w `mass0:/DESR_DVR/NET_PROBE.CNF`, np.:

```ini
IP=192.168.0.240
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
SERVER_IP=192.168.0.122
SERVER_PORT=8000
```

Dostosuj adresy do swojej sieci. Nie otwieraj portu 8000 na routerze — aplikacja jest przeznaczona **wyłącznie do lokalnego LAN**.

## Skąd pobieramy grafikę?

| Kolejność | Źródło | Grafika |
| --- | --- | --- |
| 1 | Cache PC | Już pobrany i zapisany PNG |
| 2 | OPL Manager, Archive.org | Preferowana pionowa okładka `<ID>_COV.png` |
| 3 | [CosmicScale PSBBN Art Database](https://github.com/CosmicScale/psbbn-art-database) | Zapasowe, zwykle kwadratowe artwork `<ID>.png` |

Podstawowe źródło OPL pochodzi z archiwum `OPLM_ART_2024_09`, wykorzystywanego również przez skrypty CosmicScale. PSBBN Art Database to **inna baza**, z której aplikacja korzysta tylko po błędzie lub braku poprawnego obrazu z OPL.

Przykładowe źródła dla Game ID `SLES_538.86`:

```text
OPL:   https://archive.org/download/OPLM_ART_2024_09/OPLM_ART_2024_09.zip/PS2/SLES_538.86/SLES_538.86_COV.png
PSBBN: https://raw.githubusercontent.com/CosmicScale/psbbn-art-database/main/art/SLES_538.86.png
```

W dzienniku zobaczysz `HTTPS OPL`, ewentualnie `HTTPS PSBBN FALLBACK`, a następnie `SOURCE ...: OPL` albo `SOURCE ...: PSBBN`. Plik PSBBN może być kwadratowym obrazem gry, a nie okładką pudełka. Takie grafiki są dopasowywane proporcjonalnie z czarnymi marginesami, bez rozciągania.

**Ograniczenie:** na sprzęcie potwierdzono pobranie **40/40 okładek za pomocą nowego EXE**, ale nie ma jeszcze osobnego potwierdzenia, że podczas tego testu wystąpiło użycie źródła PSBBN. Nie wszystkie Game ID muszą występować w obu bazach.

## Pamięć podręczna i bezpieczeństwo

Znormalizowane obrazy są zapisywane w:

```text
%LOCALAPPDATA%\PSX_DVR_ART_SERVER\cache
```

Żądania mogą przychodzić tylko od IP konsoli podanego w interfejsie. Program nie przyjmuje dowolnych adresów internetowych, a pobranie jest ograniczone do wskazanych domen HTTPS i poprawnych Game ID. Pliki źródłowe są ograniczone do 4 MiB; wynik PNG ma maksymalnie 256 KiB i rozmiar 140×200 pikseli, 24-bitowy RGB.

Aplikacja PC **nie zapisuje plików na konsoli ani na USB**. To Manager odbiera PNG, sprawdza go i zapisuje w `mass0:/DESR_DVR/ART`, nie zastępując istniejących okładek. Po pomyślnym pobraniu Manager sam aktualizuje indeks.

## Uruchomienie z Pythona / kompilacja EXE

```powershell
py -m pip install -r pc-art-server/requirements.txt
py pc-art-server/psx_dvr_art_server.py
```

Wymagany Python 3.10 lub nowszy oraz Pillow; interfejs wykorzystuje Tkinter. Workflow `.github/workflows/build-windows-art-server.yml` buduje Windows `.exe` przez PyInstaller i uruchamia testy pobierania/cache oraz fallbacku PSBBN.

## Rozwiązywanie problemów

- **Brak połączenia z PC:** sprawdź IP, `NET_PROBE.CNF`, zaporę Windows i uruchomiony serwer.
- **OPL / PSBBN nie znalazły grafiki:** zobacz log `NOT FOUND` w aplikacji; na konsolę zostanie odesłane HTTP 404.
- **Grafika już istnieje:** Manager celowo jej nie nadpisuje. Cache na PC również nie nadpisuje wcześniejszych obrazów.
- **Niepełny `.PART` na USB:** Manager pozostawia go do ręcznej weryfikacji zamiast nadpisywać nieznany plik.

Prawa do obrazów należą do odpowiednich właścicieli; korzystaj ze źródeł zgodnie z ich warunkami.
