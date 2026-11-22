# PDIM XMB GAME INSTALLER 0.2 — samodzielny instalator ELF

**Punkt odniesienia: v0.1 HARDWARE PASS.** Na Sony PSX DESR-7000 użytkownik utworzył skrót Killzone, a gra uruchomiła się z XMB. Wcześniej ręczny skrót ICO też działał.

**Wersja 0.2 nie była jeszcze testowana na konsoli.** Bazuje na sprawdzonym v0.1, ale ma dodatkowe kontrole bezpieczeństwa. [Kod wersji v0.1](https://github.com/adamix95-lab/PSX_DVR_ISO_MANAGER/tree/feature/xmb-game-shortcuts/pdim-xmb-game-installer).

## Jak działa?

Skanuje ISO z `dvr_pfs0:/ISO/`, odczytuje nazwy i Game ID z zapisanych przez PDIM 1.2 indeksów, a okładki pobiera **lokalnie** z `PP.DVRISO/ART`. **Nie łączy się sam z Internetem**: brakujące okładki pobieramy wcześniej funkcją HTTPS w działającym PDIM 1.2.

Po wybraniu gry przygotowuje nową partycję APA/PFS `PP.<TYTUŁ>-<GAMEID>` (128 MiB), lokalne `BOOT.ELF` i `BOOT.CNF`, XMB `EXECUTE.ELF` i `EXECUTE.CNF`, metadane oraz dwie grafiki PNG8. Neutrino, moduły DVR i konfiguracja pozostają **wyłącznie na PP.DVRISO**; właściwe ISO pozostaje w oryginalnym obszarze DVR Sony. Uruchamianie gry nie otwiera PDIM.

## Sterowanie

- **GÓRA/DÓŁ, LEWO/PRAWO**: wybór gry / zmiana strony
- **SELECT**: SYSTEM / ENGLISH — język utrwalony w osobnym `BOOT.CNF`
- **SQUARE**: kontrola celu i pliku ISO **bez zapisu**
- **X**: nowa partycja i pliki — zapis APA/PFS
- **TRIANGLE**: rejestracja skrótu w XMB — zapis sektorów APA
- **O**: wyjście

X i TRIANGLE mają dwa etapy potwierdzania. Ostatecznie wymagają **przytrzymania L1+R1 i naciśnięcia X**.

## Co poprawiono względem v0.1?

1. Instalator przed zapisem ponownie odczytuje **Game ID z wnętrza ISO**. Jeśli stary indeks okładek wskazuje na inną grę, blokuje działanie.
2. Odrzuca niebezpieczne nazwy ISO, których Neutrino FIX95 nie obsługuje, oraz kolizje nazw partycji dwóch gier.
3. Przed utworzeniem nowej partycji w pełni dekoduje obraz PNG i sprawdza, czy jest to poprawna okładka 140×200.
4. Istniejące partycje z inną zawartością nadal **nigdy nie są automatycznie nadpisywane ani formatowane**. Nagłówek XMB jest zapisywany na końcu, po weryfikacji plików.

## Bezpieczeństwo

**Operacje APA mogą uszkodzić system i dane na HDD konsoli Sony PSX.** Wymagana jest pełna, sprawdzona i możliwa do odtworzenia **kopia sektorowa całego dysku**. Nowy ELF nie został przetestowany na fizycznej konsoli. Zalecany pierwszy test to uruchomienie i użycie tylko SQUARE, bez X i TRIANGLE. Nie twórz, nie kasuj i nie formatuj obcych partycji. Nie ma automatycznej naprawy nieudanej instalacji.

Wersja 0.2 nie zawiera jeszcze pobierania metadanych, opisów i screenshotów z PSXDatacenter. Pliki `info.sys` tworzy z nazwy ISO i Game ID, a grafiki PNG8 generuje z istniejącej okładki PDIM. Tę funkcję rozszerzymy osobno.

**Oficjalne wydanie PDIM 1.2 oraz istniejące skróty ICO/Killzone pozostają bez zmian.**
