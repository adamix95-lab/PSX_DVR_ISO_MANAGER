# G4 — PSX Drive Mode + Native QUIT GAME — oddzielny test USB

**Wersja tylko eksperymentalna. Nie jest poprawką 1.0 i nie wolno jej instalować w XMB ani podmieniać HDD.**

## Potwierdzone wyniki na DESR-7000

| Wariant | SCMD 0x29 | Start gry | Fizyczny QUIT |
| --- | --- | --- | --- |
| G2B | mode=1, 1 bajt | czarny ekran | powrót do XMB po kilku sekundach |
| G3A | mode=1, 4 bajty | czarny ekran | powrót do XMB po dość długim czasie |
| G3B | mode=0, 1 bajt | gra się uruchamia | brak reakcji |

Te wyniki wskazują na wpływ wartości `mode=1`. **Nie znamy jeszcze przyczyny czarnego ekranu.**

## Dlaczego próbujemy G4?

W oryginalnej implementacji Sony/OSDMenu na DESR ścieżka po resecie IOP do `rom0:UDNL rom0:OSDCNF` robi:

1. `sceCdInit(SCECdINoD)`
2. `custom_sceCdChgSys(2)` — przełącza podsystem napędu z trybu nagrywarki Rainbow w PS2
3. `custom_sceCdNoticeGameStart(1)` — zgłasza uruchomienie gry
4. `sceCdInit(SCECdEXIT)`

Źródło: https://github.com/pcm720/OSDMenu/blob/a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f/common/src/psxinit.c#L63-L85

Bazowy Neutrino używa tylko komendy NoticeGameStart, bez poprzedzającego wywołania ChangeSys. Może to być powód problemu — **hipoteza, nie wynik testu**.

G4 dodaje do *tego samego miejsca Neutrino*, po `sceCdInit`, jednorazowe SIF RPC `0x2D`, wysyłające `mode=2`. To jest **numer funkcji RPC**, nie komenda MechaCon: nie mylić go z `sceCdApplySCmd(0x2D)`. Następnie wysyła istniejący MechaCon SCMD `0x29`, jeden bajt, w jednym z dwóch wariantów. Dla niepowodzenia `ChangeSys` kod diagnostyczny zwraca `-118` bez wysyłania SCMD `0x29`. Brak nieskończonych prób, dodatkowych resetów IOP i patchowania jądra.

| Wariant | ChangeSys RPC 0x2D | SCMD 0x29 | Cel |
| --- | --- | --- | --- |
| **G4A — kontrolny, testuj pierwszy** | 2 | mode=0, 1 bajt | Czy samo przełączenie napędu nadal pozwala wystartować grę |
| **G4B — eksperymentalny** | 2 | mode=1, 1 bajt | Czy pełna sekwencja Sony uruchamia grę i QUIT |

W obu przypadkach Neutrino pochodzi z przypiętej rewizji PR228, a komplet modułów, `BOOT.ELF`, EE core i konfiguracji pochodzi bajtowo z publicznego USB 1.0.

## Test na konsoli

