# G5 — widoczna na ekranie diagnostyka startu Neutrino / DVR ISO

**PSX DVR ISO MANAGER — gałąź `feature/native-quit-igr`.** Wyłącznie USB; nie ma instalatora XMB, nowych partycji ani podmiany stabilnej wersji 1.0.

## Wynik sprzętowy G5A — 2026-11-13

Użytkownik zgłosił: **G5A po próbie uruchomienia wraca do XMB**, zamiast wyświetlić końcowy ekran diagnostyczny `STAGE 199`. Nie zgłoszono, czy którykolwiek STAGE był widoczny pośrednio, dlatego nie przypisujemy awarii konkretnemu etapowi ani nie wnioskujemy, że ISO lub MechaCon zawiodły. **G5A nie przeszedł bramki diagnostycznej. G5B nie powinien być testowany** do czasu potwierdzenia kontroli. Analiza źródła wskazuje, że pierwszy ekran G5A pojawia się dopiero po wczytaniu konfiguracji i modułów; błędy/return przed tym punktem powodują nieinstrumentowany powrót. Dodatkowo `init_scr()` jest wywoływany dla każdego etapu, co zwiększa ingerencję w GS. Potrzebny jest test kontrolny, który wyświetli pierwszy STAGE przy wejściu do `main`, a także przechwyci wszelkie wczesne kody return.

## Cel i aktualne wyniki sprzętowe

| Wersja | Gra uruchamia się | Fizyczny QUIT |
|---|---|---|
| G3B: mode 0, len 1 | **TAK** | Nie reaguje |
| G2B: mode 1, len 1 | Czarny ekran | Powrót do XMB |
| G3A: mode 1, len 4 | Czarny ekran | Powrót do XMB |
| G4A: ChgSys(2) + mode 0 | **TAK** | Nie reaguje |
| G4B: ChgSys(2) + mode 1 | Czarny ekran | Nie reaguje, także po zimnym restarcie |

Zawężamy przyczynę. Tryb `1` umożliwił odzyskanie XMB w G2B/G3A, ale problem występuje podczas próby uruchomienia gry. G5 odpowiada na pytanie: **jak daleko dochodzi Neutrino po zmianie trybu?**

## Co ważne: G5 nigdy nie wystartuje gry — celowo!

Niezależnie od wyniku G5A/G5B, po udanej diagnostyce Neutrino wyświetli **`STAGE 199 — EE handoff reached; ISO/FHI PASS`** i zatrzyma się w bezpiecznej pętli **przed** `ExecPS2`. Nie jest to awaria ani czarny ekran gry: tak ma działać diagnostyka.

Wszystkie kontrolne ekrany mają nagłówek:
`PSX DESR NATIVE QUIT G5A` albo `G5B`,
`ISO LOAD DIAGNOSTIC - GAME WILL NOT START`,
numer STAGE, VALUE, `CDINIT`, `NOTICE RPC`, `NOTICE STATUS`.

Ekran jest rysowany przez EE `libdebug` do GS. Wynik może nie być widoczny, jeśli konsola zatrzyma się wcześniej albo w trakcie przełączania trybu GS; sam czarny ekran nie jest równoznaczny z konkretnym numerem STAGE. Nie ma plikowego logowania ani zapisu na dysk.

## Warianty

| Folder na pendrive | `sceCdApplySCmd(0x29)` | Co porównujemy |
|---|---|---|
| `DESR_G5A/` | `mode=0, len=1` | kontrola odpowiednika działającego G3B |
| `DESR_G5B/` | `mode=1, len=1` | diagnostyka odpowiednika niedziałającego G2B |

Neutrino w obu wariantach jest identycznie instrumentowane. Nie zmieniają się: kolejność resetów IOP, moduły DVR/FHI, `BOOT.ELF`, `ee_core`, systemowa konfiguracja, LANG, ścieżka ISO i kod dostępu do napędu poza parametrem `mode`.

Dodatkowe wyświetlanie przez GS i zatrzymanie przed grą oznaczają, że **G5 jest wyłącznie diagnostyką, a nie reprezentatywnym testem wydajności ani kompatybilności gry**.

## Jak przeprowadzić test

