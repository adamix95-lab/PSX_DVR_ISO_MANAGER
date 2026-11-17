# PSX DVR ISO MANAGER v1.2 USB — kompletna paczka

**Paczka przedpremierowa na gałęzi `pdim_usb_v1.2`. To NIE jest publiczny GitHub Release.**

Funkcje HTTPS, pobierania okładek, starej metody LAN oraz uruchamiania
gier z trybem ENGLISH zostały sprawdzone na fizycznym Sony PSX DESR-7000.
**Nowy ELF bez dopisków wymaga jeszcze jednego testu na konsoli,**
ponieważ pobieracz wraca teraz do niego pod zmienioną nazwą.

## Najważniejsze pliki

```text
DESR_DVR/
  DVR_ISO_MANAGER.ELF       <- GŁÓWNY Manager 1.2 USB
  BOOT.ELF                  <- OSDMenu (uruchamianie gier)
  BOOT.CNF                  <- konfiguracja startu (zachowaj swoją!)
  neutrino.elf              <- język SYSTEM
  neutrino_lang.elf         <- wymuszony ENGLISH
  N4E.CA                    <- certyfikaty HTTPS
  V12_NET.CNF.example       <- wzór konfiguracji HTTPS
  NET_PROBE.CNF             <- stara metoda LAN HTTP
  ART/                      <- pobrane okładki
  config/                   <- konfiguracja Neutrino
  modules/                  <- sterowniki
README.md / README_PL.md
tools/PSX-DVR-ART-Server-v0.8.exe
licenses/
SOURCE_COMMIT
SHA256SUMS
```

Nie ma osobnego `DVR_ISO_MANAGER_V12_CPU.ELF` ani pobieracza HTTPS ELF.
Pobieracz jest dołączony do **`DVR_ISO_MANAGER.ELF`**.

## Instalacja

1. Pendrive FAT32. Zrób kopię dotychczasowego katalogu `DESR_DVR`,
   zwłaszcza `BOOT.CNF`, `V12_NET.CNF`, `NET_PROBE.CNF`,
   `COVERS.IDX`, okładek w `ART/` i działającego ELF-a.
2. **Nowa instalacja:** wypakuj kompletny katalog `DESR_DVR` z ZIP-a
   do katalogu głównego USB, razem z `modules/`, `config/` i Neutrino.
3. **Aktualizacja:** podmień przede wszystkim
   `DESR_DVR/DVR_ISO_MANAGER.ELF`; upewnij się, że jest
   `DESR_DVR/N4E.CA`. Nie nadpisuj własnych ustawień i okładek.
4. HTTPS czyta **wyłącznie** `mass0:/DESR_DVR/V12_NET.CNF`, nie
   `PP.DVRISO12` z HDD. Jeśli masz sprawdzony plik na HDD, skopiuj
   go jednorazowo do `DESR_DVR` na pendrivie, nie kasując oryginału.
   Alternatywnie uzupełnij `V12_NET.CNF.example` własnymi wartościami
   `IP`, `NETMASK`, `GATEWAY`, `DNS` i zapisz jako `V12_NET.CNF`.
   **Archiwum zawiera tylko przykładowy plik**, bez gotowych adresów LAN.
5. Uruchom `mass0:/DESR_DVR/DVR_ISO_MANAGER.ELF` przez
   **wLaunchELF-R3Z 4.80**.

## Sterowanie

W menu gier wciśnij **Trójkąt**, aby wyszukać brakujące okładki.
Na ekranie wyboru: **Trójkąt = pobierz HTTPS bez PC** (maksymalnie
50 okładek na serię), **X = stary serwer LAN HTTP na PC**,
**O = anuluj**. Po pobieraniu **X/O** zamyka podsumowanie.
**SELECT** przełącza `SYSTEM` / `ENGLISH`. Gry startują przez
OSDMenu i Neutrino, jak wcześniej.

## Ważne

- `N4E.CA` musi być w `DESR_DVR`, nie w katalogu głównym USB.
- `NET_PROBE.CNF` nie jest konfiguracją HTTPS — służy do LAN HTTP.
- Metoda entropii z licznika COP0 Count została **bezpośrednio
  zapożyczona z projektu NULD Daniela** i zaadaptowana do DESR.
  Jest eksperymentalna, bez gwarancji kryptograficznej jakości.
- Przed przyszłym publicznym wydaniem wymagana jest weryfikacja licencji
  (przede wszystkim NULD/GPL-3.0 i wolfSSL) oraz końcowy test
  nowej nazwy ELF-a na konsoli.
- Zachowaj poprzedni, działający ELF jako kopię awaryjną.

W pliku `SHA256SUMS` są sumy kontrolne wszystkich plików;
`SOURCE_COMMIT` wskazuje źródłowy commit kompilacji.

**Podziękowania:** Daniel/NULD, Tanner Ryan/TXRyan, wolfSSL,
Monocypher, PS2SDK, ps2max32/Neutrino, pcm720, OPL Manager/Luden02,
CosmicScale, Mozilla/curl i Berion. Pełne opisy źródeł oraz
licencji są w [technicznym README projektu](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/blob/pdim_usb_v1.2/README.md).
