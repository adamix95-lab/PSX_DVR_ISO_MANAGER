# DVR ISO Manager v0.1.5 — USB Quickboot (Sony PSX DESR-7000)

**Eksperymentalna wersja: automatyczny start OSDMenu BOOT.ELF z Managera wymaga jeszcze testu na prawdziwym DESR-7000.**

## Sprawdzone na konsoli — v0.1.3

- Manager wykrywa wszystkie obrazy ISO w `dvr_pfs0:/ISO/` (czyli `__xcontents/ISO/` na DVR).
- Dzięki `sceCdNoticeGameStart(0, NULL)` otwieranie katalogów DVR działa.
- Zapis `DVR_TEST.CNF` używając PS2SDK `FIO_O_*` został zweryfikowany.
- Kopia wygenerowanego CNF pod nazwą `BOOT.CNF` uruchomiła **REZ.iso** przez działający OSDMenu Launcher z USB do menu gry.

## Co się zmienia w v0.1.5

**X** wykonuje:
1. Sprawdza dostępność `neutrino.elf` i oryginalnego działającego `BOOT.ELF` na USB.
2. Odczytuje istniejący, niepusty `BOOT.CNF`. Jeśli brakuje — **nie zapisuje nic**, aż przywrócisz działający plik.
3. Tworzy `BOOT_RECOVERY.CNF` z oryginalnego pliku, jeżeli taka kopia jeszcze nie istnieje, i weryfikuje bajty. Nigdy nie nadpisuje istniejącej, niepustej kopii bezpieczeństwa.
4. Tworzy `BOOT.CNF` dla wybranej gry, zamyka i odczytuje go ponownie, weryfikując dokładną treść i brak BOM.
5. Próbuje uruchomić `BOOT.ELF` poprzez PS2SDK `LoadELFFromFile(USB_LAUNCHER, 0, NULL)`. OSDMenu Launcher uruchomiony bez dodatkowych argumentów czyta `BOOT.CNF` i uruchamia `neutrino.elf`.

Przykład dla `HALF-LIFE.iso`:

```ini
boot=neutrino.elf
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/HALF-LIFE.iso
```

**KWADRAT:** zapisuje/odczytuje `DVR_TEST.CNF` — **nigdy nie edytuje** `BOOT.CNF`.
**GÓRA/DÓŁ:** wybór gry. **TRÓJKĄT:** ponowne skanowanie. **START:** wyjście.

## Struktura USB

```text
mass0:/DESR_DVR/
  DVR_ISO_MANAGER.ELF      <- nowy plik do testu
  BOOT.ELF                 <- sprawdzony OSDMenu Launcher; nie podmieniaj
  BOOT.CNF                 <- poprzednio działający plik; Manager aktualizuje po X
  BOOT_RECOVERY.CNF        <- utworzona JEDNORAZOWO kopia zapasowa
  DVR_TEST.CNF             <- opcjonalnie, po KWADRAT
  neutrino.elf             <- sprawdzone Neutrino PR #228
  config/
  modules/
```

**Przed testem:** zachowaj dodatkową kopię działającego `BOOT.CNF` na PC. Wcześniejszy błąd -3 potrafił pozostawić plik 0 B. Obecna implementacja nie wykonuje automatycznego rollbacku, ale **zachowuje zweryfikowaną kopię `BOOT_RECOVERY.CNF`** przed każdą próbą zapisu `BOOT.CNF`.

## Uwaga dotycząca łańcuchowego uruchamiania

Nie używamy już bezpośredniego `LoadELFFromFile(neutrino.elf, args...)` z v0.1.4. Tam pojawiał się niebieski i żółty ekran, po czym konsola wracała do XMB. Nowe rozwiązanie uruchamia **najpierw OSDMenu Launcher**. Jednak pierwszy etap (przekazanie sterowania do `BOOT.ELF` poprzez PS2SDK loader) jest nadal eksperymentalny i również może się nie udać.

W takim przypadku `BOOT.CNF` nadal zawiera wybraną grę — po ponownym uruchomieniu wLE-R3Z możesz odpalić `BOOT.ELF` **ręcznie**, co jest sprawdzone.

**Nie modyfikuj partycji DVR, APA, Neutrino ani XMB.** Wersja Managera zapisuje jedynie pliki *.CNF na USB.

## Licencje

Program korzysta z PS2SDK (biblioteka `elf-loader`) oraz binarnych modułów DVR z Neutrino [PR #228](https://github.com/ps2max32/neutrino/pull/228).
