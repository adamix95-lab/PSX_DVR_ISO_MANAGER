# G5R — diagnoza nieoczekiwanego powrotu G5A do XMB

**Sony PSX DESR-7000, PSX DVR ISO MANAGER. Tylko USB. Stan: UNTESTED na fizycznej konsoli.**

## Dlaczego G5R zamiast G5B?

W rzeczywistym teście **G5A wrócił do XMB**, zamiast osiągnąć oczekiwane `STAGE 199`. Nie mamy informacji, czy wcześniej był choć jeden widoczny etap, dlatego nie wolno uznawać tego wyniku za błąd ISO/MechaCon.

Analiza starego G5A wykazała: **pierwszy ekran dopiero po wczytaniu ROMVER, wszystkich plików konfiguracyjnych i modułów**. Jeżeli Neutrino zwróciło błąd wcześniej, nie mógł zostać pokazany żaden kod. Co więcej `init_scr()` było uruchamiane przy każdym punkcie, co mogło zakłócać diagnostykę GS.

G5R to pojedynczy bezpieczny wariant kontrolny `mode=0, len=1` (ten sam co G5A/G3B), z dodatkowymi zabezpieczeniami:

1. **`STAGE 1` widoczne zaraz po wejściu do Neutrino** — przed próbą odczytu ROMVER i konfiguracji. Wskazuje, że testowy Neutrino rzeczywiście został uruchomiony przez OSDMenu.
2. **Fazy 2–9** zapisywane jako lekkie zmiany licznika w RAM przy czytaniu ROMVER, system.toml, argumentów, compat.toml, BSD DVR, sterownika DVD, EE core i modułów — nie zapisują danych na USB/HDD i nie zmieniają liczby resetów IOP.
3. **`STAGE 298` przechwytuje dowolny normalny powrót z funkcji Neutrino**, pokazując `VALUE` (kod powrotu) i `LAST PHASE BEFORE STOP` (ostatnia faza). Zamiast zwyczajnie wrócić do XMB, wyświetla ten wynik i zatrzymuje się.
4. Po pierwszym `init_scr()` diagnostyka wykonuje tylko czyszczenie i kolejne napisy, bez ponownej inicjalizacji GS dla każdej fazy.
5. Dotychczasowe etapy 10–199 (CDVD/SCMD, IOP reset, LE, FHI, ISO, SYSTEM.CNF) zostają zachowane. **`STAGE 199` oznacza celowe zatrzymanie PRZED uruchomieniem gry**, nie awarię.

Nie dodano `ChgSys(2)`, nowego SCMD, dodatkowego resetu IOP, podmiany modułu FHI/EE core ani NULD. Kontrolny G5R **nigdy nie wystartuje gry**.

## Test sprzętowy

1. Z workflow `Native QUIT G5R - early Neutrino entry and return diagnostics USB` na GitHub Actions pobierz **`DESR-QUIT-G5R-EARLY-RETURN-USB-UNTESTED`**, a w nim `DESR_QUIT_G5R_EARLY_RETURN_USB.zip`.
2. Wypakuj folder **`DESR_G5R/`** do głównego katalogu pendrive FAT32. **Nie podmieniaj** `DESR_DVR/`, `DESR_G5A/`, `DESR_G5B/` ani żadnego pliku w `PP.DVRISO`.
3. Uruchom `mass0:/DESR_G5R/DVR_ISO_MANAGER.ELF` przez wLE-R3Z. Nagłówek musi zawierać `1.0 USB G5R`.
4. Wybierz tę samą grę z DVR ISO co w G5A, w trybie **SYSTEM**, naciśnij X.
5. Zanotuj, czy na ekranie pojawiło się chociaż na chwilę **`PSX DESR NATIVE QUIT G5R / USB ONLY`**, oraz ostatni widoczny numer STAGE. Zrób zdjęcie, najlepiej z `VALUE` i `LAST PHASE BEFORE STOP`.

## Jak interpretować?

| Wynik | Co oznacza |
| --- | --- |
| **STAGE 298**, VALUE = kod ujemny / `LAST PHASE` | Neutrino zostało uruchomione i wróciło z błędem. Ostatnia faza pozwoli zawęzić przyczynę |
| **STAGE 199** | Loader dotarł do EE handoff. To celowy STOP, gra nie wystartuje |
| STAGE 1, potem powrót do XMB | Neutrino wystartowało, ale nastąpiło nietypowe przejście lub awaria, której wrapper nie przechwycił |
| Bez napisu G5R, natychmiastowy powrót do XMB | Brak dowodu, że wykonano testowy Neutrino — następny krok to audyt OSDMenu / BOOT.CNF / ścieżek |
| Inny STAGE 10–91 lub 201–290 | Ostatni widoczny etap lub błąd w określonej części IOP/FHI/ISO |

**Uwaga:** Nie każdy czarny ekran oznacza zatrzymanie w danym STAGE — obraz GS może zostać wyłączony przez reset lub błędne uruchomienie. Podawaj tylko etap, który faktycznie widziałeś.

Test nie wymaga użycia przycisku QUIT. Po fotografii, gdy nie odbywa się zapis danych, można normalnie wyłączyć konsolę. **Nie przechodź do G5B**, dopóki nie wiemy, dlaczego G5A wracał do XMB.

## Izolacja i kontrola plików

- Oryginalne `BOOT.ELF`, `neutrino_lang.elf`, `modules/*`, `config/*` i `BOOT.CNF` pochodzą z oficjalnego USB 1.0, sprawdzonego SHA-256.
- Zmienione są tylko `DESR_G5R/DVR_ISO_MANAGER.ELF` (osobny folder USB) i `DESR_G5R/neutrino.elf` (diagnostyka ekranowa).
- Testowy Manager zapisuje swój `BOOT.CNF` wyłącznie na USB. Neutrino otwiera ISO w `dvr_pfs0:/ISO/` do odczytu; brak plikowych logów.
- Niczego nie instalujemy do XMB, na `PP.DVRISO` ani w stabilnym wydaniu 1.0.
