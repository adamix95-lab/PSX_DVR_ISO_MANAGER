# Fizyczny QUIT GAME — osobna próba naprawy Neutrino DVR

Stan: poprawka źródłowa i kompilacje są weryfikowane osobno; działanie przycisku
w grze wymaga testu na prawdziwym DESR. Nie jest to potwierdzona naprawa sprzętowa.

Gałąź: `feature/neutrino-desr-igr`. Baza Neutrino:
`aa8d80a1a0b5323cb407c814c0568c3489597c1c` (pcm720 DVR PR228).
Ta próba nie zawiera wcześniejszego prototypu przechwytywania pada z LUNA.

## Ustalenia i zakres

1. OSDMenu `switchPSX()` wywołuje `custom_sceCdNoticeGameStart(1, &stat)`
   w sekcji opisanej jako włączenie przycisku wyjścia. Czeka na wynik i sprawdza
   bit błędu/busy. Neutrino PR228 wysyłało bez sprawdzenia `0x29` z czterema
   bajtami zerowymi. PS2SDK EE libcdvd koduje parametr tego SCMD jako jeden bajt.
2. Stare wywołanie Neutrino znajdowało się tylko wewnątrz `!QuickBoot`.
   Wersja HDD wykorzystuje QuickBoot i mogła odziedziczyć ustawienie z Managera,
   który także wywołuje `sceCdNoticeGameStart(0, ...)`. Nie mamy pomiaru stanu
   Mechacon na konsoli, więc nie uznajemy tego za ostateczny dowód przyczyny.
3. Nowy loader wysyła tryb `1`/jeden bajt, wymaga odpowiedzi transportu `1`
   i statusu `0`, ogranicza ponawianie do 16 wywołań i raportuje błąd `-97`
   zamiast twierdzić, że przycisk został włączony.
4. Robi to w oryginalnym miejscu przed LE oraz ponownie po przygotowaniu LE,
   także dla QuickBoot, przed przejściem w emulowane środowisko gry.
   Nie dodaje nowych resetów IOP. Samo wywołanie RPC może czekać na usługę ROM;
   limit 16 ogranicza liczbę prób, nie gwarantuje limitu czasu zawieszonego RPC.
5. Bez zmian pozostają FIX95, wybór ISO, montowanie PP.DVRISO tylko do odczytu,
   DVR FHI, OSDLauncher, moduły emulacji oraz działający hook języka angielskiego.
   Warianty HDD mają dodatkowe nazwy dozwolone w sprawdzeniu `argv[0]`.

Źródła:
- https://github.com/pcm720/OSDMenu/blob/a2b847a2d1b319483fe1fa84d4dc14493b7bbc6f/common/src/psxinit.c
- https://github.com/ps2dev/ps2sdk/blob/d41ea62946e53d9a0c8788bb9eb8c8faf488b559/ee/rpc/cdvd/src/scmd.c
- https://github.com/ps2max32/neutrino/blob/aa8d80a1a0b5323cb407c814c0568c3489597c1c/ee/loader/src/main.c

## Pliki testowe i wymagania

Paczka zawiera cztery dodatkowe loadery i dwa osobne Managery testowe.
Nie trzeba zmieniać nazw oryginalnych ELF-ów ani wgrywać nowych modułów:

| Miejsce startu Managera | Język | Loader testowy |
| --- | --- | --- |
| USB | SYSTEM | `neutrino_quit.elf` |
| USB | ENGLISH | `neutrino_quit_lang.elf` |
| HDD | SYSTEM | `neutrino_hdd_quit.elf` |
| HDD | ENGLISH | `neutrino_hdd_quit_lang.elf` |

Każdy dalej uruchamia obrazy z `dvr_pfs0:/ISO/`. USB/HDD oznacza miejsce
aplikacji i konfiguracji. Wymagane są dotychczasowe sprawne `config/` i `modules/`;
ENGLISH używa istniejącego, sprawdzonego `config/osdlang.toml` oraz
`modules/ee_core_lang.elf`. Pusty ELF uruchomiony bez argumentów nie wybiera gry.

