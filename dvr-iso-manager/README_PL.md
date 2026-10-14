# DVR ISO Manager v0.1.4 — Direct Launch (PSX DESR-7000)

**Wersja testowa: Direct Launch NIE została jeszcze sprawdzona na konsoli.**
Repozytorium: `adamix95-lab/DESR7000-DVR`; działająca kompilacja Neutrino DVR PR #228 pozostaje bez zmian.

## Stan potwierdzony na konsoli (v0.1.3)

- `dvr_pfs0:/ISO` — odczyt **6 ISO** z `__xcontents/ISO/`, bez zmian na DVR i APA.
- `sceCdNoticeGameStart(0, NULL)` zainicjalizował dostęp do DVR (`notice=1`, `hdd0=0`, `pfs0 root=0`).
- Testowy `DVR_TEST.CNF` zapisany na USB przy pomocy `FIO_O_*`, odczytany i zweryfikowany.
- Po ręcznym skopiowaniu pliku na `BOOT.CNF` gra **REZ** uruchomiła się do menu głównego przez OSDMenu Launcher + Neutrino.

## Co testujemy w v0.1.4

**X:** natychmiast uruchamia `mass0:/DESR_DVR/neutrino.elf`, przekazując w pamięci dwa argumenty:

```text
-bsd=dvr
-dvd=dvr_pfs0:/ISO/<dokladna_nazwa_pliku.iso>
```

Wykorzystujemy `LoadELFFromFile()` z biblioteki PS2SDK **elf-loader**.
Ta biblioteka uruchamia mały loader i resetuje IOP po wczytaniu pliku ELF.
To inny mechanizm startu niż używany przez sprawdzony OSDMenu Launcher,
więc jego działanie z Neutrino DVR musi zostać zweryfikowane na DESR-7000.

**KWADRAT:** tworzy osobny `mass0:/DESR_DVR/DVR_TEST.CNF` i sprawdza bajty oraz brak BOM. Umożliwia użycie **potwierdzonej** metody przez ręczne skopiowanie do `BOOT.CNF`.

**TRÓJKĄT:** ponownie skanuje DVR; **góra/dół:** wybór ISO; **START:** wyjście.

### Bezpieczeństwo

- Direct Launch **nie zapisuje** plików na USB ani na partycjach DVR/APA.
- KWADRAT zapisuje **wyłącznie** `DVR_TEST.CNF`.
- Manager **nigdy nie nadpisuje `BOOT.CNF`**.
- Przed testem zachowaj działającą konfigurację OSDMenu Launcher, `BOOT.CNF`, `config/` i `modules/` na USB.
- W razie czarnego ekranu lub powrotu do systemu po naciśnięciu X uruchom konsolę ponownie i przetestuj dotychczasowy `BOOT.ELF`; nie modyfikuj partycji.

## USB

```text
mass0:/DESR_DVR/
├── DVR_ISO_MANAGER.ELF     # nowy artefakt testowy v0.1.4
├── BOOT.ELF                # sprawdzony OSDMenu Launcher — bez zmian
├── BOOT.CNF                # sprawdzona konfiguracja — bez zmian
├── neutrino.elf            # działające Neutrino PR #228 — bez zmian
├── config/
├── modules/
└── DVR_TEST.CNF            # generowany WYŁĄCZNIE po naciśnięciu KWADRAT
```

## Procedura testowa

1. Pobierz artefakt z workflow `Build DVR ISO Manager v0.1.4`.
2. Podmień **tylko** `DVR_ISO_MANAGER.ELF` na USB.
3. Uruchom przez wLE-R3Z i sprawdź listę ISO.
4. Zaznacz **REZ.iso**, naciśnij **X** i sprawdź, czy gra uruchomi się do menu.
5. Jeżeli start się nie powiedzie, sfotografuj ekran/komunikat. Metoda `KWADRAT` + ręczne `BOOT.CNF` pozostaje dostępna jako punkt odniesienia.

## Autorzy

- Odczyt DVR oparty o Neutrino [PR #228](https://github.com/ps2max32/neutrino/pull/228) i kod wLE-R3Z.
- Uruchamianie ELF wykorzystuje bibliotekę `elf-loader` z PS2SDK (AFL-2.0).
- Bez integracji XMB; obsługa zmiany języka gier pozostaje planem na v0.2.