1. Wejdź w GitHub Actions na workflow `Native QUIT G5 - on-screen DVR ISO stage diagnostic USB`. Pobierz artefakt `DESR-QUIT-G5-USB-STAGES-UNTESTED`, w nim `DESR_QUIT_G5_USB_STAGE_PROBES.zip`.
2. Z archiwum przekopiuj **foldery `DESR_G5A` i `DESR_G5B`** do katalogu głównego pendrive'a FAT32. **Nie** podmieniaj `DESR_DVR` ani jakiegokolwiek pliku w `PP.DVRISO` na HDD.
3. W wLE-R3Z uruchom najpierw `mass0:/DESR_G5A/DVR_ISO_MANAGER.ELF` i wybierz tę samą, sprawdzoną grę co w G3B, w trybie **SYSTEM**. Naciśnij X.
4. Oczekiwany wynik G5A: ekran z etapem `199`, a nie faktyczny start gry. Zrób zdjęcie ekranu. **Jeśli G5A nie dochodzi do 199, przerwij: nie testuj G5B**, bo sama instrumentacja może wpływać na start i wymaga korekty.
5. Jeśli G5A pokazuje STAGE 199, uruchom analogicznie `mass0:/DESR_G5B/DVR_ISO_MANAGER.ELF` (po normalnym wyłączeniu i uruchomieniu konsoli). Wybierz tę samą grę, także SYSTEM, naciśnij X.
6. Zrób zdjęcie **ostatniego widocznego STAGE w G5B**. Jeżeli widać `199`, to Neutrino potrafi załadować ISO i przygotować EE core również przy `mode=1`; błąd musi pojawiać się dopiero po tej fazie (lub być wywołany przez samą zmianę diagnostyczną). Jeśli utknie wcześniej, numer wskazuje następny punkt badań.

**Nie naciskaj QUIT podczas zapisu save/kopiowania gier/aktualizacji indeksu.** W diagnostyce G5 nie ma potrzeby testować QUIT — badamy etapy uruchamiania i zależy nam na zdjęciu ostatniego etapu. Z testu wychodzi się normalnym restartem/wyłączeniem konsoli, gdy nie trwają zapisy danych.

### Mapa etapów

| STAGE | Oznaczenie |
|---|---|
| 10–12 | Wejście w LE i reset IOP do `rom0:OSDCNF` |
| 20–22 | `sceCdInit`, komenda `0x29`, wynik RPC i MechaCon |
| 30–31 | Drugi reset IOP do środowiska Neutrino |
| 40–42 | Uruchamianie modułów LE / DVR |
| 50–51 | Inicjalizacja FHI, `VALUE=1` oznacza aktywny FHI |
| 60–62 | Próba otwarcia wskazanego ISO w `dvr_pfs0:/ISO/` |
| 63–66 | Rozmiar ISO i poprawność nagłówka PVD |
| 68–69 | Podpięcie otwartego ISO do FHI |
| 80 | Podstawowe sprawdzenie ISO i FHI zakończone |
| 90–91 | Odczyt `SYSTEM.CNF` z obrazu gry |
| 199 | Neutrino doszło do uruchomienia EE core; **celowe zatrzymanie, bez startu gry** |
| 201, 241, 260–268, 290 | Błąd i celowe zatrzymanie diagnostyki |

`VALUE` pokazuje odpowiedź lub indeks zależnie od etapu. `NOTICE RPC` powinno wynosić 1 przy udanym transporcie; `NOTICE STATUS` najlepiej 0, ale wniosek o samym QUIT wymaga osobnego testu sprzętowego.

## Bezpieczeństwo i ograniczenia

- Program startuje z dwóch odseparowanych folderów USB, korzystając z oficjalnych plików wydania 1.0 jako bazowego runtime. Testowe Managery zapisują `BOOT.CNF` **wyłącznie w swoich katalogach USB**.
- G5 Neutrino tylko **czyta** wybrany obraz ISO; nie modyfikuje `dvr_pfs0:/ISO/`, `PP.DVRISO`, Sony system partitions, zawartości HDD ani nagłówków XMB.
- Nie wymieniamy ani nie przebudowujemy `dvrfile_fhi.irx`, `ee_core.elf`, LANG i `BOOT.ELF`.
- G5 może zatrzymać się przed końcową planszą, a `libdebug` może zmienić zachowanie obrazu — to nadal nie jest gotowa poprawka. Ostatni widoczny etap trzeba interpretować ostrożnie.
- Paczka jest oznaczona **UNTESTED**, dopóki nie zostanie sprawdzona na fizycznym DESR-7000.

## Proszę o wynik

Wystarczą **dwa zdjęcia**, G5A i G5B, wraz z nazwą testowanej gry. Jeśli któryś test da czarny ekran, napisz, czy był bezpośrednio po naciśnięciu X, czy wcześniej pokazał się choć na moment ekran G5.
