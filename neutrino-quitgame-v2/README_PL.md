# PR #9 — Sony PSX DESR-7000: QUIT GAME v2 (diagnostyka, NIEPRZETESTOWANE)

## Cel i źródło problemu

PR #8 skompilował się, ale na fizycznym **DESR-7000 wyświetlał czarny ekran** — gra się nie uruchamiała.
PR #8 wysyłał sprzętowe powiadomienie `SCMD 0x29` (argument `1`) zaraz po
istniejącym powiadomieniu `0`, **zanim Neutrino otworzyło ISO z DVR**.
Mogło to zmienić stan DVRP/Mechacon, jednak **nie mamy dowodu, że właśnie to
spowodowało czarny ekran**. Trzeba ustalić etap awarii.

Wersja v2 **nie dotyka oryginalnej sekwencji inicjalizacji DVR Neutrino PR #228**.
Eksperyment jest osobnym, domyślnie wyłączonym wywołaniem przed `ExecPS2`,
dopiero gdy Neutrino otworzyło obraz ISO i przygotowało tabele FHI oraz EE core.

Nie implementujemy resetu z kontrolera ani niczego w XMB, NVRAM i partycjach.

## Oddzielne pliki do skopiowania na USB

Z artefaktu workflow `DESR7000-Neutrino-QUIT-v2-Staged-UNTESTED` **dodaj**
(dopisz do istniejących folderów, niczego nie nadpisuj):

```text
mass0:/DESR_DVR/neutrino_quit2.elf
mass0:/DESR_DVR/modules/ee_core_quit2.elf
mass0:/DESR_DVR/config/quit2.toml
```

**Nie zamieniaj istniejących** `neutrino.elf`, `neutrino_lang.elf`,
`ee_core.elf`, `ee_core_lang.elf`, `osdlang.toml`, `system.toml` ani
`BOOT.ELF`. PR #7 MONO i PR #6 v0.2 pozostają bez zmian.

Nie modyfikujemy Managera, który przy X generuje swój BOOT.CNF.
**W trakcie testów startuj ręcznie BOOT.ELF z wLE-R3Z**, nie przez X w Managerze.

## Kolejność testów: przerwij przy pierwszej awarii

Zachowaj **działający BOOT.CNF na komputerze**, nie testuj przy kopiowaniu ISO
ani podczas zapisu stanu gry. Przywracaj normalny `BOOT.CNF` po każdym
etapie lub po przerwaniu eksperymentu. Wszelkie `TEST_CNFS` są czystym ASCII.

**Etap 0 — CONTROL:**
Skopiuj `TEST_CNFS/BOOT_REZ_0_CONTROL.CNF` jako `mass0:/DESR_DVR/BOOT.CNF`.
Uruchom `BOOT.ELF`. Ten plik używa **nowego testowego ELF i EE core**,
ale **nie zawiera żadnej flagi QUIT GAME**. Sprawdź, czy REZ startuje i czy
ISO działa. Jeśli nie — NIE przechodź dalej! Sprawdź nazwy plików i ścieżki.

```ini
boot=neutrino_quit2.elf
arg=-bsd=dvr
arg=-dvd=dvr_pfs0:/ISO/REZ.iso
arg=-cfg=quit2
```

**Etap 1 — PROBE:**
Dopiero po sukcesie etapu 0 użyj `BOOT_REZ_1_PROBE.CNF`.
Dodatkowy argument `-quitgame=probe` wywołuje tylko
`sceCdInit(SCECdINoD)` oraz `sceCdInit(SCECdEXIT)` tuż przed uruchomieniem
gry. **Nie wysyła powiadomienia QUIT.**
Tekst diagnostyczny i krótkie pauzy (2 s) pozwalają zobaczyć ostatni etap.
Jeśli program się zawiesi, pojawi się czarny ekran lub gra nie czyta ISO,
przerwij test — nie przechodź do etapu 2.

**Etap 2 — LATE:**
Dopiero jeśli etapy 0 i 1 działają, uruchom `BOOT_REZ_2_LATE.CNF`.
Parametr `-quitgame=late` używa tego samego RPC, ale dodatkowo wysyła
`sceCdApplySCmd(0x29, {1}, 1, out)` **po otwarciu ISO**, zanim Neutrino odda
sterowanie grze. Używamy jednego bajtu wejścia, jak w implementacji PS2SDK
`sceCdNoticeGameStart(1)`. Próba jest tylko jedna; błąd powiadomienia
zatrzymuje dalszy start. Sprawdź, czy gra uruchamia się i czy odczytuje ISO.
Tylko jeśli gra faktycznie działa, możesz sprawdzić sprzętowy QUIT GAME,
bez aktywnego zapisu.

## Interpretacja wyników

| Wynik | Wniosek |
| --- | --- |
| Etap 0 nie działa | Możliwy problem z konfiguracją/testową kompilacją; bez diagnozowania QUIT |
| Etap 0 działa, etap 1 nie | Problem może powodować sama późna inicjalizacja CDVD RPC |
| Etap 1 działa, etap 2 nie | Problem wiąże się z późnym powiadomieniem QUIT / zmianą stanu |
| Etap 2 działa, QUIT nie działa | Możliwy problem z firmware/wake reason/DEV9 albo obsługą stanu game-start |
| Etap 2 działa i QUIT wraca do XMB | Sukces testowy, nadal wymagany powtórny test i kontrola ponownego startu ISO |

**Zapisz, jaki był ostatni widoczny komunikat `QUIT v2: ...`** oraz
czy czarny ekran pojawia się **przed tekstem Neutrino czy po nim**.

Ważne: wywołanie `sceCdInit` w LE może samo w sobie zawiesić się na
sprzęcie; test PROBE temu właśnie służy. Każda modyfikacja mechanizmu QUIT
może nie być kompatybilna z użyciem DVRP jako stałego nośnika ISO.
Sukces kompilacji nie dowodzi zgodności sprzętowej.

## Dlaczego v2 jest inny niż PR #8

- PR #8: `mode 0` → `mode 1` **przed** resetem IOP i otwarciem ISO.
- v2: niezmienione `mode 0` → reset IOP → otwarcie ISO → przygotowanie FHI →
  w zależności od flagi: żadnych zmian / sam CDVD RPC / późny `mode 1`.
- v2 na razie **nie dokłada poprawki językowej v0.2** do binarki testowej.
  Chcemy zbadać tylko zachowanie QUIT na oryginalnym trybie SYSTEM.
- v2 to **osobny eksperymentalny PR**, bez zmian w stabilnym `main`,
  bez zamykania lub scalania PR #8.

## Źródła

- Specyficzna inicjalizacja PSX OSDMenu: https://github.com/pcm720/OSDMenu/blob/main/common/src/psxinit.c
- PS2SDK SCMD `sceCdNoticeGameStart`: https://github.com/ps2dev/ps2sdk/blob/master/ee/rpc/cdvd/src/scmd.c
- Neutrino PR #228: https://github.com/ps2max32/neutrino/pull/228
- PSX OSDSYS init: https://github.com/ps2homebrew/OSD-Initialization-Libraries/blob/main/osd/main.c
- Nieudany PR #8: https://github.com/adamix95-lab/DESR7000-DVR/pull/8
