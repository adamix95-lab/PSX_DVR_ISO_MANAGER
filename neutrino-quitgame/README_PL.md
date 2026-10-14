# PR #8 — DESR QUIT GAME (EKSPERYMENT, NIE TESTOWANY NA KONSOLI)

To **osobna wersja Neutrino**, nie nowa wersja GUI Managera. PR #7 MONO pozostaje niezależny, nietknięty.
Nie scalaj PR #8 przed testem na fizycznym Sony PSX DESR-7000.

## Hipoteza i dlaczego nie robimy IGR z pada

PSX DESR ma sprzętowy przycisk QUIT GAME; w kodzie PSX OSDSYS launchery wykonują
`sceCdNoticeGameStart(1, &status)` do włączenia przycisku.
W PS2SDK funkcja ta sprowadza się do komendy SCMD 0x29 (argument 1).
Uwaga: w innych bibliotekach spotykany numer 0x2F to ID usługi RPC, a nie
ten sam niskopoziomowy numer SCMD. Nie należy ich zamieniać.

Neutrino z DVR PR #228 wykonuje już SCMD 0x29 z argumentem 0, by przygotować
dostęp do DVR, następnie resetuje IOP i otwiera ISO na DVRP. Eksperyment
wysyła **dodatkowe 0x29 z argumentem 1 po istniejącym 0**, przed resetem
IOP i przed otwarciem ISO. To tylko HIPOTEZA: tryb 1 może unieważnić
dostęp do DVR i spowodować, że ISO się nie uruchomi. Nie jest potwierdzone,
że QUIT wróci do XMB z gry uruchomionej przez Neutrino.

## Dodatkowy opis działania QUIT GAME (informacja ze społeczności PSX)

- Jeśli flaga `NoticeGameStart` nie jest ustawiona, QUIT jest ignorowany.
- Jeśli DEV9 jest zasilany, firmware ustawia flagę: zamiast żądania wyłączenia
  ma zostać wykonany reset; następnie uruchamia zwykłą procedurę wyłączania.
- Jeśli DEV9 nie jest zasilany, następuje reset bez tej sekwencji.
- OSDSYS sprawdza `wake-up reason`, rozpoznaje QUIT GAME i pomija intro.

Wniosek: ta funkcja zależy od **oryginalnego firmware DVRP/Mechacon/OSDSYS**;
nie jest zwykłym programowym `LoadExecPS2`. Samo zgłoszenie `notice(1)` nie
gwarantuje, że powrót się uda. Szczególnie ważny jest stan DEV9 podczas
odczytu ISO z `__xcontents`; QUIT testujemy wyłącznie gdy gra niczego nie zapisuje.
## Zabezpieczenia

- Wykonanie nowej komendy wymaga argumentu `-quitgame=1` **oraz** `-bsd=dvr`.
- Opcja wymaga normalnego trybu ładowania (bez `-qb`). Dla innych opcji Neutrino nie zmienia zachowania.
- Komenda jest wywoływana maksymalnie pięć razy (przy odpowiedzi busy);
  brak sukcesu zatrzymuje test przed otwarciem obrazu ISO.
- Nie zmieniamy managera, `BOOT.ELF`, partycji DVR, APA, XMB ani NVRAM.
- Instalowane są trzy **dodatkowe** pliki na USB, bez nadpisywania działających Neutrino.
- Osobny `ee_core_quit.elf` jest skompilowany razem z poprawką językową v0.2;
  obsługuje opcjonalnie `-osdlang=en`, ale pierwszy test powinien być w SYSTEM.

## Instalacja

Zachowaj aktualną działającą konfigurację `BOOT.CNF` również na komputerze.
Z artefaktu `DESR7000-Neutrino-QUIT-GAME-Experimental-UNTESTED` dodaj:

    mass0:/DESR_DVR/neutrino_quit.elf
    mass0:/DESR_DVR/modules/ee_core_quit.elf
    mass0:/DESR_DVR/config/quit.toml

NIE NADPISUJ:

    mass0:/DESR_DVR/neutrino.elf
    mass0:/DESR_DVR/neutrino_lang.elf
    mass0:/DESR_DVR/modules/ee_core.elf
    mass0:/DESR_DVR/modules/ee_core_lang.elf
    mass0:/DESR_DVR/config/system.toml
    mass0:/DESR_DVR/config/osdlang.toml
    mass0:/DESR_DVR/BOOT.ELF

Nie usuwaj ani nie zastępuj całych katalogów modules/ czy config/.

## Jak przetestować QUIT (na początek REZ, bez Managera)

1. Zatrzymaj kopiowanie plików i nie testuj QUIT w trakcie zapisu gry.
2. Upewnij się, że oryginalne Neutrino + v0.2 i Manager nadal działają.
3. Zrób własną kopię BOOT.CNF na PC. W artefakcie znajduje się
   `TEST_CNFS/BOOT_QUIT_REZ_SYSTEM.CNF` (zwykły ASCII, bez BOM).
4. Skopiuj tę przykładową konfigurację ręcznie jako
   `mass0:/DESR_DVR/BOOT.CNF` (tylko podczas eksperymentu).
5. **Uruchom `mass0:/DESR_DVR/BOOT.ELF` ręcznie przez wLE-R3Z.**
   Nie wybieraj X w Managerze, bo nadpisze BOOT.CNF konfiguracją bez QUIT.
6. Sprawdź, czy REZ uruchomi się do menu, następnie czy dane ISO nadal są
   odczytywane podczas gry. Daj mu przynajmniej minutę, bez zapisywania.
7. Gdy gra działa i niczego nie zapisuje, naciśnij **fizyczny przycisk QUIT GAME**.
   Sprawdź, czy nastąpi powrót do japońskiego XMB, czy tylko restart, czy brak reakcji.
8. Jeśli ISO się nie uruchomi, ekran zawiesi się albo pojawi się komunikat
   błędu, zakończ test, zrestartuj konsolę i przywróć działający BOOT.CNF z PC.

Przykładowy CNF (SYSTEM):

    boot=neutrino_quit.elf
    arg=-bsd=dvr
    arg=-dvd=dvr_pfs0:/ISO/REZ.iso
    arg=-cfg=quit
    arg=-quitgame=1

Dla sprawdzenia angielskiego w tej samej paczce jest
`TEST_CNFS/BOOT_QUIT_REZ_ENGLISH.CNF`, dodatkowo zawierający `arg=-osdlang=en`.
Na pierwszy test NIE używaj wariantu ENGLISH, żeby nie mieszać zmiennych.

## Warunki zaliczenia testu

- **A:** REZ nadal startuje i czyta ISO z DVR (najważniejsza regresja).
- **B:** QUIT GAME działa z gry i wraca do XMB w sposób powtarzalny.
- **C:** ponowny start i wybór ISO po QUIT nadal działa; brak naruszenia DVR.

Jeśli A nie przejdzie, PR #8 nie jest gotowy i trzeba znaleźć inne miejsce
wywołania komendy. Nie wdrażamy tego na siłę.

## Punkty odniesienia

- Neutrino DVR PR #228: https://github.com/ps2max32/neutrino/pull/228
- Sony PSX OSDMenu: https://github.com/pcm720/OSDMenu/blob/main/common/src/psxinit.c
- PS2SDK: https://github.com/ps2dev/ps2sdk/blob/master/ee/rpc/cdvd/src/scmd.c
- OSD initialization PSX: https://github.com/ps2homebrew/OSD-Initialization-Libraries/blob/main/osd/main.c
- LUNA IGR (alternatywa bardziej złożona): https://github.com/dnunezx/neutrino-luna