## Test na DESR

1. Dla USB skopiuj trzy pliki z `USB/` do dotychczasowego `mass0:/DESR_DVR/`.
   Dla HDD skopiuj trzy pliki z `HDD/` do katalogu głównego `PP.DVRISO`.
   Pozostaw oryginalne Managery, Neutrino, `BOOT.ELF`, `config/` i `modules/`.
2. W wLE uruchom `DVR_ISO_MANAGER_USB_QUIT.ELF` albo
   `DVR_ISO_MANAGER_HDD_QUIT.ELF`. Nagłówek musi zawierać `QUIT TEST`.
3. Wybierz grę, ustaw SYSTEM lub ENGLISH jak zwykle i naciśnij X. Testowy
   Manager korzysta z dotychczasowej ścieżki OSDMenu/FIX95, w tym jawnego
   argumentu `hdd0:PP.DVRISO/BOOT.CNF` na HDD. Zmienia jedynie nazwy loaderów
   wpisywanych do normalnego `BOOT.CNF`. Nie edytuj konfiguracji ręcznie.
4. Po pełnym uruchomieniu gry naciśnij raz fizyczny QUIT GAME. Nie naciskaj go
   podczas zapisywania stanu gry na karcie pamięci. Oczekiwany wynik: XMB.
5. Podaj grę, USB/HDD, SYSTEM/ENGLISH, czy gra wystartowała, oraz wynik QUIT:
   XMB / brak reakcji / czarny ekran / inne. Jeśli loader pokaże błąd `-97`,
   sfotografuj również `DESR QUIT: ... rpc=... status=...`.
6. Aby wrócić do sprawnej wersji, uruchom oryginalny Manager i wybierz grę X.
   Odtworzy on `BOOT.CNF` z nazwą oryginalnego Neutrino. Same testowe pliki
   można później usunąć. Do tego czasu samodzielny `BOOT.ELF` będzie korzystał
   z ostatniego wpisu w `BOOT.CNF`, czyli może nadal wskazywać loader testowy.

Dodatkowe Managery są budowane z bazy gałęzi HDD, z jedyną zmianą nazw
Neutrino oraz nagłówka `QUIT TEST`. Nie zawierają poprawki HDD NETFIX; test
QUIT nie sprawdza pobierania okładek. Jak normalny Manager, mogą aktualizować
indeks i zapisywać `BOOT.CNF`; to nie są aplikacje tylko do odczytu.

Nie trzeba pilota ani kombinacji przycisków pada. Jeśli tryb `1` zostanie
zaakceptowany, lecz fizyczny przycisk nadal nie zadziała, kolejnym etapem będzie
ustalenie, czy stan zmienia reset do emulacji EE. Nie dokładamy na razie
niepotwierdzonej obsługi zdarzeń przycisku jako zamiennika funkcji sprzętowej.

## Weryfikacja

`tests/test_command.c` uruchamia prawdziwy helper z atrapami usług sprzętowych:
sprawdza kod komendy, wartość i długość parametru, obsługę busy i błędu
transportu, odrzucenie przez Mechacon, ograniczenie prób i błąd inicjalizacji.
To nie emuluje sprzętowego QUIT ani powrotu do XMB.

Workflow `build-neutrino-desr-quit.yml` buduje cztery loadery w
`ps2max/dev:v20260228`, sprawdza ELF32/MIPS, komunikaty i zachowanie FIX95,
a dla ENGLISH kompiluje również niezmieniony core językowy jako kontrolę.
Osobne zadanie buduje oba testowe Managery i sprawdza nowe cele `boot=`
oraz zachowanie jawnego argumentu CNF dla HDD.
Wynik konkretnego uruchomienia CI należy sprawdzić przed użyciem artefaktów.
