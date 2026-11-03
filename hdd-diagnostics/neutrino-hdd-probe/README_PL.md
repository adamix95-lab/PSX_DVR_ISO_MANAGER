# DIAG V — Neutrino pcm720 z kontrolowanym montowaniem HDD (test bez gry)

**Stan: eksperymentalny, nieprzetestowany na sprzęcie.** Źródło: pinned publiczny `pcm720/neutrino` z `bsd-dvr`, commit `aa8d80a1a0b5323cb407c814c0568c3489597c1c`; upstream PS2 `ps2max32/neutrino` pozostaje nietknięty. OSDMenu `BOOT.ELF` i wariant USB v0.8 pozostają bez zmian.

## Dlaczego

Na prawdziwym PSX DESR-7000 **DIAG U zakończył się pełnym PASS** po uruchomieniu rzeczywistego OSDMenu z `PP.TESTDVR` i argumentami: `ARGC=3 ARGS_OK=1`; `PFS_PRE=-19` / `HDD=2`, po ponownym montowaniu `MOUNT_RO=0`, `PFS_AFTER=3`, `RPC CONFIG=3`, `DVR IRX=3`, `CHDIR=0`, `NEW CWD=pfs0:/`, `POSIX CFG=3 ERRNO=0`, `POSIX IRX=3 ERRNO=0`, `UMOUNT OWN=0`. To sprzętowo potwierdza możliwość *przywrócenia* odczytu `config/` i `modules/` po tym, jak OSDMenu odmontował `pfs0:`.

Dotychczasowy `BOOT.CNF` uruchamiający `neutrino.elf` (spatchowana gałąź pcm720 z nowymi modułami DVR) wracał do XMB, nawet gdy na testowym HDD pojawiły się `config/`, `modules/` i końcowe `-dev9=NICHDD`. DIAG V sprawdza **kolejny etap: oryginalne wczytywanie konfiguracji i modułów Neutrino z HDD**.

## Zawartość

- `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF` — osobna, zbudowana z `pcm720/neutrino:bsd-dvr` diagnostyczna wersja prawdziwego Neutrino. Nie jest produkcyjnym `neutrino.elf`. Na wejściu dopuszcza wyłącznie `hdd0:PP.TESTDVR:pfs:/...`, dokładnie `-bsd=dvr` i **fikcyjną** ścieżkę `-dvd=dvr_pfs0:/ISO/DIAG_V_NO_GAME.iso`.
- `DIAG_V.CNF` — ścisła alternatywna konfiguracja z poprawną końcową flagą OSDMenu `-dev9=NICHDD`, adresująca wyłącznie `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF`.
- `PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF` — odpalany z UDPFS w wLE-R3Z, używa realnego oryginalnego OSDMenu `BOOT.ELF` i sprawdza alternatywny `DIAG_V.CNF` byte-for-byte (dopuszczalny wyłącznie końcowy LF lub NUL, jak w DIAG R3/T/U).

## Dokładnie co robi Neutrino V

1. Uruchamia debugowy ekran i potwierdza argumenty; nie kontynuuje, jeśli argumenty/partycja różnią się od dozwolonych.
2. Korzysta z `SifInitRpc(0)`, `fileXioInit()`; próbuje zamontować **tylko** `hdd0:PP.TESTDVR` pod `pfs0:` z `FIO_MT_RDONLY`, potem `chdir("pfs0:/")`.
3. Otwiera *tylko do odczytu* `config/system.toml` i `modules/dvrfile_fhi.irx`; drukuje wyniki.
4. Wywołuje prawdziwy inicjalizator Neutrino z forka pcm720 — etap ROMVER, `system.toml`, argumenty `-bsd=dvr`, zależności `bsd-dvr.toml`, inne konfiguracje, `ee_core.elf` i binaria modułów `*.irx` wczytywane do pamięci. Widoczne są markery faz `V10`, `V20`, `V30`, `V40`, `V45`, `V50`, `V60`.
5. **Bezwzględny stop po `modlist_load`** na `V70 MODULES LOADED, SAFE STOP`. W testowym buildzie *nie dochodzi* do przełączania IOP w tryb DVR, `sceCdApplySCmd`, tworzenia `dvrfile_fhi`, otwierania obrazu gry ani `ExecPS2`.
6. Raport `V RESULT=... LAST_STAGE=...` utrzymuje się około 45 sekund, następnie kod odmontowuje wyłącznie własny PFS i wraca. Gdy przedwczesny błąd wystąpi w fazie źródłowego Neutrino, wrapper również pokazuje `LAST_STAGE` i końcowy wynik.

`V70` z `V RESULT=0` oznacza tylko poprawne **wczytanie konfiguracji i modułów**, nie uruchomienie gry ani potwierdzenie wszystkich operacji DVR. `LAST_STAGE` poniżej 70 pomaga wskazać nieudany etap.

## Instrukcja po pomyślnym GitHub CI

1. **Nie nadpisywać** oryginalnych `BOOT.ELF`, `BOOT.CNF`, `neutrino.elf`, `config/`, `modules/`, ISO, wariantu USB v0.8 ani partycji Sony.
2. Na testową `PP.TESTDVR` skopiować **tylko** `PSX_DVR_HDD_NEUTRINO_V_PROBE.ELF` i `DIAG_V.CNF` obok oryginalnych plików.
3. `PSX_DVR_HDD_NEUTRINO_V_LAUNCH.ELF` uruchomić z UDPFS przez wLE-R3Z. Przy `CHECK=0` zrobić zdjęcie pierwszego ekranu, nacisnąć X. Przy `CHECK !=0` X jest nieaktywny.
4. Zrobić zdjęcie końcowego ekranu z `V RESULT` i `LAST_STAGE` (oraz ostatniego markera, jeśli nastąpi zawieszenie).

**Bezpieczeństwo:** kod V nie tworzy ani nie zapisuje plików. Montowanie jest wyłącznie RO, dopuszczalna jest tylko odseparowana partycja testowa. Inicjalizowanie istniejącego zewnętrznego IOP i modułów może mieć niezależne skutki; zalecana jest kopia ważnych danych. Przed etapem rzeczywistego uruchamiania gry wymagany jest osobny, świadomy test. PR #26 pozostaje Draft, `main` i v0.8 USB nietknięte.