1. Z workflow `Native QUIT G4 - Sony PSX change-system then NoticeGameStart (USB only)` pobierz artefakt `DESR-QUIT-G4-SONY-SEQUENCE-USB-UNTESTED`. W środku znajduje się archiwum `DESR_QUIT_G4_SONY_SEQUENCE_USB.zip`.
2. Wypakuj foldery `DESR_G4A/` i `DESR_G4B/` bezpośrednio na **FAT32 USB**. Niczego nie nadpisuj w `DESR_DVR/` i na `PP.DVRISO`; nie instaluj żadnych nowych nagłówków XMB.
3. W wLE-R3Z 4.80 uruchom **najpierw** `mass0:/DESR_G4A/DVR_ISO_MANAGER.ELF`. Musi pojawić się `1.0 USB G4A`. Wybierz znaną działającą grę w trybie **SYSTEM**, najlepiej tę samą co w G3B. Naciśnij X.
4. **Jeżeli G4A nie uruchamia gry — przerwij eksperyment. Nie testuj G4B.** Zapisz ostatni ekran (czarny ekran, kod, zawieszenie itd.). To wskazywałoby, że samo ChangeSys(2) źle współpracuje z naszą ścieżką DVR/FHI albo wykonywanie testu wymaga dalszego debugowania.
5. Jeśli G4A uruchamia grę, bez operacji zapisu sprawdź reakcję fizycznego QUIT (prawdopodobnie brak, jak w G3B). Wróć do wLE zwykłą metodą.
6. **Tylko jeżeli G4A uruchamia grę**, uruchom `mass0:/DESR_G4B/DVR_ISO_MANAGER.ELF` i tę samą grę w SYSTEM. Zanotuj: *czy gra wystartowała*, oraz osobno *czy fizyczny QUIT przywrócił XMB*.
7. Napisz: `G4A gra tak/nie; QUIT tak/nie; G4B gra tak/nie; QUIT tak/nie`. Jeśli gra nie wystartowała, opisz dokładnie ostatni obraz i czy było jakiekolwiek opóźnienie.

**Nie naciskaj QUIT podczas zapisu save, kopiowania ISO, pobierania okładek czy aktualizacji indeksów.** Funkcja ChangeSys zmienia aktywny tryb sprzętowy napędu, jej zgodność z DVR ISO nie jest potwierdzona. Możliwy jest czarny ekran wymagający ręcznego resetu; na tej gałęzi nie ma gwarancji odzyskania sterowania. Nie używaj tych binarek do codziennej gry.

## Wyniki sprzętowe G4 — 2026-11-13

**G4A (control):** po wybraniu gry przez X gra z DVR ISO wystartowała poprawnie. Fizyczny QUIT GAME nie reagował. Wynik: **boot PASS / QUIT FAIL**.

**G4B (ChangeSys(2) + NoticeGameStart(1)):** po naciśnięciu X był **czarny ekran**, gra nie wystartowała. Fizyczny QUIT GAME **nie reagował**, nie było powrotu do XMB. Po całkowitym wyłączeniu i ponownym uruchomieniu konsoli użytkownik ponowił próbę: **bez zmian**. Wynik ostateczny: **boot FAIL / QUIT FAIL**.

**Wniosek empiryczny:** samo ChangeSys(2) jest zgodne z uruchomieniem gry w trybie 0 (G4A), ale w połączeniu z mode 1 (G4B) nie naprawia bootowania ani nie zapewnia powrotu do XMB. Różni się od G3A (bez ChangeSys, mode 1), gdzie boot również nie działał, ale fizyczny QUIT przywracał oryginalne XMB. Nie wyciągać z tego wniosku o konkretnej awarii sprzętowej bez diagnozy ścieżki uruchomieniowej.

**Kolejny etap:** żadnych następnych przypadkowych zmian SCMD. W nowym, odizolowanym teście zidentyfikować dokładny etap bootowania (dostęp DVR FHI, konfiguracja, reset IOP, przejęcie CDVDMAN, odczyt ISO) i zbadać statusy odpowiedzi ChangeSys / NoticeGameStart; bez instalacji na HDD i bez modyfikowania wydania 1.0.

## Izolacja

- Zestaw ma osobne katalogi USB z lokalnymi `BOOT.CNF`. Testowe Managery mogą zapisać swoje `BOOT.CNF` **tylko na USB**, a nie na `PP.DVRISO`; aplikacja zachowuje normalne funkcje ART, których nie używamy w teście.
- `dvr_pfs0:/ISO/` jest wyłącznie źródłem obrazu gry (odczyt).
- Zmiany programu nie dotykają wydania 1.0, tagu, gałęzi HDD, modułów `dvrfile_fhi`, LANG ani EE core.
- Test G4 wymaga odrębnego potwierdzenia na fizycznym DESR-7000; samo GitHub Actions PASS oznacza wyłącznie kompilację i sprawdzenie archiwum.
